// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "../../../ThirdParty/zlib/v1.2.8/include/Win64/VS2013/zlib.h"
#pragma comment( lib, "../../../ThirdParty/zlib/v1.2.8/lib/Win64/VS2013/zlibstatic.lib" )

// Taken from GenericPlatform.h

// Unsigned base types.
typedef unsigned char 		uint8;		// 8-bit  unsigned.
typedef unsigned short int	uint16;		// 16-bit unsigned.
typedef unsigned int		uint32;		// 32-bit unsigned.
typedef unsigned long long	uint64;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			int8;		// 8-bit  signed.
typedef signed short int	int16;		// 16-bit signed.
typedef signed int	 		int32;		// 32-bit signed.
typedef signed long long	int64;		// 64-bit signed.

// Taken from Compression.cpp

extern "C"
{
	/**
	 * Thread-safe abstract decompression routine. Decompresses memory from compressed buffer and writes it to uncompressed
	 * buffer. Returns actual uncompressed data size or a negative error code.
	 *
	 * @param	UncompressedBuffer			Buffer containing uncompressed data
	 * @param	UncompressedSize			Size of uncompressed data in bytes
	 * @param	CompressedBuffer			Buffer compressed data is going to be read from
	 * @param	CompressedSize				Size of CompressedBuffer data in bytes
	 * @return  Less than zero values are error codes if the uncompress fails, greater than zero is the uncompressed size in bytes
	 */
	__declspec(dllexport)
	int32 __cdecl UE4UncompressMemoryZLIB(void* UncompressedBuffer, int32 UncompressedSize, const void* CompressedBuffer, int32 CompressedSize)
	{
		// Zlib wants to use unsigned long.
		unsigned long ZCompressedSize	= CompressedSize;
		unsigned long ZUncompressedSize	= UncompressedSize;
	
		// Uncompress data.
		int32 Result = uncompress((uint8*)UncompressedBuffer, &ZUncompressedSize, (const uint8*)CompressedBuffer, ZCompressedSize);

		return (Result == Z_OK) ? (int32)ZUncompressedSize : Result;
	}

	/**
	 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
	 * buffer. Returns actual compressed data size or a negative error code.
	 *
	 * @param	CompressedBuffer			Buffer compressed data
	 * @param	CompressedSize				Size of CompressedBuffer data in bytes
	 * @param	UncompressedBuffer			Buffer containing uncompressed data is going to be read from
	 * @param	UncompressedSize			Size of uncompressed data in bytes
	 * @return  Less than zero values are error codes if the compress fails, greater than zero is the compressed size in bytes
	 */
	__declspec(dllexport)
	int32 __cdecl UE4CompressMemoryZLIB(void* CompressedBuffer, int32 CompressedSize, const void* UncompressedBuffer, int32 UncompressedSize)
	{
		// Zlib wants to use unsigned long.
		unsigned long ZCompressedSize = CompressedSize;
		unsigned long ZUncompressedSize = UncompressedSize;

		// Uncompress data.
		int32 Result = compress2((uint8*)CompressedBuffer, &ZCompressedSize, (const uint8*)UncompressedBuffer, ZUncompressedSize, Z_DEFAULT_COMPRESSION);

		return (Result == Z_OK) ? (int32)ZCompressedSize : Result;
	}

	/**
	 * Thread-safe file compression routine. Compresses memory from uncompressed buffer and writes it to compressed
	 * gzip file.
	 *
	 * @param	Path						File path to write a new compressed gzip file
	 * @param	UncompressedBuffer			Buffer containing uncompressed data is going to be read from
	 * @param	UncompressedSize			Size of uncompressed data in bytes
	 * @return  Less than zero values are error codes if the compress fails, Z_OK if succeeded.
	 */
	__declspec(dllexport)
	int32 __cdecl UE4CompressFileGZIP(const char* Path, const void* UncompressedBuffer, int32 UncompressedSize)
	{
		int32 ReturnVal = Z_ERRNO;

		// Zlib wants to use unsigned long.
		unsigned ZUncompressedSize = UncompressedSize;

		gzFile FilePtr = gzopen(Path, "wb");
		if (FilePtr == nullptr)
		{
			return ReturnVal;
		}

		int ZCompressedSize = gzwrite(FilePtr, UncompressedBuffer, ZUncompressedSize);
		
		if (ZCompressedSize == 0)
		{
			// Compression error in gzwrite()
			gzerror(FilePtr, &ReturnVal);
			gzclose(FilePtr);
		}
		else
		{
			ReturnVal = gzclose(FilePtr);
		}

		return ReturnVal;
	}
}