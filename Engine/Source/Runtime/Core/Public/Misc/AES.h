// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define AES_BLOCK_SIZE 16


struct CORE_API FAES
{
	static const uint32 AESBlockSize = 16;

	/** 
	 * Class representing a 256 bit AES key
	 */
	struct FAESKey
	{
		uint8 Key[32];

		FAESKey()
		{
			Reset();
		}

		bool IsValid() const
		{
			uint32* Words = (uint32*)Key;
			for (int32 Index = 0; Index < sizeof(Key) / 4; ++Index)
			{
				if (Words[Index] != 0)
				{
					return true;
				}
			}
			return false;
		}

		void Reset()
		{
			FMemory::Memset(Key, 0, sizeof(Key));
		}
	};

	/**
	* Encrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key An FAESKey object containing the encryption key
	*/
	static void EncryptData(uint8* Contents, uint32 NumBytes, const FAESKey& Key);

	/**
	 * Encrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void EncryptData(uint8* Contents, uint32 NumBytes, const ANSICHAR* Key);

	/**
	* Encrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a byte array that is a 32 byte multiple length
	*/
	static void EncryptData(uint8* Contents, uint32 NumBytes, const uint8* KeyBytes, uint32 NumKeyBytes);

	/**
	* Decrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a null terminated string that is a 32 byte multiple length
	*/
	static void DecryptData(uint8* Contents, uint32 NumBytes, const FAESKey& Key);

	/**
	 * Decrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void DecryptData(uint8* Contents, uint32 NumBytes, const ANSICHAR* Key);

	/**
	* Decrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a byte array that is a 32 byte multiple length
	*/
	static void DecryptData(uint8* Contents, uint32 NumBytes, const uint8* KeyBytes, uint32 NumKeyBytes);
};
