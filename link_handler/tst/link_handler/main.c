/**
 * @file main.c
 * @version 0.0.1
 *
 * @section License
 * Copyright (C) 2016, Erik Moqvist
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
 */

#include "simba.h"
#include "link_handler.h"

struct header_t {
    int32_t type;
    int32_t size;
};

THRD_STACK(supervisor_stack, 1024);
THRD_STACK(reader_stack, 1024);

static struct link_handler_t foo;
static struct queue_t qin;
static struct queue_t qout;
static char qinbuf[128];
static char qoutbuf[128];
static struct thrd_t *thrd_p = NULL;

static int payload(struct link_handler_t *self_p,
                   chan_t *chan_p,
                   size_t size)
{
    int a;

    BTASSERT(self_p == &foo);
    BTASSERT(chan_p == &qin);

    BTASSERT(queue_read(chan_p, &a, sizeof(a)) == sizeof(a));

    if (thrd_p != NULL) {
        thrd_resume(thrd_p, a);
    }

    return (0);
}

int reset(struct link_handler_t *self_p)
{
    BTASSERT(self_p == &foo);

    if (thrd_p != NULL) {
        /* Reset the queues. */
        BTASSERT(queue_stop(&qin) == 1);
        BTASSERT(queue_stop(&qout) == 0);
        BTASSERT(queue_start(&qin) == 0);
        BTASSERT(queue_start(&qout) == 0);
                
        thrd_resume(thrd_p, 7);
    }

    return (0);
}

static int test_init(struct harness_t *harness_p)
{
    BTASSERT(queue_init(&qin, qinbuf, sizeof(qinbuf)) == 0);
    BTASSERT(queue_init(&qout, qoutbuf, sizeof(qoutbuf)) == 0);

    BTASSERT(link_handler_init(&foo,
                               &qin,
                               &qout,
                               &qout,
                               0,
                               supervisor_stack,
                               sizeof(supervisor_stack),
                               0,
                               reader_stack,
                               sizeof(reader_stack),
                               payload,
                               reset) == 0);

    BTASSERT(link_handler_start(&foo) == 0);

    return (0);
}

static int test_ping(struct harness_t *harness_p)
{
    struct header_t header;

    /* Wait for a ping. */
    BTASSERT(queue_read(&qout, &header, sizeof(header)) == sizeof(header));

    BTASSERT(header.type == 2);
    BTASSERT(header.size == 0);

    /* Write pong. */
    header.type = 3;
    BTASSERT(queue_write(&qin, &header, sizeof(header)) == sizeof(header));

    /* Wait for a ping. */
    BTASSERT(queue_read(&qout, &header, sizeof(header)) == sizeof(header));

    BTASSERT(header.type == 2);
    BTASSERT(header.size == 0);

    /* Write pong. */
    header.type = 3;
    BTASSERT(queue_write(&qin, &header, sizeof(header)) == sizeof(header));

    /* Write ping. */
    header.type = 2;
    BTASSERT(queue_write(&qin, &header, sizeof(header)) == sizeof(header));

    /* Wait for a pong. */
    BTASSERT(queue_read(&qout, &header, sizeof(header)) == sizeof(header));

    BTASSERT(header.type == 3);
    BTASSERT(header.size == 0);

    return (0);
}

static int test_reset(struct harness_t *harness_p)
{
    thrd_p = thrd_self();

    /* Wait for link reset. */
    BTASSERT(thrd_suspend(NULL) == 7);

    return (0);
}

static int test_payload(struct harness_t *harness_p)
{
    struct header_t header;
    int a;

    a = 9;
    BTASSERT(link_handler_write(&foo, &a, sizeof(a)) == sizeof(a));

    /* Read the header. */
    BTASSERT(queue_read(&qout, &header, sizeof(header)) == sizeof(header));

    /* Discard any ping message. */
    if (header.type == 2) {
        BTASSERT(queue_read(&qout, &header, sizeof(header)) == sizeof(header));
    }

    BTASSERT(header.type == 1);
    BTASSERT(header.size == sizeof(a));

    /* Read the payload. */
    BTASSERT(queue_read(&qout, &a, sizeof(a)) == sizeof(a));

    BTASSERT(a == 9);

    /* Write a payload message. */
    header.type = 1;
    header.size = sizeof(a);
    BTASSERT(queue_write(&qin, &header, sizeof(header)) == sizeof(header));

    a = 12;
    BTASSERT(queue_write(&qin, &a, sizeof(a)) == sizeof(a));

    thrd_p = thrd_self();
    BTASSERT(thrd_suspend(NULL) == 12);

    return (0);
}

int main()
{
    struct harness_t harness;
    struct harness_testcase_t harness_testcases[] = {
        { test_init, "test_init" },
        { test_ping, "test_ping" },
        { test_reset, "test_reset" },
        { test_payload, "test_payload" },
        { NULL, NULL }
    };

    sys_start();
    uart_module_init();

    harness_init(&harness);
    harness_run(&harness, harness_testcases);

    return (0);
}
