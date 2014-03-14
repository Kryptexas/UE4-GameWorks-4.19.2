// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Structure holding information about where a texture is located in the atlas
 * Pointers to left and right children build a tree of texture rectangles so we can easily find optimal slots for textures
 */
struct FAtlasedTextureSlot
{
	/** Left child slot. If NULL there is no texture data here */
	FAtlasedTextureSlot* Left;
	/** Right child slot. If NULL there is no texture data here  */
	FAtlasedTextureSlot* Right;
	/** The X position of the character in the texture */
	uint32 X;
	/** The Y position of the character in the texture */
	uint32 Y;
	/** The width of the character */
	uint32 Width;
	/** The height of the character */
	uint32 Height;
	uint8 Padding;
	FAtlasedTextureSlot( uint32 InX, uint32 InY, uint32 InWidth, uint32 InHeight, uint8 InPadding )
		: Left(NULL)
		, Right(NULL)
		, X(InX)
		, Y(InY)
		, Width(InWidth)
		, Height(InHeight)
		, Padding(InPadding)
	
	{

	}
};


/**
 * Base class texture atlases in Slate
 */
class SLATE_API FSlateTextureAtlas
{
public:
	FSlateTextureAtlas( uint32 InWidth, uint32 InHeight, uint32 StrideBytes, uint32 InPadding )
		: AtlasData()
		, RootNode( NULL )
		, AtlasWidth( InWidth )
		, AtlasHeight( InHeight )
		, Stride( StrideBytes )
		, Padding( InPadding )
		, bNeedsUpdate( false )
	{
		InitAtlasData();
	}

	virtual ~FSlateTextureAtlas();

	/**
	 * Clears atlas data
	 */
	void Empty();

	/**
	 * Adds a texture to the atlas
	 *
	 * @param TextureWidth	Width of the texture
	 * @param TextureHeight	Height of the texture
	 * @param Data			Raw texture data
	 */
	const FAtlasedTextureSlot* AddTexture( uint32 TextureWidth, uint32 TextureHeight, const TArray<uint8>& Data );

	/** @return the width of the atlas */
	uint32 GetWidth() const { return AtlasWidth; }
	/** @return the height of the atlas */
	uint32 GetHeight() const { return AtlasHeight; }

	/** Marks the texture as dirty and needing its rendering resources updated */
	void MarkTextureDirty() { check( IsInGameThread() ); bNeedsUpdate = true; }
	
	/**
	 * Updates the texture used for rendering if needed
	 */
	virtual void ConditionalUpdateTexture() = 0;
	
protected:
	/** 
	 * Finds the optimal slot for a texture in the atlas starting the search from the root
	 * 
	 * @param Width The width of the texture we are adding
	 * @param Height The height of the texture we are adding
	 */
	const FAtlasedTextureSlot* FindSlotForTexture( uint32 InWidth, uint32 InHeight );


	/** 
	 * Destroys everything in the atlas starting at the provided node
	 *
	 * @param StartNode
	 */
	void DestroyNodes( FAtlasedTextureSlot* StartNode );
	
	/**
	 * Creates enough space for a single texture the width and height of the atlas
	 */
	void InitAtlasData();

	struct FCopyRowData
	{
		/** Source data to copy */
		const uint8* SrcData;
		/** Place to copy data to */
		uint8* DestData;
		/** The row number to copy */
		uint32 SrcRow;
		/** The row number to copy to */
		uint32 DestRow;
		/** The width of a source row */
		uint32 RowWidth;
		/** The width of the source texture */
		uint32 SrcTextureWidth;
		/** The width of the dest texture */
		uint32 DestTextureWidth;
		/** The stride of each pixel */
		uint32 DataStride;
		/** Padding in the destination */
		uint32 Padding;
		/** Whether to dialate the texture and put the extra data in the padding area */
		bool bDialateToPadding;
	};

	/**
	 * Copies a single row from a source texture to a dest texture
	 * Optionally dilating the texture by the padding amount
	 *
	 * @param CopyRowData	Information for how to copy a row
	 */
	void CopyRow( FCopyRowData& CopyRowData );

	/** 
	 * Copies texture data into the atlas at a given slot
	 *
	 * @param SlotToCopyTo	The occupied slot in the atlas where texture data should be copied to
	 * @param Data			The data to copy into the atlas
	 */
	void CopyDataIntoSlot( const FAtlasedTextureSlot* SlotToCopyTo, const TArray<uint8>& Data );

private:
	/** 
	 * Finds the optimal slot for a texture in the atlas.  
	 * Does this by doing a DFS over all existing slots to see if the texture can fit in 
	 * the empty area next to an existing slot
	 * 
	 * @param Start	The start slot to check
	 * @param Width The width of the texture we are adding
	 * @param Height The height of the texture we are adding
	 */
	const FAtlasedTextureSlot* FindSlotForTexture( FAtlasedTextureSlot& Start, uint32 InWidth, uint32 InHeight );

protected:
	/** Actual texture data contained in the atlas */
	TArray<uint8> AtlasData;
	/** Root node for the tree of data.  */
	FAtlasedTextureSlot* RootNode;
	/** Width of the atlas */
	uint32 AtlasWidth;
	/** Height of the atlas */
	uint32 AtlasHeight;
	/** Stride of the atlas in bytes */
	uint32 Stride;
	/** Padding on all sides of each texture in the atlas */
	uint32 Padding;
	/** True if this texture needs to have its rendering resources updated */
	bool bNeedsUpdate;
};
