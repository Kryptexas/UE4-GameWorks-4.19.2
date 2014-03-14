// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


/** Constructor */
SVirtualKeyboardEntry::SVirtualKeyboardEntry()
	: ScrollHelper(),
	  bWasFocusedByLastMouseDown( false ),
	  bIsChangingText( false )
{
}


/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SVirtualKeyboardEntry::Construct( const FArguments& InArgs )
{
	Text = InArgs._Text;
	HintText = InArgs._HintText;

	Font = InArgs._Font;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	IsReadOnly = InArgs._IsReadOnly;
	ClearKeyboardFocusOnCommit = InArgs._ClearKeyboardFocusOnCommit;
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;
	MinDesiredWidth = InArgs._MinDesiredWidth;
	KeyboardType = InArgs._KeyboardType;
}

/**
 * Sets the text string currently being edited 
 *
 * @param  InNewText  The new text string
 */
void SVirtualKeyboardEntry::SetText( const TAttribute< FText >& InNewText )
{
	EditedText = InNewText.Get();

	// Don't set text if the text attribute has a 'getter' binding on it, otherwise we'd blow away
	// that binding.  If there is a getter binding, then we'll assume it will provide us with
	// updated text after we've fired our 'text changed' callbacks
	if( !Text.IsBound() )
	{
		Text.Set ( EditedText );
	}

	bNeedsUpdate = true;

	// Let outsiders know that the text content has been changed
	//OnTextChanged.ExecuteIfBound( HasKeyboardFocus() ? EditedText : Text.Get() );
}

/**
 * Restores the text to the original state
 */
void SVirtualKeyboardEntry::RestoreOriginalText()
{
	if( HasTextChangedFromOriginal() )
	{
		SetText(OriginalText);

		// Let outsiders know that the text content has been changed
		OnTextChanged.ExecuteIfBound( EditedText );
	}
}

bool SVirtualKeyboardEntry::HasTextChangedFromOriginal() const
{
	return ( !IsReadOnly.Get() && !EditedText.EqualTo(OriginalText)  );
}

/**
 * Sets the font used to draw the text
 *
 * @param  InNewFont	The new font to use
 */
void SVirtualKeyboardEntry::SetFont( const TAttribute< FSlateFontInfo >& InNewFont )
{
	Font = InNewFont;
}

/**
 * Checks to see if this widget supports keyboard focus.  Override this in derived classes.
 *
 * @return  True if this widget can take keyboard focus
 */
bool SVirtualKeyboardEntry::SupportsKeyboardFocus() const
{
	return true;
}

bool SVirtualKeyboardEntry::GetIsReadOnly() const
{
	return IsReadOnly.Get();
}

/**
 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
 *
 * @param  AllottedGeometry Space allotted to this widget
 * @param  InCurrentTime  Current absolute real time
 * @param  InDeltaTime  Real time passed since last tick
 */
void SVirtualKeyboardEntry::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// Call parent implementation.
	SLeafWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

	if(bNeedsUpdate)
	{
		// Let outsiders know that the text content has been changed
		OnTextChanged.ExecuteIfBound( HasKeyboardFocus() ? EditedText : Text.Get() );
		bNeedsUpdate = false;
	}


	// We'll draw with the 'focused' look if we're either focused or we have a context menu summoned
	const bool bShouldAppearFocused = HasKeyboardFocus();
}

/**
 * Gets the height of the largest character in the font
 *
 * @return  The fonts height
 */
float SVirtualKeyboardEntry::GetFontHeight() const
{
	// Take a local copy of the FontInfo and remove any flags that could affect the outcome in a negative way
	FSlateFontInfo FontCopy = Font.Get();

	// Aesthetic choice: editable text can use a little more room, and should never be smaller than a W of the font.
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D FontSize = FontMeasureService->Measure( FString(TEXT("W")), FontCopy );
	return FontSize.Y;
}

/**
 * Computes the desired size of this widget (SWidget)
 *
 * @return  The widget's desired size
 */
FVector2D SVirtualKeyboardEntry::ComputeDesiredSize() const
{
	const FSlateFontInfo& FontInfo = Font.Get();
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const float FontMaxCharHeight = GetFontHeight();

	FVector2D TextSize;

	const FString StringToRender = GetStringToRender();
	if( !StringToRender.IsEmpty() )
	{
		TextSize = FontMeasureService->Measure( StringToRender, FontInfo );
	}
	else
	{
		TextSize = FontMeasureService->Measure( HintText.Get().ToString(), FontInfo );
	}
	
	return FVector2D( FMath::Max(TextSize.X, MinDesiredWidth.Get()), FMath::Max(TextSize.Y, FontMaxCharHeight) );
}

/**
 * The widget should respond by populating the OutDrawElements array with FDrawElements 
 * that represent it and any of its children.
 *
 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
 * @param OutDrawElements   A list of FDrawElements to populate with the output.
 * @param LayerId           The Layer onto which this widget should be rendered.
 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 * @param bParentEnabled	True if the parent of this widget is enabled.
 *
 * @return The maximum layer ID attained by this widget or any of its children.
 */
int32 SVirtualKeyboardEntry::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// The text and some effects draws in front of the widget's background and selection.
	const int32 SelectionLayer = 0;
	const int32 TextLayer = 1;

	// See if a disabled effect should be used
	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	bool bIsReadonly = IsReadOnly.Get();
	ESlateDrawEffect::Type DrawEffects = (bEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const FSlateFontInfo& FontInfo = Font.Get();
	const FString VisibleText = GetStringToRender();
	const FLinearColor ThisColorAndOpacity = ColorAndOpacity.Get().GetColor(InWidgetStyle);
	const FColor ColorAndOpacitySRGB = ThisColorAndOpacity * InWidgetStyle.GetColorAndOpacityTint();
	const float FontMaxCharHeight = GetFontHeight();
	const double CurrentTime = FSlateApplication::Get().GetCurrentTime();

	// We'll draw with the 'focused' look if we're either focused or we have a context menu summoned
	const bool bShouldAppearFocused = HasKeyboardFocus();

	const float DrawPositionY = ( AllottedGeometry.Size.Y / 2 ) - ( FontMaxCharHeight / 2 );

	if (VisibleText.Len() == 0)
	{
		// Draw the hint text.
		const FLinearColor HintTextColor = FLinearColor(ColorAndOpacitySRGB.R, ColorAndOpacitySRGB.G, ColorAndOpacitySRGB.B, 0.35f);
		const FString ThisHintText = this->HintText.Get().ToString();
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + TextLayer,
			AllottedGeometry.ToPaintGeometry( FVector2D( 0, DrawPositionY ), AllottedGeometry.Size ),
			ThisHintText,          // Text
			FontInfo,              // Font information (font name, size)
			MyClippingRect,        // Clipping rect
			DrawEffects,           // Effects to use
			HintTextColor          // Color
		);
	}
	else
	{
		// Draw the text

		// Only draw the text that's potentially visible
		const float ScrollAreaLeft = ScrollHelper.ToScrollerSpace( FVector2D::ZeroVector ).X;
		const float ScrollAreaRight = ScrollHelper.ToScrollerSpace( AllottedGeometry.Size ).X;
		const int32 FirstVisibleCharacter = FontMeasureService->FindCharacterIndexAtOffset( VisibleText, FontInfo, ScrollAreaLeft );
		const int32 LastVisibleCharacter = FontMeasureService->FindCharacterIndexAtOffset( VisibleText, FontInfo, ScrollAreaRight );
		const FString PotentiallyVisibleText( VisibleText.Mid( FirstVisibleCharacter, ( LastVisibleCharacter - FirstVisibleCharacter ) + 1 ) );
		const float FirstVisibleCharacterPosition = FontMeasureService->Measure( VisibleText.Left( FirstVisibleCharacter ), FontInfo ).X;

		const float TextVertOffset = FontMaxCharHeight;
		const FVector2D DrawPosition = ScrollHelper.FromScrollerSpace( FVector2D( FirstVisibleCharacterPosition, DrawPositionY + TextVertOffset ) );
		const FVector2D DrawSize = ScrollHelper.SizeFromScrollerSpace( AllottedGeometry.Size );

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + TextLayer,
			AllottedGeometry.ToPaintGeometry( DrawPosition, DrawSize ),
			PotentiallyVisibleText,          // Text
			FontInfo,                        // Font information (font name, size)
			MyClippingRect,                  // Clipping rect
			DrawEffects,                     // Effects to use
			ColorAndOpacitySRGB              // Color
		);
	}
	
	return LayerId + TextLayer;
}

FReply SVirtualKeyboardEntry::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	// The user wants to edit text. Make a copy of the observed text for the user to edit.
	EditedText = Text.Get();

	int32 CaretPosition = EditedText.ToString().Len();
	FSlateApplication& CurrentApp = FSlateApplication::Get();
	CurrentApp.ShowKeyboard(true, SharedThis(this));

	return FReply::Handled();
}

/**
 * Called when this widget loses the keyboard focus.  This event does not bubble.
 *
 * @param  InKeyboardFocusEvent  KeyboardFocusEvent
 */
void SVirtualKeyboardEntry::OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	// See if user explicitly tabbed away or moved focus
	ETextCommit::Type TextAction;
	switch ( InKeyboardFocusEvent.GetCause() )
	{
		case EKeyboardFocusCause::Keyboard:
		case EKeyboardFocusCause::Mouse:
			TextAction = ETextCommit::OnUserMovedFocus;
			break;

		case EKeyboardFocusCause::Cleared:
			TextAction = ETextCommit::OnCleared;
			break;

		default:
			TextAction = ETextCommit::Default;
			break;
	}

	FSlateApplication& CurrentApp = FSlateApplication::Get();
	CurrentApp.ShowKeyboard(false);

	OnTextCommitted.ExecuteIfBound( EditedText, TextAction );
}

/**
 * Gets the text that needs to be rendered
 *
 * @return  Text string
 */
FString SVirtualKeyboardEntry::GetStringToRender() const
{
	FString VisibleText;

	if(HasKeyboardFocus())
	{
		VisibleText = EditedText.ToString();
	}
	else
	{
		VisibleText = Text.Get().ToString();
	}

	// If this string is a password we need to replace all the characters in the string with something else
	if ( KeyboardType.Get() == EKeyboardType::Keyboard_Password )
	{
		const TCHAR Dot = 0x25CF;
		int32 VisibleTextLen = VisibleText.Len();
		VisibleText.Empty();
		while ( VisibleTextLen )
		{
			VisibleText += Dot;
			VisibleTextLen--;
		}
	}

	return VisibleText;
}
