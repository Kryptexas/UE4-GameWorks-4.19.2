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
		const FString EngineContentLocalizationFolder = FPaths::EngineContentDir() / TEXT("Localization");
		const FString EngineContentLocalizationFolderWildcard = FString::Printf(TEXT("%s/*"), *EngineContentLocalizationFolder);

		TArray<FString> EngineContentManifestFolders;
		IFileManager::Get().FindFiles(EngineContentManifestFolders, *EngineContentLocalizationFolderWildcard, false, true);

		TArray<FString> ManifestFileNames;
		for (FString ProjectManifestFolder : EngineContentManifestFolders)
		{
			// Add root back on
			ProjectManifestFolder = FPaths::EngineContentDir() / TEXT("Localization") / ProjectManifestFolder;

			const FString ManifestExtension = TEXT("manifest");
			const FString EngineContentLocalizationFileWildcard = FString::Printf(TEXT("%s/*.%s"), *ProjectManifestFolder, *ManifestExtension);
			
			TArray<FString> CurrentFolderManifestFileNames;
			IFileManager::Get().FindFiles(CurrentFolderManifestFileNames, *EngineContentLocalizationFileWildcard, true, false);

			for (FString& CurrentFolderManifestFileName : CurrentFolderManifestFileNames)
			{
				// Add root back on
				CurrentFolderManifestFileName = ProjectManifestFolder / CurrentFolderManifestFileName;
			}

			ManifestFileNames.Append(CurrentFolderManifestFileNames);
		}

		// Project Folders
		const FString ProjectLocalizationFolder = FPaths::GameContentDir() / TEXT("Localization");
		const FString ProjectLocalizationFolderWildcard = FString::Printf(TEXT("%s/*"), *ProjectLocalizationFolder);

		TArray<FString> ProjectManifestFolders;
		IFileManager::Get().FindFiles(ProjectManifestFolders, *ProjectLocalizationFolderWildcard, false, true);
		for (FString ProjectManifestFolder : ProjectManifestFolders)
		{
			// Add root back on
			ProjectManifestFolder = FPaths::GameContentDir() / TEXT("Localization") / ProjectManifestFolder;

			const FString ManifestExtension = TEXT("manifest");
			const FString ProjectLocalizationFileWildcard = FString::Printf(TEXT("%s/*.%s"), *ProjectManifestFolder, *ManifestExtension);

			TArray<FString> CurrentFolderManifestFileNames;
			IFileManager::Get().FindFiles(CurrentFolderManifestFileNames, *ProjectLocalizationFileWildcard, true, false);

			for (FString& CurrentFolderManifestFileName : CurrentFolderManifestFileNames)
			{
				// Add root back on
				CurrentFolderManifestFileName = ProjectManifestFolder / CurrentFolderManifestFileName;
			}

			ManifestFileNames.Append(CurrentFolderManifestFileNames);
		}

		MenuBuilder.BeginSection("Project", LOCTEXT("Translation_ChooseProject", "Choose Project:"));
		{
			for (FString ManifestFileName : ManifestFileNames)
			{
				FString ManifestName = FPaths::GetBaseFilename(ManifestFileName);

				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("ManifestName"), FText::FromString(ManifestName));
				MenuBuilder.AddSubMenu(
					FText::Format(LOCTEXT("TranslationEditorSubMenuProjectLabel", "{ManifestName} Translations"), Arguments),
					FText::Format(LOCTEXT("TranslationEditorSubMenuProjectToolTip", "Edit Translations for {ManifestName} Strings"), Arguments),
					FNewMenuDelegate::CreateStatic(&FMainFrameTranslationEditorMenu::MakeMainFrameTranslationEditorSubMenuEditorProject, ManifestFileName)
				);
			}
		}

		MenuBuilder.EndSection();
	}

	/**
	 * Creates the main frame translation editor sub menu for editor translations.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMainFrameTranslationEditorSubMenuEditorProject( FMenuBuilder& MenuBuilder, const FString ManifestFileName )
	{
		const FString ManifestFolder = FPaths::GetPath(ManifestFileName);
		const FString ManifestFolderWildcard = FString::Printf(TEXT("%s/*"), *ManifestFolder);

		TArray<FString> ArchiveFolders;
		IFileManager::Get().FindFiles(ArchiveFolders, *ManifestFolderWildcard, false, true);

		TArray<FString> ArchiveFileNames;
		for (FString ArchiveFolder : ArchiveFolders)
		{
			// Add root back on
			ArchiveFolder = ManifestFolder / ArchiveFolder;

			const FString ArchiveExtension = TEXT("archive");
			const FString ArchiveFileWildcard = FString::Printf(TEXT("%s/*.%s"), *ArchiveFolder, *ArchiveExtension);

			TArray<FString> CurrentFolderArchiveFileNames;
			IFileManager::Get().FindFiles(CurrentFolderArchiveFileNames, *ArchiveFileWildcard, true, false);

			for (FString& CurrentFolderArchiveFileName : CurrentFolderArchiveFileNames)
			{
				// Add root back on
				CurrentFolderArchiveFileName = ArchiveFolder / CurrentFolderArchiveFileName;
			}

			ArchiveFileNames.Append(CurrentFolderArchiveFileNames);
		}

		MenuBuilder.BeginSection("Language", LOCTEXT("Translation_ChooseLanguage", "Choose Language:"));
		{
			for (FString ArchiveFileName : ArchiveFileNames)
			{
				FString ArchiveName = FPaths::GetBaseFilename(FPaths::GetPath(ArchiveFileName));
				FString ManifestName = FPaths::GetBaseFilename(ManifestFileName);

				TSharedPtr<FCulture> Culture = FInternationalization::GetCulture(ArchiveName);
				if (Culture.IsValid())
				{
					ArchiveName = Culture->GetDisplayName();
				}

				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("ArchiveName"), FText::FromString(ArchiveName));
				Arguments.Add(TEXT("ManifestName"), FText::FromString(ManifestName));

				MenuBuilder.AddMenuEntry(
					FText::Format(LOCTEXT("TranslationEditorSubMenuLanguageLabel", "{ArchiveName} Translations"), Arguments),
					FText::Format(LOCTEXT("TranslationEditorSubMenuLanguageToolTip", "Edit the {ArchiveName} Translations for the {ManifestName} Project"), Arguments),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateStatic(&FMainFrameTranslationEditorMenu::HandleMenuEntryExecute, ManifestFileName, ArchiveFileName))
					);
			}
		}

		MenuBuilder.EndSection();
	}

	private:

	// Handles clicking a menu entry.
	static void HandleMenuEntryExecute( FString ProjectName, FString Language)
	{
		FModuleManager::LoadModuleChecked<ITranslationEditor>("TranslationEditor").OpenTranslationEditor(ProjectName, Language);
	}
};


#undef LOCTEXT_NAMESPACE
