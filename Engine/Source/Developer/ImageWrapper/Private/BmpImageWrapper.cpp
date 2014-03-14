// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ImageWrapperPrivatePCH.h"
#include "BmpImageSupport.h"

/**
 * BMP image wrapper class.
 * This code was adapted from UTextureFactory::ImportTexture, but has not been throughly tested.
 */

FBmpImageWrapper::FBmpImageWrapper()
	: FImageWrapperBase()
{
}


void FBmpImageWrapper::Compress( int32 Quality )
{
	checkf(false, TEXT("BMP compression not supported"));
}


void FBmpImageWrapper::Uncompress( const ERGBFormat::Type InFormat, const int32 InBitDepth )
{
	const uint8* Buffer = CompressedData.GetData();

	const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader *)(Buffer + sizeof(FBitmapFileHeader));
	const FBitmapFileHeader* bmf   = (FBitmapFileHeader *)(Buffer + 0);
	if( (CompressedData.Num()>=sizeof(FBitmapFileHeader)+sizeof(FBitmapInfoHeader)) && Buffer[0]=='B' && Buffer[1]=='M' )
	{
		if( bmhdr->biCompression != BCBI_RGB )
		{
			UE_LOG(LogImageWrapper, Error, TEXT("RLE compression of BMP images not supported") );
			return;
		}
		if( bmhdr->biPlanes==1 && bmhdr->biBitCount==8 )
		{
			// Do palette.
			const uint8* bmpal = (uint8*)Buffer + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader);

			// Set texture properties.
			Width = bmhdr->biWidth;
			Height = bmhdr->biHeight;
			Format = ERGBFormat::BGRA;
			RawData.Empty(Height * Width * 4);
			RawData.AddUninitialized(Height * Width * 4);

			FColor* ImageData = (FColor*)RawData.GetData();

			// If the number for color palette entries is 0, we need to default to 2^biBitCount entries.  In this case 2^8 = 256
			int32 clrPaletteCount = bmhdr->biClrUsed ? bmhdr->biClrUsed : 256;
			TArray<FColor>	Palette;
			for( int32 i=0; i<clrPaletteCount; i++ )
				Palette.Add(FColor( bmpal[i*4+2], bmpal[i*4+1], bmpal[i*4+0], 255 ));
			while( Palette.Num()<256 )
				Palette.Add(FColor(0,0,0,255));

			// Copy upside-down scanlines.
			int32 SizeX = Width;
			int32 SizeY = Height;
			for(uint32 Y = 0;Y < bmhdr->biHeight;Y++)
			{
				for(uint32 X = 0;X < bmhdr->biWidth;X++)
				{
					ImageData[(SizeY - Y - 1) * SizeX + X] = Palette[*((uint8*)Buffer + bmf->bfOffBits + Y * Align(bmhdr->biWidth,4) + X)];
				}
			}
		}
		else if( bmhdr->biPlanes==1 && bmhdr->biBitCount==24 )
		{
			// Set texture properties.
			Width = bmhdr->biWidth;
			Height = bmhdr->biHeight;
			Format = ERGBFormat::BGRA;
			RawData.Empty(Height * Width * 4);
			RawData.AddUninitialized(Height * Width * 4);

			uint8* ImageData = RawData.GetData();

			// Copy upside-down scanlines.
			const uint8* Ptr = (uint8*)Buffer + bmf->bfOffBits;
			for( int32 y=0; y<(int32)bmhdr->biHeight; y++ ) 
			{
				uint8* DestPtr = &ImageData[(bmhdr->biHeight - 1 - y) * bmhdr->biWidth * 4];
				uint8* SrcPtr = (uint8*) &Ptr[y * Align(bmhdr->biWidth*3,4)];
				for( int32 x=0; x<(int32)bmhdr->biWidth; x++ )
				{
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = 0xFF;
				}
			}
		}
		else if( bmhdr->biPlanes==1 && bmhdr->biBitCount==32 )
		{
			// Set texture properties.
			Width = bmhdr->biWidth;
			Height = bmhdr->biHeight;
			Format = ERGBFormat::BGRA;
			RawData.Empty(Height * Width * 4);
			RawData.AddUninitialized(Height * Width * 4);

			uint8* ImageData = RawData.GetData();

			// Copy upside-down scanlines.
			const uint8* Ptr = (uint8*)Buffer + bmf->bfOffBits;
			for( int32 y=0; y<(int32)bmhdr->biHeight; y++ ) 
			{
				uint8* DestPtr = &ImageData[(bmhdr->biHeight - 1 - y) * bmhdr->biWidth * 4];
				uint8* SrcPtr = (uint8*) &Ptr[y * bmhdr->biWidth * 4];
				for( int32 x=0; x<(int32)bmhdr->biWidth; x++ )
				{
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = *SrcPtr++;
					*DestPtr++ = *SrcPtr++;
				}
			}
		}
		else if( bmhdr->biPlanes==1 && bmhdr->biBitCount==16 )
		{
			UE_LOG(LogImageWrapper, Error, TEXT("BMP 16 bit format no longer supported.") );
		}
		else
		{
			UE_LOG(LogImageWrapper, Error, TEXT("BMP uses an unsupported format (%i/%i)"), bmhdr->biPlanes, bmhdr->biBitCount );
		}
	}
}

bool FBmpImageWrapper::SetCompressed( const void* InCompressedData, int32 InCompressedSize )
{
	bool bResult = FImageWrapperBase::SetCompressed( InCompressedData, InCompressedSize );

	return bResult && LoadHeader();	// Fetch the variables from the header info
}

bool FBmpImageWrapper::LoadHeader()
{
	const uint8* Buffer = CompressedData.GetData();

	const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader *)(Buffer + sizeof(FBitmapFileHeader));
	const FBitmapFileHeader* bmf   = (FBitmapFileHeader *)(Buffer + 0);
	if( (CompressedData.Num()>=sizeof(FBitmapFileHeader)+sizeof(FBitmapInfoHeader)) && Buffer[0]=='B' && Buffer[1]=='M' )
	{
		if( bmhdr->biCompression != BCBI_RGB )
		{
			return false;
		}
		if( bmhdr->biPlanes==1 && ( bmhdr->biBitCount==8 || bmhdr->biBitCount==24 || bmhdr->biBitCount==32 ) )
		{
			// Set texture properties.
			Width = bmhdr->biWidth;
			Height = bmhdr->biHeight;
			Format = ERGBFormat::BGRA;
			return true;
		}
	}

	return false;
}
