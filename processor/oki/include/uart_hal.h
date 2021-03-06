/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @brief UART driver for OKI processor
 * @author Andrew Barton-Sweeney (abs@cs.yale.edu)
 *
 */


#ifndef _UART_HAL_H
#define _UART_HAL_H


/**
 * @brief UART task
 */
extern void uart_hardware_init(void);
extern int uart_putchar(char c);
#define uart_recv_interrupt()      void uart_recv_int_(void)
#define uart_send_interrupt()      void uart_send_int_(void)

#define uart_enable_rx()
#define uart_disable_rx()
void uart_enable_tx();
int uart_is_disabled();
void uart_disable_tx();
char uart_getByte();
void uart_setByte(char b);
char uart_checkError();

extern void uart_recv_int_(void);
extern void uart_send_int_(void);

#endif // _OKI_UART_H


