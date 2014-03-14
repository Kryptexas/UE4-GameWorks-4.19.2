// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


class SSimpleGradient : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSimpleGradient )
		: _StartColor( FLinearColor(0,0,0) )
		, _EndColor( FLinearColor(1,1,1) )
		, _HasAlphaBackground( false )
		, _Orientation( Orient_Vertical )
		, _UseSRGB( true )
		{}

		/** The leftmost gradient color */
		SLATE_ATTRIBUTE( FLinearColor, StartColor )
		
		/** The rightmost gradient color */
		SLATE_ATTRIBUTE( FLinearColor, EndColor )

		/** Whether a checker background is displayed for alpha viewing */
		SLATE_ATTRIBUTE( bool, HasAlphaBackground )

		/** Horizontal or vertical gradient */
		SLATE_ATTRIBUTE( EOrientation, Orientation)

		/** Whether to display sRGB color */
		SLATE_ATTRIBUTE( bool, UseSRGB )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs )
	{
		StartColor = InArgs._StartColor;
		EndColor = InArgs._EndColor;
		bHasAlphaBackground = InArgs._HasAlphaBackground.Get();
		Orientation = InArgs._Orientation.Get();
	}

private:
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
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
	{
		const ESlateDrawEffect::Type DrawEffects = this->ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		if (bHasAlphaBackground)
		{
			const FSlateBrush* StyleInfo = FCoreStyle::Get().GetBrush("ColorPicker.AlphaBackground");

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				StyleInfo,
				MyClippingRect,
				DrawEffects
			);
		}

		TArray<FSlateGradientStop> GradientStops;

		GradientStops.Add(FSlateGradientStop(FVector2D::ZeroVector, StartColor.Get()));
		GradientStops.Add(FSlateGradientStop(AllottedGeometry.Size, EndColor.Get()));

		FSlateDrawElement::MakeGradient(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			GradientStops,
			Orientation,
			MyClippingRect,
			DrawEffects,
			false
		);

		return LayerId + 1;
	}
	
	/** The leftmost gradient color */
	TAttribute<FLinearColor> StartColor;

	/** The rightmost gradient color */
	TAttribute<FLinearColor> EndColor;

	/** Whether a checker background is displayed for alpha viewing */
	bool bHasAlphaBackground;
	
	/** Horizontal or vertical gradient */
	EOrientation Orientation;

	/** Whether to display sRGB color */
	TAttribute<bool> UseSRGB;
};


class SComplexGradient : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SComplexGradient )
		: _GradientColors()
		, _HasAlphaBackground( false )
		, _Orientation( Orient_Vertical )
		{}

		/** The colors used in the gradient */
		SLATE_ATTRIBUTE( TArray<FLinearColor>, GradientColors )

		/** Whether a checker background is displayed for alpha viewing */
		SLATE_ATTRIBUTE( bool, HasAlphaBackground )
		
		/** Horizontal or vertical gradient */
		SLATE_ATTRIBUTE( EOrientation, Orientation)

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs )
	{
		GradientColors = InArgs._GradientColors;
		bHasAlphaBackground = InArgs._HasAlphaBackground.Get();
		Orientation = InArgs._Orientation.Get();
	}

private:
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
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
	{
		ESlateDrawEffect::Type DrawEffects = (bParentEnabled && IsEnabled()) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		if (bHasAlphaBackground)
		{
			const FSlateBrush* StyleInfo = FCoreStyle::Get().GetBrush("ColorPicker.AlphaBackground");

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				StyleInfo,
				MyClippingRect,
				DrawEffects
				);
		}

		if (GradientColors.Get().Num())
		{
			TArray<FSlateGradientStop> GradientStops;
			for (int32 i = 0; i < GradientColors.Get().Num(); ++i)
			{
				GradientStops.Add( FSlateGradientStop( AllottedGeometry.Size * (float(i) / (GradientColors.Get().Num() - 1)),
					GradientColors.Get()[i] ) );
			}

			FSlateDrawElement::MakeGradient(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				GradientStops,
				Orientation,
				MyClippingRect,
				DrawEffects
				);
		}

		return LayerId + 1;
	}
	
	/** The colors used in the gradient */
	TAttribute< TArray<FLinearColor> > GradientColors;

	/** Whether a checker background is displayed for alpha viewing */
	bool bHasAlphaBackground;
	
	/** Horizontal or vertical gradient */
	EOrientation Orientation;
};


/** The value slider is a simple control like the color wheel for selecting value */
class SValueSlider : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS( SValueSlider )
		: _SelectedColor()
		, _OnValueChanged()
		, _OnMouseCaptureBegin()
		, _OnMouseCaptureEnd()
		{}
		
		/** The current color selected by the user */
		SLATE_ATTRIBUTE( FLinearColor, SelectedColor )
		
		/** Invoked when a new value is selected on the color wheel */
		SLATE_EVENT( FOnLinearColorValueChanged, OnValueChanged )
		
		/** Invoked when the mouse is pressed and sliding begins */
		SLATE_EVENT( FSimpleDelegate, OnMouseCaptureBegin )

		/** Invoked when the mouse is released and sliding ends */
		SLATE_EVENT( FSimpleDelegate, OnMouseCaptureEnd )

	SLATE_END_ARGS()
	
	void Construct( const FArguments& InArgs )
	{
		SelectorImage = FCoreStyle::Get().GetBrush("ColorPicker.Selector");

		OnValueChanged = InArgs._OnValueChanged;
		OnMouseCaptureBegin = InArgs._OnMouseCaptureBegin;
		OnMouseCaptureEnd = InArgs._OnMouseCaptureEnd;
		SelectedColor = InArgs._SelectedColor;
	}

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
	{
		const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
		const uint32 DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	
		FLinearColor FullValueColor = SelectedColor.Get();
		FullValueColor.B = FullValueColor.A = 1.f;
		FLinearColor StopColor = FullValueColor.HSVToLinearRGB();

		TArray<FSlateGradientStop> GradientStops;
		GradientStops.Add( FSlateGradientStop( FVector2D::ZeroVector, FLinearColor(0,0,0,1.0f) ) );
		GradientStops.Add( FSlateGradientStop( AllottedGeometry.Size, StopColor ) );

		FSlateDrawElement::MakeGradient(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GradientStops,
			Orient_Vertical,
			MyClippingRect,
			DrawEffects
		);
	
		float Value = SelectedColor.Get().B;
		FVector2D RelativeSelectedPosition = FVector2D(Value, 0.5f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry( RelativeSelectedPosition*AllottedGeometry.Size - SelectorImage->ImageSize*0.5, SelectorImage->ImageSize ),
			SelectorImage,
			MyClippingRect,
			DrawEffects,
			InWidgetStyle.GetColorAndOpacityTint() * SelectorImage->GetTint( InWidgetStyle ) );

		return LayerId + 1;
	}
	
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			OnMouseCaptureBegin.ExecuteIfBound();

			return FReply::Handled().CaptureMouse( SharedThis(this) );
		}
		else
		{
			return FReply::Unhandled();
		}
	}
	
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture() )
		{
			OnMouseCaptureEnd.ExecuteIfBound();

			return FReply::Handled().ReleaseMouseCapture();
		}
		else
		{
			return FReply::Unhandled();
		}
	}

	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		if (!HasMouseCapture())
		{
			return FReply::Unhandled();
		}

		FVector2D LocalMouseCoordinate = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
		FVector2D Location = LocalMouseCoordinate / (MyGeometry.Size);
		float Value = FMath::Clamp(Location.X, 0.f, 1.f);

		FLinearColor NewColor = SelectedColor.Get();
		NewColor.B = Value;

		OnValueChanged.ExecuteIfBound(NewColor);

		return FReply::Handled();
	}
	
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
	{
		return FReply::Handled();
	}

	virtual FVector2D ComputeDesiredSize() const
	{
		return SelectorImage->ImageSize * 2.f;
	}
	
protected:

	/** The color selector image to show */
	const FSlateBrush* SelectorImage;
	
	/** The current color selected by the user */
	TAttribute< FLinearColor > SelectedColor;

	/** Invoked when a new value is selected on the color wheel */
	FOnLinearColorValueChanged OnValueChanged;

	/** Invoked when the mouse is pressed */
	FSimpleDelegate OnMouseCaptureBegin;

	/** Invoked when the mouse is let up */
	FSimpleDelegate OnMouseCaptureEnd;
};



#define LOCTEXT_NAMESPACE "EyeDroppperButton"

/**
 * Class for placing a color picker eye-dropper button.
 * A self contained until that only needs client code to set the display gamma and listen
 * for the OnValueChanged events. It toggles the dropper when clicked.
 * When active it captures the mouse, shows a dropper cursor and samples the pixel color constantly.
 * It is stopped normally by hitting the Esc key.
 */
class SEyeDropperButton : public SButton
{
public:
	DECLARE_DELEGATE(FOnDropperComplete)

	SLATE_BEGIN_ARGS( SEyeDropperButton )
		: _OnValueChanged()
		, _OnBegin()
		, _OnComplete()
		, _DisplayGamma()
		{}

		/** Invoked when a new value is selected by the dropper */
		SLATE_EVENT(FOnLinearColorValueChanged, OnValueChanged)

		/** Invoked when the dropper goes from inactive to active */
		SLATE_EVENT(FSimpleDelegate, OnBegin)

		/** Invoked when the dropper goes from active to inactive */
		SLATE_EVENT(FOnDropperComplete, OnComplete)

		/** Sets the display Gamma setting - used to correct colors sampled from the screen */
		SLATE_ATTRIBUTE(float, DisplayGamma)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs )
	{
		OnValueChanged = InArgs._OnValueChanged;
		OnBegin = InArgs._OnBegin;
		OnComplete = InArgs._OnComplete;
		DisplayGamma = InArgs._DisplayGamma;

		// This is a button containing an dropper image and text that tells the user to hit Esc.
		// Their visibility and colors are changed according to whether dropper mode is active or not.
		SButton::Construct(
			SButton::FArguments()
				.ContentPadding(1.0f)
				.OnClicked(this, &SEyeDropperButton::OnClicked)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
						.Padding(FMargin(1.0f, 0.0f))
						[
							SNew(SImage)
								.Image(FCoreStyle::Get().GetBrush("ColorPicker.EyeDropper"))
								.ToolTipText(LOCTEXT("EyeDropperButton_ToolTip", "Activates the eye-dropper for selecting a colored pixel from any window.").ToString())
								.ColorAndOpacity(this, &SEyeDropperButton::GetDropperImageColor)				
						]

					+ SOverlay::Slot()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("EscapeCue", "Esc").ToString())
								.ToolTipText(LOCTEXT("EyeDropperEscapeCue_ToolTip", "Hit Escape key to stop the eye dropper").ToString())
								.Visibility(this, &SEyeDropperButton::GetEscapeTextVisibility)
						]
				]
		);
	}

	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		// Clicking ANY mouse button when the dropper isn't active resets the active dropper states ready to activate
		if (!HasMouseCapture())
		{
			bWasClickActivated = false;
			bWasLeft = false;
			bWasReEntered = false;
		}

		return SButton::OnMouseButtonDown(MyGeometry, MouseEvent);
	}

	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		// If a mouse click is completing and the dropper is active ALWAYS deactivate
		bool bDeactivating = false;
		if (bWasClickActivated)
		{
			bDeactivating = true;
		}

		// bWasClicked is reset here because if it is set during SButton::OnMouseButtonUp()
		// then the button was 'clicked' according to the usual rules. We might want to capture the
		// mouse when the button is clicked but can't do it in the Clicked callback.
		bWasClicked = false;
		FReply Reply = SButton::OnMouseButtonUp(MyGeometry, MouseEvent);

		// Switching dropper mode off?
		if (bDeactivating)
		{
			// These state flags clear dropper mode
			bWasClickActivated = false;
			bWasLeft = false;
			bWasReEntered = false;

			Reply.ReleaseMouseCapture();

			OnComplete.ExecuteIfBound();
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bWasClicked)
		{
			// A normal LMB mouse click on the button occurred
			// Set the initial dropper mode state and capture the mouse
			bWasClickActivated = true;
			bWasLeft = false;
			bWasReEntered = false;

			OnBegin.ExecuteIfBound();

			Reply.CaptureMouse(this->AsShared());
		}
		bWasClicked = false;

		return Reply;
	}

	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		// If the mouse is captured and bWasClickActivated is set then we are in dropper mode
		if (HasMouseCapture() && bWasClickActivated)
		{
			if (IsMouseOver(MyGeometry, MouseEvent))
			{
				if (bWasLeft)
				{
					// Mouse is over the button having left it once
					bWasReEntered = true;
				}
			}
			else
			{
				// Mouse is outside the button
				bWasLeft = true;
				bWasReEntered = false;

				// In dropper mode and outside the button - sample the pixel color and push it to the client
				FLinearColor ScreenColor = FPlatformMisc::GetScreenPixelColor(FSlateApplication::Get().GetCursorPos(), false);
				OnValueChanged.ExecuteIfBound(ScreenColor);
			}
		}

		return SButton::OnMouseMove( MyGeometry, MouseEvent );
	}

	FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
	{
		// Escape key when in dropper mode
		if (InKeyboardEvent.GetKey() == EKeys::Escape &&
			HasMouseCapture() &&
			bWasClickActivated)
		{
			// Clear the dropper mode states
			bWasClickActivated = false;
			bWasLeft = false;
			bWasReEntered = false;

			// This is needed to switch the dropper cursor off immediately so the user can see the Esc key worked
			FSlateApplication::Get().QueryCursor();

			FReply ReleaseReply = FReply::Handled().ReleaseMouseCapture();

			OnComplete.ExecuteIfBound();

			return ReleaseReply;
		}

		return FReply::Unhandled();
	}

	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE
	{
		// Cursor is changed to the dropper when dropper mode is active and the states are correct
		if (HasMouseCapture() &&
			bWasClickActivated && bWasLeft && !bWasReEntered )
		{
			return FCursorReply::Cursor( EMouseCursor::EyeDropper );
		}

		return SButton::OnCursorQuery( MyGeometry, CursorEvent );
	}

	FReply OnClicked()
	{
		// Log clicks so that OnMouseButtonUp() can post process clicks
		bWasClicked = true;
		return FReply::Handled();
	}

private:

	EVisibility GetEscapeTextVisibility() const
	{
		// Show the Esc key message in the button when dropper mode is active
		if (HasMouseCapture() && bWasClickActivated)
		{
			return EVisibility::Visible;
		}
		return EVisibility::Hidden;
	}

	FSlateColor GetDropperImageColor() const
	{
		// Make the dropper image in the button pale when dropper mode is active
		if (HasMouseCapture() && bWasClickActivated)
		{
			return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
		}
		return FSlateColor::UseForeground();
	}

	bool IsMouseOver(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
	{ 
		return MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
	}

	/** Invoked when a new value is selected by the dropper */
	FOnLinearColorValueChanged OnValueChanged;

	/** Invoked when the dropper goes from inactive to active */
	FSimpleDelegate OnBegin;

	/** Invoked when the dropper goes from active to inactive - can be used to commit colors by the owning picker */
	FOnDropperComplete OnComplete;

	/** Sets the display Gamma setting - used to correct colors sampled from the screen */
	TAttribute<float> DisplayGamma;

	// Dropper states
	bool bWasClicked;
	bool bWasClickActivated;
	bool bWasLeft;
	bool bWasReEntered;
};

#undef LOCTEXT_NAMESPACE


FColorTheme::FColorTheme( const FString& InName, const TArray< TSharedPtr<FLinearColor> >& InColors )
	: Name(InName)
	, Colors(InColors)
	, RefreshEvent()
{ }


void FColorTheme::InsertNewColor( TSharedPtr<FLinearColor> InColor, int32 InsertPosition )
{
	Colors.Insert(InColor, InsertPosition);
	RefreshEvent.Broadcast();
}


int32 FColorTheme::FindApproxColor( const FLinearColor& InColor, float Tolerance ) const
{
	for (int32 ColorIndex = 0; ColorIndex < Colors.Num(); ++ColorIndex)
	{
		if (Colors[ColorIndex]->Equals(InColor, Tolerance))
		{
			return ColorIndex;
		}
	}

	return INDEX_NONE;
}


void FColorTheme::RemoveAll()
{
	Colors.Empty();
	RefreshEvent.Broadcast();
}


void FColorTheme::RemoveColor( int32 ColorIndex )
{
	Colors.RemoveAt(ColorIndex);
	RefreshEvent.Broadcast();
}


int32 FColorTheme::RemoveColor( const TSharedPtr<FLinearColor> InColor )
{
	const int32 Position = Colors.Find(InColor);
	if (Position != INDEX_NONE)
	{
		RemoveColor(Position);
	}

	return Position;
}


#define LOCTEXT_NAMESPACE "ColorPicker"

/** A default window size for the color picker which looks nice */
const FVector2D SColorPicker::DEFAULT_WINDOW_SIZE = FVector2D(308, 458);

/** The max time allowed for updating before we shut off auto-updating */
const double SColorPicker::MAX_ALLOWED_UPDATE_TIME = 0.1;

TSharedPtr<SColorThemesViewer> SColorPicker::ColorThemesViewer;


/* SColorPicker structors
 *****************************************************************************/

SColorPicker::~SColorPicker()
{
	if (ColorThemesViewer.IsValid())
	{
		ColorThemesViewer->OnCurrentThemeChanged().RemoveAll( this );
	}
}


/* SColorPicker methods
 *****************************************************************************/

void SColorPicker::Construct( const FArguments& InArgs )
{
	TargetColorAttribute = InArgs._TargetColorAttribute;
	CurrentColorHSV = OldColor = TargetColorAttribute.Get().LinearRGBToHSV();
	CurrentColorRGB = TargetColorAttribute.Get();
	CurrentMode = EColorPickerModes::Wheel;
	TargetFColors = InArgs._TargetFColors.Get();
	TargetLinearColors = InArgs._TargetLinearColors.Get();
	TargetColorChannels = InArgs._TargetColorChannels.Get();
	bUseAlpha = InArgs._UseAlpha;
	bOnlyRefreshOnMouseUp = InArgs._OnlyRefreshOnMouseUp.Get();
	bOnlyRefreshOnOk = InArgs._OnlyRefreshOnOk.Get();
	OnColorCommitted = InArgs._OnColorCommitted;
	PreColorCommitted = InArgs._PreColorCommitted;
	OnColorPickerCancelled = InArgs._OnColorPickerCancelled;
	OnInteractivePickBegin = InArgs._OnInteractivePickBegin;
	OnInteractivePickEnd = InArgs._OnInteractivePickEnd;
	OnColorPickerWindowClosed = InArgs._OnColorPickerWindowClosed;
	ParentWindowPtr = InArgs._ParentWindow.Get();
	DisplayGamma = InArgs._DisplayGamma;
	bClosedViaOkOrCancel = false;

	// We need a parent window to set the close callback
	if (ParentWindowPtr.IsValid())
	{
		ParentWindowPtr.Pin()->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &SColorPicker::HandleParentWindowClosed));
	}

	bColorPickerIsInlineVersion = InArgs._DisplayInlineVersion;
	bIsInteractive = false;
	bPerfIsTooSlowToUpdate = false;

	BackupColors();

	BeginAnimation(FLinearColor(ForceInit), CurrentColorHSV);

	bool bAdvancedSectionExpanded = false;

	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		bool WheelMode = true;

		GConfig->GetBool(TEXT("ColorPickerUI"), TEXT("bWheelMode"), WheelMode, GEditorUserSettingsIni);
		GConfig->GetBool(TEXT("ColorPickerUI"), TEXT("bAdvancedSectionExpanded"), bAdvancedSectionExpanded, GEditorUserSettingsIni);
		GConfig->GetBool(TEXT("ColorPickerUI"), TEXT("bSRGBEnabled"), SColorThemesViewer::bSRGBEnabled, GEditorUserSettingsIni);

		CurrentMode = WheelMode ? EColorPickerModes::Wheel : EColorPickerModes::Spectrum;
	}

	if (bColorPickerIsInlineVersion)
	{
		GenerateInlineColorPickerContent();
	}
	else
	{
		GenerateDefaultColorPickerContent(bAdvancedSectionExpanded);
	}
}


/* SColorPicker implementation
 *****************************************************************************/

void SColorPicker::BackupColors()
{
	OldTargetFColors.Empty();
	for (int32 i = 0; i < TargetFColors.Num(); ++i)
	{
		OldTargetFColors.Add( *TargetFColors[i] );
	}

	OldTargetLinearColors.Empty();
	for (int32 i = 0; i < TargetLinearColors.Num(); ++i)
	{
		OldTargetLinearColors.Add( *TargetLinearColors[i] );
	}

	OldTargetColorChannels.Empty();
	for (int32 i = 0; i < TargetColorChannels.Num(); ++i)
	{
		// Remap the color channel as a linear color for ease
		const FColorChannels& Channel = TargetColorChannels[i];
		const FLinearColor Color( Channel.Red ? *Channel.Red : 0.f, Channel.Green ? *Channel.Green : 0.f, Channel.Blue ? *Channel.Blue : 0.f, Channel.Alpha ? *Channel.Alpha : 0.f );
		OldTargetColorChannels.Add( Color );
	}
}


void SColorPicker::RestoreColors()
{
	check(TargetFColors.Num() == OldTargetFColors.Num());

	for (int32 i = 0; i < TargetFColors.Num(); ++i)
	{
		*TargetFColors[i] = OldTargetFColors[i];
	}

	check(TargetLinearColors.Num() == OldTargetLinearColors.Num());

	for (int32 i = 0; i < TargetLinearColors.Num(); ++i)
	{
		*TargetLinearColors[i] = OldTargetLinearColors[i];
	}

	check(TargetColorChannels.Num() == OldTargetColorChannels.Num());

	for (int32 i = 0; i < TargetColorChannels.Num(); ++i)
	{
		// Copy back out of the linear to the color channel
		FColorChannels& Channel = TargetColorChannels[i];
		const FLinearColor& OldChannel = OldTargetColorChannels[i];
		if (Channel.Red)
		{
			*Channel.Red = OldChannel.R;
		}
		if (Channel.Green)
		{
			*Channel.Green = OldChannel.G;
		}
		if (Channel.Blue)
		{
			*Channel.Blue = OldChannel.B;
		}
		if (Channel.Alpha)
		{
			*Channel.Alpha = OldChannel.A;
		}
	}
}


void SColorPicker::SetColors(const FLinearColor& InColor)
{
	for (int32 i = 0; i < TargetFColors.Num(); ++i)
	{
		*TargetFColors[i] = InColor.ToFColor(false);
	}

	for (int32 i = 0; i < TargetLinearColors.Num(); ++i)
	{
		*TargetLinearColors[i] = InColor;
	}

	for (int32 i = 0; i < TargetColorChannels.Num(); ++i)
	{
		// Only set those channels who have a valid ptr
		FColorChannels& Channel = TargetColorChannels[i];
		if (Channel.Red)
		{
			*Channel.Red = InColor.R;
		}
		if (Channel.Green)
		{
			*Channel.Green = InColor.G;
		}
		if (Channel.Blue)
		{
			*Channel.Blue = InColor.B;
		}
		if (Channel.Alpha)
		{
			*Channel.Alpha = InColor.A;
		}
	}
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SColorPicker::GenerateDefaultColorPickerContent( bool bAdvancedSectionExpanded )
{	
	// The height of the gradient bars beneath the sliders
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::Get().GetFontStyle("ColorPicker.Font");

	if (!ColorThemesViewer.IsValid())
	{
		ColorThemesViewer = SNew(SColorThemesViewer);
	}

	ColorThemesViewer->OnCurrentThemeChanged().AddSP(this, &SColorPicker::HandleThemesViewerThemeChanged);
	ColorThemesViewer->SetUseAlpha(bUseAlpha);
	ColorThemesViewer->MenuToStandardNoReturn();

	// The standard button to open the color themes can temporarily become a trash for colors
	ColorThemeComboButton = SNew(SComboButton)
		.ContentPadding(3.0f)
		.MenuPlacement(MenuPlacement_ComboBox)
		.ToolTipText(LOCTEXT("OpenThemeManagerToolTip", "Open Color Theme Manager").ToString());

	ColorThemeComboButton->SetMenuContent(ColorThemesViewer.ToSharedRef());

	SmallTrash = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SColorTrash)
				.UsesSmallIcon(true)
		];
	
	this->ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SGridPanel)
					.FillColumn(0, 1.0f)

				+ SGridPanel::Slot(0, 0)
					.Padding(0.0f, 1.0f, 20.0f, 1.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(0.0f, 1.0f)
							[
								SNew(SOverlay)

								+ SOverlay::Slot()
									[
										// color theme bar
										SAssignNew(CurrentThemeBar, SThemeColorBlocksBar)
											.ColorTheme(this, &SColorPicker::HandleThemeBarColorTheme)
											.EmptyText(LOCTEXT("EmptyBarHint", "Drag & drop colors here to save").ToString())
											.HideTrashCallback(this, &SColorPicker::HideSmallTrash)
											.ShowTrashCallback(this, &SColorPicker::ShowSmallTrash)
											.ToolTipText(LOCTEXT("CurrentThemeBarToolTip", "Current Color Theme").ToString())
											.UseAlpha(SharedThis(this), &SColorPicker::HandleThemeBarUseAlpha)
											.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
											.OnSelectColor(this, &SColorPicker::HandleThemeBarColorSelected)
									]

								// hack: need to fix SThemeColorBlocksBar::EmptyText to render properly
								+ SOverlay::Slot()
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(LOCTEXT("EmptyBarHint", "Drag & drop colors here to save").ToString())
											.Visibility(this, &SColorPicker::HandleThemeBarHintVisibility)
									]
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								// color theme selector
								SAssignNew(ColorThemeButtonOrSmallTrash, SBorder)
									.BorderImage(FStyleDefaults::GetNoBrush())
									.Padding(0.0f)							
							]
					]

				+ SGridPanel::Slot(1, 0)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						// sRGB check box
						SNew(SCheckBox)
							.ToolTipText(LOCTEXT("SRGBCheckboxToolTip", "Toggle gamma corrected sRGB previewing").ToString())
							.IsChecked(this, &SColorPicker::HandleSRGBCheckBoxIsChecked)
							.OnCheckStateChanged(this, &SColorPicker::HandleSRGBCheckBoxCheckStateChanged)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("SRGBCheckboxLabel", "sRGB").ToString())
							]
					]

				+ SGridPanel::Slot(0, 1)
					.Padding(0.0f, 8.0f, 20.0f, 0.0f)
					[
						SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
							.Padding(0.0f)
							.OnMouseButtonDown(this, &SColorPicker::HandleColorAreaMouseDown)
							[
								SNew(SOverlay)

								// color wheel
								+ SOverlay::Slot()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Center)
											[
												SNew(SColorWheel)
													.SelectedColor(this, &SColorPicker::GetCurrentColor)
													.Visibility(this, &SColorPicker::HandleColorPickerModeVisibility, EColorPickerModes::Wheel)
													.OnValueChanged(this, &SColorPicker::HandleColorSpectrumValueChanged)
													.OnMouseCaptureBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
													.OnMouseCaptureEnd(this, &SColorPicker::HandleInteractiveChangeEnd)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											.Padding(4.0f, 0.0f)
											[
												// saturation slider
												MakeColorSlider(EColorPickerChannels::Saturation)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												// value slider
												MakeColorSlider(EColorPickerChannels::Value)
											]
									]

								// color spectrum
								+ SOverlay::Slot()
									[
										SNew(SBox)
											.HeightOverride(200.0f)
											.WidthOverride(292.0f)
											[
												SNew(SColorSpectrum)
													.SelectedColor(this, &SColorPicker::GetCurrentColor)
													.Visibility(this, &SColorPicker::HandleColorPickerModeVisibility, EColorPickerModes::Spectrum)
													.OnValueChanged(this, &SColorPicker::HandleColorSpectrumValueChanged)
													.OnMouseCaptureBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
													.OnMouseCaptureEnd(this, &SColorPicker::HandleInteractiveChangeEnd)
											]
									]
							]
					]

				+ SGridPanel::Slot(1, 1)
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
									.HeightOverride(100.0f)
									.WidthOverride(70.0f)
									[
										// color preview
										MakeColorPreviewBox()
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 16.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Top)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.HAlign(HAlign_Left)
									[
										// mode selector
										SNew(SButton)
											.OnClicked(this, &SColorPicker::HandleColorPickerModeButtonClicked)
											.Content()
											[
												SNew(SImage)
													.Image(FCoreStyle::Get().GetBrush("ColorPicker.Mode"))
													.ToolTipText(LOCTEXT("ColorPickerModeEToolTip", "Toggle between color wheel and color spectrum.").ToString())
											]									
									]

								+ SHorizontalBox::Slot()
									.HAlign(HAlign_Right)
									[
										// eye dropper
										SNew(SEyeDropperButton)
											.OnValueChanged(this, &SColorPicker::HandleRGBColorChanged)
											.OnBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
											.OnComplete(this, &SColorPicker::HandleEyeDropperButtonComplete)
											.DisplayGamma(DisplayGamma)								
									]
							]
					]
			]

		// advanced settings
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced"))
					.BorderBackgroundColor(FLinearColor::Transparent)
					.InitiallyCollapsed(!bAdvancedSectionExpanded)
					.OnAreaExpansionChanged(this, &SColorPicker::HandleAdvancedAreaExpansionChanged)
					.Padding(FMargin(0.0f, 1.0f, 0.0f, 8.0f))
					.BodyContent()
					[
						SNew(SHorizontalBox)

						// RGBA inputs
						+ SHorizontalBox::Slot()
							.Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SVerticalBox)

								// Red
								+ SVerticalBox::Slot()
									[
										MakeColorSpinBox(EColorPickerChannels::Red)
									]

								// Green
								+ SVerticalBox::Slot()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										MakeColorSpinBox(EColorPickerChannels::Green)
									]

								// Blue
								+ SVerticalBox::Slot()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										MakeColorSpinBox(EColorPickerChannels::Blue)
									]

								// Alpha
								+ SVerticalBox::Slot()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										MakeColorSpinBox(EColorPickerChannels::Alpha)
									]
							]

						// HSV & Hex inputs
						+ SHorizontalBox::Slot()
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SVerticalBox)
							
								// Hue
								+ SVerticalBox::Slot()
									[
										MakeColorSpinBox(EColorPickerChannels::Hue)
									]

								// Saturation
								+ SVerticalBox::Slot()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										MakeColorSpinBox(EColorPickerChannels::Saturation)
									]

								// Value
								+ SVerticalBox::Slot()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										MakeColorSpinBox(EColorPickerChannels::Value)
									]

								// Hex
								+ SVerticalBox::Slot()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Top)
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)
											.ToolTipText(LOCTEXT("HexSliderToolTip", "Hexadecimal Value").ToString())

										+ SHorizontalBox::Slot()
											.AutoWidth()
											.Padding(0.0f, 0.0f, 4.0f, 0.0f)
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("HexInputLabel", "Hex"))
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SEditableTextBox)
													.MinDesiredWidth(72.0f)
													.Text(this, &SColorPicker::HandleHexBoxText)
													.OnTextCommitted(this, &SColorPicker::HandleHexInputTextCommitted)
											]
									]
							]
					]
			]

		// dialog buttons
		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SUniformGridPanel)
					.MinDesiredSlotHeight(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotHeight"))
					.MinDesiredSlotWidth(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.SlotPadding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					.Visibility(ParentWindowPtr.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)

				+ SUniformGridPanel::Slot(0, 0)
					[
						// ok button
						SNew(SButton)
							.ContentPadding( FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding") )
							.HAlign(HAlign_Center)
							.Text(LOCTEXT("OKButton", "OK").ToString())
							.OnClicked(this, &SColorPicker::HandleOkButtonClicked)
					]

				+ SUniformGridPanel::Slot(1, 0)
					[
						// cancel button
						SNew(SButton)
							.ContentPadding( FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding") )
							.HAlign(HAlign_Center)
							.Text(LOCTEXT("CancelButton", "Cancel").ToString())
							.OnClicked(this, &SColorPicker::HandleCancelButtonClicked)
					]
			]
	];
	
	HideSmallTrash();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SColorPicker::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// animate the wheel and all the sliders
	static const float AnimationTime = 0.2f;
	if (CurrentTime < AnimationTime)
	{
		CurrentColorHSV = FMath::Lerp(ColorBegin, ColorEnd, CurrentTime / AnimationTime);
		if (CurrentColorHSV.R < 0.f)
		{
			CurrentColorHSV.R += 360.f;
		}
		else if (CurrentColorHSV.R > 360.f)
		{
			CurrentColorHSV.R -= 360.f;
		}

		CurrentTime += InDeltaTime;
		if (CurrentTime >= AnimationTime)
		{
			CurrentColorHSV = ColorEnd;
		}

		CurrentColorRGB = CurrentColorHSV.HSVToLinearRGB();
	}
}


void SColorPicker::GenerateInlineColorPickerContent()
{
	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.AutoWidth()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SColorWheel)
							.SelectedColor(this, &SColorPicker::GetCurrentColor)
							.OnValueChanged(this, &SColorPicker::HandleHSVColorChanged)
							.OnMouseCaptureBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
							.OnMouseCaptureEnd(this, &SColorPicker::HandleInteractiveChangeEnd)
					]

				+ SVerticalBox::Slot()
					.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
					.AutoHeight()
					[
						SNew(SValueSlider)
							.SelectedColor(this, &SColorPicker::GetCurrentColor)
							.OnValueChanged(this, &SColorPicker::HandleHSVColorChanged)
							.OnMouseCaptureBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
							.OnMouseCaptureEnd(this, &SColorPicker::HandleInteractiveChangeEnd)
					]
			]
	];
}





void SColorPicker::DiscardColor()
{
	if (OnColorPickerCancelled.IsBound())
	{
		// let the user decide what to do about cancel
		OnColorPickerCancelled.Execute( OldColor );
	}
	else
	{	
		SetNewTargetColorHSV(OldColor, true);
		RestoreColors();
	}
}


bool SColorPicker::SetNewTargetColorHSV( const FLinearColor& NewValue, bool bForceUpdate /*= false*/ )
{
	CurrentColorHSV = NewValue;
	CurrentColorRGB = NewValue.HSVToLinearRGB();

	return ApplyNewTargetColor(bForceUpdate);
}


bool SColorPicker::SetNewTargetColorRGB( const FLinearColor& NewValue, bool bForceUpdate /*= false*/ )
{
	CurrentColorRGB = NewValue;
	CurrentColorHSV = NewValue.LinearRGBToHSV();

	return ApplyNewTargetColor(bForceUpdate);
}


bool SColorPicker::ApplyNewTargetColor( bool bForceUpdate /*= false*/ )
{
	bool bUpdated = false;

	if ((bForceUpdate || (!bOnlyRefreshOnMouseUp && !bPerfIsTooSlowToUpdate)) && (!bOnlyRefreshOnOk || bColorPickerIsInlineVersion))
	{
		double StartUpdateTime = FPlatformTime::Seconds();
		UpdateColorPickMouseUp();
		double EndUpdateTime = FPlatformTime::Seconds();

		if (EndUpdateTime - StartUpdateTime > MAX_ALLOWED_UPDATE_TIME)
		{
			bPerfIsTooSlowToUpdate = true;
		}

		bUpdated = true;
	}

	return bUpdated;
}


void SColorPicker::UpdateColorPickMouseUp()
{
	if (!bOnlyRefreshOnOk || bColorPickerIsInlineVersion)
	{
		UpdateColorPick();
	}
}


void SColorPicker::UpdateColorPick()
{
	bPerfIsTooSlowToUpdate = false;
	FLinearColor OutColor = CurrentColorRGB;

	PreColorCommitted.ExecuteIfBound(OutColor);

	SetColors(OutColor);
	OnColorCommitted.ExecuteIfBound(OutColor);
	
	// This callback is only necessary for wx backwards compatibility
	FCoreDelegates::ColorPickerChanged.Broadcast();
}


void SColorPicker::BeginAnimation( FLinearColor Start, FLinearColor End )
{
	ColorEnd = End;
	ColorBegin = Start;
	CurrentTime = 0.f;
	
	// wraparound with hue
	float HueDif = FMath::Abs(ColorBegin.R - ColorEnd.R);
	if (FMath::Abs(ColorBegin.R + 360.f - ColorEnd.R) < HueDif)
	{
		ColorBegin.R += 360.f;
	}
	else if (FMath::Abs(ColorBegin.R - 360.f - ColorEnd.R) < HueDif)
	{
		ColorBegin.R -= 360.f;
	}
}


void SColorPicker::HideSmallTrash()
{
	if (ColorThemeComboButton.IsValid())
	{
		ColorThemeButtonOrSmallTrash->SetContent(ColorThemeComboButton.ToSharedRef());
	}
	else
	{
		ColorThemeButtonOrSmallTrash->ClearContent();
	}
}


void SColorPicker::ShowSmallTrash()
{
	if ( SmallTrash.IsValid() )
	{
		ColorThemeButtonOrSmallTrash->SetContent(SmallTrash.ToSharedRef());
	}
	else
	{
		ColorThemeButtonOrSmallTrash->ClearContent();
	}
}


/* SColorPicker implementation
 *****************************************************************************/

void SColorPicker::CycleMode( )
{
	if (CurrentMode == EColorPickerModes::Spectrum)
	{
		CurrentMode = EColorPickerModes::Wheel;
	}
	else
	{
		CurrentMode = EColorPickerModes::Spectrum;
	}
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SColorPicker::MakeColorSlider( EColorPickerChannels::Type Channel ) const
{
	FText SliderTooltip;

	switch (Channel)
	{
	case EColorPickerChannels::Red:			SliderTooltip = LOCTEXT("RedSliderToolTip", "Red"); break;
	case EColorPickerChannels::Green:		SliderTooltip = LOCTEXT("GreenSliderToolTip", "Green"); break;
	case EColorPickerChannels::Blue:		SliderTooltip = LOCTEXT("BlueSliderToolTip", "Blue"); break;
	case EColorPickerChannels::Alpha:		SliderTooltip = LOCTEXT("AlphaSliderToolTip", "Alpha"); break;
	case EColorPickerChannels::Hue:			SliderTooltip = LOCTEXT("HueSliderToolTip", "Hue"); break;
	case EColorPickerChannels::Saturation:	SliderTooltip = LOCTEXT("SaturationSliderToolTip", "Saturation"); break;
	case EColorPickerChannels::Value:		SliderTooltip = LOCTEXT("ValueSliderToolTip", "Value"); break;
	default:
		return SNullWidget::NullWidget;
	}

	return SNew(SOverlay)
		.ToolTipText(SliderTooltip.ToString())

	+ SOverlay::Slot()
		.Padding(FMargin(4.0f, 0.0f))
		[
			SNew(SSimpleGradient)
				.EndColor(this, &SColorPicker::HandleColorSliderEndColor, Channel)
				.StartColor(this, &SColorPicker::HandleColorSliderStartColor, Channel)
				.Orientation(Orient_Horizontal)
				.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
		]

	+ SOverlay::Slot()
		[
			SNew(SSlider)
				.IndentHandle(false)
				.Orientation(Orient_Vertical)
				.SliderBarColor(FLinearColor::Transparent)
				.Style(&FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("ColorPicker.Slider"))
				.Value(this, &SColorPicker::HandleColorSpinBoxValue, Channel)
				.OnMouseCaptureBegin(this, &SColorPicker::HandleInteractiveChangeBegin)
				.OnMouseCaptureEnd(this, &SColorPicker::HandleInteractiveChangeEnd)
				.OnValueChanged(this, &SColorPicker::HandleColorSpinBoxValueChanged, Channel)
		];
}


TSharedRef<SWidget> SColorPicker::MakeColorSpinBox( EColorPickerChannels::Type Channel ) const
{
	if ((Channel == EColorPickerChannels::Alpha) && !bUseAlpha.Get())
	{
		return SNullWidget::NullWidget;
	}

	const int32 GradientHeight = 6;
	const float HDRMaxValue = (TargetFColors.Num()) ? 1.f : FLT_MAX;
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::Get().GetFontStyle("ColorPicker.Font");

	// create gradient widget
	TSharedPtr<SWidget> GradientWidget;
	
	if (Channel == EColorPickerChannels::Hue)
	{
		TArray<FLinearColor> HueGradientColors;

		for (int32 i = 0; i < 7; ++i)
		{
			HueGradientColors.Add( FLinearColor((i % 6) * 60.f, 1.f, 1.f).HSVToLinearRGB() );
		}

		GradientWidget = SNew(SComplexGradient)
			.GradientColors(HueGradientColors);
	}
	else
	{
		GradientWidget = SNew(SSimpleGradient)
			.StartColor(this, &SColorPicker::GetGradientStartColor, Channel)
			.EndColor(this, &SColorPicker::GetGradientEndColor, Channel)
			.HasAlphaBackground(Channel == EColorPickerChannels::Alpha)
			.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB);
	}
	
	// create spin box
	float MaxValue;
	FText SliderLabel;
	FText SliderTooltip;

	switch (Channel)
	{
	case EColorPickerChannels::Red:
		MaxValue = HDRMaxValue;
		SliderLabel = LOCTEXT("RedSliderLabel", "R");
		SliderTooltip = LOCTEXT("RedSliderToolTip", "Red");
		break;
		
	case EColorPickerChannels::Green:
		MaxValue = HDRMaxValue;
		SliderLabel = LOCTEXT("GreenSliderLabel", "G");
		SliderTooltip = LOCTEXT("GreenSliderToolTip", "Green");
		break;
		
	case EColorPickerChannels::Blue:
		MaxValue = HDRMaxValue;
		SliderLabel = LOCTEXT("BlueSliderLabel", "B");
		SliderTooltip = LOCTEXT("BlueSliderToolTip", "Blue");
		break;
		
	case EColorPickerChannels::Alpha:
		MaxValue = HDRMaxValue;
		SliderLabel = LOCTEXT("AlphaSliderLabel", "A");
		SliderTooltip = LOCTEXT("AlphaSliderToolTip", "Alpha");
		break;
		
	case EColorPickerChannels::Hue:
		MaxValue = 359.0f;
		SliderLabel = LOCTEXT("HueSliderLabel", "H");
		SliderTooltip = LOCTEXT("HueSliderToolTip", "Hue");
		break;
		
	case EColorPickerChannels::Saturation:
		MaxValue = 1.0f;
		SliderLabel = LOCTEXT("SaturationSliderLabel", "S");
		SliderTooltip = LOCTEXT("SaturationSliderToolTip", "Saturation");
		break;
		
	case EColorPickerChannels::Value:
		MaxValue = HDRMaxValue;
		SliderLabel = LOCTEXT("ValueSliderLabel", "V");
		SliderTooltip = LOCTEXT("ValueSliderToolTip", "Value");
		break;

	default:
		return SNullWidget::NullWidget;
	}
	
	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(SliderLabel.ToString())
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
				.ToolTipText(SliderTooltip.ToString())

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSpinBox<float>)
						.MinValue(0.0f)
						.MaxValue(MaxValue)
						.MinSliderValue(0.0f)
						.MaxSliderValue(Channel == EColorPickerChannels::Hue ? 359.0f : 1.0f)
						.Delta(Channel == EColorPickerChannels::Hue ? 1.0f : 0.001f)
						.Font(SmallLayoutFont)
						.Value(this, &SColorPicker::HandleColorSpinBoxValue, Channel)
						.OnBeginSliderMovement(this, &SColorPicker::HandleInteractiveChangeBegin)
						.OnEndSliderMovement(this, &SColorPicker::HandleInteractiveChangeEnd)
						.OnValueChanged(this, &SColorPicker::HandleColorSpinBoxValueChanged, Channel)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
						.HeightOverride(GradientHeight)
						[
							GradientWidget.ToSharedRef()
						]
				]
		];
}


TSharedRef<SWidget> SColorPicker::MakeColorPreviewBox( ) const
{
	return SNew(SOverlay)

	+ SOverlay::Slot()
		[
			// preview blocks
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("OldColorLabel", "Old"))
				]

			+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						[
							// old color
							SNew(SColorBlock) 
								.ColorIsHSV(true) 
								.IgnoreAlpha(true)
								.ToolTipText(LOCTEXT("OldColorToolTip", "Old color without alpha (drag to theme bar to save)"))
								.Color(OldColor) 
								.OnMouseButtonDown(this, &SColorPicker::HandleOldColorBlockMouseButtonDown, false)
								.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
								.Cursor(EMouseCursor::GrabHand)
						]

					+ SHorizontalBox::Slot()
						[
							// old color (alpha)
							SNew(SColorBlock) 
								.ColorIsHSV(true) 
								.ShowBackgroundForAlpha(true)
								.ToolTipText(LOCTEXT("OldColorAlphaToolTip", "Old color with alpha (drag to theme bar to save)"))
								.Color(OldColor) 
								.Visibility(SharedThis(this), &SColorPicker::HandleAlphaColorBlockVisibility)
								.OnMouseButtonDown(this, &SColorPicker::HandleOldColorBlockMouseButtonDown, true)
								.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
								.Cursor(EMouseCursor::GrabHand)
						]
				]

			+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						[
							// new color
							SNew(SColorBlock) 
								.ColorIsHSV(true) 
								.IgnoreAlpha(true)
								.ToolTipText(LOCTEXT("NewColorToolTip", "New color without alpha (drag to theme bar to save)"))
								.Color(this, &SColorPicker::GetCurrentColor)
								.OnMouseButtonDown(this, &SColorPicker::HandleNewColorBlockMouseButtonDown, false)
								.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
								.Cursor(EMouseCursor::GrabHand)
						]

					+ SHorizontalBox::Slot()
						[
							// new color (alpha)
							SNew(SColorBlock) 
								.ColorIsHSV(true) 
								.ShowBackgroundForAlpha(true)
								.ToolTipText(LOCTEXT("NewColorAlphaToolTip", "New color with alpha (drag to theme bar to save)"))
								.Color(this, &SColorPicker::GetCurrentColor)
								.Visibility(SharedThis(this), &SColorPicker::HandleAlphaColorBlockVisibility)
								.OnMouseButtonDown(this, &SColorPicker::HandleNewColorBlockMouseButtonDown, true)
								.UseSRGB(SharedThis(this), &SColorPicker::HandleColorPickerUseSRGB)
								.Cursor(EMouseCursor::GrabHand)
						]
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("NewColorLabel", "New"))
				]
		]

	+ SOverlay::Slot()
		.VAlign(VAlign_Center)
		[
			// block separators
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
						.HeightOverride(2.0f)
						.WidthOverride(4.0f)
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("ColorPicker.Separator"))
								.Padding(0.0f)
						]
				]

			+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.HAlign(HAlign_Right)
				[
					SNew(SBox)
						.HeightOverride(2.0f)
						.WidthOverride(4.0f)
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("ColorPicker.Separator"))
								.Padding(0.0f)
						]
				]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SColorPicker callbacks
 *****************************************************************************/

FLinearColor SColorPicker::GetGradientEndColor( EColorPickerChannels::Type Channel ) const
{
	switch (Channel)
	{
	case EColorPickerChannels::Red:			return FLinearColor(1.0f, CurrentColorRGB.G, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Green:		return FLinearColor(CurrentColorRGB.R, 1.0f, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Blue:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, 1.0f, 1.0f);
	case EColorPickerChannels::Alpha:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Saturation:	return FLinearColor(CurrentColorHSV.R, 1.0f, CurrentColorHSV.B, 1.0f).HSVToLinearRGB();
	case EColorPickerChannels::Value:		return FLinearColor(CurrentColorHSV.R, CurrentColorHSV.G, 1.0f, 1.0f).HSVToLinearRGB();
	default:								return FLinearColor();
	}
}


FLinearColor SColorPicker::GetGradientStartColor( EColorPickerChannels::Type Channel ) const
{
	switch (Channel)
	{
	case EColorPickerChannels::Red:			return FLinearColor(0.0f, CurrentColorRGB.G, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Green:		return FLinearColor(CurrentColorRGB.R, 0.0f, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Blue:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, 0.0f, 1.0f);
	case EColorPickerChannels::Alpha:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, CurrentColorRGB.B, 0.0f);
	case EColorPickerChannels::Saturation:	return FLinearColor(CurrentColorHSV.R, 0.0f, CurrentColorHSV.B, 1.0f).HSVToLinearRGB();
	case EColorPickerChannels::Value:		return FLinearColor(CurrentColorHSV.R, CurrentColorHSV.G, 0.0f, 1.0f).HSVToLinearRGB();
	default:								return FLinearColor();
	}
}


void SColorPicker::HandleAdvancedAreaExpansionChanged( bool Expanded )
{
	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		GConfig->SetBool(TEXT("ColorPickerUI"), TEXT("bAdvancedSectionExpanded"), Expanded, GEditorUserSettingsIni);
	}
}


EVisibility SColorPicker::HandleAlphaColorBlockVisibility( ) const
{
	return bUseAlpha.Get() ? EVisibility::Visible : EVisibility::Collapsed;
}


FReply SColorPicker::HandleCancelButtonClicked( )
{
	bClosedViaOkOrCancel = true;

	DiscardColor();
	ParentWindowPtr.Pin()->RequestDestroyWindow();

	return FReply::Handled();
}


EVisibility SColorPicker::HandleColorPickerModeVisibility( EColorPickerModes::Type Mode ) const
{
	return (CurrentMode == Mode) ? EVisibility::Visible : EVisibility::Hidden;
}


FLinearColor SColorPicker::HandleColorSliderEndColor( EColorPickerChannels::Type Channel ) const
{
	switch (Channel)
	{
	case EColorPickerChannels::Red:			return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	case EColorPickerChannels::Green:		return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	case EColorPickerChannels::Blue:		return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	case EColorPickerChannels::Alpha:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, CurrentColorRGB.B, 0.0f);
	case EColorPickerChannels::Saturation:	return FLinearColor(CurrentColorHSV.R, 0.0f, 1.0f, 1.0f).HSVToLinearRGB();
	case EColorPickerChannels::Value:		return FLinearColor(CurrentColorHSV.R, CurrentColorHSV.G, 0.0f, 1.0f).HSVToLinearRGB();
	default:								return FLinearColor();
	}
}


FLinearColor SColorPicker::HandleColorSliderStartColor( EColorPickerChannels::Type Channel ) const
{
	switch (Channel)
	{
	case EColorPickerChannels::Red:			return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	case EColorPickerChannels::Green:		return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
	case EColorPickerChannels::Blue:		return FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
	case EColorPickerChannels::Alpha:		return FLinearColor(CurrentColorRGB.R, CurrentColorRGB.G, CurrentColorRGB.B, 1.0f);
	case EColorPickerChannels::Saturation:	return FLinearColor(CurrentColorHSV.R, 1.0f, 1.0f, 1.0f).HSVToLinearRGB();
	case EColorPickerChannels::Value:		return FLinearColor(CurrentColorHSV.R, CurrentColorHSV.G, 1.0f, 1.0f).HSVToLinearRGB();
	default:								return FLinearColor();
	}
}


void SColorPicker::HandleColorSpectrumValueChanged( FLinearColor NewValue )
{
	SetNewTargetColorHSV(NewValue);
}


float SColorPicker::HandleColorSpinBoxValue( EColorPickerChannels::Type Channel ) const
{
	switch (Channel)
	{
	case EColorPickerChannels::Red:			return CurrentColorRGB.R;
	case EColorPickerChannels::Green:		return CurrentColorRGB.G;
	case EColorPickerChannels::Blue:		return CurrentColorRGB.B;
	case EColorPickerChannels::Alpha:		return CurrentColorRGB.A;
	case EColorPickerChannels::Hue:			return CurrentColorHSV.R;
	case EColorPickerChannels::Saturation:	return CurrentColorHSV.G;
	case EColorPickerChannels::Value:		return CurrentColorHSV.B;
	default:								return 0.0f;
	}
}


void SColorPicker::HandleColorSpinBoxValueChanged( float NewValue, EColorPickerChannels::Type Channel )
{
	int32 ComponentIndex;
	bool IsHSV = false;

	switch (Channel)
	{
	case EColorPickerChannels::Red:			ComponentIndex = 0; break;
	case EColorPickerChannels::Green:		ComponentIndex = 1; break;
	case EColorPickerChannels::Blue:		ComponentIndex = 2; break;
	case EColorPickerChannels::Alpha:		ComponentIndex = 3; break;
	case EColorPickerChannels::Hue:			ComponentIndex = 0; IsHSV = true; break;
	case EColorPickerChannels::Saturation:	ComponentIndex = 1; IsHSV = true; break;
	case EColorPickerChannels::Value:		ComponentIndex = 2; IsHSV = true; break;
	default:								
		return;
	}

	FLinearColor& NewColor = IsHSV ? CurrentColorHSV : CurrentColorRGB;

	if (FMath::IsNearlyEqual(NewValue, NewColor.Component(ComponentIndex), KINDA_SMALL_NUMBER))
	{
		return;
	}

	NewColor.Component(ComponentIndex) = NewValue;

	if (IsHSV)
	{
		SetNewTargetColorHSV(NewColor, !bIsInteractive);
	}
	else
	{
		SetNewTargetColorRGB(NewColor, !bIsInteractive);
	}
}


void SColorPicker::HandleEyeDropperButtonComplete( )
{
	bIsInteractive = false;

	if (bOnlyRefreshOnMouseUp || bPerfIsTooSlowToUpdate)
	{
		UpdateColorPick();
	}

	OnInteractivePickEnd.ExecuteIfBound();
}


FText SColorPicker::HandleHexBoxText( ) const
{
	return FText::FromString(CurrentColorRGB.ToFColor(false).ToHex());
}


void SColorPicker::HandleHexInputTextCommitted( const FText& Text, ETextCommit::Type CommitType )
{
	if ((CommitType == ETextCommit::OnEnter) || (CommitType == ETextCommit::OnUserMovedFocus))
	{
		SetNewTargetColorRGB(FColor::FromHex(Text.ToString()), false);
	}	
}


void SColorPicker::HandleHSVColorChanged( FLinearColor NewValue )
{
	SetNewTargetColorHSV(NewValue);
}


void SColorPicker::HandleInteractiveChangeBegin( )
{
	OnInteractivePickBegin.ExecuteIfBound();
	bIsInteractive = true;
}


void SColorPicker::HandleInteractiveChangeEnd( )
{
	HandleInteractiveChangeEnd(0.0f);
}


void SColorPicker::HandleInteractiveChangeEnd( float NewValue )
{
	bIsInteractive = false;

	UpdateColorPickMouseUp();
	OnInteractivePickEnd.ExecuteIfBound();
}


FReply SColorPicker::HandleColorAreaMouseDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		CycleMode();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


FReply SColorPicker::HandleColorPickerModeButtonClicked( )
{
	CycleMode();

	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		GConfig->SetBool(TEXT("ColorPickerUI"), TEXT("bWheelMode"), CurrentMode == EColorPickerModes::Wheel, GEditorUserSettingsIni);
	}

	return FReply::Handled();
}


FReply SColorPicker::HandleNewColorBlockMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bCheckAlpha )
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		TSharedRef<FColorDragDrop> Operation = FColorDragDrop::New(
			CurrentColorHSV,
			SColorThemesViewer::bSRGBEnabled,
			bCheckAlpha ? bUseAlpha.Get() : false,
			FSimpleDelegate::CreateSP(this, &SColorPicker::ShowSmallTrash),
			FSimpleDelegate::CreateSP(this, &SColorPicker::HideSmallTrash)
		);

		return FReply::Handled().BeginDragDrop(Operation);
	}

	return FReply::Unhandled();
}


FReply SColorPicker::HandleOkButtonClicked( )
{
	bClosedViaOkOrCancel = true;

	if (bOnlyRefreshOnOk)
	{
		UpdateColorPick();
	}

	ParentWindowPtr.Pin()->RequestDestroyWindow();

	return FReply::Handled();
}


FReply SColorPicker::HandleOldColorBlockMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bCheckAlpha )
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		TSharedRef<FColorDragDrop> Operation = FColorDragDrop::New(
			OldColor,
			SColorThemesViewer::bSRGBEnabled,
			bCheckAlpha ? bUseAlpha.Get() : false,
			FSimpleDelegate::CreateSP(this, &SColorPicker::ShowSmallTrash),
			FSimpleDelegate::CreateSP(this, &SColorPicker::HideSmallTrash)
		);

		return FReply::Handled().BeginDragDrop(Operation);
	}

	return FReply::Unhandled();
}


bool SColorPicker::HandleColorPickerUseSRGB( ) const
{
	return SColorThemesViewer::bSRGBEnabled;
}


void SColorPicker::HandleParentWindowClosed( const TSharedRef<SWindow>& Window )
{
	check(Window == ParentWindowPtr.Pin());

	// We always have to call the close callback
	if (OnColorPickerWindowClosed.IsBound())
	{
		OnColorPickerWindowClosed.Execute(Window);
	}

	// If we weren't closed via the OK or Cancel button, we need to perform the default close action
	if (!bClosedViaOkOrCancel && bOnlyRefreshOnOk)
	{
		DiscardColor();
	}
}


void SColorPicker::HandleRGBColorChanged( FLinearColor NewValue )
{
	SetNewTargetColorRGB(NewValue);
}


void SColorPicker::HandleSRGBCheckBoxCheckStateChanged( ESlateCheckBoxState::Type InIsChecked )
{
	SColorThemesViewer::bSRGBEnabled = (InIsChecked == ESlateCheckBoxState::Checked);

	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		GConfig->SetBool(TEXT("ColorPickerUI"), TEXT("bSRGBEnabled"), SColorThemesViewer::bSRGBEnabled, GEditorUserSettingsIni);
	}
}


ESlateCheckBoxState::Type SColorPicker::HandleSRGBCheckBoxIsChecked( ) const
{
	return SColorThemesViewer::bSRGBEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}


void SColorPicker::HandleThemeBarColorSelected( FLinearColor NewValue )
{
	// Force the alpha component to 1 when we don't care about the alpha
	if (!bUseAlpha.Get())
	{
		NewValue.A = 1.0f;
	}

	BeginAnimation(CurrentColorHSV, NewValue);
	SetNewTargetColorHSV(NewValue, true);
}


TSharedPtr<FColorTheme> SColorPicker::HandleThemeBarColorTheme() const
{
	return ColorThemesViewer->GetCurrentColorTheme();
}


EVisibility SColorPicker::HandleThemeBarHintVisibility( ) const
{
	TSharedPtr<FColorTheme> SelectedTheme = ColorThemesViewer->GetCurrentColorTheme();

	if (SelectedTheme.IsValid() && SelectedTheme->GetColors().Num() == 0)
	{
		return EVisibility::HitTestInvisible;
	}

	return EVisibility::Hidden;
}


bool SColorPicker::HandleThemeBarUseAlpha( ) const
{
	return bUseAlpha.Get();
}


void SColorPicker::HandleThemesViewerThemeChanged()
{
	if (CurrentThemeBar.IsValid())
	{
		CurrentThemeBar->RemoveRefreshCallback();
		CurrentThemeBar->AddRefreshCallback();
		CurrentThemeBar->Refresh();
	}
}


/* Global functions
 *****************************************************************************/

/** A static color picker that everything should use. */
static TWeakPtr<SWindow> ColorPickerWindow;


bool OpenColorPicker(const FColorPickerArgs& Args)
{
	DestroyColorPicker();

	bool Result = false;
	
	//guard to verify that we're not in game using edit actor
	if (GIsEditor)
	{
		FLinearColor OldColor = Args.InitialColorOverride;

		if (Args.ColorArray && Args.ColorArray->Num() > 0)
		{
			OldColor = (*Args.ColorArray)[0]->ReinterpretAsLinear();
		}
		else if (Args.LinearColorArray && Args.LinearColorArray->Num() > 0)
		{
			OldColor = *(*Args.LinearColorArray)[0];
		}
		else if (Args.ColorChannelsArray && Args.ColorChannelsArray->Num() > 0)
		{
			OldColor.R = (*Args.ColorChannelsArray)[0].Red ? *(*Args.ColorChannelsArray)[0].Red : 0.0f;
			OldColor.G = (*Args.ColorChannelsArray)[0].Green ? *(*Args.ColorChannelsArray)[0].Green : 0.0f;
			OldColor.B = (*Args.ColorChannelsArray)[0].Blue ? *(*Args.ColorChannelsArray)[0].Blue : 0.0f;
			OldColor.A = (*Args.ColorChannelsArray)[0].Alpha ? *(*Args.ColorChannelsArray)[0].Alpha : 0.0f;
		}
		else
		{
			check(Args.OnColorCommitted.IsBound());
		}
		
		// Determine the position of the window so that it will spawn near the mouse, but not go off the screen.
		const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
		FSlateRect Anchor(CursorPos.X, CursorPos.Y, CursorPos.X, CursorPos.Y);

		FVector2D AdjustedSummonLocation = FSlateApplication::Get().CalculatePopupWindowPosition( Anchor, SColorPicker::DEFAULT_WINDOW_SIZE, Orient_Horizontal );

		TSharedPtr<SWindow> Window = SNew(SWindow)
			.AutoCenter(EAutoCenter::None)
			.ScreenPosition(AdjustedSummonLocation)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.SizingRule(ESizingRule::Autosized)
			.Title(LOCTEXT("WindowHeader", "Color Picker"));

		TSharedRef<SColorPicker> ColorPicker = SNew(SColorPicker)
			.TargetColorAttribute(OldColor)
			.TargetFColors(Args.ColorArray ? *Args.ColorArray : TArray<FColor*>())
			.TargetLinearColors(Args.LinearColorArray ? *Args.LinearColorArray : TArray<FLinearColor*>())
			.TargetColorChannels(Args.ColorChannelsArray ? *Args.ColorChannelsArray : TArray<FColorChannels>())
			.UseAlpha(Args.bUseAlpha)
			.OnlyRefreshOnMouseUp(Args.bOnlyRefreshOnMouseUp && !Args.bIsModal)
			.OnlyRefreshOnOk(Args.bOnlyRefreshOnOk || Args.bIsModal)
			.OnColorCommitted(Args.OnColorCommitted)
			.PreColorCommitted(Args.PreColorCommitted)
			.OnColorPickerCancelled(Args.OnColorPickerCancelled)
			.OnInteractivePickBegin(Args.OnInteractivePickBegin)
			.OnInteractivePickEnd(Args.OnInteractivePickEnd)
			.OnColorPickerWindowClosed(Args.OnColorPickerWindowClosed)
			.ParentWindow(Window)
			.DisplayGamma(Args.DisplayGamma);
		
		Window->SetContent(
			SNew(SBox)
				[
					SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(8.0f, 8.0f))
						[
							ColorPicker
						]
				]
		);

		if (Args.bIsModal)
		{
			FSlateApplication::Get().AddModalWindow(Window.ToSharedRef(), Args.ParentWidget);
		}
		else
		{
			if ( Args.ParentWidget.IsValid() )
			{
				// Find the window of the parent widget
				FWidgetPath WidgetPath;
				FSlateApplication::Get().GeneratePathToWidgetChecked( Args.ParentWidget.ToSharedRef(), WidgetPath );
				Window = FSlateApplication::Get().AddWindowAsNativeChild( Window.ToSharedRef(), WidgetPath.GetWindow() );
			}
			else
			{
				Window = FSlateApplication::Get().AddWindow(Window.ToSharedRef());
			}
			
		}

		Result = true;

		//hold on to the window created for external use...
		ColorPickerWindow = Window;
	}

	return Result;
}


/**
 * Destroys the current color picker. Necessary if the values the color picker
 * currently targets become invalid.
 */
void DestroyColorPicker()
{
	if (ColorPickerWindow.IsValid())
	{
		ColorPickerWindow.Pin()->RequestDestroyWindow();
		ColorPickerWindow.Reset();
	}
}


#undef LOCTEXT_NAMESPACE
