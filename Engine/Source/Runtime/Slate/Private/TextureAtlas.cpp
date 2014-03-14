// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "TextureAtlas.h"

FSlateTextureAtlas::~FSlateTextureAtlas()
{
	Empty();
}

void FSlateTextureAtlas::Empty()
{
	// Remove all nodes
	DestroyNodes( RootNode );
	RootNode = NULL;

	STAT( uint32 MemoryBefore = AtlasData.GetAllocatedSize() );

	// Clear all raw data
	AtlasData.Empty();

	STAT( uint32 MemoryAfter = AtlasData.GetAllocatedSize() );

	DEC_MEMORY_STAT_BY( STAT_SlateTextureAtlasMemory, MemoryBefore-MemoryAfter );
}

const FAtlasedTextureSlot* FSlateTextureAtlas::AddTexture( uint32 TextureWidth, uint32 TextureHeight, const TArray<uint8>& Data )
{
	// Find a spot for the character in the texture
	const FAtlasedTextureSlot* NewSlot = FindSlotForTexture( TextureWidth, TextureHeight );

	if( NewSlot )
	{
		CopyDataIntoSlot( NewSlot, Data );
		MarkTextureDirty();
	}

	return NewSlot;
}
	
const FAtlasedTextureSlot* FSlateTextureAtlas::FindSlotForTexture( uint32 InWidth, uint32 InHeight )
{
	return FindSlotForTexture( *RootNode, InWidth, InHeight );
}

void FSlateTextureAtlas::DestroyNodes( FAtlasedTextureSlot* StartNode )
{
	if( StartNode->Left )
	{
		DestroyNodes( StartNode->Left );
	}

	if( StartNode->Right )
	{
		DestroyNodes( StartNode->Right );
	}

	delete StartNode;
}
	
void FSlateTextureAtlas::InitAtlasData()
{
	check( RootNode == NULL && AtlasData.Num() == 0 );
	RootNode = new FAtlasedTextureSlot( 0, 0, AtlasWidth, AtlasHeight, Padding );

	AtlasData.AddZeroed(AtlasWidth*AtlasHeight*Stride);

	INC_MEMORY_STAT_BY( STAT_SlateTextureAtlasMemory, AtlasData.GetAllocatedSize() );
}

void FSlateTextureAtlas::CopyRow( FCopyRowData& CopyRowData )
{
	const uint8* Data = CopyRowData.SrcData;
	uint8* Start = CopyRowData.DestData;
	const uint32 SourceWidth = CopyRowData.SrcTextureWidth;
	const uint32 DestWidth = CopyRowData.DestTextureWidth;
	const uint32 CopyStride = CopyRowData.DataStride;
	const uint32 CopyPadding = CopyRowData.Padding;
	const uint32 SrcRow = CopyRowData.SrcRow;
	const uint32 DestRow = CopyRowData.DestRow;

	const uint8* SourceDataAddr = &Data[SrcRow*SourceWidth*CopyStride]; 
	uint8* DestDataAddr = &Start[DestRow*DestWidth*CopyStride + CopyPadding*CopyStride]; 
	FMemory::Memcpy( DestDataAddr, SourceDataAddr, SourceWidth * CopyStride ); 
	if( CopyRowData.bDialateToPadding ) 
	{ 
		const uint8* FirstPixel = SourceDataAddr; 
		const uint8* LastPixel = SourceDataAddr+((SourceWidth-1)*CopyStride); 
		uint8* DestDataAddrNoPadding = &Start[DestRow*DestWidth*CopyStride]; 
		FMemory::Memcpy(DestDataAddrNoPadding, FirstPixel, CopyStride ); 
		DestDataAddrNoPadding = &Start[DestRow*DestWidth*CopyStride+(CopyRowData.RowWidth-1)*CopyStride]; 
		FMemory::Memcpy(DestDataAddrNoPadding, LastPixel, CopyStride ); 
	} 
}

void FSlateTextureAtlas::CopyDataIntoSlot( const FAtlasedTextureSlot* SlotToCopyTo, const TArray<uint8>& Data )
{
	// Copy pixel data to the texture
	uint8* Start = &AtlasData[ SlotToCopyTo->Y*AtlasWidth*Stride + SlotToCopyTo->X*Stride ];
	
	// Account for same padding on each sides
	const uint32 AllPadding = Padding*2;
	// The width of the source texture without padding (actual width)
	const uint32 SourceWidth = SlotToCopyTo->Width-AllPadding; 

	FCopyRowData CopyRowData;
	CopyRowData.bDialateToPadding = Padding != 0;
	CopyRowData.DataStride = Stride;
	CopyRowData.DestData = Start;
	CopyRowData.SrcData = Data.GetData();
	CopyRowData.DestTextureWidth = AtlasWidth;
	CopyRowData.SrcTextureWidth = SourceWidth;
	CopyRowData.Padding = Padding;
	CopyRowData.RowWidth = SlotToCopyTo->Width;


	// Copy color data into padding for bilinear filtering. 
	// Not used if no padding (assumes sampling outside boundaries of the sub texture is not possible)
	if( Padding )
	{
		// Copy first color row into padding.  
		CopyRowData.SrcRow = 0;
		CopyRowData.DestRow = 0;

		CopyRow( CopyRowData );
		
	}

	// Copy each row of the texture
	for( uint32 Row = Padding; Row < SlotToCopyTo->Height-Padding; ++Row )
	{
		CopyRowData.SrcRow = Row-Padding;
		CopyRowData.DestRow = Row;

		CopyRow( CopyRowData );
	}

	if( Padding )
	{
		// Copy last color row into padding row for bilinear filtering
		CopyRowData.SrcRow = SlotToCopyTo->Height-AllPadding-1;
		CopyRowData.DestRow = SlotToCopyTo->Height-Padding;

		CopyRow( CopyRowData );
	}
}

const FAtlasedTextureSlot* FSlateTextureAtlas::FindSlotForTexture( FAtlasedTextureSlot& Start, uint32 InWidth, uint32 InHeight )
{
	// If there are left and right slots there are empty regions around this slot.  
	// It also means this slot is occupied by a texture
	if( Start.Left || Start.Right )
	{
		// Recursively search left for the smallest empty slot that can fit the texture
		if( Start.Left )
		{
			const FAtlasedTextureSlot* NewSlot = FindSlotForTexture( *Start.Left, InWidth, InHeight );
			if( NewSlot )
			{
				return NewSlot;
			}
		}

		// Recursively search left for the smallest empty slot that can fit the texture
		if( Start.Right )
		{
			const FAtlasedTextureSlot* NewSlot = FindSlotForTexture( *Start.Right, InWidth, InHeight );
			if( NewSlot )
			{
				return NewSlot;
			}
		}

		// Not enough space
		return NULL;
	}

	// Account for padding on both sides
	uint32 TotalPadding = Padding * 2;

	// This slot can't fit the character
	if( InWidth+TotalPadding > (Start.Width) || InHeight+TotalPadding > (Start.Height) )
	{
		// not enough space
		return NULL;
	}
	
	uint32 PaddedWidth = InWidth+TotalPadding;
	uint32 PaddedHeight = InHeight+TotalPadding;
	// The width and height of the new child node
	uint32 RemainingWidth =  FMath::Max<int32>(0,Start.Width - PaddedWidth);
	uint32 RemainingHeight = FMath::Max<int32>(0,Start.Height - PaddedHeight);


	// Split the remaining area around this slot into two children.
	if( RemainingHeight <= RemainingWidth )
	{
		// Split vertically
		Start.Left = new FAtlasedTextureSlot( Start.X, Start.Y + PaddedHeight, PaddedWidth, RemainingHeight, Padding );
		Start.Right = new FAtlasedTextureSlot( Start.X + PaddedWidth, Start.Y, RemainingWidth, Start.Height, Padding );
	}
	else
	{
		// Split horizontally
		Start.Left = new FAtlasedTextureSlot( Start.X + PaddedWidth, Start.Y, RemainingWidth, PaddedHeight, Padding );
		Start.Right = new FAtlasedTextureSlot( Start.X, Start.Y + PaddedHeight, Start.Width, RemainingHeight, Padding );
	}

	// Shrink the slot to the remaining area.
	Start.Width = InWidth+TotalPadding;
	Start.Height = InHeight+TotalPadding;

	return &Start;
}
