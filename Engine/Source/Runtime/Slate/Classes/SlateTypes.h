// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateTypes.generated.h"

UENUM()
enum EHorizontalAlignment
{
	HAlign_Fill,
	HAlign_Left,
	HAlign_Center,
	HAlign_Right,
};

UENUM()
enum EVerticalAlignment
{
	VAlign_Fill,
	VAlign_Top,
	VAlign_Center,
	VAlign_Bottom
};

UENUM()
enum EOrientation
{
	Orient_Horizontal,
	Orient_Vertical
};

UENUM()
enum EMenuPlacement
{
	/** Place the menu immediately below the anchor */
	MenuPlacement_BelowAnchor,
	/** Place the menu immediately below the anchor and match is width to the anchor's content */
	MenuPlacement_ComboBox,
	/** Place the menu to the right of the anchor */
	MenuPlacement_MenuRight,
	/** Place the menu immediately above the anchor, not transition effect */
	MenuPlacement_AboveAnchor,
};

/**
 * Describes the space around a Widget.
 */
USTRUCT()
struct FMargin
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor (zero size margin) */
	FMargin()
		: Left( 0.0f ), Top( 0.0f ), Right( 0.0f ), Bottom( 0.0f )
	{		
	}

	/** Construct a Margin with uniform space on all sides */
	FMargin( float UnifromMargin )
	: Left(UnifromMargin)
	, Top(UnifromMargin)
	, Right(UnifromMargin)
	, Bottom(UnifromMargin)
	{
		
	}
	
	/** Construct a Margin where Horizontal describes Left and Right spacing while Vertical describes Top and Bottom spacing */
	FMargin( float Horizontal, float Vertical )
	: Left(Horizontal)
	, Top(Vertical)
	, Right(Horizontal)
	, Bottom(Vertical)
	{
	
	}
	
	/** Construct a Margin where the spacing on each side is individually specified. */
	FMargin( float InLeft, float InTop, float InRight, float InBottom )
	: Left(InLeft)
	, Top(InTop)
	, Right(InRight)
	, Bottom(InBottom)
	{
	
	}
	
	/**
	 * Multiply the margin by a scalar.
	 *
	 * @param Scale   How much to scale the margin
	 *
	 * @return an FMargin where each value is scaled by Scale.
	 */
	FMargin operator * (float Scale) const
	{
		return FMargin( Left*Scale, Top*Scale, Right*Scale, Bottom*Scale );
	}

	/** Add two margins */
	FMargin operator + (const FMargin& InDelta) const
	{
		return FMargin( Left+InDelta.Left, Top+InDelta.Top, Right+InDelta.Right, Bottom+InDelta.Bottom );
	}

	FMargin operator - (const FMargin& Other) const
	{
		return FMargin( Left-Other.Left, Top-Other.Top, Right-Other.Right, Bottom-Other.Bottom );
	}

	bool operator==( const FMargin& Other ) const 
	{
		return Left == Other.Left && Right == Other.Right && Top == Other.Top && Bottom == Other.Bottom;
	}

	bool operator!=( const FMargin& Other ) const 
	{
		return Left != Other.Left || Right != Other.Right || Top != Other.Top || Bottom != Other.Bottom;
	}

	/** Return the total horizontal or vertical margin */
	template<EOrientation Orientation>
	float GetTotalSpaceAlong() const
	{
		return 0.0f;
	}
	
	FVector2D GetDesiredSize() const
	{
		return FVector2D( Left+Right, Top+Bottom );
	}
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	float Left;

	UPROPERTY(EditAnywhere, Category=Appearance)
	float Top;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	float Right;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	float Bottom;
};

template<>
inline float FMargin::GetTotalSpaceAlong<Orient_Horizontal>() const { return Left + Right; }

template<>
inline float FMargin::GetTotalSpaceAlong<Orient_Vertical>() const { return Top + Bottom; }

/**
 * A representation of a font in Slate
 */
USTRUCT()
struct SLATE_API FSlateFontInfo
{
	GENERATED_USTRUCT_BODY()

	FSlateFontInfo( const FString& InFontName, uint16 InSize );

	FSlateFontInfo( const FName& InFontName, uint16 InSize );

	FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize );

	FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize );

	FSlateFontInfo();

	bool operator==( const FSlateFontInfo& Other ) const 
	{
		return FontName == Other.FontName && Size == Other.Size;
	}

	friend inline uint32 GetTypeHash( const FSlateFontInfo& FontInfo )
	{
		return GetTypeHash( FontInfo.FontName ) + (FontInfo.Size << 17);
	}

	UPROPERTY(EditAnywhere, Category=SlateStyleRules)
	/** The name of the font */
	FName FontName;

	UPROPERTY(EditAnywhere, Category=SlateStyleRules)
	/** The size of the font */
	int32 Size;
};

/** Ways in which an image can be drawn: */
UENUM()
namespace ESlateBrushDrawType
{
	enum Type
	{
		/** Don't do anything */
		NoDrawType,
		/** Draw a 3x3 box, where the sides and the middle stretch based on the Margin */
		Box,
		/** Draw a 3x3 border where the sides tile and the middle is empty */
		Border,
		/** Draw an image; margin is ignored */
		Image
	};
}


/** When drawing an image, we can tile instead of stretching */
UENUM()
namespace ESlateBrushTileType
{
	enum Type
	{
		/** Just stretch */
		NoTile,
		/** Tile the image horizontally */
		Horizontal,
		/** Tile the image vertically */
		Vertical,
		/** Tile in both directions */
		Both
	};
}

UENUM()
namespace ESlateBrushImageType
{
	enum Type
	{
		/** No image is loaded.  Color only brushes, transparent brushes etc */
		NoImage,

		/** The image to be loaded is in full color */
		FullColor,

		/** The image is a special texture in linear space (usually a rendering resource such as a lookup table)*/
		Linear,
	};
}

/** Should we use the specified color? If not, then which color from the style should we use. */
UENUM()
namespace ESlateColorStylingMode
{
	enum Type
	{
		UseColor_Specified,
		UseColor_Specified_Link,
		UseColor_Foreground,
		UseColor_Foreground_Subdued
	};
}

/** A Slate color can be a directly specified value, or the color can be pulled from a WidgetStyle */
USTRUCT()
struct FSlateColor
{
	GENERATED_USTRUCT_BODY()

public:
	FSlateColor()
	// Use fuschia for uninitialized slate colors; nobody will miss that.
	: SpecifiedColor( FLinearColor(1,0,1) )
	, ColorUseRule( ESlateColorStylingMode::UseColor_Specified )
	{
	}

	/** Construct a FSlateColor that uses a specified color */
	FSlateColor( const FLinearColor& InColor )
	: SpecifiedColor( InColor )
	, ColorUseRule( ESlateColorStylingMode::UseColor_Specified )
	{
	}

	/** Construct a FSlateColor that uses a specified color */
	FSlateColor( const TSharedRef< FLinearColor >& InColor )
		: SpecifiedColor( FLinearColor(1,0,1) )
		, ColorUseRule( ESlateColorStylingMode::UseColor_Specified_Link )
		, LinkedSpecifiedColor( InColor )
	{
	}

	/** @returns an FSlateColor that is the widget's foreground. */
	static FSlateColor UseForeground()
	{
		return FSlateColor( ESlateColorStylingMode::UseColor_Foreground );
	}

	/** @returns an FSlateColor that is the subdued version of the widget's foreground. */
	static FSlateColor UseSubduedForeground()
	{
		return FSlateColor( ESlateColorStylingMode::UseColor_Foreground_Subdued );
	}

	bool IsColorSpecified() const
	{
		return ColorUseRule == ESlateColorStylingMode::UseColor_Specified || ESlateColorStylingMode::UseColor_Specified_Link;
	}

	FLinearColor GetSpecifiedColor() const
	{
		if ( LinkedSpecifiedColor.IsValid() )
		{
			return *LinkedSpecifiedColor;
		}

		return SpecifiedColor;
	}

	/** @return the specified color or the appropriate color from the widget's style */
	const FLinearColor& GetColor( const FWidgetStyle& InWidgetStyle ) const
	{
		switch( ColorUseRule )
		{
			default:
			case ESlateColorStylingMode::UseColor_Foreground: return InWidgetStyle.GetForegroundColor(); break;
			case ESlateColorStylingMode::UseColor_Specified: return SpecifiedColor; break;
			case ESlateColorStylingMode::UseColor_Specified_Link: return *LinkedSpecifiedColor; break;
			case ESlateColorStylingMode::UseColor_Foreground_Subdued: return InWidgetStyle.GetSubduedForegroundColor(); break; 
		};
	}

protected:
	
	/** Private constructor; allows construction of invalid FSlateColors! */
	FSlateColor( ESlateColorStylingMode::Type InColorUseRule )
	: ColorUseRule( InColorUseRule )
	, LinkedSpecifiedColor()
	{
	}

	/** The current specified color; only meaningful when ColorToUse == UseColor_Specified */
	UPROPERTY(EditAnywhere, Category=Color)
	FLinearColor SpecifiedColor;

	/** The rule for which color to pick */
	UPROPERTY(EditAnywhere, Category=Color)
	TEnumAsByte<ESlateColorStylingMode::Type> ColorUseRule;

	TSharedPtr< FLinearColor > LinkedSpecifiedColor;
};

/**
 * An intermediary to make UBaseSound available for Slate to play sounds
 */
USTRUCT()
struct SLATE_API FSlateSound
{
	GENERATED_USTRUCT_BODY()

	FSlateSound()
		: ResourceObject(nullptr)
	{
	}

	/** Get the resource object associated with this sound
	 *
	 * @note Ensure that you only access the resource as a USoundBase 
	 *
	 * @return The resource object, or null if this sound resource is empty
	 */
	UObject* GetResourceObject() const;

	/**
	 * Construct a FSlateSound from a FName
	 * @note This functionality is deprecated and should only be used for upgrading old data
	 *
	 * @param SoundName The name of the sound to load, eg) SoundCue'/Game/QA_Assets/Sound/TEST_Music_Ambient.TEST_Music_Ambient'
	 *
	 * @return An FSlateSound object with a ResourceObject pointer that's either set the given sound, or points to null if the sound could not be found
	 */
	static FSlateSound FromName_DEPRECATED(const FName& SoundName);

	/** Used when updating the ResourceObject within an FSlateSound from the editor to clear out any legacy data that may be set */
	void StripLegacyData_DEPRECATED();

	/** Used to upgrade a FName property to a FSlateSound property */
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

protected:
	/**
	 * Pointer to the USoundBase. Holding onto it as a UObject because USoundBase is not available in Slate core.
	 * Edited via FSlateSoundStructCustomization to ensure you can only set USoundBase assets on it.
	 */
	UPROPERTY(EditAnywhere, Category=Sound)
	UObject* ResourceObject;

	/** The legacy resource name; only used by sounds that have been set-up in code, or otherwise upgraded from old FName properties, set to NAME_None in non-legacy instances */
	FName LegacyResourceName_DEPRECATED;
	/** A weak pointer the the resource loaded from the legacy resource name; may be null if the resource needs (re)loading */
	mutable TWeakObjectPtr<UObject> LegacyResourceObject_DEPRECATED;
};

template<>
struct TStructOpsTypeTraits<FSlateSound> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithSerializeFromMismatchedTag = true,
	};
};

/**
 * An brush which contains information about how to draw a Slate element
 */
USTRUCT()
struct SLATE_API FSlateBrush
{
	GENERATED_USTRUCT_BODY()

	FSlateBrush()
	: ImageSize(32,32)
	, DrawAs(ESlateBrushDrawType::Image)
	, Margin(0)
	, Tint_DEPRECATED(FLinearColor::White)
	, TintColor(FLinearColor::White)
	, Tiling(ESlateBrushTileType::NoTile)
	, ImageType(ESlateBrushImageType::NoImage)
	, ResourceObject(NULL)
	, ResourceName(NAME_None)
	, bIsDynamicallyLoaded(false)
	, bHasUObject_DEPRECATED(false)
	{
	}

	/** Size of the resource in Slate Units */
	UPROPERTY(EditAnywhere, Category=Brush)
	FVector2D ImageSize;

	/** How to draw the image */
	UPROPERTY(EditAnywhere, Category=Brush)
	TEnumAsByte<enum ESlateBrushDrawType::Type > DrawAs;

	/** The margin to use in Box and Border modes */
	UPROPERTY(EditAnywhere, Category=Brush, meta=(UVSpace="true"))
	FMargin Margin;

	/** Tinting applied to the image. */
	UPROPERTY()
	FLinearColor Tint_DEPRECATED;

	/** Tinting applied to the image. */
	UPROPERTY(EditAnywhere, Category=Brush, meta=(DisplayName="Tint"))
	FSlateColor TintColor;

	/** How to tile the image in Image mode */
	UPROPERTY(EditAnywhere, Category=Brush)
	TEnumAsByte<enum ESlateBrushTileType::Type > Tiling;

	/** The type of image */
	UPROPERTY()
	TEnumAsByte<enum ESlateBrushImageType::Type > ImageType;

	/** Identifier for UObject based texture paths */
	static const FString UTextureIdentifier();

	/** Ensure that we only access the TextureObject as a UTexture2D */
	class UObject* GetResourceObject() const
	{
		return ResourceObject;
	}

	const FName GetResourceName() const
	{
		return (ResourceName != NAME_None || ResourceObject == NULL)
			? ResourceName
			: ResourceObject->GetFName();
	}

	FLinearColor GetTint( const FWidgetStyle& InWidgetStyle ) const
	{
		return TintColor.GetColor( InWidgetStyle );
	}

protected:
	/** Pointer to the UTexture2D. Holding onto it as a UObject because UTexture2D is not available in Slate core. */
	UPROPERTY( EditAnywhere, Category=Brush, meta=(DisplayThumbnail="true", DisplayName="Texture") )
	UObject* ResourceObject;

	/** The name of the rendering resource to use */
	UPROPERTY()
	FName ResourceName;

public:
	/** @return Whether or not the brush resource is dynamically loaded */
	bool IsDynamicallyLoaded() const { return bIsDynamicallyLoaded; }

	/** Does this brush have a UTexture? */
	bool HasUObject() const { return ResourceObject != NULL || bHasUObject_DEPRECATED; }
protected:
	/** Whether or not the brush path is a path to a UObject */
	bool bIsDynamicallyLoaded;

	/** Whether or not the brush has a UTexture resource */
	bool bHasUObject_DEPRECATED;
protected:
	/** 
	 * This constructor is protected; use one of the deriving classes instead.
	 *
	 * @param InDrawType      How to draw the texture
	 * @param InResourceName  The name of the resource
	 * @param InMargin        Margin to use in border and box modes
	 * @param InTiling        Tile horizontally/vertically or both? (only in image mode)
	 * @param InImageType	  The type of image
	 * @param InTint		  Tint to apply to the element.
	 */
	 FORCENOINLINE FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const FLinearColor& InTint = FLinearColor::White, UObject* InObjectResource = NULL, bool bInDynamicallyLoaded = false );

	 FORCENOINLINE FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, UObject* InObjectResource = NULL, bool bInDynamicallyLoaded = false );

	 FORCENOINLINE FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const FSlateColor& InTint, UObject* InObjectResource = NULL, bool bInDynamicallyLoaded = false );
};

/**
 * A resource that has no appearance
 */
struct SLATE_API FSlateNoResource : public FSlateBrush
{
	FORCENOINLINE FSlateNoResource(const FVector2D& InImageSize = FVector2D::ZeroVector)
		: FSlateBrush( ESlateBrushDrawType::NoDrawType, FName(NAME_None), FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, InImageSize )
	{

	}
};

/**
 * A 3x3 box where the sides stretch horizontally and vertically and the middle stretches to fill.
 * The corners will not be stretched. The size of the sides and corners is determined by the
 * Margin as follows:
 *
 *                 _____________________
 *                |  | Margin.Top    |  |
 *                |__|_______________|__|   Margin.Right
 *                |  |               |  |  /
 *              +--> |               | <--+
 *             /  |__|_______________|__|
 *  Margin.Left   |  | Margin.Bottom |  |
 *                |__|_______________|__|
 *
 */
struct SLATE_API FSlateBoxBrush : public FSlateBrush
{
	/**
	 * Make a 3x3 box that stretches the texture.
	 *
	 * @param InImageName		The name of image to make into a box
	 * @param InMargin			The size of corners and sides in normalized texture UV space.
	 * @param InColorAndOpacity	Color and opacity scale.
	 */
	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}
	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	} 

	FORCENOINLINE FSlateBoxBrush( const WIDECHAR* InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const TCHAR* InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const TCHAR* InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{

	}

	/**
	 * Make a 3x3 box that stretches the texture.
	 *
	 * @param InImageName       The name of image to make into a box
	 * @param ImageSize         The size of the resource as we want it to appear in slate units.
	 * @param InMargin          The size of corners and sides in texture space.
	 * @param InColorAndOpacity	Color and opacity scale. Note of the image type is ImageType_TintMask, this value should be in HSV
	 */
	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const WIDECHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const TCHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const FName& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{

	}

	FORCENOINLINE FSlateBoxBrush( const FString& InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, *InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const ANSICHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}

	FORCENOINLINE FSlateBoxBrush( const TCHAR* InImageName, const FVector2D& ImageSize, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Box, InImageName, InMargin, ESlateBrushTileType::NoTile, InImageType, ImageSize, InColorAndOpacity )
	{
	}
};


/**
 * Similar to FSlateBoxBrush but has no middle and the sides tile instead of stretching.
 * The margin is applied exactly as in FSlateBoxBrush.
 */
struct SLATE_API FSlateBorderBrush : public FSlateBrush
{
	/**
	 * @param InImageName		The name of the texture to draw
	 * @param InMargin			Determines the sides and corner sizes; see FSlateBoxBrush.
	 * @param InColorAndOpacity	Color and opacity scale.
	 * @param InImageType	The type of image this this is
	 */
	FORCENOINLINE FSlateBorderBrush( const FName& InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const FString& InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, *InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const WIDECHAR* InImageName, const FMargin& InMargin, const FLinearColor& InColorAndOpacity = FLinearColor(1,1,1,1), ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const FName& InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const FString& InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, *InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const TCHAR* InImageName, const FMargin& InMargin, const TSharedRef< FLinearColor >& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const FName& InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const FString& InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, *InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const ANSICHAR* InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}

	FORCENOINLINE FSlateBorderBrush( const TCHAR* InImageName, const FMargin& InMargin, const FSlateColor& InColorAndOpacity, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Border, InImageName, InMargin, ESlateBrushTileType::Both, InImageType, FVector2D::ZeroVector, InColorAndOpacity )
	{
		// A border with no margin will not show up at all.
		check( InMargin.GetDesiredSize().SizeSquared() > 0 );
	}
};

/**
 * Ignores the Margin. Just renders the image. Can tile the image instead of stretching.
 */
struct SLATE_API FSlateImageBrush : public FSlateBrush
{
	/**
	 * @param InImageName	The name of the texture to draw
	 * @param InImageSize	How large should the image be (not necessarily the image size on disk)
	 * @param InTint		The tint of the image.
	 * @param InTiling		How do we tile if at all?
	 * @param InImageType	The type of image this this is
	 */
	FORCENOINLINE FSlateImageBrush( const FName& InImageName, const FVector2D& InImageSize, const FLinearColor& InTint = FLinearColor(1,1,1,1), ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const FString& InImageName, const FVector2D& InImageSize, const FLinearColor& InTint = FLinearColor(1,1,1,1), ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, *InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const ANSICHAR* InImageName, const FVector2D& InImageSize, const FLinearColor& InTint = FLinearColor(1,1,1,1), ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const WIDECHAR* InImageName, const FVector2D& InImageSize, const FLinearColor& InTint = FLinearColor(1,1,1,1), ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const FName& InImageName, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const FString& InImageName, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, *InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const ANSICHAR* InImageName, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const TCHAR* InImageName, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const FName& InImageName, const FVector2D& InImageSize, const FSlateColor& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const FString& InImageName, const FVector2D& InImageSize, const FSlateColor& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, *InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const ANSICHAR* InImageName, const FVector2D& InImageSize, const FSlateColor& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}

	FORCENOINLINE FSlateImageBrush( const TCHAR* InImageName, const FVector2D& InImageSize, const FSlateColor& InTint, ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InImageName, FMargin(0), InTiling, InImageType, InImageSize, InTint )
	{
	}
};

/**
 * Ignores the Margin. Just renders the image. Can tile the image instead of stretching.
 */
struct SLATE_API FSlateDynamicImageBrush : public FSlateBrush
{
	/**
	 * @param InTexture		The UTexture2D being used for this brush.
	 * @param InImageSize		How large should the image be (not necessarily the image size on disk)
	 * @param InTint		The tint of the image
	 * @param InTiling		How do we tile if at all?
	 * @param InImageType		The type of image this this is
	 */
	FORCENOINLINE FSlateDynamicImageBrush( 
		class UTexture2D* InTexture, 
		const FVector2D& InImageSize,
		const FName InTextureName,
		const FLinearColor& InTint = FLinearColor(1,1,1,1), 
		ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, 
		ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, FName(TEXT("None")), FMargin(0), InTiling, InImageType, InImageSize, InTint, (UObject*)InTexture )
	{
		bIsDynamicallyLoaded = true;
		//if we have a texture, make a unique name
		if(GetResourceObject() != NULL)
		{
			ResourceName = InTextureName;
		}
	}

	/**
	 * @param InTextureName		The name of the texture to load.
	 * @param InImageSize		How large should the image be (not necessarily the image size on disk)
	 * @param InTint		The tint of the image.
	 * @param InTiling		How do we tile if at all?
	 * @param InImageType		The type of image this this is
	 */
	FSlateDynamicImageBrush( 
		const FName InTextureName,
		const FVector2D& InImageSize,
		const FLinearColor& InTint = FLinearColor(1,1,1,1), 
		ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, 
		ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor )
		: FSlateBrush( ESlateBrushDrawType::Image, InTextureName, FMargin(0), InTiling, InImageType, InImageSize, InTint, NULL, true )
	{
		bIsDynamicallyLoaded = true;
	}

	/** Defined in Style.cpp */
	virtual ~FSlateDynamicImageBrush();
};

/**
 * A color brush.  Draws a box with no margins that only has a color applied to it                   
 */
struct SLATE_API FSlateColorBrush : public FSlateBrush
{
	/** 
	 * @param InColor	The color to use
	 */
	FORCENOINLINE FSlateColorBrush( const FLinearColor& InColor )
		: FSlateBrush( ESlateBrushDrawType::Box, NAME_None, FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, FVector2D::ZeroVector, InColor )
	{
	}

	/** 
	 * @param InColor	The color to use 
	 */
	FORCENOINLINE FSlateColorBrush( const FColor& InColor )
		: FSlateBrush( ESlateBrushDrawType::Box, NAME_None, FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, FVector2D::ZeroVector, InColor.ReinterpretAsLinear() )
	{
	}

	FORCENOINLINE FSlateColorBrush( const TSharedRef< FLinearColor >& InColor )
		: FSlateBrush( ESlateBrushDrawType::Box, NAME_None, FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, FVector2D::ZeroVector, InColor )
	{
	}
};

/**
 * Struct used to represent an icon in Slate
 */
struct SLATE_API FSlateIcon
{
	/** Default construct for an empty icon */
	FSlateIcon();

	/**
	 * Construct this icon from a style set and style name
	 * @param InStyleSetName The name of the style set the icon can be found in, eg) FCoreStyle::Get().GetStyleSetName()
	 * @param StyleName The name of the style for the icon (assumes there may also be a ".Small" variant)
	 */
	FSlateIcon(const FName& InStyleSetName, const FName& InStyleName);

	/**
	 * Construct this icon from a style set and style name
	 * @param InStyleSetName The name of the style set the icon can be found in, eg) FCoreStyle::Get().GetStyleSetName()
	 * @param StyleName The name of the style for the icon
	 * @param InSmallStyleName The name of the style for the small icon
	 */
	FSlateIcon(const FName& InStyleSetName, const FName& InStyleName, const FName& InSmallStyleName);

	/** @return The name of the style set the icon can be found in */
	const FName& GetStyleSetName() const
	{
		return StyleSetName;
	}

	/** @return The name of the style for the icon */
	const FName& GetStyleName() const
	{
		return StyleName;
	}

	/** @return The name of the style for the icon */
	const FName& GetSmallStyleName() const
	{
		return SmallStyleName;
	}

	/** @return True if this icon is set to something */
	const bool IsSet() const
	{
		return bIsSet;
	}

	/** @return The resolved style set, or null if the style set wasn't found */
	const class ISlateStyle* GetStyleSet() const;

	/** @return The resolved icon, or FStyleDefaults::GetNoBrush() if the icon wasn't found */
	const FSlateBrush* GetIcon() const;

	/** @return The resolved small icon, or FStyleDefaults::GetNoBrush() if the icon wasn't found */
	const FSlateBrush* GetSmallIcon() const;

private:
	/** The name of the style set the icon can be found in */
	FName StyleSetName;

	/** The name of the style for the icon */
	FName StyleName;

	/** The name of the style for the small icon */
	FName SmallStyleName;

	/** Has this icon been explicitly set-up? */
	bool bIsSet;
};

/**
 * 
 */
USTRUCT()
struct SLATE_API FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSlateWidgetStyle();

	virtual ~FSlateWidgetStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* > & OutBrushes ) const {}

	virtual const FName GetTypeName() const { return FName( NAME_None ); } 
};

/** Type of check box */
UENUM()
namespace ESlateCheckBoxType
{
	enum Type
	{
		/** Traditional check box with check button and label (or other content) */
		CheckBox,

		/** Toggle button.  You provide button content (such as an image), and the user can press to toggle it. */
		ToggleButton,
	};
}

/**
 * Represents the appearance of an SCheckBox
 */
USTRUCT()
struct SLATE_API FCheckBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FCheckBoxStyle();

	virtual ~FCheckBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* > & OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FCheckBoxStyle& GetDefault();

	/** The visual type of the checkbox */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<ESlateCheckBoxType::Type> CheckBoxType;
	FCheckBoxStyle& SetCheckBoxType( ESlateCheckBoxType::Type InCheckBoxType ){ CheckBoxType = InCheckBoxType; return *this; }

	/* CheckBox appearance when the CheckBox is unchecked (normal) */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UncheckedImage;
	FCheckBoxStyle & SetUncheckedImage( const FSlateBrush & InUncheckedImage ){ UncheckedImage = InUncheckedImage; return *this; }

	/* CheckBox appearance when the CheckBox is unchecked and hovered */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UncheckedHoveredImage;
	FCheckBoxStyle & SetUncheckedHoveredImage( const FSlateBrush & InUncheckedHoveredImage ){ UncheckedHoveredImage = InUncheckedHoveredImage; return *this; }

	/* CheckBox appearance when the CheckBox is unchecked and hovered */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UncheckedPressedImage;
	FCheckBoxStyle & SetUncheckedPressedImage( const FSlateBrush & InUncheckedPressedImage ){ UncheckedPressedImage = InUncheckedPressedImage; return *this; }

	/* CheckBox appearance when the CheckBox is checked */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush CheckedImage;
	FCheckBoxStyle & SetCheckedImage( const FSlateBrush & InCheckedImage ){ CheckedImage = InCheckedImage; return *this; }

	/* CheckBox appearance when checked and hovered */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush CheckedHoveredImage;
	FCheckBoxStyle & SetCheckedHoveredImage( const FSlateBrush & InCheckedHoveredImage ){ CheckedHoveredImage = InCheckedHoveredImage; return *this; }

	/* CheckBox appearance when checked and pressed */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush CheckedPressedImage;
	FCheckBoxStyle & SetCheckedPressedImage( const FSlateBrush & InCheckedPressedImage ){ CheckedPressedImage = InCheckedPressedImage; return *this; }
	
	/* CheckBox appearance when the CheckBox is undetermined */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UndeterminedImage;
	FCheckBoxStyle & SetUndeterminedImage( const FSlateBrush & InUndeterminedImage ){ UndeterminedImage = InUndeterminedImage; return *this; }

	/* CheckBox appearance when CheckBox is undetermined and hovered */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UndeterminedHoveredImage;
	FCheckBoxStyle & SetUndeterminedHoveredImage( const FSlateBrush & InUndeterminedHoveredImage ){ UndeterminedHoveredImage = InUndeterminedHoveredImage; return *this; }

	/* CheckBox appearance when CheckBox is undetermined and pressed */
	UPROPERTY( EditAnywhere, Category = Appearance )
	FSlateBrush UndeterminedPressedImage;
	FCheckBoxStyle & SetUndeterminedPressedImage( const FSlateBrush & InUndeterminedPressedImage ){ UndeterminedPressedImage = InUndeterminedPressedImage; return *this; }

	/** Padding */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;
	FCheckBoxStyle& SetPadding( const FMargin& InPadding ){ Padding = InPadding; return *this; }

	/** The foreground color */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ForegroundColor;
	FCheckBoxStyle& SetForegroundColor(const FSlateColor& InForegroundColor) { ForegroundColor = InForegroundColor; return *this; }

	/** BorderBackgroundColor refers to the actual color and opacity of the supplied border image on toggle buttons */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor BorderBackgroundColor;
	FCheckBoxStyle& SetBorderBackgroundColor(const FSlateColor& InBorderBackgroundColor) { BorderBackgroundColor = InBorderBackgroundColor; return *this; }

	/**
	 * The sound the check box should play when checked
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Checked Sound"))
	FSlateSound CheckedSlateSound;
	FCheckBoxStyle& SetCheckedSound( const FSlateSound& InCheckedSound ){ CheckedSlateSound = InCheckedSound; return *this; }

	/**
	 * The sound the check box should play when unchecked
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Unchecked Sound"))
	FSlateSound UncheckedSlateSound;
	FCheckBoxStyle& SetUncheckedSound( const FSlateSound& InUncheckedSound ){ UncheckedSlateSound = InUncheckedSound; return *this; }

	/**
	 * The sound the check box should play when initially hovered over
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Hovered Sound"))
	FSlateSound HoveredSlateSound;
	FCheckBoxStyle& SetHoveredSound( const FSlateSound& InHoveredSound ){ HoveredSlateSound = InHoveredSound; return *this; }

	UPROPERTY()
	FName CheckedSound_DEPRECATED;
	UPROPERTY()
	FName UncheckedSound_DEPRECATED;
	UPROPERTY()
	FName HoveredSound_DEPRECATED;

	/**
	 * Used to upgrade the deprecated FName sound properties into the new-style FSlateSound properties
	 */	
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FCheckBoxStyle> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithPostSerialize = true,
	};
};

/**
 * Represents the appearance of an STextBlock
 */
USTRUCT()
struct SLATE_API FTextBlockStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTextBlockStyle();

	virtual ~FTextBlockStyle() {}

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FTextBlockStyle& GetDefault();

	/** Font family and size to be used when displaying this text. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;
	FTextBlockStyle& SetFont(const FSlateFontInfo& InFont) { Font = InFont; return *this; }
	FTextBlockStyle& SetFont(const FName& InFontName, uint16 InSize) { Font = FSlateFontInfo(InFontName, InSize); return *this; }
	FTextBlockStyle& SetFont(const FString& InFontName, uint16 InSize) { Font = FSlateFontInfo(*InFontName, InSize); return *this; }
	FTextBlockStyle& SetFont(const WIDECHAR* InFontName, uint16 InSize) { Font = FSlateFontInfo(InFontName, InSize); return *this; }
	FTextBlockStyle& SetFont(const ANSICHAR* InFontName, uint16 InSize) { Font = FSlateFontInfo(InFontName, InSize); return *this; }
	FTextBlockStyle& SetFontName(const FName& InFontName) { Font.FontName = InFontName; return *this; }
	FTextBlockStyle& SetFontName(const FString& InFontName) { Font.FontName = FName(*InFontName); return *this; }
	FTextBlockStyle& SetFontName(const WIDECHAR* InFontName) { Font.FontName = InFontName; return *this; }
	FTextBlockStyle& SetFontName(const ANSICHAR* InFontName) { Font.FontName = InFontName; return *this; }
	FTextBlockStyle& SetFontSize(uint16 InSize) { Font.Size = InSize; return *this; }

	/** The color and opacity of this text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ColorAndOpacity;
	FTextBlockStyle& SetColorAndOpacity(const FSlateColor& InColorAndOpacity) { ColorAndOpacity = InColorAndOpacity; return *this; }

	/** How much should the shadow be offset? An offset of 0 implies no shadow. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D ShadowOffset;
	FTextBlockStyle& SetShadowOffset(const FVector2D& InShadowOffset) { ShadowOffset = InShadowOffset; return *this; }

	/** The color and opacity of the shadow */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor ShadowColorAndOpacity;
	FTextBlockStyle& SetShadowColorAndOpacity(const FLinearColor& InShadowColorAndOpacity) { ShadowColorAndOpacity = InShadowColorAndOpacity; return *this; }

	/** The color of highlighted text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor HighlightColor;
	FTextBlockStyle& SetHighlightColor(const FLinearColor& InHighlightColor) { HighlightColor = InHighlightColor; return *this; }

	/** The shape of highlighted text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HighlightShape;
	FTextBlockStyle& SetHighlightShape( const FSlateBrush& InHighlightShape ){ HighlightShape = InHighlightShape; return *this; }
};

/**
 * Represents the appearance of an SButton
 */
USTRUCT()
struct SLATE_API FButtonStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FButtonStyle();

	virtual ~FButtonStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FButtonStyle& GetDefault();

	/** Button appearance when the button is not hovered or pressed */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Normal;
	FButtonStyle& SetNormal( const FSlateBrush& InNormal ){ Normal = InNormal; return *this; }

	/** Button appearance when hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Hovered;
	FButtonStyle& SetHovered( const FSlateBrush& InHovered){ Hovered = InHovered; return *this; }

	/** Button appearance when pressed */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Pressed;
	FButtonStyle& SetPressed( const FSlateBrush& InPressed ){ Pressed = InPressed; return *this; }

	/** Button appearance when disabled */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Disabled;
	FButtonStyle& SetDisabled( const FSlateBrush& InDisabled ){ Disabled = InDisabled; return *this; }

	/**
	 * Padding that accounts for the border in the button's background image.
	 * When this is applied, the content of the button should appear flush
	 * with the button's border. Use this padding when the button is not pressed.
	 */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin NormalPadding;
	FButtonStyle& SetNormalPadding( const FMargin& InNormalPadding){ NormalPadding = InNormalPadding; return *this; }

	/**
	 * Same as NormalPadding but used when the button is pressed. Allows for moving the content to match
	 * any "movement" in the button's border image.
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin PressedPadding;
	FButtonStyle& SetPressedPadding( const FMargin& InPressedPadding){ PressedPadding = InPressedPadding; return *this; }

	/**
	 * The sound the button should play when pressed
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Pressed Sound"))
	FSlateSound PressedSlateSound;
	FButtonStyle& SetPressedSound( const FSlateSound& InPressedSound ){ PressedSlateSound = InPressedSound; return *this; }

	/**
	 * The sound the button should play when initially hovered over
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Hovered Sound"))
	FSlateSound HoveredSlateSound;
	FButtonStyle& SetHoveredSound( const FSlateSound& InHoveredSound ){ HoveredSlateSound = InHoveredSound; return *this; }

	UPROPERTY()
	FName PressedSound_DEPRECATED;
	UPROPERTY()
	FName HoveredSound_DEPRECATED;

	/**
	 * Used to upgrade the deprecated FName sound properties into the new-style FSlateSound properties
	 */	
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FButtonStyle> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithPostSerialize = true,
	};
};

/**
 * Represents the appearance of an SComboButton
 */
USTRUCT()
struct SLATE_API FComboButtonStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FComboButtonStyle();

	virtual ~FComboButtonStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FComboButtonStyle& GetDefault();

	/**
	 * The style to use for our SButton
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle ButtonStyle;
	FComboButtonStyle& SetButtonStyle( const FButtonStyle& InButtonStyle ){ ButtonStyle = InButtonStyle; return *this; }

	/**
	 * The scale to apply to this widget
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D ContentScale;
	FComboButtonStyle& SetContentScale( const FVector2D& InContentScale ){ ContentScale = InContentScale; return *this; }

	/**
	 * The padding to apply around our content
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin ContentPadding;
	FComboButtonStyle& SetContentPadding( const FMargin& InContentPadding ){ ContentPadding = InContentPadding; return *this; }

	/**
	 * Should this combo display a down arrow?
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	bool HasDownArrow;
	FComboButtonStyle& SetHasDownArrow( const bool InHasDownArrow ){ HasDownArrow = InHasDownArrow; return *this; }

	/**
	 * Image to use for the down arrow
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush DownArrowImage;
	FComboButtonStyle& SetDownArrowImage( const FSlateBrush& InDownArrowImage ){ DownArrowImage = InDownArrowImage; return *this; }

	/**
	 * The foreground color of our button
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ForegroundColor;
	FComboButtonStyle& SetForegroundColor( const FSlateColor& InForegroundColor ){ ForegroundColor = InForegroundColor; return *this; }

	/**
	 * The tint and opacity of our button
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ButtonColorAndOpacity;
	FComboButtonStyle& SetButtonColorAndOpacity( const FSlateColor& InButtonColorAndOpacity ){ ButtonColorAndOpacity = InButtonColorAndOpacity; return *this; }

	/**
	 * Brush to use to add a "menu border" around the drop-down content
	 */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MenuBorderBrush;
	FComboButtonStyle& SetMenuBorderBrush( const FSlateBrush& InMenuBorderBrush ){ MenuBorderBrush = InMenuBorderBrush; return *this; }

	/**
	 * Padding to use to add a "menu border" around the drop-down content
	 */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin MenuBorderPadding;
	FComboButtonStyle& SetMenuBorderPadding( const FMargin& InMenuBorderPadding ){ MenuBorderPadding = InMenuBorderPadding; return *this; }

	/**
	 * Where to place the menu relative to its anchor point
	 */
	UPROPERTY(EditAnywhere, Category=Layout)
	TEnumAsByte<EMenuPlacement> MenuPlacement;
	FComboButtonStyle& SetMenuPlacement( const EMenuPlacement InMenuPlacement ){ MenuPlacement = InMenuPlacement; return *this; }

	/**
	 * Horizontal alignment to use when positioning the content within the button
	 */
	UPROPERTY(EditAnywhere, Category=Layout)
	TEnumAsByte<EHorizontalAlignment> HAlign;
	FComboButtonStyle& SetHAlign( const EHorizontalAlignment InHAlign ){ HAlign = InHAlign; return *this; }

	/**
	 * Vertical alignment to use when positioning the content within the button
	 */
	UPROPERTY(EditAnywhere, Category=Layout)
	TEnumAsByte<EVerticalAlignment> VAlign;
	FComboButtonStyle& SetVAlign( const EVerticalAlignment InVAlign ){ VAlign = InVAlign; return *this; }
};


/**
 * Represents the appearance of an SComboBox
 */
USTRUCT()
struct SLATE_API FComboBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FComboBoxStyle();

	virtual ~FComboBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FComboBoxStyle& GetDefault();

	/**
	 * The style to use for our SComboButton
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ShowOnlyInnerProperties))
	FComboButtonStyle ComboButtonStyle;
	FComboBoxStyle& SetComboButtonStyle( const FComboButtonStyle& InComboButtonStyle ){ ComboButtonStyle = InComboButtonStyle; return *this; }

	/**
	 * The sound the button should play when pressed
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Pressed Sound"))
	FSlateSound PressedSlateSound;
	FComboBoxStyle& SetPressedSound( const FSlateSound& InPressedSound ){ PressedSlateSound = InPressedSound; return *this; }

	/**
	 * The Sound to play when the selection is changed
	 */	
	UPROPERTY(EditAnywhere, Category=Sound, meta=(DisplayName="Selection Change Sound"))
	FSlateSound SelectionChangeSlateSound;
	FComboBoxStyle& SetSelectionChangeSound( const FSlateSound& InSelectionChangeSound ){ SelectionChangeSlateSound = InSelectionChangeSound; return *this; }

	UPROPERTY()
	FName PressedSound_DEPRECATED;
	UPROPERTY()
	FName SelectionChangeSound_DEPRECATED;

	/**
	 * Used to upgrade the deprecated FName sound properties into the new-style FSlateSound properties
	 */	
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FComboBoxStyle> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithPostSerialize = true,
	};
};

/**
 * Represents the appearance of an SHyperlink
 */
USTRUCT()
struct SLATE_API FHyperlinkStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FHyperlinkStyle();

	virtual ~FHyperlinkStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FHyperlinkStyle& GetDefault();

	/** Underline style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle UnderlineStyle;
	FHyperlinkStyle& SetUnderlineStyle( const FButtonStyle& InUnderlineStyle ){ UnderlineStyle = InUnderlineStyle; return *this; }

	/** Text style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTextBlockStyle TextStyle;
	FHyperlinkStyle& SetTextStyle( const FTextBlockStyle& InTextStyle ){ TextStyle = InTextStyle; return *this; }

	/** Padding */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;
	FHyperlinkStyle& SetPadding( const FMargin& InPadding ){ Padding = InPadding; return *this; }
};

/**
 * Represents the appearance of an SEditableText
 */
USTRUCT()
struct SLATE_API FEditableTextStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FEditableTextStyle();

	virtual ~FEditableTextStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; }

	static const FEditableTextStyle& GetDefault();

	/** Font family and size to be used when displaying this text. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;
	FEditableTextStyle& SetFont(const FSlateFontInfo& InFont) { Font = InFont; return *this; }
	FEditableTextStyle& SetFont(const FName& InFontName, uint16 InSize) { Font = FSlateFontInfo(InFontName, InSize); return *this; }

	/** The color and opacity of this text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ColorAndOpacity;
	FEditableTextStyle& SetColorAndOpacity(const FSlateColor& InColorAndOpacity) { ColorAndOpacity = InColorAndOpacity; return *this; }

	/** Background image for the selected text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageSelected;
	FEditableTextStyle& SetBackgroundImageSelected( const FSlateBrush& InBackgroundImageSelected ){ BackgroundImageSelected = InBackgroundImageSelected; return *this; }

	/** Background image for the selection targeting effect */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageSelectionTarget;
	FEditableTextStyle& SetBackgroundImageSelectionTarget( const FSlateBrush& InBackgroundImageSelectionTarget ){ BackgroundImageSelectionTarget = InBackgroundImageSelectionTarget; return *this; }

	/** Background image for the selected text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageComposing;
	FEditableTextStyle& SetBackgroundImageComposing( const FSlateBrush& InBackgroundImageComposing ){ BackgroundImageComposing = InBackgroundImageComposing; return *this; }	

	/** Image brush used for the caret */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush CaretImage;
	FEditableTextStyle& SetCaretImage( const FSlateBrush& InCaretImage ){ CaretImage = InCaretImage; return *this; }
};

/**
 * Represents the appearance of an SEditableTextBox
 */
USTRUCT()
struct SLATE_API FEditableTextBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FEditableTextBoxStyle();

	virtual ~FEditableTextBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FEditableTextBoxStyle& GetDefault();

	/** Border background image when the box is not hovered or focused */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageNormal;
	FEditableTextBoxStyle& SetBackgroundImageNormal( const FSlateBrush& InBackgroundImageNormal ){ BackgroundImageNormal = InBackgroundImageNormal; return *this; }

	/** Border background image when the box is hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageHovered;
	FEditableTextBoxStyle& SetBackgroundImageHovered( const FSlateBrush& InBackgroundImageHovered ){ BackgroundImageHovered = InBackgroundImageHovered; return *this; }

	/** Border background image when the box is focused */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageFocused;
	FEditableTextBoxStyle& SetBackgroundImageFocused( const FSlateBrush& InBackgroundImageFocused ){ BackgroundImageFocused = InBackgroundImageFocused; return *this; }

	/** Border background image when the box is read-only */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImageReadOnly;
	FEditableTextBoxStyle& SetBackgroundImageReadOnly( const FSlateBrush& InBackgroundImageReadOnly ){ BackgroundImageReadOnly = InBackgroundImageReadOnly; return *this; }

	/** Padding */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;
	FEditableTextBoxStyle& SetPadding( const FMargin& InPadding ){ Padding = InPadding; return *this; }

	/** Font family and size to be used when displaying this text. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;
	FEditableTextBoxStyle& SetFont(const FSlateFontInfo& InFont) { Font = InFont; return *this; }
	FEditableTextBoxStyle& SetFont(const FName& InFontName, uint16 InSize) { Font = FSlateFontInfo(InFontName, InSize); return *this; }

	/** The foreground color of text. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ForegroundColor;
	FEditableTextBoxStyle& SetForegroundColor(const FSlateColor& InForegroundColor) { ForegroundColor = InForegroundColor; return *this; }

	/** The background color applied to the active background image */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor BackgroundColor;
	FEditableTextBoxStyle& SetBackgroundColor(const FSlateColor& InBackgroundColor) { BackgroundColor = InBackgroundColor; return *this; }

	/** The read-only foreground color of text in read-only mode. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ReadOnlyForegroundColor;
	FEditableTextBoxStyle& SetReadOnlyForegroundColor(const FSlateColor& InReadOnlyForegroundColor) { ReadOnlyForegroundColor = InReadOnlyForegroundColor; return *this; }
};


/**
 * Represents the appearance of an SInlineEditableTextBlock
 */
USTRUCT()
struct SLATE_API FInlineEditableTextBlockStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FInlineEditableTextBlockStyle();

	virtual ~FInlineEditableTextBlockStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FInlineEditableTextBlockStyle& GetDefault();

	/** The style of the editable text box, which dictates the font, color, and shadow options. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FEditableTextBoxStyle EditableTextBoxStyle;
	FInlineEditableTextBlockStyle& SetEditableTextBoxStyle( const FEditableTextBoxStyle& InEditableTextBoxStyle ){ EditableTextBoxStyle = InEditableTextBoxStyle; return *this; }

	/** The style of the text block, which dictates the font, color, and shadow options. Style overrides all other properties! */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTextBlockStyle TextStyle;
	FInlineEditableTextBlockStyle& SetTextStyle( const FTextBlockStyle& InTextStyle ){ TextStyle = InTextStyle; return *this; }
};


/**
 * Represents the appearance of an SProgressBar
 */
USTRUCT()
struct SLATE_API FProgressBarStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FProgressBarStyle();

	virtual ~FProgressBarStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FProgressBarStyle& GetDefault();

	/** Background image to use for the progress bar */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundImage;
	FProgressBarStyle& SetBackgroundImage( const FSlateBrush& InBackgroundImage ){ BackgroundImage = InBackgroundImage; return *this; }

	/** Foreground image to use for the progress bar */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush FillImage;
	FProgressBarStyle& SetFillImage( const FSlateBrush& InFillImage ){ FillImage = InFillImage; return *this; }

	/** Image to use for marquee mode */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MarqueeImage;
	FProgressBarStyle& SetMarqueeImage( const FSlateBrush& InMarqueeImage ){ MarqueeImage = InMarqueeImage; return *this; }
};


/**
 * Represents the appearance of an SScrollBar
 */
USTRUCT()
struct SLATE_API FScrollBarStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FScrollBarStyle();

	virtual ~FScrollBarStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FScrollBarStyle& GetDefault();

	/** Background image to use when the scrollbar is oriented horizontally */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HorizontalBackgroundImage;
	FScrollBarStyle& SetHorizontalBackgroundImage( const FSlateBrush& InHorizontalBackgroundImage ){ HorizontalBackgroundImage = InHorizontalBackgroundImage; return *this; }

	/** Background image to use when the scrollbar is oriented vertically */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush VerticalBackgroundImage;
	FScrollBarStyle& SetVerticalBackgroundImage( const FSlateBrush& InVerticalBackgroundImage ){ VerticalBackgroundImage = InVerticalBackgroundImage; return *this; }

	/** Image to use when the scrollbar thumb is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush NormalThumbImage;
	FScrollBarStyle& SetNormalThumbImage( const FSlateBrush& InNormalThumbImage ){ NormalThumbImage = InNormalThumbImage; return *this; }

	/** Image to use when the scrollbar thumb is in its hovered state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HoveredThumbImage;
	FScrollBarStyle& SetHoveredThumbImage( const FSlateBrush& InHoveredThumbImage ){ HoveredThumbImage = InHoveredThumbImage; return *this; }

	/** Image to use when the scrollbar thumb is in its dragged state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush DraggedThumbImage;
	FScrollBarStyle& SetDraggedThumbImage( const FSlateBrush& InDraggedThumbImage ){ DraggedThumbImage = InDraggedThumbImage; return *this; }
};


/**
 * Represents the appearance of an SExpandableArea
 */
USTRUCT()
struct SLATE_API FExpandableAreaStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FExpandableAreaStyle();

	virtual ~FExpandableAreaStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FExpandableAreaStyle& GetDefault();

	/** Image to use when the area is collapsed */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush CollapsedImage;
	FExpandableAreaStyle& SetCollapsedImage( const FSlateBrush& InCollapsedImage ){ CollapsedImage = InCollapsedImage; return *this; }

	/** Image to use when the area is expanded */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ExpandedImage;
	FExpandableAreaStyle& SetExpandedImage( const FSlateBrush& InExpandedImage ){ ExpandedImage = InExpandedImage; return *this; }
};


/**
 * Represents the appearance of an SSearchBox
 */
USTRUCT()
struct SLATE_API FSearchBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSearchBoxStyle();

	virtual ~FSearchBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FSearchBoxStyle& GetDefault();

	/** Style to use for the text box part of the search box */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FEditableTextBoxStyle TextBoxStyle;
	FSearchBoxStyle& SetTextBoxStyle( const FEditableTextBoxStyle& InTextBoxStyle );

	/** Font to use for the text box part of the search box when a search term is entered*/
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo ActiveFontInfo;
	FSearchBoxStyle& SetActiveFont( const FSlateFontInfo& InFontInfo ){ ActiveFontInfo = InFontInfo; return *this; }

	/** Image to use for the search "up" arrow */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush UpArrowImage;
	FSearchBoxStyle& SetUpArrowImage( const FSlateBrush& InUpArrowImage ){ UpArrowImage = InUpArrowImage; return *this; }

	/** Image to use for the search "down" arrow */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush DownArrowImage;
	FSearchBoxStyle& SetDownArrowImage( const FSlateBrush& InDownArrowImage ){ DownArrowImage = InDownArrowImage; return *this; }

	/** Image to use for the search "glass" */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush GlassImage;
	FSearchBoxStyle& SetGlassImage( const FSlateBrush& InGlassImage ){ GlassImage = InGlassImage; return *this; }

	/** Image to use for the search "clear" button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ClearImage;
	FSearchBoxStyle& SetClearImage( const FSlateBrush& InClearImage ){ ClearImage = InClearImage; return *this; }
};


/**
 * Represents the appearance of an SSlider
 */
USTRUCT()
struct SLATE_API FSliderStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSliderStyle();

	virtual ~FSliderStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FSliderStyle& GetDefault();

	/** Image to use when the slider thumb is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush NormalThumbImage;
	FSliderStyle& SetNormalThumbImage( const FSlateBrush& InNormalThumbImage ){ NormalThumbImage = InNormalThumbImage; return *this; }

	/** Image to use when the slider thumb is in its disabled state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush DisabledThumbImage;
	FSliderStyle& SetDisabledThumbImage( const FSlateBrush& InDisabledThumbImage ){ DisabledThumbImage = InDisabledThumbImage; return *this; }
};


/**
 * Represents the appearance of an SVolumeControl
 */
USTRUCT()
struct SLATE_API FVolumeControlStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FVolumeControlStyle();

	virtual ~FVolumeControlStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FVolumeControlStyle& GetDefault();

	/** The style of the volume control slider */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSliderStyle SliderStyle;
	FVolumeControlStyle& SetSliderStyle( const FSliderStyle& InSliderStyle ){ SliderStyle = InSliderStyle; return *this; }

	/** Image to use when the volume is set to high */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HighVolumeImage;
	FVolumeControlStyle& SetHighVolumeImage( const FSlateBrush& InHighVolumeImage ){ HighVolumeImage = InHighVolumeImage; return *this; }

	/** Image to use when the volume is set to mid-range */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MidVolumeImage;
	FVolumeControlStyle& SetMidVolumeImage( const FSlateBrush& InMidVolumeImage ){ MidVolumeImage = InMidVolumeImage; return *this; }

	/** Image to use when the volume is set to low */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LowVolumeImage;
	FVolumeControlStyle& SetLowVolumeImage( const FSlateBrush& InLowVolumeImage ){ LowVolumeImage = InLowVolumeImage; return *this; }

	/** Image to use when the volume is set to off */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush NoVolumeImage;
	FVolumeControlStyle& SetNoVolumeImage( const FSlateBrush& InNoVolumeImage ){ NoVolumeImage = InNoVolumeImage; return *this; }

	/** Image to use when the volume is muted */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MutedImage;
	FVolumeControlStyle& SetMutedImage( const FSlateBrush& InMutedImage ){ MutedImage = InMutedImage; return *this; }
};

/**
 * Represents the appearance of an inline image used by rich text
 */
USTRUCT()
struct SLATE_API FInlineTextImageStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FInlineTextImageStyle();

	virtual ~FInlineTextImageStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FInlineTextImageStyle& GetDefault();

	/** Image to use when the slider thumb is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Image;
	FInlineTextImageStyle& SetImage( const FSlateBrush& InImage ){ Image = InImage; return *this; }

	/** The offset from the bottom of the image height to the baseline. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	int16 Baseline;
	FInlineTextImageStyle& SetBaseline( int16 InBaseline ){ Baseline = InBaseline; return *this; }
};

/**
 * Represents the appearance of an SSpinBox
 */
USTRUCT()
struct SLATE_API FSpinBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSpinBoxStyle();

	virtual ~FSpinBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FSpinBoxStyle& GetDefault();

	/** Brush used to draw the background of the spinbox */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundBrush;
	FSpinBoxStyle& SetBackgroundBrush( const FSlateBrush& InBackgroundBrush ){ BackgroundBrush = InBackgroundBrush; return *this; }

	/** Brush used to draw the background of the spinbox when it's hovered over */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HoveredBackgroundBrush;
	FSpinBoxStyle& SetHoveredBackgroundBrush( const FSlateBrush& InHoveredBackgroundBrush ){ HoveredBackgroundBrush = InHoveredBackgroundBrush; return *this; }

	/** Brush used to fill the spinbox when it's active */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ActiveFillBrush;
	FSpinBoxStyle& SetActiveFillBrush( const FSlateBrush& InActiveFillBrush ){ ActiveFillBrush = InActiveFillBrush; return *this; }

	/** Brush used to fill the spinbox when it's inactive */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush InactiveFillBrush;
	FSpinBoxStyle& SetInactiveFillBrush( const FSlateBrush& InInactiveFillBrush ){ InactiveFillBrush = InInactiveFillBrush; return *this; }

	/** Image used to draw the spinbox arrows */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ArrowsImage;
	FSpinBoxStyle& SetArrowsImage( const FSlateBrush& InArrowsImage ){ ArrowsImage = InArrowsImage; return *this; }

	/** Color used to draw the spinbox foreground elements */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ForegroundColor;
	FSpinBoxStyle& SetForegroundColor( const FSlateColor& InForegroundColor ){ ForegroundColor = InForegroundColor; return *this; }

	/** Padding to add around the spinbox and its text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin TextPadding;
	FSpinBoxStyle& SetTextPadding( const FMargin& InTextPadding ){ TextPadding = InTextPadding; return *this; }
};


/**
 * Represents the appearance of an SSplitter
 */
USTRUCT()
struct SLATE_API FSplitterStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSplitterStyle();

	virtual ~FSplitterStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FSplitterStyle& GetDefault();

	/** Brush used to draw the handle in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HandleNormalBrush;
	FSplitterStyle& SetHandleNormalBrush( const FSlateBrush& InHandleNormalBrush ){ HandleNormalBrush = InHandleNormalBrush; return *this; }

	/** Brush used to draw the handle in its highlight state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HandleHighlightBrush;
	FSplitterStyle& SetHandleHighlightBrush( const FSlateBrush& InHandleHighlightBrush ){ HandleHighlightBrush = InHandleHighlightBrush; return *this; }
};


/**
 * Represents the appearance of an STableRow
 */
USTRUCT()
struct SLATE_API FTableRowStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTableRowStyle();

	virtual ~FTableRowStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FTableRowStyle& GetDefault();

	/** Brush used as a selector when a row is focused */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush SelectorFocusedBrush;
	FTableRowStyle& SetSelectorFocusedBrush( const FSlateBrush& InSelectorFocusedBrush ){ SelectorFocusedBrush = InSelectorFocusedBrush; return *this; }

	/** Brush used when a selected row is active and hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ActiveHoveredBrush;
	FTableRowStyle& SetActiveHoveredBrush( const FSlateBrush& InActiveHoveredBrush ){ ActiveHoveredBrush = InActiveHoveredBrush; return *this; }

	/** Brush used when a selected row is active */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ActiveBrush;
	FTableRowStyle& SetActiveBrush( const FSlateBrush& InActiveBrush ){ ActiveBrush = InActiveBrush; return *this; }

	/** Brush used when an selected row is inactive and hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush InactiveHoveredBrush;
	FTableRowStyle& SetInactiveHoveredBrush( const FSlateBrush& InInactiveHoveredBrush ){ InactiveHoveredBrush = InInactiveHoveredBrush; return *this; }

	/** Brush used when a selected row is inactive */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush InactiveBrush;
	FTableRowStyle& SetInactiveBrush( const FSlateBrush& InInactiveBrush ){ InactiveBrush = InInactiveBrush; return *this; }

	/** Brush used when an even row is hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush EvenRowBackgroundHoveredBrush;
	FTableRowStyle& SetEvenRowBackgroundHoveredBrush( const FSlateBrush& InEvenRowBackgroundHoveredBrush ){ EvenRowBackgroundHoveredBrush = InEvenRowBackgroundHoveredBrush; return *this; }

	/** Brush used when an even row is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush EvenRowBackgroundBrush;
	FTableRowStyle& SetEvenRowBackgroundBrush( const FSlateBrush& InEvenRowBackgroundBrush ){ EvenRowBackgroundBrush = InEvenRowBackgroundBrush; return *this; }

	/** Brush used when an odd row is hovered */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OddRowBackgroundHoveredBrush;
	FTableRowStyle& SetOddRowBackgroundHoveredBrush( const FSlateBrush& InOddRowBackgroundHoveredBrush ){ OddRowBackgroundHoveredBrush = InOddRowBackgroundHoveredBrush; return *this; }

	/** Brush to used when an odd row is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OddRowBackgroundBrush;
	FTableRowStyle& SetOddRowBackgroundBrush( const FSlateBrush& InOddRowBackgroundBrush ){ OddRowBackgroundBrush = InOddRowBackgroundBrush; return *this; }

	/** Text color used for all rows */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor TextColor;
	FTableRowStyle& SetTextColor( const FSlateColor& InTextColor ){ TextColor = InTextColor; return *this; }

	/** Text color used for the selected row */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor SelectedTextColor;
	FTableRowStyle& SetSelectedTextColor( const FSlateColor& InSelectedTextColor ){ SelectedTextColor = InSelectedTextColor; return *this; }
};


/**
 * Represents the appearance of an STableColumnHeader
 */
USTRUCT()
struct SLATE_API FTableColumnHeaderStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTableColumnHeaderStyle();

	virtual ~FTableColumnHeaderStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FTableColumnHeaderStyle& GetDefault();

	/** Image used when a column is sorted in ascending order */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush SortAscendingImage;
	FTableColumnHeaderStyle& SetSortAscendingImage( const FSlateBrush& InSortAscendingImage ){ SortAscendingImage = InSortAscendingImage; return *this; }

	/** Image used when a column is sorted in descending order */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush SortDescendingImage;
	FTableColumnHeaderStyle& SetSortDescendingImage( const FSlateBrush& InSortDescendingImage ){ SortDescendingImage = InSortDescendingImage; return *this; }

	/** Brush used to draw the header in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush NormalBrush;
	FTableColumnHeaderStyle& SetNormalBrush( const FSlateBrush& InNormalBrush ){ NormalBrush = InNormalBrush; return *this; }

	/** Brush used to draw the header in its hovered state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HoveredBrush;
	FTableColumnHeaderStyle& SetHoveredBrush( const FSlateBrush& InHoveredBrush ){ HoveredBrush = InHoveredBrush; return *this; }

	/** Image used for the menu drop-down button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MenuDropdownImage;
	FTableColumnHeaderStyle& SetMenuDropdownImage( const FSlateBrush& InMenuDropdownImage ){ MenuDropdownImage = InMenuDropdownImage; return *this; }

	/** Brush used to draw the menu drop-down border in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MenuDropdownNormalBorderBrush;
	FTableColumnHeaderStyle& SetMenuDropdownNormalBorderBrush( const FSlateBrush& InMenuDropdownNormalBorderBrush ){ MenuDropdownNormalBorderBrush = InMenuDropdownNormalBorderBrush; return *this; }

	/** Brush used to draw the menu drop-down border in its hovered state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush MenuDropdownHoveredBorderBrush;
	FTableColumnHeaderStyle& SetMenuDropdownHoveredBorderBrush( const FSlateBrush& InMenuDropdownHoveredBorderBrush ){ MenuDropdownHoveredBorderBrush = InMenuDropdownHoveredBorderBrush; return *this; }
};


/**
 * Represents the appearance of an SHeaderRow
 */
USTRUCT()
struct SLATE_API FHeaderRowStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FHeaderRowStyle();

	virtual ~FHeaderRowStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FHeaderRowStyle& GetDefault();

	/** Style of the normal header row columns */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTableColumnHeaderStyle ColumnStyle;
	FHeaderRowStyle& SetColumnStyle( const FTableColumnHeaderStyle& InColumnStyle ){ ColumnStyle = InColumnStyle; return *this; }

	/** Style of the last header row column */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTableColumnHeaderStyle LastColumnStyle;
	FHeaderRowStyle& SetLastColumnStyle( const FTableColumnHeaderStyle& InLastColumnStyle ){ LastColumnStyle = InLastColumnStyle; return *this; }

	/** Style of the splitter used between the columns */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSplitterStyle ColumnSplitterStyle;
	FHeaderRowStyle& SetColumnSplitterStyle( const FSplitterStyle& InColumnSplitterStyle ){ ColumnSplitterStyle = InColumnSplitterStyle; return *this; }

	/** Brush used to draw the header row background */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundBrush;
	FHeaderRowStyle& SetBackgroundBrush( const FSlateBrush& InBackgroundBrush ){ BackgroundBrush = InBackgroundBrush; return *this; }

	/** Color used to draw the header row foreground */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ForegroundColor;
	FHeaderRowStyle& SetForegroundColor( const FSlateColor& InForegroundColor ){ ForegroundColor = InForegroundColor; return *this; }
};


/**
 * Represents the appearance of an SDockTab
 */
USTRUCT()
struct SLATE_API FDockTabStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FDockTabStyle();

	virtual ~FDockTabStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FDockTabStyle& GetDefault();

	/** Style used for the close button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle CloseButtonStyle;
	FDockTabStyle& SetCloseButtonStyle( const FButtonStyle& InCloseButtonStyle ){ CloseButtonStyle = InCloseButtonStyle; return *this; }

	/** Brush used when this tab is in its normal state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush NormalBrush;
	FDockTabStyle& SetNormalBrush( const FSlateBrush& InNormalBrush ){ NormalBrush = InNormalBrush; return *this; }

	/** Brush used when this tab is in its active state */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ActiveBrush;
	FDockTabStyle& SetActiveBrush( const FSlateBrush& InActiveBrush ){ ActiveBrush = InActiveBrush; return *this; }

	/** Brush used to overlay a given color onto this tab */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ColorOverlayBrush;
	FDockTabStyle& SetColorOverlayBrush( const FSlateBrush& InColorOverlayBrush ){ ColorOverlayBrush = InColorOverlayBrush; return *this; }

	/** Brush used when this tab is in the foreground */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ForegroundBrush;
	FDockTabStyle& SetForegroundBrush( const FSlateBrush& InForegroundBrush ){ ForegroundBrush = InForegroundBrush; return *this; }

	/** Brush used when this tab is hovered over */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HoveredBrush;
	FDockTabStyle& SetHoveredBrush( const FSlateBrush& InHoveredBrush ){ HoveredBrush = InHoveredBrush; return *this; }

	/** Brush used by the SDockingTabStack to draw the content associated with this tab; Documents, Apps, and Tool Panels have different backgrounds */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ContentAreaBrush;
	FDockTabStyle& SetContentAreaBrush( const FSlateBrush& InContentAreaBrush ){ ContentAreaBrush = InContentAreaBrush; return *this; }

	/** Brush used by the SDockingTabStack to draw the content associated with this tab; Documents, Apps, and Tool Panels have different backgrounds */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush TabWellBrush;
	FDockTabStyle& SetTabWellBrush( const FSlateBrush& InTabWellBrush ){ TabWellBrush = InTabWellBrush; return *this; }

	/** Padding used around this tab */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin TabPadding;
	FDockTabStyle& SetTabPadding( const FMargin& InTabPadding ){ TabPadding = InTabPadding; return *this; }

	/** The width that this tab will overlap with side-by-side tabs */
	UPROPERTY(EditAnywhere, Category=Appearance)
	float OverlapWidth;
	FDockTabStyle& SetOverlapWidth( const float InOverlapWidth ){ OverlapWidth = InOverlapWidth; return *this; }
		
	/** Color used when flashing this tab */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor FlashColor;
	FDockTabStyle& SetFlashColor( const FSlateColor& InFlashColor ){ FlashColor = InFlashColor; return *this; }
};


/**
 * Represents the appearance of an SScrollBox
 */
USTRUCT()
struct SLATE_API FScrollBoxStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FScrollBoxStyle();

	virtual ~FScrollBoxStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FScrollBoxStyle& GetDefault();

	/** Brush used to draw the top shadow of a scrollbox */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush TopShadowBrush;
	FScrollBoxStyle& SetTopShadowBrush( const FSlateBrush& InTopShadowBrush ){ TopShadowBrush = InTopShadowBrush; return *this; }

	/** Brush used to draw the bottom shadow of a scrollbox */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BottomShadowBrush;
	FScrollBoxStyle& SetBottomShadowBrush( const FSlateBrush& InBottomShadowBrush ){ BottomShadowBrush = InBottomShadowBrush; return *this; }
};


/**
* Represents the appearance of an FScrollBorderStyle
*/
USTRUCT()
struct SLATE_API FScrollBorderStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FScrollBorderStyle();

	virtual ~FScrollBorderStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FScrollBorderStyle& GetDefault();

	/** Brush used to draw the top shadow of a scrollborder */
	UPROPERTY( EditAnywhere, Category = Appearance )
		FSlateBrush TopShadowBrush;
	FScrollBorderStyle& SetTopShadowBrush( const FSlateBrush& InTopShadowBrush ){ TopShadowBrush = InTopShadowBrush; return *this; }

	/** Brush used to draw the bottom shadow of a scrollborder */
	UPROPERTY( EditAnywhere, Category = Appearance )
		FSlateBrush BottomShadowBrush;
	FScrollBorderStyle& SetBottomShadowBrush( const FSlateBrush& InBottomShadowBrush ){ BottomShadowBrush = InBottomShadowBrush; return *this; }
};


/**
 * Represents the appearance of an SWindow
 */
USTRUCT()
struct SLATE_API FWindowStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FWindowStyle();

	virtual ~FWindowStyle() {}

	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE;

	static const FName TypeName;
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

	static const FWindowStyle& GetDefault();

	/** Style used to draw the window minimize button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle MinimizeButtonStyle;
	FWindowStyle& SetMinimizeButtonStyle( const FButtonStyle& InMinimizeButtonStyle ){ MinimizeButtonStyle = InMinimizeButtonStyle; return *this; }

	/** Style used to draw the window maximize button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle MaximizeButtonStyle;
	FWindowStyle& SetMaximizeButtonStyle( const FButtonStyle& InMaximizeButtonStyle ){ MaximizeButtonStyle = InMaximizeButtonStyle; return *this; }

	/** Style used to draw the window restore button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle RestoreButtonStyle;
	FWindowStyle& SetRestoreButtonStyle( const FButtonStyle& InRestoreButtonStyle ){ RestoreButtonStyle = InRestoreButtonStyle; return *this; }

	/** Style used to draw the window close button */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FButtonStyle CloseButtonStyle;
	FWindowStyle& SetCloseButtonStyle( const FButtonStyle& InCloseButtonStyle ){ CloseButtonStyle = InCloseButtonStyle; return *this; }

	/** Style used to draw the window title text */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTextBlockStyle TitleTextStyle;
	FWindowStyle& SetTitleTextStyle( const FTextBlockStyle& InTitleTextStyle ){ TitleTextStyle = InTitleTextStyle; return *this; }

	/** Brush used to draw the window title area when the window is active */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ActiveTitleBrush;
	FWindowStyle& SetActiveTitleBrush( const FSlateBrush& InActiveTitleBrush ){ ActiveTitleBrush = InActiveTitleBrush; return *this; }

	/** Brush used to draw the window title area when the window is inactive */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush InactiveTitleBrush;
	FWindowStyle& SetInactiveTitleBrush( const FSlateBrush& InInactiveTitleBrush ){ InactiveTitleBrush = InInactiveTitleBrush; return *this; }

	/** Brush used to draw the window title area when the window is flashing */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush FlashTitleBrush;
	FWindowStyle& SetFlashTitleBrush( const FSlateBrush& InFlashTitleBrush ){ FlashTitleBrush = InFlashTitleBrush; return *this; }

	/** Brush used to draw the window outline */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OutlineBrush;
	FWindowStyle& SetOutlineBrush( const FSlateBrush& InOutlineBrush ){ OutlineBrush = InOutlineBrush; return *this; }

	/** Color used to draw the window outline */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor OutlineColor;
	FWindowStyle& SetOutlineColor( const FSlateColor& InOutlineColor ){ OutlineColor = InOutlineColor; return *this; }

	/** Brush used to draw the window border */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BorderBrush;
	FWindowStyle& SetBorderBrush( const FSlateBrush& InBorderBrush ){ BorderBrush = InBorderBrush; return *this; }

	/** Brush used to draw the window background */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundBrush;
	FWindowStyle& SetBackgroundBrush( const FSlateBrush& InBackgroundBrush ){ BackgroundBrush = InBackgroundBrush; return *this; }

	/** Brush used to draw the background of child windows */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ChildBackgroundBrush;
	FWindowStyle& SetChildBackgroundBrush( const FSlateBrush& InChildBackgroundBrush ){ ChildBackgroundBrush = InChildBackgroundBrush; return *this; }
};


/** HACK: We need a UClass here or UHT will complain. */
UCLASS()
class USlateTypes : public UObject
{
public:
	GENERATED_UCLASS_BODY()

};
