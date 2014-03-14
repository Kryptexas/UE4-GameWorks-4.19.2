// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateEditorStyle.h: Implements the FSlateEditorStyle class.
=============================================================================*/

#pragma once


/**
 * Declares the Editor's visual style.
 */
class FSlateEditorStyle
	: public FEditorStyle
{
public:

	static void Initialize() 
	{
		Settings = NULL;

#if WITH_EDITOR
		Settings = GetMutableDefault<UEditorStyleSettings>();
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings( "Editor", "General", "Appearance",
				NSLOCTEXT("EditorStyle", "Appearance_UserSettingsName", "Appearance"),
				NSLOCTEXT("EditorStyle", "Appearance_UserSettingsDescription", "Customize the look of the editor."),
				TWeakObjectPtr<UObject>( Settings.Get() )
			);
		}
#endif

		StyleInstance = Create( Settings );
		SetStyle( StyleInstance.ToSharedRef() );
	}

	static void Shutdown()
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings( "Editor", "General", "Appearance");
		}

		ResetToDefault();
		ensure( StyleInstance.IsUnique() );
		StyleInstance.Reset();
	}
	
	static void SyncCustomizations()
	{
		FSlateEditorStyle::StyleInstance->SyncSettings();
	}

	class FStyle : public FSlateStyleSet
	{
	public:
		FStyle( const TWeakObjectPtr< UEditorStyleSettings >& InSettings );

		void Initialize();
		void SetupGeneralStyles();
		void SetupWindowStyles();
		void SetupDockingStyles();
		void SetupTutorialStyles();
		void SetupPropertyEditorStyles();
		void SetupProfilerStyle();
		void SetupGraphEditorStyles();
		void SetupLevelEditorStyle();
		void SetupPersonaStyle();
		void SetupClassIconsAndThumbnails();
		void SetupContentBrowserStyle();
		void SetupLandscapeEditorStyle();
		void SetupToolkitStyles();
		void SetupMatineeStyle();
		void SetupSourceControlStyles();
		void SetupAutomationStyles();

		void SettingsChanged( UObject* ChangedObject );
		void SyncSettings();

		const FVector2D Icon5x16;
		const FVector2D Icon8x4;
		const FVector2D Icon8x8;
		const FVector2D Icon10x10;
		const FVector2D Icon12x12;
		const FVector2D Icon12x16;
		const FVector2D Icon14x14;
		const FVector2D Icon16x16;
		const FVector2D Icon16x20;
		const FVector2D Icon20x20;
		const FVector2D Icon22x22;
		const FVector2D Icon24x24;
		const FVector2D Icon25x25;
		const FVector2D Icon32x32;
		const FVector2D Icon40x40;
		const FVector2D Icon48x48;
		const FVector2D Icon64x64;
		const FVector2D Icon36x24;
		const FVector2D Icon128x128;

		// These are the colors that are updated by the user style customizations
		const TSharedRef< FLinearColor > DefaultForeground_LinearRef;
		const TSharedRef< FLinearColor > InvertedForeground_LinearRef;
		const TSharedRef< FLinearColor > SelectorColor_LinearRef;
		const TSharedRef< FLinearColor > SelectionColor_LinearRef;
		const TSharedRef< FLinearColor > SelectionColor_Inactive_LinearRef;
		const TSharedRef< FLinearColor > SelectionColor_Pressed_LinearRef;

		// These are the Slate colors which reference those above; these are the colors to put into the style
		const FSlateColor DefaultForeground;
		const FSlateColor InvertedForeground;
		const FSlateColor SelectorColor;
		const FSlateColor SelectionColor;
		const FSlateColor SelectionColor_Inactive;
		const FSlateColor SelectionColor_Pressed;

		FTextBlockStyle NormalText;
		FTableRowStyle NormalTableRowStyle;
		FButtonStyle Button;
		FButtonStyle HoverHintOnly;

		TWeakObjectPtr< UEditorStyleSettings > Settings;
	};

	static TSharedRef< class FSlateEditorStyle::FStyle > Create( const TWeakObjectPtr< UEditorStyleSettings >& InCustomization )
	{
		TSharedRef< class FSlateEditorStyle::FStyle > NewStyle = MakeShareable( new FSlateEditorStyle::FStyle( InCustomization ) );
		NewStyle->Initialize();

		FCoreDelegates::OnObjectPropertyChanged.AddSP( NewStyle, &FSlateEditorStyle::FStyle::SettingsChanged );

		return NewStyle;
}

	static TSharedPtr< FSlateEditorStyle::FStyle > StyleInstance;
	static TWeakObjectPtr< UEditorStyleSettings > Settings;
};
