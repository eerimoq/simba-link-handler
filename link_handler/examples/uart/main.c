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

THRD_STACK(supervisor_stack, 1024);
THRD_STACK(reader_stack, 1024);

static struct uart_driver_t link_handler_uart;

static int payload(struct link_handler_t *self_p,
                   chan_t *chan_p,
                   size_t size)
{
    std_printf(FSTR("payload: size = %d\r\n"), (int)size);
    
    return (0);
}

static int reset(struct link_handler_t *self_p)
{
    std_printf(FSTR("reset function called\r\n"));

    /* Reset the transport channel, the uart. */
    uart_stop(&link_handler_uart);
    uart_start(&link_handler_uart);

    return (0);
}

int main()
{
    struct uart_driver_t uart;
    struct link_handler_t link_handler;
    uint8_t buf[16];

    sys_start();

    uart_module_init();
    uart_init(&uart, &uart_device[0], 38400, NULL, 0);
    uart_start(&uart);
    sys_set_stdout(&uart.chout);

    /* Initialize and start the link handler. */
    uart_init(&link_handler_uart, &uart_device[1], 38400, buf, sizeof(buf));
    uart_start(&link_handler_uart);
    link_handler_init(&link_handler,
                      &link_handler_uart.chin,
                      &link_handler_uart.chout,
                      &link_handler_uart.chout,
                      0,
                      supervisor_stack,
                      sizeof(supervisor_stack),
                      0,
                      reader_stack,
                      sizeof(reader_stack),
                      payload,
                      reset);
    link_handler_start(&link_handler);

    thrd_suspend(NULL);

    return (0);
}
