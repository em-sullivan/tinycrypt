/* hmac.c - TinyCrypt implementation of the HMAC algorithm */

/*
 *  Copyright (C) 2017 by Intel Corporation, All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *    - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    - Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <tinycrypt/hmac.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>

static void rekey(uint_least8_t *key, const uint_least8_t *new_key, uint32_t key_size)
{
	const uint_least8_t inner_pad = (uint_least8_t) 0x36;
	const uint_least8_t outer_pad = (uint_least8_t) 0x5c;
	uint32_t i;

	for (i = 0; i < key_size; ++i) {
		key[i] = inner_pad ^ new_key[i];
		key[i + TC_SHA256_BLOCK_SIZE] = outer_pad ^ new_key[i];
	}
	for (; i < TC_SHA256_BLOCK_SIZE; ++i) {
		key[i] = inner_pad; key[i + TC_SHA256_BLOCK_SIZE] = outer_pad;
	}
}

int tc_hmac_set_key(TCHmacState_t ctx, const uint_least8_t *key,
		    uint32_t key_size)
{
	/* Input sanity check */
	if (ctx == (TCHmacState_t) 0 ||
	    key == (const uint_least8_t *) 0 ||
	    key_size == 0) {
		return TC_CRYPTO_FAIL;
	}

	const uint_least8_t dummy_key[TC_SHA256_BLOCK_SIZE];
	struct tc_hmac_state_struct dummy_state;

	if (key_size <= TC_SHA256_BLOCK_SIZE) {
		/*
		 * The next three calls are dummy calls just to avoid
		 * certain timing attacks. Without these dummy calls,
		 * adversaries would be able to learn whether the key_size is
		 * greater than TC_SHA256_BLOCK_SIZE by measuring the time
		 * consumed in this process.
		 */
		(void)tc_sha256_init(&dummy_state.hash_state);
		(void)tc_sha256_update(&dummy_state.hash_state,
				       dummy_key,
				       key_size);
		(void)tc_sha256_final(&dummy_state.key[TC_SHA256_DIGEST_SIZE],
				      &dummy_state.hash_state);

		/* Actual code for when key_size <= TC_SHA256_BLOCK_SIZE: */
		rekey(ctx->key, key, key_size);
	} else {
		(void)tc_sha256_init(&ctx->hash_state);
		(void)tc_sha256_update(&ctx->hash_state, key, key_size);
		(void)tc_sha256_final(&ctx->key[TC_SHA256_DIGEST_SIZE],
				      &ctx->hash_state);
		rekey(ctx->key,
		      &ctx->key[TC_SHA256_DIGEST_SIZE],
		      TC_SHA256_DIGEST_SIZE);
	}

	return TC_CRYPTO_SUCCESS;
}

int tc_hmac_init(TCHmacState_t ctx)
{

	/* input sanity check: */
	if (ctx == (TCHmacState_t) 0) {
		return TC_CRYPTO_FAIL;
	}

  (void) tc_sha256_init(&ctx->hash_state);
  (void) tc_sha256_update(&ctx->hash_state, ctx->key, TC_SHA256_BLOCK_SIZE);

	return TC_CRYPTO_SUCCESS;
}

int tc_hmac_update(TCHmacState_t ctx,
		   const void *data,
		   uint32_t data_length)
{

	/* input sanity check: */
	if (ctx == (TCHmacState_t) 0) {
		return TC_CRYPTO_FAIL;
	}

	(void)tc_sha256_update(&ctx->hash_state, data, data_length);

	return TC_CRYPTO_SUCCESS;
}

int tc_hmac_final(uint_least8_t *tag, uint32_t taglen, TCHmacState_t ctx)
{

	/* input sanity check: */
	if (tag == (uint_least8_t *) 0 ||
	    taglen != TC_SHA256_DIGEST_SIZE ||
	    ctx == (TCHmacState_t) 0) {
		return TC_CRYPTO_FAIL;
	}

	(void) tc_sha256_final(tag, &ctx->hash_state);

	(void)tc_sha256_init(&ctx->hash_state);
	(void)tc_sha256_update(&ctx->hash_state,
			       &ctx->key[TC_SHA256_BLOCK_SIZE],
				TC_SHA256_BLOCK_SIZE);
	(void)tc_sha256_update(&ctx->hash_state, tag, TC_SHA256_DIGEST_SIZE);
	(void)tc_sha256_final(tag, &ctx->hash_state);

	/* destroy the current state */
	_set(ctx, 0, sizeof(*ctx));

	return TC_CRYPTO_SUCCESS;
}
