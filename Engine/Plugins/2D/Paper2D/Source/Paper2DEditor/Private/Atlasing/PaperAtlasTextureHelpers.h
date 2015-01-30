// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Copy texture data from one 32bpp image to another
static void CopyTextureData(const uint8* Source, uint8* Dest, uint32 SizeX, uint32 SizeY, uint32 BytesPerPixel, uint32 SourceStride, uint32 DestStride)
{
	const uint32 NumBytesPerRow = SizeX * BytesPerPixel;

	for (uint32 Y = 0; Y < SizeY; ++Y)
	{
		FMemory::Memcpy(
			Dest + (DestStride * Y),
			Source + (SourceStride * Y),
			NumBytesPerRow
			);
	}
}

// Read sprite data into the 4 bytes per pixel target array
static bool ReadSpriteTexture(TArray<uint8>& TargetBuffer, FIntPoint& OutSpriteSize, UPaperSprite* Sprite)
{
	const int32 BytesPerPixel = 4;
	const FVector2D SpriteSizeFloat = Sprite->GetSourceSize();
	const FIntPoint SpriteSize(FMath::TruncToInt(SpriteSizeFloat.X), FMath::TruncToInt(SpriteSizeFloat.Y));
	TargetBuffer.Empty();
	TargetBuffer.AddZeroed(SpriteSize.X * SpriteSize.Y * BytesPerPixel);

	check(Sprite->GetSourceTexture());
	FTextureSource& SourceData = Sprite->GetSourceTexture()->Source;
	if (SourceData.GetFormat() == TSF_BGRA8)
	{
		uint32 BytesPerPixel = SourceData.GetBytesPerPixel();
		uint8* OffsetSource = SourceData.LockMip(0) + (FMath::TruncToInt(Sprite->GetSourceUV().X) + FMath::TruncToInt(Sprite->GetSourceUV().Y) * SourceData.GetSizeX()) * BytesPerPixel;
		uint8* OffsetDest = TargetBuffer.GetData();
		CopyTextureData(OffsetSource, OffsetDest, SpriteSize.X, SpriteSize.Y, BytesPerPixel, SourceData.GetSizeX() * BytesPerPixel, SpriteSize.X * BytesPerPixel);
		SourceData.UnlockMip(0);
	}
	else
	{
		UE_LOG(LogPaper2DEditor, Error, TEXT("Sprite %s is not BGRA8, which isn't supported in atlases yet"), *(Sprite->GetPathName()));
	}

	OutSpriteSize = SpriteSize;
	return true;
}

// Fills the padding space with the correct values
static void PadSprite(const FPaperSpriteAtlasSlot& Slot, EPaperSpriteAtlasPadding PaddingType, int32 Padding, FIntPoint &SpriteSize, int32 AtlasWidth, int32 AtlasBytesPerPixel, TArray<uint8>& TextureData)
{
#define PIXEL(PIXEL__X, PIXEL__Y) TextureData[((Slot.Y + (PIXEL__Y)) * AtlasWidth + (Slot.X + (PIXEL__X))) * AtlasBytesPerPixel + (PixelIndex)]
	if (PaddingType == EPaperSpriteAtlasPadding::DilateBorder)
	{
		for (int32 X = 0; X < Padding; ++X)
		{
			for (int32 Y = 0; Y < SpriteSize.Y + Padding * 2; ++Y)
			{
				for (int32 PixelIndex = 0; PixelIndex < AtlasBytesPerPixel; ++PixelIndex)
				{
					int32 ClampedY = Padding + FMath::Clamp(Y - Padding, 0, SpriteSize.Y - 1);
					PIXEL(X, Y) = PIXEL(Padding, ClampedY);
					PIXEL(Padding + SpriteSize.X + X, Y) = PIXEL(Padding + SpriteSize.X - 1, ClampedY);
				}
			}
		}
		for (int32 Y = 0; Y < Padding; ++Y)
		{
			for (int32 X = 0; X < SpriteSize.X + Padding * 2; ++X)
			{
				for (int32 PixelIndex = 0; PixelIndex < AtlasBytesPerPixel; ++PixelIndex)
				{
					int32 ClampedX = Padding + FMath::Clamp(X - Padding, 0, SpriteSize.X - 1);
					PIXEL(X, Y) = PIXEL(ClampedX, Padding);
					PIXEL(X, Padding + SpriteSize.Y + Y) = PIXEL(ClampedX, Padding + SpriteSize.Y - 1);
				}
			}
		}
	}
	else if (PaddingType == EPaperSpriteAtlasPadding::PadWithZero)
	{
		for (int32 X = 0; X < Padding; ++X)
		{
			for (int32 Y = 0; Y < SpriteSize.Y + Padding * 2; ++Y)
			{
				for (int32 PixelIndex = 0; PixelIndex < AtlasBytesPerPixel; ++PixelIndex)
				{
					PIXEL(X, Y) = 0;
					PIXEL(Padding + SpriteSize.X + X, Y) = 0;
				}
			}
		}
		for (int32 Y = 0; Y < Padding; ++Y)
		{
			for (int32 X = 0; X < SpriteSize.X + Padding * 2; ++X)
			{
				for (int32 PixelIndex = 0; PixelIndex < AtlasBytesPerPixel; ++PixelIndex)
				{
					PIXEL(X, Y) = 0;
					PIXEL(X, Padding + SpriteSize.Y + Y) = 0;
				}
			}
		}
	}
#undef PIXEL
}

// Copies the sprite texture data at the position described by slot
static void CopySpriteToAtlasTextureData(TArray<uint8>& TextureData, int32 AtlasWidth, int32 AtlasHeight, int32 AtlasBytesPerPixel, EPaperSpriteAtlasPadding PaddingType, int32 Padding, UPaperSprite* Sprite, const FPaperSpriteAtlasSlot& Slot)
{
	FIntPoint SpriteSize;
	TArray<uint8> DummyBuffer;
	ReadSpriteTexture(DummyBuffer, /*out*/SpriteSize, Sprite);

	// Copy the texture into the texture buffer
	for (int32 Y = 0; Y < SpriteSize.Y; ++Y)
	{
		for (int32 X = 0; X < SpriteSize.X; ++X)
		{
			for (int32 PixelIndex = 0; PixelIndex < AtlasBytesPerPixel; ++PixelIndex)
			{
				TextureData[((Slot.Y + Y + Padding) * AtlasWidth + (Slot.X + X + Padding)) * AtlasBytesPerPixel + PixelIndex] = DummyBuffer[(Y * SpriteSize.X + X) * AtlasBytesPerPixel + PixelIndex];
			}
		}
	}

	// Padding
	PadSprite(Slot, PaddingType, Padding, SpriteSize, AtlasWidth, AtlasBytesPerPixel, TextureData);
}

static int32 ClampMips(int Width, int Height, int MipCount)
{
	int32 NumMips = 1;
	while (MipCount > 1 && (Width % 2) == 0 && (Height % 2) == 0)
	{
		Width /= 2;
		Height /= 2;
		MipCount--;
		NumMips++;
	}
	return NumMips;
}

static void GenerateMipChainARGB(const TArray<FPaperSpriteAtlasSlot>& Slots, TArray<uint8>& AtlasTextureData, int32 MipCount, int32 Width, int32 Height)
{
	int32 SourceMipOffset = 0;
	int32 SourceMipWidth = Width;
	int32 SourceMipHeight = Height;

	// Mask bitmap stores all valid pixels in the image, i.e. of the rects that contain data
	TArray<int8> MaskBitmap;
	MaskBitmap.AddZeroed(SourceMipWidth * SourceMipHeight);
	for (const FPaperSpriteAtlasSlot& Slot : Slots)
	{
		for (int32 Y = 0; Y < Slot.Height; ++Y)
		{
			for (int32 X = 0; X < Slot.Width; ++X)
			{
				MaskBitmap[(Slot.Y + Y) * SourceMipWidth + (Slot.X + X)] = 1;
			}
		}
	}

	int32 BytesPerPixel = 4;
	check(BytesPerPixel == 4); // Only support 4 bytes per pixel

	// Mipmap offset to mip level being written into
	int32 TargetMipOffset = SourceMipWidth * SourceMipHeight * BytesPerPixel;
	int32 TargetMipWidth = SourceMipWidth / 2;
	int32 TargetMipHeight = SourceMipHeight / 2;

	for (int32 MipIndex = 1; MipIndex < MipCount; ++MipIndex)
	{
		int32 MipLevelSize = TargetMipHeight * TargetMipWidth * BytesPerPixel;

		for (int32 Y = 0; Y < TargetMipHeight; ++Y)
		{
			for (int32 X = 0; X < TargetMipWidth; ++X)
			{
				int32 TotalChannelValues[4] = { 0, 0, 0, 0 };
				check(BytesPerPixel == 4); // refer to above

				int32 ValidPixelCount = 0;
				int32 Mask0 = MaskBitmap[(Y * 2 + 0) * SourceMipWidth + (X * 2 + 0)];
				int32 Mask1 = MaskBitmap[(Y * 2 + 0) * SourceMipWidth + (X * 2 + 1)];
				int32 Mask2 = MaskBitmap[(Y * 2 + 1) * SourceMipWidth + (X * 2 + 0)];
				int32 Mask3 = MaskBitmap[(Y * 2 + 1) * SourceMipWidth + (X * 2 + 1)];
				ValidPixelCount = Mask0 + Mask1 + Mask2 + Mask3;

				for (int32 PixelIndex = 0; PixelIndex < BytesPerPixel; ++PixelIndex)
				{
					TotalChannelValues[PixelIndex] += AtlasTextureData[SourceMipOffset + ((Y * 2 + 0) * SourceMipWidth + (X * 2 + 0)) * BytesPerPixel + PixelIndex];
					TotalChannelValues[PixelIndex] += AtlasTextureData[SourceMipOffset + ((Y * 2 + 0) * SourceMipWidth + (X * 2 + 1)) * BytesPerPixel + PixelIndex];
					TotalChannelValues[PixelIndex] += AtlasTextureData[SourceMipOffset + ((Y * 2 + 1) * SourceMipWidth + (X * 2 + 0)) * BytesPerPixel + PixelIndex];
					TotalChannelValues[PixelIndex] += AtlasTextureData[SourceMipOffset + ((Y * 2 + 1) * SourceMipWidth + (X * 2 + 1)) * BytesPerPixel + PixelIndex];
				}

				for (int32 PixelIndex = 0; PixelIndex < BytesPerPixel; ++PixelIndex)
				{
					uint8 TargetPixelValue = (ValidPixelCount > 0) ? (TotalChannelValues[PixelIndex] / ValidPixelCount) : 0;
					AtlasTextureData[TargetMipOffset + (Y * TargetMipWidth + X) * BytesPerPixel + PixelIndex] = TargetPixelValue;
				}
			}
		}

		// Downsample mask in place, if any of the 4 mask pixels == 1, the target mask should be 1
		for (int32 Y = 0; Y < TargetMipHeight; ++Y)
		{
			for (int32 X = 0; X < TargetMipWidth; ++X)
			{
				uint8 TargetPixelValue = 0;
				TargetPixelValue |= MaskBitmap[(Y * 2 + 0) * SourceMipWidth + (X * 2 + 0)];
				TargetPixelValue |= MaskBitmap[(Y * 2 + 0) * SourceMipWidth + (X * 2 + 1)];
				TargetPixelValue |= MaskBitmap[(Y * 2 + 1) * SourceMipWidth + (X * 2 + 0)];
				TargetPixelValue |= MaskBitmap[(Y * 2 + 1) * SourceMipWidth + (X * 2 + 1)];
				MaskBitmap[Y * TargetMipWidth + X] = TargetPixelValue;
			}
		}

		// Update for next mip
		SourceMipOffset = TargetMipOffset;
		SourceMipWidth = TargetMipWidth;
		SourceMipHeight = TargetMipHeight;
		TargetMipOffset += MipLevelSize;
		TargetMipWidth /= 2;
		TargetMipHeight /= 2;
	}
}

