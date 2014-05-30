// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FBitmap

struct FBitmap
{
	FBitmap(UTexture2D* SourceTexture, int AlphaThreshold, int DefaultValue)
		: Width(0)
		, Height(0)
		, DefaultValue(DefaultValue)
	{
		ExtractFromTexture(SourceTexture, AlphaThreshold);
	}

	void ExtractFromTexture(UTexture2D* SourceTexture, int AlphaThreshold)
	{
		// use the source art if it exists
		FTextureSource* TextureSource = NULL;
		if ((SourceTexture != NULL) && SourceTexture->Source.IsValid())
		{
			switch (SourceTexture->Source.GetFormat())
			{
			case TSF_G8:
			case TSF_BGRA8:
				TextureSource = &(SourceTexture->Source);
				break;
			default:
				break;
			};
		}

		if (TextureSource != NULL)
		{
			TArray<uint8> TextureRawData;
			TextureSource->GetMipData(TextureRawData, 0);
			int32 BytesPerPixel = TextureSource->GetBytesPerPixel();
			ETextureSourceFormat PixelFormat = TextureSource->GetFormat();
			Width = TextureSource->GetSizeX();
			Height = TextureSource->GetSizeY();
			RawData.SetNumZeroed(Width * Height);

			for (int Y = 0; Y < Height; ++Y)
			{
				for (int X = 0; X < Width; ++X)
				{
					int32 PixelByteOffset = (X + Y * Width) * BytesPerPixel;
					const uint8* PixelPtr = TextureRawData.GetTypedData() + PixelByteOffset;
					FColor Color;
					if (PixelFormat == TSF_BGRA8)
					{
						Color = *((FColor*)PixelPtr);
					}
					else
					{
						checkSlow(PixelFormat == TSF_G8);
						const uint8 Intensity = *PixelPtr;
						Color = FColor(Intensity, Intensity, Intensity, Intensity);
					}

					if (Color.A > AlphaThreshold)
					{
						RawData[Y * Width + X] = 1;
					}
					else
					{
						RawData[Y * Width + X] = DefaultValue;
					}
				}
			}
		}
	}

	// Create an empty bitmap
	FBitmap(int Width, int Height, int DefaultValue)
		: Width(Width)
		, Height(Height)
		, DefaultValue(DefaultValue)
	{
		RawData.SetNumZeroed(Width * Height);
		ClearToDefaultValue();
	}

	bool IsValid() const
	{
		return Width != 0 && Height != 0;
	}

	uint8 GetPixel(int X, int Y)
	{
		if (X >= 0 && X < Width && Y >= 0 && Y < Height)
		{
			return RawData[Y * Width + X];
		}
		else
		{
			return DefaultValue;
		}
	}

	void SetPixel(int X, int Y, uint8 Value)
	{
		if (X >= 0 && X < Width && Y >= 0 && Y < Height)
		{
			RawData[Y * Width + X] = Value;
		}
	}

	void ClearToDefaultValue()
	{
		int RawDataCount = Width * Height;
		for (int RawDataIndex = 0; RawDataIndex < RawDataCount; ++RawDataIndex)
		{
			RawData[RawDataIndex] = DefaultValue;
		}
	}

	bool IsColumnEmpty(int X, int Y0, int Y1)
	{
		for (int Y = Y0; Y < Y1; ++Y)
		{
			if (GetPixel(X, Y) != 0)
			{
				return false;
			}
		}
		return true;
	}

	bool IsRowEmpty(int X0, int X1, int Y)
	{
		for (int X = X0; X < X1; ++X)
		{
			if (GetPixel(X, Y) != 0)
			{
				return false;
			}
		}
		return true;
	}

	// Returns the tight bounding box around pixels that are not 0
	void GetTightBounds(int& OutX, int& OutY, int& OutWidth, int& OutHeight)
	{
		int Top = 0;
		int Bottom = Height - 1;
		int Left = 0;
		int Right = Width - 1;
		while (Top < Bottom && IsRowEmpty(Left, Right, Top))
		{
			++Top;
		}
		while (Bottom >= Top && IsRowEmpty(Left, Right, Bottom))
		{
			--Bottom;
		}
		while (Left < Right && IsColumnEmpty(Left, Top, Bottom))
		{
			++Left;
		}
		while (Right >= Left && IsColumnEmpty(Right, Top, Bottom))
		{
			--Right;
		}

		OutX = Left;
		OutY = Top;
		OutWidth = Right - Left + 1;
		OutHeight = Bottom - Top + 1;
	}

	// Performs a flood fill on the target bitmap, with the boundary defined
	// by the current bitmap.
	void FloodFill(FBitmap& MaskBitmap, int StartX, int StartY)
	{
		TArray<FIntPoint> QueuedPoints;
		QueuedPoints.Reserve(Width);

		QueuedPoints.Add(FIntPoint(StartX, StartY));
		while (QueuedPoints.Num() > 0)
		{
			FIntPoint Point = QueuedPoints.Pop(false);
			if (MaskBitmap.GetPixel(Point.X, Point.Y) == 0 && GetPixel(Point.X, Point.Y) == 1)
			{
				MaskBitmap.SetPixel(Point.X, Point.Y, 1);
				if (Point.X > 0) 
				{
					QueuedPoints.Add(FIntPoint(Point.X - 1, Point.Y));
				}
				if (Point.X < Width - 1) 
				{
					QueuedPoints.Add(FIntPoint(Point.X + 1, Point.Y));
				}
				if (Point.Y > 0) 
				{
					QueuedPoints.Add(FIntPoint(Point.X, Point.Y - 1));
				}
				if (Point.Y < Height - 1) 
				{
					QueuedPoints.Add(FIntPoint(Point.X, Point.Y + 1));
				}
			}
		}
	}

	// Walk the border pixels to find any intersecting islands we haven't already filled in the mask bitmap
	bool HasOverlappingIsland(FBitmap& MaskBitmap, int X0, int Y0, int Width, int Height, int& FillX, int& FillY)
	{
		FillX = 0;
		FillY = 0;
		int X1 = X0 + Width - 1;
		int Y1 = Y0 + Height - 1;

		for (int X = X0; X <= X1; ++X)
		{
			if (MaskBitmap.GetPixel(X, Y0) == 0 && GetPixel(X, Y0) != 0)
			{
				FillX = X;
				FillY = Y0;
				return true;
			}
			if (MaskBitmap.GetPixel(X, Y1) == 0 && GetPixel(X, Y1) != 0)
			{
				FillX = X;
				FillY = Y1;
				return true;
			}
		}

		for (int Y = Y0; Y <= Y1; ++Y)
		{
			if (MaskBitmap.GetPixel(X0, Y) == 0 && GetPixel(X0, Y) != 0)
			{
				FillX = X0;
				FillY = Y;
				return true;
			}
			if (MaskBitmap.GetPixel(X1, Y) == 0 && GetPixel(X1, Y) != 0)
			{
				FillX = X1;
				FillY = Y;
				return true;
			}
		}

		return false;
	}

	// Sets pixels to 1 marking the rectangle outline
	void DrawRectOutline(int StartX, int StartY, int Width, int Height)
	{
		int X0 = StartX;
		int Y0 = StartY;
		int X1 = StartX + Width - 1;
		int Y1 = StartY + Height - 1;
		for (int Y = Y0; Y <= Y1; ++Y)
		{
			SetPixel(X0, Y, 1);
			SetPixel(X1, Y, 1);
		}
		for (int X = X0; X <= X1; ++X)
		{
			SetPixel(X, Y0, 1);
			SetPixel(X, Y1, 1);
		}
	}

	// Finds the rect of the contour of the shape clicked on, extended by rectangles to support
	// separated but intersecting islands.
	bool HasConnectedRect(int X, int Y, bool Extend, int& OutOriginX, int& OutOriginY, int& OutDimensionX, int& OutDimensionY)
	{
		if (GetPixel(X, Y) == 0) 
		{
			// selected an empty pixel
			OutOriginX = 0;
			OutOriginY = 0;
			OutDimensionX = 0;
			OutDimensionY = 0;
			return false;
		}

		// Tmp get rid of this
		// This whole thing can be much more efficiently using the 8 bpp
		FBitmap MaskBitmap(Width, Height, 0);
		if (Extend)
		{
			MaskBitmap.DrawRectOutline(OutOriginX, OutOriginY, OutDimensionX, OutDimensionY);
		}

		// MaxPasses shouldn't be nessecary, but worst case interlocked pixel
		// patterns can cause problems. Dilating the bitmap before processing will
		// reduce these problems, but may not be desirable in all cases.
		int NumPasses = 0;
		int MaxPasses = 40;
		OutOriginX = 0;
		OutOriginY = 0;
		OutDimensionX = 0;
		OutDimensionY = 0;
		int FillPointX = X;
		int FillPointY = Y;
		do
		{
			// This is probably going to be a botleneck at larger texture sizes
			// A contour tracing algorithm will probably suffice here.
			FloodFill(MaskBitmap, FillPointX, FillPointY);
			MaskBitmap.GetTightBounds(OutOriginX, OutOriginY, OutDimensionX, OutDimensionY);
		} while (NumPasses++ < MaxPasses && HasOverlappingIsland(MaskBitmap, OutOriginX, OutOriginY, OutDimensionX, OutDimensionY, /*out*/FillPointX, /*out*/FillPointY));

		return true;
	}

	// Wind through the bitmap from StartX, StartY to find the closest hit point
	bool FoundClosestValidPoint(int StartX, int StartY, int MaxAllowedSearchDistance, int& HitX, int& HitY)
	{
		// Constrain the point within the image bounds
		StartX = FMath::Clamp(StartX, 0, Width - 1);
		StartY = FMath::Clamp(StartY, 0, Height - 1);
		HitX = StartX;
		HitY = StartY;

		// Should probably calculate a better max based on StartX and StartY
		int RequiredSearchDistance = (FMath::Max(Width, Height) + 1) / 2;
		int MaxSearchDistance = FMath::Min(RequiredSearchDistance, MaxAllowedSearchDistance);
		int SearchDistance = 0;
		while (SearchDistance < MaxSearchDistance)
		{
			int X0 = FMath::Max(StartX - SearchDistance, 0);
			int X1 = FMath::Min(StartX + SearchDistance, Width - 1);
			int Y0 = FMath::Max(StartY - SearchDistance, 0);
			int Y1 = FMath::Min(StartY + SearchDistance, Height - 1);
			// Search along the rectangular edge
			for (int Y = Y0; Y <= Y1; ++Y)
			{
				if (GetPixel(X0, Y) != 0) 
				{
					HitX = X0;
					HitY = Y;
					return true;
				}
				if (GetPixel(X1, Y) != 0) 
				{
					HitX = X1;
					HitY = Y;
					return true;
				}
			}
			for (int X = X0; X <= X1; ++X)
			{
				if (GetPixel(X, Y0) != 0) 
				{
					HitX = X;
					HitY = Y0;
					return true;
				}
				if (GetPixel(X, Y1) != 0) 
				{
					HitX = X;
					HitY = Y1;
					return true;
				}
			}
			SearchDistance += 1;
		}
		return false;
	}

	TArray<uint8> RawData;
	int32 Width;
	int32 Height;
	int32 DefaultValue;
};


