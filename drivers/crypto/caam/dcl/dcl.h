/*
 * CAAM Descriptor Construction Library
 * Application level usage definitions and prototypes
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

#ifndef DCL_H
#define DCL_H

#include "../desc.h"

/*
 * Section 1 - Descriptor command construction definitions
 * Under development and NOT to be used at present, these are
 * almost guaranteed to change upon review.
 */

enum key_dest {
	KEYDST_KEYREG,
	KEYDST_PK_E,
	KEYDST_AF_SBOX,
	KEYDST_MD_SPLIT
};

enum key_cover {
	KEY_CLEAR,
	KEY_COVERED
};

enum item_inline {
	ITEM_REFERENCE,
	ITEM_INLINE
};

enum item_purpose {
	ITEM_CLASS1,
	ITEM_CLASS2
};

enum ref_type {
	PTR_DIRECT,
	PTR_SGLIST
};

enum ctxsave {
	CTX_SAVE,
	CTX_ERASE
};

enum shrnext {
	SHRNXT_SHARED,
	SHRNXT_LENGTH
};

enum execorder {
	ORDER_REVERSE,
	ORDER_FORWARD
};

enum shrst {
	SHR_NEVER,
	SHR_WAIT,
	SHR_SERIAL,
	SHR_ALWAYS,
	SHR_DEFER
};

enum protdir {
	DIR_ENCAP,
	DIR_DECAP
};

enum mktrust {
	DESC_SIGN,
	DESC_STD
};

/*
 * Type selectors for cipher types in IPSec
 */
#define CIPHER_TYPE_IPSEC_DESCBC              2
#define CIPHER_TYPE_IPSEC_3DESCBC             3
#define CIPHER_TYPE_IPSEC_AESCBC              12
#define CIPHER_TYPE_IPSEC_AESCTR              13
#define CIPHER_TYPE_IPSEC_AES_CCM_ICV8        14
#define CIPHER_TYPE_IPSEC_AES_CCM_ICV12       15
#define CIPHER_TYPE_IPSEC_AES_CCM_ICV16       16
#define CIPHER_TYPE_IPSEC_AES_GCM_ICV8        18
#define CIPHER_TYPE_IPSEC_AES_GCM_ICV12       19
#define CIPHER_TYPE_IPSEC_AES_GCM_ICV16       20

/*
 * Type selectors for authentication in IPSec
 */

#define AUTH_TYPE_IPSEC_MD5HMAC_96            1
#define AUTH_TYPE_IPSEC_SHA1HMAC_96           2
#define AUTH_TYPE_IPSEC_AESXCBCMAC_96         6
#define AUTH_TYPE_IPSEC_SHA1HMAC_160          7
#define AUTH_TYPE_IPSEC_SHA2HMAC_256          12
#define AUTH_TYPE_IPSEC_SHA2HMAC_384          13
#define AUTH_TYPE_IPSEC_SHA2HMAC_512          14

/*
 * Insert a shared descriptor header into a descriptor
 *
 * Inputs:
 * + descwd   = pointer to target descriptor word to hold this command.
 *              Note that this should always be the first word of a
 *              descriptor.
 *
 * + startidx = index to continuation of descriptor data, normally the
 *              first descriptor word past a PDB. This tells DECO what
 *              to skip over.
 *
 * + desclen  = length of descriptor in words, including header.
 *
 * + ctxsave  = Saved or erases context when a descriptor is self-shared
 *              - CTX_SAVE  = context saved between iterations
 *              - CTX_ERASE = context is erased
 *
 * + share    = Share state of this descriptor:
 *              - SHR_NEVER  = Never share. Fetching is repeated for each
 *                             processing pass.
 *              - SHR_WAIT   = Share once processing starts.
 *              - SHR_SERIAL = Share once completed.
 *              - SHR_ALWAYS = Always share (except keys)
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the header just
 *   constructed. If an error occurred, returns 0.
 *
 * Note: Headers should normally be constructed as the final operation
 *       in the descriptor construction, because the start index and
 *       overall descriptor length will likely not be known until
 *       construction is complete. For this reason, there is little use
 *       to the "incremental pointer" convention. The exception is probably
 *       in the construction of simple descriptors where the size is easily
 *       known early in the construction process.
 */
u_int32_t *cmd_insert_shared_hdr(u_int32_t    *descwd,
				 u_int8_t      startidx,
				 u_int8_t      desclen,
				 enum ctxsave  ctxsave,
				 enum shrst    share);

/*
 * Insert a standard descriptor header into a descriptor
 *
 * Inputs:
 * + descwd   = pointer to target descriptor word to hold this command.
 *              Note that this should always be the first word of a
 *              descriptor.
 *
 * + startidx = index to continuation of descriptor data, or if
 *              sharenext = SHRNXT_SHARED, then specifies the size
 *              of the associated shared descriptor referenced in
 *              the following instruction.
 *
 * + desclen  = length of descriptor in words, including header
 *
 * + share    = Share state for this descriptor:
 *              - SHR_NEVER  = Never share. Fetching is repeated for each
 *                             processing pass.
 *              - SHR_WAIT   = Share once processing starts.
 *              - SHR_SERIAL = Share once completed.
 *              - SHR_ALWAYS = Always share (except keys)
 *              - SHR_DEFER  = Use the referenced sharedesc to determine
 *                             sharing intent
 *
 * + sharenext = Control state of shared descriptor processing
 *              - SHRNXT_SHARED = This is a job descriptor consisting
 *                                of a header and a pointer to a shared
 *                                descriptor only.
 *              - SHRNXT_LENGTH = This is a detailed job descriptor, thus
 *                                desclen refers to the full length of this
 *                                descriptor.
 *
 * + reverse   = Reverse execution order between this job descriptor, and
 *               an associated shared descriptor:
 *              - ORDER_REVERSE - execute this descriptor before the shared
 *                                descriptor referenced.
 *              - ORDER_FORWARD - execute the shared descriptor, then this
 *                                descriptor.
 *
 * + mktrusted = DESC_SIGN - sign this descriptor prior to execuition
 *               DESC_STD  - leave descriptor non-trusted
 *
 */
u_int32_t *cmd_insert_hdr(u_int32_t *descwd, u_int8_t startidx,
			  u_int8_t desclen, enum shrst share,
			  enum shrnext sharenext, enum execorder reverse,
			  enum mktrust mktrusted);

/*
 * Insert a key command into a descriptor
 *
 * Inputs:
 * + descwd  = pointer to target descriptor word to hold this command
 *
 * + key     = pointer to key data as an array of bytes.
 *
 * + keylen  = pointer to key size, expressed in bits.
 *
 * + sgref   = pointer is actual data, or a scatter-gather list
 *             representing the key:
 *             - PTR_DIRECT = points to data
 *             - PTR_SGLIST = points to CAAM-specific scatter gather
 *               table. Cannot use if imm = ITEM_INLINE.
 *
 * + dest    = target destination in CAAM to receive the key. This may be:
 *             - KEYDST_KEYREG   = Key register in the CHA selected by an
 *                                 OPERATION command.
 *             - KEYDST_PK_E     = The 'e' register in the public key block
 *             - KEYDST_AF_SBOX  = Direct SBOX load if ARC4 is selected
 *             - KEYDST_MD_SPLIT = Message digest IPAD/OPAD direct load.
 *
 * + cover   = Key was encrypted, and must be decrypted during the load.
 *             If trusted descriptor, use TDEK, else use JDEK to decrypt.
 *             - KEY_CLEAR   = key is cleartext, no decryption needed
 *             - KEY_COVERED = key is ciphertext, decrypt.
 *
 * + imm     = Key can either be referenced, or loaded into the descriptor
 *             immediately following the command for improved performance.
 *             - ITEM_REFERENCE = a pointer follows the command.
 *             - ITEM_INLINE    = key data follows the command, padded out
 *                                to a descriptor word boundary.
 *
 * + purpose = Sends the key to the class 1 or 2 CHA as selected by an
 *             OPERATION command. If dest is KEYDST_PK_E or KEYDST_AF_SBOX,
 *             then this must be ITEM_CLASS1.
 *
 * Returns:
 * + If successful, returns a pointer to the target word incremented
 *   past the newly-inserted command (including item pointer or inlined
 *   data). Effectively, this becomes a pointer to the next word to receive
 *   a new command in this descriptor.
 *
 * + If error, returns 0
 */
u_int32_t *cmd_insert_key(u_int32_t        *descwd,
			  u_int8_t         *key,
			  u_int32_t         keylen,
			  enum ref_type     sgref,
			  enum key_dest     dest,
			  enum key_cover    cover,
			  enum item_inline  imm,
			  enum item_purpose purpose);

/*
 * Insert an IPSec protocol operation command into a descriptor
 *
 * Inputs:
 * + descwd    = pointer to target descriptor word intended to hold
 *               this command. For an OPERATION command, this is normally
 *               the final word of a single descriptor.
 *
 * + cipheralg = blockcipher selection for this protocol descriptor.
 *               This should be one of CIPHER_TYPE_IPSEC_.
 *
 * + authalg   = authentication selection for this protocol descriptor.
 *               This should be one of AUTH_TYPE_IPSEC_.
 *
 * + dir       = Select DIR_ENCAP for encapsulation, or DIR_DECAP for
 *               decapsulation operations.
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the command
 *   just constructed. If an error occurred, returns 0;
 */
u_int32_t *cmd_insert_proto_op_ipsec(u_int32_t   *descwd,
				     u_int8_t     cipheralg,
				     u_int8_t     authalg,
				     enum protdir dir);

/*
 * Insert an SEQ IN PTR command into a descriptor
 *
 * Inputs:
 * + descwd    = pointer to target descriptor word intended to hold
 *               this command. For an OPERATION command, this is normally
 *               the final word of a single descriptor.
 * + ptr       = bus address pointing to the input data buffer
 * + len       = input length
 * + sgref     = pointer is actual data, or a scatter-gather list
 *               representing the key:
 *               - PTR_DIRECT = points to data
 *               - PTR_SGLIST = points to CAAM-specific scatter gather
 *                 table.
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the command
 *   just constructed. If an error occurred, returns 0;
 */
int *cmd_insert_seq_in_ptr(u_int32_t *descwd, u_int32_t *ptr, u_int32_t len,
			   enum ref_type sgref);

/*
 * Insert an SEQ OUT PTR command into a descriptor
 *
 * Inputs:
 * + descwd    = pointer to target descriptor word intended to hold
 *               this command. For an OPERATION command, this is normally
 *               the final word of a single descriptor.
 * + ptr       = bus address pointing to the output data buffer
 * + len       = output length
 * + sgref     = pointer is actual data, or a scatter-gather list
 *               representing the key:
 *               - PTR_DIRECT = points to data
 *               - PTR_SGLIST = points to CAAM-specific scatter gather
 *                 table.
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the command
 *   just constructed. If an error occurred, returns 0;
 */
int *cmd_insert_seq_out_ptr(u_int32_t *descwd, u_int32_t *ptr, u_int32_t len,
			    enum ref_type sgref);

/*
 * Insert an SEQ LOAD command into a descriptor
 *
 * Inputs:
 * descwd       = pointer to target descriptor word intended to hold
 *                this command. For an OPERATION command, this is normally
 *                the final word of a single descriptor.
 * class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *              = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *              = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *              = LDST_CLASS_DECO    = access DECO objects
 * variable_len_flag = use the variable input sequence length
 * dest         = destination
 * offset       = the start point for writing in the destination
 * len          = length of data in bytes
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the command
 *   just constructed. If an error occurred, returns 0;
 */
int *cmd_insert_seq_load(u_int32_t *descwd, unsigned int class_access,
			 int variable_len_flag, unsigned char dest,
			 unsigned char offset, unsigned char len);

/*
 * Insert an SEQ FIFO LOAD command into a descriptor
 *
 * Inputs:
 * + descwd    = pointer to target descriptor word intended to hold
 *               this command. For an OPERATION command, this is normally
 *               the final word of a single descriptor.
 * class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *              = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *              = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *              = LDST_CLASS_DECO    = access DECO objects
 * variable_len_flag = use the variable input sequence length
 * data_type    = FIFO input data type (FIFOLD_TYPE_* in caam_desc.h)
 * len          = output length
 * ptr          = bus address pointing to the output data buffer
 *
 * Returns:
 * + Pointer to next incremental descriptor word past the command
 *   just constructed. If an error occurred, returns 0;
 */
int *cmd_insert_seq_fifo_load(u_int32_t *descwd, unsigned int class_access,
			      int variable_len_flag, unsigned char data_type,
			      u_int32_t len, u_int32_t *ptr);

/*
 * Section 2 - Simple descriptor construction definitions
 */

/*
 * Construct simple sequence job descriptor
 *
 * Constructs a simple job descriptor that contains 3 references:
 *   (1) A pointer to a shared descriptor to do the work. This is
 *       normally assumed to be some sort of a protocol sharedesc,
 *       but doesn't have to be.
 *   (2) A pointer to a packet/frame for input data
 *   (3) A pointer to a packet/frame for output data
 *
 * This descriptor is always a simple reverse-order descriptor,
 * and has no provisions for other content specifications.
 *
 * Inputs:
 *
 */
int cnstr_seq_jobdesc(u_int32_t *jobdesc, unsigned short *jobdescsz,
		      u_int32_t *shrdesc, unsigned short shrdescsize,
		      unsigned char *inbuf, unsigned long insize,
		      unsigned char *outbuf, unsigned long outsize);

/*
 * Section 3 - Single-pass descriptor construction definitions
 */

/*
 * Section 4 - Protocol descriptor construction definitions
 */

/* If protocol descriptor, IPV4 or 6? */
enum protocolvers {
	PDB_IPV4,
	PDB_IPV6
};

/* If antireplay in PDB, how big? */
enum antirply_winsiz {
	PDB_ANTIRPLY_NONE,
	PDB_ANTIRPLY_32,
	PDB_ANTIRPLY_64
};

/* Tunnel or Transport (for next-header byte) ? */
enum connect_type {
	PDB_TUNNEL,
	PDB_TRANSPORT
};

/* Extended sequence number support? */
enum esn {
	PDB_NO_ESN,
	PDB_INCLUDE_ESN
};

/* Decapsulation output format */
enum decap_out {
	PDB_OUTPUT_COPYALL,
	PDB_OUTPUT_DECAP_PDU
};

/* IV source */
enum ivsrc {
	PDB_IV_FROM_PDB,
	PDB_IV_FROM_RNG
};

/*
 * Request parameters for specifying authentication data
 * for a single-pass or protocol descriptor
 */
struct authparams {
	u_int8_t   algtype;  /* Select algorithm */
	u_int8_t  *key;      /* Key as an array of bytes */
	u_int32_t  keylen;   /* Length of key in bits */
};

/*
 * Request parameters for specifying blockcipher data
 * for a single-pass or protocol descriptor
 */
struct cipherparams {
	u_int8_t   algtype;
	u_int8_t  *key;
	u_int32_t  keylen;
};

/*
 * A collection of common definitions for the contents
 * of a Protocol Data Block. At this point in time,
 * it only reflects common data used in IPSec-CBC
 * descriptor construction
 *
 * This could possibly use some seed values for SN/ESN,
 * IV, etc.
 */
struct seqnum {
	enum esn              esn;
	enum antirply_winsiz  antirplysz;
};

struct pdbcont {
	u_int16_t             opthdrlen;
	u_int8_t             *opthdr;
	enum connect_type     transmode;
	enum protocolvers     pclvers;
	enum decap_out        outfmt;
	enum ivsrc            ivsrc;
	struct seqnum         seq;
};

/*
 * Protocol-level shared descriptor constructors
 *
 * These build a full protocol-level shared descriptor for semi-autonomous
 * processing of secured traffic through CAAM. Such descriptors function
 * as single-pass processors (integrating cipher and authentication
 * functions into a single logical step) with the added factor of
 * performing protocol-level packet manipulation in the same step
 * in the packet-handling process, by maintaining protocol-level
 * connection state information within the descriptor itself.
 *
 * For each function, the arguments are uniform:
 *
 * Inputs:
 *
 *    * descbuf    = Points to a buffer to construct the descriptor in.
 *                   All CAAM descriptors are built of an array of up to
 *                   63 32-bit words. If the caller wishes to construct
 *                   a descriptor directly in the executable buffer, then
 *                   that buffer must be hardware DMA-able, and physically
 *                   contiguous.
 *
 *    * bufsize    = Points to an unsigned 16-bit word with the max length
 *                   of the buffer to hold the descriptor. This will be
 *                   written back to with the actual size of the descriptor
 *                   once constructed. (Note: bounds checking not yet
 *                   implemented).
 *
 *    * pdb        = Points to a block of data (struct pdbcont) used to
 *                   describe the content if the Protocol Data Block to be
 *                   maintained inside the descriptor. PDB content is
 *                   protocol and mode specific.
 *
 *    * cipherdata = Points to a block of data used to describe the cipher
 *                   information for encryption/decryption of packet
 *                   content:
 *                   - algtype = one of CIPHER_TYPE_IPSEC_xxx
 *                   - key     = pointer to the cipher key data
 *                   - keydata = size of the key data in bits
 *
 *    * authdata   = Points to a block of data used to describe the
 *                   authentication information for validating the
 *                   authenticity of the packet source.
 *                   - algtype = one of AUTH_TYPE_IPSEC_xxx
 *                   - key     = pointer to the HMAC key data
 *                   - keydata = size of the key data in bits
 *
 *    * clear      = If nonzero, buffer is cleared before writing
 *
 * Returns:
 *
 *    * -1 if the descriptor creation failed for any reason, zero
 *      if creation succeeded.
 *
 */


/*
 * IPSec ESP CBC decapsulation case:
 *
 * pdb.opthdrlen      = Size of inbound header to skip over.
 * pdb.transmode      = PDB_TUNNEL/PDB_TRANSPORT for tunnel or transport
 *                      handling for the next header.
 * pdb.pclvers        = PDB_IPV4/PDB_IPV6 as appropriate for this connection.
 * pdb.seq.esn        = PDB_NO_ESN unless extended sequence numbers are to
 *                      be supported, then PDB_INCLUDE_ESN.
 * pdb.seq/antirplysz = PDB_ANTIRPLY_NONE if no antireplay window is to be
 *                      maintained in the PDB. Otherwise may be
 *                      PDB_ANTIRPLY_32 for a 32-entry window, or
 *                      PDB_ANTIRPLY_64 for a 64-entry window.
 *
 */
int32_t cnstr_pcl_shdsc_ipsec_cbc_decap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear);

/*
 * IPSec ESP CBC encapsulation case:
 *
 * pdbinfo.opthdrlen = Size of outbound IP header to be prepended to
 *                     output.
 * pdbinfo.opthdr    = Pointer to the IP header to be prepended to the
 *                     output, of size opthdrlen.
 * pdbinfo.transmode = PDB_TUNNEL/PDB_TRANSPORT for tunnel/transport
 *                     handling for the next header.
 * pdbinfo.pclvers   = PDB_IPV4/PDB_IPV6 as appropriate for this connection.
 * pdbinfo.seq.esn   = PDB_NO_ESN unless extended sequence numbers are to
 *                     be supported, then PDB_INCLUDE_ESN.
 * pdbinfo.ivsrc     = PDB_IV_FROM_PDB if the IV is to be maintained in
 *                     the PDB, else PDB_IV_FROM_RNG if the IV is to
 *                     be generated internally by CAAM's random number
 *                     generator.
 *
 */
int32_t cnstr_pcl_shdsc_ipsec_cbc_encap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear);

/*
 * Section 5 - disassembler definitions
 */
void desc_hexdump(u_int32_t *descdata,
		  u_int32_t  size,
		  u_int32_t  wordsperline,
		  int8_t    *indentstr);

void caam_desc_disasm(u_int32_t *desc);

#endif /* DCL_H */
