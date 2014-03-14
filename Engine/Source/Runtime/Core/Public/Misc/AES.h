// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AES.h: Handling of Advanced Encryption Standard
=============================================================================*/

#pragma once

#define AES_BLOCK_SIZE				16

struct FAES
{
	static const uint32 AESBlockSize = 16;

	static void EncryptData( uint8 *Contents, uint32 FileSize );
	static void DecryptData( uint8 *Contents, uint32 FileSize );
};


