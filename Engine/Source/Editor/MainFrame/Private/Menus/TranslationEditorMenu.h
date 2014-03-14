// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PackageProjectMenu.h: Declares the FPackageProjectMenu class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "FMainFrameTranslationEditorMenu"


/**
 * Static helper class for populating the "Cook Content" menu.
 */
class FMainFrameTranslationEditorMenu
{
public:

	/**
	 * Creates the main frame translation editor sub menu.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMainFrameTranslationEditorSubMenu( FMenuBuilder& MenuBuilder )
	{
		MenuBuilder.BeginSection("Project", LOCTEXT("Translation_ChooseProject", "Choose Project:"));
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("TranslationEditorSubMenuEditorProjectLabel", "Editor Translations"),
				LOCTEXT("EditorPreferencesSubMenuEditorProjectToolTip", "Edit Translations for Strings in Unreal Editor"),
				FNewMenuDelegate::CreateStatic(&FMainFrameTranslationEditorMenu::MakeMainFrameTranslationEditorSubMenuEditorProject)
			);
			MenuBuilder.AddSubMenu(
				LOCTEXT("TranslationEditorSubMenuEngineProjectLabel", "Engine Translations"),
				LOCTEXT("EditorPreferencesSubMenuEngineProjectToolTip", "Edit Translations for Strings in Unreal Engine"),
				FNewMenuDelegate::CreateStatic(&FMainFrameTranslationEditorMenu::MakeMainFrameTranslationEditorSubMenuEngineProject)
			);
		}
		MenuBuilder.EndSection();
	}

	/**
	 * Creates the main frame translation editor sub menu for editor translations.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMainFrameTranslationEditorSubMenuEditorProject( FMenuBuilder& MenuBuilder )
	{
		MenuBuilder.BeginSection("Language", LOCTEXT("Translation_ChooseLanguage", "Choose Language:"));
		{
			//MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().OpenTranslationEditor );

			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorJapaneseLabel", "Translate Japanese"),
				LOCTEXT("TranslationEditorJapaneseTooltip", "Edit the Japanese Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Editor"), FName("ja")))
			);
			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorKoreanLabel", "Translate Korean"),
				LOCTEXT("TranslationEditorKoreanTooltip", "Edit the Korean Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Editor"), FName("ko")))
			);
			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorChineseLabel", "Translate Chinese"),
				LOCTEXT("TranslationEditorChineseTooltip", "Edit the Chinese Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Editor"), FName("zh")))
			);
		}
		MenuBuilder.EndSection();
	}

		/**
	 * Creates the main frame translation editor sub menu for engine translations.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMainFrameTranslationEditorSubMenuEngineProject( FMenuBuilder& MenuBuilder )
	{
		MenuBuilder.BeginSection("Language", LOCTEXT("Translation_ChooseLanguage", "Choose Language:"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorJapaneseLabel", "Translate Japanese"),
				LOCTEXT("TranslationEditorJapaneseTooltip", "Edit the Japanese Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Engine"), FName("ja")))
			);
			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorKoreanLabel", "Translate Korean"),
				LOCTEXT("TranslationEditorKoreanTooltip", "Edit the Korean Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Engine"), FName("ko")))
			);
			MenuBuilder.AddMenuEntry(
				LOCTEXT("TranslationEditorChineseLabel", "Translate Chinese"),
				LOCTEXT("TranslationEditorChineseTooltip", "Edit the Chinese Translations for the Selected Project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, FName("Engine"), FName("zh")))
			);
		}
		MenuBuilder.EndSection();
	}

	private:

	// Handles clicking a menu entry.
	static void HandleMenuEntryExecute( FName ProjectName, FName Language)
	{
		FModuleManager::LoadModuleChecked<ITranslationEditor>("TranslationEditor").OpenTranslationEditor(ProjectName, Language);
	}
};


#undef LOCTEXT_NAMESPACE
