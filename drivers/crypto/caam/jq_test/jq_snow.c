/*
 * jq_snow.c - JobQ unit test for SNOW examples
 *
 * Copyright (c) 2009, Freescale Semiconductor, Inc.
 * All rights reserved.
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

#include "caam_jqtest.h"

/*
 * Case taken from SNOW conformance document, UEA2 case #1
 * Last byte 0x7c from 0x78 on account of length change from
 * 253 bits to 16 bytes
 */
static u_int8_t snow_key[] = {
	0xd3, 0xc5, 0xd5, 0x92,
	0x32, 0x7f, 0xb1, 0x1c,
	0x40, 0x35, 0xc6, 0x68,
	0x0a, 0xf8, 0xc6, 0xd1
};

#define SNOW_F8_TESTSET_SIZE 32

static u_int8_t input_msg_data[] = {
	0x98, 0x1b, 0xa6, 0x82,
	0x4c, 0x1b, 0xfb, 0x1a,
	0xb4, 0x85, 0x47, 0x20,
	0x29, 0xb7, 0x1d, 0x80,
	0x8c, 0xe3, 0x3e, 0x2c,
	0xc3, 0xc0, 0xb5, 0xfc,
	0x1f, 0x3d, 0xe8, 0xa6,
	0xdc, 0x66, 0xb1, 0xf0
};

static u_int8_t output_msg_data[] = {
	0x5d, 0x5b, 0xfe, 0x75,
	0xeb, 0x04, 0xf6, 0x8c,
	0xe0, 0xa1, 0x23, 0x77,
	0xea, 0x00, 0xb3, 0x7d,
	0x47, 0xc6, 0xa0, 0xba,
	0x06, 0x30, 0x91, 0x55,
	0x08, 0x6a, 0x85, 0x9c,
	0x43, 0x41, 0xb3, 0x7c
};

static const u_int32_t count = 0x398a59b4;
static const u_int8_t bearer = 0x15;
static const u_int8_t direction = 0x1;

extern wait_queue_head_t jqtest_wq;

void jq_snow_done(struct device *dev, u32 *head, u32 status, void *auxarg)
{
	/* Bump volatile completion test value and wake calling thread */
	(*(int *)auxarg)++;
	wake_up_interruptible(&jqtest_wq);
}

int jq_snow_f8(struct device *dev, int show)
{
	int stat, exit, rtnval = 0;
	u32 *sdesc, *jdesc, *sdmap;
	u8 *inbuf, *outbuf, *inmap, *outmap;
	u16 sdsz, jdsz, inbufsz, outbufsz;
	int jqarg;

	jqarg = 0;

	/* Allocate more than necessary for both descs */
	sdsz = 64 * sizeof(u32);
	jdsz = 16 * sizeof(u32);
	sdesc = kzalloc(sdsz, GFP_KERNEL | GFP_DMA);
	jdesc = kzalloc(jdsz, GFP_KERNEL | GFP_DMA);

	/* Allocate buffers */
	inbufsz = SNOW_F8_TESTSET_SIZE;
	outbufsz = SNOW_F8_TESTSET_SIZE;
	inbuf = kmalloc(inbufsz, GFP_KERNEL | GFP_DMA);
	outbuf = kmalloc(outbufsz, GFP_KERNEL | GFP_DMA);

	if ((sdesc == NULL) || (jdesc == NULL) ||
	    (inbuf == NULL) || (outbuf == NULL)) {
		printk(KERN_INFO "jq_snow_f8: can't get buffers\n");
		kfree(sdesc);
		kfree(jdesc);
		kfree(inbuf);
		kfree(outbuf);
		return -1;
	};

	memcpy(inbuf, input_msg_data, inbufsz);

	stat = cnstr_shdsc_snow_f8(sdesc, &sdsz, snow_key, 128,
				   DIR_ENCRYPT, count, bearer, direction,
				   0);

	if (stat) {
		printk(KERN_INFO
		       "jq_snow_f8: sharedesc construct failed\n");
		kfree(sdesc);
		kfree(jdesc);
		return -1;
	};

	/* Map data prior to jobdesc build */
	sdmap = (u32 *)dma_map_single(dev, sdesc, sdsz, DMA_BIDIRECTIONAL);

	inmap = (u8 *)dma_map_single(dev, inbuf, inbufsz, DMA_TO_DEVICE);
	outmap = (u8 *)dma_map_single(dev, outbuf, outbufsz, DMA_FROM_DEVICE);

	/* build jobdesc */
	cnstr_seq_jobdesc(jdesc, &jdsz, sdmap, sdsz, inmap, inbufsz,
			  outmap, outbufsz);

	/* Show it before we run it */
	if (show == SHOW_DESC) {
		caam_desc_disasm(jdesc);
		caam_desc_disasm(sdesc);
	}

	/* Enqueue and block*/
	stat = caam_jq_enqueue(dev, jdesc, jq_snow_done, (void *)&jqarg);
	if (stat) {
		printk(KERN_INFO "jq_snow_f8: can't enqueue\n");
		rtnval = -1;
	}
	exit = wait_event_interruptible(jqtest_wq, (jqarg));
	if (exit)
		printk(KERN_INFO "jq_snow_f8: interrupted\n");

	dma_unmap_single(dev, (u32)sdmap, sdsz, DMA_BIDIRECTIONAL);
	dma_unmap_single(dev, (u32)inmap, inbufsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)outmap, outbufsz, DMA_FROM_DEVICE);

	if (memcmp(output_msg_data, outbuf, outbufsz))
		printk(KERN_INFO "jq_snow_f8: output mismatch\n");

	kfree(sdesc);
	kfree(jdesc);
	kfree(inbuf);
	kfree(outbuf);

	return rtnval;
}


static const unsigned char uia2_key[] =
{
    0x2b, 0xd6, 0x45, 0x9f, 0x82, 0xc5, 0xb3, 0x00,
    0x95, 0x2c, 0x49, 0x10, 0x48, 0x81, 0xff, 0x48
};

static const unsigned char uia2_in[] =
{
    0x33, 0x32, 0x34, 0x62, 0x63, 0x39, 0x38, 0x61,
    0x37, 0x34, 0x79
};

static const unsigned char uia2_out[] =
{
    0xee, 0x41, 0x9e, 0x0d
};

static const u_int32_t uia2_count = 0x38a6f056;
static const u_int32_t uia2_fresh = 0xb8aefda9;
static const u_int8_t uia2_dir;

int jq_snow_f9(struct device *dev, int show)
{
	int stat, exit, rtnval = 0;
	u32 *sdesc, *jdesc, *sdmap;
	u8 *inbuf, *outbuf, *inmap, *outmap;
	u16 sdsz, jdsz, inbufsz, outbufsz;
	int jqarg;

	jqarg = 0;

	/* Allocate more than necessary for both descs */
	sdsz = 64 * sizeof(u32);
	jdsz = 16 * sizeof(u32);
	sdesc = kzalloc(sdsz, GFP_KERNEL | GFP_DMA);
	jdesc = kzalloc(jdsz, GFP_KERNEL | GFP_DMA);

	/* Allocate buffers */
	inbufsz = 11;
	outbufsz = 4;
	inbuf = kmalloc(inbufsz, GFP_KERNEL | GFP_DMA);
	outbuf = kmalloc(outbufsz, GFP_KERNEL | GFP_DMA);

	if ((sdesc == NULL) || (jdesc == NULL) ||
	    (inbuf == NULL) || (outbuf == NULL)) {
		printk(KERN_INFO "jq_snow_f9: can't get buffers\n");
		kfree(sdesc);
		kfree(jdesc);
		kfree(inbuf);
		kfree(outbuf);
		return -1;
	};

	memcpy(inbuf, uia2_in, inbufsz);

	stat = cnstr_shdsc_snow_f9(sdesc, &sdsz, (u_int8_t *)uia2_key, 128,
				   DIR_ENCRYPT, uia2_count, uia2_fresh,
				   uia2_dir, 0);

	if (stat) {
		printk(KERN_INFO
		       "jq_snow_f9: sharedesc construct failed\n");
		kfree(sdesc);
		kfree(jdesc);
		return -1;
	};

	/* Map data prior to jobdesc build */
	sdmap = (u32 *)dma_map_single(dev, sdesc, sdsz, DMA_BIDIRECTIONAL);
	inmap = (u8 *)dma_map_single(dev, inbuf, inbufsz, DMA_TO_DEVICE);
	outmap = (u8 *)dma_map_single(dev, outbuf, outbufsz, DMA_FROM_DEVICE);

	/* build jobdesc */
	cnstr_seq_jobdesc(jdesc, &jdsz, sdmap, sdsz, inmap, inbufsz,
			  outmap, outbufsz);

	/* Show it before we run it */
	if (show == SHOW_DESC) {
		caam_desc_disasm(jdesc);
		caam_desc_disasm(sdesc);
	}

	/* Enqueue and block*/
	stat = caam_jq_enqueue(dev, jdesc, jq_snow_done, (void *)&jqarg);
	if (stat) {
		printk(KERN_INFO "jq_snow_f9: can't enqueue\n");
		rtnval = -1;
	}
	exit = wait_event_interruptible(jqtest_wq, (jqarg));
	if (exit)
		printk(KERN_INFO "jq_snow_f9: interrupted\n");

	dma_unmap_single(dev, (u32)sdmap, sdsz, DMA_BIDIRECTIONAL);
	dma_unmap_single(dev, (u32)inmap, inbufsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)outmap, outbufsz, DMA_FROM_DEVICE);

	if (memcmp(uia2_out, outbuf, outbufsz))
		printk(KERN_INFO "jq_snow_f9: output mismatch\n");

	kfree(sdesc);
	kfree(jdesc);
	kfree(inbuf);
	kfree(outbuf);

	return rtnval;
}

