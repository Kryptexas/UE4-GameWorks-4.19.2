// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UTranslationDataObject;
class FTranslationEditor;
class FInternationalizationArchive;
class FInternationalizationManifest;

#include "InternationalizationArchiveJsonSerializer.h"
#include "InternationalizationManifestJsonSerializer.h"
#include "TranslationDataObject.h"

class FTranslationDataManager
{

public:
	FTranslationDataManager( const FName& ProjectName, const FName& TranslationTargetLanguage );

	UTranslationDataObject* GetTranslationDataObject() 
	{
		return TranslationData;
	}
	
	/** Write the translation data in memory out to .archive file (check out the .archive file first if necessary) */
	void WriteTranslationData();

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
	void GetHistoryForTranslationUnits( TArray<FTranslationUnit>& TranslationUnits, const FString& ManifestFilePath );

	/** UObject containing our translation information */
	UTranslationDataObject* TranslationData;

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
	/** Path to the Archive File **/
	FString ArchiveFilePath;

	/** Files that are already checked out from Perforce **/
	TArray<FString> CheckedOutFiles;
};


