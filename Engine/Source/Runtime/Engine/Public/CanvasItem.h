// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FCanvasItem.h: Unreal canvas item definitions
=============================================================================*/

#pragma once

class FCanvasItem
{
public:
	/* 
	 * Basic render item.
	 *
	 * @param	InPosition		Draw position
	 */
	FCanvasItem( const FVector2D& InPosition )
		: Position( InPosition )
		, BlendMode( SE_BLEND_Opaque )
		, bFreezeTime( false )
		, BatchedElementParameters( NULL )
		, Color( FLinearColor::White ) {};

	virtual void Draw( class FCanvas* InCanvas ) = 0;

	/* 
	 * Draw this item (this will affect the items position for future draw calls that do no specify a position)
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	InPosition		Draw position - this will not preserve the items position
	 */
	virtual void Draw( class FCanvas* InCanvas, const FVector2D& InPosition )
	{
		Position = InPosition;
		Draw( InCanvas );
	}

	/* 
	 * Draw this item (this will affect the items position for future draw calls that do no specify a position)
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	X				X Draw position 
	 * @param	Y				Y Draw position
	 */
	virtual void Draw( class FCanvas* InCanvas, float X, float Y )
	{
		Position.X = X;
		Position.Y = Y;
		Draw( InCanvas );
	}
		
	/* Set the Color of the item. */
	virtual void SetColor( const FLinearColor& InColor )
	{
		Color = InColor;
	}
	
	/* The position to draw the item. */
	FVector2D Position;

		/* Blend mode. */
	ESimpleElementBlendMode BlendMode;
		
	bool bFreezeTime;

	/* Used for batch rendering. */
	FBatchedElementParameters* BatchedElementParameters;
protected:
	/* Color of the item. */
	FLinearColor Color;
};


/* 'Tile' item can override size and UV . */
class ENGINE_API FCanvasTileItem : public FCanvasItem
{
public:
	/* 
	 * Tile item using size from texture.
	 *
	 * @param	InPosition		Draw position
	 * @param	InTexture		The texture
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FTexture* InTexture, const FLinearColor& InColor )
		: FCanvasItem( InPosition )
		, Z( 1.0f )
		, UV0( 0.0f, 0.0f )
		, UV1( 1.0f, 1.0f )
		, Texture( InTexture )
		, MaterialRenderProxy( NULL )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		SetColor( InColor );
		// Ensure texture is valid.
		check( InTexture );
		Size.X = InTexture->GetSizeX();
		Size.Y = InTexture->GetSizeY();
	}

	/* 
	 * Tile item with texture using given size. 
	 *
	 * @param	InPosition		Draw position
	 * @param	InTexture		The texture
	 * @param	InSize			The size to render
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InSize, const FLinearColor& InColor )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, Z( 1.0f )
		, UV0( 0.0f, 0.0f )
		, UV1( 1.0f, 1.0f )
		, Texture( InTexture )
		, MaterialRenderProxy( NULL )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		SetColor( InColor );
		// Ensure texture is valid
		check( InTexture );
	}

	/* 
	 * Tile item which uses the default white texture using given size. 
	 *
	 * @param	InPosition		Draw position
	 * @param	InSize			The size to render
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FVector2D& InSize, const FLinearColor& InColor );

	/* 
	 * Tile item with texture using size from texture specific UVs. 
	 *
	 * @param	InPosition		Draw position
	 * @param	InTexture		The texture
	 * @param	InUV0			UV coordinates (Normalized Top/Left)
	 * @param	InUV1			UV coordinates (Normalized Bottom/Right)
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InUV0, const FVector2D& InUV1, const FLinearColor& InColor )
		: FCanvasItem( InPosition )	 
		, Z( 1.0f )
		, UV0( InUV0 )
		, UV1( InUV1 )
		, MaterialRenderProxy( NULL )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		SetColor( InColor );
		// Ensure texture is valid.
		check( InTexture );
		
		Size.X = InTexture->GetSizeX();
		Size.Y = InTexture->GetSizeY();
	}

	/* 
	 * Tile item with texture using given size and specific UVs.
	 *
	 * @param	InPosition		Draw position
	 * @param	InTexture		The texture
	 * @param	InSize			The size to render
	 * @param	InUV0			UV coordinates (Normalized Top/Left)
	 * @param	InUV1			UV coordinates (Normalized Bottom/Right)
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InSize, const FVector2D& InUV0, const FVector2D& InUV1, const FLinearColor& InColor )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, Z( 1.0f )
		, UV0( InUV0 )
		, UV1( InUV1 )
		, Texture( InTexture )
		, MaterialRenderProxy( NULL )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		SetColor( InColor );
		// Ensure texture is valid.
		check( InTexture != NULL);
	}

	/* 
	 * Tile item with FMaterialRenderProxy using given size. 
	 *
	 * @param	InPosition				Draw position
	 * @param	InMaterialRenderProxy	Material proxy for rendering
	 * @param	InSize					The size to render
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FMaterialRenderProxy* InMaterialRenderProxy, const FVector2D& InSize )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, Z( 1.0f )
		, UV0( 0.0f, 0.0f )
		, UV1( 1.0f, 1.0f )
		, Texture( NULL )
		, MaterialRenderProxy( InMaterialRenderProxy )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		// Ensure specify Texture or material, but not both.
		check( InMaterialRenderProxy );
	}


	/* 
	 * Tile item with FMaterialRenderProxy using given size and UVs.
	 *
	 * @param	InPosition				Draw position
	 * @param	InMaterialRenderProxy	Material proxy for rendering
	 * @param	InSize					The size to render
	 * @param	InUV0					UV coordinates (Normalized Top/Left)
	 * @param	InUV1					UV coordinates (Normalized Bottom/Right)
	 */
	FCanvasTileItem( const FVector2D& InPosition, const FMaterialRenderProxy* InMaterialRenderProxy, const FVector2D& InSize, const FVector2D& InUV0, const FVector2D& InUV1 )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, Z( 1.0f )
		, UV0( InUV0 )
		, UV1( InUV1 )
		, Texture( NULL )
		, MaterialRenderProxy( InMaterialRenderProxy )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		// Ensure material proxy is valid.
		check( InMaterialRenderProxy );
	}


	/* 
	 * Draw the item at the given coordinates.
	 *
	 * @param	InPosition		Draw position.
	 */
	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;

	/* Expose the functions defined in the base class. */
	using FCanvasItem::Draw;

	/* Size of the tile. */
	FVector2D Size;

	/* used to calculate depth. */
	float Z;

	/* UV Coordinates 0 (Left/Top). */
	FVector2D UV0;
	
	/* UV Coordinates 0 (Right/Bottom). */
	FVector2D UV1;

	/* Texture to render. */
	const FTexture* Texture;
	
	/* Material proxy for rendering. */
	const FMaterialRenderProxy* MaterialRenderProxy;

	/* Rotation. */
	FRotator Rotation;
	
	/* Pivot point, as percentage of tile (0-1). */
	FVector2D	PivotPoint;

private:
	/* Render when we have a material proxy. */
	void RenderMaterialTile( class FCanvas* InCanvas, const FVector2D& InPosition );
};

/* Resizable 3x3 border item. */
class ENGINE_API FCanvasBorderItem : public FCanvasItem
{
public:

	/* 
	 * 3x3 grid border with tiled frame and tiled interior. 
	 *
	 * @param	InPosition		    Draw position
	 * @param	InBorderTexture		The texture to use for border
	 * @param	InBackgroundTexture	The texture to use for border background
	 * @param	InSize			    The size to render
	 * @param	InColor			    Tint of the border
	 */
	FCanvasBorderItem( const FVector2D& InPosition, const FTexture* InBorderTexture, const FTexture* InBackgroundTexture, 
		const FTexture* InBorderLeftTexture, const FTexture* InBorderRightTexture, const FTexture* InBorderTopTexture, const FTexture* InBorderBottomTexture,
		const FVector2D& InSize, const FLinearColor& InColor )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, BorderScale( FVector2D(1.0f,1.0f ))
		, BackgroundScale( FVector2D(1.0f,1.0f ))
		, Z( 1.0f )
		, BorderUV0( 0.0f, 0.0f )
		, BorderUV1( 1.0f, 1.0f )
		, BorderTexture( InBorderTexture )
		, BackgroundTexture( InBackgroundTexture )
		, BorderLeftTexture( InBorderLeftTexture )
		, BorderRightTexture( InBorderRightTexture )
		, BorderTopTexture( InBorderTopTexture )
		, BorderBottomTexture( InBorderBottomTexture )
		, Rotation( ForceInitToZero )
		, PivotPoint( FVector2D::ZeroVector )
	{
		SetColor( InColor );
		// Ensure textures is valid
		check( InBorderTexture );
		check( InBackgroundTexture );
	}

	/* 
	 * Draw the item at the given coordinates.
	 *
	 * @param	InPosition		Draw position.
	 */
	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;

	/* Expose the functions defined in the base class. */
	using FCanvasItem::Draw;

	/* Size of the border. */
	FVector2D Size;

	/** Scale of the border */
	FVector2D BorderScale;

	/** Scale of the border */
	FVector2D BackgroundScale;

	/* used to calculate depth. */
	float Z;

	/* Border UV Coordinates 0 (Left/Top). */
	FVector2D BorderUV0;
	
	/* Border UV Coordinates 1 (Right/Bottom). */
	FVector2D BorderUV1;

	/* Corners texture. */
	const FTexture* BorderTexture;

	/* Background tiling texture. */
	const FTexture* BackgroundTexture;

	/* Border left tiling texture. */
	const FTexture* BorderLeftTexture;

	/* Border right tiling texture. */
	const FTexture* BorderRightTexture;

	/* Border top tiling texture. */
	const FTexture* BorderTopTexture;

	/* Border bottom tiling texture. */
	const FTexture* BorderBottomTexture;
	
	/* Rotation. */
	FRotator Rotation;
	
	/* Pivot point. */
	FVector2D	PivotPoint;

	/** Frame corner size in percent of frame texture (should be < 0.5f) */
	FVector2D   CornerSize;
};

/* Text item with misc optional items such as shadow, centering etc. */
class ENGINE_API FCanvasTextItem : public FCanvasItem
{
public:
	/* 	 
	 * Text item
	 *
	 * @param	InPosition		Draw position
	 * @param	InText			String to draw
	 * @param	InFont			Font to draw with
	 */
	FCanvasTextItem( const FVector2D& InPosition, const FText& InText, class UFont* InFont, const FLinearColor& InColor )
		: FCanvasItem( InPosition )
		, Text( InText )
		, Font( InFont )
		, HorizSpacingAdjust( 0.0f )
	  	, ForcedViewportHeight( NULL )
		, Depth( 1.0f )
		, ShadowColor( FLinearColor::Black )
		, ShadowOffset( FVector2D::ZeroVector )
		, DrawnSize( FVector2D::ZeroVector )
		, bCentreX( false )
		, bCentreY( false )
		, bOutlined( false )
		, OutlineColor( FLinearColor::Black )
		, bDontCorrectStereoscopic( true )
		, TileItem( InPosition, FVector2D::ZeroVector, InColor )
	{
		SetColor( InColor );
		Scale.Set( 1.0f, 1.0f );
		BlendMode = SE_BLEND_Translucent;
	};
		
	/* 
	 * Set the shadow offset and color. 
	 *
	 * @param	InColor			Shadow color
	 * @param	InOffset		Shadow offset. Defaults to 1,1. (Passing zero vector will disable the shadow)	 
	 */
	void EnableShadow( const FLinearColor& InColor, const FVector2D& InOffset = FVector2D( 1.0f, 1.0f ) )
	{
		ShadowOffset = InOffset;
		ShadowColor = InColor;		
	}
	
	/* 
	 * Disable the shadow
	 */
	void DisableShadow() 
	{
		ShadowOffset = FVector2D::ZeroVector;
	}

	/* 
	 * Draw the item at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 */
	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;

	/* Expose the functions defined in the base class. */
	using FCanvasItem::Draw;

	/* The text to draw. */
	FText Text;
	
	/* Font to draw text with. */
	class UFont* Font;

	/* Horizontal spacing adjustment. */
	float HorizSpacingAdjust;
	
	/* Override viewport height. */
	float* ForcedViewportHeight;

	/* Depth sort key. */
	float Depth;

	/* Custom font render information. */
	FFontRenderInfo	FontRenderInfo;	
	
	/* The color of the shadow */
	FLinearColor ShadowColor;

	/* The offset of the shadow. */
	FVector2D ShadowOffset;

	/* The size of the drawn text after the draw call. */
	FVector2D DrawnSize;

	/* Centre the text in the viewport on horizontally. */
	bool	bCentreX;
	
	/* Centre the text in the viewport on vertically. */
	bool	bCentreY;

	/* Draw an outline on the text. */
	bool	bOutlined;

	/* The color of the outline. */
	FLinearColor OutlineColor;

	/* Disables correction of font render issue when using stereoscopic display. */
	bool	bDontCorrectStereoscopic;

	/* The scale of the text */
	FVector2D Scale;
protected:
	/* Background tile used to fixup 3d text issues. */
	FCanvasTileItem	TileItem;

	/* 
	 * Internal string draw
	 *
	 * In a method to make it simpler to do effects like shadow, outline
	 */
	void DrawStringInternal( class FCanvas* InCanvas, const FVector2D& DrawPos, const FLinearColor& DrawColor );

	/* 
	 * These are used bye the DrawStringInternal function. 
	 */
	/* String char length. */
	int32 TextLen;

	/* Used for batching. */
	FBatchedElements* BatchedElements;
	
	/* Font page index. */
	int32 PageIndex;
	
	/* Font scale (Read from font). */
	float FontScale;
	
	/* Render X scale (based on item scale and font scale). */
	float XScale;
	
	/* Render Y scale (based on item scale and font scale). */
	float YScale;

	float CharIncrement;

};

/* Line item. */
class ENGINE_API FCanvasLineItem : public FCanvasItem
{
public:
	FCanvasLineItem()
		: FCanvasItem( FVector2D::ZeroVector )
		, LineThickness( 0.0f )
	{
		Origin.X = 0.0f;
		Origin.Y = 0.0f;
		Origin.Z = 0.0f;
		EndPos.X = 0.0f;
		EndPos.Y = 0.0f;
		EndPos.Z = 0.0f;
	}
	/* 
	 * A Line. 
	 * 
	 * @param	InPosition		Start position
	 * @param	InEndPos		End position 
	 */
	FCanvasLineItem( const FVector2D& InPosition, const FVector2D& InEndPos )
		: FCanvasItem( InPosition )
		, LineThickness( 0.0f )
	{
		Origin.X = InPosition.X;
		Origin.Y = InPosition.Y;
		Origin.Z = 0.0f;
		EndPos.X = InEndPos.X;
		EndPos.Y = InEndPos.Y;
		EndPos.Z = 0.0f;
	}
	
	/* 
	 * A Line. 
	 * 
	 * @param	InPosition		Start position
	 * @param	InEndPos		End position 
	 */
	FCanvasLineItem( const FVector& InPosition, const FVector& InEndPos )
		: FCanvasItem( FVector2D( InPosition.X, InPosition.Y ) )
		, LineThickness( 0.0f )
	{
		Origin = InPosition;
		EndPos = InEndPos;
	}

	/* 
	 * Draw line at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 */
	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;
	
	/* 
	 * Draw line at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	InPosition		Draw Start position
	 */
	virtual void Draw( class FCanvas* InCanvas, const FVector2D& InPosition ) OVERRIDE
	{
		Origin.X = InPosition.X;
		Origin.Y = InPosition.Y;
		Draw( InCanvas );
	}
	/* 
	 * Draw line using the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	InStartPos		Line start position
	 * @param	InEndPos		Line end position
	 */
	virtual void Draw( class FCanvas* InCanvas, const FVector2D& InStartPos, const FVector2D& InEndPos )
	{
		Origin.X = InStartPos.X;
		Origin.Y = InStartPos.Y;
		EndPos.X = InEndPos.X;
		EndPos.Y = InEndPos.Y;
		Draw( InCanvas );
	}
	/* 
	 * Draw line at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	X				X Draw position 
	 * @param	Y				Y Draw position
	 */
	virtual void Draw( class FCanvas* InCanvas, float InX, float InY ) OVERRIDE
	{
		Origin.X = InX;
		Origin.Y = InY;
		Draw( InCanvas );
	}
		
	/* 
	 * Draw line at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	InPosition		Draw position
	 */
	virtual void Draw( class FCanvas* InCanvas, const FVector& InPosition )
	{
		Origin = InPosition;
		Draw( InCanvas );
	}

	/* 
	 * Draw line at the given coordinates.
	 *
	 * @param	InCanvas		Canvas on which to draw
	 * @param	X				X Draw position 
	 * @param	Y				Y Draw position
	 * @param	Z				Z Draw position
	 */
	virtual void Draw( class FCanvas* InCanvas, float X, float Y, float Z )
	{
		Origin.X = X;
		Origin.Y = Y;
		Origin.Z = Z;
		Draw( InCanvas );
	}

	/* 
	 * Set the line end position.
	 *
	 * @param	InEndPos		End position of the line
	 */
	void SetEndPos( const FVector2D& InEndPos )
	{
		EndPos.X = InEndPos.X;
		EndPos.Y = InEndPos.Y;
	}

	/* The origin of the line. */
	FVector		Origin;

	/* The end position of the line. */
	FVector		EndPos;

	/* The thickness of the line. */
	float		LineThickness;
};

class ENGINE_API FCanvasBoxItem : public FCanvasItem
{
public:
	FCanvasBoxItem( const FVector2D& InPosition, const FVector2D& InSize )
		: FCanvasItem( InPosition )
		, Size( InSize )
		, LineThickness( 0.0f )	{};

	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;

	/* Expose the functions defined in the base class. */
	using FCanvasItem::Draw;

	/* Size of the box. */
	FVector2D Size;

	/* The thickness of the line. */
	float		LineThickness;
private:
	void SetupBox();
	TArray< FVector > Corners;
};


class ENGINE_API FCanvasTriangleItem : public FCanvasItem
{
public:	
	/* 	 
	 * Triangle item (no texture)
	 *
	 * @param	InPointA		Point A of triangle
	 * @param	InPointB		Point B of triangle
	 * @param	InPointC		Point C of triangle
	 */
	FCanvasTriangleItem( const FVector2D& InPointA, const FVector2D& InPointB, const FVector2D& InPointC, const FTexture* InTexture )
		: FCanvasItem( InPointA )
		, Texture( InTexture )
		, BatchedElementParameters( NULL )
	{
		FCanvasUVTri SingleTri;
		SingleTri.V0_Pos = InPointA;
		SingleTri.V1_Pos = InPointB;
		SingleTri.V2_Pos = InPointC;
		
		SingleTri.V0_UV = FVector2D::ZeroVector;
		SingleTri.V1_UV = FVector2D::ZeroVector;
		SingleTri.V2_UV = FVector2D::ZeroVector;

		SingleTri.V0_Color = FLinearColor::White;
		SingleTri.V1_Color = FLinearColor::White;
		SingleTri.V2_Color = FLinearColor::White;

		TriangleList.Add( SingleTri );
	};

	/* 	 
	 * Triangle item
	 *
	 * @param	InPointA		Point A of triangle
	 * @param	InPointB		Point B of triangle
	 * @param	InPointC		Point C of triangle
	 * @param	InPointA		UV of Point A of triangle
	 * @param	InPointB		UV of Point B of triangle
	 * @param	InPointC		UV of Point C of triangle
	 * @param	InTexture		Texture
	 */
	FCanvasTriangleItem( const FVector2D& InPointA, const FVector2D& InPointB, const FVector2D& InPointC, const FVector2D& InTexCoordPointA, const FVector2D& InTexCoordPointB, const FVector2D& InTexCoordPointC, const FTexture* InTexture )
		:FCanvasItem( InPointA )
		, Texture( InTexture )
		, BatchedElementParameters( NULL )
	{
		FCanvasUVTri SingleTri;
		SingleTri.V0_Pos = InPointA;
		SingleTri.V1_Pos = InPointB;
		SingleTri.V2_Pos = InPointC;

		SingleTri.V0_UV = InTexCoordPointA;
		SingleTri.V1_UV = InTexCoordPointB;
		SingleTri.V2_UV = InTexCoordPointC;

		SingleTri.V0_Color = FLinearColor::White;
		SingleTri.V1_Color = FLinearColor::White;
		SingleTri.V2_Color = FLinearColor::White;
		TriangleList.Add( SingleTri );
	};

	/* 	 
	 * Triangle item
	 *
	 * @param	InSingleTri		Triangle struct
	 * @param	InTexture		Texture
	 */
	FCanvasTriangleItem( FCanvasUVTri InSingleTri, const FTexture* InTexture )
		:FCanvasItem( InSingleTri.V0_Pos )
		, Texture( InTexture )
		, BatchedElementParameters( NULL )
	{
		TriangleList.Add( InSingleTri );
	};

	/* 	 
	 * Triangle item
	 *
	 * @param	InTriangleList	List of triangles
	 * @param	InTexture		Texture
	 */
	FCanvasTriangleItem( const TArray< FCanvasUVTri >&	InTriangleList, const FTexture* InTexture )
		:FCanvasItem( FVector2D::ZeroVector )
		, Texture( InTexture )
		, BatchedElementParameters( NULL )
	{
		check( InTriangleList.Num() >= 1 );
		
		TriangleList = InTriangleList;
		Position = TriangleList[ 0 ].V0_Pos;
	};

	virtual ~FCanvasTriangleItem(){};

	/* Set all 3 points of triangle */
	void SetPoints( const FVector2D& InPointA, const FVector2D& InPointB, const FVector2D& InPointC )
	{
		check( TriangleList.Num() != 0 );
		TriangleList[0].V0_Pos = InPointA;
		TriangleList[0].V1_Pos = InPointB;
		TriangleList[0].V2_Pos = InPointC;
	}

	virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;
		
	/* Expose the functions defined in the base class. */
	using FCanvasItem::Draw;

	/* Set the Color of the item. */
	virtual void SetColor( const FLinearColor& InColor ) OVERRIDE;

	/* texture to use for triangle(s). */
	const FTexture* Texture;

	FBatchedElementParameters* BatchedElementParameters;

	/* List of triangles. */
	TArray< FCanvasUVTri >	TriangleList;
};


class ENGINE_API FCanvasNGonItem :  public FCanvasItem
{
public:	
	/* 	 
	 * NGon item Several texture tris with a common central point with a fixed radius. 
	 *
	 * @param	InPosition	List of triangles
	 * @param	InRadius	Size of the object
	 * @param	InNumSides	How many tris/sides the object has	
	 * @param	InTexture	Texture to render
	 * @param	InColor		Color to tint the texture with
	 */
	 FCanvasNGonItem( const FVector2D& InPosition, const FVector2D& InRadius, int32 InNumSides, const FTexture* InTexture, const FLinearColor& InColor )
	  : FCanvasItem( InPosition )
		, TriListItem( NULL )
		, Texture( InTexture )
	 {
		 Color = InColor;
		 check( InNumSides >= 3 );
		 TriangleList.SetNum( InNumSides );
		 SetupPosition( InPosition, InRadius );
	 }

	 /* 	 
	 * NGon item Several tris with a common central point with a fixed radius. 
	 *
	 * @param	InPosition	List of triangles
	 * @param	InNumSides	How many tris/sides the object has	
	 * @param	InTexture	Texture to render
	 * @param	InColor		Color to tint the texture with
	 */
	 FCanvasNGonItem( const FVector2D& InPosition, const FVector2D& InRadius, int32 InNumSides, const FLinearColor& InColor );

	 virtual ~FCanvasNGonItem()
	 {
		 delete TriListItem;
	 }
	 virtual void Draw( class FCanvas* InCanvas ) OVERRIDE;

	 /* 	 
	 * Regenerates the tri list for the object with a new central point and radius
	 *
	 * @param	InPosition	List of triangles
	 * @param	InRadius	Size of the object
	 */
	 void SetupPosition( const FVector2D& InPosition, const FVector2D& InRadius )
	 {
		 if( TriListItem )
		 {
			 delete TriListItem;
		 }		 
		 // todo: make this calculate the UVs correctly so if you pass a texture it will work as you would expect
		 float Angle = 0.0f;
		 int32 NumSides = TriangleList.Num();
		 FVector2D LastPoint = InPosition + FVector2D( InRadius.X*FMath::Cos(Angle), InRadius.Y*FMath::Sin(Angle) );
		 for(int32 i=1; i< NumSides+1; i++)
		 {
			 Angle = (2 * (float)PI) * (float)i/(float)NumSides;			 			 
			 TriangleList[ i - 1 ].V0_Pos = InPosition;
			 TriangleList[ i - 1 ].V0_Color = Color;
			 TriangleList[ i - 1 ].V1_Pos = LastPoint;
			 TriangleList[ i - 1 ].V1_Color = Color;
			 LastPoint = InPosition + FVector2D( InRadius.X*FMath::Cos(Angle), InRadius.Y*FMath::Sin(Angle) );
			 TriangleList[ i - 1 ].V2_Pos = LastPoint;
			 TriangleList[ i - 1 ].V2_Color = Color;
		 }
		 TriListItem = new FCanvasTriangleItem( TriangleList, Texture );
	 }

	 /* Set the Color of the item. */
	 virtual void SetColor( const FLinearColor& InColor ) OVERRIDE;

private:
	
	TArray< FCanvasUVTri >	TriangleList;
	FCanvasTriangleItem*	TriListItem;
	const FTexture* Texture;	
};


#if WITH_EDITOR
class ENGINE_API FCanvasItemTestbed
{
public:
	FCanvasItemTestbed(){};
	void  Draw( class FViewport* Viewport, class FCanvas* Canvas );
	struct LineVars
	{
		LineVars()
		{
			LineStart = FVector2D::ZeroVector;
			LineEnd = FVector2D::ZeroVector;
			LineMove = FVector2D::ZeroVector;
			LineMove2 = FVector2D::ZeroVector;
			bTestSet = false;
			Testangle = 0.0f;
		}
		FVector2D LineStart;
		FVector2D LineEnd;
		FVector2D LineMove;
		FVector2D LineMove2;
		bool bTestSet;
		float Testangle;
	};
	static LineVars TestLine;
	static bool	bTestState;
	static bool	bShowTestbed;
};
#endif // WITH_EDITOR

