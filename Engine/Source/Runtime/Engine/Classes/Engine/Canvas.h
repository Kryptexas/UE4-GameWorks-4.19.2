// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Canvas: A drawing canvas.
//=============================================================================

#pragma once
#include "WordWrapper.h"
#include "Canvas.generated.h"

/**
 * Holds texture information with UV coordinates as well.
 */
USTRUCT()
struct FCanvasIcon
{
	GENERATED_USTRUCT_BODY()

	/** Source texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasIcon)
	class UTexture* Texture;

	/** UV coords */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasIcon)
	float U;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasIcon)
	float V;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasIcon)
	float UL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasIcon)
	float VL;


	FCanvasIcon()
		: Texture(NULL)
		, U(0)
		, V(0)
		, UL(0)
		, VL(0)
	{
	}

};

/** info for glow when using depth field rendering */
USTRUCT()
struct FDepthFieldGlowInfo
{
	GENERATED_USTRUCT_BODY()

	/** whether to turn on the outline glow (depth field fonts only) */
	UPROPERTY()
	uint32 bEnableGlow:1;

	/** base color to use for the glow */
	UPROPERTY()
	FLinearColor GlowColor;

	/** if bEnableGlow, outline glow outer radius (0 to 1, 0.5 is edge of character silhouette)
	 * glow influence will be 0 at GlowOuterRadius.X and 1 at GlowOuterRadius.Y
	*/
	UPROPERTY()
	FVector2D GlowOuterRadius;

	/** if bEnableGlow, outline glow inner radius (0 to 1, 0.5 is edge of character silhouette)
	 * glow influence will be 1 at GlowInnerRadius.X and 0 at GlowInnerRadius.Y
	 */
	UPROPERTY()
	FVector2D GlowInnerRadius;


	FDepthFieldGlowInfo()
		: bEnableGlow(false)
		, GlowColor(ForceInit)
		, GlowOuterRadius(ForceInit)
		, GlowInnerRadius(ForceInit)
	{
	}


		bool operator==(const FDepthFieldGlowInfo& Other) const
		{
			if (Other.bEnableGlow != bEnableGlow)
			{
				return false;
			}
			else if (!bEnableGlow)
			{
				// if the glow is disabled on both, the other values don't matter
				return true;
			}
			else
			{
				return (Other.GlowColor == GlowColor && Other.GlowOuterRadius == GlowOuterRadius && Other.GlowInnerRadius == GlowInnerRadius);
			}
		}
		bool operator!=(const FDepthFieldGlowInfo& Other) const
		{
			return !(*this == Other);
		}
	
};

/** information used in font rendering */
USTRUCT()
struct FFontRenderInfo
{
	GENERATED_USTRUCT_BODY()

	/** whether to clip text */
	UPROPERTY()
	uint32 bClipText:1;

	/** whether to turn on shadowing */
	UPROPERTY()
	uint32 bEnableShadow:1;

	/** depth field glow parameters (only usable if font was imported with a depth field) */
	UPROPERTY()
	struct FDepthFieldGlowInfo GlowInfo;

	FFontRenderInfo()
		: bClipText(false), bEnableShadow(false)
	{}
};

/** Simple 2d triangle with UVs */
USTRUCT()
struct FCanvasUVTri
{
	GENERATED_USTRUCT_BODY()

	/** Position of first vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V0_Pos;

	/** UV of first vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V0_UV;

	/** Color of first vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FLinearColor V0_Color;

	/** Position of second vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V1_Pos;

	/** UV of second vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V1_UV;

	/** Color of second vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FLinearColor V1_Color;
	
	/** Position of third vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V2_Pos;

	/** UV of third vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FVector2D V2_UV;

	/** Color of third vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CanvasUVTri)
	FLinearColor V2_Color;

	FCanvasUVTri()
		: V0_Pos(ForceInit)
		, V0_UV(ForceInit)
		, V0_Color(ForceInit)
		, V1_Pos(ForceInit)
		, V1_UV(ForceInit)
		, V1_Color(ForceInit)
		, V2_Pos(ForceInit)
		, V2_UV(ForceInit)
		, V2_Color(ForceInit)
	{
	}

};

template <> struct TIsZeroConstructType<FCanvasUVTri> { enum { Value = true }; };

/**
 * General purpose data structure for grouping all parameters needed when sizing or wrapping a string
 */
USTRUCT(transient)
struct FTextSizingParameters
{
	GENERATED_USTRUCT_BODY()

	/** a pixel value representing the horizontal screen location to begin rendering the string */
	UPROPERTY()
	float DrawX;

	/** a pixel value representing the vertical screen location to begin rendering the string */
	UPROPERTY()
	float DrawY;

	/** a pixel value representing the width of the area available for rendering the string */
	UPROPERTY()
	float DrawXL;

	/** a pixel value representing the height of the area available for rendering the string */
	UPROPERTY()
	float DrawYL;

	/** A value between 0.0 and 1.0, which represents how much the width/height should be scaled, where 1.0 represents 100% scaling. */
	UPROPERTY()
	FVector2D Scaling;

	/** the font to use for sizing/wrapping the string */
	UPROPERTY()
	class UFont* DrawFont;

	/** Horizontal spacing adjustment between characters and vertical spacing adjustment between wrapped lines */
	UPROPERTY()
	FVector2D SpacingAdjust;

	FTextSizingParameters()
		: DrawX(0)
		, DrawY(0)
		, DrawXL(0)
		, DrawYL(0)
		, Scaling(ForceInit)
		, DrawFont(NULL)
		, SpacingAdjust(ForceInit)
	{
	}


		FTextSizingParameters( float inDrawX, float inDrawY, float inDrawXL, float inDrawYL, UFont* inFont=NULL )
		: DrawX(inDrawX), DrawY(inDrawY), DrawXL(inDrawXL), DrawYL(inDrawYL)
		, Scaling(1.f,1.f), DrawFont(inFont)
		, SpacingAdjust( 0.0f, 0.0f )
		{
		}
		FTextSizingParameters( UFont* inFont, float ScaleX, float ScaleY)
		: DrawX(0.f), DrawY(0.f), DrawXL(0.f), DrawYL(0.f)
		, Scaling(ScaleX,ScaleY), DrawFont(inFont)
		, SpacingAdjust( 0.0f, 0.0f )
		{
		}
	
};

/**
 * Used by UUIString::WrapString to track information about each line that is generated as the result of wrapping.
 */
USTRUCT(transient)
struct FWrappedStringElement
{
	GENERATED_USTRUCT_BODY()

	/** the string associated with this line */
	UPROPERTY()
	FString Value;

	/** the size (in pixels) that it will take to render this string */
	UPROPERTY()
	FVector2D LineExtent;


	FWrappedStringElement()
		: LineExtent(ForceInit)
	{
	}


		/** Constructor */
		FWrappedStringElement( const TCHAR* InValue, float Width, float Height )
		: Value(InValue), LineExtent(Width,Height)
		{}
	
};

UCLASS(transient)
class ENGINE_API UCanvas : public UObject
{
	GENERATED_UCLASS_BODY()

	// Modifiable properties.
	UPROPERTY()
	float OrgX;		// Origin for drawing in X.

	UPROPERTY()
	float OrgY;    // Origin for drawing in Y.

	UPROPERTY()
	float ClipX;	// Bottom right clipping region.

	UPROPERTY()
	float ClipY;    // Bottom right clipping region.
		
	UPROPERTY()
	FColor DrawColor;    // Color for drawing.

	UPROPERTY()
	uint32 bCenterX:1;    // Whether to center the text horizontally (about CurX)

	UPROPERTY()
	uint32 bCenterY:1;    // Whether to center the text vertically (about CurY)

	UPROPERTY()
	uint32 bNoSmooth:1;    // Don't bilinear filter.

	UPROPERTY()
	int32 SizeX;		// Zero-based actual dimensions X.

	UPROPERTY()
	int32 SizeY;		// Zero-based actual dimensions Y.

	// Internal.
	UPROPERTY()
	FPlane ColorModulate; 

	UPROPERTY()
	class UTexture2D* DefaultTexture; //Default texture to use 

	UPROPERTY()
	class UTexture2D* GradientTexture0; //Default texture to use 

	int32 UnsafeSizeX;   // Canvas size before safe frame adjustment
	int32 UnsafeSizeY;	// Canvas size before safe frame adjustment

	//cached data for safe zone calculation.  Some platforms have very expensive functions
	//to grab display metrics
	int32 SafeZonePadX;
	int32 SafeZonePadY;
	int32 CachedDisplayWidth;
	int32 CachedDisplayHeight;

public:
	FCanvas* Canvas;
	FSceneView* SceneView;
	FMatrix	ViewProjectionMatrix;
	FQuat HmdOrientation;	

	// UCanvas interface.
	
	/** Initialise Canvas */
	void Init(int32 InSizeX, int32 InSizeY, FSceneView* InSceneView);

	/** Changes the view for the canvas */
	void SetView(FSceneView* InView);

	/** Update Canvas */
	void Update();

	/* Applies the current Platform's safezone to the current Canvas position.  Automatically called by Update */
	void ApplySafeZoneTransform();
	void PopSafeZoneTransform();

	/* Updates cached SafeZone data from the device.  Call when main device is resized */
	void UpdateSafeZoneData();

	/* Function to go through all constructed canvas items and update their safezone data */
	static void UpdateAllCanvasSafeZoneData();
	

	/** 
	 * Draw arbitrary aligned rectangle.
	 * @param Tex - Texture to draw
	 * @param X - Position to draw X
	 * @param Y - Position to draw Y
	 * @param Z - Position to draw Z
	 * @param XL - Width of tile
	 * @param YL - Height of tile
	 * @param U - Horizontal position of the upper left corner of the portion of the texture to be shown(texels)
	 * @param V - Vertical position of the upper left corner of the portion of the texture to be shown(texels)
	 * @param UL - The width of the portion of the texture to be drawn(texels). 
	 * @param VL - The height of the portion of the texture to be drawn(texels). 
	 * @param ClipTile - true to clip tile
	 * @param BlendMode - blending mode of texture
	 */
	void DrawTile(UTexture* Tex, float X, float Y, float Z, float XL, float YL, float U, float V, float UL, float VL, EBlendMode BlendMode=BLEND_Translucent);

	/** calculate the length of a string 
	 * @param Font - font used
	 * @param ScaleX - scale in X axis
	 * @param ScaleY - scale in Y axis
	 * @param XL out - horizontal length of string
	 * @param YL out - vertical length of string
	 * @param Text - string to calculate for
	 */
	static void ClippedStrLen(UFont* Font, float ScaleX, float ScaleY, int32& XL, int32& YL, const TCHAR* Text);

	/**	
	 * Calculate the size of a string built from a font, word wrapped
	 * to a specified region.
	 */
	void VARARGS WrappedStrLenf(UFont* Font, float ScaleX, float ScaleY, int32& XL, int32& YL, const TCHAR* Fmt, ...);
		
	/**
	 * Compute size and optionally print text with word wrap.
	 */
	int32 WrappedPrint(bool Draw, float X, float Y, int32& out_XL, int32& out_YL, UFont* Font, float ScaleX, float ScaleY, bool bCenterTextX, bool bCenterTextY, const TCHAR* Text, const FFontRenderInfo& RenderInfo) ;
	
	/** Draws a string of text to the screen 
	 * @param InFont - font to draw with
	 * @param InText - string to be drawn
	 * @param X - Position to draw X
	 * @param Y - Position to draw Y
	 * @param XScale - Optional. The horizontal scaling to apply to the text. 
	 * @param YScale - Optional. The vertical scaling to apply to the text. 
	 * @param RenderInfo - Optional. The FontRenderInfo to use when drawing the text.
	 *
	 * returns the Y extent of the rendered text
	 */
	float DrawText(UFont* InFont, const FString& InText, float X, float Y, float XScale = 1.f, float YScale = 1.f, const FFontRenderInfo& RenderInfo = FFontRenderInfo());

	enum ELastCharacterIndexFormat
	{
		// The last whole character before the horizontal offset
		LastWholeCharacterBeforeOffset,
		// The character directly at the offset
		CharacterAtOffset,
		// Not used
		Unused,
	};

	/** 
	 * Measures a string, optionally stopped after the specified horizontal offset in pixels is reached.
	 * 
	 * @param	Parameters	Used for various purposes
	 *							DrawXL:		[out] will be set to the width of the string
	 *							DrawYL:		[out] will be set to the height of the string
	 *							DrawFont:	[in] specifies the font to use for retrieving the size of the characters in the string
	 *							Scale:		[in] specifies the amount of scaling to apply to the string
	 * @param	pText		the string to calculate the size for
	 * @param	TextLength	the number of code units in pText
	 * @param	StopAfterHorizontalOffset  Offset horizontally into the string to stop measuring characters after, in pixels (or INDEX_NONE)
	 * @param	CharIndexFormat  Behavior to use for StopAfterHorizontalOffset
	 * @param	OutCharacterIndex  The index of the last character processed (used with StopAfterHorizontalOffset)
	 *
	 */
	static void MeasureStringInternal( FTextSizingParameters& Parameters, const TCHAR* const pText, const int32 TextLength, const int32 StopAfterHorizontalOffset, const ELastCharacterIndexFormat CharIndexFormat, int32& OutLastCharacterIndex );

	/**
	 * Calculates the size of the specified string.
	 *
	 * @param	Parameters	Used for various purposes
	 *							DrawXL:		[out] will be set to the width of the string
	 *							DrawYL:		[out] will be set to the height of the string
	 *							DrawFont:	[in] specifies the font to use for retrieving the size of the characters in the string
	 *							Scale:		[in] specifies the amount of scaling to apply to the string
	 * @param	pText		the string to calculate the size for
	 */
	static void CanvasStringSize( FTextSizingParameters& Parameters, const TCHAR* pText );


	/**
	 * Parses a single string into an array of strings that will fit inside the specified bounding region.
	 *
	 * @param	Parameters		Used for various purposes:
	 *							DrawX:		[in] specifies the pixel location of the start of the horizontal bounding region that should be used for wrapping.
	 *							DrawY:		[in] specifies the Y origin of the bounding region.  This should normally be set to 0, as this will be
	 *										     used as the base value for DrawYL.
	 *										[out] Will be set to the Y position (+YL) of the last line, i.e. the total height of all wrapped lines relative to the start of the bounding region
	 *							DrawXL:		[in] specifies the pixel location of the end of the horizontal bounding region that should be used for wrapping
	 *							DrawYL:		[in] specifies the height of the bounding region, in pixels.  A input value of 0 indicates that
	 *										     the bounding region height should not be considered.  Once the total height of lines reaches this
	 *										     value, the function returns and no further processing occurs.
	 *							DrawFont:	[in] specifies the font to use for retrieving the size of the characters in the string
	 *							Scale:		[in] specifies the amount of scaling to apply to the string
	 * @param	CurX			specifies the pixel location to begin the wrapping; usually equal to the X pos of the bounding region, unless wrapping is initiated
	 *								in the middle of the bounding region (i.e. indentation)
	 * @param	pText			the text that should be wrapped
	 * @param	out_Lines		[out] will contain an array of strings which fit inside the bounding region specified.  Does
	 *							not clear the array first.
	 * @param	OutWrappedLineData An optional array to fill with the indices from the source string marking the begin and end points of the wrapped lines
	 */
	static void WrapString( FTextSizingParameters& Parameters, const float InCurX, const TCHAR* const pText, TArray<FWrappedStringElement>& out_Lines, FWordWrapper::FWrappedLineData* const OutWrappedLineData = nullptr);

	/** Transforms a 3D world-space vector into 2D screen coordinates. */
	FVector Project(FVector Location);

	/** 
	 * Transforms 2D screen coordinates into a 3D world-space origin and direction.
	 * @param ScreenPos - screen coordinates in pixels
	 * @param WorldOrigin (out) - world-space origin vector
	 * @param WorldDirection (out) - world-space direction vector
	 */
	void Deproject(FVector2D ScreenPos, /*out*/ FVector& WorldOrigin, /*out*/ FVector& WorldDirection);

	/**
	 * Calculate the length of a string, taking text wrapping into account.
	 * @param InFont - The Font use
	 * @param Text	 - string to calculate for
	 * @param XL out - horizontal length of string
	 * @param YL out - vertical length of string
	 */
	void StrLen(UFont* InFont, const FString& InText, float& XL, float& YL);

	/** 
	 * Calculates the horizontal and vertical size of a given string. This is used for clipped text as it does not take wrapping into account.
	 * @param Font	 - Font 
	 * @param Text	 - string to calculate for
	 * @param XL out - horizontal length of string
	 * @param YL out - vertical length of string
	 * @param ScaleX - scale that the string is expected to draw at horizontally
	 * @param ScaleY - scale that the string is expected to draw at vertically
	 */
	void TextSize( UFont* InFont, const FString& InText, float& XL, float& YL, float ScaleX=1.f, float ScaleY=1.f);
	
	/** Set DrawColor with a FLinearColor and optional opacity override */
	void SetLinearDrawColor(FLinearColor InColor, float OpacityOverride=-1.f);

	/** Set draw color. (R,G,B,A) */
	void SetDrawColor(uint8 R, uint8 G, uint8 B, uint8 A = 255);

	/** Set draw color. (FColor) */
	void SetDrawColor(FColor const& C);

	/** constructor for FontRenderInfo */
	FFontRenderInfo CreateFontRenderInfo(bool bClipText = false, bool bEnableShadow = false, FLinearColor GlowColor = FLinearColor(), FVector2D GlowOuterRadius = FVector2D(), FVector2D GlowInnerRadius = FVector2D());
	
	/** reset canvas parameters, optionally do not change the origin */
	virtual void Reset(bool bKeepOrigin = false);

	/** Sets the position of the lower-left corner of the clipping region of the Canvas */
	void SetClip(float X, float Y);

	/** Return X,Y for center of the draw region. */
	void GetCenter(float& outX, float& outY) const;

	/** Fake CanvasIcon constructor.	 */
	static FCanvasIcon MakeIcon(class UTexture* Texture, float U = 0.f, float V = 0.f, float UL = 0.f, float VL = 0.f);

	/** Draw a scaled CanvasIcon at the desired canvas position. */
	void DrawScaledIcon(FCanvasIcon Icon, float X, float Y, FVector Scale);
	
	/** Draw a CanvasIcon at the desired canvas position.	 */
	void DrawIcon(FCanvasIcon Icon, float X, float Y, float Scale = 0.f);

	/**
	 * Draws a graph comparing 2 variables.  Useful for visual debugging and tweaking.
	 *
	 * @param Title		Label to draw on the graph, or "" for none
	 * @param ValueX	X-axis value of the point to plot
	 * @param ValueY	Y-axis value of the point to plot
	 * @param UL_X		X screen coord of the upper-left corner of the graph
	 * @param UL_Y		Y screen coord of the upper-left corner of the graph
	 * @param W			Width of the graph, in pixels
	 * @param H			Height of the graph, in pixels
	 * @param RangeX	Range of values expressed by the X axis of the graph
	 * @param RangeY	Range of values expressed by the Y axis of the graph
	 */
	virtual void DrawDebugGraph(const FString& Title, float ValueX, float ValueY, float UL_X, float UL_Y, float W, float H, FVector2D RangeX, FVector2D RangeY);

	/* 
	 * Draw a CanvasItem
	 *
	 * @param Item			Item to draw
	 */
	void DrawItem( class FCanvasItem& Item );

	/* 
	 * Draw a CanvasItem at the given coordinates
	 *
	 * @param Item			Item to draw
	 * @param InPostion		Position to draw item
	 */
	void DrawItem( class FCanvasItem& Item, const FVector2D& InPosition );
	
	/* 
	 * Draw a CanvasItem at the given coordinates
	 *
	 * @param Item			Item to draw
	 * @param X				X Position to draw item
	 * @param Y				Y Position to draw item
	 */
	void DrawItem( class FCanvasItem& Item, float X, float Y );
};



