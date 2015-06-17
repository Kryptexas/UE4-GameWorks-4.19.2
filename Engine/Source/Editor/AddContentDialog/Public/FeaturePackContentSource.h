// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Private/IContentSource.h"
#include "FeaturePackContentSource.generated.h"

class UObject;

struct FPackData
{
	FString PackSource;
	FString PackName;
	FString PackMap;
	TArray<UObject*>	ImportedObjects;
};

class FLocalizedTextArray
{
public:
	FLocalizedTextArray()
	{
	}

	/** Creates a new FLocalizedText
		@param InTwoLetterLanguage - The iso 2-letter language specifier.
		@param InText - The text in the language specified */
	FLocalizedTextArray(FString InTwoLetterLanguage, FString InText)
	{
		TwoLetterLanguage = InTwoLetterLanguage;
		TArray<FString> AsArray;
		InText.ParseIntoArray(AsArray,TEXT(","));
		for (int32 iString = 0; iString < AsArray.Num() ; iString++)
		{
			Tags.Add(FText::FromString(AsArray[iString]));
		}
	}

	/** Gets the iso 2-letter language specifier for this text. */
	FString GetTwoLetterLanguage() const
	{
		return TwoLetterLanguage;
	}

	/** Gets the array of tags in the language specified. */
	TArray<FText> GetTags() const
	{
		return Tags;
	}

private:
	FString TwoLetterLanguage;
	TArray<FText> Tags;
};


/** Defines categories for shared template resource levels. */
UENUM()
enum class EFeaturePackDetailLevel :uint8
{
	Standard,
	High,
};

/* Structure that defines a shared feature pack resource. */
USTRUCT()
struct FFeaturePackLevelSet
{
	GENERATED_BODY()

	FFeaturePackLevelSet(){};

	/** Creates a new FFeaturePackLevelSet
		@param InMountName - Name of the pack/folder to insert to 
		@param InDetailLevels - The levels available for this pack*/
	FFeaturePackLevelSet(FString InMountName, TArray<EFeaturePackDetailLevel> InDetailLevels)
	{
		MountName = InMountName;
		DetailLevels = InDetailLevels;
	}

	/* List of shared resource levels for this shared resource.*/
	UPROPERTY()
	TArray<EFeaturePackDetailLevel> DetailLevels;

	/* Mount name for the shared resource - this is the folder the resource will be copied to on project generation as well as the name of the folder that will appear in the content browser. */
	UPROPERTY()
	FString MountName;

	FString GetFeaturePackNameForLevel(EFeaturePackDetailLevel InLevel, bool bLevelRequired = false)
	{
		check(DetailLevels.Num()>0); // We need at least one detail level defined
		int32 Index = DetailLevels.Find(InLevel);
		FString DetailString;
		if( Index != INDEX_NONE)
		{			
			UEnum::GetValueAsString(TEXT("/Script/AddContentDialog.EFeaturePackDetailLevel"), InLevel, DetailString);					
		}
		else 
		{
			check(bLevelRequired==false); // The level is REQUIRED and we don't have it !
			// If we didn't have the requested level, use the first
			UEnum::GetValueAsString(TEXT("/Script/AddContentDialog.EFeaturePackDetailLevel"), DetailLevels[0], DetailString);
		}
		FString NameString = MountName+DetailString + TEXT(".upack");
		return NameString;
	}
};

class FPakPlatformFile;
struct FSearchEntry;

/** A content source which represents a content upack. */
class FFeaturePackContentSource : public IContentSource
{
public:
	ADDCONTENTDIALOG_API FFeaturePackContentSource(FString InFeaturePackPath, bool bDontRegister = false);

	virtual TArray<FLocalizedText> GetLocalizedNames() const override;
	virtual TArray<FLocalizedText> GetLocalizedDescriptions() const override;
	
	virtual EContentSourceCategory GetCategory() const override;
	virtual TArray<FLocalizedText> GetLocalizedAssetTypes() const override;
	virtual FString GetSortKey() const override;
	virtual FString GetClassTypesUsed() const override;
	
	virtual TSharedPtr<FImageData> GetIconData() const override;
	virtual TArray<TSharedPtr<FImageData>> GetScreenshotData() const override;

	virtual bool InstallToProject(FString InstallPath) override;
	virtual bool IsDataValid() const override;
	
	virtual ~FFeaturePackContentSource();
	
	void HandleActOnSearchText(TSharedPtr<FSearchEntry> SearchEntry);
	void HandleSuperSearchTextChanged(const FString& InText, TArray< TSharedPtr<FSearchEntry> >& OutSuggestions);
		
	/*
	 * Copies the list of files specified in 'AdditionFilesToInclude' section in the config.ini of the feature pack.
	 *
	 * @param DestinationFolder	Destination folder for the files
	 * @param FilesCopied		List of files copied
	 * @param bContainsSource 	Set to true if the file list contains any source files
	 * @returns true if config file was read and parsed successfully
	 */
	ADDCONTENTDIALOG_API void CopyAdditionalFilesToFolder( const FString& DestinationFolder, TArray<FString>& FilesCopied, bool &bHasSourceFiles );

	/*
	 * Returns a list of additional files (including the path) as specified in the config file if one exists in the pack file.
	 *
	 * @param FileList		  array to receive list of files
	 * @param bContainsSource did the file list contain any source files
	 * @returns true if config file was read and parsed successfully
	 */
	ADDCONTENTDIALOG_API bool GetAdditionalFilesForPack(TArray<FString>& FileList, bool& bContainsSource);

	static ADDCONTENTDIALOG_API void ImportPendingPacks();
	
	TArray<FString>	ParseErrors;
	
private:
	static void ParseAndImportPacks();
	bool LoadPakFileToBuffer(FPakPlatformFile& PakPlatformFile, FString Path, TArray<uint8>& Buffer);
	FString GetFocusAssetName() const;
	
	/*
	 * Extract the list of additional files defined in config file to an array
	 *
	 * @param ConfigFile	  config file as a string
	 * @param FileList		  array to receive list of files
	 * @param bContainsSource did the file list contain any source files
	 */
	bool ExtractListOfAdditionalFiles(const FString& ConfigFile, TArray<FString>& FileList,bool& bContainsSource) const;

	void RecordAndLogError(const FString& ErrorString);
	/** Selects an FLocalizedText from an array which matches either the supplied language code, or the default language code. */
	FLocalizedTextArray ChooseLocalizedTextArray(TArray<FLocalizedTextArray> Choices, FString LanguageCode);
	FLocalizedText ChooseLocalizedText(TArray<FLocalizedText> Choices, FString LanguageCode);
	void TryAddFeaturePackCategory(FString CategoryTitle, TArray< TSharedPtr<FSearchEntry> >& OutSuggestions);

	FString FeaturePackPath;
	TArray<FLocalizedText> LocalizedNames;
	TArray<FLocalizedText> LocalizedDescriptions;
	EContentSourceCategory Category;
	TSharedPtr<FImageData> IconData;
	TArray<TSharedPtr<FImageData>> ScreenshotData;
	TArray<FLocalizedText> LocalizedAssetTypesList;
	FString ClassTypes;
	bool bPackValid;
	FString FocusAssetIdent;
	FString SortKey;
	TArray<FLocalizedTextArray> LocalizedSearchTags;
	TArray<FFeaturePackLevelSet> AdditionalFeaturePacks;
};