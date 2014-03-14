// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AES.h: Handling of Advanced Encryption Standard
=============================================================================*/

#pragma once

#define AES_BLOCK_SIZE				16

struct FAES
{
	static const uint32 AESBlockSize = 16;

	static void EncryptData( uint8 *Contents, uint32 NumBytes );
	static void DecryptData( uint8 *Contents, uint32 NumBytes );
	/**
	 * Encrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void EncryptData(uint8* Contents, uint32 NumBytes, ANSICHAR* Key);
	/**
	 * Decrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void DecryptData(uint8* Contents, uint32 NumBytes, ANSICHAR* Key);
};


