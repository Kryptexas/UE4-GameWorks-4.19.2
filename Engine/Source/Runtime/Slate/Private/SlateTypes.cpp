// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "PropertyTag.h"

namespace SlateTypeDefs
{
	// Colors
	const static FLinearColor DefaultForeground = FLinearColor(0.72f, 0.72f, 0.72f, 1.f);
	const static FLinearColor InvertedForeground = FLinearColor(0,0,0);
}

FSlateFontInfo::FSlateFontInfo( const FString& InFontName, uint16 InSize )
	: FontName( *InFontName )
	, Size( InSize )
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}

FSlateFontInfo::FSlateFontInfo( const FName& InFontName, uint16 InSize )
	: FontName( InFontName )
	, Size( InSize )
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}

FSlateFontInfo::FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize )
	: FontName( InFontName )
	, Size( InSize )
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}

FSlateFontInfo::FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize )
	: FontName( InFontName )
	, Size( InSize )
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}

FSlateFontInfo::FSlateFontInfo()
	: FontName( NAME_None )
	, Size( 0 )
{

}

UObject* FSlateSound::GetResourceObject() const
{
	// We might still be holding a legacy resource name, in which case we need to test that first and load it if required
	if(LegacyResourceName_DEPRECATED != NAME_None)
	{
		// Do we still have a valid reference in our weak-ptr?
		UObject* LegacyResourceObject = LegacyResourceObject_DEPRECATED.Get();
		if(!LegacyResourceObject)
		{
			// We can't check the object type against USoundBase as we don't have access to it here
			// The user is required to cast the result of FSlateSound::GetResourceObject so we should be fine
			LegacyResourceObject = StaticFindObject(UObject::StaticClass(), nullptr, *LegacyResourceName_DEPRECATED.ToString());
			if(!ResourceObject)
			{
				LegacyResourceObject = StaticLoadObject(UObject::StaticClass(), nullptr, *LegacyResourceName_DEPRECATED.ToString());
			}

			// Cache this in the weak-ptr to try and avoid having to load it all the time
			LegacyResourceObject_DEPRECATED = LegacyResourceObject;
		}

		return LegacyResourceObject;
	}

	return ResourceObject;
}

FSlateSound FSlateSound::FromName_DEPRECATED(const FName& SoundName)
{
	FSlateSound Sound;

	// Just set the name, the sound will get loaded the first time it's required
	Sound.LegacyResourceName_DEPRECATED = SoundName;

#if WITH_EDITOR
	if(GIsEditor)
	{
		// If we're in the editor, we need to load the sound immediately so that the ResourceObject is valid for editing
		Sound.ResourceObject = Sound.GetResourceObject();
	}
#endif

	return Sound;
}

void FSlateSound::StripLegacyData_DEPRECATED()
{
	LegacyResourceName_DEPRECATED = NAME_None;
	LegacyResourceObject_DEPRECATED.Reset();
}

bool FSlateSound::SerializeFromMismatchedTag(FPropertyTag const& Tag, FArchive& Ar)
{
	// Sounds in Slate used to be stored as FName properties, so allow them to be upgraded in-place
	if(Tag.Type == NAME_NameProperty)
	{
		FName SoundName;
		Ar << SoundName;
		*this = FromName_DEPRECATED(SoundName);
		return true;
	}

	return false;
}


const float FWidgetStyle::SubdueAmount = 0.6f;

FWidgetStyle& FWidgetStyle::SetForegroundColor( const TAttribute< struct FSlateColor >& InForeground )
{
	ForegroundColor = InForeground.Get().GetColor( *this );

	SubduedForeground = ForegroundColor;
	SubduedForeground.A *= SubdueAmount;

	return *this;
}

FSlateBrush::FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const FLinearColor& InTint, UObject* InObjectResource, bool bInDynamicallyLoaded )
	: ImageSize( InImageSize )
	, DrawAs( InDrawType )
	, Margin( InMargin )
	, TintColor( InTint )
	, Tiling( InTiling )
	, ImageType( InImageType )
	, ResourceObject( InObjectResource )
	, ResourceName( InResourceName )
	, bIsDynamicallyLoaded( bInDynamicallyLoaded )
{
	bHasUObject_DEPRECATED = InObjectResource != NULL || InResourceName.ToString().StartsWith(FSlateBrush::UTextureIdentifier());

	//Useful for debugging style breakages
	//if ( !bHasUObject_DEPRECATED && InResourceName.IsValid() && InResourceName != NAME_None )
	//{
	//	checkf( FPaths::FileExists( InResourceName.ToString() ), *FPaths::ConvertRelativePathToFull( InResourceName.ToString() ) );
	//}
}

FSlateBrush::FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const TSharedRef< FLinearColor >& InTint, UObject* InObjectResource, bool bInDynamicallyLoaded )
	: ImageSize( InImageSize )
	, DrawAs( InDrawType )
	, Margin( InMargin )
	, TintColor( InTint )
	, Tiling( InTiling )
	, ImageType( InImageType )
	, ResourceObject( InObjectResource )
	, ResourceName( InResourceName )
	, bIsDynamicallyLoaded( bInDynamicallyLoaded )
{
	bHasUObject_DEPRECATED = InObjectResource != NULL || InResourceName.ToString().StartsWith(FSlateBrush::UTextureIdentifier());

	//Useful for debugging style breakages
	//if ( !bHasUObject_DEPRECATED && InResourceName.IsValid() && InResourceName != NAME_None )
	//{
	//	checkf( FPaths::FileExists( InResourceName.ToString() ), *FPaths::ConvertRelativePathToFull( InResourceName.ToString() ) );
	//}
}

FSlateBrush::FSlateBrush( ESlateBrushDrawType::Type InDrawType, const FName InResourceName, const FMargin& InMargin, ESlateBrushTileType::Type InTiling, ESlateBrushImageType::Type InImageType, const FVector2D& InImageSize, const FSlateColor& InTint, UObject* InObjectResource, bool bInDynamicallyLoaded )
	: ImageSize( InImageSize )
	, DrawAs( InDrawType )
	, Margin( InMargin )
	, TintColor( InTint )
	, Tiling( InTiling )
	, ImageType( InImageType )
	, ResourceObject( InObjectResource )
	, ResourceName( InResourceName )
	, bIsDynamicallyLoaded( bInDynamicallyLoaded )
{
	bHasUObject_DEPRECATED = InObjectResource != NULL || InResourceName.ToString().StartsWith(FSlateBrush::UTextureIdentifier());

	//Useful for debugging style breakages
	//if ( !bHasUObject_DEPRECATED && InResourceName.IsValid() && InResourceName != NAME_None )
	//{
	//	checkf( FPaths::FileExists( InResourceName.ToString() ), *FPaths::ConvertRelativePathToFull( InResourceName.ToString() ) );
	//}
}

const FString FSlateBrush::UTextureIdentifier()
{
	return FString(TEXT("texture:/"));
}

FSlateDynamicImageBrush::~FSlateDynamicImageBrush()
{
	if(FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid())
	{
		FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource( *this );
	}
}

FSlateIcon::FSlateIcon()
	: StyleSetName(NAME_None)
	, StyleName(NAME_None)
	, SmallStyleName(NAME_None)
	, bIsSet(false)
{
}

FSlateIcon::FSlateIcon(const FName& InStyleSetName, const FName& InStyleName)
	: StyleSetName(InStyleSetName)
	, StyleName(InStyleName)
	, SmallStyleName(ISlateStyle::Join(InStyleName, ".Small"))
	, bIsSet(true)
{
}

FSlateIcon::FSlateIcon(const FName& InStyleSetName, const FName& InStyleName, const FName& InSmallStyleName)
	: StyleSetName(InStyleSetName)
	, StyleName(InStyleName)
	, SmallStyleName(InSmallStyleName)
	, bIsSet(true)
{
}

const ISlateStyle* FSlateIcon::GetStyleSet() const
{
	return FSlateStyleRegistry::FindSlateStyle(StyleSetName);
}

const FSlateBrush* FSlateIcon::GetIcon() const
{
	const ISlateStyle* StyleSet = GetStyleSet();
	if(StyleSet)
	{
		return StyleSet->GetOptionalBrush(StyleName);
	}
	return FStyleDefaults::GetNoBrush();
}

const FSlateBrush* FSlateIcon::GetSmallIcon() const
{
	const ISlateStyle* StyleSet = GetStyleSet();
	if(StyleSet)
	{
		return StyleSet->GetOptionalBrush(SmallStyleName);
	}
	return FStyleDefaults::GetNoBrush();
}

FSlateWidgetStyle::FSlateWidgetStyle() 
{

}

FCheckBoxStyle::FCheckBoxStyle()
: CheckBoxType(ESlateCheckBoxType::CheckBox)
, UncheckedImage()
, UncheckedHoveredImage()
, UncheckedPressedImage()
, CheckedImage()
, CheckedHoveredImage()
, CheckedPressedImage()
, UndeterminedImage()
, UndeterminedHoveredImage()
, UndeterminedPressedImage()
, Padding(FMargin(2,0,0,0))
, ForegroundColor(FSlateColor::UseForeground())
, BorderBackgroundColor(FLinearColor::White)
{
}

const FName FCheckBoxStyle::TypeName( TEXT("FCheckBoxStyle") );

const FCheckBoxStyle& FCheckBoxStyle::GetDefault()
{
	static FCheckBoxStyle Default;
	return Default;
}

void FCheckBoxStyle::GetResources( TArray< const FSlateBrush* > & OutBrushes ) const
{
	OutBrushes.Add( &UncheckedImage );
	OutBrushes.Add( &UncheckedHoveredImage );
	OutBrushes.Add( &UncheckedPressedImage );
	OutBrushes.Add( &CheckedImage );
	OutBrushes.Add( &CheckedHoveredImage );
	OutBrushes.Add( &CheckedPressedImage );
	OutBrushes.Add( &UndeterminedImage );
	OutBrushes.Add( &UndeterminedHoveredImage );
	OutBrushes.Add( &UndeterminedPressedImage );
}

void FCheckBoxStyle::PostSerialize(const FArchive& Ar)
{
	if(Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_FSLATESOUND_CONVERSION)
	{
		// eg, SoundCue'/Game/QA_Assets/Sound/TEST_Music_Ambient.TEST_Music_Ambient'
		CheckedSlateSound = FSlateSound::FromName_DEPRECATED(CheckedSound_DEPRECATED);
		UncheckedSlateSound = FSlateSound::FromName_DEPRECATED(UncheckedSound_DEPRECATED);
		HoveredSlateSound = FSlateSound::FromName_DEPRECATED(HoveredSound_DEPRECATED);
	}
}

FTextBlockStyle::FTextBlockStyle()
: Font()
, ColorAndOpacity()
, ShadowOffset(FVector2D::ZeroVector)
, ShadowColorAndOpacity(FLinearColor::Black)
{
}

const FName FTextBlockStyle::TypeName( TEXT("FTextBlockStyle") );

const FTextBlockStyle& FTextBlockStyle::GetDefault()
{
	static TSharedPtr< FTextBlockStyle > Default;
	if ( !Default.IsValid() )
	{
		Default = MakeShareable( new FTextBlockStyle() );
		Default->Font = FStyleDefaults::GetFontInfo();
	}

	return *Default.Get();
}

FButtonStyle::FButtonStyle()
: Normal()
, Hovered()
, Pressed()
, NormalPadding()
, PressedPadding()
{
}

const FName FButtonStyle::TypeName( TEXT("FButtonStyle") );

void FButtonStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &Normal );
	OutBrushes.Add( &Hovered );
	OutBrushes.Add( &Pressed );
	OutBrushes.Add( &Disabled );
}

const FButtonStyle& FButtonStyle::GetDefault()
{
	static FButtonStyle Default;
	return Default;
}

void FButtonStyle::PostSerialize(const FArchive& Ar)
{
	if(Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_FSLATESOUND_CONVERSION)
	{
		// eg, SoundCue'/Game/QA_Assets/Sound/TEST_Music_Ambient.TEST_Music_Ambient'
		PressedSlateSound = FSlateSound::FromName_DEPRECATED(PressedSound_DEPRECATED);
		HoveredSlateSound = FSlateSound::FromName_DEPRECATED(HoveredSound_DEPRECATED);
	}
}

FComboButtonStyle::FComboButtonStyle()
	: ContentScale(FVector2D(1,1))
	, ContentPadding(FMargin(5))
	, HasDownArrow(true)
	, ButtonColorAndOpacity(FLinearColor::White)
	, MenuBorderPadding(FMargin(0.0f))
	, MenuPlacement(MenuPlacement_ComboBox)
	, HAlign(HAlign_Fill)
	, VAlign(VAlign_Fill)
{
}

const FName FComboButtonStyle::TypeName( TEXT("FComboButtonStyle") );

void FComboButtonStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add(&MenuBorderBrush);
	OutBrushes.Add(&DownArrowImage);
	ButtonStyle.GetResources(OutBrushes);
}

const FComboButtonStyle& FComboButtonStyle::GetDefault()
{
	static FComboButtonStyle Default;
	return Default;
}

FComboBoxStyle::FComboBoxStyle()
{
	ComboButtonStyle.SetContentPadding(FMargin(4.0, 2.0));
	ComboButtonStyle.SetMenuBorderPadding(FMargin(1.0));
}

const FName FComboBoxStyle::TypeName( TEXT("FComboBoxStyle") );

void FComboBoxStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	ComboButtonStyle.GetResources(OutBrushes);
}

const FComboBoxStyle& FComboBoxStyle::GetDefault()
{
	static FComboBoxStyle Default;
	return Default;
}

void FComboBoxStyle::PostSerialize(const FArchive& Ar)
{
	if(Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_FSLATESOUND_CONVERSION)
	{
		// eg, SoundCue'/Game/QA_Assets/Sound/TEST_Music_Ambient.TEST_Music_Ambient'
		PressedSlateSound = FSlateSound::FromName_DEPRECATED(PressedSound_DEPRECATED);
		SelectionChangeSlateSound = FSlateSound::FromName_DEPRECATED(SelectionChangeSound_DEPRECATED);
	}
}

FHyperlinkStyle::FHyperlinkStyle()
: UnderlineStyle()
, TextStyle()
, Padding()
{
}

const FName FHyperlinkStyle::TypeName( TEXT("FHyperlinkStyle") );

void FHyperlinkStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	UnderlineStyle.GetResources(OutBrushes);
	TextStyle.GetResources(OutBrushes);
}

const FHyperlinkStyle& FHyperlinkStyle::GetDefault()
{
	static FHyperlinkStyle Default;
	return Default;
}

FEditableTextStyle::FEditableTextStyle()
	: Font(FStyleDefaults::GetFontInfo(9))
	, ColorAndOpacity(FSlateColor::UseForeground())
	, BackgroundImageSelected()
	, BackgroundImageSelectionTarget()
	, CaretImage()
{
}

void FEditableTextStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &BackgroundImageSelected );
	OutBrushes.Add( &BackgroundImageSelectionTarget );
	OutBrushes.Add( &CaretImage );
}

const FName FEditableTextStyle::TypeName( TEXT("FEditableTextStyle") );

const FEditableTextStyle& FEditableTextStyle::GetDefault()
{
	static FEditableTextStyle Default;
	return Default;
}

FEditableTextBoxStyle::FEditableTextBoxStyle()
	: BackgroundImageNormal()
	, BackgroundImageHovered()
	, BackgroundImageFocused()
	, BackgroundImageReadOnly()
	, Padding(FMargin(4.0f, 2.0f))
	, Font(FStyleDefaults::GetFontInfo(9))
	, ForegroundColor(SlateTypeDefs::InvertedForeground)
	, BackgroundColor(FLinearColor::White)
	, ReadOnlyForegroundColor(SlateTypeDefs::DefaultForeground)
{
}

void FEditableTextBoxStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &BackgroundImageNormal );
	OutBrushes.Add( &BackgroundImageHovered );
	OutBrushes.Add( &BackgroundImageFocused );
	OutBrushes.Add( &BackgroundImageReadOnly );
}

const FName FEditableTextBoxStyle::TypeName( TEXT("FEditableTextBoxStyle") );

const FEditableTextBoxStyle& FEditableTextBoxStyle::GetDefault()
{
	static FEditableTextBoxStyle Default;
	return Default;
}


FInlineEditableTextBlockStyle::FInlineEditableTextBlockStyle()
{
}

void FInlineEditableTextBlockStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	EditableTextBoxStyle.GetResources(OutBrushes);
	TextStyle.GetResources(OutBrushes);
}

const FName FInlineEditableTextBlockStyle::TypeName( TEXT("FInlineEditableTextBlockStyle") );

const FInlineEditableTextBlockStyle& FInlineEditableTextBlockStyle::GetDefault()
{
	static FInlineEditableTextBlockStyle Default;
	return Default;
}


FProgressBarStyle::FProgressBarStyle()
{
}

void FProgressBarStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &BackgroundImage );
	OutBrushes.Add( &FillImage );
	OutBrushes.Add( &MarqueeImage );
}

const FName FProgressBarStyle::TypeName( TEXT("FProgressBarStyle") );

const FProgressBarStyle& FProgressBarStyle::GetDefault()
{
	static FProgressBarStyle Default;
	return Default;
}


FScrollBarStyle::FScrollBarStyle()
{
}

void FScrollBarStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &HorizontalBackgroundImage );
	OutBrushes.Add( &VerticalBackgroundImage );
	OutBrushes.Add( &NormalThumbImage );
	OutBrushes.Add( &HoveredThumbImage );
	OutBrushes.Add( &DraggedThumbImage );
}

const FName FScrollBarStyle::TypeName( TEXT("FScrollBarStyle") );

const FScrollBarStyle& FScrollBarStyle::GetDefault()
{
	static FScrollBarStyle Default;
	return Default;
}


FExpandableAreaStyle::FExpandableAreaStyle()
{
}

void FExpandableAreaStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &CollapsedImage );
	OutBrushes.Add( &ExpandedImage );
}

const FName FExpandableAreaStyle::TypeName( TEXT("FExpandableAreaStyle") );

const FExpandableAreaStyle& FExpandableAreaStyle::GetDefault()
{
	static FExpandableAreaStyle Default;
	return Default;
}


FSearchBoxStyle::FSearchBoxStyle()
{
}

FSearchBoxStyle& FSearchBoxStyle::SetTextBoxStyle( const FEditableTextBoxStyle& InTextBoxStyle )
{ 
	TextBoxStyle = InTextBoxStyle;
	if (ActiveFontInfo.FontName.IsNone())
	{
		ActiveFontInfo = TextBoxStyle.Font;
	}
	return *this;
}

void FSearchBoxStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	TextBoxStyle.GetResources(OutBrushes);
	OutBrushes.Add( &UpArrowImage );
	OutBrushes.Add( &DownArrowImage );
	OutBrushes.Add( &GlassImage );
	OutBrushes.Add( &ClearImage );
}

const FName FSearchBoxStyle::TypeName( TEXT("FSearchBoxStyle") );

const FSearchBoxStyle& FSearchBoxStyle::GetDefault()
{
	static FSearchBoxStyle Default;
	return Default;
}


FSliderStyle::FSliderStyle()
{
}

void FSliderStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &NormalThumbImage );
	OutBrushes.Add( &DisabledThumbImage );
}

const FName FSliderStyle::TypeName( TEXT("FSliderStyle") );

const FSliderStyle& FSliderStyle::GetDefault()
{
	static FSliderStyle Default;
	return Default;
}


FVolumeControlStyle::FVolumeControlStyle()
{
}

void FVolumeControlStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	SliderStyle.GetResources(OutBrushes);
	OutBrushes.Add( &HighVolumeImage );
	OutBrushes.Add( &MidVolumeImage );
	OutBrushes.Add( &LowVolumeImage );
	OutBrushes.Add( &NoVolumeImage );
	OutBrushes.Add( &MutedImage );
}

const FName FVolumeControlStyle::TypeName( TEXT("FVolumeControlStyle") );

const FVolumeControlStyle& FVolumeControlStyle::GetDefault()
{
	static FVolumeControlStyle Default;
	return Default;
}

FInlineTextImageStyle::FInlineTextImageStyle()
	: Image()
	, Baseline( 0 )
{
}

void FInlineTextImageStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &Image );
}

const FName FInlineTextImageStyle::TypeName( TEXT("FInlineTextImageStyle") );

const FInlineTextImageStyle& FInlineTextImageStyle::GetDefault()
{
	static FInlineTextImageStyle Default;
	return Default;
}

FSpinBoxStyle::FSpinBoxStyle()
	: ForegroundColor(FSlateColor::UseForeground())
	, TextPadding(FMargin(1.0f,2.0f))
{
}

void FSpinBoxStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &BackgroundBrush );
	OutBrushes.Add( &HoveredBackgroundBrush );
	OutBrushes.Add( &ActiveFillBrush );
	OutBrushes.Add( &InactiveFillBrush );
	OutBrushes.Add( &ArrowsImage );
}

const FName FSpinBoxStyle::TypeName( TEXT("FSpinBoxStyle") );

const FSpinBoxStyle& FSpinBoxStyle::GetDefault()
{
	static FSpinBoxStyle Default;
	return Default;
}


FSplitterStyle::FSplitterStyle()
{
}

void FSplitterStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &HandleNormalBrush );
	OutBrushes.Add( &HandleHighlightBrush );
}

const FName FSplitterStyle::TypeName( TEXT("FSplitterStyle") );

const FSplitterStyle& FSplitterStyle::GetDefault()
{
	static FSplitterStyle Default;
	return Default;
}


FTableRowStyle::FTableRowStyle()
	: TextColor(FSlateColor::UseForeground())
	, SelectedTextColor(FLinearColor::White)
{
}

void FTableRowStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &SelectorFocusedBrush );
	OutBrushes.Add( &ActiveHoveredBrush );
	OutBrushes.Add( &ActiveBrush );
	OutBrushes.Add( &InactiveHoveredBrush );
	OutBrushes.Add( &InactiveBrush );
	OutBrushes.Add( &EvenRowBackgroundHoveredBrush );
	OutBrushes.Add( &EvenRowBackgroundBrush );
	OutBrushes.Add( &OddRowBackgroundHoveredBrush );
	OutBrushes.Add( &OddRowBackgroundBrush );
}

const FName FTableRowStyle::TypeName( TEXT("FTableRowStyle") );

const FTableRowStyle& FTableRowStyle::GetDefault()
{
	static FTableRowStyle Default;
	return Default;
}


FTableColumnHeaderStyle::FTableColumnHeaderStyle()
{
}

void FTableColumnHeaderStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &SortAscendingImage );
	OutBrushes.Add( &SortDescendingImage );
	OutBrushes.Add( &NormalBrush );
	OutBrushes.Add( &HoveredBrush );
	OutBrushes.Add( &MenuDropdownImage );
	OutBrushes.Add( &MenuDropdownNormalBorderBrush );
	OutBrushes.Add( &MenuDropdownHoveredBorderBrush );
}

const FName FTableColumnHeaderStyle::TypeName( TEXT("FTableColumnHeaderStyle") );

const FTableColumnHeaderStyle& FTableColumnHeaderStyle::GetDefault()
{
	static FTableColumnHeaderStyle Default;
	return Default;
}


FHeaderRowStyle::FHeaderRowStyle()
{
}

void FHeaderRowStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	ColumnStyle.GetResources(OutBrushes);
	LastColumnStyle.GetResources(OutBrushes);
	ColumnSplitterStyle.GetResources(OutBrushes);
	OutBrushes.Add( &BackgroundBrush );
}

const FName FHeaderRowStyle::TypeName( TEXT("FHeaderRowStyle") );

const FHeaderRowStyle& FHeaderRowStyle::GetDefault()
{
	static FHeaderRowStyle Default;
	return Default;
}


FDockTabStyle::FDockTabStyle()
	: OverlapWidth(0.0f)
{
}

void FDockTabStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	CloseButtonStyle.GetResources(OutBrushes);
	OutBrushes.Add( &NormalBrush );
	OutBrushes.Add( &ActiveBrush );
	OutBrushes.Add( &ColorOverlayBrush );
	OutBrushes.Add( &ForegroundBrush );
	OutBrushes.Add( &HoveredBrush );
	OutBrushes.Add( &ContentAreaBrush );
	OutBrushes.Add( &TabWellBrush );
}

const FName FDockTabStyle::TypeName( TEXT("FDockTabStyle") );

const FDockTabStyle& FDockTabStyle::GetDefault()
{
	static FDockTabStyle Default;
	return Default;
}


FScrollBoxStyle::FScrollBoxStyle()
{
}

void FScrollBoxStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &TopShadowBrush );
	OutBrushes.Add( &BottomShadowBrush );
}

const FName FScrollBoxStyle::TypeName( TEXT("FScrollBoxStyle") );

const FScrollBoxStyle& FScrollBoxStyle::GetDefault()
{
	static FScrollBoxStyle Default;
	return Default;
}


FScrollBorderStyle::FScrollBorderStyle()
{
}

void FScrollBorderStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	OutBrushes.Add( &TopShadowBrush );
	OutBrushes.Add( &BottomShadowBrush );
}

const FName FScrollBorderStyle::TypeName( TEXT( "FScrollBorderStyle" ) );

const FScrollBorderStyle& FScrollBorderStyle::GetDefault()
{
	static FScrollBorderStyle Default;
	return Default;
}


FWindowStyle::FWindowStyle()
	: OutlineColor( FLinearColor(0.1f, 0.1f, 0.1f, 1.0f) )
{
}

void FWindowStyle::GetResources( TArray< const FSlateBrush* >& OutBrushes ) const
{
	MinimizeButtonStyle.GetResources(OutBrushes);
	MaximizeButtonStyle.GetResources(OutBrushes);
	RestoreButtonStyle.GetResources(OutBrushes);
	CloseButtonStyle.GetResources(OutBrushes);

	TitleTextStyle.GetResources(OutBrushes);

	OutBrushes.Add( &ActiveTitleBrush );
	OutBrushes.Add( &InactiveTitleBrush );
	OutBrushes.Add( &FlashTitleBrush );
	OutBrushes.Add( &BorderBrush );
	OutBrushes.Add( &OutlineBrush );
	OutBrushes.Add( &BackgroundBrush );
	OutBrushes.Add( &ChildBackgroundBrush );
}

const FName FWindowStyle::TypeName( TEXT("FWindowStyle") );

const FWindowStyle& FWindowStyle::GetDefault()
{
	static FWindowStyle Default;
	return Default;
}


USlateTypes::USlateTypes( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
}
