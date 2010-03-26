/*
 * CAAM Descriptor Construction Library
 * Protocol-level Shared Descriptor Constructors
 *
 * Copyright (c) 2008, 2009, Freescale Semiconductor, Inc.
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

int32_t cnstr_pcl_shdsc_ipsec_cbc_decap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear)
{
	u_int32_t *start;
	u_int8_t   pdbopts;
	u_int16_t  startidx, endidx;

	start = descbuf++; /* save start for eventual header write */
			   /* bump to first word of PDB */

	/* first, got to have clean buffer */
	if (!descbuf)
		return -1;

	/* Prove other arguments */
	if (!authdata->key)
		return -3;

	if (!cipherdata->key)
		return -4;

	if (!authdata->keylen)
		return -5;

	if (!cipherdata->keylen)
		return -6;

	/* If user requested a buffer clear, do it from the start */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Build PDB, beginning with length and options */
	pdbopts = 0;

	if (pdb->transmode == PDB_TUNNEL)
		pdbopts |= PDBOPTS_ESPCBC_TUNNEL;

	if (pdb->pclvers == PDB_IPV6)
		pdbopts |= PDBOPTS_ESPCBC_IPVSN;

	if (pdb->outfmt == PDB_OUTPUT_DECAP_PDU)
		pdbopts |= PDBOPTS_ESPCBC_OUTFMT;

	if (pdb->seq.esn == PDB_INCLUDE_ESN)
		pdbopts |= PDBOPTS_ESPCBC_ESN;

	switch (pdb->seq.antirplysz) {
	case PDB_ANTIRPLY_32:
		pdbopts |= PDBOPTS_ESPCBC_ARS32;
		break;

	case PDB_ANTIRPLY_64:
		pdbopts |= PDBOPTS_ESPCBC_ARS64;
		break;

	case PDB_ANTIRPLY_NONE:
	default:
		pdbopts |= PDBOPTS_ESPCBC_ARSNONE;
		break;
	}

	*descbuf++ = (pdb->opthdrlen << 16) | pdbopts;

	/* Skip reserved section */
	descbuf += 2;

	/* Skip DECO writeback part */
	descbuf += 4;

	/* Save current location for computing start index */
	startidx = descbuf - start;

	/*
	 * PDB now complete
	 * Now process keys, starting with class 2/authentication
	 * This is assuming keys are immediate for sharedesc
	 */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS2);

	/* Now the class 1/cipher key */
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);


	/* Insert the operation command */
	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_DECAP);

	/* Now update the header with size/offsets */
	endidx = descbuf - start + 1; /* add 1 to include header */

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_ipsec_cbc_decap);

int32_t cnstr_pcl_shdsc_ipsec_cbc_encap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear)
{
	u_int32_t *start;
	u_int8_t   pdbopts;
	u_int16_t  hdrwds, startidx, endidx;
	u_int32_t *hdrbuf;

	start = descbuf++; /* save start for eventual header write */
			   /* bump to first word of PDB */

	/* Verify a clean buffer */
	if (!descbuf)
		return -1;

	/* Verify other pertinent arguments */
	if (!authdata->key)
		return -3;

	if (!cipherdata->key)
		return -4;

	if (!authdata->keylen)
		return -5;

	if (!cipherdata->keylen)
		return -6;

	/* If user requested a buffer clear, do it from the start */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Construct PDB, starting with options word */
	pdbopts = 0;

	if (pdb->transmode == PDB_TUNNEL)
		pdbopts |= PDBOPTS_ESPCBC_TUNNEL;

	if (pdb->opthdrlen)
		pdbopts |= PDBOPTS_ESPCBC_INCIPHDR | PDBOPTS_ESPCBC_IPHDRSRC;

	if (pdb->pclvers == PDB_IPV6)
		pdbopts |= PDBOPTS_ESPCBC_IPVSN;

	if (pdb->seq.esn == PDB_INCLUDE_ESN)
		pdbopts |= PDBOPTS_ESPCBC_ESN;

	if (pdb->ivsrc == PDB_IV_FROM_RNG)
		pdbopts |= PDBOPTS_ESPCBC_IVSRC;

	*descbuf++ = pdbopts;

	/* Skip sequence numbers */
	descbuf += 2;

	/* Skip IV */
	descbuf += 4;

	/* Skip SPI */
	descbuf++;

		*descbuf++ = pdb->opthdrlen;
	/* If PDB-resident IPHeader, set size and append it */
	if (pdb->opthdr && pdb->opthdrlen) {
		hdrwds = pdb->opthdrlen >> 2;
		hdrbuf = (u_int32_t *)pdb->opthdr;
		while (hdrwds) {
			*descbuf++ = *hdrbuf++;
			hdrwds--;
		}
	}


	/* Save current location for computing start index */
	startidx = descbuf - start;

	/*
	 * Now process keys, starting with class 2/authentication
	 * This is assuming keys are immediate for sharedesc
	 */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS2);

	/* Now the class 1/cipher key */
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);


	/* Now insert the operation command */
	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_ENCAP);

	/*
	 * Now update the header with size/offsets
	 * add 1 to include header
	 */
	endidx = descbuf - start + 1;

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_ipsec_cbc_encap);
