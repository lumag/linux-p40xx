/*
 * CAAM Descriptor Construction Library
 * Basic job descriptor construction
 *
 * Copyright (c) 2009, Freescale Semiconductor, Inc.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../compat.h"
#include "dcl.h"


int cnstr_seq_jobdesc(u_int32_t *jobdesc, unsigned short *jobdescsz,
		      u_int32_t *shrdesc, unsigned short shrdescsz,
		      unsigned char *inbuf, unsigned long insize,
		      unsigned char *outbuf, unsigned long outsize)
{
	u_int32_t *next;

	/*
	 * Basic structure is
	 * - header (assume sharing, reverse order)
	 * - sharedesc physical address
	 * - SEQ_OUT_PTR
	 * - SEQ_IN_PTR
	 */

	/* Make running pointer past where header will go */
	next = jobdesc;
	next++;

	/* Insert sharedesc */
	*next++ = (u_int32_t)shrdesc;

	/* Sequence pointers */
	next = cmd_insert_seq_out_ptr(next, (void *)outbuf, outsize,
				      PTR_DIRECT);
	next = cmd_insert_seq_in_ptr(next, (void *)inbuf, insize,
				     PTR_DIRECT);

	/* Now update header */
	*jobdescsz = next - jobdesc; /* add 1 to include header */
	cmd_insert_hdr(jobdesc, shrdescsz, *jobdescsz, SHR_SERIAL,
		       SHRNXT_SHARED, ORDER_REVERSE, DESC_STD);

	return 0;
}
