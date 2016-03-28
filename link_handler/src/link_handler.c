/**
 * @file link_handler.c
 * @version 0.2.0
 *
 * @section License
 * Copyright (C) 2014-2016, Erik Moqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * This file is part of the Simba project.
 */

#include "simba.h"
#include "link_handler.h"

/** Message types. */
#define TYPE_PAYLOAD     1
#define TYPE_PING        2
#define TYPE_PONG        3

struct link_handler_header_t {
    int32_t type;
    int32_t size;
};

static int read_message(struct link_handler_t *self_p)
{
    int res;
    struct link_handler_header_t header;

    res = chan_read(&self_p->supervisor, &header, sizeof(header));
    
    if (res == sizeof(header)) {
        switch (header.type) {
            
        case TYPE_PONG:
            log_object_print(NULL, LOG_INFO, FSTR("received pong\r\n"));
            res = 0;
            break;
                    
        default:
            res = -1;
            break;
        }
    } else {
        res = -1;
    }
    
    return (res);
}

static int supervisor_reset(struct link_handler_t *self_p)
{
    uint32_t mask;

    log_object_print(NULL,
                     LOG_WARNING,
                     FSTR("calling the reset callback\r\n"));

    if (self_p->callbacks.reset(self_p) == 0) {
        link_handler_start(self_p);
    }

    log_object_print(NULL,
                     LOG_WARNING,
                     FSTR("suspending the supervisor thread\r\n"));

    mask = 0x1;
    event_read(&self_p->supervisor.event, &mask, sizeof(mask));

    log_object_print(NULL,
                     LOG_WARNING,
                     FSTR("supervisor thread resumed\r\n"));
    
    return (0);
}

/**
 * The supervisor is continuisly monitoring the link by sending ping
 * and expecting to receive pong. The link is reset if no pong is
 * received.
 */
static void *supervisor_main(void *arg_p)
{
    struct link_handler_t *self_p = arg_p;
    int res;
    struct link_handler_header_t ping;
    struct time_t timeout;
    struct chan_list_t list;
    int workspace[16];
    chan_t *chan_p;

    ping.type = TYPE_PING;
    ping.size = 0;

    timeout.seconds = 1;
    timeout.nanoseconds = 0;

    chan_list_init(&list, workspace, sizeof(workspace));
    chan_list_add(&list, &self_p->supervisor);

    while (1) {
        /* Send ping. */
        if (chan_write(self_p->transport.unreliable_out_p,
                       &ping,
                       sizeof(ping)) != sizeof(ping)) {
            log_object_print(NULL, LOG_WARNING, FSTR("failed to write ping\r\n"));
            supervisor_reset(self_p);
        }

        /* Wait for pong or timeout. */
        chan_p = chan_list_poll(&list, &timeout);

        if (chan_p == &self_p->supervisor) {
            res = read_message(self_p);
        } else {
            res = -1;
        }

        /* Reset on timeout or failure. */
        if (res != 0) {
            log_object_print(NULL, LOG_WARNING, FSTR("ping timeout\r\n"));
            supervisor_reset(self_p);
        }

        /* Ping period of one second. */
        thrd_usleep(1000000);
    }

    return (NULL);
}

/**
 * The reader thread reads messages from the input transport
 * channel. Messages are parsed and send to their proper destination;
 * either the supervisor or the application. It also answers to the
 * ping message received from the remote link handler instance.
 */
static void *reader_main(void *arg_p)
{
    struct link_handler_t *self_p = arg_p;
    int res;
    struct link_handler_header_t header;
    struct link_handler_header_t pong;
    uint32_t mask;

    pong.type = TYPE_PONG;
    pong.size = 0;

    while (1) {
        /* Wait for a message on the transport input channel. */
        res = chan_read(self_p->transport.in_p, &header, sizeof(header));

        if (res == sizeof(header)) {
            switch (header.type) {

            case TYPE_PAYLOAD:
                self_p->callbacks.payload(self_p,
                                          self_p->transport.in_p,
                                          header.size);
                break;
            
            case TYPE_PING:
                chan_write(self_p->transport.out_p, &pong, sizeof(pong));
                break;

            default:
                chan_write(&self_p->supervisor, &header, sizeof(header));
                break;
            }
        } else {
            log_object_print(NULL,
                             LOG_WARNING,
                             FSTR("input transport channel down: suspending "
                                  "the reader thread\r\n"));
            mask = 0x1;
            event_read(&self_p->reader.event, &mask, sizeof(mask));
            log_object_print(NULL,
                             LOG_WARNING,
                             FSTR("reader thread resumed\r\n"));
        }
    }

    return (NULL);
}

int link_handler_init(struct link_handler_t *self_p,
                      chan_t *transport_in_p,
                      chan_t *transport_out_p,
                      chan_t *transport_unreliable_out_p,
                      int supervisor_prio,
                      void *supervisor_stack_buf_p,
                      size_t supervisor_stack_size,
                      int reader_prio,
                      void *reader_stack_buf_p,
                      size_t reader_stack_size,
                      link_handler_payload_t payload,
                      link_handler_reset_t reset)
{
    self_p->transport.in_p = transport_in_p;
    self_p->transport.out_p = transport_out_p;
    self_p->transport.unreliable_out_p = transport_unreliable_out_p;
    self_p->transport.in_p = transport_in_p;

    queue_init(&self_p->supervisor.queue, NULL, 0);
    self_p->supervisor.prio = supervisor_prio;
    self_p->supervisor.stack.buf_p = supervisor_stack_buf_p;
    self_p->supervisor.stack.size = supervisor_stack_size;
    self_p->supervisor.thrd_p = NULL;
    event_init(&self_p->supervisor.event);

    self_p->reader.prio = reader_prio;
    self_p->reader.stack.buf_p = reader_stack_buf_p;
    self_p->reader.stack.size = reader_stack_size;
    self_p->reader.thrd_p = NULL;
    event_init(&self_p->reader.event);

    self_p->callbacks.payload = payload;
    self_p->callbacks.reset = reset;

    return (0);
}

int link_handler_start(struct link_handler_t *self_p)
{
    uint32_t mask;

    if (self_p->supervisor.thrd_p == NULL) {
        self_p->supervisor.thrd_p =
            thrd_spawn(supervisor_main,
                       self_p,
                       self_p->supervisor.prio,
                       self_p->supervisor.stack.buf_p,
                       self_p->supervisor.stack.size);
    } else {
        mask = 0x1;
        event_write(&self_p->supervisor.event, &mask, sizeof(mask));
    }

    if (self_p->reader.thrd_p == NULL) {
        self_p->reader.thrd_p =
            thrd_spawn(reader_main,
                       self_p,
                       self_p->reader.prio,
                       self_p->reader.stack.buf_p,
                       self_p->reader.stack.size);
    } else {
        mask = 0x1;
        event_write(&self_p->reader.event, &mask, sizeof(mask));
    }

    return (0);
}

int link_handler_stop(struct link_handler_t *self_p)
{
    return (-1);
}

ssize_t link_handler_write(struct link_handler_t *self_p,
                           const void *buf_p,
                           size_t size)
{
    ssize_t res;
    struct link_handler_header_t header;
    
    /* Write the header. */
    header.type = TYPE_PAYLOAD;
    header.size = size;

    res = chan_write(self_p->transport.out_p, &header, sizeof(header));

    if (res != sizeof(header)) {
        return (res);
    }

    /* Write the payload. */
    res = chan_write(self_p->transport.out_p, buf_p, size);

    return (res);
}
