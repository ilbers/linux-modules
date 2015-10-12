/*
 * Basic ring buffer routine.
 *
 * Copyright (c) 2014-2015 ilbers GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LIB_RING_BUFFER_H__
#define __LIB_RING_BUFFER_H__

/* Simple ring buffer implementation.
 *
 * NOTE: size of buffer should be 2^N
 */

#define RING_BUFFER(name, type, len)					\
	typedef struct {						\
		type buf[len];						\
		int start;						\
		int fill;						\
		int size;						\
	} name

#define RING_BUFFER_INIT(rb)						\
	do								\
	{								\
		rb.start = 0;						\
		rb.fill = 0;						\
		rb.size = sizeof(rb.buf) / sizeof(rb.buf[0]);		\
	}								\
	while(0)

#define RING_BUFFER_PUSH(rb, c)						\
	do								\
	{								\
		int idx = (rb.start + rb.fill) & (rb.size - 1);		\
									\
		rb.buf[idx] = c;					\
		if (rb.fill < rb.size)					\
		{							\
			rb.fill++;					\
		}							\
		else							\
		{							\
			rb.start = (rb.start + 1) & (rb.size - 1);	\
		}							\
	}								\
	while(0)

#define RING_BUFFER_POP(rb)						\
	({								\
		typeof(rb.buf[rb.start]) _r = 0;			\
		if (rb.fill)						\
		{							\
			_r = rb.buf[rb.start];				\
			rb.fill--;					\
			rb.start = (rb.start + 1) & (rb.size - 1);	\
		}							\
		_r;							\
	})								\

#define RING_BUFFER_FILL(rb)						\
	({								\
		rb.fill;						\
	})

#define RING_BUFFER_FULL(rb)						\
	({								\
		rb.fill == rb.size;					\
	})

#endif /* __LIB_RING_BUFFER_H__ */
