// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "TranslationEditorPrivatePCH.h"
#include "TranslationEditor.h"
#include "GraphEditorActions.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "KismetToolbar"

void FTranslationEditorMenu::FillTranslationMenu( FMenuBuilder& MenuBuilder/*, FTranslationEditor& TranslationEditor*/ )
{
	MenuBuilder.BeginSection("Font", LOCTEXT("Translation_FontHeading", "Font"));
	{
		MenuBuilder.AddMenuEntry( FTranslationEditorCommands::Get().ChangeSourceFont );
		MenuBuilder.AddMenuEntry( FTranslationEditorCommands::Get().ChangeTranslationTargetFont );
	}
	MenuBuilder.EndSection();
}

void FTranslationEditorMenu::SetupTranslationEditorMenu( TSharedPtr< FExtender > Extender, FTranslationEditor& TranslationEditor)
{
	// Add additional editor menu
	{
		struct Local
		{
			static void AddSaveMenuOption( FMenuBuilder& MenuBuilder )
			{
				MenuBuilder.AddMenuEntry( FTranslationEditorCommands::Get().SaveTranslations, "SaveTranslations", TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "AssetEditor.SaveAsset.Greyscale") );
			}

			static void AddTranslationEditorMenu( FMenuBarBuilder& MenuBarBuilder )
			{
				// View
				MenuBarBuilder.AddPullDownMenu( 
					LOCTEXT("TranslationMenu", "Translation"),
					LOCTEXT("TranslationMenu_ToolTip", "Open the Translation menu"),
					FNewMenuDelegate::CreateStatic( &FTranslationEditorMenu::FillTranslationMenu ),
					"View");
			}
		};

		Extender->AddMenuExtension(
			"FileLoadAndSave",
			EExtensionHook::First,
			TranslationEditor.GetToolkitCommands(),
			FMenuExtensionDelegate::CreateStatic( &Local::AddSaveMenuOption ) );

		Extender->AddMenuBarExtension(
			"Edit",
			EExtensionHook::After,
			TranslationEditor.GetToolkitCommands(), 
			FMenuBarExtensionDelegate::CreateStatic( &Local::AddTranslationEditorMenu ) );
	}
}

void FTranslationEditorMenu::SetupTranslationEditorToolbar( TSharedPtr< FExtender > Extender, FTranslationEditor& TranslationEditor )
{
	struct Local
	{
		static void AddToolbarButtons( FToolBarBuilder& ToolbarBuilder )
		{
			ToolbarBuilder.AddToolBarButton(
				FTranslationEditorCommands::Get().SaveTranslations, "SaveTranslations", TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "AssetEditor.SaveAsset"));
		}
	};

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::First,
		TranslationEditor.GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::AddToolbarButtons ) );
}


//////////////////////////////////////////////////////////////////////////
// FTranslationEditorCommands

void FTranslationEditorCommands::RegisterCommands() 
{
	UI_COMMAND( ChangeSourceFont, "Change Source Font", "Change the Font for the Source Lanugage", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( ChangeTranslationTargetFont, "Change Translation Font", "Change the Translation Target Language Font", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( SaveTranslations, "Save", "Saves the translations to file", EUserInterfaceActionType::Button, FInputGesture() );
}


#undef LOCTEXT_NAMESPACE
