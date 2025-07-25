/*
 * Copyright (C) 2002, Simon Nieuviarts.
 */
/*
 * Copyright (c) 1994-1999 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 *
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * Mach Operating System
 * Copyright (c) 1993,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <stdarg.h>
#include <doprnt.h>
#include <n7OS/console.h>

#include <n7OS/sys.h>
#include <unistd.h>


/*
 * This is the function called by printfk to send its output to the screen. You
 * have to implement it in the kernel.
 */
extern void console_putbytes(const char *s, int len);

/* This version of printfk is implemented in terms of putchark and putsk.  */

#define	printfk_BUFMAX	128

struct printfk_state {
	char buf[printfk_BUFMAX];
	unsigned int index;
};

static void
flush(struct printfk_state *state)
{
	/*
	 * It would be nice to call write(1,) here, but if fd_set_console
	 * has not been called, it will break.
	 */
	
	console_putbytes((const char *)state->buf, state->index);

	state->index = 0;
}

static void
printfk_char(arg, c)
	char *arg;
	int c;
{
	struct printfk_state *state = (struct printfk_state *) arg;

	if ((c == 0) || (c == '\n') || (state->index >= printfk_BUFMAX))
	{
		flush(state);
		state->buf[0] = c;
		console_putbytes((const char *)state->buf, 1);
	}
	else
	{
		state->buf[state->index] = c;
		state->index++;
	}
}

/*
 * Printing (to console)
 */
int vprintfk(const char *fmt, va_list args)
{
	struct printfk_state state;

	state.index = 0;
	_doprnt(fmt, args, 0, (void (*)())printfk_char, (char *) &state);

	if (state.index != 0)
	    flush(&state);

	/* _doprnt currently doesn't pass back error codes,
	   so just assume nothing bad happened.  */
	return 0;
}

int
printfk(const char *fmt, ...)
{
	va_list	args;
	int err;

	va_start(args, fmt);
	err = vprintfk(fmt, args);
	va_end(args);

	return err;
}

int putchark(int c)
{
	char ch = c;
	console_putbytes(&ch, 1);
        return (unsigned char)ch;
}

int putsk(const char *s)
{
        while (*s) {
                putchark(*s++);
        }
	putchark('\n');
        return 0;
}
