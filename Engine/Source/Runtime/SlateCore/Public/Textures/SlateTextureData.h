// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FSlateTextureData.h: Declares the FFSlateTextureData structure.
=============================================================================*/

#pragma once


/**
 * Holds texture data for upload to a rendering resource
 */
struct SLATECORE_API FSlateTextureData
{
	FSlateTextureData( uint32 InWidth = 0, uint32 InHeight = 0, uint32 InStride = 0, const TArray<uint8>& InBytes = TArray<uint8>() )
		: Bytes(InBytes)
		, Width(InWidth)
		, Height(InHeight)
		, StrideBytes(InStride)
	{
		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	FSlateTextureData( const FSlateTextureData &Other )
		: Bytes( Other.Bytes )
		, Width( Other.Width )
		, Height( Other.Height )
		, StrideBytes( Other.StrideBytes )
	{
		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	FSlateTextureData& operator=( const FSlateTextureData& Other )
	{
		if( this != &Other )
		{
			SetRawData( Other.Width, Other.Height, Other.StrideBytes, Other.Bytes );
		}
		return *this;
	}

	~FSlateTextureData()
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	void SetRawData( uint32 InWidth, uint32 InHeight, uint32 InStride, const TArray<uint8>& InBytes )
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );

		Width = InWidth;
		Height = InHeight;
		StrideBytes = InStride;
		Bytes = InBytes;

		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}
	
	void Empty()
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
		Bytes.Empty();
	}

	uint32 GetWidth() const
	{
		return Width;
	}
	
	uint32 GetHeight() const
	{
		return Height;
	}

	uint32 GetStride() const
	{
		return StrideBytes;
	}

	const TArray<uint8>& GetRawBytes() const
	{
		return Bytes;
	}

	/** Accesses the raw bytes of already sized texture data */
	uint8* GetRawBytesPtr( )
	{
		return Bytes.GetTypedData();
	}

private:

	/** Raw uncompressed texture data */
	TArray<uint8> Bytes;

	/** Width of the texture */
	uint32 Width;

	/** Height of the texture */
	uint32 Height;

	/** The number of bytes of each pixel */
	uint32 StrideBytes;
};


typedef TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> FSlateTextureDataPtr;
typedef TSharedRef<FSlateTextureData, ESPMode::ThreadSafe> FSlateTextureDataRef;
