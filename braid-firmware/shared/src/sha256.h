/*!
 * @file WjCryptLib_Sha256.h
 * @brief Implementation of SHA256 hash function.
 *
 * Original author: Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 * Modified by WaterJuice retaining Public Domain license.
 *
 * This is free and unencumbered software released into the public domain -
 * June 2013 waterjuice.org
 */

#pragma once

#include <stdint.h>

/*!
 * @struct Sha256Context
 * @brief SHA256 context structure.
 *
 * Contains the state of the SHA256 computation.
 */
typedef struct {
    uint64_t length;   /*!< Total length of the processed data. */
    uint32_t state[8]; /*!< The current state of the hash. */
    uint32_t curlen;   /*!< The current length of the data being processed. */
    uint8_t buf[64];   /*!< Buffer for storing data to be hashed. */
} Sha256Context;

#define SHA256_HASH_SIZE (256 / 8)

/*!
 * @struct SHA256_HASH
 * @brief SHA256 hash structure.
 *
 * Contains the final SHA256 hash.
 */
typedef struct {
    uint8_t bytes[SHA256_HASH_SIZE]; /*!< Byte array of the SHA256 hash. */
} SHA256_HASH;

/*!
 * @brief Initialises a SHA256 context.
 *
 * Use this to initialise or reset a SHA256 context.
 *
 * @param[out] Context Pointer to the SHA256 context to initialise.
 */
void Sha256Initialise(Sha256Context *Context);

/*!
 * @brief Adds data to the SHA256 context.
 *
 * This function processes the data and updates the internal state of the context.
 * Keep calling this function until all the data has been added. Then call Sha256Finalise
 * to calculate the hash.
 *
 * @param[in, out] Context Pointer to the SHA256 context.
 * @param[in] Buffer Pointer to the data to add.
 * @param[in] BufferSize Size of the data to add.
 */
void Sha256Update(Sha256Context *Context, void const *Buffer, uint32_t BufferSize);

/*!
 * @brief Performs the final calculation of the hash.
 *
 * This function returns the digest (a 32-byte buffer containing the 256-bit hash).
 * After calling this, Sha256Initialise must be used to reuse the context.
 *
 * @param[in, out] Context Pointer to the SHA256 context.
 * @param[out] Digest Pointer to the SHA256 hash structure to store the final hash.
 */
void Sha256Finalise(Sha256Context *Context, SHA256_HASH *Digest);

/*!
 * @brief Calculates the SHA256 hash of a buffer.
 *
 * This function combines Sha256Initialise, Sha256Update, and Sha256Finalise into one.
 * It calculates the SHA256 hash of the buffer.
 *
 * @param[in] Buffer Pointer to the data to hash.
 * @param[in] BufferSize Size of the data to hash.
 * @param[out] Digest Pointer to the SHA256 hash structure to store the final hash.
 */
void Sha256Calculate(void const *Buffer, uint32_t BufferSize, SHA256_HASH *Digest);
