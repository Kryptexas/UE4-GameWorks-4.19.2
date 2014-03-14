// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace ELocalizationResourceSource
{
	enum Type
	{
		ManifestArchive,
		LocalizationResource
	};
}

class FTextLocalizationManager
{
	friend CORE_API void BeginInitTextLocalization();
	friend CORE_API void EndInitTextLocalization();

private:
	struct FLocalizationEntryTracker
	{
		struct FEntry
		{
			FString TableName;
			uint32 SourceStringHash;
			FString LocalizedString;
		};

		typedef TArray<FEntry> FEntryArray;
		typedef TMap<FString, FEntryArray> FKeyTable;
		typedef TMap<FString, FKeyTable> FNamespaceTable;

		FNamespaceTable Namespaces;

		void ReportCollisions() const;
		void ReadFromDirectory(const FString& DirectoryPath);
		bool ReadFromFile(const FString& FilePath);
		void ReadFromArchive(FArchive& Archive, const FString& Identifier);
	};

	struct FStringEntry
	{
		bool bIsLocalized;
		FString TableName;
		uint32 SourceStringHash;
		TSharedRef<FString> String;

		FStringEntry(const bool InIsLocalized, const FString& InTableName, const uint32 InSourceStringHash, const TSharedRef<FString>& InString)
			:	bIsLocalized(InIsLocalized), TableName(InTableName), SourceStringHash(InSourceStringHash), String(InString)
		{
		}
	};

	struct FTextLookupTable
	{
		typedef TMap<FString, FStringEntry> FKeyTable;
		typedef TMap<FString, FKeyTable> FNamespaceTable;
		FNamespaceTable NamespaceTable;

		const TSharedRef<FString>* GetString(const FString& Namespace, const FString& Key, const uint32 SourceStringHash) const;
	};

public:
	static CORE_API FTextLocalizationManager& Get();

	TSharedPtr<FString> FindString( const FString& Namespace, const FString& Key );
	TSharedRef<FString> GetString(const FString& Namespace, const FString& Key, const FString* const SourceString);

	void CORE_API RegenerateResources(const FString& ConfigFilePath);

private:
	FTextLocalizationManager() 
		: bIsInitialized(false)
		, SynchronizationObject()
	{}

	void LoadResources(const bool ShouldLoadEditor, const bool ShouldLoadGame);
	void UpdateLiveTable(const TArray<FLocalizationEntryTracker>& LocalizationEntryTrackers, const bool FilterUpdatesByTableName = false);
	void OnCultureChanged();

private:
	bool bIsInitialized;
	FTextLookupTable LiveTable;
	FCriticalSection SynchronizationObject;
};