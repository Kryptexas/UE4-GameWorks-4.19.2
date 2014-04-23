// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FTranslationEditor;
class FInternationalizationArchive;
class FInternationalizationManifest;
class UTranslationUnit;

#include "InternationalizationArchiveJsonSerializer.h"
#include "InternationalizationManifestJsonSerializer.h"

class FTranslationDataManager : public TSharedFromThis<FTranslationDataManager>
{

public:

	FTranslationDataManager( const FString& InManifestFilePath, const FString& InArchiveFilePath);

	TArray<UTranslationUnit*>& GetAllTranslationsArray()
	{
		return AllTranslations;
	}

	TArray<UTranslationUnit*>& GetUntranslatedArray()
	{
		return Untranslated;
	}

	TArray<UTranslationUnit*>& GetReviewArray()
	{
		return Review;
	}

	TArray<UTranslationUnit*>& GetCompleteArray()
	{
		return Complete;
	}

	TArray<UTranslationUnit*>& GetSearchResultsArray()
	{
		return SearchResults;
	}
	
	/** Write the translation data in memory out to .archive file (check out the .archive file first if necessary) */
	void WriteTranslationData();

	/** Delegate called when a TranslationDataObject property is changed */
	void HandlePropertyChanged(FName PropertyName);

	/** Regenerate and reload archives to reflect modifications in the UI */
	void PreviewAllTranslationsInEditor();

	/** Put items in the Search Array if they match this filter */
	void PopulateSearchResultsUsingFilter(const FString& SearchFilter);

private:
	/** Read text file into a JSON file */
	static TSharedPtr<FJsonObject> ReadJSONTextFile( const FString& InFilePath ) ;

	/** Take a path and a manifest name and return a manifest data structure */
	TSharedPtr< FInternationalizationManifest > ReadManifest ( const FString& ManifestFilePath );
	/** Take a path and an archive name and return an archive data structure */
	TSharedPtr< FInternationalizationArchive > ReadArchive( TSharedRef< FInternationalizationManifest >& InternationalizationManifest );

	/** Write JSON file to text file */
	bool WriteJSONToTextFile( TSharedRef<FJsonObject>& Output, const FString& Filename );

	/** Get the history data for a given translation unit */
	void GetHistoryForTranslationUnits( TArray<UTranslationUnit*>& TranslationUnits, const FString& ManifestFilePath );

	// Arrays containing the translation data
	TArray<UTranslationUnit*> AllTranslations;
	TArray<UTranslationUnit*> Untranslated;
	TArray<UTranslationUnit*> Review;
	TArray<UTranslationUnit*> Complete;
	TArray<UTranslationUnit*> SearchResults;

	/** Serializes and deserializes our Archive */
	FInternationalizationArchiveJsonSerializer ArchiveSerializer;
	/** Serializes and deserializes Manifests */
	FInternationalizationManifestJsonSerializer ManifestSerializer;

	/** Archive for the current project and translation language */
	TSharedPtr< FInternationalizationArchive > ArchivePtr;
	/** Manifest for the current project */
	TSharedPtr< FInternationalizationManifest > ManifestAtHeadRevisionPtr;

	/** Name of the manifest file */
	FString ManifestName;
	/** Path to the project */
	FString ProjectPath;
	/** Name of the archive file */
	FString ArchiveName;
	/** Path to the culture (language, sort of) we are targeting */
	FString CulturePath;
	/** Path to the Manifest File **/
	FString ManifestFilePath;
	/** Path to the Archive File **/
	FString ArchiveFilePath;

	/** Files that are already checked out from Perforce **/
	TArray<FString> CheckedOutFiles;
};


