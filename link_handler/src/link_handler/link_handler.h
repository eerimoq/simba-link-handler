/**
 * @file link_handler/link_handler.h
 * @version 0.2.0
 *
 * @section License
 * Copyright (C) 2015-2016, Erik Moqvist
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

#ifndef __LINK_HANDLER_LINK_HANDLER_H__
#define __LINK_HANDLER_LINK_HANDLER_H__

#include "simba.h"

struct link_handler_t;

/**
 * The payload callback is called by the link handler when a payload
 * mesage arrives on the transport channel.
 *
 * @param[in] self_p Link handler to reset.
 *
 * @return Number of read bytes or negative error code.
 */
typedef int (*link_handler_payload_t)(struct link_handler_t *self_p,
                                      chan_t *chan_p,
                                      size_t size);

/**
 * This callback is called when the transport channels fails transfer
 * data to/from the remote link handler. The link handler is suspended
 * until the transport channels have been connected to the peer
 * again. The link handler is resumed if this function return zero(0),
 * otherwise `link_handler_start()` can be used to resume the link
 * handler.
 *
 * @param[in] self_p Link handler to reset.
 *
 * @return zero(0) on successful reset or negative error code.
 */
typedef int (*link_handler_reset_t)(struct link_handler_t *self_p);

struct link_handler_t {
    struct {
        chan_t *in_p;
        chan_t *out_p;
        chan_t *unreliable_out_p;
    } transport;
    struct {
        struct queue_t queue;
        int prio;
        struct {
            void *buf_p;
            size_t size;
        } stack;
        struct thrd_t *thrd_p;
        struct event_t event;
    } supervisor;
    struct {
        int prio;
        struct {
            void *buf_p;
            size_t size;
        } stack;
        struct thrd_t *thrd_p;
        struct event_t event;
    } reader;
    struct {
        link_handler_payload_t payload;
        link_handler_reset_t reset;
    } callbacks;
};

/**
 * Initialize given link handler. A supervisor monitors the link state
 * and tries to reconnect if the link is disconnected.
 *
 * @param[in,out] self_p Link handler to initialize.
 * @param[in] transport_in_p Input transport channel. The link handler
 *                           reads data from the remote handler on
 *                           this channel.
 * @param[in] transport_out_p Output transport channel. The link
 *                            handler writes data to the remote
 *                            handler on this channel.
 * @param[in] transport_unreliable_out_p Unreliable output transport
 *                                       channel. Used by the
 *                                       supervisor to ping the remote
 *                                       link handler.
 * @param[in] payload Link handler payload callback.
 * @param[in] event Link handler reset callback.
 *
 * @return zero(0) or negative error code.
 */
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
                      link_handler_reset_t reset);

/**
 * Start given link handler.
 *
 * @param[in] self_p Link handler to start.
 *
 * @return zero(0) or negative error code.
 */
int link_handler_start(struct link_handler_t *self_p);

/**
 * Stop given link handler.
 *
 * @param[in] self_p Link handler to stop.
 *
 * @return zero(0) or negative error code.
 */
int link_handler_stop(struct link_handler_t *self_p);

/**
 * Write data to given link handler. 
 *
 * @param[in] self_p Link handler.
 *
 * @return zero(0) or negative error code.
 */
ssize_t link_handler_write(struct link_handler_t *self_p,
                           const void *buf_p,
                           size_t size);

#endif
