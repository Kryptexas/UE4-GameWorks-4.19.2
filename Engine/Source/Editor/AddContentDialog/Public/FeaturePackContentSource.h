// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Private/IContentSource.h"

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

	static ADDCONTENTDIALOG_API void ImportPendingPacks();
	TArray<FString>	ParseErrors;
private:
	static void ParseAndImportPacks();
	bool LoadPakFileToBuffer(FPakPlatformFile& PakPlatformFile, FString Path, TArray<uint8>& Buffer);
	FString GetFocusAssetName() const;

private:
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
	
};