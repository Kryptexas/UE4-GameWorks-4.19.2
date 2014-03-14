// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorStyle.cpp: Implements the FSlateEditorStyle class.
=============================================================================*/

#include "EditorStylePrivatePCH.h"


#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define TTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( RootToCoreContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define OTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( RootToCoreContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )


/* FSlateEditorStyle static initialization
 *****************************************************************************/

TSharedPtr< FSlateEditorStyle::FStyle > FSlateEditorStyle::StyleInstance = NULL;
TWeakObjectPtr< UEditorStyleSettings > FSlateEditorStyle::Settings = NULL;


/* FSlateEditorStyle interface
 *****************************************************************************/

FSlateEditorStyle::FStyle::FStyle( const TWeakObjectPtr< UEditorStyleSettings >& InSettings )
	: FSlateStyleSet("EditorStyle")

	// Note, these sizes are in Slate Units.
	// Slate Units do NOT have to map to pixels.
	, Icon5x16(5.0f, 16.0f)
	, Icon8x4(8.0f, 4.0f)
	, Icon8x8(8.0f, 8.0f)
	, Icon10x10(10.0f, 10.0f)
	, Icon12x12(12.0f, 12.0f)
	, Icon12x16(12.0f, 16.0f)
	, Icon14x14(14.0f, 14.0f)
	, Icon16x16(16.0f, 16.0f)
	, Icon16x20(16.0f, 20.0f)
	, Icon20x20(20.0f, 20.0f)
	, Icon22x22(22.0f, 22.0f)
	, Icon24x24(24.0f, 24.0f)
	, Icon25x25(25.0f, 25.0f)
	, Icon32x32(32.0f, 32.0f)
	, Icon40x40(40.0f, 40.0f)
	, Icon48x48(48.0f, 48.0f)
	, Icon64x64(64.0f, 64.0f)
	, Icon36x24(36.0f, 24.0f)
	, Icon128x128(128.0f, 128.0f)

	// These are the colors that are updated by the user style customizations
	, DefaultForeground_LinearRef( MakeShareable( new FLinearColor( 0.72f, 0.72f, 0.72f, 1.f ) ) )
	, InvertedForeground_LinearRef( MakeShareable( new FLinearColor( 0, 0, 0 ) ) )
	, SelectorColor_LinearRef( MakeShareable( new FLinearColor( 0.701f, 0.225f, 0.003f ) ) )
	, SelectionColor_LinearRef( MakeShareable( new FLinearColor( 0.728f, 0.364f, 0.003f ) ) )
	, SelectionColor_Inactive_LinearRef( MakeShareable( new FLinearColor( 0.25f, 0.25f, 0.25f ) ) )
	, SelectionColor_Pressed_LinearRef( MakeShareable( new FLinearColor( 0.701f, 0.225f, 0.003f ) ) )

	// These are the Slate colors which reference those above; these are the colors to put into the style
	, DefaultForeground( DefaultForeground_LinearRef )
	, InvertedForeground( InvertedForeground_LinearRef )
	, SelectorColor( SelectorColor_LinearRef )
	, SelectionColor( SelectionColor_LinearRef )
	, SelectionColor_Inactive( SelectionColor_Inactive_LinearRef )
	, SelectionColor_Pressed( SelectionColor_Pressed_LinearRef )

	, Settings( InSettings )
{

}

void SetColor( const TSharedRef< FLinearColor >& Source, const FLinearColor& Value )
{
	Source->R = Value.R;
	Source->G = Value.G;
	Source->B = Value.B;
	Source->A = Value.A;
}

void FSlateEditorStyle::FStyle::SettingsChanged( UObject* ChangedObject )
{
	if ( ChangedObject == Settings.Get() )
	{
		SyncSettings();
	}
}

void FSlateEditorStyle::FStyle::SyncSettings()
{
	if ( Settings.IsValid() )
	{
		// Sync the colors used by FEditorStyle
		SetColor( SelectorColor_LinearRef, Settings->KeyboardFocusColor );
		SetColor( SelectionColor_LinearRef, Settings->SelectionColor );
		SetColor( SelectionColor_Inactive_LinearRef, Settings->InactiveSelectionColor );
		SetColor( SelectionColor_Pressed_LinearRef, Settings->PressedSelectionColor );

		// Also sync the colors used by FCoreStyle, as FEditorStyle isn't yet being used as an override everywhere
		FCoreStyle::SetSelectorColor( Settings->KeyboardFocusColor );
		FCoreStyle::SetSelectionColor( Settings->SelectionColor );
		FCoreStyle::SetInactiveSelectionColor( Settings->InactiveSelectionColor );
		FCoreStyle::SetPressedSelectionColor( Settings->PressedSelectionColor );
	}
}

void FSlateEditorStyle::FStyle::Initialize()
{
	SyncSettings();

	SetContentRoot( FPaths::EngineContentDir() / TEXT("Editor/Slate") );
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	SetupGeneralStyles();
	SetupWindowStyles();
	SetupDockingStyles();
	SetupTutorialStyles();
	SetupPropertyEditorStyles();
	SetupProfilerStyle();
	SetupGraphEditorStyles();
	SetupLevelEditorStyle();
	SetupPersonaStyle();
	SetupClassIconsAndThumbnails();
	SetupContentBrowserStyle();
	SetupLandscapeEditorStyle();
	SetupToolkitStyles();
	SetupMatineeStyle();
	SetupSourceControlStyles();
	SetupAutomationStyles();

//	LogUnusedBrushResources();
}

void FSlateEditorStyle::FStyle::SetupGeneralStyles()
{
	// Define some 'normal' styles, upon which other variations can be based
	const FSlateFontInfo NormalFont = TTF_CORE_FONT("Fonts/Roboto-Regular", 9 );

	NormalText = FTextBlockStyle()
		.SetFont( TTF_CORE_FONT("Fonts/Roboto-Regular", 10 ))
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor( FLinearColor( 0.02f, 0.3f, 0.0f ) )
			.SetHighlightShape( BOX_BRUSH( "Common/TextBlockHighlightShape", FMargin(3.f/8.f) ) );

	NormalTableRowStyle = FTableRowStyle()
		.SetEvenRowBackgroundBrush( FSlateNoResource() )
		.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
		.SetOddRowBackgroundBrush( FSlateNoResource() )
		.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
		.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
		.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
		.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
		.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
		.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
		.SetTextColor( DefaultForeground )
		.SetSelectedTextColor( InvertedForeground );

	// Normal Text
	{
		Set( "RichTextBlock.TextHighlight", FTextBlockStyle(NormalText)
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) ) );

		Set( "TextBlock.HighlightShape",  new BOX_BRUSH( "Common/TextBlockHighlightShape", FMargin(3.f/8.f) ));
		Set( "TextBlock.HighlighColor", FLinearColor( 0.02f, 0.3f, 0.0f ) );

		Set("NormalText", NormalText);
	}

	// Rendering resources that never change
	{
		Set( "SplineFilterTable", new IMAGE_BRUSH( "SplineFilterTable", FVector2D(32,1), FLinearColor::White, ESlateBrushTileType::Both, ESlateBrushImageType::Linear ) );
		Set( "None", new FSlateNoResource() );
	}

	Set( "Checkerboard", new IMAGE_BRUSH( "Checkerboard", Icon16x16, FLinearColor::White, ESlateBrushTileType::Both) );

	Set( "BlackBrush", new FSlateColorBrush(FLinearColor::Black) );
	Set( "WhiteBrush", new FSlateColorBrush(FLinearColor::White) );

	Set( "PlainBorder", new BORDER_BRUSH( "Common/PlainBorder", 2.f/8.f) );

	// Debug Colors
	Set( "MultiboxHookColor", FLinearColor(0.f, 1.f, 0.f, 1.f) );

	// Important colors
	{
		Set( "DefaultForeground", DefaultForeground );
		Set( "InvertedForeground", InvertedForeground );

		Set( "SelectorColor", SelectorColor );
		Set( "SelectionColor", SelectionColor );
		Set( "SelectionColor_Inactive", SelectionColor_Inactive );
		Set( "SelectionColor_Pressed", SelectionColor_Pressed );
	}

	// Invisible buttons, borders, etc.
	Set( "NoBrush", new FSlateNoResource() );
	Set( "NoBorder", new FSlateNoResource() );
	Set( "NoBorder.Normal", new FSlateNoResource() );
	Set( "NoBorder.Hovered", new FSlateNoResource() );
	Set( "NoBorder.Pressed", new FSlateNoResource() );

	const FButtonStyle NoBorder = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
#if PLATFORM_MAC // Disable the shifting behaviour for borderless buttons on Mac OS X - which include the Window close, maximise & minimise.
		.SetNormalPadding(FMargin(0,0,0,0))
		.SetPressedPadding(FMargin(0,0,0,0));
#else
		.SetNormalPadding(FMargin(0,0,0,1))
		.SetPressedPadding(FMargin(0,1,0,0));
#endif
	Set( "NoBorder", NoBorder );

	// Buttons that only provide a hover hint.
	HoverHintOnly = FButtonStyle()
			.SetNormal( FSlateNoResource() )
			.SetHovered( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.15f) ) )
			.SetPressed( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.25f) ) )
			.SetNormalPadding( FMargin(0,0,0,1) )
			.SetPressedPadding( FMargin(0,1,0,0) );
	Set( "HoverHintOnly", HoverHintOnly );

	// Common glyphs
	{
		Set( "Symbols.SearchGlass", new IMAGE_BRUSH( "Common/SearchGlass", Icon16x16 ) );
		Set( "Symbols.X", new IMAGE_BRUSH( "Common/X", Icon16x16 ) );
		Set( "Symbols.VerticalPipe", new BOX_BRUSH( "Common/VerticalPipe", FMargin(0) ) );
		Set( "Symbols.UpArrow", new IMAGE_BRUSH( "Common/UpArrow", Icon8x8 ) );
		Set( "Symbols.DoubleUpArrow", new IMAGE_BRUSH( "Common/UpArrow2", Icon8x8 ) );
		Set( "Symbols.DownArrow", new IMAGE_BRUSH( "Common/DownArrow", Icon8x8 ) );
		Set( "Symbols.DoubleDownArrow", new IMAGE_BRUSH( "Common/DownArrow2", Icon8x8 ) );
		Set( "Symbols.Check", new IMAGE_BRUSH( "Common/Check", Icon16x16 ) );
	}

	// Common icons
	{
		Set( "Icons.Cross", new IMAGE_BRUSH( "Icons/Cross_12x", Icon12x12 ) );
		Set( "Icons.Denied", new IMAGE_BRUSH( "Icons/Denied_16x", Icon16x16 ) );
		Set( "Icons.Error", new IMAGE_BRUSH( "Icons/icon_error_16x", Icon16x16) );
		Set( "Icons.Help", new IMAGE_BRUSH( "Icons/icon_help_16x", Icon16x16) );
		Set( "Icons.Info", new IMAGE_BRUSH( "Icons/icon_info_16x", Icon16x16) );
		Set( "Icons.Warning", new IMAGE_BRUSH( "Icons/icon_warning_16x", Icon16x16) );
		Set( "Icons.Download", new IMAGE_BRUSH( "Icons/icon_Downloads_16x", Icon16x16) );
	}

	Set( "WarningStripe", new IMAGE_BRUSH( "Common/WarningStripe", FVector2D(20,6), FLinearColor::White, ESlateBrushTileType::Horizontal ) );

	// Normal button
	Button = FButtonStyle()
		.SetNormal( BOX_BRUSH( "Common/Button", FVector2D(32,32), 8.0f/32.0f ) )
		.SetHovered( BOX_BRUSH( "Common/Button_Hovered", FVector2D(32,32), 8.0f/32.0f ) )
		.SetPressed( BOX_BRUSH( "Common/Button_Pressed", FVector2D(32,32), 8.0f/32.0f ) )
		.SetDisabled( BOX_BRUSH( "Common/Button_Disabled", 8.0f/32.0f ) )
		.SetNormalPadding( FMargin( 2,2,2,2 ) )
		.SetPressedPadding( FMargin( 2,3,2,1 ) );
	Set( "Button", Button );
	Set( "Button.Disabled", new BOX_BRUSH( "Common/Button_Disabled", 8.0f/32.0f ) );
	

	// Toggle button
	{
		Set( "ToggleButton", FButtonStyle(Button)
			.SetNormal(FSlateNoResource())
			.SetHovered(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ))
			.SetPressed(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ))
		);

		/* Create a checkbox style for "ToggleButton" ... */
		const FCheckBoxStyle ToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
			.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );
		/* ... and set new style */
		Set( "ToggleButtonCheckbox", ToggleButtonStyle );

		/* Create a checkbox style for "ToggleButton" but with the images used by a normal checkbox (see "Checkbox" below) ... */
		const FCheckBoxStyle CheckboxLookingToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage( IMAGE_BRUSH( "Common/Checkbox", Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/Checkbox", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Checked", Icon16x16 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16 ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );
		/* ... and set new style */
		Set( "CheckboxLookToggleButtonCheckbox", CheckboxLookingToggleButtonStyle );


		Set( "ToggleButton.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "ToggleButtonCheckbox.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
	}

	// Combo Button, Combo Box
	{
		// Legacy style; still being used by some editor widgets
		Set( "ComboButton.Arrow", new IMAGE_BRUSH("Common/ComboArrow", Icon8x8 ) );

		FComboButtonStyle ComboButton = FComboButtonStyle()
			.SetButtonStyle(GetWidgetStyle<FButtonStyle>("Button"))
			.SetContentScale(FVector2D(1,1))
			.SetContentPadding(FMargin(5))
			.SetHasDownArrow(true)
			.SetDownArrowImage(IMAGE_BRUSH("Common/ComboArrow", Icon8x8))
			.SetHAlign(HAlign_Fill)
			.SetVAlign(VAlign_Fill)
			.SetForegroundColor(InvertedForeground)
			.SetButtonColorAndOpacity(FLinearColor::White)
			.SetMenuBorderBrush(BOX_BRUSH("Old/Menu_Background", FMargin(8.0f/64.0f)))
			.SetMenuBorderPadding(FMargin(0.0f))
			.SetMenuPlacement(MenuPlacement_ComboBox);
		Set( "ComboButton", ComboButton );

		FComboButtonStyle ToolbarComboButton = FComboButtonStyle()
			.SetButtonStyle( GetWidgetStyle<FButtonStyle>( "ToggleButton" ) )
			.SetContentScale( FVector2D( 1, 1 ) )
			.SetContentPadding( FMargin( 5 ) )
			.SetHasDownArrow( true )
			.SetDownArrowImage( IMAGE_BRUSH( "Common/ShadowComboArrow", Icon8x8 ) )
			.SetHAlign( HAlign_Fill )
			.SetVAlign( VAlign_Fill )
			.SetForegroundColor( FLinearColor::White )
			.SetButtonColorAndOpacity( FLinearColor::White )
			.SetMenuBorderBrush( BOX_BRUSH( "Old/Menu_Background", FMargin( 8.0f / 64.0f ) ) )
			.SetMenuBorderPadding( FMargin( 0.0f ) )
			.SetMenuPlacement( MenuPlacement_ComboBox );
		Set( "ToolbarComboButton", ToolbarComboButton );

		ComboButton.SetContentPadding(FMargin(4.0, 2.0));
		ComboButton.SetMenuBorderPadding(FMargin(1.0));
		ComboButton.SetForegroundColor(InvertedForeground);

		FComboBoxStyle ComboBox = FComboBoxStyle()
			.SetComboButtonStyle(ComboButton);
		Set( "ComboBox", ComboBox );
	}

	// CheckBox
	{
		/* Set images for various SCheckBox states ... */
		const FCheckBoxStyle BasicCheckBoxStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
			.SetUncheckedImage( IMAGE_BRUSH( "Common/Checkbox", Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/Checkbox", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Checked", Icon16x16 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16 ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );
 
		/* ... and add the new style */
		Set( "Checkbox", BasicCheckBoxStyle );

		/* Set images for various transparent SCheckBox states ... */
		const FCheckBoxStyle BasicTransparentCheckBoxStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedHoveredImage( FSlateNoResource() )
			.SetUncheckedPressedImage( FSlateNoResource() )
			.SetCheckedImage( FSlateNoResource() )
			.SetCheckedHoveredImage( FSlateNoResource() )
			.SetCheckedPressedImage( FSlateNoResource() )
			.SetUndeterminedImage( FSlateNoResource() )
			.SetUndeterminedHoveredImage( FSlateNoResource() )
			.SetUndeterminedPressedImage( FSlateNoResource() );
		/* ... and add the new style */
		Set( "TransparentCheckBox", BasicTransparentCheckBoxStyle );
	}

	// Help button
	Set( "HelpButton", FButtonStyle(Button)
		.SetNormal( FSlateNoResource() )
		.SetHovered(FSlateNoResource() )
		.SetPressed(FSlateNoResource() )
	);

	Set( "HelpIcon", new IMAGE_BRUSH( "Common/icon_Help_Default_16x", Icon16x16 ) );
	Set( "HelpIcon.Hovered", new IMAGE_BRUSH( "Common/icon_Help_Hover_16x", Icon16x16 ) );
	Set( "HelpIcon.Pressed", new IMAGE_BRUSH( "Common/icon_Help_Pressed_16x", Icon16x16 ) );

	{
		/* A radio button is actually just a SCheckBox box with different images */
		/* Set images for various radio button (SCheckBox) states ... */
		const FCheckBoxStyle BasicRadioButtonStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) );

		/* ... and add the new style */
		Set( "RadioButton", BasicRadioButtonStyle );
	}

	// Error Reporting
	{
		Set( "ErrorReporting.Box",  new BOX_BRUSH( "Common/TextBlockHighlightShape", FMargin(3.f/8.f) ));
		Set( "ErrorReporting.EmptyBox",  new BOX_BRUSH( "Common/TextBlockHighlightShape_Empty", FMargin(3.f/8.f) ));
		Set( "ErrorReporting.BackgroundColor", FLinearColor(0.35f,0,0) );
		Set( "ErrorReporting.WarningBackgroundColor", FLinearColor(0.828f, 0.364f, 0.003f) );
		Set( "InfoReporting.BackgroundColor", FLinearColor(0.828f, 0.364f, 0.003f) );
		Set( "ErrorReporting.ForegroundColor", FLinearColor::White );
		
	}

	// EditableTextBox
	{
		Set( "EditableTextBox.Background.Normal", new BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.Hovered", new BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.Focused", new BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.ReadOnly", new BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.BorderPadding", FMargin(4.0f, 2.0f) );
	}

	// EditableTextBox Special
	{
		FSlateBrush* SpecialEditableTextImageNormal = new BOX_BRUSH( "Common/TextBox_Special", FMargin(8.0f/32.0f) );
		Set( "SpecialEditableTextImageNormal", SpecialEditableTextImageNormal );

		const FEditableTextBoxStyle SpecialEditableTextBoxStyle = FEditableTextBoxStyle()
			.SetBackgroundImageNormal( *SpecialEditableTextImageNormal )
			.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Special_Hovered", FMargin(8.0f/32.0f) ) )
			.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Special_Hovered", FMargin(8.0f/32.0f) ) )
			.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
		Set( "SpecialEditableTextBox", SpecialEditableTextBoxStyle );

		Set( "SearchBox.ActiveBorder", new BOX_BRUSH( "Common/TextBox_Special_Active", FMargin(8.0f/32.0f) ) );
	}
	
	// ProgressBar
	{
		Set( "ProgressBar", FProgressBarStyle()
			.SetBackgroundImage( BOX_BRUSH( "Common/ProgressBar_Background", FMargin(5.f/12.f) ) )
			.SetFillImage( BOX_BRUSH( "Common/ProgressBar_Fill", FMargin(5.f/12.f) ) )
			.SetMarqueeImage( IMAGE_BRUSH( "Common/ProgressBar_Marquee", FVector2D(20,12), FLinearColor::White, ESlateBrushTileType::Horizontal ) )
			);

		// Legacy ProgressBar styles; kept here because other FEditorStyle controls (mis)use them
		// todo: jdale - Widgets using these styles should be updated to use SlateStyle types once FEditorStyle has been obliterated from the Slate core
		Set( "ProgressBar.Background", new BOX_BRUSH( "Common/ProgressBar_Background", FMargin(5.f/12.f) ) );
		Set( "ProgressBar.Marquee", new IMAGE_BRUSH( "Common/ProgressBar_Marquee", FVector2D(20,12), FLinearColor::White, ESlateBrushTileType::Horizontal ) );
		Set( "ProgressBar.BorderPadding", FVector2D(1,0) );
	}

	// WorkingBar
	{
		Set( "WorkingBar", FProgressBarStyle()
			.SetBackgroundImage( FSlateNoResource() )
			.SetFillImage( BOX_BRUSH( "Common/ProgressBar_Fill", FMargin(5.f/12.f) ) )
			.SetMarqueeImage( IMAGE_BRUSH( "Common/WorkingBar_Marquee", FVector2D(20,2), FLinearColor::White, ESlateBrushTileType::Horizontal ) )
			);
	}

	// TaskGraph
	{
		Set( "TaskGraph.Background", new BOX_BRUSH( "Common/ProgressBar_Background", FMargin(5.f/12.f) ) );
		Set( "TaskGraph.Normal", new BOX_BRUSH( "Common/TaskGraph_Normal", FMargin(5.f/12.f) ) );
		Set( "TaskGraph.Selected", new BOX_BRUSH( "Common/TaskGraph_Selected", FMargin(5.f/12.f) ) );
		Set( "TaskGraph.Mono", new BOX_BRUSH( "Common/TaskGraph_Mono", FMargin(5.f/12.f) ) );
		Set( "TaskGraph.BorderPadding", FVector2D(1,0) );
		Set( "TaskGraph.SortUp", new IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) );
		Set( "TaskGraph.SortDown", new IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) );
		Set( "TaskGraph.Home", new IMAGE_BRUSH( "Icons/Home16x16", Icon16x16 ) );
		Set( "TaskGraph.ToParent", new IMAGE_BRUSH( "Icons/ToParent", Icon16x16 ) );
	}

	// Tool panels
	{
		Set( "ToolPanel.GroupBorder", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );
		Set( "ToolPanel.DarkGroupBorder", new BOX_BRUSH( "Common/DarkGroupBorder", FMargin( 4.0f / 16.0f ) ) );
	}

	// Inline Editable Text Block
	{
		FTextBlockStyle InlineEditableTextBlockReadOnly = FTextBlockStyle(NormalText)
			.SetColorAndOpacity( FSlateColor::UseForeground() )
			.SetShadowOffset( FVector2D::ZeroVector )
			.SetShadowColorAndOpacity( FLinearColor::Black );

		FEditableTextBoxStyle InlineEditableTextBlockEditable = FEditableTextBoxStyle()
			.SetFont(NormalText.Font)
			.SetBackgroundImageNormal( BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );

		FInlineEditableTextBlockStyle InlineEditableTextBlockStyle = FInlineEditableTextBlockStyle()
			.SetTextStyle(InlineEditableTextBlockReadOnly)
			.SetEditableTextBoxStyle(InlineEditableTextBlockEditable);
		Set( "InlineEditableTextBlockStyle", InlineEditableTextBlockStyle );
	}

	// Images sizes are specified in Slate Screen Units. These do not necessarily correspond to pixels!
	// An IMAGE_BRUSH( "SomeImage", FVector2D(32,32)) will have a desired size of 16x16 Slate Screen Units.
	// This allows the original resource to be scaled up or down as needed.

	Set( "WhiteTexture", new IMAGE_BRUSH( "Old/White", Icon16x16 ) );

	Set( "NormalFont", NormalFont );
	Set( "BoldFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) );

	Set( "Debug.Border", new BOX_BRUSH( "Old/DebugBorder", 4.0f/16.0f) );

	Set( "Editor.AppIcon", new IMAGE_BRUSH( "Icons/EditorAppIcon", Icon24x24) );

	Set( "FocusRectangle", new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f/32.0f) ) );

	Set( "MarqueeSelection", new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f/32.0f) ) );

	Set( "GenericLock", new IMAGE_BRUSH( "Icons/padlock_locked_16x", Icon16x16 ) );
	Set( "GenericUnlock", new IMAGE_BRUSH( "Icons/padlock_unlocked_16x", Icon16x16 ) );

	Set( "SoftwareCursor_Grab", new IMAGE_BRUSH( "Icons/cursor_grab", Icon16x16 ) );
	Set( "SoftwareCursor_CardinalCross", new IMAGE_BRUSH( "Icons/cursor_cardinal_cross", Icon24x24 ) );
	Set( "SoftwareCursor_UpDown", new IMAGE_BRUSH( "Icons/cursor_updown", Icon16x20 ) );
	
	Set( "Border", new BOX_BRUSH( "Old/Border", 4.0f/16.0f ) );

	Set( "NoteBorder", new BOX_BRUSH( "Old/NoteBorder", FMargin(15.0f/40.0f, 15.0f/40.0f) ) );
	
	Set( "FilledBorder", new BOX_BRUSH( "Old/FilledBorder", 4.0f/16.0f ) );

	Set( "GenericViewButton", new IMAGE_BRUSH( "Icons/view_button", Icon20x20 ) );

#if WITH_EDITOR || IS_PROGRAM
	{
		// Dark Hyperlink - for use on light backgrounds
		FButtonStyle DarkHyperlinkButton = FButtonStyle()
			.SetNormal ( BORDER_BRUSH( "Old/HyperlinkDotted", FMargin(0,0,0,3/16.0f), FLinearColor::Black ) )
			.SetPressed( FSlateNoResource() )
			.SetHovered( BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f), FLinearColor::Black ) );
		FHyperlinkStyle DarkHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(DarkHyperlinkButton)
			.SetTextStyle(NormalText)
			.SetPadding(FMargin(0.0f));
		Set("DarkHyperlink", DarkHyperlink);

		// Visible on hover hyper link
		FButtonStyle HoverOnlyHyperlinkButton = FButtonStyle()
			.SetNormal (FSlateNoResource() )
			.SetPressed(FSlateNoResource() )
			.SetHovered(BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f) ) );
		FHyperlinkStyle HoverOnlyHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(HoverOnlyHyperlinkButton)
			.SetTextStyle(NormalText)
			.SetPadding(FMargin(0.0f));
		Set("HoverOnlyHyperlink", HoverOnlyHyperlink);
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Scrollbar
	{
		Set( "Scrollbar", FScrollBarStyle()
			.SetVerticalBackgroundImage( IMAGE_BRUSH( "Common/Scrollbar_Background_Vertical", FVector2D(8,8) ) )
			.SetHorizontalBackgroundImage( IMAGE_BRUSH( "Common/Scrollbar_Background_Horizontal", FVector2D(8,8) ) )
			.SetNormalThumbImage( BOX_BRUSH( "Common/Scrollbar_Thumb", FMargin(4.f/16.f) ) )
			.SetDraggedThumbImage( BOX_BRUSH( "Common/Scrollbar_Thumb", FMargin(4.f/16.f) ) )
			.SetHoveredThumbImage( BOX_BRUSH( "Common/Scrollbar_Thumb", FMargin(4.f/16.f) ) )
			);
	}


	// Expandable button
	{
		Set( "ExpandableButton.Background", new BOX_BRUSH( "Common/Button", 8.0f/32.0f ) );
		// Extra padding on the right and bottom to account for image shadow
		Set( "ExpandableButton.Padding", FMargin(3.f, 3.f, 6.f, 6.f) );

		Set( "ExpandableButton.Collapsed", new IMAGE_BRUSH( "Old/ExpansionButton_Collapsed", Icon32x32) );
		Set( "ExpandableButton.Expanded_Left", new IMAGE_BRUSH( "Old/ExpansionButton_ExpandedLeft", Icon32x32) );
		Set( "ExpandableButton.Expanded_Center", new IMAGE_BRUSH( "Old/ExpansionButton_ExpandedMiddle", Icon32x32) );
		Set( "ExpandableButton.Expanded_Right", new IMAGE_BRUSH( "Old/ExpansionButton_ExpandedRight", Icon32x32) );

		Set( "ExpandableButton.CloseButton", new IMAGE_BRUSH( "Old/ExpansionButton_CloseOverlay", Icon16x16) );
	}

	// Content reference
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "ContentReference.Background.Normal", new BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) );
		Set( "ContentReference.Background.Hovered", new BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "ContentReference.BorderPadding", FMargin(4.0f, 2.0f) );
		Set( "ContentReference.FindInContentBrowser", new IMAGE_BRUSH( "Icons/lens_12x", Icon12x12 ) );
		Set( "ContentReference.UseSelectionFromContentBrowser", new IMAGE_BRUSH( "Icons/assign_12x", Icon12x12 ) );
		Set( "ContentReference.PickAsset", new IMAGE_BRUSH( "Icons/pillarray_16x", Icon12x12 ) );
		Set( "ContentReference.Clear", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );
		Set( "ContentReference.Tools", new IMAGE_BRUSH( "Icons/wrench_16x", Icon12x12 ) );
	}
#endif // WITH_EDITOR

#if WITH_EDITOR || IS_PROGRAM

	{
		Set( "SystemWideCommands.FindInContentBrowser", new IMAGE_BRUSH( "Icons/icon_toolbar_genericfinder_40px", Icon40x40 ) );
		Set( "SystemWideCommands.FindInContentBrowser.Small", new IMAGE_BRUSH( "Icons/icon_toolbar_genericfinder_40px", Icon20x20 ) );
	}

	// PList Editor
	{
		Set( "PListEditor.HeaderRow.Background",				new BOX_BRUSH( "Common/TableViewHeader", 4.f/32.f ) );

		Set( "PListEditor.FilteredColor",						new FSlateColorBrush( FColor( 0, 255, 0, 80 ) ) );
		Set( "PListEditor.NoOverlayColor",						new FSlateNoResource() );

		Set( "PListEditor.Button_AddToArray",					new IMAGE_BRUSH( "Icons/PlusSymbol_12x", Icon12x12 ) );
	}
	
	// Material List
	{
		Set( "MaterialList.DragDropBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f ) );
		Set( "MaterialList.HyperlinkStyle", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );
		Set( "MaterialList.HyperlinkStyle.ShadowOffset", FVector2D::ZeroVector );
	}

	// Dialogue Wave Details
	{
		Set( "DialogueWaveDetails.SpeakerToTarget", new IMAGE_BRUSH( "PropertyView/SpeakerToTarget", FVector2D(30.0f, 30.0f) ) );
		Set( "DialogueWaveDetails.HeaderBorder", new BOX_BRUSH( "Common/MenuBarBorder", FMargin(4.0f/16.0f) ) );
		Set( "DialogueWaveDetails.PropertyEditorMenu", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
	}

	// Dialogue Wave Parameter Border
	{
		Set( "DialogueWaveParameter.DropDownBorder", new BOX_BRUSH( "Old/Border", 4.0f/16.0f, FLinearColor::Black) );
	}

#endif // WITH_EDITOR || IS_PROGRAM

	Set( "DashedBorder", new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f/32.0f) ) );
	Set( "Checker", new IMAGE_BRUSH( "Old/Checker", Icon16x16, FLinearColor::White, ESlateBrushTileType::Both ) );
	Set( "UniformShadow", new BORDER_BRUSH( "Common/UniformShadow", FMargin( 16.0f / 64.0f ) ) );

	// Splitter
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "Splitter", FSplitterStyle()
			.SetHandleNormalBrush( FSlateNoResource() )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/SplitterHandleHighlight", Icon8x8, FLinearColor::White ) )
			);
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Scroll Box
	{
		Set( "ScrollBox", FScrollBoxStyle()
			.SetTopShadowBrush( IMAGE_BRUSH( "Common/ScrollBoxShadowTop", FVector2D(64,8) ) )
			.SetBottomShadowBrush( IMAGE_BRUSH( "Common/ScrollBoxShadowBottom", FVector2D(64,8) ) )
			);
	}

	// Lists, Trees
	{
		Set( "TableView.Row", FTableRowStyle( NormalTableRowStyle) );
		Set( "TreeArrow_Collapsed", new IMAGE_BRUSH( "Common/TreeArrow_Collapsed", Icon10x10, DefaultForeground ) );
		Set( "TreeArrow_Collapsed_Hovered", new IMAGE_BRUSH( "Common/TreeArrow_Collapsed_Hovered", Icon10x10, DefaultForeground ) );
		Set( "TreeArrow_Expanded", new IMAGE_BRUSH( "Common/TreeArrow_Expanded", Icon10x10, DefaultForeground ) );
		Set( "TreeArrow_Expanded_Hovered", new IMAGE_BRUSH( "Common/TreeArrow_Expanded_Hovered", Icon10x10, DefaultForeground ) );

		const FTableColumnHeaderStyle TableColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortAscendingImage( IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) )
			.SetSortDescendingImage( IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) )
			.SetNormalBrush( BOX_BRUSH( "Common/ColumnHeader", 4.f/32.f ) )
			.SetHoveredBrush( BOX_BRUSH( "Common/ColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );
		Set( "TableView.Header.Column", TableColumnHeaderStyle );

		const FTableColumnHeaderStyle TableLastColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortAscendingImage( IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) )
			.SetSortDescendingImage( IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) )
			.SetNormalBrush( FSlateNoResource() )
			.SetHoveredBrush( BOX_BRUSH( "Common/LastColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );

		const FSplitterStyle TableHeaderSplitterStyle = FSplitterStyle()
			.SetHandleNormalBrush( FSlateNoResource() )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/HeaderSplitterGrip", Icon8x8 ) );

		Set( "TableView.Header", FHeaderRowStyle()
			.SetColumnStyle( TableColumnHeaderStyle )
			.SetLastColumnStyle( TableLastColumnHeaderStyle )
			.SetColumnSplitterStyle( TableHeaderSplitterStyle )
			.SetBackgroundBrush( BOX_BRUSH( "Common/TableViewHeader", 4.f/32.f ) )
			.SetForegroundColor( DefaultForeground )
			);
	}
	
	// Spinboxes
	{
		Set( "SpinBox", FSpinBoxStyle()
			.SetBackgroundBrush( BOX_BRUSH( "Common/Spinbox", FMargin(4.0f/16.0f) ) )
			.SetHoveredBackgroundBrush( BOX_BRUSH( "Common/Spinbox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetActiveFillBrush( BOX_BRUSH( "Common/Spinbox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetInactiveFillBrush( BOX_BRUSH( "Common/Spinbox_Fill", FMargin(4.0f/16.0f, 4.0f/16.0f, 8.0f/16.0f, 4.0f/16.0f) ) )
			.SetArrowsImage( IMAGE_BRUSH( "Common/SpinArrows", Icon12x12 ) )
			.SetForegroundColor( InvertedForeground )
			);

		// Legacy styles; used by other editor widgets
		Set( "SpinBox.Background", new BOX_BRUSH( "Common/Spinbox", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Background.Hovered", new BOX_BRUSH( "Common/Spinbox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Fill", new BOX_BRUSH( "Common/Spinbox_Fill", FMargin(4.0f/16.0f, 4.0f/16.0f, 8.0f/16.0f, 4.0f/16.0f) ) );
		Set( "SpinBox.Fill.Hovered", new BOX_BRUSH( "Common/Spinbox_Fill_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Arrows", new IMAGE_BRUSH( "Common/SpinArrows", Icon12x12 ) );
		Set( "SpinBox.TextMargin", FMargin(1.0f,2.0f) );
	}

	// Numeric entry boxes
	{
		Set( "NumericEntrySpinBox", FSpinBoxStyle()
			.SetBackgroundBrush( FSlateNoResource() )
			.SetHoveredBackgroundBrush( FSlateNoResource() )
			.SetActiveFillBrush( BOX_BRUSH( "Common/Spinbox_Fill_Hovered", FMargin(4.0f/16.0f) ) )
			.SetInactiveFillBrush( BOX_BRUSH( "Common/Spinbox_Fill", FMargin(4.0f/16.0f, 4.0f/16.0f, 8.0f/16.0f, 4.0f/16.0f) ) )
			.SetArrowsImage( IMAGE_BRUSH( "Common/SpinArrows", Icon12x12 ) )
			.SetTextPadding( FMargin(0.0f) )
			.SetForegroundColor( InvertedForeground )
			);
	}

	Set( "ToolTip.Background", new BOX_BRUSH( "Old/ToolTip_Background", FMargin(8.0f/64.0f) ) );

	// Throbber
	{
		Set( "Throbber.Chunk", new IMAGE_BRUSH( "Old/Throbber/Throbber_Piece", FVector2D(16,16) ) );
		Set( "Throbber.CircleChunk", new IMAGE_BRUSH( "Old/Throbber/Throbber_Piece", FVector2D(8,8) ) );
		Set( "SmallThrobber.Chunk", new IMAGE_BRUSH( "Common/ThrobberPiece_Small", FVector2D(8,16) ) );
	}

	{
		Set( "CurveEd.TimelineArea", new IMAGE_BRUSH( "Old/White", Icon16x16, FLinearColor(1,1,1,0.25f) ) );
		Set( "CurveEd.FitHorizontal", new IMAGE_BRUSH( "Icons/FitHorz_16x", Icon16x16 ) );
		Set( "CurveEd.FitVertical", new IMAGE_BRUSH( "Icons/FitVert_16x", Icon16x16 ) );
		Set( "CurveEd.CurveKey", new IMAGE_BRUSH( "Icons/CurveKey_8x", Icon8x8 ) );
		Set( "CurveEd.CurveKeySelected", new IMAGE_BRUSH( "Icons/CurveKey_8x", Icon8x8, SelectionColor ) );
		Set( "CurveEd.InfoFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "CurveEd.Tangent", new IMAGE_BRUSH( "Icons/Tangente_8x", Icon8x8 ) );
	}
	
	// Scrub control buttons
	{
		Set( "Animation.Pause", new IMAGE_BRUSH( "/Animation/Pause", Icon24x24 ) );
		Set( "Animation.Forward", new IMAGE_BRUSH( "/Animation/Forward", Icon24x24 ) );
		Set( "Animation.Forward_Step", new IMAGE_BRUSH( "/Animation/Forward_Step", Icon16x16 ) );
		Set( "Animation.Forward_End", new IMAGE_BRUSH( "/Animation/Forward_End", Icon16x16 ) );
		Set( "Animation.Backward", new IMAGE_BRUSH( "/Animation/Backward", Icon24x24 ) );
		Set( "Animation.Backward_Step", new IMAGE_BRUSH( "/Animation/Backward_Step", Icon16x16 ) );
		Set( "Animation.Backward_End", new IMAGE_BRUSH( "/Animation/Backward_End", Icon16x16 ) );
		Set( "Animation.Loop", new IMAGE_BRUSH( "/Animation/Loop", Icon24x24 ) );
		Set( "Animation.Record", new IMAGE_BRUSH( "/Animation/Record", Icon24x24 ) );
	}

	// Message Log
	{
		Set( "MessageLog", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetShadowOffset( FVector2D::ZeroVector )
		);
		Set( "MessageLog.Error", new IMAGE_BRUSH( "Old/Kismet2/Log_Error", Icon16x16 ) );
		Set( "MessageLog.Warning", new IMAGE_BRUSH( "Old/Kismet2/Log_Warning", Icon16x16 ) );
		Set( "MessageLog.Note", new IMAGE_BRUSH( "Old/Kismet2/Log_Note", Icon16x16 ) );

		Set( "MessageLog.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_MessageLog_16x", Icon16x16 ) );
	}

#if WITH_EDITOR || IS_PROGRAM

	// Animation tools
	{
		Set( "AnimEditor.RefreshButton", new IMAGE_BRUSH( "Old/AnimEditor/RefreshButton", Icon16x16 ) );
		Set( "AnimEditor.VisibleEye", new IMAGE_BRUSH( "Old/AnimEditor/RefreshButton", Icon16x16 ) );
		Set( "AnimEditor.InvisibleEye", new IMAGE_BRUSH( "Old/AnimEditor/RefreshButton", Icon16x16 ) );
		Set( "AnimEditor.FilterSearch", new IMAGE_BRUSH( "Old/FilterSearch", Icon16x16 ) );
		Set( "AnimEditor.FilterCancel", new IMAGE_BRUSH( "Old/FilterCancel", Icon16x16 ) );

		Set( "AnimEditor.NotifyGraphBackground", new IMAGE_BRUSH( "Old/AnimEditor/NotifyTrackBackground", FVector2D(64, 64), FLinearColor::White, ESlateBrushTileType::Both) );

		Set( "BlendSpace.SamplePoint", new IMAGE_BRUSH( "Old/AnimEditor/BlendSpace_Sample", Icon16x16 ) );
		Set( "BlendSpace.SamplePoint_Highlight", new IMAGE_BRUSH( "Old/AnimEditor/BlendSpace_Sample_Highlight", Icon16x16 ) );
		Set( "BlendSpace.SamplePoint_Invalid", new IMAGE_BRUSH( "Old/AnimEditor/BlendSpace_Sample_Invalid", Icon16x16 ) );

		Set( "AnimEditor.EditPreviewParameters", new IMAGE_BRUSH( "Icons/icon_adjust_parameters_40x", Icon40x40) );		
		Set( "AnimEditor.EditPreviewParameters.Small", new IMAGE_BRUSH( "Icons/icon_adjust_parameters_40x", Icon20x20) );		
	}


	// Gamma reference.
	Set("GammaReference",new IMAGE_BRUSH( "Old/GammaReference",FVector2D(256,128)));
	
	Set("TrashCan", new IMAGE_BRUSH( "Old/TrashCan", FVector2D(64, 64)));
	Set("TrashCan_Small", new IMAGE_BRUSH( "Old/TrashCan_Small", FVector2D(18, 18)));
#endif // WITH_EDITOR || IS_PROGRAM

	// Embossed Widget Text
	Set( "EmbossedText", FTextBlockStyle(NormalText)
		.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 24 ) )
		.SetColorAndOpacity( FLinearColor::Black )
		.SetShadowOffset( FVector2D( 0,1 ) )
		.SetShadowColorAndOpacity(FLinearColor(0.8f,0.8f,0.8f, 0.5) )
	);


	// Output Log Window
#if WITH_EDITOR || IS_PROGRAM
	{
		Set("LogTableRow.Normal", FTableRowStyle(NormalTableRowStyle)
			.SetTextColor( FLinearColor(FColor(0xffaaaaaa)) )
			.SetSelectedTextColor( FLinearColor(FColor(0xff333333)) ) );

		Set("LogTableRow.Command", FTableRowStyle(NormalTableRowStyle)
			.SetTextColor( FLinearColor(FColor(0xff33ff33)) )
			.SetSelectedTextColor( FLinearColor(FColor(0xff055505)) ) );

		Set("LogTableRow.Warning", FTableRowStyle(NormalTableRowStyle)
			.SetTextColor( FLinearColor(FColor(0xffbbbb44)) )
			.SetSelectedTextColor( FLinearColor(FColor(0xff444411)) ) );

		Set("LogTableRow.Error", FTableRowStyle(NormalTableRowStyle)
			.SetTextColor( FLinearColor(FColor(0xffff0000)) )
			.SetSelectedTextColor( FLinearColor(FColor(0xff440000)) ) );

		Set("Log.Normal", FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/DroidSansMono", 9 ) ) );

		Set("Log.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_OutputLog_16x", Icon16x16 ) );
	}

	// Debug Tools Window
	{
		Set("DebugTools.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_DebugTools_16x", Icon16x16 ) );
	}

	// Modules Window
	{
		Set("Modules.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_Modules_16px", Icon16x16 ) );
	}

	// Class Viewer Window
	{
		Set("ClassViewer.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_ClassViewer_16x", Icon16x16 ) );
	}

	// Blueprint Debugger Window
	{
		Set("BlueprintDebugger.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_BlueprintDebugger_16x", Icon16x16 ) );
	}

	// Live Editor Window
	{
		Set("LiveEditor.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_LiveEditor_16x", Icon16x16 ) );
	}

	// Developer Tools Menu
	{
		Set("DeveloperTools.MenuIcon", new IMAGE_BRUSH( "Icons/icon_tab_DevTools_16x", Icon16x16 ) );
	}

	// Session Console Window
	{
		Set( "SessionConsole.Copy", new IMAGE_BRUSH( "Icons/icon_file_open_40x", Icon40x40 ) );
		Set( "SessionConsole.Copy.Small", new IMAGE_BRUSH( "Icons/icon_file_open_16px", Icon20x20 ) );
		Set( "SessionConsole.Clear", new IMAGE_BRUSH( "Icons/icon_file_new_40x", Icon40x40 ) );
		Set( "SessionConsole.Clear.Small", new IMAGE_BRUSH( "Icons/icon_file_new_16px", Icon20x20 ) );
		Set( "SessionConsole.Save", new IMAGE_BRUSH( "Icons/icon_file_savelevels_40x", Icon40x40 ) );
		Set( "SessionConsole.Save.Small", new IMAGE_BRUSH( "Icons/icon_file_savelevels_16px", Icon20x20 ) );
	}

	// Session Frontend Window
	{
		Set("SessionFrontEnd.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_SessionFrontEnd_16x", Icon16x16 ) );
		Set("SessionFrontEnd.Tabs.Tools", new IMAGE_BRUSH( "/Icons/icon_tab_Tools_16x", Icon16x16 ) );
	}

	// Session Launcher Window
	{
		Set("SessionLauncher.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_SessionLauncher_16x", Icon16x16 ) );
		Set("SessionLauncher.Tabs.Tools", new IMAGE_BRUSH( "/Icons/icon_tab_Tools_16x", Icon16x16 ) );
	}

	// InputBinding editor
	{
		Set( "InputBindingEditor.ContextFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) );
		Set( "InputBindingEditor.ContextBorder", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, FLinearColor(0.5,0.5,0.5,1.0) ) );
		Set( "InputBindingEditor.SmallFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );

		Set( "InputBindingEditor.HeaderButton", FButtonStyle(NoBorder)
			.SetNormalPadding(FMargin( 1,1,2,2 ))
			.SetPressedPadding(FMargin( 2,2,2,2 )) );

		Set( "InputBindingEditor.HeaderButton.Disabled", new FSlateNoResource() );


		Set( "InputBindingEditor.Tab",  new IMAGE_BRUSH( "Icons/icon_tab_KeyBindings_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.AssetEditor",  new IMAGE_BRUSH( "Icons/icon_keyb_AssetEditor_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.AssetEditor",  new IMAGE_BRUSH( "Icons/icon_keyb_AssetEditor_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.GenericCommands",  new IMAGE_BRUSH( "Icons/icon_keyb_CommonCommands_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.FoliageEditMode",  new IMAGE_BRUSH( "Icons/icon_keyb_FoliageEditMode_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.LandscapeEditor",  new IMAGE_BRUSH( "Icons/icon_keyb_LandscapeEditor_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.LayersView",  new IMAGE_BRUSH( "Icons/icon_keyb_Layers_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.LevelEditor",  new IMAGE_BRUSH( "Icons/icon_keyb_LevelEditor_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.LevelViewport",  new IMAGE_BRUSH( "Icons/icon_keyb_LevelViewports_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.MainFrame",  new IMAGE_BRUSH( "Icons/icon_keyb_MainFrame_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.OutputLog",  new IMAGE_BRUSH( "Icons/icon_keyb_OutputLog_16px", FVector2D( 16, 16 ) ) );
		Set( "InputBindingEditor.PlayWorld",  new IMAGE_BRUSH( "Icons/icon_keyb_PlayWorld_16px", FVector2D( 16, 16 ) ) );		
	}

	// Package restore
	{
		Set( "PackageRestore.FolderOpen", new IMAGE_BRUSH( "Icons/FolderOpen", FVector2D(18, 16) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

#if WITH_EDITOR || IS_PROGRAM
	// Expandable area
	{
		Set( "ExpandableArea", FExpandableAreaStyle()
			.SetCollapsedImage( IMAGE_BRUSH( "Common/TreeArrow_Collapsed", Icon10x10, DefaultForeground ) )
			.SetExpandedImage( IMAGE_BRUSH( "Common/TreeArrow_Expanded", Icon10x10, DefaultForeground ) )
			);
		Set( "ExpandableArea.TitleFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 8 ) );
		Set( "ExpandableArea.Border", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );

		// Legacy styles used by other editor only controls
		Set( "ExpandableArea.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
	}

	// Package Dialog

	{
		Set( "PackageDialog.ListHeader", new BOX_BRUSH( "Old/SavePackages/ListHeader", 4.0f/32.0f ) );
		Set( "SavePackages.SCC_DlgCheckedOutOther", new IMAGE_BRUSH( "Old/SavePackages/SCC_DlgCheckedOutOther", FVector2D( 18, 16 ) ) );
		Set( "SavePackages.SCC_DlgNotCurrent", new IMAGE_BRUSH( "Old/SavePackages/SCC_DlgNotCurrent", FVector2D( 18, 16 ) ) );
		Set( "SavePackages.SCC_DlgReadOnly", new IMAGE_BRUSH( "Old/SavePackages/SCC_DlgReadOnly", FVector2D( 18, 16 ) ) );
		Set( "SavePackages.SCC_DlgNoIcon", new FSlateNoResource() );
	}
#endif // WITH_EDITOR || IS_PROGRAM

#if WITH_EDITOR || IS_PROGRAM
	// Layers General
	{
		Set( "Layer.Icon16x", new IMAGE_BRUSH( "Icons/layer_16x", Icon16x16 ) );
		Set( "Layer.VisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_layer_visible", Icon16x16 ) );
		Set( "Layer.NotVisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_layer_not_visible", Icon16x16 ) );
	}

	// Layer Stats
	{
		Set( "LayerStats.Item.ClearButton", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );
	}

	// Layer Cloud
	{
		Set( "LayerCloud.Item.BorderImage", new BOX_BRUSH( "Common/RoundedSelection_16x", FMargin(4.0f/16.0f) ) );
		Set( "LayerCloud.Item.ClearButton", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );
		Set( "LayerCloud.Item.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) );
	}

	// Layer Browser

	{
		Set( "LayerBrowser.LayerContentsQuickbarBackground",  new BOX_BRUSH( "Common/DarkGroupBorder", 4.f/16.f ) );
		Set( "LayerBrowser.ExploreLayerContents",  new IMAGE_BRUSH( "Icons/ExploreLayerContents", Icon16x16 ) );
		Set( "LayerBrowser.ReturnToLayersList",  new IMAGE_BRUSH( "Icons/ReturnToLayersList", Icon16x16) );
		Set( "LayerBrowser.Actor.RemoveFromLayer", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );

		Set( "LayerBrowserButton", FButtonStyle( Button )
			.SetNormal(FSlateNoResource())
			.SetHovered(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ))
			.SetPressed(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ))
		);

		Set( "LayerBrowserButton.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
	}

	// Levels General
	{
		Set( "Level.Icon16x", new IMAGE_BRUSH( "Icons/levels_16x", Icon16x16 ) );
		Set( "Level.VisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_visible_16px", Icon16x16 ) );
		Set( "Level.VisibleHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_visible_hi_16px", Icon16x16 ) );
		Set( "Level.NotVisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_invisible_16px", Icon16x16 ) );
		Set( "Level.NotVisibleHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_invisible_hi_16px", Icon16x16 ) );
		Set( "Level.LockedIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Locked_16px", Icon16x16 ) );
		Set( "Level.LockedHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Locked_hi_16px", Icon16x16 ) );
		Set( "Level.UnlockedIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_unlocked_16px", Icon16x16 ) );
		Set( "Level.UnlockedHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_unlocked_hi_16px", Icon16x16 ) );
		Set( "Level.ReadOnlyLockedIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_LockedReadOnly_16px", Icon16x16 ) );
		Set( "Level.ReadOnlyLockedHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_LockedReadOnly_hi_16px", Icon16x16 ) );
		Set( "Level.SaveIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Save_16px", Icon16x16 ) );
		Set( "Level.SaveHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Save_hi_16px", Icon16x16 ) );
		Set( "Level.SaveModifiedIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_SaveModified_16px", Icon16x16 ) );
		Set( "Level.SaveModifiedHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_SaveModified_hi_16px", Icon16x16 ) );
		Set( "Level.SaveDisabledIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_SaveDisabled_16px", Icon16x16 ) );
		Set( "Level.SaveDisabledHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_SaveDisabled_hi_16px", Icon16x16 ) );
		Set( "Level.ScriptIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Blueprint_16px", Icon16x16 ) );
		Set( "Level.ScriptHighlightIcon16x", new IMAGE_BRUSH( "Icons/icon_levels_Blueprint_hi_16px", Icon16x16 ) );
		Set( "Level.EmptyIcon16x", new IMAGE_BRUSH( "Icons/Empty_16x", Icon16x16 ) );
		Set( "Level.ColorIcon40x", new IMAGE_BRUSH( "Icons/icon_levels_back_16px", Icon16x16 ) );

		Set( "Level.ArrowUp", new IMAGE_BRUSH( "Old/ArrowUp", Icon12x12 ) );
		Set( "Level.ArrowDown", new IMAGE_BRUSH( "Old/ArrowDown", Icon12x12 ) );
	}

	// Level Browser
	{
		Set( "LevelBrowserButton.Normal", new FSlateNoResource() ); // Note: Intentionally transparent background
		Set( "LevelBrowserButton.Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "LevelBrowserButton.Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) );

		Set( "LevelBrowser.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "LevelBrowser.LabelFontBold", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );
	}

	// World Browser
	{
		Set( "WorldBrowser.Icon16x", new IMAGE_BRUSH( "Icons/levels_16x", Icon16x16 ) );
		Set( "WorldBrowser.VisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_layer_visible", Icon16x16 ) );
		Set( "WorldBrowser.NotVisibleIcon16x", new IMAGE_BRUSH( "Icons/icon_layer_not_visible", Icon16x16 ) );
		Set( "WorldBrowser.LockedIcon16x", new IMAGE_BRUSH( "Icons/padlock_locked_16x", Icon16x16 ) );
		Set( "WorldBrowser.UnlockedIcon16x", new IMAGE_BRUSH( "Icons/padlock_unlocked_16x", Icon16x16 ) );
		Set( "WorldBrowser.SaveIcon16x", new IMAGE_BRUSH( "Icons/Save_16x", Icon16x16 ) );
		Set( "WorldBrowser.SaveModifiedIcon16x", new IMAGE_BRUSH( "Icons/SaveModified_16x", Icon16x16 ) );
		Set( "WorldBrowser.SaveDisabledIcon16x", new IMAGE_BRUSH( "Icons/save_disabled_16x", Icon16x16 ) );
		Set( "WorldBrowser.ScriptIcon16x", new IMAGE_BRUSH( "Icons/ActorIcons/PlayerStart_16x", Icon16x16 ) );
		Set( "WorldBrowser.EmptyIcon16x", new IMAGE_BRUSH( "Icons/Empty_16x", Icon16x16 ) );
		Set( "WorldBrowser.ColorIcon40x", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon16x16 ) );
		Set( "WorldBrowser.LevelLoaded", new IMAGE_BRUSH( "Common/TaskGraph_Normal", Icon16x16 ) );
		Set( "WorldBrowser.LevelUnloaded", new IMAGE_BRUSH( "Common/TaskGraph_Mono", Icon16x16 ) );
		Set( "WorldBrowser.LevelPending", new IMAGE_BRUSH( "Common/TaskGraph_Selected", Icon16x16 ) );
		Set( "WorldBrowser.AddLayer", new IMAGE_BRUSH( "Icons/icon_add_40x", Icon16x16 ) );
		Set( "WorldBrowser.RegularTreeItemFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "WorldBrowser.BoldTreeItemFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );
		Set( "WorldBrowser.StreamingAlwaysLoaded", new IMAGE_BRUSH( "Icons/icon_simulate_40x", Icon16x16 ) );
		Set( "WorldBrowser.StreamingKismet", new IMAGE_BRUSH( "Icons/icon_kismet2_40x", Icon16x16 ) );
		Set( "WorldBrowser.StreamingLevelBounds", new IMAGE_BRUSH( "Icons/icon_box_40x", Icon16x16 ) );
		Set( "WorldBrowser.SimulationViewPositon", new IMAGE_BRUSH( "Icons/icon_variable_used_16x", Icon16x16 ) );
		Set( "WorldBrowser.MouseLocation", new IMAGE_BRUSH( "Icons/cursor_cardinal_cross", Icon16x16 ) );
		Set( "WorldBrowser.MarqueeRectSize", new IMAGE_BRUSH( "Icons/icon_ViewMode_VisualisationGBuffer_16px", Icon16x16 ) );
		Set( "WorldBrowser.WorldSize", new IMAGE_BRUSH( "Icons/icon_world_16x", Icon16x16 ) );
		Set( "WorldBrowser.WorldOrigin", new IMAGE_BRUSH( "Icons/icon_axis_16px", Icon16x16 ) );
		Set( "WorldBrowser.WorldViewScale", new IMAGE_BRUSH( "Icons/ExploreLayerContents", Icon16x16 ) );
		Set( "WorldBrowser.DirectionXPositive", new IMAGE_BRUSH( "Icons/icon_PanRight", Icon16x16 ) );
		Set( "WorldBrowser.DirectionXNegative", new IMAGE_BRUSH( "Icons/icon_PanLeft", Icon16x16 ) );
		Set( "WorldBrowser.DirectionYPositive", new IMAGE_BRUSH( "Icons/icon_PanUp", Icon16x16 ) );
		Set( "WorldBrowser.DirectionYNegative", new IMAGE_BRUSH( "Icons/icon_PanDown", Icon16x16 ) );

		Set( "WorldBrowser.StatusBarText", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 12 ) )
				.SetColorAndOpacity( FLinearColor(0.9, 0.9f, 0.9f, 0.5f) )
				.SetShadowOffset( FVector2D::ZeroVector )
			);
	}


	// Scene Outliner
	{
		Set( "SceneOutliner.FilterSearch", new IMAGE_BRUSH( "Old/FilterSearch", Icon16x16 ) );
		Set( "SceneOutliner.FilterCancel", new IMAGE_BRUSH( "Old/FilterCancel", Icon16x16 ) );
		Set( "SceneOutliner.ChangedItemHighlight", new BOX_BRUSH( "Common/EditableTextSelectionBackground", FMargin(4.f/16.f) ) );
		// Selection color should still be orange to align with the editor viewport.
		// But must also give the hint that the tree is no longer focused.
		Set( "SceneOutliner.TableViewRow", FTableRowStyle(NormalTableRowStyle)
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(FColor(188,161,97)) ) )
		);

		Set( "SceneOutliner.EditBlueprintHyperlinkStyle",
			FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FSlateColor::UseForeground() ) );
		Set( "SceneOutliner.GoToCodeHyperlinkStyle",
			FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FSlateColor::UseSubduedForeground() ) );
	}

	// Socket chooser
	{
		Set( "SocketChooser.TitleFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );
		Set( "SocketIcon.Bone", new IMAGE_BRUSH( "Old/bone", Icon16x16 ) );
		Set( "SocketIcon.Socket", new IMAGE_BRUSH( "Old/socket", Icon16x16 ) );
		Set( "SocketIcon.None", new IMAGE_BRUSH( "Old/Favorites_Disabled", Icon16x16 ) );
	}
	
	// Matinee Recorder
	{
		Set( "MatineeRecorder.Record", new IMAGE_BRUSH( "Icons/Record_16x", Icon16x16 ) );
		Set( "MatineeRecorder.Stop", new IMAGE_BRUSH( "Icons/Stop_16x", Icon16x16 ) );
	}

	// Graph breadcrumb button
	{
		Set( "GraphBreadcrumbButton", FButtonStyle()
			.SetNormal        ( FSlateNoResource() )
			.SetPressed       ( BOX_BRUSH( "Common/Button_Pressed", 8.0f/32.0f, SelectionColor_Pressed ) )
			.SetHovered       ( BOX_BRUSH( "Common/Button_Hovered", 8.0f/32.0f, SelectionColor ) )
			.SetNormalPadding ( FMargin( 2,2,4,4 ) )
			.SetPressedPadding( FMargin( 3,3,3,3 ) )
		);

		Set( "GraphBreadcrumbButtonText", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 14 ) )
			.SetColorAndOpacity( FLinearColor(1,1,1,0.5) )
			.SetShadowOffset( FVector2D::ZeroVector )
		);

		Set( "GraphBreadcrumb.BrowseBack", new IMAGE_BRUSH( "Icons/icon_BlueprintbrowserL_24x", Icon24x24) );
		Set( "GraphBreadcrumb.BrowseForward", new IMAGE_BRUSH( "Icons/icon_BlueprintbrowserR_24x", Icon24x24) );
	}
#endif // WITH_EDITOR

#if WITH_EDITOR || IS_PROGRAM
	// Breadcrumb Trail
		{
		Set( "BreadcrumbTrail.Delimiter", new IMAGE_BRUSH( "Common/Delimiter", Icon16x16 ) );

		Set( "BreadcrumbButton", FButtonStyle()
			.SetNormal ( FSlateNoResource() )
			.SetPressed( BOX_BRUSH( "Common/Button_Pressed", 8.0f/32.0f, SelectionColor_Pressed ) )
			.SetHovered( BOX_BRUSH( "Common/Button_Pressed", 8.0f/32.0f, SelectionColor ) )
			);
		
	}

	// Notification List
	{
		Set( "NotificationList.FontBold", TTF_CORE_FONT( "Fonts/Roboto-Bold", 16 ) );
		Set( "NotificationList.FontLight", TTF_CORE_FONT( "Fonts/Roboto-Light", 12 ) );
		Set( "NotificationList.ItemBackground", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
		Set( "NotificationList.ItemBackground_Border", new BOX_BRUSH( "Old/Menu_Background_Inverted_Border_Bold", FMargin(8.0f/64.0f) ) );
		Set( "NotificationList.SuccessImage", new IMAGE_BRUSH( "Old/Checkbox_checked", Icon16x16 ) );
		Set( "NotificationList.FailImage", new IMAGE_BRUSH( "Old/PropertyEditor/Button_Clear", Icon16x16 ) );
		Set( "NotificationList.DefaultMessage", new IMAGE_BRUSH( "Old/EventMessage_Default", Icon40x40 ) );
		Set( "NotificationList.Glow", new FSlateColorBrush( FColor(255, 255, 255, 255) ) );
		}
#endif // WITH_EDITOR || IS_PROGRAM

#if WITH_EDITOR || IS_PROGRAM
	// Asset editors (common)
	{
		Set( "AssetEditor.SaveAsset.Greyscale", new IMAGE_BRUSH( "icons/icon_file_save_16px", Icon16x16 ) );
		Set( "AssetEditor.SaveAsset", new IMAGE_BRUSH( "Icons/icon_SaveAsset_40x", Icon40x40 ) );
		Set( "AssetEditor.SaveAsset.Small", new IMAGE_BRUSH( "Icons/icon_SaveAsset_40x", Icon20x20 ) );
		Set( "AssetEditor.Reimport", new IMAGE_BRUSH( "Icons/icon_TextureEd_Reimport_40x", Icon40x40 ) );
		Set( "AssetEditor.Reimport.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_Reimport_40x", Icon20x20 ) );		
	}
		
	// Asset Thumbnail
		{
		Set( "AssetThumbnail.AssetBackground", new IMAGE_BRUSH( "Common/AssetBackground", FVector2D(64.f, 64.f), FLinearColor(0.75f, 0.75f, 0.75f, 1.0f) ) );
		Set( "AssetThumbnail.ClassBackground", new IMAGE_BRUSH( "Common/ClassBackground_64x", FVector2D(64.f, 64.f), FLinearColor(0.75f, 0.75f, 0.75f, 1.0f) ) );
		Set( "AssetThumbnail.DataOnlyBPAssetBackground", new IMAGE_BRUSH( "Common/DataOnlyBPAssetBackground_64x", FVector2D(64.f, 64.f), FLinearColor(1, 1, 1, 1) ) );
		Set( "AssetThumbnail.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "AssetThumbnail.FontSmall", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "AssetThumbnail.ColorAndOpacity", FLinearColor(0.75f, 0.75f, 0.75f, 1) );
		Set( "AssetThumbnail.ShadowOffset", FVector2D(1,1) );
		Set( "AssetThumbnail.ShadowColorAndOpacity", FLinearColor(0, 0, 0, 1) );
		Set( "AssetThumbnail.HintFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "AssetThumbnail.HintFontSmall", TTF_CORE_FONT( "Fonts/Roboto-Regular", 6 ) );
		Set( "AssetThumbnail.HintColorAndOpacity", FLinearColor(0.75f, 0.75f, 0.75f, 1) );
		Set( "AssetThumbnail.HintShadowOffset", FVector2D(1,1) );
		Set( "AssetThumbnail.HintShadowColorAndOpacity", FLinearColor(0, 0, 0, 1) );
		Set( "AssetThumbnail.HintBackground", new BOX_BRUSH( "Common/TableViewHeader", FMargin(8.0f/32.0f) ) );
		Set( "AssetThumbnail.Border", new IMAGE_BRUSH( "Common/DiagonalGradient", FVector2D(32.f, 32.f), FLinearColor(1, 1, 1, 1) ) );
	}

	// Open any asset dialog
		{
		Set( "SystemWideCommands.SummonOpenAssetDialog", new IMAGE_BRUSH( "icons/icon_asset_open_16px", Icon16x16 ) );
	
		Set( "GlobalAssetPicker.Background", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
		Set( "GlobalAssetPicker.OutermostMargin", FMargin(4, 4, 4, 4) );

		Set( "GlobalAssetPicker.TitleFont", FTextBlockStyle(NormalText)
				.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) )
			.SetColorAndOpacity( FLinearColor::White )
				.SetShadowOffset( FVector2D( 1,1 ) )
				.SetShadowColorAndOpacity( FLinearColor::Black )
			);
		}


	// Main frame
	{
		Set( "MainFrame.AutoSaveImage", new IMAGE_BRUSH( "icons/icon_Autosave", Icon24x24 ) );
		Set( "GenericCommands.Undo", new IMAGE_BRUSH( "icons/icon_undo_16px", Icon16x16 ) );
		Set( "GenericCommands.Redo", new IMAGE_BRUSH( "icons/icon_redo_16px", Icon16x16 ) );
		Set( "MainFrame.SaveAll", new IMAGE_BRUSH( "icons/icon_file_saveall_16px", Icon16x16 ) );
		Set( "MainFrame.ChoosePackagesToSave", new IMAGE_BRUSH( "icons/icon_file_choosepackages_16px", Icon16x16 ) );
		Set( "MainFrame.NewProject", new IMAGE_BRUSH( "icons/icon_file_ProjectNew_16x", Icon16x16 ) );
		Set( "MainFrame.OpenProject", new IMAGE_BRUSH( "icons/icon_file_ProjectOpen_16x", Icon16x16 ) );
		Set( "MainFrame.AddCodeToProject", new IMAGE_BRUSH( "icons/icon_file_ProjectAddCode_16x", Icon16x16 ) );
		Set( "MainFrame.Exit", new IMAGE_BRUSH( "icons/icon_file_exit_16px", Icon16x16 ) );
		Set( "MainFrame.CookContent", new IMAGE_BRUSH( "icons/icon_package_16x", Icon16x16 ) );
		Set( "MainFrame.PackageProject", new IMAGE_BRUSH( "icons/icon_package_16x", Icon16x16 ) );
		Set( "MainFrame.RecentProjects", new IMAGE_BRUSH( "icons/icon_file_ProjectsRecent_16px", Icon16x16 ) );
		Set( "MainFrame.RecentLevels", new IMAGE_BRUSH( "icons/icon_file_LevelsRecent_16px", Icon16x16 ) );

		Set( "MainFrame.DebugTools.SmallFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "MainFrame.DebugTools.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "MainFrame.DebugTools.LabelFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
	}

	// Main frame
	{
		Set("MainFrame.StatusInfoButton", FButtonStyle(Button)
			.SetNormal( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetHovered( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetPressed( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetNormalPadding(0)
			.SetPressedPadding(0)
		);
	}

	// Main frame
	{
		Set( "MainFrame.SaveLayout", new IMAGE_BRUSH( "Icons/icon_tab_SaveLayout_16x", Icon16x16 ) );
	}

	// CodeView selection detail view section
	{
		Set( "CodeView.ClassIcon", new IMAGE_BRUSH( "Icons/icon_class_16x", Icon16x16 ) );
		Set( "CodeView.FunctionIcon", new IMAGE_BRUSH( "Icons/icon_codeview_16x", Icon16x16 ) );
	}

	Set( "Editor.SearchBoxFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12) );
#endif // WITH_EDITOR || IS_PROGRAM

	// Slider and Volume Control
	{
		FSliderStyle SliderStyle = FSliderStyle()
			.SetNormalThumbImage( BOX_BRUSH( "Common/Button", 8.0f/32.0f ) )
			.SetDisabledThumbImage( BOX_BRUSH( "Common/Button_Disabled", 8.0f/32.0f ) );
		Set( "Slider", SliderStyle );

		Set( "VolumeControl", FVolumeControlStyle()
			.SetSliderStyle( SliderStyle )
			.SetHighVolumeImage( IMAGE_BRUSH( "Common/VolumeControl_High", Icon16x16 ) )
			.SetMidVolumeImage( IMAGE_BRUSH( "Common/VolumeControl_Mid", Icon16x16 ) )
			.SetLowVolumeImage( IMAGE_BRUSH( "Common/VolumeControl_Low", Icon16x16 ) )
			.SetNoVolumeImage( IMAGE_BRUSH( "Common/VolumeControl_Off", Icon16x16 ) )
			.SetMutedImage( IMAGE_BRUSH( "Common/VolumeControl_Muted", Icon16x16 ) )
			);
	}

	// Console
	{
		Set( "DebugConsole.Background", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
	}

#if WITH_EDITOR || IS_PROGRAM
	// About screen
	{
		Set( "AboutScreen.Background", new IMAGE_BRUSH( "About/Background", FVector2D(600,332), FLinearColor::White, ESlateBrushTileType::Both) );
		Set( "AboutScreen.Facebook", new IMAGE_BRUSH( "About/FacebookIcon", FVector2D(35,35) ) );
		Set( "AboutScreen.FacebookHovered", new IMAGE_BRUSH( "About/FacebookIcon_Hovered", FVector2D(35,35) ) );
		Set( "AboutScreen.UE4", new IMAGE_BRUSH( "About/UE4Icon", FVector2D(50,50) ) );
		Set( "AboutScreen.UE4Hovered", new IMAGE_BRUSH( "About/UE4Icon_Hovered", FVector2D(50,50) ) );
		Set( "AboutScreen.EpicGames", new IMAGE_BRUSH( "About/EpicGamesIcon", FVector2D(50,50) ) );
		Set( "AboutScreen.EpicGamesHovered", new IMAGE_BRUSH( "About/EpicGamesIcon_Hovered", FVector2D(50,50) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// New Level Dialog
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "NewLevelDialog.BlackBorder", new FSlateColorBrush( FColor(0, 0, 0, 100) ) );
		Set( "NewLevelDialog.Blank", new IMAGE_BRUSH( "NewLevels/NewLevelBlank", FVector2D(256,256) ) );
		Set( "NewLevelDialog.Default", new IMAGE_BRUSH( "NewLevels/NewLevelDefault", FVector2D(256,256) ) );
	}

	// Build and Submit
	{
		Set( "BuildAndSubmit.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "BuildAndSubmit.SmallFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 7 ) );
	}

	// Sequencer
	{
		Set( "Sequencer.Timeline.ScrubHandleDown", new BOX_BRUSH( "Sequencer/ScrubHandleDown", FMargin( 6.f/13.f, 5/12.f, 6/13.f, 8/12.f ) ) );
		Set( "Sequencer.Timeline.ScrubHandleUp", new BOX_BRUSH( "Sequencer/ScrubHandleUp", FMargin( 6.f/13.f, 8/12.f, 6/13.f, 5/12.f ) ) );
		Set( "Sequencer.Timeline.ScrubHandleWhole", new BOX_BRUSH( "Sequencer/ScrubHandleWhole", FMargin( 6.f/13.f, 10/24.f, 6/13.f, 10/24.f  ) ) );
		Set( "Sequencer.Timeline.NotifyAlignmentMarker", new IMAGE_BRUSH( "Sequencer/NotifyAlignmentMarker", FVector2D(10,19) ) );
		Set( "Sequencer.NotificationImage_AddedPlayMovieSceneEvent", new IMAGE_BRUSH( "Old/Checkbox_checked", Icon16x16 ) );
		Set( "Sequencer.Key", new IMAGE_BRUSH( "Sequencer/Key", FVector2D(7.0f,12.0f) ) );
		Set( "Sequencer.GenericSection.Background", new BOX_BRUSH( "Sequencer/GenericSectionBackground", FMargin(5.f/16.f) ) );
		Set( "Sequencer.SectionArea.Background", new FSlateColorBrush( FColor::White ) );
		Set( "Sequencer.AnimationOutliner.SelectionBorder", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "Sequencer.AnimationOutliner.BoldFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 11 ) );
		Set( "Sequencer.AnimationOutliner.RegularFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "Sequencer.ShotFilter", new IMAGE_BRUSH( "Sequencer/FilteredArea", FVector2D(74,74), FLinearColor::White, ESlateBrushTileType::Both ) );
	}

	// Foliage Edit Mode
		{
		Set( "FoliageEditMode.SetPaint", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Paint_40x", Icon40x40 ) );
		Set( "FoliageEditMode.SetReapplySettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Reapply_40x", Icon40x40 ) );
		Set( "FoliageEditMode.SetSelect", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Select_40x", Icon40x40 ) );
		Set( "FoliageEditMode.SetLassoSelect", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Lasso_40x", Icon40x40 ) );
		Set( "FoliageEditMode.SetPaintBucket", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_PaintBucket_40x", Icon40x40 ) );
		Set( "FoliageEditMode.SetPaint.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Paint_40x", Icon20x20 ) );
		Set( "FoliageEditMode.SetReapplySettings.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Reapply_40x", Icon20x20 ) );
		Set( "FoliageEditMode.SetSelect.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Select_40x", Icon20x20 ) );
		Set( "FoliageEditMode.SetLassoSelect.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_Lasso_40x", Icon20x20 ) );
		Set( "FoliageEditMode.SetPaintBucket.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_PaintBucket_40x", Icon20x20 ) );
						
		Set( "FoliageEditMode.SetNoSettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_NoSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SetPaintSettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_PaintingSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SetClusterSettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_ClusterSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SetNoSettings.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_NoSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SetPaintSettings.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_PaintingSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SetClusterSettings.Small", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEdMode_ClusterSettings_20x", Icon20x20 ) );

		Set( "FoliageEditMode.OpenSettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEditMode_LoadSettings_20px", Icon20x20 ) );
		Set( "FoliageEditMode.SaveSettings", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEditMode_SaveSettings_20px", Icon20x20 ) );
		Set( "FoliageEditMode.DeleteItem", new IMAGE_BRUSH( "Icons/FoliageEditMode/icon_FoliageEditMode_RemoveSettings_20x", Icon20x20 ) );
		Set( "FoliageEditMode.SelectionBackground", new IMAGE_BRUSH( "Icons/FoliageEditMode/FoliageEditMode_SelectionBackground", Icon32x32 ) );
		Set( "FoliageEditMode.ItemBackground", new IMAGE_BRUSH( "Icons/FoliageEditMode/FoliageEditMode_Background", Icon64x64 ) );
		Set( "FoliageEditMode.BubbleBorder", new BOX_BRUSH( "Icons/FoliageEditMode/FoliageEditMode_BubbleBorder", FMargin(8/32.0f) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Crash Tracker
	{
		Set( "CrashTracker", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 18 ) )
			.SetShadowOffset( FVector2D::ZeroVector ) );
			
		Set( "CrashTracker.Cursor", new IMAGE_BRUSH( "Common/MouseCursor", Icon32x32, FLinearColor(1,1,1,1), ESlateBrushTileType::Both ) );
		Set( "CrashTracker.Record", new IMAGE_BRUSH( "Animation/Record", Icon20x20 ) );
	}

#if WITH_EDITOR || IS_PROGRAM
	// Collision Analyzer
	{
		Set( "CollisionAnalyzer.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_CollisionAnalyser_16x", Icon16x16 ) );
		Set( "CollisionAnalyzer.Record", new IMAGE_BRUSH( "Icons/CA_Record", Icon24x24 ) );
		Set( "CollisionAnalyzer.Stop", new IMAGE_BRUSH( "Icons/CA_Stop", Icon24x24 ) );
		Set( "CollisionAnalyzer.ShowRecent", new IMAGE_BRUSH( "Icons/CA_ShowRecent", Icon24x24 ) );
		Set( "CollisionAnalyzer.Group", new IMAGE_BRUSH( "Icons/CA_Group", FVector2D(10,18) ) );
		Set( "CollisionAnalyzer.GroupBackground", new BOX_BRUSH( "/Icons/CA_GroupBackground", FMargin(4.f/16.f) ) );
	}

	// Log Visualizer
	{
		Set( "LogVisualizer.Pause", new IMAGE_BRUSH( "Icons/icon_pause_40x", Icon40x40 ) );
		Set( "LogVisualizer.LogBar.Background", new BOX_BRUSH( "Common/ProgressBar_Background", FMargin(5.f/12.f) ) );
		Set( "LogVisualizer.LogBar.Selected", new BOX_BRUSH( "Common/TaskGraph_Selected", FMargin(5.f/12.f) ) );
		Set( "LogVisualizer.LogBar.EntryDefault", new BOX_BRUSH( "Common/TaskGraph_Mono", FMargin(5.f/12.f) ) );
		Set( "LogVisualizer.LogBar.TimeMark", new BOX_BRUSH( "Icons/LV_BarMark", FMargin(5.f/12.f) ) );
		Set( "LogVisualizer.Record", new IMAGE_BRUSH( "Icons/LV_Record", Icon24x24 ) );
		Set( "LogVisualizer.Stop", new IMAGE_BRUSH( "Icons/LV_Stop", Icon24x24 ) );
		Set( "LogVisualizer.Camera", new IMAGE_BRUSH( "Icons/LV_Camera", Icon40x40 ) );
		Set( "LogVisualizer.Save", new IMAGE_BRUSH( "Icons/LV_Save", Icon40x40 ) );
		Set( "LogVisualizer.Load", new IMAGE_BRUSH( "Icons/LV_Load", Icon40x40 ) );
		Set( "LogVisualizer.Remove", new IMAGE_BRUSH( "Icons/LV_Remove", Icon40x40 ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

#if WITH_EDITOR
	// Surface Props
	{
		Set( "SurfaceDetails.PanUPositive", new IMAGE_BRUSH( "Icons/icon_PanRight", Icon16x16 ) );
		Set( "SurfaceDetails.PanUNegative", new IMAGE_BRUSH( "Icons/icon_PanLeft", Icon16x16 ) );

		Set( "SurfaceDetails.PanVPositive", new IMAGE_BRUSH( "Icons/icon_PanUp", Icon16x16 ) );
		Set( "SurfaceDetails.PanVNegative", new IMAGE_BRUSH( "Icons/icon_PanDown", Icon16x16 ) );

		
		Set( "SurfaceDetails.ClockwiseRotation", new IMAGE_BRUSH( "Icons/icon_ClockwiseRotation_16x", Icon16x16 ) );
		Set( "SurfaceDetails.AntiClockwiseRotation", new IMAGE_BRUSH( "Icons/icon_AntiClockwiseRotation_16x", Icon16x16 ) );
	}

	// GameProjectDialog
	{
		Set( "GameProjectDialog.NewProjectTitle", FTextBlockStyle(NormalText) 
			.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 28 ) )
			.SetShadowOffset( FVector2D( 1,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);

		Set( "GameProjectDialog.RecentProjectsTitle", FTextBlockStyle(NormalText) 
			.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 16 ) )
			.SetColorAndOpacity( FLinearColor(0.8f,0.8f,0.8f,1) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);

		Set( "GameProjectDialog.ProjectNamePathLabels", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);

		Set( "GameProjectDialog.ErrorLabelFont", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetShadowOffset( FVector2D::ZeroVector )
		);

		Set( "GameProjectDialog.ErrorLabelBorder", new FSlateColorBrush( FLinearColor(0.2, 0, 0) ) );
		Set( "GameProjectDialog.ErrorLabelCloseButton", new IMAGE_BRUSH( "Icons/Cross_12x", Icon12x12 ) );

		Set( "GameProjectDialog.TemplateListView.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateNoResource() )
			.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetOddRowBackgroundBrush( FSlateNoResource() )
			.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
			.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor )  )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);

		Set( "GameProjectDialog.DefaultGameThumbnail", new IMAGE_BRUSH( "GameProjectDialog/default_game_thumbnail", Icon128x128 ) );
		Set( "GameProjectDialog.BlankProjectThumbnail", new IMAGE_BRUSH( "GameProjectDialog/blank_project_thumbnail", Icon128x128 ) );
		Set( "GameProjectDialog.BasicCodeThumbnail", new IMAGE_BRUSH( "GameProjectDialog/basic_code_thumbnail", Icon128x128 ) );
		Set( "GameProjectDialog.CodeImage", new IMAGE_BRUSH( "GameProjectDialog/feature_code", FVector2D(70,31) ) );
		Set( "GameProjectDialog.CodeBorder", new BOX_BRUSH( "GameProjectDialog/feature_border", FMargin(4.0f/16.0f), FLinearColor(0.570, 0.359, 0.081, 1.f) ) );
		Set( "GameProjectDialog.FeatureText", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 14 ) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) ) );
		Set( "GameProjectDialog.TemplateItemTitle", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 14 ) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) ) );

		Set( "GameProjectDialog.FolderIconClosed", new IMAGE_BRUSH( "Icons/FolderClosed", FVector2D(18, 16) ) );
		Set( "GameProjectDialog.FolderIconOpen", new IMAGE_BRUSH( "Icons/FolderOpen", FVector2D(18, 16) ) );
		Set( "GameProjectDialog.ProjectFileIcon", new IMAGE_BRUSH( "Icons/doc_16x", FVector2D(18, 16) ) );
	}

	// NewClassDialog
	{
		Set( "NewClassDialog.PageTitle", FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 28 ) )
			.SetShadowOffset( FVector2D( 1,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);

		Set( "NewClassDialog.SelectedParentClassLabel", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);

		Set( "NewClassDialog.ErrorLabelFont", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
		 );
	
		Set( "NewClassDialog.ErrorLabelBorder", new FSlateColorBrush( FLinearColor(0.2, 0, 0) ) );
		Set( "NewClassDialog.ErrorLabelCloseButton", new IMAGE_BRUSH( "Icons/Cross_12x", Icon12x12 ) );

		Set( "NewClassDialog.ParentClassListView.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateNoResource() )
			.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetOddRowBackgroundBrush( FSlateNoResource() )
			.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
			.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor )  )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);

		Set( "NewClassDialog.ParentClassItemTitle", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 14 ) )
			.SetShadowOffset( FVector2D( 0,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
		);
	}

	// Stats visualizer
	{
		Set( "StatsHeader", new BOX_BRUSH( "Common/TableViewHeader", 4.f/32.f ) );
	}

	// Package Migration
	{
		Set( "PackageMigration.DialogTitle", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) )
		);
	}


#endif // WITH_EDITOR

#if WITH_EDITOR || IS_PROGRAM
	// ToolBar
	{
		Set( "ToolBar.Background", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );
		Set( "ToolBar.Icon", new IMAGE_BRUSH( "Icons/icon_tab_toolbar_16px", Icon16x16 ) );
		Set( "ToolBar.Expand", new IMAGE_BRUSH( "Icons/toolbar_expand_16x", Icon16x16) );
		Set( "ToolBar.SubMenuIndicator", new IMAGE_BRUSH( "Common/SubmenuArrow", Icon8x8 ) );
		Set( "ToolBar.SToolBarComboButtonBlock.Padding", FMargin(4.0f));
		Set( "ToolBar.SToolBarButtonBlock.Padding", FMargin(4.0f));
		Set( "ToolBar.SToolBarCheckComboButtonBlock.Padding", FMargin(4.0f));
		Set( "ToolBar.SToolBarButtonBlock.CheckBox.Padding", FMargin(0.0f) );
		Set( "ToolBar.SToolBarComboButtonBlock.ComboButton.Color", DefaultForeground );

		Set( "ToolBar.Block.IndentedPadding", FMargin( 18.0f, 2.0f, 4.0f, 4.0f ) );
		Set( "ToolBar.Block.Padding", FMargin( 2.0f, 2.0f, 4.0f, 4.0f ) );

		Set( "ToolBar.Separator", new BOX_BRUSH( "Old/Button", 4.0f/32.0f ) );
		Set( "ToolBar.Separator.Padding", FMargin( 0.5f ) );

		Set( "ToolBar.Label", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "ToolBar.EditableText", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "ToolBar.Keybinding", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );

		Set( "ToolBar.Heading", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor( 0.4f, 0.4, 0.4f, 1.0f ) ) );

		/* Create style for "ToolBar.CheckBox" widget ... */
		const FCheckBoxStyle ToolBarCheckBoxStyle = FCheckBoxStyle()
				.SetUncheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox", Icon14x14 ) )
				.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked",  Icon14x14 ) )
				.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
				.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14 ) )
				.SetUncheckedPressedImage(IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
				.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );
		/* ... and add new style */
		Set( "ToolBar.CheckBox", ToolBarCheckBoxStyle );

		// This radio button is actually just a check box with different images
		/* Create style for "ToolBar.RadioButton" widget ... */
		const FCheckBoxStyle ToolbarRadioButtonCheckBoxStyle = FCheckBoxStyle()
				.SetUncheckedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
				.SetCheckedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x",  Icon16x16 ) )
				.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor ) )
				.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor ) )
				.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) )
				.SetCheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor_Pressed ) );

		/* ... and add new style */
		Set( "ToolBar.RadioButton", ToolbarRadioButtonCheckBoxStyle );

		/* Create style for "ToolBar.ToggleButton" widget ... */
		const FCheckBoxStyle ToolBarToggleButtonCheckBoxStyle = FCheckBoxStyle()
				.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
				.SetUncheckedImage( FSlateNoResource() )
				.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
				.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
				.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
				.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
				.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );
		/* ... and add new style */
		Set( "ToolBar.ToggleButton", ToolBarToggleButtonCheckBoxStyle );

		Set( "ToolBar.Button", FButtonStyle(Button)
			.SetNormal ( FSlateNoResource() )
			.SetPressed( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetHovered( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
		);

		Set( "ToolBar.Button.Normal", new FSlateNoResource() );
		Set( "ToolBar.Button.Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "ToolBar.Button.Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) );

		Set( "ToolBar.Button.Checked", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "ToolBar.Button.Checked_Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "ToolBar.Button.Checked_Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );
	}


	// MenuBar
	{
		Set( "Menu.Background",	new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
		Set( "Menu.Icon", new IMAGE_BRUSH( "Icons/icon_tab_toolbar_16px", Icon16x16 ) );
		Set( "Menu.Expand", new IMAGE_BRUSH( "Icons/toolbar_expand_16x", Icon16x16) );
		Set( "Menu.SubMenuIndicator", new IMAGE_BRUSH( "Common/SubmenuArrow", Icon8x8 ) );
		Set( "Menu.SToolBarComboButtonBlock.Padding", FMargin(4.0f));
		Set( "Menu.SToolBarButtonBlock.Padding", FMargin(4.0f));
		Set( "Menu.SToolBarCheckComboButtonBlock.Padding", FMargin(4.0f));
		Set( "Menu.SToolBarButtonBlock.CheckBox.Padding", FMargin(0.0f) );
		Set( "Menu.SToolBarComboButtonBlock.ComboButton.Color", DefaultForeground );

		Set( "Menu.Block.IndentedPadding", FMargin( 18.0f, 2.0f, 4.0f, 4.0f ) );
		Set( "Menu.Block.Padding", FMargin( 2.0f, 2.0f, 4.0f, 4.0f ) );

		Set( "Menu.Separator", new BOX_BRUSH( "Old/Button", 4.0f/32.0f ) );
		Set( "Menu.Separator.Padding", FMargin( 0.5f ) );

		Set( "Menu.Label", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "Menu.EditableText", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "Menu.Keybinding", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );

		Set( "Menu.Heading", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor( 0.4f, 0.4, 0.4f, 1.0f ) ) );

		/* Set images for various SCheckBox states associated with menu check box items... */
		const FCheckBoxStyle BasicMenuCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox", Icon14x14 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked", Icon14x14 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14 ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16 ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );
 
		/* ...and add the new style */
		Set( "Menu.CheckBox", BasicMenuCheckBoxStyle );
						
		/* Read-only checkbox that appears next to a menu item */
		/* Set images for various SCheckBox states associated with read-only menu check box items... */
		const FCheckBoxStyle BasicMenuCheckStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheck", Icon14x14 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheck", Icon14x14 ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheck", Icon14x14 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUndeterminedHoveredImage( FSlateNoResource() )
			.SetUndeterminedPressedImage( FSlateNoResource() );

		/* ...and add the new style */
		Set( "Menu.Check", BasicMenuCheckStyle );

		/* This radio button is actually just a check box with different images */
		/* Set images for various Menu radio button (SCheckBox) states... */
		const FCheckBoxStyle BasicMenuRadioButtonStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) );

		/* ...and set new style */
		Set( "Menu.RadioButton", BasicMenuRadioButtonStyle );

		/* Create style for "Menu.ToggleButton" widget ... */
		const FCheckBoxStyle MenuToggleButtonCheckBoxStyle = FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
			.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
			.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );
		/* ... and add new style */
		Set( "Menu.ToggleButton", MenuToggleButtonCheckBoxStyle );

		Set( "Menu.Button", FButtonStyle( NoBorder )
			.SetNormal ( FSlateNoResource() )
			.SetPressed( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetHovered( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
			.SetNormalPadding( FMargin(0,1) )
			.SetPressedPadding( FMargin(0,2,0,0) )
		);

		Set( "Menu.Button.Checked", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "Menu.Button.Checked_Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "Menu.Button.Checked_Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );

		/* The style of a menu bar button when it has a sub menu open */
		Set( "Menu.Button.SubMenuOpen", new BORDER_BRUSH( "Common/Selection", FMargin(4.f/16.f), FLinearColor(0.10f, 0.10f, 0.10f) ) );
	}

	// ViewportLayoutToolbar
	{
		const FLinearColor LayoutSelectionColor_Hovered = FLinearColor(0.5f, 0.5f, 0.5f);

		Set( "ViewportLayoutToolbar.Background",	new FSlateNoResource() );

		/* Create style for "ViewportLayoutToolbar.ToggleButton" ... */
		const FCheckBoxStyle ViewportLayoutToolbarToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, LayoutSelectionColor_Hovered ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, LayoutSelectionColor_Hovered ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) )
			.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) )
			.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );
		/* ... and add new style */
		Set( "ViewportLayoutToolbar.ToggleButton", ViewportLayoutToolbarToggleButtonStyle );

		Set( "ViewportLayoutToolbar.SToolBarButtonBlock.Padding", FMargin(4.0f) );
		Set( "ViewportLayoutToolbar.SToolBarButtonBlock.CheckBox.Padding", FMargin(0.0f) );
		Set( "ViewportLayoutToolbar.SToolBarComboButtonBlock.ComboButton.Color", DefaultForeground );
		}

	// NotificationBar
	{
		Set( "NotificationBar.Background",	new FSlateNoResource() );
		Set( "NotificationBar.Icon", new FSlateNoResource() );
		Set( "NotificationBar.Expand", new IMAGE_BRUSH( "Icons/toolbar_expand_16x", Icon16x16) );
		Set( "NotificationBar.SubMenuIndicator", new IMAGE_BRUSH( "Common/SubmenuArrow", Icon8x8 ) );

		Set( "NotificationBar.Block.IndentedPadding", FMargin( 0 ) );
		Set( "NotificationBar.Block.Padding", FMargin( 0 ) );

		Set( "NotificationBar.Separator", new BOX_BRUSH( "Old/Button", 4.0f/32.0f ) );
		Set( "NotificationBar.Separator.Padding", FMargin( 0.5f ) );

		Set( "NotificationBar.Label", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "NotificationBar.EditableText", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "NotificationBar.Keybinding", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );

		Set( "NotificationBar.Heading", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor( 0.4f, 0.4, 0.4f, 1.0f ) ) );

		const FCheckBoxStyle NotificationBarCheckBoxCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox", Icon14x14 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14 ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked",  Icon14x14 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined", Icon16x16 ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/Checkbox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/Checkbox_Checked_Hovered", Icon16x16 ) );
		Set( "NotificationBar.CheckBox", NotificationBarCheckBoxCheckBoxStyle );

		// Read-only checkbox that appears next to a menu item
		const FCheckBoxStyle NotificationBarCheckCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUncheckedPressedImage( FSlateNoResource() )
			.SetUncheckedHoveredImage( FSlateNoResource() )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUndeterminedPressedImage( FSlateNoResource() )
			.SetUndeterminedHoveredImage( FSlateNoResource() );
		Set( "NotificationBar.Check", NotificationBarCheckCheckBoxStyle );

		// This radio button is actually just a check box with different images
		const FCheckBoxStyle NotificationBarRadioButtonCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor_Pressed ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x",  Icon16x16 ) );
		Set( "NotificationBar.RadioButton", NotificationBarRadioButtonCheckBoxStyle );

		const FCheckBoxStyle NotificationBarToggleButtonCheckBoxStyle = FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) )
			.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) )
			.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "NotificationBar.ToggleButton", NotificationBarToggleButtonCheckBoxStyle );

		Set( "NotificationBar.Button", FButtonStyle( NoBorder )
			.SetNormal ( FSlateNoResource() )
			.SetPressed( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) )
			.SetHovered( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) )
			.SetNormalPadding( FMargin(0,1) )
			.SetPressedPadding( FMargin(0,2,0,0) )
			);

		Set( "NotificationBar.Button.Checked", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "NotificationBar.Button.Checked_Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "NotificationBar.Button.Checked_Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x",  4.0f/16.0f, SelectionColor ) );

		Set( "NotificationBar.SToolBarButtonBlock.CheckBox.Padding", FMargin(4.0f) );
		Set( "NotificationBar.SToolBarComboButtonBlock.ComboButton.Color", DefaultForeground );
	}

	// Viewport ToolbarBar
	{
		Set( "ViewportMenu.Background",	new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f), FLinearColor::Transparent ) );
		Set( "ViewportMenu.Icon", new IMAGE_BRUSH( "Icons/icon_tab_toolbar_16px", Icon16x16 ) );
		Set( "ViewportMenu.Expand", new IMAGE_BRUSH( "Icons/toolbar_expand_16x", Icon8x8) );
		Set( "ViewportMenu.SubMenuIndicator", new IMAGE_BRUSH( "Common/SubmenuArrow", Icon8x8) );
		Set( "ViewportMenu.SToolBarComboButtonBlock.Padding", FMargin(0));
		Set( "ViewportMenu.SToolBarButtonBlock.Padding", FMargin(0));
		Set( "ViewportMenu.SToolBarCheckComboButtonBlock.Padding", FMargin(0));
		Set( "ViewportMenu.SToolBarButtonBlock.CheckBox.Padding", FMargin(4.0f) );
		Set( "ViewportMenu.SToolBarComboButtonBlock.ComboButton.Color", FLinearColor(0.f,0.f,0.f,0.75f) );

		Set( "ViewportMenu.Separator", new BOX_BRUSH( "Old/Button", 8.0f/32.0f, FLinearColor::Transparent ) );
		Set( "ViewportMenu.Separator.Padding", FMargin( 100.0f ) );

		Set( "ViewportMenu.Label.Font", TTF_CORE_FONT( "Fonts/Roboto-Bold", 9) );
		Set( "ViewportMenu.EditableText.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "ViewportMenu.Keybinding.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );

		Set( "ViewportMenu.Block.IndentedPadding", FMargin( 0 ) );
		Set( "ViewportMenu.Block.Padding", FMargin( 0 ) );

		Set( "ViewportMenu.Heading.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "ViewportMenu.Heading.ColorAndOpacity", FLinearColor( 0.4f, 0.4, 0.4f, 1.0f ) );
	
		const FCheckBoxStyle ViewportMenuCheckBoxCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage(			IMAGE_BRUSH( "Common/SmallCheckbox", Icon14x14 ) )
			.SetUncheckedPressedImage(	IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetUncheckedHoveredImage(	IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
			.SetCheckedHoveredImage(	IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14 ) )
			.SetCheckedPressedImage(	IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage(			IMAGE_BRUSH( "Common/SmallCheckbox_Checked",  Icon14x14 ) );
		Set( "ViewportMenu.CheckBox", ViewportMenuCheckBoxCheckBoxStyle );

		// Read-only checkbox that appears next to a menu item
		const FCheckBoxStyle ViewportMenuCheckCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage(			IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetUncheckedPressedImage(	FSlateNoResource() )
			.SetUncheckedHoveredImage(	FSlateNoResource() )
			.SetCheckedHoveredImage(	IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetCheckedPressedImage(	IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetCheckedImage(			IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) );
		Set( "ViewportMenu.Check", ViewportMenuCheckCheckBoxStyle );

		const FString SmallRoundedButton			(TEXT("Common/SmallRoundedButton"));
		const FString SmallRoundedButtonStart		(TEXT("Common/SmallRoundedButtonLeft"));
		const FString SmallRoundedButtonMiddle	(TEXT("Common/SmallRoundedButtonCentre"));
		const FString SmallRoundedButtonEnd		(TEXT("Common/SmallRoundedButtonRight"));

		const FLinearColor NormalColor(1,1,1,0.75f);
		const FLinearColor PressedColor(1,1,1,1.f);

		const FCheckBoxStyle ViewportMenuRadioButtonCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage(			IMAGE_BRUSH( "Common/MenuItemRadioButton_Off", Icon14x14 ) )
			.SetUncheckedPressedImage(	IMAGE_BRUSH( "Common/MenuItemRadioButton_Off", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetUncheckedHoveredImage(	IMAGE_BRUSH( "Common/MenuItemRadioButton_Off", Icon14x14 ) )
			.SetCheckedHoveredImage(	IMAGE_BRUSH( "Common/MenuItemRadioButton_On", Icon14x14 ) )
			.SetCheckedPressedImage(	IMAGE_BRUSH( "Common/MenuItemRadioButton_On_Pressed", Icon14x14 ) )
			.SetCheckedImage(			IMAGE_BRUSH( "Common/MenuItemRadioButton_On",  Icon14x14 ) );
		Set( "ViewportMenu.RadioButton", ViewportMenuRadioButtonCheckBoxStyle );

		/* Create style for "ViewportMenu.ToggleButton" ... */
		const FCheckBoxStyle ViewportMenuToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(			ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage(			BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), NormalColor ) )
			.SetUncheckedPressedImage(	BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), PressedColor ) )
			.SetUncheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), PressedColor ) )
			.SetCheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), SelectionColor_Pressed ) )
			.SetCheckedPressedImage(	BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), SelectionColor_Pressed )  )
			.SetCheckedImage(			BOX_BRUSH( *SmallRoundedButton, FMargin(7.f/16.f), SelectionColor_Pressed ) );
		/* ... and add new style */
		Set( "ViewportMenu.ToggleButton", ViewportMenuToggleButtonStyle );

		/* Create style for "ViewportMenu.ToggleButton.Start" ... */
		const FCheckBoxStyle ViewportMenuToggleStartButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(			ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage(			BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), NormalColor ) )
			.SetUncheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), PressedColor ) )
			.SetUncheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), PressedColor ) )
			.SetCheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), SelectionColor_Pressed ) )
			.SetCheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), SelectionColor_Pressed )  )
			.SetCheckedImage(			BOX_BRUSH( *SmallRoundedButtonStart, FMargin(7.f/16.f), SelectionColor_Pressed ) );
		/* ... and add new style */
		Set( "ViewportMenu.ToggleButton.Start", ViewportMenuToggleStartButtonStyle );

		/* Create style for "ViewportMenu.ToggleButton.Middle" ... */
		const FCheckBoxStyle ViewportMenuToggleMiddleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(			ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage(			BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), NormalColor ) )
			.SetUncheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), PressedColor ) )
			.SetUncheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), PressedColor ) )
			.SetCheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), SelectionColor_Pressed ) )
			.SetCheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), SelectionColor_Pressed )  )
			.SetCheckedImage(			BOX_BRUSH( *SmallRoundedButtonMiddle, FMargin(7.f/16.f), SelectionColor_Pressed ) );
		/* ... and add new style */
		Set( "ViewportMenu.ToggleButton.Middle", ViewportMenuToggleMiddleButtonStyle );

		/* Create style for "ViewportMenu.ToggleButton.End" ... */
		const FCheckBoxStyle ViewportMenuToggleEndButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(			ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage(			BOX_BRUSH( *SmallRoundedButtonEnd, FMargin(7.f/16.f), NormalColor ) )
			.SetUncheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonEnd, FMargin(7.f/16.f), PressedColor ) )
			.SetUncheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonEnd, FMargin(7.f/16.f), PressedColor ) )
			.SetCheckedHoveredImage(	BOX_BRUSH( *SmallRoundedButtonEnd,  FMargin(7.f/16.f), SelectionColor_Pressed ) )
			.SetCheckedPressedImage(	BOX_BRUSH( *SmallRoundedButtonEnd,  FMargin(7.f/16.f), SelectionColor_Pressed )  )
			.SetCheckedImage(			BOX_BRUSH( *SmallRoundedButtonEnd,  FMargin(7.f/16.f), SelectionColor_Pressed ) );
		/* ... and add new style */
		Set( "ViewportMenu.ToggleButton.End", ViewportMenuToggleEndButtonStyle );

		const FMargin NormalPadding = FMargin(4.0f,4.0f,4.0f,4.0f);
		const FMargin PressedPadding = FMargin(4.0f,4.0f,4.0f,4.0f);

		const FButtonStyle ViewportMenuButton = FButtonStyle(Button)
			.SetNormal ( BOX_BRUSH( *SmallRoundedButton, 7.0f/16.0f, NormalColor))
			.SetPressed( BOX_BRUSH( *SmallRoundedButton, 7.0f/16.0f, PressedColor ) )
			.SetHovered( BOX_BRUSH( *SmallRoundedButton, 7.0f/16.0f, PressedColor ) )
			.SetPressedPadding(PressedPadding)
			.SetNormalPadding(NormalPadding);

		Set( "ViewportMenu.Button", ViewportMenuButton );

		Set( "ViewportMenu.Button.Start", FButtonStyle(ViewportMenuButton)
			.SetNormal ( BOX_BRUSH( *SmallRoundedButtonStart, 7.0f/16.0f, NormalColor))
			.SetPressed( BOX_BRUSH( *SmallRoundedButtonStart, 7.0f/16.0f, PressedColor ) )
			.SetHovered( BOX_BRUSH( *SmallRoundedButtonStart, 7.0f/16.0f, PressedColor ) )
			);

		Set( "ViewportMenu.Button.Middle", FButtonStyle(ViewportMenuButton)
			.SetNormal ( BOX_BRUSH( *SmallRoundedButtonMiddle, 7.0f/16.0f, NormalColor))
			.SetPressed( BOX_BRUSH( *SmallRoundedButtonMiddle, 7.0f/16.0f, PressedColor ) )
			.SetHovered( BOX_BRUSH( *SmallRoundedButtonMiddle, 7.0f/16.0f, PressedColor ) )
			);

		Set( "ViewportMenu.Button.End", FButtonStyle(ViewportMenuButton)
			.SetNormal ( BOX_BRUSH( *SmallRoundedButtonEnd, 7.0f/16.0f, NormalColor))
			.SetPressed( BOX_BRUSH( *SmallRoundedButtonEnd, 7.0f/16.0f, PressedColor ) )
			.SetHovered( BOX_BRUSH( *SmallRoundedButtonEnd, 7.0f/16.0f, PressedColor ) )
			);
	}

	// Viewport actor preview's pin/unpin buttons
	{
		Set( "ViewportActorPreview.Pinned", new IMAGE_BRUSH( "Common/PushPin_Down", Icon16x16 ) );
		Set( "ViewportActorPreview.Unpinned", new IMAGE_BRUSH( "Common/PushPin_Up", Icon16x16 ) );
	}

	// Standard Dialog Settings
	{
		Set("StandardDialog.ContentPadding",FMargin(12.0f,2.0f));
		Set("StandardDialog.SlotPadding",FMargin(6.0f, 0.0f, 6.0f, 0.0f));
		Set("StandardDialog.MinDesiredSlotWidth", 80.0f );
		Set("StandardDialog.MinDesiredSlotHeight", 0.0f );
		Set("StandardDialog.LargeFont",  TTF_CORE_FONT( "Fonts/Roboto-Regular", 11 ) );
	}

	// Highres Screenshot
	{
		Set("HighresScreenshot.WarningStrip", new IMAGE_BRUSH( "Common/WarningStripe", FVector2D(20,6), FLinearColor::White, ESlateBrushTileType::Horizontal ) );
		Set("HighresScreenshot.SpecifyCaptureRectangle", new IMAGE_BRUSH( "Icons/icon_CaptureRegion_24x", Icon24x24 ) );
		Set("HighresScreenshot.FullViewportCaptureRegion", new IMAGE_BRUSH( "Icons/icon_CaptureRegion_FullViewport_24x", Icon24x24 ) );
		Set("HighresScreenshot.Capture", new IMAGE_BRUSH( "Icons/icon_HighResScreenshotCapture_24px", Icon24x24 ) );
		Set("HighresScreenshot.AcceptCaptureRegion", new IMAGE_BRUSH( "Icons/icon_CaptureRegionAccept_24x", Icon24x24 ) );
		Set("HighresScreenshot.DiscardCaptureRegion", new IMAGE_BRUSH( "Icons/icon_CaptureRegionDiscard_24x", Icon24x24 ) );
	}

	// Scalability 
	{
		const float Tint = 0.65f;
		Set("Scalability.RowBackground", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f), FLinearColor(Tint, Tint, Tint) ) );
		Set("Scalability.TitleFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) );
		Set("Scalability.GroupFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );
	}

	// User Feedback
	{
		Set( "UserFeedback.Button", FButtonStyle()
			.SetNormalPadding(0)
			.SetPressedPadding(0)
			.SetNormal (IMAGE_BRUSH( "UserFeedback/Feedback_Normal", Icon16x16 ))
			.SetHovered(IMAGE_BRUSH( "UserFeedback/Feedback_Hovered", Icon16x16 ))
			.SetPressed(IMAGE_BRUSH( "UserFeedback/Feedback_Pressed", Icon16x16 ))
			);

		Set( "UserFeedback.PositiveIcon", new IMAGE_BRUSH( "UserFeedback/Feedback_Positive", Icon16x16) );
		Set( "UserFeedback.NegativeIcon", new IMAGE_BRUSH( "UserFeedback/Feedback_Negative", Icon16x16) );
	}
#endif // WITH_EDITOR || IS_PROGRAM
}

void FSlateEditorStyle::FStyle::SetupWindowStyles()
{
	// Window styling
	{
		Set( "Window.Background",                 new IMAGE_BRUSH( "Old/Window/WindowBackground", FVector2D(74, 74), FLinearColor::White, ESlateBrushTileType::Both) );
		Set( "Window.Border",                     new BOX_BRUSH( "Old/Window/WindowBorder", 0.48f ) );
		Set( "Window.Outline",					  new BORDER_BRUSH( "Old/Window/WindowOutline", FMargin(4.0f/32.0f) ) );
		Set( "Window.Outline.Color",			  FLinearColor(0.1f, 0.1f, 0.1f, 1.0f) );
		Set( "Window.Title.Active",               new IMAGE_BRUSH( "Old/Window/WindowTitle", Icon32x32, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) );
		Set( "Window.Title.Inactive",             new IMAGE_BRUSH( "Old/Window/WindowTitle_Inactive", Icon32x32, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) );
		Set( "Window.Title.Flash",				  new IMAGE_BRUSH( "Old/Window/WindowTitle_Flashing", Icon24x24, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) );

#if PLATFORM_MAC
		const FButtonStyle MinimizeButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Normal", FVector2D(20, 20) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Hovered", FVector2D(20, 20) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Pressed", FVector2D(20, 20) ) )
			.SetDisabled( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Disabled", FVector2D(20, 20) ) );

		Set( "Window.Buttons.Minimize.Normal",    new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Normal", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Minimize.Hovered",   new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Hovered", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Minimize.Pressed",   new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Minimize_Pressed", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Minimize.Disabled",  new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Disabled", FVector2D(20, 20) ) );

		const FButtonStyle MaximizeButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Normal", FVector2D(20, 20) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Hovered", FVector2D(20, 20) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Pressed", FVector2D(20, 20) ) )
			.SetDisabled( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Disabled", FVector2D(20, 20) ) );

		Set( "Window.Buttons.Maximize.Normal",    new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Normal", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Maximize.Hovered",   new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Hovered", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Maximize.Pressed",   new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Pressed", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Maximize.Disabled",  new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Disabled", FVector2D(20, 20) ) );

		const FButtonStyle RestoreButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Normal", FVector2D(20, 20) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Hovered", FVector2D(20, 20) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Pressed", FVector2D(20, 20) ) );

		Set( "Window.Buttons.Restore.Normal",     new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Normal", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Restore.Hovered",    new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Hovered", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Restore.Pressed",    new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Maximize_Pressed", FVector2D(20, 20) ) );

		const FButtonStyle CloseButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Normal", FVector2D(20, 20) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Hovered", FVector2D(20, 20) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Pressed", FVector2D(20, 20) ) );

		Set( "Window.Buttons.Close.Normal",       new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Normal", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Close.Hovered",      new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Hovered", FVector2D(20, 20) ) );
		Set( "Window.Buttons.Close.Pressed",      new IMAGE_BRUSH( "Old/Window/WindowButton_Mac_Close_Pressed", FVector2D(20, 20) ) );
#else
		const FButtonStyle MinimizeButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Normal", FVector2D(27, 18) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Hovered", FVector2D(27, 18) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Pressed", FVector2D(27, 18) ) )
			.SetDisabled( IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Disabled", FVector2D(27, 18) ) );

		Set( "Window.Buttons.Minimize.Normal",    new IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Normal", FVector2D(27, 18) ) );
		Set( "Window.Buttons.Minimize.Hovered",   new IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Hovered", FVector2D(27, 18) ) );
		Set( "Window.Buttons.Minimize.Pressed",   new IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Pressed", FVector2D(27, 18) ) );
		Set( "Window.Buttons.Minimize.Disabled",   new IMAGE_BRUSH( "Old/Window/WindowButton_Minimize_Disabled", FVector2D(27, 18) ) );

		const FButtonStyle MaximizeButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Normal", FVector2D(23, 18) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Hovered", FVector2D(23, 18) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Pressed", FVector2D(23, 18) ) )
			.SetDisabled( IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Disabled", FVector2D(23, 18) ) );

		Set( "Window.Buttons.Maximize.Normal",    new IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Normal", FVector2D(23, 18) ) );
		Set( "Window.Buttons.Maximize.Hovered",   new IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Hovered", FVector2D(23, 18) ) );
		Set( "Window.Buttons.Maximize.Pressed",   new IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Pressed", FVector2D(23, 18) ) );
		Set( "Window.Buttons.Maximize.Disabled",   new IMAGE_BRUSH( "Old/Window/WindowButton_Maximize_Disabled", FVector2D(23, 18) ) );

		const FButtonStyle RestoreButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Normal", FVector2D(23, 18) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Hovered", FVector2D(23, 18) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Pressed", FVector2D(23, 18) ) );

		Set( "Window.Buttons.Restore.Normal",     new IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Normal", FVector2D(23, 18) ) );
		Set( "Window.Buttons.Restore.Hovered",    new IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Hovered", FVector2D(23, 18) ) );
		Set( "Window.Buttons.Restore.Pressed",    new IMAGE_BRUSH( "Old/Window/WindowButton_Restore_Pressed", FVector2D(23, 18) ) );	

		const FButtonStyle CloseButtonStyle = FButtonStyle(Button)
			.SetNormal ( IMAGE_BRUSH( "Old/Window/WindowButton_Close_Normal", FVector2D(44, 18) ) )
			.SetHovered( IMAGE_BRUSH( "Old/Window/WindowButton_Close_Hovered", FVector2D(44, 18) ) )
			.SetPressed( IMAGE_BRUSH( "Old/Window/WindowButton_Close_Pressed", FVector2D(44, 18) ) );

		Set( "Window.Buttons.Close.Normal",       new IMAGE_BRUSH( "Old/Window/WindowButton_Close_Normal", FVector2D(44, 18) ) );
		Set( "Window.Buttons.Close.Hovered",      new IMAGE_BRUSH( "Old/Window/WindowButton_Close_Hovered", FVector2D(44, 18) ) );
		Set( "Window.Buttons.Close.Pressed",      new IMAGE_BRUSH( "Old/Window/WindowButton_Close_Pressed", FVector2D(44, 18) ) );
#endif
		
		Set( "Window.Buttons.Minimize", MinimizeButtonStyle );
		Set( "Window.Buttons.Maximize", MaximizeButtonStyle );
		Set( "Window.Buttons.Restore", RestoreButtonStyle );
		Set( "Window.Buttons.Close", CloseButtonStyle );

		// Title Text
		const FTextBlockStyle TitleTextStyle = FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D( 1,1 ) )
			.SetShadowColorAndOpacity( FLinearColor::Black );
		Set( "Window.TitleText", TitleTextStyle );

		Set( "ChildWindow.Background",            new IMAGE_BRUSH( "Common/NoiseBackground", FVector2D(64, 64), FLinearColor::White, ESlateBrushTileType::Both) );

		Set( "Window", FWindowStyle()
			.SetMinimizeButtonStyle( MinimizeButtonStyle )
			.SetMaximizeButtonStyle( MaximizeButtonStyle )
			.SetRestoreButtonStyle( RestoreButtonStyle )
			.SetCloseButtonStyle( CloseButtonStyle )
			.SetTitleTextStyle( TitleTextStyle )
			.SetActiveTitleBrush( IMAGE_BRUSH( "Old/Window/WindowTitle", Icon32x32, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) )
			.SetInactiveTitleBrush( IMAGE_BRUSH( "Old/Window/WindowTitle_Inactive", Icon32x32, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) )
			.SetFlashTitleBrush( IMAGE_BRUSH( "Old/Window/WindowTitle_Flashing", Icon24x24, FLinearColor(1,1,1,1), ESlateBrushTileType::Horizontal  ) )
			.SetOutlineBrush( BORDER_BRUSH( "Old/Window/WindowOutline", FMargin(4.0f/32.0f) ) )
			.SetOutlineColor( FLinearColor(0.1f, 0.1f, 0.1f, 1.0f) )
			.SetBorderBrush( BOX_BRUSH( "Old/Window/WindowBorder", 0.48f ) )
			.SetBackgroundBrush( IMAGE_BRUSH( "Old/Window/WindowBackground", FVector2D(74, 74), FLinearColor::White, ESlateBrushTileType::Both) )
			.SetChildBackgroundBrush( IMAGE_BRUSH( "Common/NoiseBackground", FVector2D(64, 64), FLinearColor::White, ESlateBrushTileType::Both) )
			);
	}
	}
	
void FSlateEditorStyle::FStyle::SetupDockingStyles()
{
#if WITH_EDITOR || IS_PROGRAM
	// Tabs, Docking, Flexible Layout
	{
		// Tab Text
		{
			Set( "Docking.TabFont", FTextBlockStyle(NormalText)
				.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) )
				.SetColorAndOpacity( FLinearColor(0.72f, 0.72f, 0.72f, 1.f) )
				.SetShadowOffset( FVector2D( 1,1 ) )
				.SetShadowColorAndOpacity( FLinearColor::Black )
				);
		}

		{
			// Flash using the selection color for consistency with the rest of the UI scheme
			const FSlateColor& TabFlashColor = SelectionColor;

			const FButtonStyle CloseButton = FButtonStyle()
				.SetNormal ( IMAGE_BRUSH( "/Docking/CloseApp_Normal", Icon16x16 ) )
				.SetPressed( IMAGE_BRUSH( "/Docking/CloseApp_Pressed", Icon16x16 ) )
				.SetHovered( IMAGE_BRUSH( "/Docking/CloseApp_Hovered", Icon16x16 ) );

			// Panel Tab
			// Legacy styles used by other editor specific widgets; see "Docking.Tab" in FCoreStyle for the current tab style
			Set( "Docking.Tab.Normal", new BOX_BRUSH( "/Docking/Tab_Inactive", 4/16.0f ) );
			Set( "Docking.Tab.Active", new BOX_BRUSH( "/Docking/Tab_Active", 4/16.0f ) );
			Set( "Docking.Tab.Foreground", new BOX_BRUSH( "/Docking/Tab_Foreground", 4/16.0f ) );
			Set( "Docking.Tab.Hovered", new BOX_BRUSH( "/Docking/Tab_Hovered", 4/16.0f ) );
			Set( "Docking.Tab.ColorOverlay", new BOX_BRUSH( "/Docking/Tab_ColorOverlay", 4/16.0f ) );
			Set( "Docking.Tab.Padding", FMargin(5, 2, 5, 2) );
			Set( "Docking.Tab.OverlapWidth", -1.0f );
			Set( "Docking.Tab.ContentAreaBrush", new BOX_BRUSH( "/Docking/TabContentArea", FMargin(4/16.0f) ) );
			Set( "Docking.Tab.TabWellBrush", new IMAGE_BRUSH( "/Docking/TabWellSeparator", FVector2D(16,4) ) );
			Set( "Docking.Tab.TabWellPadding", FMargin(0, 0, 4, 0) );
			Set( "Docking.Tab.FlashColor", TabFlashColor );
			Set( "Docking.Tab.CloseButton", CloseButton );

			// App Tab
			// Legacy styles used by other editor specific widgets; see "Docking.MajorTab" in FCoreStyle for the current tab style
			Set( "Docking.MajorTab.Normal", new BOX_BRUSH( "/Docking/AppTab_Inactive", FMargin(24.0f/64.0f, 4/32.0f) ) );
			Set( "Docking.MajorTab.Active", new BOX_BRUSH( "/Docking/AppTab_Active", FMargin(24.0f/64.0f, 4/32.0f) ) );
			Set( "Docking.MajorTab.ColorOverlay", new BOX_BRUSH( "/Docking/AppTab_ColorOverlay", FMargin(24.0f/64.0f, 4/32.0f) ) );
			Set( "Docking.MajorTab.Foreground", new BOX_BRUSH( "/Docking/AppTab_Foreground", FMargin(24.0f/64.0f, 4/32.0f) ) );
			Set( "Docking.MajorTab.Hovered", new BOX_BRUSH( "/Docking/AppTab_Hovered", FMargin(24.0f/64.0f, 4/32.0f) ) );
			Set( "Docking.MajorTab.Padding", FMargin(17, 4, 15, 4) );
			Set( "Docking.MajorTab.OverlapWidth", 21.0f );
			Set( "Docking.MajorTab.ContentAreaBrush", new BOX_BRUSH( "/Docking/AppTabContentArea", FMargin(4/16.0f) ) );
			Set( "Docking.MajorTab.TabWellBrush", new IMAGE_BRUSH( "/Docking/AppTabWellSeparator", FVector2D(16,2) ) );
			Set( "Docking.MajorTab.TabWellPadding", FMargin(0, 2, 0, 0) );
			Set( "Docking.MajorTab.FlashColor", TabFlashColor );
			Set( "Docking.MajorTab.CloseButton", FButtonStyle( CloseButton ) );
		}

		Set( "Docking.DefaultTabIcon", new IMAGE_BRUSH( "Old/Docking/DefaultTabIcon", Icon16x16 ) );

		Set( "Docking.TabConextButton.Normal", new IMAGE_BRUSH( "/Docking/TabContextButton", FVector2D(24,24) ) );
		Set( "Docking.TabConextButton.Pressed", new IMAGE_BRUSH( "/Docking/TabContextButton", FVector2D(24,24) ) );
		Set( "Docking.TabConextButton.Hovered", new IMAGE_BRUSH( "/Docking/TabContextButton", FVector2D(24,24) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM
	}

void FSlateEditorStyle::FStyle::SetupTutorialStyles()
{
	// Documentation tooltip defaults
	{
		const FTextBlockStyle DocumentationTooltipText = FTextBlockStyle(NormalText)
			.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 8));
		Set("Documentation.SDocumentationTooltip", FTextBlockStyle(DocumentationTooltipText));
	}


	// Documentation defaults
	const FTextBlockStyle DocumentationText = FTextBlockStyle(NormalText)
		.SetFont(TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ));

	const FTextBlockStyle DocumentationHyperlinkText = FTextBlockStyle(DocumentationText)
		.SetColorAndOpacity(FLinearColor::Blue);

	const FTextBlockStyle DocumentationHeaderText = FTextBlockStyle(NormalText)
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 32));

	const FButtonStyle DocumentationHyperlink = FButtonStyle()
			.SetNormal(BORDER_BRUSH( "Old/HyperlinkDotted", FMargin(0,0,0,3/16.0f)))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f)));

	// Documentation
	{
		Set( "Documentation.Content", FTextBlockStyle(DocumentationText) );
		Set("Documentation.Hyperlink.Button", FButtonStyle(DocumentationHyperlink));
		Set("Documentation.Hyperlink.Text",   FTextBlockStyle(DocumentationHyperlinkText));
		Set("Documentation.NumberedContent",  FTextBlockStyle(DocumentationText));
		Set( "Documentation.BoldContent", FTextBlockStyle(DocumentationText)
			.SetFontName(RootToCoreContentDir("Fonts/Roboto-Bold", TEXT(".ttf"))));

		Set("Documentation.Header1", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(32));

		Set("Documentation.Header2", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(24));

		Set( "Documentation.Separator", new BOX_BRUSH( "Common/Separator", 1/4.0f, FLinearColor(1,1,1,0.5f) ) );
	}

	{
		Set("Documentation.ToolTip.Background", new BOX_BRUSH("Tutorials/TutorialContentBackground", FMargin(4 / 16.0f)));
	}

	// Tutorials
	{
		const FLinearColor TutorialButtonColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
		const FLinearColor TutorialSelectionColor = FLinearColor(0.19f, 0.33f, 0.72f);

		const FTextBlockStyle TutorialText = FTextBlockStyle(DocumentationText)
			.SetColorAndOpacity(FLinearColor::Black);

		const FTextBlockStyle TutorialHeaderText = FTextBlockStyle(DocumentationHeaderText)
			.SetColorAndOpacity(FLinearColor::Black);

		Set( "Tutorials.ButtonColor", TutorialButtonColor );
		Set( "Tutorials.ButtonHighlightColor", TutorialSelectionColor );
		Set( "Tutorials.ButtonDisabledColor", SelectionColor_Inactive );
		Set( "Tutorials.ContentAreaBackground", new BOX_BRUSH( "Tutorials/TutorialContentBackground", FMargin(4/16.0f) ) );
		Set( "Tutorials.HomeContentAreaBackground", new BOX_BRUSH( "Tutorials/TutorialHomeContentBackground", FMargin(4/16.0f) ) );
		Set( "Tutorials.ContentAreaFrame", new BOX_BRUSH( "Tutorials/ContentAreaFrame", FMargin(26.0f/64.0f) ) );
		Set( "Tutorials.CurrentExcerpt", new IMAGE_BRUSH( "Tutorials/CurrentExcerpt", FVector2D(24.0f, 24.0f), TutorialSelectionColor ) );
		Set( "Tutorials.Home", new IMAGE_BRUSH( "Tutorials/HomeButton", FVector2D(32.0f, 32.0f) ) );
		Set( "Tutorials.Back", new IMAGE_BRUSH("Tutorials/BackButton", FVector2D(24.0f, 24.0f) ) );
		Set( "Tutorials.Next", new IMAGE_BRUSH("Tutorials/NextButton", FVector2D(24.0f, 24.0f) ) );

		Set("Tutorials.PageHeader", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(22));

		Set("Tutorials.CurrentExcerpt", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(16));
 
		Set("Tutorials.NavigationButtons", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(16));

		// UDN documentation styles
		Set("Tutorials.Content",         FTextBlockStyle(TutorialText));
		Set("Tutorials.Hyperlink.Text",  FTextBlockStyle(DocumentationHyperlinkText));
		Set("Tutorials.NumberedContent", FTextBlockStyle(TutorialText));
		Set("Tutorials.BoldContent",     FTextBlockStyle(TutorialText)
			.SetFontName(RootToCoreContentDir("Fonts/Roboto-Bold", TEXT(".ttf"))));

		Set("Tutorials.Header1", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(32));

		Set("Tutorials.Header2", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(24));

		Set( "Tutorials.Hyperlink.Button", FButtonStyle(DocumentationHyperlink)
			.SetNormal(BORDER_BRUSH( "Old/HyperlinkDotted", FMargin(0,0,0,3/16.0f), FLinearColor::Black))
			.SetHovered(BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f), FLinearColor::Black)));

		Set( "Tutorials.Separator", new BOX_BRUSH( "Common/Separator", 1/4.0f, FLinearColor::Black));

		Set( "Tutorials.ProgressBar", FProgressBarStyle()
			.SetBackgroundImage( BOX_BRUSH( "Common/ProgressBar_Background", FMargin(5.f/12.f) ) )
			.SetFillImage( BOX_BRUSH( "Common/ProgressBar_NeutralFill", FMargin(5.f/12.f) ) )
			.SetMarqueeImage( IMAGE_BRUSH( "Common/ProgressBar_Marquee", FVector2D(20,12), FLinearColor::White, ESlateBrushTileType::Horizontal ) )
			);
	}
	}

void FSlateEditorStyle::FStyle::SetupPropertyEditorStyles()
	{
	// Property / details Window / PropertyTable 
	{
		Set( "PropertyEditor.Grid.TabIcon", new IMAGE_BRUSH( "Icons/icon_PropertyMatrix_16px", Icon16x16 ) );
		Set( "PropertyEditor.Properties.TabIcon", new IMAGE_BRUSH( "Icons/lens_16x", Icon16x16 ) );

		Set( "PropertyEditor.RemoveColumn", new IMAGE_BRUSH( "Common/PushPin_Down", Icon16x16, FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );
		Set( "PropertyEditor.AddColumn", new IMAGE_BRUSH( "Common/PushPin_Up", Icon16x16, FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );

		Set( "PropertyEditor.AddColumnOverlay",	new IMAGE_BRUSH( "Common/TinyChalkArrow", FVector2D( 71, 20 ), FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );
		Set( "PropertyEditor.AddColumnMessage", FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensedItalic", 10 ) )
			.SetColorAndOpacity(FColor( 96, 194, 253, 255 ).ReinterpretAsLinear())
		);
	
		Set( "PropertyEditor.AssetName", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1,1) )
			.SetShadowColorAndOpacity( FLinearColor::Black )
		);

		Set( "PropertyEditor.AssetThumbnailShadow", new BOX_BRUSH( "ContentBrowser/ThumbnailShadow", FMargin( 5.0f / 64.0f ) ) );
		Set( "PropertyEditor.AssetClass", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1,1) )
			.SetShadowColorAndOpacity( FLinearColor::Black )
		);

		const FButtonStyle AssetComboStyle = FButtonStyle()
			.SetNormal( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.15f) ) )
			.SetHovered( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.25f) ) )
			.SetPressed( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.30f) ) )
			.SetNormalPadding( FMargin(0,0,0,1) )
			.SetPressedPadding( FMargin(0,1,0,0) );
		Set( "PropertyEditor.AssetComboStyle", AssetComboStyle );

		Set( "PropertyEditor.HorizontalDottedLine",		new IMAGE_BRUSH( "Common/HorizontalDottedLine_16x1px", FVector2D(16.0f, 1.0f), FLinearColor::White, ESlateBrushTileType::Horizontal ) );
		Set( "PropertyEditor.VerticalDottedLine",		new IMAGE_BRUSH( "Common/VerticalDottedLine_1x16px", FVector2D(1.0f, 16.0f), FLinearColor::White, ESlateBrushTileType::Vertical ) );
		Set( "PropertyEditor.SlateBrushPreview",		new BOX_BRUSH( "PropertyView/SlateBrushPreview_32px", Icon32x32, FMargin(3.f/32.f, 3.f/32.f, 15.f/32.f, 13.f/32.f) ) );

		Set( "PropertyTable.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateColorBrush( FLinearColor( 0.70f, 0.70f, 0.70f, 255 ) ) )
			.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetOddRowBackgroundBrush( FSlateColorBrush( FLinearColor( 0.80f, 0.80f, 0.80f, 255 ) ) )
			.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
			.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);

		const FTableColumnHeaderStyle PropertyTableColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortAscendingImage( IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) )
			.SetSortDescendingImage( IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) )
			.SetNormalBrush( BOX_BRUSH( "Common/ColumnHeader", 4.f/32.f ) )
			.SetHoveredBrush( BOX_BRUSH( "Common/ColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );

		const FTableColumnHeaderStyle PropertyTableLastColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortAscendingImage( IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) )
			.SetSortDescendingImage( IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) )
			.SetNormalBrush( FSlateNoResource() )
			.SetHoveredBrush( BOX_BRUSH( "Common/LastColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );

		const FSplitterStyle PropertyTableHeaderSplitterStyle = FSplitterStyle()
			.SetHandleNormalBrush( FSlateNoResource() )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/HeaderSplitterGrip", Icon8x8 ) );

		Set( "PropertyTable.HeaderRow", FHeaderRowStyle()
			.SetColumnStyle( PropertyTableColumnHeaderStyle )
			.SetLastColumnStyle( PropertyTableLastColumnHeaderStyle )
			.SetColumnSplitterStyle( PropertyTableHeaderSplitterStyle )
			.SetBackgroundBrush( BOX_BRUSH( "Common/TableViewHeader", 4.f/32.f ) )
			.SetForegroundColor( DefaultForeground )
			);

		Set( "PropertyTable.Selection.Active",						new IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) );

		Set( "PropertyTable.HeaderRow.Column.PathDelimiter",		new IMAGE_BRUSH( "Common/SmallArrowRight", Icon10x10 ) );

		Set( "PropertyTable.RowHeader.Background",					new BOX_BRUSH( "Old/Menu_Background", FMargin(4.f/64.f) ) );
		Set( "PropertyTable.RowHeader.BackgroundActive",			new BOX_BRUSH( "Old/Menu_Background", FMargin(4.f/64.f), SelectionColor_Inactive ) );
		Set( "PropertyTable.ColumnBorder",							new BOX_BRUSH( "Common/ColumnBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.CellBorder",							new BOX_BRUSH( "Common/CellBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.ReadOnlyEditModeCellBorder",			new BORDER_BRUSH( "Common/ReadOnlyEditModeCellBorder", FMargin(6.f/32.f), SelectionColor ) );
		Set( "PropertyTable.ReadOnlyCellBorder",					new BOX_BRUSH( "Common/ReadOnlyCellBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.CurrentCellBorder",						new BOX_BRUSH( "Common/CurrentCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.ReadOnlySelectedCellBorder",			new BOX_BRUSH( "Common/ReadOnlySelectedCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.ReadOnlyCurrentCellBorder",				new BOX_BRUSH( "Common/ReadOnlyCurrentCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.Cell.DropDown.Background",				new BOX_BRUSH( "Common/GroupBorder", FMargin(4.f/16.f) ) );
		Set( "PropertyTable.ContentBorder",							new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );	
		Set( "PropertyTable.NormalFont",							TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "PropertyTable.BoldFont",								TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) );
		Set( "PropertyTable.FilterFont",							TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );

		Set( "PropertyWindow.FilterSearch", new IMAGE_BRUSH( "Old/FilterSearch", Icon16x16 ) );
		Set( "PropertyWindow.FilterCancel", new IMAGE_BRUSH( "Old/FilterCancel", Icon16x16 ) );
		Set( "PropertyWindow.Favorites_Enabled", new IMAGE_BRUSH( "Icons/Star_16x", Icon16x16 ) );
		Set( "PropertyWindow.Favorites_Disabled", new IMAGE_BRUSH( "Icons/EmptyStar_16x", Icon16x16 ) );
		Set( "PropertyWindow.Locked", new IMAGE_BRUSH( "Icons/padlock_locked_16x", Icon16x16 ) );
		Set( "PropertyWindow.Unlocked", new IMAGE_BRUSH( "Icons/padlock_unlocked_16x", Icon16x16 ) );
		Set( "PropertyWindow.DiffersFromDefault", new IMAGE_BRUSH( "/PropertyView/DiffersFromDefault_8x8", FVector2D(8,8) ) ) ;
		Set( "PropertyWindow.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "PropertyWindow.BoldFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 8 ) );
		Set( "PropertyWindow.ItalicFont", TTF_FONT( "Fonts/Roboto-Italic", 8 ) );
		Set( "PropertyWindow.FilterFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "PropertyWindow.NoOverlayColor", new FSlateNoResource() );
		Set( "PropertyWindow.EditConstColor", new FSlateColorBrush( FColor( 152, 152, 152, 80 ) ) );
		Set( "PropertyWindow.FilteredColor", new FSlateColorBrush( FColor( 0, 255, 0, 80 ) ) );
		Set( "PropertyWindow.FilteredEditConstColor", new FSlateColorBrush( FColor( 152, 152, 152, 80 ).ReinterpretAsLinear() * FColor(0,255,0,255).ReinterpretAsLinear() ) );
		Set( "PropertyWindow.CategoryBackground", new BOX_BRUSH( "/PropertyView/CategoryBackground", FMargin(4.f/16.f) ) );
		Set( "PropertyWindow.CategoryForeground", FLinearColor::Black );
		Set( "PropertyWindow.Button_Browse", new IMAGE_BRUSH( "Icons/lens_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Use", new IMAGE_BRUSH( "Icons/assign_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Delete", new IMAGE_BRUSH("Icons/cross_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Clear", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Edit", new IMAGE_BRUSH( "Icons/wrench_16x", Icon12x12 ) );
		Set( "PropertyWindow.Button_EmptyArray", new IMAGE_BRUSH( "Icons/empty_set_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_AddToArray", new IMAGE_BRUSH( "Icons/PlusSymbol_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_RemoveFromArray", new IMAGE_BRUSH( "Icons/MinusSymbol_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Ellipsis", new IMAGE_BRUSH( "Icons/Ellipsis_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_PickAsset", new IMAGE_BRUSH( "Icons/pillarray_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_PickActor", new IMAGE_BRUSH( "Icons/hiererchy_16x", Icon12x12 ) );
		Set( "PropertyWindow.Button_PickActorInteractive", new IMAGE_BRUSH( "Icons/eyedropper_16px", Icon12x12 ) );

		Set( "PropertyWindow.WindowBorder", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );
		Set( "DetailsView.NameChangeCommitted", new BOX_BRUSH( "Common/EditableTextSelectionBackground", FMargin(4.f/16.f) ) );
		Set( "DetailsView.HyperlinkStyle", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );
		Set( "DetailsView.GroupSection", new BOX_BRUSH( "Common/RoundedSelection_16x", FMargin(4.0f/16.0f) ) );

		Set( "DetailsView.PulldownArrow.Down", new IMAGE_BRUSH( "PropertyView/AdvancedButton_Down", FVector2D(10,8) ) );
		Set( "DetailsView.PulldownArrow.Down.Hovered", new IMAGE_BRUSH( "PropertyView/AdvancedButton_Down_Hovered", FVector2D(10,8) ) );
		Set( "DetailsView.PulldownArrow.Up", new IMAGE_BRUSH( "PropertyView/AdvancedButton_Up", FVector2D(10,8) ) );
		Set( "DetailsView.PulldownArrow.Up.Hovered", new IMAGE_BRUSH( "PropertyView/AdvancedButton_Up_Hovered", FVector2D(10,8) ) );

		Set( "DetailsView.EditRawProperties", new IMAGE_BRUSH( "Icons/icon_PropertyMatrix_16px", Icon16x16, FLinearColor( 0, 0, 0, 255 ) ) );

		Set( "DetailsView.CollapsedCategory", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );
		Set( "DetailsView.CategoryTop", new BOX_BRUSH( "PropertyView/DetailCategoryTop", FMargin( 4/16.0f, 8.0f/16.0f, 4/16.0f, 4/16.0f ) ) );
		Set( "DetailsView.CollapsedCategory_Hovered", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f), FLinearColor(0.5f,0.5f,0.5f,1.0f)  ) );
		Set( "DetailsView.CategoryTop_Hovered", new BOX_BRUSH( "PropertyView/DetailCategoryTop", FMargin( 4/16.0f, 8.0f/16.0f, 4/16.0f, 4/16.0f ), FLinearColor(0.5f,0.5f,0.5f,1.0f) ) );
		Set( "DetailsView.CategoryBottom", new BOX_BRUSH( "PropertyView/DetailCategoryBottom", FMargin(4.0f/16.0f) ) );
		Set( "DetailsView.CategoryMiddle", new IMAGE_BRUSH( "PropertyView/DetailCategoryMiddle", FVector2D( 16, 16 ) ) );

		Set( "DetailsView.Splitter", FSplitterStyle()
			.SetHandleNormalBrush( IMAGE_BRUSH( "Common/SplitterHandleHighlight", Icon8x8, FLinearColor::Black ) )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/SplitterHandleHighlight", Icon8x8, FLinearColor::White ) )
			);

		Set( "DetailsView.AdvancedDropdownBorder", new BOX_BRUSH( "PropertyView/DetailCategoryAdvanced", FMargin(4.0f/16.0f) ) );
		Set( "DetailsView.AdvancedDropdownBorder.Open", new IMAGE_BRUSH( "Common/ScrollBoxShadowTop", FVector2D(64,8) ) );
		Set( "DetailsView.CategoryFontStyle", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );

		Set( "DetailsView.EditBlueprintHyperlinkStyle", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ).SetColorAndOpacity( FLinearColor( 0.25f, 0.5f, 1.0f ) ) );
		Set( "DetailsView.GoToCodeHyperlinkStyle", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ).SetColorAndOpacity( FLinearColor( 0.7f, 0.7f, 0.7f ) ) );

		Set( "DetailsView.TreeView.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateNoResource() )
			.SetEvenRowBackgroundHoveredBrush( FSlateNoResource() )
			.SetOddRowBackgroundBrush( FSlateNoResource() )
			.SetOddRowBackgroundHoveredBrush( FSlateNoResource() )
			.SetSelectorFocusedBrush( FSlateNoResource() )
			.SetActiveBrush( FSlateNoResource() )
			.SetActiveHoveredBrush( FSlateNoResource() )
			.SetInactiveBrush( FSlateNoResource() )
			.SetInactiveHoveredBrush( FSlateNoResource() )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);
	}
	}

void FSlateEditorStyle::FStyle::SetupProfilerStyle()
{
#if WITH_EDITOR || IS_PROGRAM
	// Profiler
	{
		// Profiler group brushes
		Set( "Profiler.Group.16", new BOX_BRUSH( "Icons/Profiler/GroupBorder-16Gray", FMargin(4.0f/16.0f) ) );

		// Profiler toolbar icons
		Set( "Profiler.Tab", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Tab_16x", Icon16x16 ) );
		Set( "Profiler.Tab.GraphView", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Graph_View_Tab_16x", Icon16x16 ) );
		Set( "Profiler.Tab.EventGraph", new IMAGE_BRUSH( "Icons/Profiler/profiler_OpenEventGraph_32x", Icon16x16 ) );
		Set( "Profiler.Tab.FiltersAndPresets", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Filter_Presets_Tab_16x", Icon16x16 ) );

		Set( "ProfilerCommand.ProfilerManager_Load", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Load_Profiler_40x", Icon40x40 ) );
		Set( "ProfilerCommand.ProfilerManager_Load.Small", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Load_Profiler_40x", Icon20x20 ) );

		Set( "ProfilerCommand.Save", new IMAGE_BRUSH( "Icons/LV_Save", Icon40x40 ) );
		Set( "ProfilerCommand.Save.Small", new IMAGE_BRUSH( "Icons/LV_Save", Icon20x20 ) );
		
		Set( "ProfilerCommand.ProfilerManager_ToggleLivePreview", new IMAGE_BRUSH( "Automation/RefreshTests", Icon40x40) );
		Set( "ProfilerCommand.ProfilerManager_ToggleLivePreview.Small", new IMAGE_BRUSH( "Automation/RefreshTests", Icon20x20) );

		Set( "ProfilerCommand.StatsProfiler", new IMAGE_BRUSH( "Icons/Profiler/profiler_stats_40x", Icon40x40 ) );
		Set( "ProfilerCommand.StatsProfiler.Small", new IMAGE_BRUSH( "Icons/Profiler/profiler_stats_40x", Icon20x20 ) );

		Set( "ProfilerCommand.MemoryProfiler", new IMAGE_BRUSH( "Icons/Profiler/profiler_mem_40x", Icon40x40 ) );
		Set( "ProfilerCommand.MemoryProfiler.Small", new IMAGE_BRUSH( "Icons/Profiler/profiler_mem_40x", Icon20x20 ) );

		Set( "ProfilerCommand.FPSChart", new IMAGE_BRUSH( "Icons/Profiler/Profiler_FPS_Chart_40x", Icon40x40 ) );
		Set( "ProfilerCommand.FPSChart.Small", new IMAGE_BRUSH( "Icons/Profiler/Profiler_FPS_Chart_40x", Icon20x20 ) );

		Set( "ProfilerCommand.OpenSettings", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Settings_40x", Icon40x40 ) );
		Set( "ProfilerCommand.OpenSettings.Small", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Settings_40x", Icon20x20 ) );

		Set( "ProfilerCommand.ToggleDataPreview", new IMAGE_BRUSH( "Icons/Profiler/profiler_sync_40x", Icon40x40 ) );
		Set( "ProfilerCommand.ToggleDataPreview.Small", new IMAGE_BRUSH( "Icons/Profiler/profiler_sync_40x", Icon20x20 ) );

		Set( "ProfilerCommand.ToggleDataCapture", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Data_Capture_40x", Icon40x40 ) );
		Set( "ProfilerCommand.ToggleDataCapture.Small", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Data_Capture_40x", Icon20x20 ) );

		Set( "ProfilerCommand.ToggleDataCapture.Checked", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon40x40 ) );
		Set( "ProfilerCommand.ToggleDataCapture.Checked.Small", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon20x20 ) );

		Set( "ProfilerCommand.ToggleShowDataGraph", new IMAGE_BRUSH( "Icons/Profiler/profiler_ShowGraphData_32x", Icon32x32 ) );
		Set( "ProfilerCommand.OpenEventGraph", new IMAGE_BRUSH( "Icons/Profiler/profiler_OpenEventGraph_32x", Icon16x16 ) );

		// Generic
		Set( "Profiler.LineGraphArea", new IMAGE_BRUSH( "Old/White", Icon16x16, FLinearColor(1.0f,1.0f,1.0f,0.25f) ) );
		
		// Tooltip hint icon
		Set( "Profiler.Tooltip.HintIcon10", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Custom_Tooltip_12x", Icon12x12 ) );

		// Text styles
		Set( "Profiler.CaptionBold", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1.0f, 1.0f) )
			.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f,0.5f) )
		);

		Set( "Profiler.Caption", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1.0f, 1.0f) )
			.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f,0.5f) )
		);

		Set( "Profiler.TooltipBold", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 8 ) )
			.SetColorAndOpacity( FLinearColor(0.5f,0.5f,0.5f,1.0f) )
			.SetShadowOffset( FVector2D(1.0f, 1.0f) )
			.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f,0.5f) )
		);

		Set( "Profiler.Tooltip", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1.0f, 1.0f) )
			.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f,0.5f) )
		);

		// Event graph icons
		Set( "Profiler.EventGraph.SetRoot", new IMAGE_BRUSH( "Icons/Profiler/profiler_SetRoot_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.CullEvents", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Cull_Events_16x", Icon16x16) );
		Set( "Profiler.EventGraph.FilterEvents", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Filter_Events_16x", Icon16x16) );

		Set( "Profiler.EventGraph.SelectStack", new IMAGE_BRUSH( "Icons/Profiler/profiler_SelectStack_32x", Icon32x32 ) );

		Set( "Profiler.EventGraph.ExpandAll", new IMAGE_BRUSH( "Icons/Profiler/profiler_ExpandAll_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.CollapseAll", new IMAGE_BRUSH( "Icons/Profiler/profiler_CollapseAll_32x", Icon32x32 ) );
		
		Set( "Profiler.EventGraph.ExpandSelection", new IMAGE_BRUSH( "Icons/Profiler/profiler_ExpandSelection_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.CollapseSelection", new IMAGE_BRUSH( "Icons/Profiler/profiler_CollapseSelection_32x", Icon32x32 ) );

		Set( "Profiler.EventGraph.ExpandThread", new IMAGE_BRUSH( "Icons/Profiler/profiler_ExpandThread_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.CollapseThread", new IMAGE_BRUSH( "Icons/Profiler/profiler_CollapseThread_32x", Icon32x32 ) );

		Set( "Profiler.EventGraph.ExpandHotPath", new IMAGE_BRUSH( "Icons/Profiler/profiler_ExpandHotPath_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.HotPathSmall", new IMAGE_BRUSH( "Icons/Profiler/profiler_HotPath_32x", Icon12x12 ) );

		Set( "Profiler.EventGraph.ExpandHotPath16", new IMAGE_BRUSH( "Icons/Profiler/profiler_HotPath_32x", Icon16x16 ) );

		Set( "Profiler.EventGraph.GameThread", new IMAGE_BRUSH( "Icons/Profiler/profiler_GameThread_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.RenderThread", new IMAGE_BRUSH( "Icons/Profiler/profiler_RenderThread_32x", Icon32x32 ) );
	
		Set( "Profiler.EventGraph.ViewColumn", new IMAGE_BRUSH( "Icons/Profiler/profiler_ViewColumn_32x", Icon32x32 ) );
		Set( "Profiler.EventGraph.ResetColumn", new IMAGE_BRUSH( "Icons/Profiler/profiler_ResetColumn_32x", Icon32x32 ) );

		Set( "Profiler.EventGraph.HistoryBack", new IMAGE_BRUSH( "Icons/Profiler/Profiler_History_Back_16x", Icon16x16) );
		Set( "Profiler.EventGraph.HistoryForward", new IMAGE_BRUSH( "Icons/Profiler/Profiler_History_Fwd_16x", Icon16x16) );

		Set( "Profiler.EventGraph.MaximumIcon", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Max_Event_Graph_16x", Icon16x16) );
		Set( "Profiler.EventGraph.AverageIcon", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Average_Event_Graph_16x", Icon16x16) );

		Set( "Profiler.EventGraph.FlatIcon", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Events_Flat_16x", Icon16x16) );
		Set( "Profiler.EventGraph.FlatCoalescedIcon", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Events_Flat_Coalesced_16x", Icon16x16) );
		Set( "Profiler.EventGraph.HierarchicalIcon", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Events_Hierarchial_16x", Icon16x16) );

		Set( "Profiler.EventGraph.HasCulledEventsSmall", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Has_Culled_Children_12x", Icon12x12) );
		Set( "Profiler.EventGraph.CulledEvent", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Culled_12x", Icon12x12) );
		Set( "Profiler.EventGraph.FilteredEvent", new IMAGE_BRUSH( "Icons/Profiler/Profiler_Filtered_12x", Icon12x12) );

		Set( "Profiler.EventGraph.DarkText", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor::Black )
			.SetShadowOffset( FVector2D(0.0f, 0.0f) )
			);

		// Event graph selected event border
		Set( "Profiler.EventGraph.Border.TB", new BOX_BRUSH( "Icons/Profiler/Profiler_Border_TB_16x", FMargin(4.0f/16.0f) ) );
		Set( "Profiler.EventGraph.Border.L", new BOX_BRUSH( "Icons/Profiler/Profiler_Border_L_16x",   FMargin(4.0f/16.0f) ) );
		Set( "Profiler.EventGraph.Border.R", new BOX_BRUSH( "Icons/Profiler/Profiler_Border_R_16x",   FMargin(4.0f/16.0f) ) );

		// Misc
		Set( "Profiler.Misc.WarningSmall", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon12x12 ) );

		Set( "Profiler.Misc.SortBy", new IMAGE_BRUSH( "Icons/Profiler/profiler_SortBy_32x", Icon32x32 ) );
		Set( "Profiler.Misc.SortAscending", new IMAGE_BRUSH( "Icons/Profiler/profiler_SortAscending_32x", Icon32x32 ) );
		Set( "Profiler.Misc.SortDescending", new IMAGE_BRUSH( "Icons/Profiler/profiler_SortDescending_32x", Icon32x32 ) );

		Set( "Profiler.Misc.ResetToDefault", new IMAGE_BRUSH( "Icons/Profiler/profiler_ResetToDefault_32x", Icon32x32 ) );

		Set( "Profiler.Misc.Save16", new IMAGE_BRUSH( "Icons/LV_Save", Icon16x16 ) );
		Set( "Profiler.Misc.Reset16", new IMAGE_BRUSH( "Icons/Profiler/profiler_ResetToDefault_32x", Icon16x16 ) );

		Set( "Profiler.Type.Calls", new IMAGE_BRUSH( "Icons/Profiler/profiler_Calls_32x", Icon16x16 ) );
		Set( "Profiler.Type.Event", new IMAGE_BRUSH( "Icons/Profiler/profiler_Event_32x", Icon16x16 ) );
		Set( "Profiler.Type.Memory", new IMAGE_BRUSH( "Icons/Profiler/profiler_Memory_32x", Icon16x16 ) );
		Set( "Profiler.Type.Number", new IMAGE_BRUSH( "Icons/Profiler/profiler_Number_32x", Icon16x16 ) );

		// NumberInt, NumberFloat, Memory, Hierarchical
		Set( "Profiler.Type.NumberInt", new IMAGE_BRUSH( "Icons/Profiler/profiler_Number_32x", Icon16x16 ) );
		Set( "Profiler.Type.NumberFloat", new IMAGE_BRUSH( "Icons/Profiler/profiler_Number_32x", Icon16x16 ) );
		Set( "Profiler.Type.Memory", new IMAGE_BRUSH( "Icons/Profiler/profiler_Memory_32x", Icon16x16 ) );
		Set( "Profiler.Type.Hierarchical", new IMAGE_BRUSH( "Icons/Profiler/profiler_Event_32x", Icon16x16 ) );

		Set( "Profiler.Misc.GenericFilter", new IMAGE_BRUSH( "Icons/Profiler/profiler_GenericFilter_32x", Icon16x16 ) );
		Set( "Profiler.Misc.GenericGroup", new IMAGE_BRUSH( "Icons/Profiler/profiler_GenericGroup_32x", Icon16x16 ) );
		Set( "Profiler.Misc.CopyToClipboard", new IMAGE_BRUSH( "Icons/Profiler/profiler_CopyToClipboard_32x", Icon32x32 ) );
	
		Set( "Profiler.Misc.Disconnect", new IMAGE_BRUSH( "Icons/Profiler/profiler_Disconnect_32x", Icon32x32 ) );

		//Set( "Profiler.Type.Calls", new IMAGE_BRUSH( "Icons/Profiler/profiler_Calls_32x", Icon40x40) );
		//Set( "Profiler.Type.Calls.Small", new IMAGE_BRUSH( "Icons/Profiler/profiler_Calls_32x", Icon20x20) );
	}
#endif // WITH_EDITOR || IS_PROGRAM
}
	
void FSlateEditorStyle::FStyle::SetupGraphEditorStyles()
{
	// Graph Editor
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "Graph.ForegroundColor", FLinearColor(218.0f/255.0f, 218.0f/255.0f, 218.0f/255.0f, 1.0f) );

		Set( "Graph.TitleBackground", new BOX_BRUSH( "Old/Graph/GraphTitleBackground", FMargin(0) ) );
		Set( "Graph.Shadow", new BOX_BRUSH( "Old/Window/WindowBorder", 0.48f ) );
		Set( "Graph.Arrow", new IMAGE_BRUSH( "Old/Graph/Arrow", Icon16x16 ) );
		Set( "Graph.ExecutionBubble", new IMAGE_BRUSH( "Old/Graph/ExecutionBubble", Icon16x16 ) );

		Set( "Graph.PlayInEditor", new BOX_BRUSH( "/Graph/RegularNode_shadow_selected", FMargin(18.0f/64.0f) ) );

		Set( "Graph.Panel.SolidBackground", new IMAGE_BRUSH( "/Graph/GraphPanel_SolidBackground", FVector2D(16, 16), FLinearColor::White, ESlateBrushTileType::Both) );
		Set( "Graph.Panel.GridLineColor",   FLinearColor(0.035f, 0.035f, 0.035f) );
		Set( "Graph.Panel.GridRuleColor",   FLinearColor(0.008f, 0.008f, 0.008f) );
		Set( "Graph.Panel.GridCenterColor", FLinearColor(0.000f, 0.000f, 0.000f) );
		
		Set( "Graph.Panel.GridRulePeriod", 8.0f ); // should be a strictly positive integral value

		Set( "Graph.Node.Separator", new IMAGE_BRUSH( "Old/Graph/NodeVerticalSeparator", FVector2D(8,8) ) );
		Set( "Graph.Node.TitleBackground", new BOX_BRUSH( "Old/Graph/NodeTitleBackground", FMargin(12.0f/64) ) );
		Set( "Graph.Node.NodeBackground", new BOX_BRUSH( "Old/Graph/NodeBackground", FMargin(12.0f/64) ) );

		Set( "Graph.Node.Body", new BOX_BRUSH( "/Graph/RegularNode_body", FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ) );
		Set( "Graph.Node.TitleGloss", new BOX_BRUSH( "/Graph/RegularNode_title_gloss", FMargin(12.0f/64.0f) ) );
		Set( "Graph.Node.ColorSpill", new BOX_BRUSH( "/Graph/RegularNode_color_spill", FMargin(8.0f/64.0f, 3.0f/32.0f, 0, 0) ) );
		Set( "Graph.Node.TitleHighlight", new BOX_BRUSH( "/Graph/RegularNode_title_highlight", FMargin(16.0f/64.0f, 1.0f, 16.0f/64.0f, 0.0f) ) );

		Set( "Graph.Node.ShadowSize", FVector2D(12,12) );
		Set( "Graph.Node.ShadowSelected", new BOX_BRUSH( "/Graph/RegularNode_shadow_selected", FMargin(18.0f/64.0f) ) );
		Set( "Graph.Node.Shadow", new BOX_BRUSH( "/Graph/RegularNode_shadow", FMargin(18.0f/64.0f) ) );

		Set( "Graph.CompactNode.Body", new BOX_BRUSH( "/Graph/MathNode_body", FMargin(4.0f/64.0f, 26.0f/64.0f, 4.0f/64.0f, 4.0f/64.0f) ) );
		Set( "Graph.CompactNode.ShadowSelected", new BOX_BRUSH( "/Graph/MathNode_shadow_selected", FMargin(18.0f/64.0f) ) );
		

		Set( "Graph.Node.CommentBubble", new BOX_BRUSH( "Old/Graph/CommentBubble", FMargin(8/32.0f) ) );
		Set( "Graph.Node.CommentArrow", new IMAGE_BRUSH( "Old/Graph/CommentBubbleArrow", FVector2D(8,8) ) );
		Set( "Graph.Node.CommentFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "Graph.Node.Comment.TextColor", FLinearColor::Black );
		Set( "Graph.Node.Comment.BubblePadding", FVector2D(3,3) );

		Set( "Graph.VarNode.Body", new BOX_BRUSH( "/Graph/VarNode_body", FMargin(16.f/64.f, 12.f/28.f) ) );
		Set( "Graph.VarNode.ColorSpill", new IMAGE_BRUSH( "/Graph/VarNode_color_spill", FVector2D(132,28) ) );
		Set( "Graph.VarNode.Gloss", new BOX_BRUSH( "/Graph/VarNode_gloss", FMargin(16.f/64.f, 16.f/28.f, 16.f/64.f, 4.f/28.f) ) );
		
		Set( "Graph.VarNode.ShadowSelected", new BOX_BRUSH( "/Graph/VarNode_shadow_selected", FMargin(26.0f/64.0f) ) );
		Set( "Graph.VarNode.Shadow", new BOX_BRUSH( "/Graph/VarNode_shadow", FMargin(26.0f/64.0f) ) );

		Set( "Graph.CollapsedNode.Body", new BOX_BRUSH( "/Graph/RegularNode_body", FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ) );
		Set( "Graph.CollapsedNode.BodyColorSpill", new BOX_BRUSH( "/Graph/CollapsedNode_Body_ColorSpill", FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ) );

		{
			// State or conduit node
			{
				Set( "Graph.StateNode.Body", new BOX_BRUSH( "/Persona/StateMachineEditor/StateNode_Node_Body", FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ) );
				Set( "Graph.StateNode.ColorSpill", new BOX_BRUSH( "/Persona/StateMachineEditor/StateNode_Node_ColorSpill", FMargin(4.0f/64.0f, 4.0f/32.0f) ) );

				Set( "Graph.StateNode.Icon", new IMAGE_BRUSH( "/Persona/StateMachineEditor/State_Node_Icon_32x", Icon16x16 ) );
				Set( "Graph.ConduitNode.Icon", new IMAGE_BRUSH( "/Persona/StateMachineEditor/Conduit_Node_Icon_32x", Icon16x16 ) );

				Set( "Graph.StateNode.Pin.BackgroundHovered", new BOX_BRUSH( "/Persona/StateMachineEditor/StateNode_Pin_HoverCue", FMargin(12.0f/64.0f,12.0f/64.0f,12.0f/64.0f,12.0f/64.0f)));
				Set( "Graph.StateNode.Pin.Background", new FSlateNoResource() );
			}

			{
				FTextBlockStyle GraphStateNodeTitle = FTextBlockStyle(NormalText)
					.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 14 ) )
					.SetColorAndOpacity( FLinearColor(230.0f/255.0f,230.0f/255.0f,230.0f/255.0f) )
					.SetShadowOffset( FVector2D( 2,2 ) )
					.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) );
				Set( "Graph.StateNode.NodeTitle", GraphStateNodeTitle );

				FEditableTextBoxStyle GraphStateNodeTitleEditableText = FEditableTextBoxStyle()
					.SetFont(NormalText.Font)
					.SetBackgroundImageNormal( BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
				Set( "Graph.StateNode.NodeTitleEditableText", GraphStateNodeTitleEditableText );

				Set( "Graph.StateNode.NodeTitleInlineEditableText", FInlineEditableTextBlockStyle()
					.SetTextStyle(GraphStateNodeTitle)
					.SetEditableTextBoxStyle(GraphStateNodeTitleEditableText)
					);
			}

			// Transition node
			{
				FMargin TestMargin(16.f/64.f, 16.f/28.f, 16.f/64.f, 4.f/28.f);
				Set( "Graph.TransitionNode.Body", new BOX_BRUSH( "/Persona/StateMachineEditor/Trans_Node_Body", FMargin(16.f/64.f, 12.f/28.f) ) );
				Set( "Graph.TransitionNode.ColorSpill", new BOX_BRUSH( "/Persona/StateMachineEditor/Trans_Node_ColorSpill", TestMargin ) );
				Set( "Graph.TransitionNode.Gloss", new BOX_BRUSH( "/Persona/StateMachineEditor/Trans_Node_Gloss", TestMargin) );
				Set( "Graph.TransitionNode.Icon", new IMAGE_BRUSH( "/Persona/StateMachineEditor/Trans_Node_Icon", FVector2D(25,25) ) );
			}

			// Transition rule tooltip name 
			{
				Set( "Graph.TransitionNode.TooltipName", FTextBlockStyle(NormalText)
					.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) )
					.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,218.0f/255.0f) )
					.SetShadowOffset( FVector2D(1.0f, 1.0f) )
					.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) )
				);
			}

			// Transition rule tooltip caption
			{
				Set( "Graph.TransitionNode.TooltipRule", FTextBlockStyle(NormalText)
					.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 8 ) )
					.SetColorAndOpacity( FLinearColor(180.0f/255.0f,180.0f/255.0f,180.0f/255.0f) )
					.SetShadowOffset( FVector2D(1.0f, 1.0f) )
					.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) )
				);
			}
		}

		// Behavior Tree Editor
		{
			Set( "BTEditor.Graph.BTNode.Body", new BOX_BRUSH( "/BehaviorTree/BTNode_ColorSpill", FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ) );
			Set( "BTEditor.Graph.BTNode.Icon", new IMAGE_BRUSH( "/Persona/StateMachineEditor/State_Node_Icon_32x", Icon16x16 ) );

			Set( "BTEditor.Graph.BTNode.Composite.Icon", new IMAGE_BRUSH( "/BehaviorTree/Composite_Node_Icon_32x", Icon16x16 ) );
			Set( "BTEditor.Graph.BTNode.CompositeDecorator.Icon", new IMAGE_BRUSH( "/BehaviorTree/CompositeDecorator_Node_Icon_32x", Icon16x16 ) );
			Set( "BTEditor.Graph.BTNode.Decorator.Icon", new IMAGE_BRUSH( "/BehaviorTree/Decorator_Node_Icon_32x", Icon16x16 ) );
			Set( "BTEditor.Graph.BTNode.Root.Icon", new IMAGE_BRUSH( "/BehaviorTree/Root_Node_Icon_32x", Icon16x16 ) );
			Set( "BTEditor.Graph.BTNode.Service.Icon", new IMAGE_BRUSH( "/BehaviorTree/Service_Node_Icon_32x", Icon16x16 ) );
			Set( "BTEditor.Graph.BTNode.Task.Icon", new IMAGE_BRUSH( "/BehaviorTree/Task_Node_Icon_32x", Icon16x16 ) );

			Set( "BTEditor.Graph.FlowControl.Icon", new IMAGE_BRUSH( "/Persona/StateMachineEditor/Conduit_Node_Icon_32x", Icon16x16 ) );

			Set( "BTEditor.Debugger.ShowPrevStep", new IMAGE_BRUSH( "Icons/icon_step_back_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.ShowPrevStep.Small", new IMAGE_BRUSH( "Icons/icon_step_back_40x", Icon20x20 ) );
			Set( "BTEditor.Debugger.ShowNextStep", new IMAGE_BRUSH( "Icons/icon_step_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.ShowNextStep.Small", new IMAGE_BRUSH( "Icons/icon_step_40x", Icon20x20 ) );
			Set( "BTEditor.Debugger.Step", new IMAGE_BRUSH( "Icons/icon_findnode_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.Step.Small", new IMAGE_BRUSH( "Icons/icon_findnode_40x", Icon20x20 ) );

			Set( "BTEditor.Debugger.PausePlaySession", new IMAGE_BRUSH( "Icons/icon_pause_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.PausePlaySession.Small", new IMAGE_BRUSH( "Icons/icon_pause_40x", Icon20x20 ) );
			Set( "BTEditor.Debugger.ResumePlaySession", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.ResumePlaySession.Small", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon20x20 ) );
			Set( "BTEditor.Debugger.StopPlaySession", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon40x40 ) );
			Set( "BTEditor.Debugger.StopPlaySession.Small", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon20x20 ) );

			Set( "BTEditor.DebuggerOverlay.Breakpoint.Disabled", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Disabled", Icon32x32 ) );
			Set( "BTEditor.DebuggerOverlay.Breakpoint.Enabled", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Valid", Icon32x32 ) );
			Set( "BTEditor.DebuggerOverlay.ActiveNodePointer", new IMAGE_BRUSH( "Old/Kismet2/IP_Normal", FVector2D(128,96)) );
			Set( "BTEditor.DebuggerOverlay.SearchTriggerPointer", new IMAGE_BRUSH( "/BehaviorTree/SearchTriggerPointer", FVector2D(48,64)) );
			Set( "BTEditor.DebuggerOverlay.FailedTriggerPointer", new IMAGE_BRUSH( "/BehaviorTree/FailedTriggerPointer", FVector2D(48,64)) );
			Set( "BTEditor.DebuggerOverlay.BreakOnBreakpointPointer", new IMAGE_BRUSH( "Old/Kismet2/IP_Breakpoint", FVector2D(128,96)) );

		}

		// Visible on hover button for transition node
		{
			Set( "TransitionNodeButton.Normal", new FSlateNoResource() );
			Set( "TransitionNodeButton.Hovered", new IMAGE_BRUSH( "/Persona/StateMachineEditor/Trans_Button_Hovered", FVector2D(12,25) ) );
			Set( "TransitionNodeButton.Pressed", new IMAGE_BRUSH( "/Persona/StateMachineEditor/Trans_Button_Pressed", FVector2D(12,25) ) );
		}

		{
			Set( "Graph.AnimationResultNode.Body", new IMAGE_BRUSH( "/Graph/Animation/AnimationNode_Result_128x", FVector2D(128, 128) ) );
		}

		// SoundCueEditor Graph Nodes
		{
			Set( "Graph.SoundResultNode.Body", new IMAGE_BRUSH( "/Graph/SoundCue_SpeakerIcon", FVector2D(144, 144) ) );
		}

		Set( "Graph.Node.NodeEntryTop", new IMAGE_BRUSH( "Old/Graph/NodeEntryTop", FVector2D(64,12) ) );
		Set( "Graph.Node.NodeEntryBottom", new IMAGE_BRUSH( "Old/Graph/NodeEntryBottom", FVector2D(64,12) ) );
		Set( "Graph.Node.NodeExitTop", new IMAGE_BRUSH( "Old/Graph/NodeExitTop", FVector2D(64,12) ) );
		Set( "Graph.Node.NodeExitBottom", new IMAGE_BRUSH( "Old/Graph/NodeExitBottom", FVector2D(64,12) ) );

		Set( "Graph.Node.NodeEntryShadow", new BOX_BRUSH( "Old/Graph/NodeEntryShadow", FMargin(5.f/80, 21.f/52) ) );
		Set( "Graph.Node.NodeEntryShadowSelected", new BOX_BRUSH( "Old/Graph/NodeEntryShadowSelected", FMargin(5.f/80, 21.f/52) ) );
		Set( "Graph.Node.NodeExitShadow", new BOX_BRUSH( "Old/Graph/NodeExitShadow", FMargin(5.f/80, 21.f/52) ) );
		Set( "Graph.Node.NodeExitShadowSelected", new BOX_BRUSH( "Old/Graph/NodeExitShadowSelected", FMargin(5.f/80, 21.f/52) ) );

		Set( "Graph.Node.Autoplay", new IMAGE_BRUSH( "Graph/Icons/Overlay_Autoplay", FVector2D(22,22) ) );
		Set( "Graph.Node.Loop", new IMAGE_BRUSH( "Graph/Icons/Overlay_Loop", FVector2D(22,22) ) );

		{
			FTextBlockStyle GraphNodeTitle = FTextBlockStyle(NormalText)
				.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,218.0f/255.0f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) );
			Set( "Graph.Node.NodeTitle", GraphNodeTitle );

			FEditableTextBoxStyle GraphNodeTitleEditableText = FEditableTextBoxStyle()
				.SetFont(NormalText.Font)
				.SetBackgroundImageNormal( BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
			Set( "Graph.Node.NodeTitleEditableText", GraphNodeTitleEditableText );

			Set( "Graph.Node.NodeTitleInlineEditableText", FInlineEditableTextBlockStyle()
				.SetTextStyle(GraphNodeTitle)
				.SetEditableTextBoxStyle(GraphNodeTitleEditableText)
			);

			Set( "Graph.Node.NodeTitleExtraLines", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-Italic", 9 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,96.0f/255.0f, 0.5f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) )
			);
		
			FTextBlockStyle GraphCommentBlockTitle = FTextBlockStyle(NormalText)
				.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 18 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,218.0f/255.0f) )
				.SetShadowOffset( FVector2D(1.5f, 1.5f) )
				.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) );
			Set( "Graph.CommentBlock.Title", GraphCommentBlockTitle );

			FEditableTextBoxStyle GraphCommentBlockTitleEditableText = FEditableTextBoxStyle()
				.SetFont(GraphCommentBlockTitle.Font)
				.SetBackgroundImageNormal( BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
				.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
			Set( "Graph.CommentBlock.TitleEditableText", GraphCommentBlockTitleEditableText );

			Set( "Graph.CommentBlock.TitleInlineEditableText", FInlineEditableTextBlockStyle()
				.SetTextStyle(GraphCommentBlockTitle)
				.SetEditableTextBoxStyle(GraphCommentBlockTitleEditableText)
				);

			Set( "Graph.CompactNode.Title", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 40 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f, 218.0f/255.0f, 218.0f/255.0f, 0.25f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor::White )
			);

			Set( "Graph.ArrayCompactNode.Title", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 20 ) )
				.SetColorAndOpacity( FLinearColor(1.0f, 1.0f, 1.0f, 0.5f) ) //218.0f/255.0f, 218.0f/255.0f, 218.0f/255.0f, 0.25f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor::White )
				);

			Set( "Graph.Node.PinName", FTextBlockStyle(NormalText)
				.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,218.0f/255.0f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor(0.8f,0.8f,0.8f, 0.5) )
			);
		}

		{
			const FLinearColor BrighterColor(1.0f, 1.0f, 1.0f, 0.4f);
			const FLinearColor DarkerColor(0.8f, 0.8f, 0.8f, 0.4f);
			const float MarginSize = 9.0f/16.0f;

			/* Set states for various SCheckBox images ... */
			const FCheckBoxStyle GraphNodeAdvancedViewCheckBoxStyle = FCheckBoxStyle()
				.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
				.SetUncheckedImage( FSlateNoResource() )
				.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", MarginSize, DarkerColor ) )
				.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", MarginSize, BrighterColor ) )
				.SetCheckedImage( FSlateNoResource() )
				.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", MarginSize, DarkerColor ) )
				.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", MarginSize, BrighterColor ) );
			/* ... and add new style */
			Set( "Graph.Node.AdvancedView", GraphNodeAdvancedViewCheckBoxStyle );
		}

		// Special style for switch statements default pin label
		{
			Set( "Graph.Node.DefaultPinName", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-Italic", 9 ) )
				.SetColorAndOpacity( FLinearColor(218.0f/255.0f,218.0f/255.0f,218.0f/255.0f) )
				.SetShadowOffset( FVector2D::ZeroVector )
				.SetShadowColorAndOpacity( FLinearColor(0.8f,0.8f,0.8f, 0.5) )
			);
		}
		Set( "Graph.Pin.DefaultPinSeparator", new IMAGE_BRUSH( "/Graph/PinSeparator", FVector2D(64,8) ) );

		/** Original Pin Styles */
		Set( "Graph.Pin.Connected", new IMAGE_BRUSH( "/Graph/Pin_connected", FVector2D(11,11) ) );
		Set( "Graph.Pin.Disconnected", new IMAGE_BRUSH( "/Graph/Pin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.ArrayPin.Connected", new IMAGE_BRUSH( "/Graph/ArrayPin_connected", FVector2D(11,11) ) );
		Set( "Graph.ArrayPin.Disconnected", new IMAGE_BRUSH( "/Graph/ArrayPin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.RefPin.Connected", new IMAGE_BRUSH( "/Graph/RefPin_connected", FVector2D(11,11) ) );
		Set( "Graph.RefPin.Disconnected", new IMAGE_BRUSH( "/Graph/RefPin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.Pin.ConnectedHovered", new IMAGE_BRUSH( "/Graph/Pin_connected", FVector2D(11,11) ) );
		Set( "Graph.Pin.DisconnectedHovered", new IMAGE_BRUSH( "/Graph/Pin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.ArrayPin.ConnectedHovered", new IMAGE_BRUSH( "/Graph/ArrayPin_connected", FVector2D(11,11) ) );
		Set( "Graph.ArrayPin.DisconnectedHovered", new IMAGE_BRUSH( "/Graph/ArrayPin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.RefPin.ConnectedHovered", new IMAGE_BRUSH( "/Graph/RefPin_connected", FVector2D(11,11) ) );
		Set( "Graph.RefPin.DisconnectedHovered", new IMAGE_BRUSH( "/Graph/RefPin_disconnected", FVector2D(11,11) ) );

		/** Variant A Pin Styles */
		Set( "Graph.Pin.Connected_VarA", new IMAGE_BRUSH( "/Graph/Pin_connected_VarA", FVector2D(15,11)) );
		Set( "Graph.Pin.Disconnected_VarA", new IMAGE_BRUSH( "/Graph/Pin_disconnected_VarA", FVector2D(15,11)) );
		Set( "Graph.Pin.ConnectedHovered_VarA", new IMAGE_BRUSH( "/Graph/Pin_connected_VarA", FVector2D(15,11) ) );
		Set( "Graph.Pin.DisconnectedHovered_VarA", new IMAGE_BRUSH( "/Graph/Pin_disconnected_VarA", FVector2D(15,11) ) );

		Set( "Graph.DelegatePin.Connected", new IMAGE_BRUSH( "/Graph/DelegatePin_connected", FVector2D(11,11) ) );
		Set( "Graph.DelegatePin.Disconnected", new IMAGE_BRUSH( "/Graph/DelegatePin_disconnected", FVector2D(11,11) ) );
		Set( "Graph.DelegatePin.ConnectedHovered", new IMAGE_BRUSH( "/Graph/DelegatePin_connected", FVector2D(11,11) ) );
		Set( "Graph.DelegatePin.DisconnectedHovered", new IMAGE_BRUSH( "/Graph/DelegatePin_disconnected", FVector2D(11,11) ) );

		Set( "Graph.Replication.AuthorityOnly", new IMAGE_BRUSH( "/Graph/AuthorityOnly", FVector2D(32,32) ) );
		Set( "Graph.Replication.ClientEvent", new IMAGE_BRUSH( "/Graph/ClientEvent", FVector2D(32,32) ) );
		Set( "Graph.Replication.Replicated", new IMAGE_BRUSH( "/Graph/Replicated", FVector2D(32,32) ) );

		Set( "Graph.Event.InterfaceEventIcon", new IMAGE_BRUSH("/Graph/InterfaceEventIcon", FVector2D(32,32) ) );

		Set( "Graph.Latent.LatentIcon", new IMAGE_BRUSH("/Graph/LatentIcon", FVector2D(32,32) ) );
		Set( "Graph.Message.MessageIcon", new IMAGE_BRUSH("/Graph/MessageIcon", FVector2D(32,32) ) );

		Set( "Graph.ExecPin.Connected", new IMAGE_BRUSH( "Old/Graph/ExecPin_Connected", Icon12x16 ) );
		Set( "Graph.ExecPin.Disconnected", new IMAGE_BRUSH( "Old/Graph/ExecPin_Disconnected", Icon12x16 ) );
		Set( "Graph.ExecPin.ConnectedHovered", new IMAGE_BRUSH( "Old/Graph/ExecPin_Connected", Icon12x16, FLinearColor(0.8f,0.8f,0.8f) ) );
		Set( "Graph.ExecPin.DisconnectedHovered", new IMAGE_BRUSH( "Old/Graph/ExecPin_Disconnected", Icon12x16, FLinearColor(0.8f,0.8f,0.8f) ) );

		const FVector2D Icon15x28(15.0f, 28.0f);
		Set("Graph.PosePin.Connected", new IMAGE_BRUSH("Graph/Animation/PosePin_Connected_15x28", Icon15x28));
		Set("Graph.PosePin.Disconnected", new IMAGE_BRUSH("Graph/Animation/PosePin_Disconnected_15x28", Icon15x28));
		Set("Graph.PosePin.ConnectedHovered", new IMAGE_BRUSH("Graph/Animation/PosePin_Connected_15x28", Icon15x28, FLinearColor(0.8f, 0.8f, 0.8f)));
		Set("Graph.PosePin.DisconnectedHovered", new IMAGE_BRUSH("Graph/Animation/PosePin_Disconnected_15x28", Icon15x28, FLinearColor(0.8f, 0.8f, 0.8f)));

		// Events Exec Pins
		Set( "Graph.ExecEventPin.Connected", new IMAGE_BRUSH( "Graph/EventPin_Connected", Icon16x16 ) );
		Set( "Graph.ExecEventPin.Disconnected", new IMAGE_BRUSH( "Graph/EventPin_Disconnected", Icon16x16 ) );
		Set( "Graph.ExecEventPin.ConnectedHovered", new IMAGE_BRUSH( "Graph/EventPin_Connected", Icon16x16, FLinearColor(0.8f,0.8f,0.8f) ) );
		Set( "Graph.ExecEventPin.DisconnectedHovered", new IMAGE_BRUSH( "Graph/EventPin_Disconnected", Icon16x16, FLinearColor(0.8f,0.8f,0.8f) ) );

		Set( "Graph.WatchedPinIcon_Pinned", new IMAGE_BRUSH( "Old/Graph/WatchedPinIcon_Pinned", Icon16x16 ) );

		Set( "Graph.Pin.BackgroundHovered", new IMAGE_BRUSH( "/Graph/Pin_hover_cue", FVector2D(32,8)));
		Set( "Graph.Pin.Background", new FSlateNoResource() );

		Set( "Graph.Pin.ObjectSet", new IMAGE_BRUSH( "Old/Graph/Pin_ObjectSet", Icon12x12 ) );
		Set( "Graph.Pin.ObjectEmpty", new IMAGE_BRUSH( "Old/Graph/Pin_ObjectEmpty", Icon12x12 ) );

		Set( "Graph.ConnectorFeedback.Border", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
		Set( "Graph.ConnectorFeedback.OK", new IMAGE_BRUSH( "Old/Graph/Feedback_OK", Icon16x16 ) );
		Set( "Graph.ConnectorFeedback.OKWarn", new IMAGE_BRUSH( "Old/Graph/Feedback_OKWarn", Icon16x16 ) );
		Set( "Graph.ConnectorFeedback.Error", new IMAGE_BRUSH( "Old/Graph/Feedback_Error", Icon16x16 ) );
		Set( "Graph.ConnectorFeedback.NewNode", new IMAGE_BRUSH( "Old/Graph/Feedback_NewNode", Icon16x16 ) );
		Set( "Graph.ConnectorFeedback.ViaCast", new IMAGE_BRUSH( "Old/Graph/Feedback_ConnectViaCast", Icon16x16 ) );
		Set( "Graph.ConnectorFeedback.ShowNode", new IMAGE_BRUSH( "Graph/Feedback_ShowNode", Icon16x16 ) );

		{
			Set( "Graph.CornerText", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 48 ) )
				.SetColorAndOpacity( FLinearColor(0.8, 0.8f, 0.8f, 0.2f) )
				.SetShadowOffset( FVector2D::ZeroVector )
			);

			Set( "Graph.SimulatingText", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 48 ) )
				.SetColorAndOpacity( FLinearColor(0.8, 0.8f, 0.0f, 0.2f) )
				.SetShadowOffset( FVector2D::ZeroVector )
			);

			Set( "GraphPreview.CornerText", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 16 ) )
				.SetColorAndOpacity( FLinearColor(0.8, 0.8f, 0.8f, 0.2f) )
				.SetShadowOffset( FVector2D::ZeroVector )
			);
		}

		{
			Set( "Graph.ZoomText", FTextBlockStyle(NormalText)
				.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 16 ) )
			);
		}

		Set( "GraphEditor.Default_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Node_16x", Icon16x16));
		Set( "GraphEditor.EventGraph_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_EventGraph_16x", Icon16x16 ) );
		Set( "GraphEditor.InterfaceFunction_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Interfacefunction_16x", Icon16x16 ) );
		Set( "GraphEditor.Macro_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Macro_16x", Icon16x16 ) );
		Set( "GraphEditor.Function_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_NewFunction_16x", Icon16x16 ) );
		Set( "GraphEditor.PotentialOverrideFunction_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_OverrideableFunction_16x", Icon16x16 ) );
		Set( "GraphEditor.OverrideFunction_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_OverrideFunction_16x", Icon16x16 ) );
		Set( "GraphEditor.SubGraph_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_SubgraphComposite_16x", Icon16x16 ) );
		Set( "GraphEditor.Animation_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Anim_16x", Icon16x16 ) );
		Set( "GraphEditor.Conduit_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Conduit_16x", Icon16x16 ) );
		Set( "GraphEditor.Rule_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Rule_16x", Icon16x16 ) );
		Set( "GraphEditor.State_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_State_16x", Icon16x16 ) );
		Set( "GraphEditor.StateMachine_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_StateMachine_16x", Icon16x16 ) );
		Set( "GraphEditor.Event_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Event_16x", Icon16x16 ) );
		Set( "GraphEditor.CustomEvent_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_CustomEvent_16x", Icon16x16 ) );
		Set( "GraphEditor.Timeline_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Timeline_16x", Icon16x16));
		Set( "GraphEditor.Comment_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Comment_16x", Icon16x16));
		Set( "GraphEditor.Switch_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Switch_16x", Icon16x16));
		Set( "GraphEditor.BreakStruct_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_BreakStruct_16x", Icon16x16));
		Set( "GraphEditor.MakeStruct_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_MakeStruct_16x", Icon16x16));
		Set( "GraphEditor.Sequence_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Sequence_16x", Icon16x16));
		Set( "GraphEditor.Branch_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Branch_16x", Icon16x16));
		Set( "GraphEditor.SpawnActor_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_SpawnActor_16x", Icon16x16));
		Set( "GraphEditor.PadEvent_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_PadEvent_16x", Icon16x16));
		Set( "GraphEditor.MouseEvent_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_MouseEvent_16x", Icon16x16));
		Set( "GraphEditor.KeyEvent_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_KeyboardEvent_16x", Icon16x16));
		Set( "GraphEditor.TouchEvent_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_TouchEvent_16x", Icon16x16));
		Set( "GraphEditor.MakeArray_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_MakeArray_16x", Icon16x16));
		Set( "GraphEditor.Enum_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Enum_16x", Icon16x16));
		Set( "GraphEditor.Select_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Select_16x", Icon16x16));
		Set( "GraphEditor.Cast_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Cast_16x", Icon16x16));

		Set( "GraphEditor.Macro.Loop_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Loop_16x", Icon16x16));
		Set( "GraphEditor.Macro.Gate_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_Gate_16x", Icon16x16));
		Set( "GraphEditor.Macro.DoN_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_DoN_16x", Icon16x16));
		Set( "GraphEditor.Macro.DoOnce_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_DoOnce_16x", Icon16x16));
		Set( "GraphEditor.Macro.IsValid_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_IsValid_16x", Icon16x16));
		Set( "GraphEditor.Macro.FlipFlop_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_FlipFlop_16x", Icon16x16));
		Set( "GraphEditor.Macro.ForEach_16x", new IMAGE_BRUSH("Icons/icon_Blueprint_ForEach_16x", Icon16x16));

		Set( "GraphEditor.Delegate_16x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Delegate_16x", Icon16x16 ) );
		Set( "GraphEditor.Delegate_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Delegate_24x", Icon24x24 ) );

		Set( "GraphEditor.EventGraph_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_EventGraph_24x", Icon24x24 ) );
		Set( "GraphEditor.InterfaceFunction_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Interfacefunction_24x", Icon24x24 ) );
		Set( "GraphEditor.Macro_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Macro_24x", Icon24x24 ) );
		Set( "GraphEditor.Function_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_NewFunction_24x", Icon24x24 ) );
		Set( "GraphEditor.PotentialOverrideFunction_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_OverrideableFunction_24x", Icon24x24 ) );
		Set( "GraphEditor.OverrideFunction_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_OverrideFunction_24x", Icon24x24 ) );
		Set( "GraphEditor.SubGraph_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_SubgraphComposite_24x", Icon24x24 ) );
		Set( "GraphEditor.Animation_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Anim_24x", Icon24x24 ) );
		Set( "GraphEditor.Conduit_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Conduit_24x", Icon24x24 ) );
		Set( "GraphEditor.Rule_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_Rule_24x", Icon24x24 ) );
		Set( "GraphEditor.State_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_State_24x", Icon24x24 ) );
		Set( "GraphEditor.StateMachine_24x", new IMAGE_BRUSH( "Icons/icon_Blueprint_StateMachine_24x", Icon24x24 ) );

		Set( "GraphEditor.FunctionGlyph", new IMAGE_BRUSH( "Graph/Icons/Function", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.NodeGlyph", new IMAGE_BRUSH( "Graph/Icons/Node", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.PinIcon", new IMAGE_BRUSH( "Graph/Icons/Pin", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.ArrayPinIcon", new IMAGE_BRUSH( "Graph/Icons/ArrayPin", Icon22x22, FLinearColor::White ) );
		Set( "GraphEditor.RefPinIcon", new IMAGE_BRUSH( "Graph/Icons/RefPin", Icon22x22, FLinearColor::White ) );
		Set( "GraphEditor.UbergraphGlyph", new IMAGE_BRUSH( "Graph/Icons/EventGraph", Icon22x22, FLinearColor::White) );		
		Set( "GraphEditor.SubgraphGlyph", new IMAGE_BRUSH( "Graph/Icons/Subgraph", Icon22x22, FLinearColor::White) );		
		Set( "GraphEditor.AnimationGlyph", new IMAGE_BRUSH( "Graph/Icons/Robot", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.MacroGlyph", new IMAGE_BRUSH( "Graph/Icons/Macro", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.EnumGlyph", new IMAGE_BRUSH( "Graph/Icons/Enum", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.TimelineGlyph", new IMAGE_BRUSH( "Graph/Icons/Timeline", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.EventGlyph", new IMAGE_BRUSH( "Graph/Icons/Event", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.EventCustomGlyph", new IMAGE_BRUSH( "Graph/Icons/Event_Custom", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.SCSGlyph", new IMAGE_BRUSH( "Graph/Icons/Hammer", Icon22x22, FLinearColor::White) );
		// Find In Blueprints
		Set( "GraphEditor.FIB_CallFunction", new IMAGE_BRUSH( "Graph/Icons/FIB_CallFunction", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.FIB_MacroInstance", new IMAGE_BRUSH( "Graph/Icons/FIB_MacroInstance", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.FIB_Event", new IMAGE_BRUSH( "Graph/Icons/FIB_Event", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.FIB_VariableGet", new IMAGE_BRUSH( "Graph/Icons/FIB_VarGet", Icon22x22, FLinearColor::White) );
		Set( "GraphEditor.FIB_VariableSet", new IMAGE_BRUSH( "Graph/Icons/FIB_VarSet", Icon22x22, FLinearColor::White) );

		Set( "GraphEditor.FunctionOL.Interface", new IMAGE_BRUSH( "Graph/Icons/Overlay_Interface", Icon22x22 ) );
		Set( "GraphEditor.FunctionOL.New", new IMAGE_BRUSH( "Graph/Icons/Overlay_New", Icon22x22 ) );
		Set( "GraphEditor.FunctionOL.Override", new IMAGE_BRUSH( "Graph/Icons/Overlay_Override", Icon22x22 ) );
		Set( "GraphEditor.FunctionOL.PotentialOverride", new IMAGE_BRUSH( "Graph/Icons/Overlay_PotentialOverride", Icon22x22 ) );

		Set( "GraphEditor.HideUnusedPins", new IMAGE_BRUSH( "Icons/Hide_UnusedPins", Icon40x40 ) );
		Set( "GraphEditor.HideUnusedPins.Small", new IMAGE_BRUSH( "Icons/Hide_UnusedPins", Icon20x20 ) );

		// Graph editor widgets
		{
			// EditableTextBox
			{
				Set( "Graph.EditableTextBox", FEditableTextBoxStyle()
					.SetBackgroundImageNormal( BOX_BRUSH( "Graph/CommonWidgets/TextBox", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageHovered( BOX_BRUSH( "Graph/CommonWidgets/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageFocused( BOX_BRUSH( "Graph/CommonWidgets/TextBox_Hovered", FMargin(4.0f/16.0f) ) )
					.SetBackgroundImageReadOnly( BOX_BRUSH( "Graph/CommonWidgets/TextBox", FMargin(4.0f/16.0f) ) )
					);
			}

			// Check Box
			{
				/* Set images for various SCheckBox states of style Graph.Checkbox ... */
				const FCheckBoxStyle BasicGraphCheckBoxStyle = FCheckBoxStyle()
					.SetUncheckedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox", Icon16x16 ) )
					.SetUncheckedHoveredImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Hovered", Icon16x16 ) )
					.SetUncheckedPressedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Hovered", Icon16x16 ) )
					.SetCheckedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Checked", Icon16x16 ) )
					.SetCheckedHoveredImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Checked_Hovered", Icon16x16 ) )
					.SetCheckedPressedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Checked", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
					.SetUndeterminedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Undetermined", Icon16x16 ) )
					.SetUndeterminedHoveredImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Undetermined_Hovered", Icon16x16 ) )
					.SetUndeterminedPressedImage( IMAGE_BRUSH( "/Graph/CommonWidgets/Checkbox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );

				/* ... and add the new style */
				Set( "Graph.Checkbox", BasicGraphCheckBoxStyle );
			}
		}

		// SCSEditor
		Set("SCSEditor.ToggleComponentEditing" , new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon40x40) );
		Set("SCSEditor.ToggleComponentEditing.Small" , new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon20x20) );

		// Timeline Editor
		{
			Set( "TimelineEditor.AddFloatTrack", new IMAGE_BRUSH( "Icons/icon_TrackAddFloat_36x24px", Icon36x24, FLinearColor::Black ) );
			Set( "TimelineEditor.AddVectorTrack", new IMAGE_BRUSH( "Icons/icon_TrackAddVector_36x24px", Icon36x24, FLinearColor::Black ) );
			Set( "TimelineEditor.AddEventTrack", new IMAGE_BRUSH( "Icons/icon_TrackAddEvent_36x24px", Icon36x24, FLinearColor::Black ) );
			Set( "TimelineEditor.AddColorTrack", new IMAGE_BRUSH( "Icons/icon_TrackAddColor_36x24px", Icon36x24, FLinearColor::Black ) );
			Set( "TimelineEditor.AddCurveAssetTrack", new IMAGE_BRUSH( "Icons/icon_TrackAddCurve_36x24px", Icon36x24, FLinearColor::Black ) );
			Set( "TimelineEditor.DeleteTrack", new IMAGE_BRUSH( "Icons/icon_TrackDelete_36x24px", Icon36x24, FLinearColor::Black ) );
		}
	}

	// Notify editor
	{
		Set( "Persona.NotifyEditor.NotifyTrackBackground", new BOX_BRUSH( "/Persona/NotifyEditor/NotifyTrackBackground", FMargin(8.0f/64.0f, 3.0f/32.0f) ) );
	}

	// Blueprint modes
	{
		Set( "ModeSelector.ToggleButton.Normal", new FSlateNoResource() );		// Note: Intentionally transparent background
		Set( "ModeSelector.ToggleButton.Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "ModeSelector.ToggleButton.Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) );


		Set( "BlueprintEditor.PipelineSeparator", new BOX_BRUSH( "Old/Kismet2/BlueprintModeSeparator", FMargin(15.0f/16.0f, 20.0f/20.0f, 1.0f/16.0f, 0.0f/20.0f), FLinearColor(1,1,1,0.5f) ) );
	}

	// Persona modes
	{
		Set( "Persona.PipelineSeparator", new BOX_BRUSH( "Persona/Modes/PipelineSeparator", FMargin(15.0f/16.0f, 22.0f/24.0f, 1.0f/16.0f, 1.0f/24.0f), FLinearColor(1,1,1,0.5f) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM
	}

void FSlateEditorStyle::FStyle::SetupLevelEditorStyle()
	{
	// Level editor tool bar icons
#if WITH_EDITOR
	{
		Set("LevelEditor.BrowseDocumentation", new IMAGE_BRUSH("Icons/Help/icon_Help_Documentation_16x", Icon16x16));
		Set("LevelEditor.BrowseAPIReference", new IMAGE_BRUSH("Icons/Help/icon_Help_api-1_16x", Icon16x16));
		Set("LevelEditor.Tutorials", new IMAGE_BRUSH("Icons/Help/icon_Help_tutorials_16x", Icon16x16));
		Set("LevelEditor.BrowseViewportControls", new IMAGE_BRUSH("Icons/Help/icon_Help_Documentation_16x", Icon16x16));

		Set("MainFrame.VisitAskAQuestionPage", new IMAGE_BRUSH("Icons/Help/icon_Help_ask_16x", Icon16x16));
		Set("MainFrame.VisitWiki", new IMAGE_BRUSH("Icons/Help/icon_Help_Documentation_16x", Icon16x16));
		Set("MainFrame.VisitForums", new IMAGE_BRUSH("Icons/Help/icon_Help_Documentation_16x", Icon16x16));
		Set("MainFrame.VisitSearchForAnswersPage", new IMAGE_BRUSH("Icons/Help/icon_Help_search_16x", Icon16x16));
		Set("MainFrame.VisitSupportWebSite", new IMAGE_BRUSH("Icons/Help/icon_Help_support_16x", Icon16x16));
		Set("MainFrame.VisitEpicGamesDotCom", new IMAGE_BRUSH("Icons/Help/icon_Help_epic_16x", Icon16x16));
		Set("MainFrame.AboutUnrealEd", new IMAGE_BRUSH("Icons/Help/icon_Help_unreal_16x", Icon16x16));

		const FLinearColor IconColor = FLinearColor::Black;
		Set( "EditorViewport.TranslateMode", new IMAGE_BRUSH( "Icons/icon_translateb_16x", Icon16x16 ) );
		Set( "EditorViewport.TranslateMode.Small", new IMAGE_BRUSH( "Icons/icon_translateb_16x", Icon16x16 ) );
		Set( "EditorViewport.RotateMode", new IMAGE_BRUSH( "Icons/icon_rotateb_16x", Icon16x16 ) );
		Set( "EditorViewport.RotateMode.Small", new IMAGE_BRUSH( "Icons/icon_rotateb_16x", Icon16x16 ) );
		Set( "EditorViewport.ScaleMode", new IMAGE_BRUSH( "Icons/icon_scaleb_16x", Icon16x16 ) );
		Set( "EditorViewport.ScaleMode.Small", new IMAGE_BRUSH( "Icons/icon_scaleb_16x", Icon16x16 ) );
		Set( "EditorViewport.TranslateRotateMode", new IMAGE_BRUSH( "Icons/icon_translate_rotate_40x", Icon20x20 ) );
		Set( "EditorViewport.TranslateRotateMode.Small", new IMAGE_BRUSH( "Icons/icon_translate_rotate_40x", Icon20x20 ) );
		Set( "EditorViewport.ToggleRealTime", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon40x40 ) );
		Set( "EditorViewport.ToggleRealTime.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon20x20 ) );
		Set( "EditorViewport.LocationGridSnap", new IMAGE_BRUSH( "Old/LevelEditor/LocationGridSnap", Icon14x14, IconColor) );
		Set( "EditorViewport.RotationGridSnap", new IMAGE_BRUSH( "Old/LevelEditor/RotationGridSnap", Icon14x14, IconColor ) );
		Set( "EditorViewport.ScaleGridSnap", new IMAGE_BRUSH( "Old/LevelEditor/ScaleGridSnap", Icon14x14, IconColor ) );
		Set( "EditorViewport.RelativeCoordinateSystem_Local", new IMAGE_BRUSH( "Icons/icon_axis_local_16px", Icon16x16, IconColor ) );
		Set( "EditorViewport.RelativeCoordinateSystem_Local.Small", new IMAGE_BRUSH( "Icons/icon_axis_local_16px", Icon16x16, IconColor ) );
		Set( "EditorViewport.RelativeCoordinateSystem_World", new IMAGE_BRUSH( "Icons/icon_axis_world_16px", Icon16x16, IconColor ) );
		Set( "EditorViewport.RelativeCoordinateSystem_World.Small", new IMAGE_BRUSH( "Icons/icon_axis_world_16px", Icon16x16, IconColor ) );
		Set( "EditorViewport.CamSpeedSetting", new IMAGE_BRUSH( "Icons/icon_CameraSpeed_24x16px", FVector2D( 24, 16 ), IconColor ) );
		
		Set( "EditorViewport.LitMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_Lit_16px", Icon16x16 ) );
		Set( "EditorViewport.UnlitMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_Unlit_16px", Icon16x16 ) );
		Set( "EditorViewport.WireframeMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_BrushWireframe_16px", Icon16x16 ) );
		Set( "EditorViewport.DetailLightingMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_DetailLighting_16px", Icon16x16 ) );
		Set( "EditorViewport.LightingOnlyMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_LightingOnly_16px", Icon16x16 ) );
		Set( "EditorViewport.LightComplexityMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_LightComplexity_16px", Icon16x16 ) );
		Set( "EditorViewport.ShaderComplexityMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_Shadercomplexity_16px", Icon16x16 ) );
		Set( "EditorViewport.StationaryLightOverlapMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_StationaryLightOverlap_16px", Icon16x16 ) );
		Set( "EditorViewport.LightmapDensityMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_LightmapDensity_16px", Icon16x16 ) );
		Set( "EditorViewport.VisualizeGBufferMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_VisualisationGBuffer_16px", Icon16x16 ) );
		Set( "EditorViewport.ReflectionOverrideMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_ReflectionOverride_16px", Icon16x16 ) );
		Set( "EditorViewport.VisualizeBufferMode", new IMAGE_BRUSH( "Icons/icon_ViewMode_VisualisationGBuffer_16px", Icon16x16 ) );
		Set( "EditorViewport.CollisionPawn", new IMAGE_BRUSH( "Icons/icon_ViewMode_CollsionPawn_16px", Icon16x16 ) );
		Set( "EditorViewport.CollisionVisibility", new IMAGE_BRUSH( "Icons/icon_ViewMode_CollisionVisibility_16px", Icon16x16 ) );
		Set( "EditorViewport.Perspective", new IMAGE_BRUSH( "Icons/icon_ViewMode_ViewPerspective_16px", Icon16x16 ) );
		Set( "EditorViewport.Top", new IMAGE_BRUSH( "Icons/icon_ViewMode_ViewTop_16px", Icon16x16 ) );
		Set( "EditorViewport.Side", new IMAGE_BRUSH( "Icons/icon_ViewMode_ViewSide_16px", Icon16x16 ) );
		Set( "EditorViewport.Front", new IMAGE_BRUSH( "Icons/icon_ViewMode_ViewFront_16px", Icon16x16 ) );
#endif

#if WITH_EDITOR || IS_PROGRAM
		{
			Set( "LevelEditor.Tabs.Details", new IMAGE_BRUSH( "/Icons/icon_tab_SelectionDetails_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.EditorModes", new IMAGE_BRUSH( "/Icons/icon_tab_Tools_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Modes", new IMAGE_BRUSH( "/Icons/icon_tab_Tools_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Outliner", new IMAGE_BRUSH( "/Icons/icon_tab_SceneOutliner_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.ContentBrowser", new IMAGE_BRUSH( "/Icons/icon_tab_ContentBrowser_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Levels", new IMAGE_BRUSH( "/Icons/icon_tab_Levels_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.WorldBrowser", new IMAGE_BRUSH( "/Icons/icon_tab_Levels_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Layers", new IMAGE_BRUSH( "/Icons/icon_tab_Layers_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.BuildAndSubmit", new IMAGE_BRUSH( "/Icons/icon_tab_BuildSubmit_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.StatsViewer", new IMAGE_BRUSH( "/Icons/icon_tab_Stats_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Toolbar", new IMAGE_BRUSH( "/Icons/icon_tab_Toolbars_16x", Icon16x16 ) );
			Set( "LevelEditor.Tabs.Viewports", new IMAGE_BRUSH( "/Icons/icon_tab_Viewports_16x", Icon16x16 ) );
		}
#endif

#if WITH_EDITOR
		Set( "LevelEditor.NewLevel", new IMAGE_BRUSH( "icons/icon_file_new_16px", Icon16x16 ) );
		Set( "LevelEditor.OpenLevel", new IMAGE_BRUSH( "icons/icon_file_open_16px", Icon16x16 ) );
		Set( "LevelEditor.Save", new IMAGE_BRUSH( "icons/icon_file_save_16px", Icon16x16 ) );
		Set( "LevelEditor.SaveAs", new IMAGE_BRUSH( "icons/icon_file_saveas_16px", Icon16x16 ) );
		Set( "LevelEditor.SaveAllLevels", new IMAGE_BRUSH( "icons/icon_file_savelevels_16px", Icon16x16 ) );

		Set( "LevelEditor.Build", new IMAGE_BRUSH( "Icons/icon_build_40x", Icon40x40 ) );
		Set( "LevelEditor.Build.Small", new IMAGE_BRUSH( "Icons/icon_build_40x", Icon20x20 ) );
		Set( "LevelEditor.MapCheck", new IMAGE_BRUSH( "Icons/icon_MapCheck_40x", Icon40x40 ) );

		Set( "LevelEditor.Recompile", new IMAGE_BRUSH( "Icons/icon_compile_40x", Icon40x40 ) );
		Set( "LevelEditor.Recompile.Small", new IMAGE_BRUSH( "Icons/icon_compile_40x", Icon20x20 ) );

		Set( "LevelEditor.ViewOptions", new IMAGE_BRUSH( "Icons/icon_view_40x", Icon40x40 ) );
		Set( "LevelEditor.ViewOptions.Small", new IMAGE_BRUSH( "Icons/icon_view_40x", Icon20x20 ) );

		Set( "LevelEditor.Create", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_40px", Icon40x40 ) );
		Set( "LevelEditor.Create.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_40px", Icon20x20 ) );
		Set( "LevelEditor.Create.OutlineHoriz", new IMAGE_BRUSH( "Common/WorkingFrame_Marquee", FVector2D(34.0f, 3.0f), FLinearColor::White, ESlateBrushTileType::Horizontal) );
		Set( "LevelEditor.Create.OutlineVert", new IMAGE_BRUSH( "Common/WorkingFrame_Marquee_Vert", FVector2D(3.0f, 34.0f), FLinearColor::White, ESlateBrushTileType::Vertical) );

		Set( "LevelEditor.EditorModes", new IMAGE_BRUSH( "Icons/icon_Editor_Modes_40x", Icon40x40 ) );
		Set( "LevelEditor.EditorModes.Small", new IMAGE_BRUSH( "Icons/icon_Editor_Modes_40x", Icon20x20 ) );
		Set( "LevelEditor.EditorModes.Menu", new IMAGE_BRUSH( "Icons/icon_Editor_Modes_16x", Icon16x16 ) );

		Set( "LevelEditor.PlacementMode", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_40px", Icon40x40 ) );
		Set( "LevelEditor.PlacementMode.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_40px", Icon20x20 ) );
		Set( "LevelEditor.PlacementMode.Selected", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_selected_40x", Icon40x40 ) );
		Set( "LevelEditor.PlacementMode.Selected.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Placement_selected_40x", Icon20x20 ) );

		Set( "LevelEditor.MeshPaintMode", new IMAGE_BRUSH( "Icons/icon_Mode_MeshPaint_40x", Icon40x40 ) );
		Set( "LevelEditor.MeshPaintMode.Small", new IMAGE_BRUSH( "Icons/icon_Mode_MeshPaint_40x", Icon20x20 ) );
		Set( "LevelEditor.MeshPaintMode.Selected", new IMAGE_BRUSH( "Icons/icon_Mode_Meshpaint_selected_40x", Icon40x40 ) );
		Set( "LevelEditor.MeshPaintMode.Selected.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Meshpaint_selected_40x", Icon20x20 ) );

		Set( "LevelEditor.LandscapeMode", new IMAGE_BRUSH( "Icons/icon_Mode_Landscape_40x", Icon40x40 ) );
		Set( "LevelEditor.LandscapeMode.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Landscape_40x", Icon20x20 ) );
		Set( "LevelEditor.LandscapeMode.Selected", new IMAGE_BRUSH( "Icons/icon_Mode_Landscape_selected_40x", Icon40x40 ) );
		Set( "LevelEditor.LandscapeMode.Selected.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Landscape_selected_40x", Icon20x20 ) );

		Set( "LevelEditor.FoliageMode", new IMAGE_BRUSH( "Icons/icon_Mode_Foliage_40x", Icon40x40 ) );
		Set( "LevelEditor.FoliageMode.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Foliage_40x", Icon20x20 ) );
		Set( "LevelEditor.FoliageMode.Selected", new IMAGE_BRUSH( "Icons/icon_Mode_Foliage_selected_40x", Icon40x40 ) );
		Set( "LevelEditor.FoliageMode.Selected.Small", new IMAGE_BRUSH( "Icons/icon_Mode_Foliage_selected_40x", Icon20x20 ) );

		Set( "LevelEditor.BspMode", new IMAGE_BRUSH( "Icons/icon_Mode_GeoEdit_40px", Icon40x40 ) );
		Set( "LevelEditor.BspMode.Small", new IMAGE_BRUSH( "Icons/icon_Mode_GeoEdit_40px", Icon20x20 ) );
		Set( "LevelEditor.BspMode.Selected", new IMAGE_BRUSH( "Icons/icon_Mode_GeoEdit-a_40px", Icon40x40 ) );
		Set( "LevelEditor.BspMode.Selected.Small", new IMAGE_BRUSH( "Icons/icon_Mode_GeoEdit-a_40px", Icon20x20 ) );

		Set( "LevelEditor.WorldProperties", new IMAGE_BRUSH( "Icons/icon_worldscript_40x", Icon40x40 ) );
		Set( "LevelEditor.WorldProperties.Small", new IMAGE_BRUSH( "Icons/icon_worldscript_40x", Icon20x20 ) );
		Set( "LevelEditor.WorldProperties.Tab", new IMAGE_BRUSH( "Icons/icon_worldscript_40x", Icon16x16 ) );
		Set( "LevelEditor.OpenContentBrowser", new IMAGE_BRUSH( "Icons/icon_ContentBrowser_40x", Icon40x40 ) );
		Set( "LevelEditor.OpenContentBrowser.Small", new IMAGE_BRUSH( "Icons/icon_ContentBrowser_40x", Icon20x20 ) );
		Set( "LevelEditor.OpenMarketplace", new IMAGE_BRUSH( "Icons/icon_Samples_Download_40x", Icon40x40 ) );
		Set( "LevelEditor.OpenMarketplace.Small", new IMAGE_BRUSH( "Icons/icon_Samples_Download_20x", Icon20x20 ) );
		Set( "LevelEditor.OpenLevelBlueprint", new IMAGE_BRUSH( "Icons/icon_kismet2_40x", Icon40x40 ) );
		Set( "LevelEditor.OpenLevelBlueprint.Small", new IMAGE_BRUSH( "Icons/icon_kismet2_40x", Icon20x20 ) );
		Set( "LevelEditor.CreateClassBlueprint", new IMAGE_BRUSH("Icons/icon_class_Blueprint_New_16x", Icon16x16));
		Set( "LevelEditor.OpenClassBlueprint", new IMAGE_BRUSH("Icons/icon_class_Blueprint_Open_16x", Icon16x16));
		Set( "LevelEditor.EditMatinee", new IMAGE_BRUSH( "Icons/icon_matinee_40x", Icon40x40 ) );
		Set( "LevelEditor.EditMatinee.Small", new IMAGE_BRUSH( "Icons/icon_matinee_40x", Icon20x20 ) );
		
		Set( "PlacementBrowser.OptionsMenu", new IMAGE_BRUSH( "Icons/icon_Blueprint_Macro_16x", Icon16x16 ) );

		Set( "PlacementBrowser.AssetToolTip.AssetName", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) ) );
		Set( "PlacementBrowser.AssetToolTip.AssetClassName", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "PlacementBrowser.AssetToolTip.AssetPath", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) ) );

		Set( "PlacementBrowser.Asset", FButtonStyle( Button )
			.SetNormal( FSlateNoResource() )
			.SetHovered( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor( 1.0f, 1.0f, 1.0f, 0.1f ) ) )
			.SetPressed( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetNormalPadding( 0 )
			.SetPressedPadding( 0 )
			);

		/* Create style for "ToolBar.ToggleButton" widget ... */
		const FCheckBoxStyle ToolBarToggleButtonCheckBoxStyle = FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor ) )
			.SetCheckedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) )
			.SetCheckedPressedImage( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor ) );
		/* ... and add new style */
		Set( "ToolBar.ToggleButton", ToolBarToggleButtonCheckBoxStyle );

		FLinearColor DimBackground = FLinearColor( FColor( 64, 64, 64 ) );
		FLinearColor DimBackgroundHover = FLinearColor( FColor( 50, 50, 50 ) );
		FLinearColor DarkBackground = FLinearColor( FColor( 42, 42, 42 ) );

		Set( "PlacementBrowser.Tab", FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( BOX_BRUSH( "Common/Selection", 8.0f / 32.0f, DimBackground ) )
			.SetUncheckedPressedImage( BOX_BRUSH( "PlacementMode/TabActive", 8.0f / 32.0f ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Common/Selection", 8.0f / 32.0f, DimBackgroundHover ) )
			.SetCheckedImage( BOX_BRUSH( "PlacementMode/TabActive", 8.0f / 32.0f ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "PlacementMode/TabActive", 8.0f / 32.0f ) )
			.SetCheckedPressedImage( BOX_BRUSH( "PlacementMode/TabActive", 8.0f / 32.0f ) )
			.SetPadding( 0 ) );

		Set( "PlacementBrowser.Tab.ImportantText", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f, 1.0f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		Set( "PlacementBrowser.Tab.Text", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f, 0.9f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		Set( "PlacementBrowser.Asset.Name", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f, 0.9f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		Set( "PlacementBrowser.Asset.Type", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor( 0.8f, 0.8f, 0.8f, 0.9f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		Set( "PlacementBrowser.ActiveTabNub", new IMAGE_BRUSH( "Icons/TabTriangle_24x", Icon24x24, FLinearColor( FColor( 42, 42, 42 ) ) ) );
		Set( "PlacementBrowser.ActiveTabBar", new IMAGE_BRUSH( "Common/Selection", FVector2D(2.0f, 2.0f), SelectionColor ) );

		Set( "PlacementBrowser.ShowAllContent", new IMAGE_BRUSH( "Icons/icon_Placement_AllContent_20px", Icon20x20 ) );
		Set( "PlacementBrowser.ShowAllContent.Small", new IMAGE_BRUSH( "Icons/icon_Placement_AllContent_20px", Icon20x20 ) );
		Set( "PlacementBrowser.ShowCollections", new IMAGE_BRUSH( "Icons/icon_Placement_Collections_20px", Icon20x20 ) );
		Set( "PlacementBrowser.ShowCollections.Small", new IMAGE_BRUSH( "Icons/icon_Placement_Collections_20px", Icon20x20 ) );

		Set( "ContentPalette.ShowAllPlaceables", new IMAGE_BRUSH( "Icons/icon_Placement_FilterAll_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowAllPlaceables.Small", new IMAGE_BRUSH( "Icons/icon_Placement_FilterAll_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowProps", new IMAGE_BRUSH( "Icons/icon_Placement_FilterProps_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowProps.Small", new IMAGE_BRUSH( "Icons/icon_Placement_FilterProps_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowParticles", new IMAGE_BRUSH( "Icons/icon_Placement_FilterParticles_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowParticles.Small", new IMAGE_BRUSH( "Icons/icon_Placement_FilterParticles_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowAudio", new IMAGE_BRUSH( "Icons/icon_Placement_FilterAudio_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowAudio.Small", new IMAGE_BRUSH( "Icons/icon_Placement_FilterAudio_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowMisc", new IMAGE_BRUSH( "Icons/icon_Placement_FilterMisc_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowMisc.Small", new IMAGE_BRUSH( "Icons/icon_Placement_FilterMisc_20px", Icon20x20 ) );
		Set( "ContentPalette.ShowRecentlyPlaced", new IMAGE_BRUSH( "Icons/icon_Placement_RecentlyPlaced_20x", Icon20x20 ) );
		Set( "ContentPalette.ShowRecentlyPlaced.Small", new IMAGE_BRUSH( "Icons/icon_Placement_RecentlyPlaced_20x", Icon20x20 ) );
	}

	{

		Set( "AssetDeleteDialog.Background", new IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor( 0.016, 0.016, 0.016 ) ) );
	}

	// Level editor tool box icons
	{
		Set( "LevelEditor.RecompileGameCode", new IMAGE_BRUSH( "Old/MainToolBar/RecompileGameCode", Icon40x40 ) );
	}

	// Level viewport layout command icons
	{
		const FVector2D IconLayoutSize(49.0f, 37.0f);
		const FVector2D IconLayoutSizeSmall(49.0f, 37.0f);		// small version set to same size as these are in their own menu and don't clutter the UI

		Set( "LevelViewport.ViewportConfig_TwoPanesH", new IMAGE_BRUSH( "Icons/ViewportLayout_TwoPanesHoriz", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_TwoPanesH.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_TwoPanesHoriz", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_TwoPanesV", new IMAGE_BRUSH( "Icons/ViewportLayout_TwoPanesVert", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_TwoPanesV.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_TwoPanesVert", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesLeft", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesLeft", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesLeft.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesLeft", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesRight", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesRight", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesRight.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesRight", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesTop", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesTop", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesTop.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesTop", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesBottom", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesBottom", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_ThreePanesBottom.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_ThreePanesBottom", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_FourPanesLeft", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesLeft", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_FourPanesLeft.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesLeft", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_FourPanesRight", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesRight", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_FourPanesRight.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesRight", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_FourPanesTop", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesTop", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_FourPanesTop.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesTop", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_FourPanesBottom", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesBottom", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_FourPanesBottom.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanesBottom", IconLayoutSizeSmall ) );
		Set( "LevelViewport.ViewportConfig_FourPanes2x2", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanes2x2", IconLayoutSize ) );
		Set( "LevelViewport.ViewportConfig_FourPanes2x2.Small", new IMAGE_BRUSH( "Icons/ViewportLayout_FourPanes2x2", IconLayoutSizeSmall ) );
	}

	// Level editor status bar
	{
		Set( "TransformSettings.RelativeCoordinateSettings", new IMAGE_BRUSH( "Icons/icon_axis_16px", FVector2D( 16, 16 ) ) );
	}

	// Mesh Proxy Window
	{
		Set("MeshProxy.SimplygonLogo", new IMAGE_BRUSH( "Icons/SimplygonBanner_Sml", FVector2D(174, 36) ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Level viewport 
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "LevelViewport.ActiveViewportBorder", new BORDER_BRUSH( "Old/White", FMargin(1), SelectionColor ) ); 
		Set( "LevelViewport.NoViewportBorder", new FSlateNoResource() );
		Set( "LevelViewport.DebugBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(.7,0,0,.5) ) );
		Set( "LevelViewport.BlackBackground", new FSlateColorBrush( FLinearColor::Black ) ); 
		Set( "LevelViewport.StartingPlayInEditorBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(0.1f,1.0f,0.1f,1.0f) ) );
		Set( "LevelViewport.StartingSimulateBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(1.0f,1.0f,0.1f,1.0f) ) );
		Set( "LevelViewport.ReturningToEditorBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(0.1f,0.1f,1.0f,1.0f) ) );
		Set( "LevelViewport.ActorLockIcon", new IMAGE_BRUSH( "Icons/ActorLockedViewport", Icon32x32 ) );
		Set( "LevelViewport.Icon", new IMAGE_BRUSH( "Icons/icon_tab_viewport_16px", Icon16x16 ) );

		Set( "LevelViewportContextMenu.ActorType.Text", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) )
			.SetColorAndOpacity( FLinearColor(0.72f, 0.72f, 0.72f, 1.f) ) );

		Set( "LevelViewportContextMenu.AssetLabel.Text", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );

		Set( "LevelViewport.CursorIcon", new IMAGE_BRUSH( "Common/Cursor", Icon16x16 ) );

		Set( "LevelViewport.AntiAliasing", new IMAGE_BRUSH( "Icons/icon_ShowAnti-aliasing_16x", Icon16x16 ) );
		Set( "LevelViewport.Atmosphere", new IMAGE_BRUSH( "Icons/icon_ShowAtmosphere_16x", Icon16x16 ) );
		Set( "LevelViewport.BSP", new IMAGE_BRUSH( "Icons/icon_ShowBSP_16x", Icon16x16 ) );
		Set( "LevelViewport.Collision", new IMAGE_BRUSH( "Icons/icon_ShowCollision_16x", Icon16x16 ) );
		Set( "LevelViewport.Decals", new IMAGE_BRUSH( "Icons/icon_ShowDecals_16x", Icon16x16 ) );
		Set( "LevelViewport.Fog", new IMAGE_BRUSH( "Icons/icon_ShowFog_16x", Icon16x16 ) );
		Set( "LevelViewport.Grid", new IMAGE_BRUSH( "Icons/icon_ShowGrid_16x", Icon16x16 ) );
		Set( "LevelViewport.Landscape", new IMAGE_BRUSH( "Icons/icon_ShowLandscape_16x", Icon16x16 ) );
		Set( "LevelViewport.Navigation", new IMAGE_BRUSH( "Icons/icon_ShowNavigation_16x", Icon16x16 ) );
		Set( "LevelViewport.Particles", new IMAGE_BRUSH( "Icons/icon_ShowParticlesSprite_16x", Icon16x16 ) );
		Set( "LevelViewport.SkeletalMeshes", new IMAGE_BRUSH( "Icons/icon_ShowSkeletalMeshes_16x", Icon16x16 ) );
		Set( "LevelViewport.StaticMeshes", new IMAGE_BRUSH( "Icons/icon_ShowStaticMeshes_16x", Icon16x16 ) );
		Set( "LevelViewport.Translucency", new IMAGE_BRUSH( "Icons/icon_ShowTranslucency_16x", Icon16x16 ) );
	}

	// Level editor ui command icons
	{
		Set( "LevelEditor.ShowAll", new IMAGE_BRUSH( "Old/SelectionDetails/ShowAll", FVector2D(32,32) ) );
		Set( "LevelEditor.ShowSelectedOnly", new IMAGE_BRUSH( "Old/SelectionDetails/ShowSelected", FVector2D(32,32) ) );
		Set( "LevelEditor.ShowSelected", new IMAGE_BRUSH( "Old/SelectionDetails/ShowSelected", FVector2D(32,32) ) );
		Set( "LevelEditor.HideSelected", new IMAGE_BRUSH( "Old/SelectionDetails/HideSelected", FVector2D(32,32) ) );
	}

	// Level viewport toolbar
	{
		Set( "EditorViewportToolBar.MenuButton", FButtonStyle(Button)
			.SetNormal(BOX_BRUSH( "Common/SmallRoundedButton", FMargin(7.f/16.f), FLinearColor(1,1,1,0.75f)))
			.SetHovered(BOX_BRUSH( "Common/SmallRoundedButton", FMargin(7.f/16.f), FLinearColor(1,1,1, 1.0f)))
			.SetPressed(BOX_BRUSH( "Common/SmallRoundedButton", FMargin(7.f/16.f) ))
		);

		Set( "EditorViewportToolBar.Button", HoverHintOnly );

		/* Set style structure for "EditorViewportToolBar.Button" ... */
		const FCheckBoxStyle EditorViewportToolBarButton = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage( FSlateNoResource() )
			.SetUncheckedPressedImage( BOX_BRUSH( "Old/LevelViewportToolBar/MenuButton_Pressed", 4.0f/16.0f ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "Old/Border", 4.0f/16.0f ) )
			.SetCheckedImage( FSlateNoResource() )
			.SetCheckedHoveredImage( BOX_BRUSH( "Old/Border", 4.0f/16.0f ) )
			.SetCheckedPressedImage( BOX_BRUSH( "Old/LevelViewportToolBar/MenuButton_Pressed", 4.0f/16.0f ) );
		/* ... and set new style */
		Set( "LevelViewportToolBar.CheckBoxButton", EditorViewportToolBarButton );

		Set( "EditorViewportToolBar.MenuDropdown", new IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) );
		Set( "LevelViewportToolBar.Maximize.Normal", new IMAGE_BRUSH( "Old/LevelViewportToolbar/Maximized_Unchecked", Icon16x16 ) );
		Set( "LevelViewportToolBar.Maximize.Checked", new IMAGE_BRUSH( "Old/LevelViewportToolbar/Maximized_Checked", Icon16x16 ) );
		Set( "LevelViewportToolBar.RestoreFromImmersive.Normal", new IMAGE_BRUSH( "Icons/icon_RestoreFromImmersive_16px", Icon16x16 ) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Mesh Paint
	{
		Set( "MeshPaint.Fill", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Fill_16x", Icon16x16 ) );
		Set( "MeshPaint.CopyInstVertColors", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Copy_16x", Icon16x16 ) );
		Set( "MeshPaint.ImportVertColors", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Import_16x", Icon16x16 ) );
		Set( "MeshPaint.FindInCB", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Find_16x", Icon16x16 ) );
		Set( "MeshPaint.SavePackage", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Save_16x", Icon16x16 ) );
		Set( "MeshPaint.CommitChanges", new IMAGE_BRUSH( "/Icons/assign_right_16x", Icon16x16 ) );
		Set( "MeshPaint.Swap", new IMAGE_BRUSH( "/Icons/icon_MeshPaint_Swap_16x", Icon16x16 ) );
	}

	// EditorModesToolbar
	{
		Set( "EditorModesToolbar.Background", new FSlateNoResource() );
		Set( "EditorModesToolbar.Icon", new IMAGE_BRUSH( "Icons/icon_tab_toolbar_16px", Icon16x16 ) );
		Set( "EditorModesToolbar.Expand", new IMAGE_BRUSH( "Icons/toolbar_expand_16x", Icon16x16) );
		Set( "EditorModesToolbar.SubMenuIndicator", new IMAGE_BRUSH( "Common/SubmenuArrow", Icon8x8 ) );
		Set( "EditorModesToolbar.SToolBarComboButtonBlock.Padding", FMargin( 0 ) );
		Set( "EditorModesToolbar.SToolBarComboButtonBlock.ComboButton.Color", DefaultForeground );
		Set( "EditorModesToolbar.SToolBarButtonBlock.Padding", FMargin( 1.0f, 0.0f, 0.0f, 0 ) );
		Set( "EditorModesToolbar.SToolBarButtonBlock.CheckBox.Padding", FMargin( 6.0f, 4.0f, 6.0f, 6.0f ) );
		Set( "EditorModesToolbar.SToolBarCheckComboButtonBlock.Padding", FMargin( 0 ) );

		Set( "EditorModesToolbar.Block.IndentedPadding", FMargin( 0 ) );
		Set( "EditorModesToolbar.Block.Padding", FMargin( 0 ) );

		Set( "EditorModesToolbar.Separator", new BOX_BRUSH( "Old/Button", 4.0f / 32.0f ) );
		Set( "EditorModesToolbar.Separator.Padding", FMargin( 0.5f ) );

		Set( "EditorModesToolbar.Label.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 7 ) );
		Set( "EditorModesToolbar.EditableText.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "EditorModesToolbar.Keybinding.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );

		Set( "EditorModesToolbar.Heading.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "EditorModesToolbar.Heading.ColorAndOpacity", FLinearColor( 0.4f, 0.4, 0.4f, 1.0f ) );

		/* Create style for "EditorModesToolbar.CheckBox" ... */
		const FCheckBoxStyle EditorModesToolbarCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox", Icon14x14 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked",  Icon14x14 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/SmallCheckbox_Checked_Hovered", Icon14x14 ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheckbox_Hovered", Icon14x14, FLinearColor( 0.5f, 0.5f, 0.5f ) ) );
		/* ... and set new style */
		Set( "EditorModesToolbar.CheckBox", EditorModesToolbarCheckBoxStyle );

		// Read-only checkbox that appears next to a menu item
		/* Create style for "EditorModesToolbar.Check" ... */
		const FCheckBoxStyle EditorModesToolbarCheckStyle = FCheckBoxStyle()
			.SetUncheckedImage(IMAGE_BRUSH( "Icons/Empty_14x", Icon14x14 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) )
			.SetUncheckedHoveredImage( FSlateNoResource() )
			.SetUncheckedPressedImage( FSlateNoResource() )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/SmallCheck",  Icon14x14 ) );
		/*  ... and set new style  */
		Set( "EditorModesToolbar.Check", EditorModesToolbarCheckStyle );

		// This radio button is actually just a check box with different images
		/* Create style for "EditorModesToolbar.RadioButton" ... */
		const FCheckBoxStyle EditorModesToolbarRadioButtonStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16 ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x",  Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Unselected_16x", Icon16x16, SelectionColor_Pressed ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/RadioButton_Selected_16x", Icon16x16, SelectionColor_Pressed ) );
		/* ... and add new style */
		Set( "EditorModesToolbar.RadioButton", EditorModesToolbarRadioButtonStyle );

		/* Create style for "EditorModesToolbar.ToggleButton" ... */
		const FCheckBoxStyle EditorModesToolbarToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType( ESlateCheckBoxType::ToggleButton )
			.SetUncheckedImage( BOX_BRUSH( "/EditorModes/Tab_Inactive", 4 / 16.0f ) )
			.SetUncheckedPressedImage( BOX_BRUSH( "/EditorModes/Tab_Active", 4 / 16.0f ) )
			.SetUncheckedHoveredImage( BOX_BRUSH( "/EditorModes/Tab_Active", 4 / 16.0f ) )
			.SetCheckedHoveredImage( BOX_BRUSH( "/EditorModes/Tab_Active", 4 / 16.0f ) )
			.SetCheckedPressedImage( BOX_BRUSH( "/EditorModes/Tab_Active", 4 / 16.0f ) )
			.SetCheckedImage( BOX_BRUSH( "/EditorModes/Tab_Active", 4 / 16.0f ) );

		/* ... and add new style */
		Set( "EditorModesToolbar.ToggleButton", EditorModesToolbarToggleButtonStyle );

		Set( "EditorModesToolbar.Button", FButtonStyle( Button )
			.SetNormal( FSlateNoResource() )
			.SetPressed( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) )
			.SetHovered( BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor ) )
			);

		Set( "EditorModesToolbar.Button.Normal", new FSlateNoResource() );
		Set( "EditorModesToolbar.Button.Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) );
		Set( "EditorModesToolbar.Button.Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor ) );

		Set( "EditorModesToolbar.Button.Checked", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) );
		Set( "EditorModesToolbar.Button.Checked_Hovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed ) );
		Set( "EditorModesToolbar.Button.Checked_Pressed", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor ) );

	{
			Set( "MultiBox.GenericToolBarIcon", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartSim_40x", Icon40x40 ) );
			Set( "MultiBox.GenericToolBarIcon.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartSim_40x", Icon20x20 ) );

			Set( "MultiBox.DeleteButton", FButtonStyle() 
				.SetNormal ( IMAGE_BRUSH( "/Docking/CloseApp_Hovered", Icon16x16 ) )
				.SetPressed( IMAGE_BRUSH( "/Docking/CloseApp_Pressed", Icon16x16 ) )
				.SetHovered( IMAGE_BRUSH( "/Docking/CloseApp_Hovered", Icon16x16 ) ) );
		}
	}
	}

void FSlateEditorStyle::FStyle::SetupPersonaStyle()
{
	// Persona
#if WITH_EDITOR
	{
		Set( "Persona.ConvertAnimationGraph", new IMAGE_BRUSH( "Old/Graph/ConvertIcon", Icon40x40 ) );
		Set( "AnimViewportMenu.TranslateMode", new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon32x32) );
		Set( "AnimViewportMenu.TranslateMode.Small", new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon16x16 ) );
		Set( "AnimViewportMenu.RotateMode", new IMAGE_BRUSH( "Icons/icon_rotate_40x", Icon32x32) );
		Set( "AnimViewportMenu.RotateMode.Small", new IMAGE_BRUSH( "Icons/icon_rotate_40x", Icon16x16 ) );
		Set( "Persona.ReimportAsset", new IMAGE_BRUSH( "Icons/reimport_12x", Icon12x12 ) );
		Set( "AnimViewportMenu.CameraFollow", new IMAGE_BRUSH( "Persona/Viewport/Camera_FollowBounds_40px", Icon32x32) );
		Set( "AnimViewportMenu.CameraFollow.Small", new IMAGE_BRUSH( "Persona/Viewport/Camera_FollowBounds_40px", Icon16x16 ) );
		Set( "SkeletonTree.SkeletonSocket", new IMAGE_BRUSH( "Persona/SkeletonTree/icon_SocketG_16px", Icon16x16) );
		Set( "SkeletonTree.MeshSocket", new IMAGE_BRUSH( "Persona/SkeletonTree/icon_SocketC_16px", Icon16x16) );
		Set( "AnimViewport.LocalSpaceEditing", new IMAGE_BRUSH( "Icons/icon_axis_local_16px", FVector2D( 16, 16 ) ) );
		Set( "AnimViewport.WorldSpaceEditing", new IMAGE_BRUSH( "Icons/icon_axis_world_16px", FVector2D( 16, 16 ) ) );
		Set( "AnimViewportMenu.SetShowNormals", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon40x40 ) );
		Set( "AnimViewportMenu.SetShowNormals.Small", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Normals_40x"), Icon20x20 ) );
		Set( "AnimViewportMenu.SetShowTangents", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Tangents_40x"), Icon40x40 ) );
		Set( "AnimViewportMenu.SetShowTangents.Small", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Tangents_40x"), Icon20x20 ) );
		Set( "AnimViewportMenu.SetShowBinormals", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Binormals_40x"), Icon40x40 ) );
		Set( "AnimViewportMenu.SetShowBinormals.Small", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_Binormals_40x"), Icon20x20 ) );
		Set( "AnimViewportMenu.SetDrawUVs", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_UVOverlay_40x"), Icon40x40 ) );
		Set( "AnimViewportMenu.SetDrawUVs.Small", new IMAGE_BRUSH( TEXT("Icons/icon_StaticMeshEd_UVOverlay_40x"), Icon20x20 ) );

		Set( "AnimViewport.MessageFont", TTF_CORE_FONT("Fonts/Roboto-Bold", 9) );
		
		Set( "Persona.Viewport.BlueprintDirtyText", FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 18 ) )
			.SetColorAndOpacity( FLinearColor(0.8, 0.8f, 0.0f, 0.8f) )
			.SetShadowOffset( FVector2D( 1,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) )
			);
	}

	// Kismet 2
	{
		Set( "FullBlueprintEditor.SwitchToScriptingMode", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_EventGraph_40x", Icon40x40 ) );
		Set( "FullBlueprintEditor.SwitchToScriptingMode.Small", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_EventGraph_40x", Icon20x20 ) );
		Set( "FullBlueprintEditor.SwitchToBlueprintDefaultsMode", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Defaults_40x", Icon40x40 ) );
		Set( "FullBlueprintEditor.SwitchToBlueprintDefaultsMode.Small", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Defaults_40x", Icon20x20 ) );
		Set( "FullBlueprintEditor.SwitchToComponentsMode", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Components_40x", Icon40x40 ) );
		Set( "FullBlueprintEditor.SwitchToComponentsMode.Small", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Components_40x", Icon20x20 ) );

		Set( "FullBlueprintEditor.EditGlobalOptions", new IMAGE_BRUSH( "Icons/icon_Blueprint_Options_40px", Icon40x40));
		Set( "FullBlueprintEditor.EditGlobalOptions.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_Options_40px", Icon20x20));

		Set( "BlueprintEditor.Details.DeleteButton", new IMAGE_BRUSH( "/Icons/GenericDelete_Black", Icon16x16 ) );

		Set( "BlueprintEditor.Details.ArgUpButton", new IMAGE_BRUSH( "/Icons/icon_FunctionArgUp", Icon16x16 ) );
		Set( "BlueprintEditor.Details.ArgDownButton", new IMAGE_BRUSH( "/Icons/icon_FunctionArgDown", Icon16x16 ) );

		Set( "FullBlueprintEditor.Diff", new IMAGE_BRUSH( "Icons/BlueprintEditorDiff", Icon40x40 ) );
		Set( "FullBlueprintEditor.Diff.Small", new IMAGE_BRUSH( "Icons/BlueprintEditorDiff", Icon20x20 ) );

		Set( "BlueprintEditor.ActionMenu.ContextDescriptionFont",  FSlateFontInfo(TEXT("Roboto-Regular"), 12) );

		Set( "BlueprintEditor.FindInBlueprint", new IMAGE_BRUSH( "Icons/icon_Blueprint_Find_40px", Icon40x40 ) );
		Set( "BlueprintEditor.FindInBlueprint.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_Find_40px", Icon20x20 ) );

		Set( "BlueprintEditor.FindInBlueprints", new IMAGE_BRUSH( "Icons/icon_FindInAnyBlueprint_40px", Icon40x40 ) );
		Set( "BlueprintEditor.FindInBlueprints.Small", new IMAGE_BRUSH( "Icons/icon_FindInAnyBlueprint_40px", Icon20x20 ) );

		Set( "Kismet.CompileBlueprint", new IMAGE_BRUSH("/Icons/icon_kismet_compile_16px", Icon16x16) );
		Set( "Kismet.DeleteUnusedVariables", new IMAGE_BRUSH("/Icons/icon_kismet_findunused_16px", Icon16x16) );

		Set( "Kismet.Toolbar.SelectedDebugObject.Background", new IMAGE_BRUSH( "Old/Kismet2/DebugObject_Background", Icon40x40) );

		{
			Set( "Kismet.Tabs.Variables", new IMAGE_BRUSH( "/Icons/pill_16x", Icon16x16 ) );
			Set( "Kismet.Tabs.Explorer", new IMAGE_BRUSH( "/Icons/hiererchy_16x", Icon16x16 ) );
			Set( "Kismet.Tabs.Palette", new IMAGE_BRUSH( "/Icons/hiererchy_16x", Icon16x16 ) );
			Set( "Kismet.Tabs.CompilerResults", new IMAGE_BRUSH( "/Icons/hiererchy_16x", Icon16x16 ) );
			Set( "Kismet.Tabs.FindResults", new IMAGE_BRUSH( "/Icons/lens_16x", Icon16x16 ) );
			Set( "Kismet.Tabs.Components", new IMAGE_BRUSH( "Graph/Icons/Hammer", Icon16x16 ) );
		}

		Set("Kismet.Palette.Favorites", new IMAGE_BRUSH("Icons/Star_16x", Icon16x16, FLinearColor(0.4f, 0.4, 0.4f, 1.f)));
		Set("Kismet.Palette.Library",   new IMAGE_BRUSH("Icons/icon_MeshPaint_Find_16x", Icon16x16, FLinearColor(0.4f, 0.4, 0.4f, 1.f)));
		const FCheckBoxStyle KismetFavoriteToggleStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
			.SetUncheckedImage( IMAGE_BRUSH("Icons/EmptyStar_16x", Icon16x16, FLinearColor(0.8f, 0.8f, 0.8f, 1.f)) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH("Icons/EmptyStar_16x", Icon16x16, FLinearColor(2.5f, 2.5f, 2.5f, 1.f)) )
			.SetUncheckedPressedImage( IMAGE_BRUSH("Icons/EmptyStar_16x", Icon16x16, FLinearColor(0.8f, 0.8f, 0.8f, 1.f)) )
			.SetCheckedImage( IMAGE_BRUSH("Icons/Star_16x", Icon16x16, FLinearColor(0.2f, 0.2f, 0.2f, 1.f)) )
			.SetCheckedHoveredImage( IMAGE_BRUSH("Icons/Star_16x", Icon16x16, FLinearColor(0.4f, 0.4f, 0.4f, 1.f)) )
			.SetCheckedPressedImage( IMAGE_BRUSH("Icons/Star_16x", Icon16x16, FLinearColor(0.2f, 0.2f, 0.2f, 1.f)) );
		Set("Kismet.Palette.FavoriteToggleStyle", KismetFavoriteToggleStyle);

		Set( "Kismet.Tooltip.SubtextFont", TTF_CORE_FONT("Fonts/Roboto-Regular", 8) );

		Set( "Kismet.Status.Unknown", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Working", Icon40x40 ) );
		Set( "Kismet.Status.Error", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Fail", Icon40x40 ) );
		Set( "Kismet.Status.Good", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Good", Icon40x40 ) );
		Set( "Kismet.Status.Warning", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Warning", Icon40x40 ) );

		Set( "BlueprintEditor.AddNewVariable", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddVariable_40px", Icon40x40) );
		Set( "BlueprintEditor.AddNewVariable.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddVariable_40px", Icon20x20) );
		Set( "BlueprintEditor.AddNewVariableButton", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddVariable_40px", Icon12x12) );
		Set( "BlueprintEditor.AddNewLocalVariable", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddVariable_40px", Icon40x40) );
		Set( "BlueprintEditor.AddNewLocalVariable.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddVariable_40px", Icon20x20) );
		Set( "BlueprintEditor.AddNewFunction", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddFunction_40px", Icon40x40) );
		Set( "BlueprintEditor.AddNewFunction.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddFunction_40px", Icon20x20) );
		Set( "BlueprintEditor.AddNewMacroDeclaration", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddMacro_40px", Icon40x40) );
		Set( "BlueprintEditor.AddNewMacroDeclaration.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddMacro_40px", Icon20x20) );
		Set( "BlueprintEditor.AddNewAnimationGraph", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_AddDocument_40x", Icon40x40) );
		Set( "BlueprintEditor.AddNewAnimationGraph.Small", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_AddDocument_40x", Icon20x20) );
		Set( "BlueprintEditor.AddNewEventGraph", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddGraph_40px", Icon40x40) );
		Set( "BlueprintEditor.AddNewEventGraph.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddGraph_40px", Icon20x20) );
		Set( "BlueprintEditor.ManageInterfaces", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Interfaces_40x", Icon40x40) );	
		Set( "BlueprintEditor.ManageInterfaces.Small", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Interfaces_40x", Icon20x20) );	
		Set( "BlueprintEditor.AddNewDelegate.Small", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddDelegate_40x", Icon20x20) );
		Set( "BlueprintEditor.AddNewDelegate", new IMAGE_BRUSH( "Icons/icon_Blueprint_AddDelegate_40x", Icon40x40) );

		Set( "Kismet.Status.Unknown.Small", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Unknown_Small", Icon16x16 ) );
		Set( "Kismet.Status.Error.Small", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Broken_Small", Icon16x16 ) );
		Set( "Kismet.Status.Good.Small", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Good_Small", Icon16x16 ) );
		Set( "Kismet.Status.Warning.Small", new IMAGE_BRUSH( "Old/Kismet2/CompileStatus_Warning_Small", Icon16x16 ) );

		Set( "Kismet.TitleBarEditor.ArrowUp", new IMAGE_BRUSH( "Old/ArrowUp", Icon16x16 ) );
		Set( "Kismet.TitleBarEditor.ArrowDown", new IMAGE_BRUSH( "Old/ArrowDown", Icon16x16 ) );

		Set( "Kismet.VariableList.TypeIcon", new IMAGE_BRUSH( "/Icons/pill_16x", Icon16x16 ) );
		Set( "Kismet.VariableList.ArrayTypeIcon", new IMAGE_BRUSH( "/Icons/pillarray_16x", Icon16x16 ) );
		Set( "Kismet.VariableList.ExposeForInstance", new IMAGE_BRUSH( "/Icons/icon_layer_visible", Icon16x16 ) );
		Set( "Kismet.VariableList.HideForInstance", new IMAGE_BRUSH( "/Icons/icon_layer_not_visible", Icon16x16 ) );
		Set( "Kismet.VariableList.VariableIsUsed", new IMAGE_BRUSH( "/Icons/icon_variable_used_16x", Icon16x16 ) );
		Set( "Kismet.VariableList.VariableNotUsed", new IMAGE_BRUSH( "/Icons/icon_variable_not_used_16x", Icon16x16 ) );

		Set( "Kismet.VariableList.Replicated", new IMAGE_BRUSH( "/Icons/icon_replication_16px", Icon16x16, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "Kismet.VariableList.NotReplicated", new IMAGE_BRUSH( "/Icons/icon_replication_16px", Icon16x16, FLinearColor(0.0f, 0.0f, 0.0f, 0.5f) ) );

		Set( "Kismet.Explorer.Title", FTextBlockStyle(NormalText) .SetFont(TTF_FONT( "Fonts/Roboto-BoldCondensedItalic", 11)));
		Set( "Kismet.Explorer.SearchDepthFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 14) );

		Set( "Kismet.Interfaces.Title",  FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT("Fonts/Roboto-Bold", 11 ) ) );
		Set( "Kismet.Interfaces.Implement", new IMAGE_BRUSH( "Icons/assign_left_16x", Icon16x16) );
		Set( "Kismet.Interfaces.Remove", new IMAGE_BRUSH( "Icons/assign_right_16x", Icon16x16) );

		Set( "Kismet.TypePicker.CategoryFont", TTF_FONT( "Fonts/Roboto-BoldCondensedItalic", 11) );
		Set( "Kismet.TypePicker.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 11) );

		Set( "Kismet.GraphPicker.Title", FTextBlockStyle(NormalText) .SetFont( TTF_FONT("Fonts/Roboto-BoldCondensedItalic", 11) ) );

		Set( "Kismet.CreateBlueprint", new IMAGE_BRUSH( "/Icons/CreateBlueprint", Icon16x16) );
		Set( "Kismet.HarvestBlueprintFromActors", new IMAGE_BRUSH( "/Icons/HarvestBlueprintFromActors", Icon16x16) );

		Set( "Kismet.Comment.Handle", new IMAGE_BRUSH( "Old/Kismet2/Comment_Handle", FVector2D(14.0f, 12.0f)) );
		Set( "Kismet.Comment.Background", new IMAGE_BRUSH( "Old/Kismet2/Comment_Background", FVector2D(100.0f, 68.0f)) );

		Set( "Kismet.AllClasses.VariableIcon", new IMAGE_BRUSH( "/Icons/pill_16x", Icon16x16 ) );
		Set( "Kismet.AllClasses.ArrayVariableIcon", new IMAGE_BRUSH( "/Icons/pillarray_16x", Icon16x16 ) );
		Set( "Kismet.AllClasses.FunctionIcon", new IMAGE_BRUSH( "/Icons/icon_BluePrintEditor_Function_16px", Icon16x16 ) );

		Set( "BlueprintEditor.ResetCamera", new IMAGE_BRUSH( "Icons/icon_Camera_Reset_40px", Icon16x16));
		Set( "Kismet.SetRealtimePreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon16x16));
		Set( "BlueprintEditor.ShowFloor", new IMAGE_BRUSH( "Icons/icon_Show_Floor_40px", Icon16x16));
		Set( "BlueprintEditor.ShowGrid", new IMAGE_BRUSH( "Icons/icon_ShowGrid_16x", Icon16x16 ) );
		Set( "BlueprintEditor.EnableSimulation", new IMAGE_BRUSH( "Icons/icon_Enable_Simulation_40px", Icon40x40));
		Set( "BlueprintEditor.EnableSimulation.Small", new IMAGE_BRUSH( "Icons/icon_Enable_Simulation_40px", Icon20x20));
		Set( "SCS.NativeComponent", new IMAGE_BRUSH( "Icons/NativeSCSComponent", Icon20x20 ));
		Set( "SCS.Component", new IMAGE_BRUSH( "Icons/SCSComponent", Icon20x20 ));
	}

	// Kismet linear expression display
	{
		Set( "KismetExpression.ReadVariable.Body", new BOX_BRUSH( "/Graph/Linear_VarNode_Background", FMargin(16.f/64.f, 12.f/28.f) ) );
		Set( "KismetExpression.ReadVariable", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );
		Set( "KismetExpression.ReadVariable.Gloss", new BOX_BRUSH( "/Graph/Linear_VarNode_Gloss", FMargin(16.f/64.f, 12.f/28.f) ) );
		
		Set( "KismetExpression.ReadAutogeneratedVariable.Body", new BOX_BRUSH( "/Graph/Linear_VarNode_Background", FMargin(16.f/64.f, 12.f/28.f) ) );
		Set( "KismetExpression.ReadAutogeneratedVariable", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) ) );

		Set( "KismetExpression.OperatorNode", FTextBlockStyle(NormalText) .SetFont( TTF_FONT( "Fonts/Roboto-BoldCondensed", 20 ) ) );
		Set( "KismetExpression.FunctionNode", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) ) );
		Set( "KismetExpression.LiteralValue", FTextBlockStyle(NormalText) .SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) ) );

	}

	//Blueprint dif
	{
		Set( "BlueprintDif.HasGraph", new IMAGE_BRUSH( "/Icons/blueprint_dif_has_graph_8x", Icon8x8 )  );
		Set( "BlueprintDif.MissingGraph", new IMAGE_BRUSH( "/Icons/blueprint_dif_missing_graph_8x", Icon8x8 )  );
		Set( "BlueprintDif.NextDiff", new IMAGE_BRUSH( "/Icons/diff_next_40x", Icon16x16 )  );
		Set( "BlueprintDif.PrevDiff", new IMAGE_BRUSH( "/Icons/diff_prev_40x", Icon16x16 )  );
	}
	// Play in editor / play in world
	{
		Set( "PlayWorld.Simulate", new IMAGE_BRUSH( "Icons/icon_simulate_40x", Icon40x40 ) );
		Set( "PlayWorld.Simulate.Small", new IMAGE_BRUSH( "Icons/icon_simulate_40x", Icon20x20 ) );

		Set( "PlayWorld.RepeatLastPlay", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon40x40 ) );
		Set( "PlayWorld.RepeatLastPlay.Small", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon20x20 ) );
		Set( "PlayWorld.PlayInViewport", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon40x40 ) );
		Set( "PlayWorld.PlayInViewport.Small", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon20x20 ) );
		Set( "PlayWorld.PlayInEditorFloating", new IMAGE_BRUSH( "Icons/icon_playInWindow_40x", Icon40x40 ) );
		Set( "PlayWorld.PlayInEditorFloating.Small", new IMAGE_BRUSH( "Icons/icon_playInWindow_40x", Icon20x20 ) );
		Set( "PlayWorld.PlayInMobilePreview", new IMAGE_BRUSH( "Icons/icon_PlayMobilePreview_40x", Icon40x40 ) );
		Set( "PlayWorld.PlayInMobilePreview.Small", new IMAGE_BRUSH( "Icons/icon_PlayMobilePreview_16x", Icon20x20 ) );
		Set( "PlayWorld.PlayInNewProcess", new IMAGE_BRUSH( "Icons/icon_PlayStandalone_40x", Icon40x40 ) );
		Set( "PlayWorld.PlayInNewProcess.Small", new IMAGE_BRUSH( "Icons/icon_PlayStandalone_40x", Icon20x20 ) );
		Set( "PlayWorld.RepeatLastLaunch", new IMAGE_BRUSH( "Icons/icon_PlayOnDevice_40px", Icon40x40 ) );
		Set( "PlayWorld.RepeatLastLaunch.Small", new IMAGE_BRUSH( "Icons/icon_PlayOnDevice_40px", Icon20x20 ) );
		Set( "PlayWorld.PlayInCameraLocation", new IMAGE_BRUSH( "Icons/icon_PlayCameraLocation_40x", Icon40x40 ) );
		Set( "PlayWorld.PlayInDefaultPlayerStart", new IMAGE_BRUSH( "Icons/icon_PlayDefaultPlayerStart_40x", Icon40x40 ) );

		Set( "PlayWorld.ResumePlaySession", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon40x40 ) );
		Set( "PlayWorld.ResumePlaySession.Small", new IMAGE_BRUSH( "Icons/icon_play_40x", Icon20x20 ) );
		Set( "PlayWorld.PausePlaySession", new IMAGE_BRUSH( "Icons/icon_pause_40x", Icon40x40 ) );
		Set( "PlayWorld.PausePlaySession.Small", new IMAGE_BRUSH( "Icons/icon_pause_40x", Icon20x20 ) );
		Set( "PlayWorld.SingleFrameAdvance", new IMAGE_BRUSH( "Icons/icon_advance_40x", Icon40x40 ) );
		Set( "PlayWorld.SingleFrameAdvance.Small", new IMAGE_BRUSH( "Icons/icon_advance_40x", Icon20x20 ) );

		Set( "PlayWorld.StopPlaySession", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon40x40 ) );
		Set( "PlayWorld.StopPlaySession.Small", new IMAGE_BRUSH( "Icons/icon_stop_40x", Icon20x20 ) );

		Set( "PlayWorld.PossessPlayer", new IMAGE_BRUSH( "Icons/icon_possess_40x", Icon40x40 ) );
		Set( "PlayWorld.PossessPlayer.Small", new IMAGE_BRUSH( "Icons/icon_possess_40x", Icon20x20 ) );
		Set( "PlayWorld.EjectFromPlayer", new IMAGE_BRUSH( "Icons/icon_eject_40x", Icon40x40 ) );
		Set( "PlayWorld.EjectFromPlayer.Small", new IMAGE_BRUSH( "Icons/icon_eject_40x", Icon20x20 ) );

		Set( "PlayWorld.ShowCurrentStatement", new IMAGE_BRUSH( "Icons/icon_findnode_40x", Icon40x40 ) );
		Set( "PlayWorld.ShowCurrentStatement.Small", new IMAGE_BRUSH( "Icons/icon_findnode_40x", Icon20x20 ) );
		Set( "PlayWorld.StepInto", new IMAGE_BRUSH( "Icons/icon_step_40x", Icon40x40 ) );
		Set( "PlayWorld.StepInto.Small", new IMAGE_BRUSH( "Icons/icon_step_40x", Icon20x20 ) );
		Set( "PlayWorld.StepOver", new IMAGE_BRUSH( "Old/Kismet2/Debugger_StepOver", Icon40x40 ) );
	}


	// Kismet 2 debugger
	{
		Set( "Kismet.Breakpoint.Disabled", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Disabled_Small", Icon16x16 ) );
		Set( "Kismet.Breakpoint.EnabledAndInvalid", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Invalid_Small", Icon16x16 ) );
		Set( "Kismet.Breakpoint.EnabledAndValid", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Valid_Small", Icon16x16 ) );
		Set( "Kismet.Breakpoint.NoneSpacer", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_None_Small", Icon16x16 ) );
		Set( "Kismet.Breakpoint.MixedStatus", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Mixed_Small", Icon16x16 ) );
		
		Set( "Kismet.WatchIcon", new IMAGE_BRUSH( "Old/Kismet2/WatchIcon", Icon16x16 ) );
		Set( "Kismet.LatentActionIcon", new IMAGE_BRUSH( "Old/Kismet2/LatentActionIcon", Icon16x16 ) );

		Set( "Kismet.Trace.CurrentIndex", new IMAGE_BRUSH( "Old/Kismet2/CurrentInstructionOverlay_Small", Icon16x16 ) );
		Set( "Kismet.Trace.PreviousIndex", new IMAGE_BRUSH( "Old/Kismet2/FaintInstructionOverlay_Small", Icon16x16 ) );

		Set( "Kismet.DebuggerOverlay.Breakpoint.Disabled", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Disabled", Icon32x32 ) );
		Set( "Kismet.DebuggerOverlay.Breakpoint.EnabledAndInvalid", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Invalid", Icon32x32 ) );
		Set( "Kismet.DebuggerOverlay.Breakpoint.EnabledAndValid", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Valid", Icon32x32 ) );
		Set( "Kismet.DebuggerOverlay.Breakpoint.DisabledCollapsed", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Disabled_Collapsed", Icon32x32 ) );
		Set( "Kismet.DebuggerOverlay.Breakpoint.EnabledAndInvalidCollapsed", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Invalid_Collapsed", Icon32x32 ) );
		Set( "Kismet.DebuggerOverlay.Breakpoint.EnabledAndValidCollapsed", new IMAGE_BRUSH( "Old/Kismet2/Breakpoint_Valid_Collapsed", Icon32x32 ) );

		Set( "Kismet.DebuggerOverlay.InstructionPointer", new IMAGE_BRUSH( "Old/Kismet2/IP_Normal", FVector2D(128,96)) );
		Set( "Kismet.DebuggerOverlay.InstructionPointerBreakpoint", new IMAGE_BRUSH( "Old/Kismet2/IP_Breakpoint", FVector2D(128,96)) );
	}
#endif // WITH_EDITOR
}
	
void FSlateEditorStyle::FStyle::SetupClassIconsAndThumbnails()
{
#if WITH_EDITOR
	// Actor Classes Outliner
	{
		Set( "ClassIcon.Default", new IMAGE_BRUSH( "Icons/ActorIcons/Default_16x", Icon16x16 ) );
		Set( "ClassIcon.Emitter", new IMAGE_BRUSH( "Icons/ActorIcons/Emitter_16x", Icon16x16 ) );
		Set( "ClassIcon.Light", new IMAGE_BRUSH( "Icons/ActorIcons/LightActor_16x", Icon16x16 ) );
		Set( "ClassIcon.PointLight", new IMAGE_BRUSH( "Icons/ActorIcons/PointLight_16x", Icon16x16 ) );
		Set( "ClassIcon.SpotLight", new IMAGE_BRUSH( "Icons/ActorIcons/SpotLight_16x", Icon16x16 ) );
		Set( "ClassIcon.DirectionalLight", new IMAGE_BRUSH( "Icons/ActorIcons/DirectionalLight_16x", Icon16x16 ) );
		Set( "ClassIcon.MatineeActor", new IMAGE_BRUSH( "Icons/ActorIcons/MatineeActor_16x", Icon16x16 ) );
		Set( "ClassIcon.PlayerStart", new IMAGE_BRUSH( "Icons/ActorIcons/PlayerStart_16x", Icon16x16 ) );
		Set( "ClassIcon.AmbientSound", new IMAGE_BRUSH( "Icons/ActorIcons/SoundActor_16x", Icon16x16 ) );
		Set( "ClassIcon.TriggerBase", new IMAGE_BRUSH( "Icons/ActorIcons/TriggerActor_16x", Icon16x16 ) );
		Set( "ClassIcon.Brush", new IMAGE_BRUSH( "Icons/ActorIcons/Brush_16x", Icon16x16 ) );
		Set( "ClassIcon.BrushAdditive", new IMAGE_BRUSH( "Icons/ActorIcons/Brush_Add_16x", Icon16x16 ) );
		Set( "ClassIcon.BrushSubtractive", new IMAGE_BRUSH( "Icons/ActorIcons/Brush_Subtract_16x", Icon16x16 ) );
		Set( "ClassIcon.Volume", new IMAGE_BRUSH( "Icons/ActorIcons/Volume_16x", Icon16x16 ) );
		Set( "ClassIcon.BlockingVolume", new IMAGE_BRUSH( "Icons/ActorIcons/BlockingVolume_16x", Icon16x16 ) );
		Set( "ClassIcon.BlueprintActorBase", new IMAGE_BRUSH( "Icons/ActorIcons/Blueprint_16x", Icon16x16 ) );
		Set( "ClassIcon.StaticMeshActor", new IMAGE_BRUSH( "Icons/ActorIcons/StaticMesh_16x", Icon16x16 ) );
		Set( "ClassIcon.SkeletalMeshActor", new IMAGE_BRUSH( "Icons/ActorIcons/SkeletalMesh_16x", Icon16x16 ) );
		Set( "ClassIcon.CameraActor", new IMAGE_BRUSH( "Icons/ActorIcons/Camera_16x", Icon16x16 ) );
		Set( "ClassIcon.DecalActor", new IMAGE_BRUSH( "Icons/ActorIcons/Decal_16x", Icon16x16 ) );
		Set( "ClassIcon.SkeletalPhysicsActor", new IMAGE_BRUSH( "Icons/ActorIcons/SkeletalPhysicsActor_16x", Icon16x16 ) );
		Set( "ClassIcon.ExponentialHeightFog", new IMAGE_BRUSH( "Icons/ActorIcons/ExpoHeightFog_16x", Icon16x16 ) );
		Set( "ClassIcon.GroupActor", new IMAGE_BRUSH( "Icons/ActorIcons/Group_16x", Icon16x16 ) );
		Set( "ClassIcon.Landscape", new IMAGE_BRUSH( "Icons/ActorIcons/Landscape_16x", Icon16x16 ) );
		Set( "ClassIcon.RadialForceActor", new IMAGE_BRUSH( "Icons/ActorIcons/RadialForce_16x", Icon16x16 ) );
		Set( "ClassIcon.TargetPoint", new IMAGE_BRUSH( "Icons/ActorIcons/Target_16x", Icon16x16 ) );
		Set( "ClassIcon.VectorFieldVolume", new IMAGE_BRUSH( "Icons/ActorIcons/VectorFieldVolume_16x", Icon16x16 ) );
		Set( "ClassIcon.Deleted", new IMAGE_BRUSH( "Icons/ActorIcons/DeletedActor_16px", Icon16x16 ) );
		Set( "ClassIcon.DestructibleActor", new IMAGE_BRUSH( "Icons/ActorIcons/Destructable_16px", Icon16x16 ) );
		Set( "ClassIcon.SceneCapture", new IMAGE_BRUSH( "Icons/ActorIcons/SceneCapture_16px", Icon16x16 ) );
		Set( "ClassIcon.SceneCapture2D", new IMAGE_BRUSH( "Icons/ActorIcons/SceneCapture2D_16px", Icon16x16 ) );
		Set( "ClassIcon.TextRenderActor", new IMAGE_BRUSH( "Icons/ActorIcons/TextRenderer_16px", Icon16x16 ) );
		Set( "ClassIcon.BoxReflectionCapture", new IMAGE_BRUSH( "Icons/ActorIcons/BoxReflectionCapture_16px", Icon16x16 ) );
		Set( "ClassIcon.SphereReflectionCapture", new IMAGE_BRUSH( "Icons/ActorIcons/SphereReflectionCapture_16px", Icon16x16 ) );
		Set( "ClassIcon.DefaultPawn", new IMAGE_BRUSH( "Icons/ActorIcons/DefaultPawn_16px", Icon16x16 ) );
		Set( "ClassIcon.Pawn", new IMAGE_BRUSH( "Icons/ActorIcons/Pawn_16px", Icon16x16 ) );
		Set( "ClassIcon.Note", new IMAGE_BRUSH( "Icons/ActorIcons/Note_16px", Icon16x16 ) );
		Set( "ClassIcon.Character", new IMAGE_BRUSH( "Icons/ActorIcons/Character_16px", Icon16x16 ) );


		// Component classes
		Set( "ClassIcon.AudioComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SoundActor_16x", Icon16x16 ) );
		Set( "ClassIcon.CameraComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Camera_16x", Icon16x16 ) );
		Set( "ClassIcon.BlueprintCore", new IMAGE_BRUSH( "Icons/ActorIcons/Blueprint_16x", Icon16x16 ) );
		Set( "ClassIcon.BrushComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Brush_16x", Icon16x16 ) );
		Set( "ClassIcon.DecalComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Decal_16x", Icon16x16 ) );
		Set( "ClassIcon.DirectionalLightComponent", new IMAGE_BRUSH( "Icons/ActorIcons/DirectionalLight_16x", Icon16x16 ) );
		Set( "ClassIcon.ExponentialHeightFogComponent", new IMAGE_BRUSH( "Icons/ActorIcons/ExpoHeightFog_16x", Icon16x16 ) );
		Set( "ClassIcon.LandscapeComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Landscape_16x", Icon16x16 ) );
		Set( "ClassIcon.LightComponent", new IMAGE_BRUSH( "Icons/ActorIcons/LightActor_16x", Icon16x16 ) );
		Set( "ClassIcon.ParticleSystemComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Emitter_16x", Icon16x16 ) );
		Set( "ClassIcon.PointLightComponent", new IMAGE_BRUSH( "Icons/ActorIcons/PointLight_16x", Icon16x16 ) );
		Set( "ClassIcon.RB_RadialForceComponent", new IMAGE_BRUSH( "Icons/ActorIcons/RadialForce_16x", Icon16x16 ) );
		Set( "ClassIcon.SingleAnimSkeletalComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SkeletalMesh_16x", Icon16x16 ) );		
		Set( "ClassIcon.SkeletalMeshComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SkeletalMesh_16x", Icon16x16 ) );		
		Set( "ClassIcon.SpotLightComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SpotLight_16x", Icon16x16 ) );
		Set( "ClassIcon.StaticMeshComponent", new IMAGE_BRUSH( "Icons/ActorIcons/StaticMesh_16x", Icon16x16 ) );
		Set( "ClassIcon.VectorFieldComponent", new IMAGE_BRUSH( "Icons/ActorIcons/VectorFieldVolume_16x", Icon16x16 ) );
		Set( "ClassIcon.ArrowComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Arrow_16px", Icon16x16 ) );
		Set( "ClassIcon.AtmosphericFogComponent", new IMAGE_BRUSH( "Icons/ActorIcons/AtmosphericFog_16px", Icon16x16 ) );
		Set( "ClassIcon.BoxComponent", new IMAGE_BRUSH( "Icons/ActorIcons/box_16px", Icon16x16 ) );
		Set( "ClassIcon.CapsuleComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Capsule_16px", Icon16x16 ) );
		Set( "ClassIcon.DestructibleComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Destructable_16px", Icon16x16 ) );
		Set( "ClassIcon.InstancedStaticMeshComponent", new IMAGE_BRUSH( "Icons/ActorIcons/InstancedStaticMesh_16px", Icon16x16 ) );
		Set( "ClassIcon.MaterialBillboardComponent", new IMAGE_BRUSH( "Icons/ActorIcons/MaterialSprite_16px", Icon16x16 ) );
		Set( "ClassIcon.SceneCaptureComponent2D", new IMAGE_BRUSH( "Icons/ActorIcons/SceneCapture2D_16px", Icon16x16 ) );
		Set( "ClassIcon.SceneCaptureComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SceneCapture_16px", Icon16x16 ) );
		Set( "ClassIcon.SceneComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Scene_16px", Icon16x16 ) );
		Set( "ClassIcon.SphereComponent", new IMAGE_BRUSH( "Icons/ActorIcons/Sphere_16px", Icon16x16 ) );
		Set( "ClassIcon.BillboardComponent", new IMAGE_BRUSH( "Icons/ActorIcons/SpriteComponent_16px", Icon16x16 ) );
		Set( "ClassIcon.TextRenderComponent", new IMAGE_BRUSH( "Icons/ActorIcons/TextRenderer_16px", Icon16x16 ) );
		Set( "ClassIcon.TimelineComponent", new IMAGE_BRUSH("Icons/ActorIcons/TimelineComponent_16px", Icon16x16));
		Set( "ClassIcon.ChildActorComponent", new IMAGE_BRUSH( "Icons/ActorIcons/ChildActorComponent_16px", Icon16x16 ) );
		Set( "ClassIcon.ComponentMobilityStaticPip", new IMAGE_BRUSH( "Icons/ActorIcons/ComponentMobilityStationary_5x16px", Icon5x16, FLinearColor(0.f, 0.f, 0.f, 0.f) ) );
		Set( "ClassIcon.ComponentMobilityStationaryPip", new IMAGE_BRUSH( "Icons/ActorIcons/ComponentMobilityStationary_5x16px", Icon5x16 ) );
		Set( "ClassIcon.ComponentMobilityMovablePip", new IMAGE_BRUSH( "Icons/ActorIcons/ComponentMobilityMovable_5x16px", Icon5x16 ) );
		Set( "ClassIcon.MovableMobilityIcon", new IMAGE_BRUSH( "Icons/ActorIcons/Light_Movable_16x", Icon16x16));
		Set( "ClassIcon.StationaryMobilityIcon", new IMAGE_BRUSH( "Icons/ActorIcons/Light_Adjustable_16x", Icon16x16));
		Set( "ClassIcon.ComponentMobilityHeaderIcon", new IMAGE_BRUSH( "Icons/ActorIcons/ComponentMobilityHeader_5x16", Icon5x16));
		
		
		// Factory classes
		Set( "ClassIcon.ActorFactoryBoxVolume", new IMAGE_BRUSH( "Icons/icon_volume_Box_16x", Icon16x16 ) );
		Set( "ClassIcon.ActorFactoryCylinderVolume", new IMAGE_BRUSH( "Icons/icon_volume_cylinder_16x", Icon16x16 ) );
		Set( "ClassIcon.ActorFactorySphereVolume", new IMAGE_BRUSH( "Icons/icon_volume_sphere_16x", Icon16x16 ) );

		// Asset Type Classes
		Set( "ClassIcon.AimOffsetBlendSpace", new IMAGE_BRUSH( "Icons/AssetIcons/AimOffsetBlendSpace_16x", Icon16x16 ) );
		Set( "ClassIcon.AimOffsetBlendSpace1D", new IMAGE_BRUSH( "Icons/AssetIcons/AimOffsetBlendSpace1D_16x", Icon16x16 ) );
		Set( "ClassIcon.AnimBlueprint", new IMAGE_BRUSH( "Icons/AssetIcons/AnimBlueprint_16x", Icon16x16 ) );
		Set( "ClassIcon.AnimComposite", new IMAGE_BRUSH( "Icons/AssetIcons/AnimComposite_16x", Icon16x16 ) );
		Set( "ClassIcon.AnimMontage", new IMAGE_BRUSH( "Icons/AssetIcons/AnimMontage_16x", Icon16x16 ) );
		Set( "ClassIcon.BlendSpace", new IMAGE_BRUSH( "Icons/AssetIcons/BlendSpace_16x", Icon16x16 ) );
		Set( "ClassIcon.BlendSpace1D", new IMAGE_BRUSH( "Icons/AssetIcons/BlendSpace1D_16x", Icon16x16 ) );
		Set( "ClassIcon.Blueprint", new IMAGE_BRUSH( "Icons/AssetIcons/Blueprint_16x", Icon16x16 ) );
		Set( "ClassIcon.BlueprintGeneratedClass", new IMAGE_BRUSH( "Icons/AssetIcons/Blueprint_16x", Icon16x16 ) );
		Set( "ClassIcon.BlueprintInterface", new IMAGE_BRUSH( "Icons/AssetIcons/Blueprint_Interface_16x", Icon16x16 ) );
		Set( "ClassIcon.BlueprintMacroLibrary", new IMAGE_BRUSH( "Icons/AssetIcons/Blueprint_MacroLibrary_16x", Icon16x16 ) );
		Set( "ClassIcon.ButtonStyleAsset", new IMAGE_BRUSH( "Icons/AssetIcons/ButtonStyle_16x", Icon16x16 ) );
		Set( "ClassIcon.CameraAnim", new IMAGE_BRUSH( "Icons/ActorIcons/Camera_16x", Icon16x16 ) );
		Set( "ClassIcon.CurveBase", new IMAGE_BRUSH( "Icons/AssetIcons/Curve_16x", Icon16x16 ) );
		Set( "ClassIcon.DataAsset", new IMAGE_BRUSH( "Icons/AssetIcons/DataAsset_16x", Icon16x16 ) );
		Set( "ClassIcon.Font", new IMAGE_BRUSH( "Icons/AssetIcons/Font_16x", Icon16x16 ) );
		Set( "ClassIcon.InterpData", new IMAGE_BRUSH( "Icons/AssetIcons/MatineeData_16x", Icon16x16 ) );
		Set( "ClassIcon.Material", new IMAGE_BRUSH( "Icons/AssetIcons/Material_16x", Icon16x16 ) );
		Set( "ClassIcon.MaterialFunction", new IMAGE_BRUSH( "Icons/AssetIcons/MaterialFunction_16x", Icon16x16 ) );
		Set( "ClassIcon.MaterialInstanceConstant", new IMAGE_BRUSH( "Icons/AssetIcons/MaterialInstanceConstant_16x", Icon16x16 ) );
		Set( "ClassIcon.MaterialParameterCollection", new IMAGE_BRUSH( "Icons/AssetIcons/MaterialParameterCollection_16x", Icon16x16 ) );
		Set( "ClassIcon.MultiFont", new IMAGE_BRUSH( "Icons/AssetIcons/MultiFont_16x", Icon16x16 ) );
		Set( "ClassIcon.ObjectLibrary", new IMAGE_BRUSH( "Icons/AssetIcons/ObjectLibrary_16x", Icon16x16 ) );
		Set( "ClassIcon.PaperFlipbook", new IMAGE_BRUSH( "Icons/AssetIcons/PaperFlipbook_16x", Icon16x16 ) );
		Set( "ClassIcon.ParticleSystem", new IMAGE_BRUSH( "Icons/AssetIcons/ParticleSystem_16x", Icon16x16 ) );
		Set( "ClassIcon.PhysicalMaterial", new IMAGE_BRUSH( "Icons/AssetIcons/PhysicalMaterial_16x", Icon16x16 ) );
		Set( "ClassIcon.ReverbEffect", new IMAGE_BRUSH( "Icons/AssetIcons/ReverbEffect_16x", Icon16x16 ) );
		Set( "ClassIcon.SlateBrushAsset", new IMAGE_BRUSH( "Icons/AssetIcons/SlateBrush_16x", Icon16x16 ) );
		Set( "ClassIcon.SoundAttenuation", new IMAGE_BRUSH( "Icons/AssetIcons/SoundAttenuation_16x", Icon16x16 ) );
		Set( "ClassIcon.SoundClass", new IMAGE_BRUSH( "Icons/AssetIcons/SoundClass_16x", Icon16x16 ) );
		Set( "ClassIcon.SoundCue", new IMAGE_BRUSH( "Icons/AssetIcons/SoundCue_16x", Icon16x16 ) );
		Set( "ClassIcon.SoundMix", new IMAGE_BRUSH( "Icons/AssetIcons/SoundMix_16x", Icon16x16 ) );
		Set( "ClassIcon.StaticMesh", new IMAGE_BRUSH( "Icons/AssetIcons/StaticMesh_16x", Icon16x16 ) );
		Set( "ClassIcon.Texture", new IMAGE_BRUSH( "Icons/AssetIcons/Texture_16x", Icon16x16 ) );
		Set( "ClassIcon.Texture2D", new IMAGE_BRUSH( "Icons/AssetIcons/Texture2D_16x", Icon16x16 ) );
		Set( "ClassIcon.TextureRenderTarget2D", new IMAGE_BRUSH( "Icons/AssetIcons/TextureRenderTarget2D_16x", Icon16x16 ) );
		Set( "ClassIcon.TextureRenderTargetCube", new IMAGE_BRUSH( "Icons/AssetIcons/TextureRenderTargetCube_16x", Icon16x16 ) );
		Set( "ClassIcon.UserDefinedEnum", new IMAGE_BRUSH( "Icons/AssetIcons/Enumeration_16x", Icon16x16 ) );
		Set( "ClassIcon.SlateWidgetStyleAsset", new IMAGE_BRUSH("Icons/AssetIcons/SlateStyle_16x", Icon16x16 ) );

		// Builder brushes
		Set( "ClassIcon.CubeBuilder", new IMAGE_BRUSH("Icons/icon_box_40x", Icon16x16 ) );
		Set( "ClassIcon.ConeBuilder", new IMAGE_BRUSH("Icons/icon_cone_40x", Icon16x16 ) );
		Set( "ClassIcon.CylinderBuilder", new IMAGE_BRUSH("Icons/icon_cylinder_40x", Icon16x16 ) );
		Set( "ClassIcon.CurvedStairBuilder", new IMAGE_BRUSH("Icons/icon_curvedstair_40x", Icon16x16 ) );
		Set( "ClassIcon.LinearStairBuilder", new IMAGE_BRUSH("Icons/icon_stair_40x", Icon16x16 ) );
		Set( "ClassIcon.SpiralStairBuilder", new IMAGE_BRUSH("Icons/icon_spiralstair_40x", Icon16x16 ) );
		Set( "ClassIcon.VolumetricBuilder", new IMAGE_BRUSH("Icons/icon_volumetric_40x", Icon16x16 ) );
		Set( "ClassIcon.TetrahedronBuilder", new IMAGE_BRUSH("Icons/icon_sphere_40x", Icon16x16 ) );
		Set( "ClassIcon.SheetBuilder", new IMAGE_BRUSH("Icons/icon_plane_40x", Icon16x16 ) );
	}

	{
		Set( "ClassThumbnail.AmbientSound", new IMAGE_BRUSH( "Icons/icon_class_AmbientSound_64px", Icon64x64 ) );
		Set( "ClassThumbnail.BoxReflectionCapture", new IMAGE_BRUSH( "Icons/icon_class_BoxReflectionCapture_64px", Icon64x64 ) );
		Set( "ClassThumbnail.CameraActor", new IMAGE_BRUSH( "Icons/icon_class_CameraActor_64px", Icon64x64 ) );
		Set( "ClassThumbnail.DecalActor", new IMAGE_BRUSH( "Icons/icon_class_DecalActor_64px", Icon64x64 ) );
		Set( "ClassThumbnail.MatineeActor", new IMAGE_BRUSH( "Icons/icon_class_MatineeActor_64px", Icon64x64 ) );
		Set( "ClassThumbnail.PlaneReflectionCapture", new IMAGE_BRUSH( "Icons/icon_class_PlaneReflectionCapture_64px", Icon64x64 ) );
		Set( "ClassThumbnail.PlayerStart", new IMAGE_BRUSH( "Icons/icon_class_PlayerStart_64px", Icon64x64 ) );
		Set( "ClassThumbnail.SphereReflectionCapture", new IMAGE_BRUSH( "Icons/icon_class_SphereReflectionCapture_64px", Icon64x64 ) );
		Set( "ClassThumbnail.TextRenderActor", new IMAGE_BRUSH( "Icons/icon_class_TextRenderActor_64px", Icon64x64 ) );
		Set( "ClassThumbnail.TriggerBox", new IMAGE_BRUSH( "Icons/icon_class_TriggerBox_64px", Icon64x64 ) );
		Set( "ClassThumbnail.TriggerCapsule", new IMAGE_BRUSH( "Icons/icon_class_TriggerCapsule_64px", Icon64x64 ) );
		Set( "ClassThumbnail.TriggerSphere", new IMAGE_BRUSH( "Icons/icon_class_TriggerSphere_64px", Icon64x64 ) );

		Set( "ClassThumbnail.NavLinkProxy", new IMAGE_BRUSH( "Icons/icon_class_NavLinkProxy_64x", Icon64x64 ) );
		Set( "ClassThumbnail.Note", new IMAGE_BRUSH( "Icons/icon_class_Notes_64x", Icon64x64 ) );
		Set( "ClassThumbnail.RadialForceActor", new IMAGE_BRUSH( "Icons/icon_class_RadialForceActor_64x", Icon64x64 ) );
		Set( "ClassThumbnail.TargetPoint", new IMAGE_BRUSH( "Icons/icon_class_TargetPoint_64x", Icon64x64 ) );
		Set( "ClassThumbnail.AtmosphericFog", new IMAGE_BRUSH( "Icons/icon_class_AtmosphericFog_64x", Icon64x64 ) );
		Set( "ClassThumbnail.ExponentialHeightFog", new IMAGE_BRUSH( "Icons/icon_class_ExpHeightFog_64x", Icon64x64 ) );

		Set( "ClassThumbnail.TriggerBase", new IMAGE_BRUSH( "Icons/icon_class_TriggerBox_64px", Icon64x64 ) );
		Set( "ClassThumbnail.DirectionalLight", new IMAGE_BRUSH( "Icons/icon_class_DirectionalLight_64px", Icon64x64 ) );
		Set( "ClassThumbnail.PointLight", new IMAGE_BRUSH( "Icons/icon_class_PointLight_64px", Icon64x64 ) );
		Set( "ClassThumbnail.SpotLight", new IMAGE_BRUSH( "Icons/icon_class_SpotLight_64px", Icon64x64 ) );
		Set( "ClassThumbnail.SkyLight", new IMAGE_BRUSH( "Icons/icon_class_Skylight_64px", Icon64x64 ) ); 
		Set( "ClassThumbnail.ReflectionCapture", new IMAGE_BRUSH( "Icons/icon_class_SphereReflectionCapture_64px", Icon64x64 ) );

		Set( "ClassThumbnail.BlockingVolume", new IMAGE_BRUSH( "Icons/icon_volume_Blocking_64x", Icon64x64 ) );
		Set( "ClassThumbnail.CullDistanceVolume", new IMAGE_BRUSH( "Icons/icon_volume_CullDistance_64x", Icon64x64 ) );
		Set( "ClassThumbnail.LevelStreamingVolume", new IMAGE_BRUSH( "Icons/icon_volume_LevelStreaming_64x", Icon64x64 ) );
		Set( "ClassThumbnail.LightmassCharacterIndirectDetailVolume", new IMAGE_BRUSH( "Icons/icon_volume_LightmassCharacterIndirectDetail_64x", Icon64x64 ) );
		Set( "ClassThumbnail.LightmassImportanceVolume", new IMAGE_BRUSH( "Icons/icon_volume_LightmassImportance_64x", Icon64x64 ) );
		Set( "ClassThumbnail.MassiveLODOverrideVolume", new IMAGE_BRUSH( "Icons/icon_volume_MassiveLODOverride_64x", Icon64x64 ) );
		Set( "ClassThumbnail.NavMeshBoundsVolume", new IMAGE_BRUSH( "Icons/icon_volume_NavMeshBounds_64x", Icon64x64 ) );
		Set( "ClassThumbnail.NavModifierVolume", new IMAGE_BRUSH( "Icons/icon_volume_NavMeshModifier_64x", Icon64x64 ) );
		Set( "ClassThumbnail.PainCausingVolume", new IMAGE_BRUSH( "Icons/icon_volume_PainCausing_64x", Icon64x64 ) );
		Set( "ClassThumbnail.PhysicsVolume", new IMAGE_BRUSH( "Icons/icon_volume_Physics_64x", Icon64x64 ) );
		Set( "ClassThumbnail.PostProcessVolume", new IMAGE_BRUSH( "Icons/icon_volume_PostProcess_64x", Icon64x64 ) );
		Set( "ClassThumbnail.PrecomputedVisibilityOverrideVolume", new IMAGE_BRUSH( "Icons/icon_volume_precomputedVisibilityOverride_64x", Icon64x64 ) );
		Set( "ClassThumbnail.PrecomputedVisibilityVolume", new IMAGE_BRUSH( "Icons/icon_volume_PrecomputedVisibility_64x", Icon64x64 ) );
		Set( "ClassThumbnail.ReverbVolume", new IMAGE_BRUSH( "Icons/icon_volume_Reverb_64x", Icon64x64 ) );
		Set( "ClassThumbnail.ReverbVolumeToggleable", new IMAGE_BRUSH( "Icons/icon_volume_ReverbToggleable_64x", Icon64x64 ) );
		Set( "ClassThumbnail.TriggerVolume", new IMAGE_BRUSH( "Icons/icon_volume_Trigger_64x", Icon64x64 ) );

		// Asset Types
		Set( "ClassThumbnail.AimOffsetBlendSpace", new IMAGE_BRUSH( "Icons/icon_class_AimOffset_64x", Icon32x32 ) );
		Set( "ClassThumbnail.AimOffsetBlendSpace1D", new IMAGE_BRUSH( "Icons/icon_class_AimOffset1D_64x", Icon32x32 ) );
		Set( "ClassThumbnail.AnimBlueprint", new IMAGE_BRUSH( "Icons/icon_class_AnimBlueprint_64x", Icon32x32 ) );
		Set( "ClassThumbnail.AnimComposite", new IMAGE_BRUSH( "Icons/icon_class_AnimComposite_64x", Icon32x32 ) );
		Set( "ClassThumbnail.AnimMontage", new IMAGE_BRUSH( "Icons/icon_class_AnimMontage_64x", Icon32x32 ) );
		Set( "ClassThumbnail.BlendSpace", new IMAGE_BRUSH( "Icons/icon_class_BlendSpace_64x", Icon32x32 ) );
		Set( "ClassThumbnail.BlendSpace1D", new IMAGE_BRUSH( "Icons/icon_class_BlendSpace1D_64x", Icon32x32 ) );
		Set( "ClassThumbnail.Blueprint", new IMAGE_BRUSH( "Icons/icon_class_Blueprint_64x", Icon32x32 ) );
		Set( "ClassThumbnail.BlueprintGeneratedClass", new IMAGE_BRUSH( "Icons/icon_class_Blueprint_64x", Icon32x32 ) );
		Set( "ClassThumbnail.BlueprintInterface", new IMAGE_BRUSH( "Icons/icon_class_Blueprint_Interface_64x", Icon32x32 ) );
		Set( "ClassThumbnail.BlueprintMacroLibrary", new IMAGE_BRUSH( "Icons/icon_class_Blueprint_MacroLibrary_64x", Icon32x32 ) );
		Set( "ClassThumbnail.ButtonStyleAsset", new IMAGE_BRUSH( "Icons/icon_class_ButtonStyle_64x", Icon32x32 ) );
		Set( "ClassThumbnail.CameraAnim", new IMAGE_BRUSH( "Icons/icon_class_CameraAnim_64px", Icon32x32 ) );
		Set( "ClassThumbnail.CurveBase", new IMAGE_BRUSH( "Icons/icon_class_Curve_64x", Icon32x32 ) );
		Set( "ClassThumbnail.DataAsset", new IMAGE_BRUSH( "Icons/icon_class_DataAsset_64x", Icon32x32 ) );
		Set( "ClassThumbnail.Font", new IMAGE_BRUSH( "Icons/icon_class_Font_64x", Icon32x32 ) );
		Set( "ClassThumbnail.InterpData", new IMAGE_BRUSH( "Icons/icon_class_MatineeData_64x", Icon32x32 ) );
		Set( "ClassThumbnail.Material", new IMAGE_BRUSH( "Icons/icon_class_Material_64x", Icon32x32 ) );
		Set( "ClassThumbnail.MaterialFunction", new IMAGE_BRUSH( "Icons/icon_class_MaterialFunction_64x", Icon32x32 ) );
		Set( "ClassThumbnail.MaterialInstanceConstant", new IMAGE_BRUSH( "Icons/icon_class_MaterialInstanceConstant_64x", Icon32x32 ) );
		Set( "ClassThumbnail.MaterialParameterCollection", new IMAGE_BRUSH( "Icons/icon_class_MaterialParameterCollection_64x", Icon32x32 ) );
		Set( "ClassThumbnail.MultiFont", new IMAGE_BRUSH( "Icons/icon_class_MultiFont_64x", Icon32x32 ) );
		Set( "ClassThumbnail.ObjectLibrary", new IMAGE_BRUSH( "Icons/icon_class_ObjectLibrary_64x", Icon32x32 ) );
		Set( "ClassThumbnail.ParticleSystem", new IMAGE_BRUSH( "Icons/icon_class_ParticleSystem_64x", Icon32x32 ) );
		Set( "ClassThumbnail.PhysicalMaterial", new IMAGE_BRUSH( "Icons/icon_class_PhysicalMaterial_64x", Icon32x32 ) );
		Set( "ClassThumbnail.ReverbEffect", new IMAGE_BRUSH( "Icons/icon_class_ReverbEffect_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SlateBrushAsset", new IMAGE_BRUSH( "Icons/icon_class_SlateBrush_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SoundAttenuation", new IMAGE_BRUSH( "Icons/icon_class_SoundAttenuation_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SoundClass", new IMAGE_BRUSH( "Icons/icon_class_SoundClass_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SoundCue", new IMAGE_BRUSH( "Icons/icon_class_SoundCue_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SoundMix", new IMAGE_BRUSH( "Icons/icon_class_SoundMix_64x", Icon32x32 ) );
		Set( "ClassThumbnail.TextureRenderTarget2D", new IMAGE_BRUSH( "Icons/icon_class_TextureRenderTarget2D_64x", Icon32x32 ) );
		Set( "ClassThumbnail.TextureRenderTargetCube", new IMAGE_BRUSH( "Icons/icon_class_TextureRenderTargetCube_64x", Icon32x32 ) );
		Set( "ClassThumbnail.UserDefinedEnum", new IMAGE_BRUSH( "Icons/icon_class_Enumeration_64x", Icon32x32 ) );
		Set( "ClassThumbnail.SlateWidgetStyleAsset", new IMAGE_BRUSH("Icons/icon_class_SlateStyle_64x", Icon32x32 ) );
	}
#endif
}

void FSlateEditorStyle::FStyle::SetupContentBrowserStyle()
{
#if WITH_EDITOR
	// Content Browser
	{
		// Tab and menu icon
		Set( "ContentBrowser.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_ContentBrowser_16x", Icon16x16 ) );

		// Sources View
		Set( "ContentBrowser.SourceTitleFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "ContentBrowser.SourceListItemFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "ContentBrowser.SourceTreeItemFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "ContentBrowser.SourceTreeRootItemFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "ContentBrowser.AssetTreeFolderClosed", new IMAGE_BRUSH( "Icons/FolderClosed", FVector2D(18, 16) ) );
		Set( "ContentBrowser.BreadcrumbPathPickerFolder", new IMAGE_BRUSH( "Icons/FolderClosed", FVector2D(18, 16), FLinearColor::Gray ) );
		Set( "ContentBrowser.AssetTreeFolderOpen", new IMAGE_BRUSH( "Icons/FolderOpen", FVector2D(18, 16) ) );
		Set( "ContentBrowser.AssetTreeFolderDeveloper", new IMAGE_BRUSH( "Icons/FolderDeveloper", FVector2D(18, 16) ) );
		Set( "ContentBrowser.AddCollectionButtonIcon", new IMAGE_BRUSH( "Icons/PlusSymbol_12x", Icon12x12, FLinearColor::Black ) );

		Set( "ContentBrowser.Splitter", FSplitterStyle()
			.SetHandleNormalBrush( FSlateNoResource() )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/SplitterHandleHighlight", Icon8x8, FLinearColor::White ) )
			);

		// Asset list view
		Set( "ContentBrowser.AssetListViewNameFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "ContentBrowser.AssetListViewNameFontDirty", TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) );
		Set( "ContentBrowser.AssetListViewClassFont", TTF_CORE_FONT( "Fonts/Roboto-Light", 10 ) );

		// Asset picker
		Set("ContentBrowser.NoneButton", FButtonStyle(Button)
			.SetNormal(FSlateNoResource())
			.SetHovered(BOX_BRUSH( "Common/Selection", 8.0f/32.0f, SelectionColor ))
			.SetPressed(BOX_BRUSH( "Common/Selection", 8.0f/32.0f, SelectionColor_Pressed ))
		);
		Set( "ContentBrowser.NoneButtonText", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) )
			.SetColorAndOpacity( FLinearColor::White )
		);

		// Tile view
		Set( "ContentBrowser.AssetTileViewNameFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "ContentBrowser.AssetTileViewNameFontDirty", TTF_CORE_FONT( "Fonts/Roboto-Bold", 10 ) );
		Set( "ContentBrowser.AssetListView.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateNoResource() )
			.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetOddRowBackgroundBrush( FSlateNoResource() )
			.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) )
			.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
			.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);

		Set( "ContentBrowser.TileViewTooltip.ToolTipBorder", new FSlateColorBrush( FLinearColor::Black ) );
		Set( "ContentBrowser.TileViewTooltip.NonContentBorder", new BOX_BRUSH( "/Docking/TabContentArea", FMargin(4/16.0f) ) );
		Set( "ContentBrowser.TileViewTooltip.ContentBorder", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );
		Set( "ContentBrowser.TileViewTooltip.NameFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "ContentBrowser.TileViewTooltip.AssetUserDescriptionFont", TTF_CORE_FONT("Fonts/Roboto-Regular", 12 ) );

		// Columns view
		Set( "ContentBrowser.SortUp", new IMAGE_BRUSH( "Common/SortUpArrow", Icon8x4 ) );
		Set( "ContentBrowser.SortDown", new IMAGE_BRUSH( "Common/SortDownArrow", Icon8x4 ) );

		// Filter list
		/* Set images for various SCheckBox states associated with "ContentBrowser.FilterButton" ... */
		const FCheckBoxStyle ContentBrowserFilterButtonCheckBoxStyle = FCheckBoxStyle()
			.SetUncheckedImage( IMAGE_BRUSH( "ContentBrowser/FilterUnchecked", FVector2D( 10.0f,20.0f ) ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "ContentBrowser/FilterUnchecked", FVector2D( 10.0f, 20.0f ), FLinearColor( 0.5f, 0.5f, 0.5f, 1.0f ) ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "ContentBrowser/FilterUnchecked",FVector2D( 10.0f, 20.0f ), FLinearColor( 0.5f, 0.5f, 0.5f, 1.0f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "ContentBrowser/FilterChecked",  FVector2D( 10.0f, 20.0f ) ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "ContentBrowser/FilterChecked",  FVector2D( 10.0f, 20.0f ), FLinearColor( 0.5f, 0.5f, 0.5f, 1.0f ) ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "ContentBrowser/FilterChecked", FVector2D( 10.0f, 20.0f ), FLinearColor( 0.5f, 0.5f, 0.5f, 1.0f ) ) );
		/* ... and add the new style */
		Set( "ContentBrowser.FilterButton", ContentBrowserFilterButtonCheckBoxStyle );

		Set( "ContentBrowser.FilterNameFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) );
		Set( "ContentBrowser.FilterButtonBorder", new BOX_BRUSH( "Common/RoundedSelection_16x", FMargin(4.0f/16.0f) ) );

		// Thumbnail editing mode
		Set( "ContentBrowser.EditModeLabelFont", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Regular", 10 ) )
			.SetColorAndOpacity( FLinearColor::Black )
			.SetShadowOffset( FVector2D::ZeroVector )
		);

		Set( "ContentBrowser.EditModeLabelBorder", new FSlateColorBrush(SelectionColor_LinearRef) );
		Set( "ContentBrowser.PrimitiveCustom", new IMAGE_BRUSH( "ContentBrowser/ThumbnailCustom", Icon32x32 ) );
		Set( "ContentBrowser.PrimitiveSphere", new IMAGE_BRUSH( "ContentBrowser/ThumbnailSphere", Icon32x32 ) );
		Set( "ContentBrowser.PrimitiveCube", new IMAGE_BRUSH( "ContentBrowser/ThumbnailCube", Icon32x32 ) );
		Set( "ContentBrowser.PrimitivePlane", new IMAGE_BRUSH( "ContentBrowser/ThumbnailPlane", Icon32x32 ) );
		Set( "ContentBrowser.PrimitiveCylinder", new IMAGE_BRUSH( "ContentBrowser/ThumbnailCylinder", Icon32x32 ) );

		Set( "ContentBrowser.TopBar.Font", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		// New Asset
		Set( "ContentBrowser.NewAsset", new IMAGE_BRUSH( "Icons/icon_file_new_40x", Icon25x25 ) );

		FComboButtonStyle NewAssetComboButton = GetWidgetStyle<FComboButtonStyle>( "ToolbarComboButton" );
		NewAssetComboButton.SetContentPadding( 0 );
		Set( "ContentBrowser.NewAsset.Style", NewAssetComboButton );

		FComboButtonStyle FiltersComboButton = GetWidgetStyle<FComboButtonStyle>( "ToolbarComboButton" );
		FiltersComboButton.SetContentPadding( 0 );
		Set( "ContentBrowser.Filters.Style", FiltersComboButton );

		Set( "ContentBrowser.Filters.Text", FTextBlockStyle( NormalText )
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f, 0.9f ) )
			.SetShadowOffset( FVector2D( 1, 1 ) )
			.SetShadowColorAndOpacity( FLinearColor( 0, 0, 0, 0.9f ) ) );

		Set( "ContentBrowser.SaveDirtyPackages", new IMAGE_BRUSH( "Icons/icon_file_saveall_40x", Icon25x25 ) );
		Set( "ContentBrowser.ImportPackage", new IMAGE_BRUSH( "Icons/icon_Import_40x", Icon25x25 ) );

		// Misc
		Set( "ContentBrowser.ThumbnailShadow", new BOX_BRUSH( "ContentBrowser/ThumbnailShadow" , FMargin( 5.0f / 64.0f ) ) );
		Set( "AssetDiscoveryIndicator.DiscovertingAssetsFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 12 ) );
		Set( "ContentBrowser.ColumnViewAssetIcon", new IMAGE_BRUSH( "Icons/doc_16x", Icon16x16 ) );
		Set( "ContentBrowser.ColumnViewFolderIcon", new IMAGE_BRUSH( "Icons/FolderClosed", FVector2D(18, 16) ) );
		Set( "ContentBrowser.ColumnViewDeveloperFolderIcon", new IMAGE_BRUSH( "Icons/FolderDeveloper", FVector2D(18, 16) ) );
		Set( "ContentBrowser.ListViewFolderIcon.Base", new IMAGE_BRUSH( "Icons/Folders/Folder_Base_256x", FVector2D(256, 256) ) );
		Set( "ContentBrowser.ListViewFolderIcon.Mask", new IMAGE_BRUSH( "Icons/Folders/Folder_BaseHi_256x", FVector2D(256, 256) ) );
		Set( "ContentBrowser.ListViewDeveloperFolderIcon.Base", new IMAGE_BRUSH( "Icons/Folders/FolderDev_Base_256x", FVector2D(256, 256) ) );
		Set( "ContentBrowser.ListViewDeveloperFolderIcon.Mask", new IMAGE_BRUSH( "Icons/Folders/FolderDev_BaseHi_256x", FVector2D(256, 256) ) );
		Set( "ContentBrowser.TileViewFolderIcon.Base", new IMAGE_BRUSH( "Icons/Folders/Folder_Base_512x", FVector2D(512, 512) ) );
		Set( "ContentBrowser.TileViewFolderIcon.Mask", new IMAGE_BRUSH( "Icons/Folders/Folder_BaseHi_512x", FVector2D(512, 512) ) );
		Set( "ContentBrowser.TileViewDeveloperFolderIcon.Base", new IMAGE_BRUSH( "Icons/Folders/FolderDev_Base_512x", FVector2D(512, 512) ) );
		Set( "ContentBrowser.TileViewDeveloperFolderIcon.Mask", new IMAGE_BRUSH( "Icons/Folders/FolderDev_BaseHi_512x", FVector2D(512, 512) ) );
		Set( "ContentBrowser.PathText", FTextBlockStyle(NormalText)
			.SetFont( TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) )
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetHighlightColor( FLinearColor( 1.0f, 1.0f, 1.0f ) )
			.SetShadowOffset( FVector2D( 1,1 ) )
			.SetShadowColorAndOpacity( FLinearColor(0,0,0,0.9f) ) );
		Set( "ContentBrowser.Sources", new IMAGE_BRUSH( "ContentBrowser/sources_16x", Icon16x16 ) );
		Set( "ContentBrowser.PathDelimiter", new IMAGE_BRUSH( "Common/SmallArrowRight", Icon10x10 ) );
		Set( "ContentBrowser.LockButton_Locked", new IMAGE_BRUSH( "Icons/padlock_locked_16x", Icon16x16 ) );
		Set( "ContentBrowser.LockButton_Unlocked", new IMAGE_BRUSH( "Icons/padlock_unlocked_16x", Icon16x16 ) );
		Set( "ContentBrowser.ShowSourcesView", new IMAGE_BRUSH( "ContentBrowser/sourcestoggle_16x_collapsed", Icon16x16 ) );
		Set( "ContentBrowser.HideSourcesView", new IMAGE_BRUSH( "ContentBrowser/sourcestoggle_16x_expanded", Icon16x16 ) );
		Set( "ContentBrowser.HistoryBack", new IMAGE_BRUSH( "Icons/assign_left_16x", Icon16x16) );
		Set( "ContentBrowser.HistoryForward", new IMAGE_BRUSH("Icons/assign_right_16x", Icon16x16) );
		Set( "ContentBrowser.DirectoryUp", new IMAGE_BRUSH("Icons/icon_folder_up_16x", Icon16x16) );
		Set( "ContentBrowser.PathPickerButton", new IMAGE_BRUSH("Icons/ellipsis_12x", Icon12x12, FLinearColor::Black) );
		Set( "ContentBrowser.SCC_CheckedOut", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon32x32) );
		Set( "ContentBrowser.SCC_OpenForAdd", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon32x32) );
		Set( "ContentBrowser.SCC_CheckedOutByOtherUser", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon32x32) );
		Set( "ContentBrowser.SCC_NotAtHeadRevision", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon32x32) );
		Set( "ContentBrowser.SCC_NotInDepot", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentMissing", Icon32x32) );
		Set( "ContentBrowser.SCC_CheckedOut_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon16x16) );
		Set( "ContentBrowser.SCC_OpenForAdd_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon16x16) );
		Set( "ContentBrowser.SCC_CheckedOutByOtherUser_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon16x16) );
		Set( "ContentBrowser.SCC_NotAtHeadRevision_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon16x16) );
		Set( "ContentBrowser.SCC_NotInDepot_Small", new IMAGE_BRUSH("ContentBrowser/SCC_ContentMissing", Icon16x16) );
		Set( "ContentBrowser.ContentDirty", new IMAGE_BRUSH( "ContentBrowser/ContentDirty", Icon16x16) );
		Set( "ContentBrowser.AssetDragDropTooltipBackground", new BOX_BRUSH( "Old/Menu_Background", FMargin(8.0f/64.0f) ) );
		Set( "ContentBrowser.PopupMessageIcon", new IMAGE_BRUSH( "Icons/alert", Icon32x32) );
		Set( "ContentBrowser.NewFolderIcon", new IMAGE_BRUSH("Icons/icon_AddFolder_16x", Icon16x16 ) );
		Set( "ContentBrowser.Local", new IMAGE_BRUSH( "ContentBrowser/Content_Local_12x", Icon12x12 ) );
		Set( "ContentBrowser.Shared", new IMAGE_BRUSH( "ContentBrowser/Content_Shared_12x", Icon12x12 ) );
		Set( "ContentBrowser.Private", new IMAGE_BRUSH( "ContentBrowser/Content_Private_12x", Icon12x12 ) );
	}
#endif // #if WITH_EDITOR
}

void FSlateEditorStyle::FStyle::SetupLandscapeEditorStyle()
{
#if WITH_EDITOR
	// Landscape Editor
	{
		// Modes
		Set( "LandscapeEditor.ManageMode", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Manage_40x", Icon40x40 ) );
		Set( "LandscapeEditor.SculptMode", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Sculpt_40x", Icon40x40 ) );
		Set( "LandscapeEditor.PaintMode", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Paint_40x", Icon40x40 ) );
		Set( "LandscapeEditor.ManageMode.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Manage_20x", Icon20x20 ) );
		Set( "LandscapeEditor.SculptMode.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Sculpt_20x", Icon20x20 ) );
		Set( "LandscapeEditor.PaintMode.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Mode_Paint_20x", Icon20x20 ) );

		// Tools
		Set( "LandscapeEditor.NewLandscape", new IMAGE_BRUSH( "Icons/icon_Landscape_New_Landscape_40x", Icon40x40 ) );
		Set( "LandscapeEditor.NewLandscape.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_New_Landscape_20x", Icon20x20 ) );
		Set( "LandscapeEditor.ResizeLandscape", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Copy_40x", Icon40x40 ) );
		Set( "LandscapeEditor.ResizeLandscape.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Copy_20x", Icon20x20 ) );

		Set( "LandscapeEditor.SculptTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Sculpt_40x", Icon40x40 ) );
		Set( "LandscapeEditor.PaintTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Paint_40x", Icon40x40 ) );
		Set( "LandscapeEditor.SmoothTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Smooth_40x", Icon40x40 ) );
		Set( "LandscapeEditor.FlattenTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Flatten_40x", Icon40x40 ) );
		Set( "LandscapeEditor.RampTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Ramp_40x", Icon40x40 ) );
		Set( "LandscapeEditor.ErosionTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Erosion_40x", Icon40x40 ) );
		Set( "LandscapeEditor.HydroErosionTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_HydraulicErosion_40x", Icon40x40 ) );
		Set( "LandscapeEditor.NoiseTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Noise_40x", Icon40x40 ) );
		Set( "LandscapeEditor.RetopologizeTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Retopologize_40x", Icon40x40 ) );
		Set( "LandscapeEditor.VisibilityTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Visibility_40x", Icon40x40 ) );
		Set( "LandscapeEditor.SculptTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Sculpt_20x", Icon20x20 ) );
		Set( "LandscapeEditor.PaintTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Paint_20x", Icon20x20 ) );
		Set( "LandscapeEditor.SmoothTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Smooth_20x", Icon20x20 ) );
		Set( "LandscapeEditor.FlattenTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Flatten_20x", Icon20x20 ) );
		Set( "LandscapeEditor.RampTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Ramp_20x", Icon20x20 ) );
		Set( "LandscapeEditor.ErosionTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Erosion_20x", Icon20x20 ) );
		Set( "LandscapeEditor.HydroErosionTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_HydraulicErosion_20x", Icon20x20 ) );
		Set( "LandscapeEditor.NoiseTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Noise_20x", Icon20x20 ) );
		Set( "LandscapeEditor.RetopologizeTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Retopologize_20x", Icon20x20 ) );
		Set( "LandscapeEditor.VisibilityTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Visibility_20x", Icon20x20 ) );

		Set( "LandscapeEditor.SelectComponentTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Selection_40x", Icon40x40 ) );
		Set( "LandscapeEditor.AddComponentTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_AddComponent_40x", Icon40x40 ) );
		Set( "LandscapeEditor.DeleteComponentTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_DeleteComponent_40x", Icon40x40 ) );
		Set( "LandscapeEditor.MoveToLevelTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_MoveToLevel_40x", Icon40x40 ) );
		Set( "LandscapeEditor.SelectComponentTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Selection_20x", Icon20x20 ) );
		Set( "LandscapeEditor.AddComponentTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_AddComponent_20x", Icon20x20 ) );
		Set( "LandscapeEditor.DeleteComponentTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_DeleteComponent_20x", Icon20x20 ) );
		Set( "LandscapeEditor.MoveToLevelTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_MoveToLevel_20x", Icon20x20 ) );

		Set( "LandscapeEditor.RegionSelectTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Mask_40x", Icon40x40 ) );
		Set( "LandscapeEditor.RegionCopyPasteTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Copy_40x", Icon40x40 ) );
		Set( "LandscapeEditor.RegionSelectTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Mask_20x", Icon20x20 ) );
		Set( "LandscapeEditor.RegionCopyPasteTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Copy_20x", Icon20x20 ) );

		Set( "LandscapeEditor.SplineTool", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Splines_40x", Icon40x40 ) );
		Set( "LandscapeEditor.SplineTool.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Tool_Splines_20x", Icon20x20 ) );

		// Brush Sets
		Set( "LandscapeEditor.CircleBrush", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Smooth_40x", Icon40x40 ) );
		Set( "LandscapeEditor.AlphaBrush", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_40x", Icon40x40 ) );
		Set( "LandscapeEditor.AlphaBrush_Pattern", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Pattern_40x", Icon40x40 ) );
		Set( "LandscapeEditor.ComponentBrush", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Component_40x", Icon40x40 ) );
		Set( "LandscapeEditor.GizmoBrush", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Gizmo_40x", Icon40x40 ) );
		Set( "LandscapeEditor.CircleBrush.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Smooth_20x", Icon20x20 ) );
		Set( "LandscapeEditor.AlphaBrush.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_20x", Icon20x20 ) );
		Set( "LandscapeEditor.AlphaBrush_Pattern.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Pattern_20x", Icon20x20 ) );
		Set( "LandscapeEditor.ComponentBrush.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Component_20x", Icon20x20 ) );
		Set( "LandscapeEditor.GizmoBrush.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Gizmo_20x", Icon20x20 ) );

		// Brushes
		Set( "LandscapeEditor.CircleBrush_Smooth", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Smooth_40x", Icon40x40 ) );
		Set( "LandscapeEditor.CircleBrush_Linear", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Linear_40x", Icon40x40 ) );
		Set( "LandscapeEditor.CircleBrush_Spherical", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Spherical_40x", Icon40x40 ) );
		Set( "LandscapeEditor.CircleBrush_Tip", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Tip_40x", Icon40x40 ) );
		Set( "LandscapeEditor.CircleBrush_Smooth.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Smooth_20x", Icon20x20 ) );
		Set( "LandscapeEditor.CircleBrush_Linear.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Linear_20x", Icon20x20 ) );
		Set( "LandscapeEditor.CircleBrush_Spherical.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Spherical_20x", Icon20x20 ) );
		Set( "LandscapeEditor.CircleBrush_Tip.Small", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Circle_Tip_20x", Icon20x20 ) );

		Set( "LandscapeEditor.Brushes.Alpha.UseRChannel", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_UseRChannel_20x", Icon20x20 ) );
		Set( "LandscapeEditor.Brushes.Alpha.UseGChannel", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_UseGChannel_20x", Icon20x20 ) );
		Set( "LandscapeEditor.Brushes.Alpha.UseBChannel", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_UseBChannel_20x", Icon20x20 ) );
		Set( "LandscapeEditor.Brushes.Alpha.UseAChannel", new IMAGE_BRUSH( "Icons/icon_Landscape_Brush_Alpha_UseAChannel_20x", Icon20x20 ) );

		// Target List
		Set( "LandscapeEditor.TargetList.RowBackground",        new FSlateNoResource() );
		Set( "LandscapeEditor.TargetList.RowBackgroundHovered", new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.1f) ) );
		Set( "LandscapeEditor.TargetList.RowSelected",          new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ) );
		Set( "LandscapeEditor.TargetList.RowSelectedHovered",   new BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ) );
		
		Set( "LandscapeEditor.Target_Heightmap", new IMAGE_BRUSH( "Icons/icon_Landscape_Target_Heightmap_32x", Icon48x48 ) );
		Set( "LandscapeEditor.Target_Invalid", new IMAGE_BRUSH( "Icons/icon_Landscape_Target_Invalid_32x", Icon48x48 ) );

		Set( "LandscapeEditor.Target_Create", new IMAGE_BRUSH( "Icons/icon_Landscape_Target_Create_12x", Icon12x12 ) );
		Set( "LandscapeEditor.Target_MakePublic", new IMAGE_BRUSH( "Icons/assign_right_12x", Icon12x12 ) );
		Set( "LandscapeEditor.Target_Delete", new IMAGE_BRUSH( "Icons/cross_12x", Icon12x12 ) );
	}
#endif
}

void FSlateEditorStyle::FStyle::SetupToolkitStyles()
{
#if WITH_EDITOR
	// Project Browser
	{
		Set("ProjectBrowser.Tab.Text", FTextBlockStyle(NormalText)
			.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 24))
			.SetShadowOffset(FVector2D(0,1)));

		Set("ProjectBrowser.Background", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), FLinearColor(FColor(0xff404040))));
		Set("ProjectBrowser.Tab.ActiveBackground", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), FLinearColor(FColor(0xff404040))));
		Set("ProjectBrowser.Tab.Background", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), FLinearColor(FColor(0xff272727))));
		Set("ProjectBrowser.Tab.ActiveHighlight", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), SelectionColor) );
		Set("ProjectBrowser.Tab.Highlight", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), SelectionColor_Inactive));
		Set("ProjectBrowser.Tab.PressedHighlight", new BOX_BRUSH("Common/FlatColorSquare", FVector2D(1.0f, 1.0f), FMargin(0), SelectionColor_Pressed));
	}

	// Toolkit Display
	{
		Set( "ToolkitDisplay.UnsavedChangeIcon", new IMAGE_BRUSH( "Common/UnsavedChange", Icon8x8 ) );
		Set( "ToolkitDisplay.MenuDropdown", new IMAGE_BRUSH( "Common/ComboArrow", Icon8x8 ) );
		Set( "ToolkitDisplay.ColorOverlay", new BOX_BRUSH( "/Docking/Tab_ColorOverlay", 4/16.0f ) );
	}
	
	// Generic Editor
	{
		Set( "GenericEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
	}

	// CurveTable Editor
	{
		Set( "CurveTableEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/pillarray_16x", Icon16x16 ) );
	}

	// DataTable Editor
	{
		Set( "DataTableEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/pillarray_16x", Icon16x16 ) );
	}

	// DestructibleMesh Editor
	{
		Set( "DestructibleMeshEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/pillarray_16x", Icon16x16 ) );
		Set( "DestructibleMeshEditor.Fracture", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_Fracture_40x", Icon40x40 ) );
		Set( "DestructibleMeshEditor.Fracture.Small", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_Fracture_20x", Icon20x20 ) );
		Set( "DestructibleMeshEditor.Refresh", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_Refresh_40x", Icon40x40 ) );
		Set( "DestructibleMeshEditor.Refresh.Small", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_Refresh_40x", Icon20x20 ) );
		Set( "DestructibleMeshEditor.ImportFBXChunks", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_ImportFBX_40x", Icon40x40 ) );
		Set( "DestructibleMeshEditor.ImportFBXChunks.Small", new IMAGE_BRUSH( "Icons/icon_DestructibleMeshEd_ImportFBX_40x", Icon20x20 ) );

	}
#endif //#if WITH_EDITOR

	// Material Editor
#if WITH_EDITOR
	{
		Set( "MaterialEditor.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 9 ) );
		Set( "MaterialEditor.BoldFont", TTF_CORE_FONT( "Fonts/Roboto-Bold", 9 ) );
		
		Set( "MaterialEditor.Apply", new IMAGE_BRUSH( "Icons/icon_MatEd_Apply_40x", Icon40x40 ) );
		Set( "MaterialEditor.Apply.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Apply_40x", Icon20x20 ) );

		Set( "MaterialEditor.ShowAllMaterialParameters", new IMAGE_BRUSH( "Icons/icon_MatInsEd_Params_40x", Icon40x40 ) );
		Set( "MaterialEditor.ShowAllMaterialParameters.Small", new IMAGE_BRUSH( "Icons/icon_MatInsEd_Params_40x", Icon20x20 ) );

		Set( "MaterialEditor.SetCylinderPreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Cylinder_40x", Icon40x40 ) );
		Set( "MaterialEditor.SetCylinderPreview.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Cylinder_40x", Icon20x20 ) );
		Set( "MaterialEditor.SetSpherePreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Sphere_40x", Icon40x40 ) );
		Set( "MaterialEditor.SetSpherePreview.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Sphere_40x", Icon20x20 ) );
		Set( "MaterialEditor.SetPlanePreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Plane_40x", Icon40x40 ) );
		Set( "MaterialEditor.SetPlanePreview.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Plane_40x", Icon20x20 ) );
		Set( "MaterialEditor.SetCubePreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Cube_40x", Icon40x40 ) );
		Set( "MaterialEditor.SetCubePreview.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Cube_40x", Icon20x20 ) );
		Set( "MaterialEditor.SetPreviewMeshFromSelection", new IMAGE_BRUSH( "Icons/icon_MatEd_Mesh_40x", Icon40x40 ) );
		Set( "MaterialEditor.SetPreviewMeshFromSelection.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Mesh_40x", Icon20x20 ) );
		Set( "MaterialEditor.TogglePreviewGrid", new IMAGE_BRUSH( "Icons/icon_MatEd_Grid_40x", Icon40x40 ) );
		Set( "MaterialEditor.TogglePreviewGrid.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Grid_40x", Icon20x20 ) );

		Set( "MaterialEditor.ToggleStats", new IMAGE_BRUSH( "Icons/icon_MatEd_Stats_40x", Icon40x40 ) );
		Set( "MaterialEditor.ToggleStats.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Stats_40x", Icon20x20 ) );
		Set( "MaterialEditor.ToggleMobileStats", new IMAGE_BRUSH( "Icons/icon_tab_DeviceManager_40x", Icon40x40 ) );
		Set( "MaterialEditor.ToggleMobileStats.Small", new IMAGE_BRUSH( "Icons/icon_tab_DeviceManager_40x", Icon20x20 ) );
		Set( "MaterialEditor.CleanUnusedExpressions", new IMAGE_BRUSH( "Icons/icon_MatEd_CleanUp_40x", Icon40x40 ) );
		Set( "MaterialEditor.CleanUnusedExpressions.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_CleanUp_40x", Icon20x20 ) );
		Set( "MaterialEditor.ToggleRealtimeExpressions", new IMAGE_BRUSH( "Icons/icon_MatEd_LiveNodes_40x", Icon40x40 ) );
		Set( "MaterialEditor.ToggleRealtimeExpressions.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_LiveNodes_40x", Icon20x20 ) );
		Set( "MaterialEditor.AlwaysRefreshAllPreviews", new IMAGE_BRUSH( "Icons/icon_MatEd_Refresh_40x", Icon40x40 ) );
		Set( "MaterialEditor.AlwaysRefreshAllPreviews.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Refresh_40x", Icon20x20 ) );
		Set( "MaterialEditor.ShowHideConnectors", new IMAGE_BRUSH( "Icons/icon_MatEd_Connectors_40x", Icon40x40 ) );
		Set( "MaterialEditor.ShowHideConnectors.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Connectors_40x", Icon20x20 ) );
		Set( "MaterialEditor.CameraHome", new IMAGE_BRUSH( "Icons/icon_MatEd_Home_40x", Icon40x40 ) );
		Set( "MaterialEditor.CameraHome.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Home_40x", Icon20x20 ) );
	}

	// Material Instance Editor
	{
		Set( "MaterialInstanceEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		Set( "MaterialInstanceEditor.Tabs.Parents", new IMAGE_BRUSH( "/Icons/layers_16x", Icon16x16 ) );
	}

	// Sound Class Editor
	{
		Set( "SoundClassEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
	}

	// Font Editor
	{
		// Tab icons
		{
			Set( "FontEditor.Tabs.Preview", new IMAGE_BRUSH( "/Icons/lens_16x", Icon16x16 ) );
			Set( "FontEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
			Set( "FontEditor.Tabs.PageProperties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		}

		Set( "FontEditor.Update", new IMAGE_BRUSH( "Icons/icon_FontEd_Update_40x", Icon40x40 ) );
		Set( "FontEditor.Update.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_Update_40x", Icon20x20 ) );
		Set( "FontEditor.UpdateAll", new IMAGE_BRUSH( "Icons/icon_FontEd_UpdateAll_40x", Icon40x40 ) );
		Set( "FontEditor.UpdateAll.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_UpdateAll_40x", Icon20x20 ) );
		Set( "FontEditor.Export", new IMAGE_BRUSH( "Icons/icon_FontEd_Export_40x", Icon40x40 ) );
		Set( "FontEditor.Export.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_Export_40x", Icon20x20 ) );
		Set( "FontEditor.ExportAll", new IMAGE_BRUSH( "Icons/icon_FontEd_ExportAll_40x", Icon40x40 ) );
		Set( "FontEditor.ExportAll.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_ExportAll_40x", Icon20x20 ) );

		Set( "FontEditor.BackgroundColor", new IMAGE_BRUSH( "Icons/icon_FontEd_Background_40x", Icon40x40 ) );
		Set( "FontEditor.BackgroundColor.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_Background_40x", Icon20x20 ) );
		Set( "FontEditor.ForegroundColor", new IMAGE_BRUSH( "Icons/icon_FontEd_Foreground_40x", Icon40x40 ) );
		Set( "FontEditor.ForegroundColor.Small", new IMAGE_BRUSH( "Icons/icon_FontEd_Foreground_40x", Icon20x20 ) );
	}

	// SoundCueGraph Editor
	{
		Set("SoundCueGraphEditor.PlayCue", new IMAGE_BRUSH( "Icons/icon_SCueEd_PlayCue_40x", Icon40x40));
		Set("SoundCueGraphEditor.PlayCue.Small", new IMAGE_BRUSH( "Icons/icon_SCueEd_PlayCue_40x", Icon20x20));
		Set("SoundCueGraphEditor.PlayNode", new IMAGE_BRUSH( "Icons/icon_SCueEd_PlayNode_40x", Icon40x40));
		Set("SoundCueGraphEditor.PlayNode.Small", new IMAGE_BRUSH( "Icons/icon_SCueEd_PlayNode_40x", Icon20x20));
		Set("SoundCueGraphEditor.Stop", new IMAGE_BRUSH( "Icons/icon_SCueEd_Stop_40x", Icon40x40));
		Set("SoundCueGraphEditor.Stop.Small", new IMAGE_BRUSH( "Icons/icon_SCueEd_Stop_40x", Icon20x20));
	}

	// Static Mesh Editor
	{
		Set( "StaticMeshEditor.NormalFont", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "StaticMeshEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		Set( "StaticMeshEditor.SetShowWireframe", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Wireframe_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowWireframe.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Wireframe_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowVertexColor", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_VertColor_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowVertexColor.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_VertColor_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetRealtimePreview", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetRealtimePreview.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowBounds", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Bounds_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowBounds.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Bounds_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowCollision", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Collision_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowCollision.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Collision_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowGrid", new IMAGE_BRUSH( "Icons/icon_MatEd_Grid_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowGrid.Small", new IMAGE_BRUSH( "Icons/icon_MatEd_Grid_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetDrawUVs", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_UVOverlay_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetDrawUVs.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_UVOverlay_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.ResetCamera", new IMAGE_BRUSH( "Icons/icon_Camera_Reset_40px", Icon40x40 ) );
		Set( "StaticMeshEditor.ResetCamera.Small", new IMAGE_BRUSH( "Icons/icon_Camera_Reset_40px", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowPivot", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_ShowPivot_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowPivot.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_ShowPivot_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowSockets", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_ShowSockets_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowSockets.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_ShowSockets_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SaveThumbnail", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_SaveThumbnail_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SaveThumbnail.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_SaveThumbnail_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowNormals", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Normals_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowNormals.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Normals_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowTangents", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Tangents_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowTangents.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Tangents_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetShowBinormals", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Binormals_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetShowBinormals.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_Binormals_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.SetDrawAdditionalData", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_AdditionalData_40x", Icon40x40 ) );
		Set( "StaticMeshEditor.SetDrawAdditionalData.Small", new IMAGE_BRUSH( "Icons/icon_StaticMeshEd_AdditionalData_40x", Icon20x20 ) );
		Set( "StaticMeshEditor.GroupSection",  new BOX_BRUSH( "Common/RoundedSelection_16x", FMargin(4.0f/16.0f) ) );
	}

	// Skeletal Mesh Editor
	{
		Set( "SkeletalMeshEditor.GroupSection",  new BOX_BRUSH( "Common/RoundedSelection_16x", FMargin(4.0f/16.0f) ) );
	}

	// Texture Editor
	{
		Set( "TextureEditor.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		
		Set( "TextureEditor.RedChannel", new IMAGE_BRUSH( "Icons/icon_TextureEd_RedChannel_40x", Icon40x40 ) );
		Set( "TextureEditor.RedChannel.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_RedChannel_40x", Icon20x20 ) );
		Set( "TextureEditor.GreenChannel", new IMAGE_BRUSH( "Icons/icon_TextureEd_GreenChannel_40x", Icon40x40 ) );
		Set( "TextureEditor.GreenChannel.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_GreenChannel_40x", Icon20x20 ) );
		Set( "TextureEditor.BlueChannel", new IMAGE_BRUSH( "Icons/icon_TextureEd_BlueChannel_40x", Icon40x40 ) );
		Set( "TextureEditor.BlueChannel.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_BlueChannel_40x", Icon20x20 ) );
		Set( "TextureEditor.AlphaChannel", new IMAGE_BRUSH( "Icons/icon_TextureEd_AlphaChannel_40x", Icon40x40 ) );
		Set( "TextureEditor.AlphaChannel.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_AlphaChannel_40x", Icon20x20 ) );
		Set( "TextureEditor.Saturation", new IMAGE_BRUSH( "Icons/icon_TextureEd_Saturation_40x", Icon40x40 ) );
		Set( "TextureEditor.Saturation.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_Saturation_40x", Icon20x20 ) );

		Set( "TextureEditor.CompressNow", new IMAGE_BRUSH( "Icons/icon_TextureEd_CompressNow_40x", Icon40x40 ) );
		Set( "TextureEditor.CompressNow.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_CompressNow_40x", Icon20x20 ) );
		Set( "TextureEditor.Reimport", new IMAGE_BRUSH( "Icons/icon_TextureEd_Reimport_40x", Icon40x40 ) );
		Set( "TextureEditor.Reimport.Small", new IMAGE_BRUSH( "Icons/icon_TextureEd_Reimport_40x", Icon20x20 ) );
	}

	// Cascade
	{
		Set( "Cascade.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		
		Set( "Cascade.RestartSimulation", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartSim_40x", Icon40x40 ) );
		Set( "Cascade.RestartInLevel", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartInLevel_40x", Icon40x40 ) );
		Set( "Cascade.SaveThumbnailImage", new IMAGE_BRUSH( "Icons/icon_Cascade_Thumbnail_40x", Icon40x40 ) );
		Set( "Cascade.Undo", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon40x40 ) );
		Set( "Cascade.Redo", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon40x40 ) );
		Set( "Cascade.ToggleBounds", new IMAGE_BRUSH( "Icons/icon_Cascade_Bounds_40x", Icon40x40 ) );
		Set( "Cascade.ToggleOriginAxis", new IMAGE_BRUSH( "Icons/icon_Cascade_Axis_40x", Icon40x40 ) );
		Set( "Cascade.CascadeBackgroundColor", new IMAGE_BRUSH( "Icons/icon_Cascade_Color_40x", Icon40x40 ) );
		Set( "Cascade.RegenerateLowestLODDuplicatingHighest", new IMAGE_BRUSH( "Icons/icon_Cascade_RegenLOD1_40x", Icon40x40 ) );
		Set( "Cascade.RegenerateLowestLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_RegenLOD2_40x", Icon40x40 ) );
		Set( "Cascade.JumpToHighestLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_HighestLOD_40x", Icon40x40 ) );
		Set( "Cascade.JumpToHigherLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_HigherLOD_40x", Icon40x40 ) );
		Set( "Cascade.AddLODAfterCurrent", new IMAGE_BRUSH( "Icons/icon_Cascade_AddLOD1_40x", Icon40x40 ) );
		Set( "Cascade.AddLODBeforeCurrent", new IMAGE_BRUSH( "Icons/icon_Cascade_AddLOD2_40x", Icon40x40 ) );
		Set( "Cascade.JumpToLowerLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_LowerLOD_40x", Icon40x40 ) );
		Set( "Cascade.JumpToLowestLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_LowestLOD_40x", Icon40x40 ) );
		Set( "Cascade.DeleteLOD", new IMAGE_BRUSH( "Icons/icon_Cascade_DeleteLOD_40x", Icon40x40 ) );
		
		Set( "Cascade.RestartSimulation.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartSim_40x", Icon20x20 ) );
		Set( "Cascade.RestartInLevel.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartInLevel_40x", Icon20x20 ) );
		Set( "Cascade.SaveThumbnailImage.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_Thumbnail_40x", Icon20x20 ) );
		Set( "Cascade.Undo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon20x20 ) );
		Set( "Cascade.Redo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon20x20 ) );
		Set( "Cascade.ToggleBounds.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_Bounds_40x", Icon20x20 ) );
		Set( "Cascade.ToggleOriginAxis.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_Axis_40x", Icon20x20 ) );
		Set( "Cascade.CascadeBackgroundColor.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_Color_40x", Icon20x20 ) );
		Set( "Cascade.RegenerateLowestLODDuplicatingHighest.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_RegenLOD1_40x", Icon20x20 ) );
		Set( "Cascade.RegenerateLowestLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_RegenLOD2_40x", Icon20x20 ) );
		Set( "Cascade.JumpToHighestLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_HighestLOD_40x", Icon20x20 ) );
		Set( "Cascade.JumpToHigherLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_HigherLOD_40x", Icon20x20 ) );
		Set( "Cascade.AddLODAfterCurrent.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_AddLOD1_40x", Icon20x20 ) );
		Set( "Cascade.AddLODBeforeCurrent.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_AddLOD2_40x", Icon20x20 ) );
		Set( "Cascade.JumpToLowerLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_LowerLOD_40x", Icon20x20 ) );
		Set( "Cascade.JumpToLowestLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_LowestLOD_40x", Icon20x20 ) );
		Set( "Cascade.DeleteLOD.Small", new IMAGE_BRUSH( "Icons/icon_Cascade_DeleteLOD_40x", Icon20x20 ) );
	}

	// Level Script
	{
		Set( "LevelScript.Delete", new IMAGE_BRUSH( "Icons/icon_delete_16px", Icon16x16 ) );
	}

	// Curve Editor
	{
		Set( "CurveEditor.FitHorizontally", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Horizontal_40x", Icon40x40 ) );
		Set( "CurveEditor.FitVertically", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Vertical_40x", Icon40x40 ) );
		Set( "CurveEditor.FitToAll", new IMAGE_BRUSH( "Icons/icon_CurveEditor_All_40x", Icon40x40 ) );
		Set( "CurveEditor.FitToSelected", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Selected_40x", Icon40x40 ) );
		Set( "CurveEditor.PanMode", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Pan_40x", Icon40x40 ) );
		Set( "CurveEditor.ZoomMode", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Zoom_40x", Icon40x40 ) );
		Set( "CurveEditor.CurveAuto", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Auto_40x", Icon40x40 ) );
		Set( "CurveEditor.CurveAutoClamped", new IMAGE_BRUSH( "Icons/icon_CurveEditor_AutoClamped_40x", Icon40x40 ) );
		Set( "CurveEditor.CurveUser", new IMAGE_BRUSH( "Icons/icon_CurveEditor_User_40x", Icon40x40 ) );
		Set( "CurveEditor.CurveBreak", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Break_40x", Icon40x40 ) );
		Set( "CurveEditor.Linear", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Linear_40x", Icon40x40 ) );
		Set( "CurveEditor.Constant", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Constant_40x", Icon40x40 ) );
		Set( "CurveEditor.FlattenTangents", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Flatten_40x", Icon40x40 ) );
		Set( "CurveEditor.StraightenTangents", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Straighten_40x", Icon40x40 ) );
		Set( "CurveEditor.ShowAllTangents", new IMAGE_BRUSH( "Icons/icon_CurveEditor_ShowAll_40x", Icon40x40 ) );
		Set( "CurveEditor.CreateTab", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Create_40x", Icon40x40 ) );
		Set( "CurveEditor.DeleteTab", new IMAGE_BRUSH( "Icons/icon_CurveEditor_DeleteTab_40x", Icon40x40 ) );
		
		Set( "CurveEditor.FitHorizontally.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Horizontal_40x", Icon20x20 ) );
		Set( "CurveEditor.FitVertically.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Vertical_40x", Icon20x20 ) );
		Set( "CurveEditor.FitToAll.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_All_40x", Icon20x20 ) );
		Set( "CurveEditor.FitToSelected.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Selected_40x", Icon20x20 ) );
		Set( "CurveEditor.PanMode.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Pan_40x", Icon20x20 ) );
		Set( "CurveEditor.ZoomMode.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Zoom_40x", Icon20x20 ) );
		Set( "CurveEditor.CurveAuto.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Auto_40x", Icon20x20 ) );
		Set( "CurveEditor.CurveAutoClamped.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_AutoClamped_40x", Icon20x20 ) );
		Set( "CurveEditor.CurveUser.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_User_40x", Icon20x20 ) );
		Set( "CurveEditor.CurveBreak.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Break_40x", Icon20x20 ) );
		Set( "CurveEditor.Linear.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Linear_40x", Icon20x20 ) );
		Set( "CurveEditor.Constant.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Constant_40x", Icon20x20 ) );
		Set( "CurveEditor.FlattenTangents.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Flatten_40x", Icon20x20 ) );
		Set( "CurveEditor.StraightenTangents.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Straighten_40x", Icon20x20 ) );
		Set( "CurveEditor.ShowAllTangents.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_ShowAll_40x", Icon20x20 ) );
		Set( "CurveEditor.CreateTab.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_Create_40x", Icon20x20 ) );
		Set( "CurveEditor.DeleteTab.Small", new IMAGE_BRUSH( "Icons/icon_CurveEditor_DeleteTab_40x", Icon20x20 ) );
	}

	// PhAT
	{
		Set( "PhAT.Tabs.Properties", new IMAGE_BRUSH( "/Icons/properties_16x", Icon16x16 ) );
		Set( "PhAT.Tabs.Hierarchy", new IMAGE_BRUSH( "/Icons/hiererchy_16x", Icon16x16 ) );

		Set( "PhAT.EditingMode_Body", new IMAGE_BRUSH( "/Icons/icon_PHatMode_Body_40x", Icon40x40) );
		Set( "PhAT.EditingMode_Constraint", new IMAGE_BRUSH( "/Icons/icon_PHatMode_Joint_40x", Icon40x40) );

		Set( "PhAT.EditingMode_Body.Small", new IMAGE_BRUSH( "/Icons/icon_PHatMode_Body_40x", Icon20x20) );
		Set( "PhAT.EditingMode_Constraint.Small", new IMAGE_BRUSH( "/Icons/icon_PHatMode_Joint_40x", Icon20x20) );

		Set( "PhAT.SimulationNormal", new IMAGE_BRUSH( "Icons/icon_PhAT_PlaySim_40x", Icon40x40 ) );
		Set( "PhAT.SimulationNoGravity", new IMAGE_BRUSH( "Icons/icon_PhAT_PlaySimNoGravity_40x", Icon40x40 ) );
		Set( "PhAT.ToggleSelectedSimulation", new IMAGE_BRUSH( "Icons/icon_PhAT_PlaySimSelected_40x", Icon40x40 ) );
		Set( "PhAT.Undo", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon40x40 ) );
		Set( "PhAT.Redo", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon40x40 ) );
		Set( "PhAT.ChangeDefaultMesh", new IMAGE_BRUSH( "Icons/icon_PhAT_Mesh_40x", Icon40x40 ) );
		Set( "PhAT.ResetEntireAsset", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetAsset_40x", Icon40x40 ) );
		Set( "PhAT.RestetBoneCollision", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetCollision_40x", Icon40x40 ) );
		Set( "PhAT.ApplyPhysicalMaterial", new IMAGE_BRUSH( "Icons/icon_PhAT_Physmat_40x", Icon40x40 ) );
		Set( "PhAT.CopyJointSettings", new IMAGE_BRUSH( "Icons/icon_PhAT_CopyJoints_40x", Icon40x40 ) );
		Set( "PhAT.PlayAnimation", new IMAGE_BRUSH( "Icons/icon_PhAT_Play_40x", Icon40x40 ) );
		Set( "PhAT.TranslationMode", new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon40x40 ) );
		Set( "PhAT.RotationMode", new IMAGE_BRUSH( "Icons/icon_rotate_40x", Icon40x40 ) );
		Set( "PhAT.ScaleMode", new IMAGE_BRUSH( "Icons/icon_scale_40x", Icon40x40 ) );
		Set( "PhAT.Snap", new IMAGE_BRUSH( "Icons/icon_PhAT_Snap_40x", Icon40x40 ) );
		Set( "PhAT.CopyProperties", new IMAGE_BRUSH( "Icons/icon_PhAT_CopyProperties_40x", Icon40x40 ) );
		Set( "PhAT.DisableCollision", new IMAGE_BRUSH( "Icons/icon_PhAT_DisableCollision_40x", Icon40x40 ) );
		Set( "PhAT.EnableCollision", new IMAGE_BRUSH( "Icons/icon_PhAT_EnableCollision_40x", Icon40x40 ) );
		Set( "PhAT.WeldToBody", new IMAGE_BRUSH( "Icons/icon_PhAT_Weld_40x", Icon40x40 ) );
		Set( "PhAT.AddNewBody", new IMAGE_BRUSH( "Icons/icon_PhAT_NewBody_40x", Icon40x40 ) );
		Set( "PhAT.AddSphere", new IMAGE_BRUSH( "Icons/icon_PhAT_Sphere_40x", Icon40x40 ) );
		Set( "PhAT.AddSphyl", new IMAGE_BRUSH( "Icons/icon_PhAT_Sphyl_40x", Icon40x40 ) );
		Set( "PhAT.AddBox", new IMAGE_BRUSH( "Icons/icon_PhAT_Box_40x", Icon40x40 ) );
		Set( "PhAT.DeletePrimitive", new IMAGE_BRUSH( "Icons/icon_PhAT_DeletePrimitive_40x", Icon40x40 ) );
		Set( "PhAT.DuplicatePrimitive", new IMAGE_BRUSH( "Icons/icon_PhAT_DupePrim_40x", Icon40x40 ) );
		Set( "PhAT.ResetConstraint", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetConstraint_40x", Icon40x40 ) );
		Set( "PhAT.SnapConstraint", new IMAGE_BRUSH( "Icons/icon_PhAT_SnapConstraint_40x", Icon40x40 ) );
		Set( "PhAT.SnapAllConstraints", new IMAGE_BRUSH( "Icons/icon_PhAT_SnapAll_40x", Icon40x40 ) );
		Set( "PhAT.ConvertToBallAndSocket", new IMAGE_BRUSH( "Icons/icon_PhAT_Ball_40x", Icon40x40 ) );
		Set( "PhAT.ConvertToHinge", new IMAGE_BRUSH( "Icons/icon_PhAT_Hinge_40x", Icon40x40 ) );
		Set( "PhAT.ConvertToPrismatic", new IMAGE_BRUSH( "Icons/icon_PhAT_Prismatic_40x", Icon40x40 ) );
		Set( "PhAT.ConvertToSkeletal", new IMAGE_BRUSH( "Icons/icon_PhAT_Skeletal_40x", Icon40x40 ) );
		Set( "PhAT.DeleteConstraint", new IMAGE_BRUSH( "Icons/icon_PhAT_DeleteConstraint_40x", Icon40x40 ) );

		Set( "PhAT.ToggleSimulation.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_PlaySim_40x", Icon20x20 ) );
		Set( "PhAT.Undo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon20x20 ) );
		Set( "PhAT.Redo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon20x20 ) );
		Set( "PhAT.ChangeDefaultMesh.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Mesh_40x", Icon20x20 ) );
		Set( "PhAT.ResetEntireAsset.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetAsset_40x", Icon20x20 ) );
		Set( "PhAT.RestetBoneCollision.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetCollision_40x", Icon20x20 ) );
		Set( "PhAT.ApplyPhysicalMaterial.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Physmat_40x", Icon20x20 ) );
		Set( "PhAT.CopyJointSettings.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_CopyJoints_40x", Icon20x20 ) );
		Set( "PhAT.PlayAnimation.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Play_40x", Icon20x20 ) );
		Set( "PhAT.TranslationMode.Small", new IMAGE_BRUSH( "Icons/icon_translate_40x", Icon20x20 ) );
		Set( "PhAT.RotationMode.Small", new IMAGE_BRUSH( "Icons/icon_rotate_40x", Icon20x20 ) );
		Set( "PhAT.ScaleMode.Small", new IMAGE_BRUSH( "Icons/icon_scale_40x", Icon20x20 ) );
		Set( "PhAT.Snap.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Snap_40x", Icon20x20 ) );
		Set( "PhAT.CopyProperties.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_CopyProperties_40x", Icon20x20 ) );
		Set( "PhAT.DisableCollision.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_DisableCollision_40x", Icon20x20 ) );
		Set( "PhAT.EnableCollision.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_EnableCollision_40x", Icon20x20 ) );
		Set( "PhAT.WeldToBody.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Weld_40x", Icon20x20 ) );
		Set( "PhAT.AddNewBody.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_NewBody_40x", Icon20x20 ) );
		Set( "PhAT.AddSphere.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Sphere_40x", Icon20x20 ) );
		Set( "PhAT.AddSphyl.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Sphyl_40x", Icon20x20 ) );
		Set( "PhAT.AddBox.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Box_40x", Icon20x20 ) );
		Set( "PhAT.DeletePrimitive.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_DeletePrimitive_40x", Icon20x20 ) );
		Set( "PhAT.DuplicatePrimitive.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_DupePrim_40x", Icon20x20 ) );
		Set( "PhAT.ResetConstraint.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_ResetConstraint_40x", Icon20x20 ) );
		Set( "PhAT.SnapConstraint.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_SnapConstraint_40x", Icon20x20 ) );
		Set( "PhAT.SnapAllConstraints.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_SnapAll_40x", Icon20x20 ) );
		Set( "PhAT.ConvertToBallAndSocket.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Ball_40x", Icon20x20 ) );
		Set( "PhAT.ConvertToHinge.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Hinge_40x", Icon20x20 ) );
		Set( "PhAT.ConvertToPrismatic.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Prismatic_40x", Icon20x20 ) );
		Set( "PhAT.ConvertToSkeletal.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_Skeletal_40x", Icon20x20 ) );
		Set( "PhAT.DeleteConstraint.Small", new IMAGE_BRUSH( "Icons/icon_PhAT_DeleteConstraint_40x", Icon20x20 ) );
	}
#endif // WITH_EDITOR
}

void FSlateEditorStyle::FStyle::SetupMatineeStyle()
{
	//Matinee
#if WITH_EDITOR
	{	
		Set( "Matinee.AddKey", new IMAGE_BRUSH( "Icons/icon_Matinee_AddKey_40x", Icon40x40 ) );	
		Set( "Matinee.CreateMovie", new IMAGE_BRUSH( "Icons/icon_Matinee_CreateMovie_40x", Icon40x40 ) );	
		Set( "Matinee.Play", new IMAGE_BRUSH( "Icons/icon_Matinee_Play_40x", Icon40x40 ) );	
		Set( "Matinee.PlayLoop", new IMAGE_BRUSH( "Icons/icon_Matinee_PlayLoopSection_40x", Icon40x40 ) );	
		Set( "Matinee.Stop", new IMAGE_BRUSH( "Icons/icon_Matinee_Stop_40x", Icon40x40 ) );	
		Set( "Matinee.PlayReverse", new IMAGE_BRUSH( "Icons/icon_Matinee_PlayReverse_40x", Icon40x40 ) );	
		Set( "Matinee.Undo", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon40x40 ) );	
		Set( "Matinee.Redo", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon40x40 ) );	
		Set( "Matinee.ToggleSnap", new IMAGE_BRUSH( "Icons/icon_Matinee_ToggleSnap_40x", Icon40x40 ) );
		Set( "Matinee.FitSequence", new IMAGE_BRUSH( "Icons/icon_Matinee_FitSequence_40x", Icon40x40 ) );	
		Set( "Matinee.FitViewToSelected", new IMAGE_BRUSH( "Icons/icon_Matinee_FitSelected_40x", Icon40x40 ) );	
		Set( "Matinee.FitLoop", new IMAGE_BRUSH( "Icons/icon_Matinee_FitLoop_40x", Icon40x40 ) );	
		Set( "Matinee.FitLoopSequence", new IMAGE_BRUSH( "Icons/icon_Matinee_FitLoopSequnce_40x", Icon40x40 ) );	
		Set( "Matinee.ViewEndofTrack", new IMAGE_BRUSH( "Icons/icon_Matinee_EndOfTrack_40x", Icon40x40 ) );		
		Set( "Matinee.ToggleSnapTimeToFrames", new IMAGE_BRUSH( "Icons/icon_Matinee_SnapTimeToFrames_40x", Icon40x40 ) );	
		Set( "Matinee.FixedTimeStepPlayback", new IMAGE_BRUSH( "Icons/icon_Matinee_FixedTimeStepPlayback_40x", Icon40x40 ) );	
		Set( "Matinee.ToggleGorePreview", new IMAGE_BRUSH( "Icons/icon_Matinee_GorePreview_40x", Icon40x40 ) );	
		Set( "Matinee.CreateCameraActor", new IMAGE_BRUSH( "Icons/icon_Matinee_CreateCameraActor_40x", Icon40x40 ) );	
		Set( "Matinee.LaunchRecordWindow", new IMAGE_BRUSH( "Icons/icon_Matinee_LaunchRecorderWindow_40x", Icon40x40 ) );	

		Set( "Matinee.AddKey.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_AddKey_40x", Icon20x20 ) );	
		Set( "Matinee.CreateMovie.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_CreateMovie_40x", Icon20x20 ) );	
		Set( "Matinee.Play.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_Play_40x", Icon20x20 ) );	
		Set( "Matinee.PlayLoop.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_PlayLoopSection_40x", Icon20x20 ) );	
		Set( "Matinee.Stop.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_Stop_40x", Icon20x20 ) );	
		Set( "Matinee.PlayReverse.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_PlayReverse_40x", Icon20x20 ) );	
		Set( "Matinee.Undo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Undo_40x", Icon20x20 ) );	
		Set( "Matinee.Redo.Small", new IMAGE_BRUSH( "Icons/icon_Generic_Redo_40x", Icon20x20 ) );	
		Set( "Matinee.ToggleSnap.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_ToggleSnap_40x", Icon20x20 ) );
		Set( "Matinee.FitSequence.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_FitSequence_40x", Icon20x20 ) );	
		Set( "Matinee.FitViewToSelected.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_FitSelected_40x", Icon20x20 ) );	
		Set( "Matinee.FitLoop.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_FitLoop_40x", Icon20x20 ) );	
		Set( "Matinee.FitLoopSequence.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_FitLoopSequnce_40x", Icon20x20 ) );	
		Set( "Matinee.ViewEndofTrack.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_EndOfTrack_40x", Icon20x20 ) );		
		Set( "Matinee.ToggleSnapTimeToFrames.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_SnapTimeToFrames_40x", Icon20x20 ) );	
		Set( "Matinee.FixedTimeStepPlayback.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_FixedTimeStepPlayback_40x", Icon20x20 ) );	
		Set( "Matinee.ToggleGorePreview.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_GorePreview_40x", Icon20x20 ) );	
		Set( "Matinee.CreateCameraActor.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_CreateCameraActor_40x", Icon20x20 ) );	
		Set( "Matinee.LaunchRecordWindow.Small", new IMAGE_BRUSH( "Icons/icon_Matinee_LaunchRecorderWindow_40x", Icon20x20 ) );	
	}
#endif // WITH_EDITOR || IS_PROGRAM
	}

void FSlateEditorStyle::FStyle::SetupSourceControlStyles()
{
	//Source Control
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "SourceControl.Add", new IMAGE_BRUSH( "Old/SourceControl/SCC_Action_Add",Icon10x10));
		Set( "SourceControl.Edit", new IMAGE_BRUSH( "Old/SourceControl/SCC_Action_Edit",Icon10x10));
		Set( "SourceControl.Delete", new IMAGE_BRUSH( "Old/SourceControl/SCC_Action_Delete",Icon10x10));
		Set( "SourceControl.Branch", new IMAGE_BRUSH( "Old/SourceControl/SCC_Action_Branch",Icon10x10));
		Set( "SourceControl.Integrate", new IMAGE_BRUSH( "Old/SourceControl/SCC_Action_Integrate",Icon10x10));
		Set( "SourceControl.Settings.StatusBorder", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f), FLinearColor(0.5f,0.5f,0.5f,1.0f)  ) );
		Set( "SourceControl.Settings.StatusFont", FTextBlockStyle(NormalText).SetFont(TTF_CORE_FONT( "Fonts/Roboto-Bold", 12 ) ));
		Set( "SourceControl.StatusIcon.On", new IMAGE_BRUSH( "Icons/SourceControlOn_16x", Icon16x16 ) );
		Set( "SourceControl.StatusIcon.Error", new IMAGE_BRUSH( "Icons/SourceControlProblem_16x", Icon16x16 ) );
		Set( "SourceControl.StatusIcon.Off", new IMAGE_BRUSH( "Icons/SourceControlOff_16x", Icon16x16 ) );
		Set( "SourceControl.StatusIcon.Unknown", new IMAGE_BRUSH( "Icons/SourceControlUnknown_16x", Icon16x16 ) );
		Set( "SourceControl.LoginWindow.Font", TTF_CORE_FONT( "Fonts/Roboto-Regular", 8 ) );
		Set( "SourceControl.ProgressWindow.Warning", new IMAGE_BRUSH( "Icons/alert", Icon32x32) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Perforce
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "Perforce.CheckedOut", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon32x32) );
		Set( "Perforce.CheckedOut_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon16x16) );
		Set( "Perforce.OpenForAdd", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon32x32) );
		Set( "Perforce.OpenForAdd_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon16x16) );
		Set( "Perforce.CheckedOutByOtherUser", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon32x32) );
		Set( "Perforce.CheckedOutByOtherUser_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon16x16) );
		Set( "Perforce.NotAtHeadRevision", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon32x32) );
		Set( "Perforce.NotAtHeadRevision_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon16x16) );
		Set( "Perforce.NotInDepot", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentMissing", Icon32x32) );
		Set( "Perforce.NotInDepot_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentMissing", Icon16x16) );
	}
#endif // WITH_EDITOR || IS_PROGRAM

	// Subversion
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "Subversion.CheckedOut", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon32x32) );
		Set( "Subversion.CheckedOut_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOut", Icon16x16) );
		Set( "Subversion.OpenForAdd", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon32x32) );
		Set( "Subversion.OpenForAdd_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentAdd", Icon16x16) );
		Set( "Subversion.CheckedOutByOtherUser", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon32x32) );
		Set( "Subversion.CheckedOutByOtherUser_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_CheckedOutByOtherUser", Icon16x16) );
		Set( "Subversion.NotAtHeadRevision", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon32x32) );
		Set( "Subversion.NotAtHeadRevision_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_NotAtHeadRevision", Icon16x16) );
		Set( "Subversion.NotInDepot", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentMissing", Icon32x32) );
		Set( "Subversion.NotInDepot_Small", new IMAGE_BRUSH( "ContentBrowser/SCC_ContentMissing", Icon16x16) );
	}
#endif // WITH_EDITOR || IS_PROGRAM
	}

void FSlateEditorStyle::FStyle::SetupAutomationStyles()
{
	//Automation
#if WITH_EDITOR || IS_PROGRAM
	{
		Set( "Automation.Header" , FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/DroidSansMono", 12 ) )
			.SetColorAndOpacity(FLinearColor(FColor(0xffffffff))) );

		Set( "Automation.Normal" , FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/DroidSansMono", 9 ) )
			.SetColorAndOpacity(FLinearColor(FColor(0xffaaaaaa))) );

		Set( "Automation.Warning", FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/DroidSansMono", 9 ) )
			.SetColorAndOpacity(FLinearColor(FColor(0xffbbbb44))) );

		Set( "Automation.Error"  , FTextBlockStyle(NormalText)
			.SetFont( TTF_FONT( "Fonts/DroidSansMono", 9 ) )
			.SetColorAndOpacity(FLinearColor(FColor(0xffff0000))) );
		
		//state of individual tests
		Set( "Automation.Success", new IMAGE_BRUSH( "Automation/Success", Icon16x16 ) );
		Set( "Automation.Warning", new IMAGE_BRUSH( "Automation/Warning", Icon16x16 ) );
		Set( "Automation.Fail", new IMAGE_BRUSH( "Automation/Fail", Icon16x16 ) );
		Set( "Automation.InProcess", new IMAGE_BRUSH( "Automation/InProcess", Icon16x16 ) );
		Set( "Automation.NotRun", new IMAGE_BRUSH( "Automation/NotRun", Icon16x16, FLinearColor(0.0f, 0.0f, 0.0f, 0.4f) ) );
		Set( "Automation.NotEnoughParticipants", new IMAGE_BRUSH( "Automation/NotEnoughParticipants", Icon16x16 ) );
		Set( "Automation.ParticipantsWarning", new IMAGE_BRUSH( "Automation/ParticipantsWarning", Icon16x16 ) );
		Set( "Automation.Participant", new IMAGE_BRUSH( "Automation/participant", Icon16x16 ) );
		
		//status as a regression test or not
		Set( "Automation.SmokeTest", new IMAGE_BRUSH( "Automation/SmokeTest", Icon16x16 ) );
		Set( "Automation.SmokeTestParent", new IMAGE_BRUSH( "Automation/SmokeTestParent", Icon16x16 ) );

		//run icons
		Set( "AutomationWindow.RunTests", new IMAGE_BRUSH( "Automation/RunTests", Icon40x40) );
		Set( "AutomationWindow.RefreshTests", new IMAGE_BRUSH( "Automation/RefreshTests", Icon40x40) );
		Set( "AutomationWindow.FindWorkers", new IMAGE_BRUSH( "Automation/RefreshWorkers", Icon40x40) );
		Set( "AutomationWindow.StopTests", new IMAGE_BRUSH( "Automation/StopTests", Icon40x40 ) );
		Set( "AutomationWindow.RunTests.Small", new IMAGE_BRUSH( "Automation/RunTests", Icon20x20) );
		Set( "AutomationWindow.RefreshTests.Small", new IMAGE_BRUSH( "Automation/RefreshTests", Icon20x20) );
		Set( "AutomationWindow.FindWorkers.Small", new IMAGE_BRUSH( "Automation/RefreshWorkers", Icon20x20) );
		Set( "AutomationWindow.StopTests.Small", new IMAGE_BRUSH( "Automation/StopTests", Icon20x20 ) );

		//filter icons
		Set( "AutomationWindow.ErrorFilter", new IMAGE_BRUSH( "Automation/ErrorFilter", Icon40x40) );
		Set( "AutomationWindow.WarningFilter", new IMAGE_BRUSH( "Automation/WarningFilter", Icon40x40) );
		Set( "AutomationWindow.SmokeTestFilter", new IMAGE_BRUSH( "Automation/SmokeTestFilter", Icon40x40) );
		Set( "AutomationWindow.DeveloperDirectoryContent", new IMAGE_BRUSH( "Automation/DeveloperDirectoryContent", Icon40x40) );
		Set( "AutomationWindow.VisualCommandlet", new IMAGE_BRUSH( "Automation/VisualCommandlet", Icon40x40) );
		Set( "AutomationWindow.ErrorFilter.Small", new IMAGE_BRUSH( "Automation/ErrorFilter", Icon20x20) );
		Set( "AutomationWindow.WarningFilter.Small", new IMAGE_BRUSH( "Automation/WarningFilter", Icon20x20) );
		Set( "AutomationWindow.SmokeTestFilter.Small", new IMAGE_BRUSH( "Automation/SmokeTestFilter", Icon20x20) );
		Set( "AutomationWindow.DeveloperDirectoryContent.Small", new IMAGE_BRUSH( "Automation/DeveloperDirectoryContent", Icon20x20) );
		Set( "AutomationWindow.VisualCommandlet.Small", new IMAGE_BRUSH( "Automation/VisualCommandlet", Icon20x20) );
	}

	// Launcher
	{
		Set( "Launcher.Instance_Commandlet", new IMAGE_BRUSH( "Launcher/Instance_Commandlet", Icon25x25 ) );
		Set( "Launcher.Instance_Editor", new IMAGE_BRUSH( "Launcher/Instance_Editor", Icon25x25 ) );
		Set( "Launcher.Instance_Game", new IMAGE_BRUSH( "Launcher/Instance_Game", Icon25x25 ) );
		Set( "Launcher.Instance_Other", new IMAGE_BRUSH( "Launcher/Instance_Other", Icon25x25 ) );
		Set( "Launcher.Instance_Server", new IMAGE_BRUSH( "Launcher/Instance_Server", Icon25x25 ) );
		Set( "Launcher.Instance_Unknown", new IMAGE_BRUSH( "Launcher/Instance_Unknown", Icon25x25 ) );
		Set( "LauncherCommand.DeployBuild", new IMAGE_BRUSH( "Launcher/Launcher_Deploy", Icon40x40 ) );
		Set( "LauncherCommand.QuickLaunch", new IMAGE_BRUSH( "Launcher/Launcher_Launch", Icon40x40 ) );
		Set( "LauncherCommand.Build", new IMAGE_BRUSH( "Launcher/Launcher_Build", Icon40x40 ) );
		Set( "LauncherCommand.AdvancedBuild", new IMAGE_BRUSH( "Launcher/Launcher_Advanced", Icon40x40 ) );

#if (WITH_EDITOR || (IS_PROGRAM && PLATFORM_DESKTOP))
		const TArray<ITargetPlatform*>& TargetPlatforms = GetTargetPlatformManager()->GetTargetPlatforms();
		for (int32 PlatformIdx = 0; PlatformIdx < TargetPlatforms.Num(); PlatformIdx++)
		{
			ITargetPlatform* TargetPlatform = TargetPlatforms[PlatformIdx];
			if (TargetPlatform)
			{
				// Grab each icon name
				FString PropertyName = FString::Printf(TEXT("Launcher.Platform_%s"), *(TargetPlatform->PlatformName()));
				FString IconPath = TargetPlatform->GetIconPath(ETargetPlatformIcons::Normal);
				Set( *PropertyName, new IMAGE_BRUSH( *IconPath, Icon24x24 ) );

				PropertyName = FString::Printf(TEXT("Launcher.Platform_%s.Large"), *(TargetPlatform->PlatformName()));
				IconPath = TargetPlatform->GetIconPath(ETargetPlatformIcons::Large);
				Set( *PropertyName, new IMAGE_BRUSH( *IconPath, Icon64x64 ) );

				PropertyName = FString::Printf(TEXT("Launcher.Platform_%s.XLarge"), *(TargetPlatform->PlatformName()));
				IconPath = TargetPlatform->GetIconPath(ETargetPlatformIcons::XLarge);
				Set( *PropertyName, new IMAGE_BRUSH( *IconPath, Icon128x128 ) );
			}
		}
#endif
	}

	// Device Manager
	{
		Set( "DeviceDetails.Claim", new IMAGE_BRUSH( "Icons/icon_DeviceClaim_40x", Icon40x40 ) );
		Set( "DeviceDetails.Claim.Small", new IMAGE_BRUSH( "Icons/icon_DeviceClaim_40x", Icon20x20 ) );
		Set( "DeviceDetails.Release", new IMAGE_BRUSH( "Icons/icon_DeviceRelease_40x", Icon40x40 ) );
		Set( "DeviceDetails.Release.Small", new IMAGE_BRUSH( "Icons/icon_DeviceRelease_40x", Icon20x20 ) );
		Set( "DeviceDetails.Share", new IMAGE_BRUSH( "Icons/icon_DeviceShare_40x", Icon40x40 ) );
		Set( "DeviceDetails.Share.Small", new IMAGE_BRUSH( "Icons/icon_DeviceShare_40x", Icon20x20 ) );

		Set( "DeviceDetails.PowerOn", new IMAGE_BRUSH( "Icons/icon_DevicePowerOn_40x", Icon40x40 ) );
		Set( "DeviceDetails.PowerOn.Small", new IMAGE_BRUSH( "Icons/icon_DevicePowerOn_40x", Icon20x20 ) );
		Set( "DeviceDetails.PowerOff", new IMAGE_BRUSH( "Icons/icon_DevicePowerOff_40x", Icon40x40 ) );
		Set( "DeviceDetails.PowerOff.Small", new IMAGE_BRUSH( "Icons/icon_DevicePowerOff_40x", Icon20x20 ) );
		Set( "DeviceDetails.Reboot", new IMAGE_BRUSH( "Icons/icon_DeviceReboot_40x", Icon40x40 ) );
		Set( "DeviceDetails.Reboot.Small", new IMAGE_BRUSH( "Icons/icon_DeviceReboot_40x", Icon20x20 ) );

		Set( "DeviceDetails.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_DeviceManager_16x", Icon16x16 ) );
		Set( "DeviceDetails.Tabs.Tools", new IMAGE_BRUSH( "/Icons/icon_tab_Tools_16x", Icon16x16 ) );
		Set( "DeviceDetails.Tabs.ProfileEditor", new IMAGE_BRUSH( "/Icons/icon_tab_SceneOutliner_16x", Icon16x16 ) );
	}

	// Settings Editor
	{
		Set( "SettingsEditor.Category_ContentEditors", new IMAGE_BRUSH( "Icons/icon_ViewMode_VisualisationGBuffer_40px", Icon40x40 ) );
		Set( "SettingsEditor.Category_Default", new IMAGE_BRUSH( "Icons/icon_MatEd_Sphere_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_Editor", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartInLevel_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_Engine", new IMAGE_BRUSH( "Icons/icon_Cascade_RestartSim_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_Game", new IMAGE_BRUSH( "Icons/icon_MatEd_Realtime_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_LevelEditor", new IMAGE_BRUSH( "Icons/icon_volume_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_Project", new IMAGE_BRUSH( "Icons/icon_BlueprintEditor_Settings_40x", Icon40x40 ) );
		Set( "SettingsEditor.Category_Platforms", new IMAGE_BRUSH( "Icons/icon_PlayOnDevice_40px", Icon40x40 ) );
		Set( "SettingsEditor.Category_Plugins", new IMAGE_BRUSH( "Icons/Profiler/profiler_mem_40x", Icon40x40 ) );
		Set( "SettingsEditor.Collision_Engine", new IMAGE_BRUSH("Icons/icon_Cascade_RestartSim_40x", Icon16x16));
		Set( "SettingsEditor.Collision_Game", new IMAGE_BRUSH("Icons/icon_MatEd_Realtime_40x", Icon16x16));

		// Settings editor
		Set("SettingsEditor.GoodIcon", new IMAGE_BRUSH("Settings/Settings_Good", Icon40x40));
		Set("SettingsEditor.WarningIcon", new IMAGE_BRUSH("Settings/Settings_Warning", Icon40x40));
	}

	{
		// Navigation defaults
		const FLinearColor NavHyperlinkColor(0.03847f, 0.33446f, 1.0f);
		const FTextBlockStyle NavigationHyperlinkText = FTextBlockStyle(NormalText)
			.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 12))
			.SetColorAndOpacity(NavHyperlinkColor);

		const FButtonStyle NavigationHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), NavHyperlinkColor))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), NavHyperlinkColor));

		FHyperlinkStyle NavigationHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(NavigationHyperlinkButton)
			.SetTextStyle(NavigationHyperlinkText)
			.SetPadding(FMargin(0.0f));

		Set("NavigationHyperlink", NavigationHyperlink);
	}

#endif // WITH_EDITOR || IS_PROGRAM

	// External image picker
	{
		Set("ExternalImagePicker.BlankImage", new IMAGE_BRUSH( "Icons/BlankIcon", Icon16x16 ) );
		Set("ExternalImagePicker.PickImageButton", new IMAGE_BRUSH( "Icons/ellipsis_12x", Icon12x12 ) );
	}
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef TTF_CORE_FONT
#undef OTF_FONT
