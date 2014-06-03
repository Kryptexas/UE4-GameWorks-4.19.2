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

	/** Stores a Namespace and Key, used in reverse lookups from a DisplayString to find the Namespace and Key */
	struct FNamespaceKeyEntry
	{
		TSharedPtr<FString> Namespace;
		TSharedPtr<FString> Key;

		FNamespaceKeyEntry(TSharedPtr<FString> InNamespace, TSharedPtr<FString> InKey)
			: Namespace(InNamespace)
			, Key(InKey)
		{}
	};

public:
	static CORE_API FTextLocalizationManager& Get();

	TSharedPtr<FString> FindString( const FString& Namespace, const FString& Key, const FString* const SourceString = nullptr );
	TSharedRef<FString> GetString(const FString& Namespace, const FString& Key, const FString* const SourceString);

	/**
	 * Using an FText SourceString's, returns the associated Namespace and Key
	 *
	 * @param InSourceString		The FText DisplayString
	 * @param OutNamespace			Returns with the associated Namespace
	 * @param OutKey				Returns with the associated Key
	 */
	void CORE_API FindKeyNamespaceFromDisplayString(TSharedRef<FString> InDisplayString, TSharedPtr<FString>& OutNamespace, TSharedPtr<FString>& OutKey);

	void CORE_API RegenerateResources(const FString& ConfigFilePath);

	/** Returns the current culture revision index */
	int CORE_API GetHeadCultureRevision() const { return HeadCultureRevisionIndex; }

private:
	FTextLocalizationManager() 
		: bIsInitialized(false)
		, SynchronizationObject()
		, HeadCultureRevisionIndex(0)
	{}

	void LoadResources(const bool ShouldLoadEditor, const bool ShouldLoadGame);
	void UpdateLiveTable(const TArray<FLocalizationEntryTracker>& LocalizationEntryTrackers, const bool FilterUpdatesByTableName = false);
	void OnCultureChanged();

private:
	bool bIsInitialized;
	FTextLookupTable LiveTable;
	
	/** Table for looking up namespaces and keys using the DisplayString of an FText */
	TMap< TSharedPtr< FString >, FNamespaceKeyEntry > ReverseLiveTable;

	FCriticalSection SynchronizationObject;

	/** The current culture revision, increments every time the culture is changed and allows for FTexts to know when to rebuild under a new culture */
	int32 HeadCultureRevisionIndex;
};