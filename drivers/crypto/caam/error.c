/*
 * CAAM Error Reporting
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

#include "compat.h"
#include "regs.h"
#include "intern.h"
#include "desc.h"
#include "jq.h"

#define SPRINTFCAT(str, format, param, max_alloc)		\
{								\
	char *tmp;						\
								\
	tmp = kmalloc(sizeof(format) + max_alloc, GFP_ATOMIC);	\
	sprintf(tmp, format, param);				\
	strcat(str, tmp);					\
	kfree(tmp);						\
}

static void report_jump_idx(u32 status, char *outstr)
{
	u8 idx = (status & JQSTA_DECOERR_INDEX_MASK) >>
		  JQSTA_DECOERR_INDEX_SHIFT;

	if (status & JQSTA_DECOERR_JUMP)
		strcat(outstr, "jump tgt desc idx ");
	else
		strcat(outstr, "desc idx ");

	SPRINTFCAT(outstr, "%d: ", idx, sizeof("255"));
}

static void report_ccb_status(u32 status, char *outstr)
{
	char *cha_id_list[] = {
		"",
		"AES",
		"DES, 3DES",
		"ARC4",
		"MD5, SHA-1, SH-224, SHA-256, SHA-384, SHA-512",
		"RNG",
		"SNOW f8, SNOW f9",
		"Kasumi f8, f9",
		"All Public Key Algorithms",
		"CRC",
	};
	char *err_id_list[] = {
		"None. No error.",
		"Mode error.",
		"Data size error.",
		"Key size error.",
		"PKHA A memory size error.",
		"PKHA B memory size error.",
		"Data arrived out of sequence error.",
		"PKHA divide-by-zero error.",
		"PKHA modulus even error.",
		"DES key parity error.",
		"ICV check failed.",
		"Hardware error.",
		"Unsupported CCM AAD size.",
		"Invalid CHA selected.",
	};
	u8 cha_id = (status & JQSTA_CCBERR_CHAID_MASK) >>
		    JQSTA_CCBERR_CHAID_SHIFT;
	u8 err_id = status & JQSTA_CCBERR_ERRID_MASK;

	report_jump_idx(status, outstr);

	if (cha_id < sizeof(cha_id_list)) {
		SPRINTFCAT(outstr, "%s: ", cha_id_list[cha_id],
			   strlen(cha_id_list[cha_id]));
	} else {
		SPRINTFCAT(outstr, "unidentified cha_id value 0x%02x: ",
			   cha_id, sizeof("ff"));
	}

	if (err_id < sizeof(err_id_list)) {
		SPRINTFCAT(outstr, "%s", err_id_list[err_id],
			   strlen(err_id_list[err_id]));
	} else {
		SPRINTFCAT(outstr, "unidentified err_id value 0x%02x",
			   err_id, sizeof("ff"));
	}
}

static void report_jump_status(u32 status, char *outstr)
{
	SPRINTFCAT(outstr, "%s() not implemented", __func__, sizeof(__func__));
}

static void report_deco_status(u32 status, char *outstr)
{
	const struct {
		u8 value;
		char *error_text;
	} desc_error_list[] = {
		{ 0x00, "None. No error." },
		{ 0x01, "Link Length Error. The length in the links do not "
			"equal the length in the Descriptor." },
		{ 0x02, "Link Pointer Error. The pointer in link is Null "
			"(Zero)." },
		{ 0x03, "Job Queue Control Error. There is a bad value in "
			"the Job Queue Control register." },
		{ 0x04, "Invalid DESCRIPTOR Command. The Descriptor command "
			"field is invalid." },
		{ 0x05, "Order Error. The commands in the Descriptor are out "
			"of order." },
		{ 0x06, "Invalid KEY Command. The key command is invalid." },
		{ 0x07, "Invalid LOAD Command. The load command is invalid." },
		{ 0x08, "Invalid STORE Command. The store command is "
			"invalid." },
		{ 0x09, "Invalid OPERATION Command. The operation command is "
			"invalid." },
		{ 0x0A, "Invalid FIFO LOAD Command. The FIFO load command is "
			"invalid." },
		{ 0x0B, "Invalid FIFO STORE Command. The FIFO store command "
			"is invalid." },
		{ 0x0C, "Invalid MOVE Command. The move command is invalid." },
		{ 0x0D, "Invalid JUMP Command. The jump command is invalid." },
		{ 0x0E, "Invalid MATH Command. The math command is invalid." },
		{ 0x0F, "Invalid Signed Hash Command. The signed hash command "
			"is invalid." },
		{ 0x10, "Invalid Sequence Command. The SEQ KEY, SEQ LOAD, "
			"SEQ FIFO LOAD, SEQ FIFIO STORE, SEQ IN PTR OR SEQ "
			"OUT PTR command is invalid." },
		{ 0x11, "Internal Error. There is an internal error in the "
			"DECO block. This is a catch all." },
		{ 0x12, "Init Header Error. The Shared Descriptor is "
			"invalid." },
		{ 0x13, "Header Error. Invalid length or parity." },
		{ 0x14, "Burster Error. Burster has gotten to an illegal "
			"state." },
		{ 0x15, "Secure Desc Error. The signed hash of a secure "
			"Descriptor failed." },
		{ 0x16, "DMA Error. A DMA error was encountered." },
		{ 0x17, "Burster IFIFO Error. Burster detected a FIFO LOAD "
			"command and a LOAD command both trying to put data "
			"into the input FIFO." },
		{ 0x1A, "Job failed due to JQ reset" },
		{ 0x1B, "Job failed due to Fail Mode" },
		{ 0x1C, "DECO Watchdog timer timeout error" },
		{ 0x80, "DNR error" },
		{ 0x81, "undefined protocol command" },
		{ 0x82, "invalid setting in PDB" },
		{ 0x83, "Anti-replay LATE error" },
		{ 0x84, "Anti-replay REPLAY error" },
		{ 0x85, "Sequence number overflow" },
		{ 0x86, "Sigver invalid signature" },
		{ 0x87, "DSA Sign Illegal test descriptor" },
		{ 0xC1, "Blob command error - Undefined mode" },
		{ 0xC2, "Blob command error - Secure Memory Blob mode error" },
		{ 0xC4, "Blob command error - Black Blob key or input size "
			"error" },
		{ 0xC8, "Blob command error: Trusted/Secure mode error" },
		{ 0x1D, "Shared Descriptor attempted to load key from a "
			"locked DECO" },
		{ 0x1E, "Shared Descriptor attempted to share data from a "
			"DECO that had a Descriptor error" },
	};
	u8 desc_error = status & JQSTA_DECOERR_ERROR_MASK;
	int i;

	report_jump_idx(status, outstr);

	for (i = 0; i < sizeof(desc_error_list); i++)
		if (desc_error_list[i].value == desc_error)
			break;

	if (i != sizeof(desc_error_list) && desc_error_list[i].error_text) {
		SPRINTFCAT(outstr, "%s", desc_error_list[i].error_text,
			   strlen(desc_error_list[i].error_text));
	} else {
		SPRINTFCAT(outstr, "unidentified error value 0x%02x",
			   desc_error, sizeof("ff"));
	}
}

static void report_jq_status(u32 status, char *outstr)
{
	SPRINTFCAT(outstr, "%s() not implemented", __func__, sizeof(__func__));
}

static void report_cond_code_status(u32 status, char *outstr)
{
	SPRINTFCAT(outstr, "%s() not implemented", __func__, sizeof(__func__));
}

char *caam_jq_strstatus(char *outstr, u32 status)
{
	struct stat_src {
		void (*report_ssed)(u32 status, char *outstr);
		char *error;
	} status_src[] = {
		{ NULL, "No error" },
		{ NULL, NULL },
		{ report_ccb_status, "CCB" },
		{ report_jump_status, "Jump" },
		{ report_deco_status, "DECO" },
		{ NULL, NULL },
		{ report_jq_status, "Job Queue" },
		{ report_cond_code_status, "Condition Code" },
	};
	u32 ssrc = status >> JQSTA_SSRC_SHIFT;

	sprintf(outstr, "%s: ", status_src[ssrc].error);

	if (status_src[ssrc].report_ssed)
		status_src[ssrc].report_ssed(status, outstr);

	return outstr;
}
EXPORT_SYMBOL(caam_jq_strstatus);
