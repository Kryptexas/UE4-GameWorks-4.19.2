// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "InternationalizationManifest.h"
#include "Json.h"
#include "JsonDocumentObjectModel.h"
#include "TextLocalizationResourceGenerator.h"
#include "InternationalizationManifestJsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTextLocalizationManager, Log, All);

static FString AccessedStringBeforeLocLoadedErrorMsg = TEXT("Can't access string. Loc System hasn't been initialized yet!");

void BeginInitTextLocalization()
{
	FCoreDelegates::OnCultureChanged.AddRaw( &(FTextLocalizationManager::Get()), &FTextLocalizationManager::OnCultureChanged );
}

void EndInitTextLocalization()
{
	FTextLocalizationManager::Get().LoadResources(WITH_EDITOR, !GIsEditor);
	FTextLocalizationManager::Get().bIsInitialized = true;
}

FTextLocalizationManager& FTextLocalizationManager::Get()
{
	static FTextLocalizationManager* GTextLocalizationManager = NULL;
	if( !GTextLocalizationManager )
	{
		GTextLocalizationManager = new FTextLocalizationManager;
	}

	return *GTextLocalizationManager;
}

const TSharedRef<FString>* FTextLocalizationManager::FTextLookupTable::GetString(const FString& Namespace, const FString& Key, const uint32 SourceStringHash) const
{
	const FKeyTable* const KeyTable = NamespaceTable.Find(Namespace);
	if( KeyTable )
	{
		const FStringEntry* const Entry = KeyTable->Find(Key);
		if( Entry )
		{
			return &(Entry->String);
		}
	}

	return NULL;
}

void FTextLocalizationManager::FLocalizationEntryTracker::ReportCollisions() const
{
	for(auto i = Namespaces.CreateConstIterator(); i; ++i)
	{
		const FString& NamespaceName = i.Key();
		const FLocalizationEntryTracker::FKeyTable& KeyTable = i.Value();
		for(auto j = KeyTable.CreateConstIterator(); j; ++j)
		{
			const FString& KeyName = j.Key();
			const FLocalizationEntryTracker::FEntryArray& EntryArray = j.Value();

			bool bWasCollisionDetected = false;
			for(int32 k = 0; k < EntryArray.Num(); ++k)
			{
				const FLocalizationEntryTracker::FEntry& LeftEntry = EntryArray[k];
				for(int32 l = k + 1; l < EntryArray.Num(); ++l)
				{
					const FLocalizationEntryTracker::FEntry& RightEntry = EntryArray[l];
					const bool bDoesSourceStringHashDiffer = LeftEntry.SourceStringHash != RightEntry.SourceStringHash;
					const bool bDoesLocalizedStringDiffer = LeftEntry.LocalizedString != RightEntry.LocalizedString;
					bWasCollisionDetected = bDoesSourceStringHashDiffer || bDoesLocalizedStringDiffer;
				}
			}

			if(bWasCollisionDetected)
			{
				FString CollidingEntryListString;
				for(int32 k = 0; k < EntryArray.Num(); ++k)
				{
					const FLocalizationEntryTracker::FEntry& Entry = EntryArray[k];

					if( !(CollidingEntryListString.IsEmpty()) )
					{
						CollidingEntryListString += '\n';
					}

					CollidingEntryListString += FString::Printf( TEXT("Resource: (%s) Hash: (%d) String: (%s)"), *(Entry.TableName), Entry.SourceStringHash, *(Entry.LocalizedString) );
				}

				UE_LOG(LogTextLocalizationManager, Warning, TEXT("Loading localization resources contain conflicting entries for (Namespace:%s, Key:%s):\n%s"), *NamespaceName, *KeyName, *CollidingEntryListString);
			}
		}
	}
}

void FTextLocalizationManager::FLocalizationEntryTracker::ReadFromDirectory(const FString& DirectoryPath)
{
	// Find resources in the specified folder.
	TArray<FString> ResourceFileNames;
	IFileManager::Get().FindFiles(ResourceFileNames, *(DirectoryPath / TEXT("*.locres")), true, false);

	// For each resource:
	for(int32 ResourceIndex = 0; ResourceIndex < ResourceFileNames.Num(); ++ResourceIndex)
	{
		const FString ResourceFilePath = DirectoryPath / ResourceFileNames[ResourceIndex];
		ReadFromFile( FPaths::ConvertRelativePathToFull(ResourceFilePath) );
	}
}

bool FTextLocalizationManager::FLocalizationEntryTracker::ReadFromFile(const FString& FilePath)
{
	FArchive* Reader = IFileManager::Get().CreateFileReader( *FilePath );
	if( !Reader )
	{
		return false;
	}

	Reader->SetForceUnicode(true);

	ReadFromArchive(*Reader, FilePath);

	bool Success = Reader->Close();
	delete Reader;

	return Success;
}

void FTextLocalizationManager::FLocalizationEntryTracker::ReadFromArchive(FArchive& Archive, const FString& Identifier)
{
	// Read namespace count
	uint32 NamespaceCount;
	Archive << NamespaceCount;

	for(uint32 i = 0; i < NamespaceCount; ++i)
	{
		// Read namespace
		FString Namespace;
		Archive << Namespace;

		// Read key count
		uint32 KeyCount;
		Archive << KeyCount;

		FLocalizationEntryTracker::FKeyTable& KeyTable = Namespaces.FindOrAdd(*Namespace);

		for(uint32 j = 0; j < KeyCount; ++j)
		{
			// Read key
			FString Key;
			Archive << Key;

			FLocalizationEntryTracker::FEntryArray& EntryArray = KeyTable.FindOrAdd( *Key );

			FLocalizationEntryTracker::FEntry NewEntry;
			NewEntry.TableName = Identifier;

			// Read string entry.
			Archive << NewEntry.SourceStringHash;
			Archive << NewEntry.LocalizedString;

			EntryArray.Add(NewEntry);
		}
	}
}

void FTextLocalizationManager::LoadResources(const bool ShouldLoadEditor, const bool ShouldLoadGame)
{
	const FString& CultureName = FInternationalization::GetCurrentCulture()->GetName();
	const FString& BaseLanguageName = FInternationalization::GetCurrentCulture()->GetTwoLetterISOLanguageName();

#if ENABLE_LOC_TESTING
	if(CultureName == TEXT("LEET"))
	{
		for(auto NamespaceIterator = LiveTable.NamespaceTable.CreateIterator(); NamespaceIterator; ++NamespaceIterator)
		{
			const FString& Namespace = NamespaceIterator.Key();
			FTextLookupTable::FKeyTable& LiveKeyTable = NamespaceIterator.Value();
			for(auto KeyIterator = LiveKeyTable.CreateIterator(); KeyIterator; ++KeyIterator)
			{
				const FString& Key = KeyIterator.Key();
				FStringEntry& LiveStringEntry = KeyIterator.Value();
				LiveStringEntry.bIsLocalized = true;
				FInternationalization::Leetify( *LiveStringEntry.String );
			}
		}

		return;
	}
#endif

	TArray<FString> LocalizationPaths;
	if(ShouldLoadGame)
	{
		LocalizationPaths += FPaths::GetGameLocalizationPaths();
	}
	if(ShouldLoadEditor)
	{
		LocalizationPaths += FPaths::GetEditorLocalizationPaths();
	}
	LocalizationPaths += FPaths::GetEngineLocalizationPaths();

	// Prioritized array of localization entry trackers.
	TArray<FLocalizationEntryTracker> LocalizationEntryTrackers;

	// Read culture localization resources.
	FLocalizationEntryTracker& CultureTracker = LocalizationEntryTrackers[LocalizationEntryTrackers.Add(FLocalizationEntryTracker())];
	for(int32 PathIndex = 0; PathIndex < LocalizationPaths.Num(); ++PathIndex)
	{
		const FString& LocalizationPath = LocalizationPaths[PathIndex];
		const FString CulturePath = LocalizationPath / CultureName;

		CultureTracker.ReadFromDirectory(CulturePath);
	}
	CultureTracker.ReportCollisions();

	// Read base language localization resources.
	FLocalizationEntryTracker& BaseLanguageTracker = LocalizationEntryTrackers[LocalizationEntryTrackers.Add(FLocalizationEntryTracker())];
	for(int32 PathIndex = 0; PathIndex < LocalizationPaths.Num(); ++PathIndex)
	{
		const FString& LocalizationPath = LocalizationPaths[PathIndex];
		const FString BaseLanguagePath = LocalizationPath / BaseLanguageName;
		const FString CulturePath = LocalizationPath / CultureName;

		if( BaseLanguagePath != CulturePath )
		{
			BaseLanguageTracker.ReadFromDirectory(BaseLanguagePath);
		}
	}
	BaseLanguageTracker.ReportCollisions();

	UpdateLiveTable(LocalizationEntryTrackers);
}

void FTextLocalizationManager::UpdateLiveTable(const TArray<FLocalizationEntryTracker>& LocalizationEntryTrackers, const bool FilterUpdatesByTableName)
{
	// Update existing localized entries/flag existing unlocalized entries.
	for(auto NamespaceIterator = LiveTable.NamespaceTable.CreateIterator(); NamespaceIterator; ++NamespaceIterator)
	{
		const FString& NamespaceName = NamespaceIterator.Key();
		FTextLookupTable::FKeyTable& LiveKeyTable = NamespaceIterator.Value();
		for(auto KeyIterator = LiveKeyTable.CreateIterator(); KeyIterator; ++KeyIterator)
		{
			const FString& KeyName = KeyIterator.Key();
			FStringEntry& LiveStringEntry = KeyIterator.Value();

			const FLocalizationEntryTracker::FEntry* NewEntry = NULL;

			// Attempt to use resources in prioritized order until we find an entry.
			for(int32 i = 0; i < LocalizationEntryTrackers.Num() && !NewEntry; ++i)
			{
				const FLocalizationEntryTracker& Tracker = LocalizationEntryTrackers[i];
				const FLocalizationEntryTracker::FKeyTable* const NewKeyTable = Tracker.Namespaces.Find(NamespaceName);
				const FLocalizationEntryTracker::FEntryArray* const NewEntryArray = NewKeyTable ? NewKeyTable->Find(KeyName) : NULL;
				const FLocalizationEntryTracker::FEntry* Entry = NewEntryArray && NewEntryArray->Num() ? &((*NewEntryArray)[0]) : NULL;
				NewEntry = (Entry && (!FilterUpdatesByTableName || LiveStringEntry.TableName == Entry->TableName)) ? Entry : NULL;
			}

			if( NewEntry )
			{
				// If an entry is unlocalized and the source hash differs, it suggests that the source hash changed - do not replace the display string.
				if(LiveStringEntry.bIsLocalized || LiveStringEntry.SourceStringHash == NewEntry->SourceStringHash)
				{
					LiveStringEntry.bIsLocalized = true;
					*(LiveStringEntry.String) = NewEntry->LocalizedString;
					LiveStringEntry.SourceStringHash = NewEntry->SourceStringHash;
				}
			}
			else if(!FilterUpdatesByTableName)
			{
				if ( !LiveStringEntry.bIsLocalized && LiveStringEntry.String.Get() == AccessedStringBeforeLocLoadedErrorMsg )
				{
					*(LiveStringEntry.String) = TEXT("");
				}

				LiveStringEntry.bIsLocalized = false;

#if ENABLE_LOC_TESTING
				const bool bShouldLEETIFYUnlocalizedString = FParse::Param(FCommandLine::Get(), TEXT("LEETIFYUnlocalized"));
				if(bShouldLEETIFYUnlocalizedString )
				{
					FInternationalization::Leetify(*(LiveStringEntry.String));
				}
#endif
			}
		}
	}

	// Add new entries. 
	for(int32 i = 0; i < LocalizationEntryTrackers.Num(); ++i)
	{
		const FLocalizationEntryTracker& Tracker = LocalizationEntryTrackers[i];
		for(auto NamespaceIterator = Tracker.Namespaces.CreateConstIterator(); NamespaceIterator; ++NamespaceIterator)
		{
			const FString& NamespaceName = NamespaceIterator.Key();
			const FLocalizationEntryTracker::FKeyTable& NewKeyTable = NamespaceIterator.Value();
			for(auto KeyIterator = NewKeyTable.CreateConstIterator(); KeyIterator; ++KeyIterator)
			{
				const FString& Key = KeyIterator.Key();
				const FLocalizationEntryTracker::FEntryArray& NewEntryArray = KeyIterator.Value();
				const FLocalizationEntryTracker::FEntry& NewEntry = NewEntryArray[0];

				FTextLookupTable::FKeyTable& LiveKeyTable = LiveTable.NamespaceTable.FindOrAdd(NamespaceName);
				FStringEntry* const LiveStringEntry = LiveKeyTable.Find(Key);
				// Note: Anything we find in the table has already been updated above.
				if( !LiveStringEntry )
				{
					FStringEntry NewLiveEntry(
						true,													/*bIsLocalized*/
						NewEntry.TableName,
						NewEntry.SourceStringHash,								/*SourceStringHash*/
						MakeShareable( new FString(NewEntry.LocalizedString) )	/*String*/
						);
					LiveKeyTable.Add( Key, NewLiveEntry );
				}
			}
		}
	}
}

void FTextLocalizationManager::OnCultureChanged()
{
	const bool ShouldLoadEditor = bIsInitialized ? WITH_EDITOR : false;
	const bool ShouldLoadGame = bIsInitialized ? !GIsEditor : false;
	LoadResources(ShouldLoadEditor, ShouldLoadGame);
}

TSharedPtr<FString> FTextLocalizationManager::FindString( const FString& Namespace, const FString& Key )
{
	FScopeLock ScopeLock( &SynchronizationObject );

	// Find namespace's key table.
	const FTextLookupTable::FKeyTable* LiveKeyTable = LiveTable.NamespaceTable.Find( Namespace );

	// Find key table's entry.
	const FStringEntry* LiveEntry = LiveKeyTable ? LiveKeyTable->Find( Key ) : NULL;

	if ( LiveEntry != NULL )
	{
		return LiveEntry->String;
	}

	return NULL;
}

TSharedRef<FString> FTextLocalizationManager::GetString(const FString& Namespace, const FString& Key, const FString* const SourceString)
{
	FScopeLock ScopeLock( &SynchronizationObject );

	// Hack fix for old assets that don't have namespace/key info
	if(Namespace.IsEmpty() && Key.IsEmpty())
	{
		return MakeShareable( new FString( SourceString ? **SourceString : TEXT("") ) );
	}

#if ENABLE_LOC_TESTING
	const bool bShouldLEETIFYAll = bIsInitialized && FInternationalization::GetCurrentCulture()->GetName() == TEXT("LEET");
	static const bool bShouldLEETIFYUnlocalizedString = FCommandLine::IsInitialized() && FParse::Param(FCommandLine::Get(), TEXT("LEETIFYUnlocalized"));
#endif

	// Find namespace's key table.
	FTextLookupTable::FKeyTable* LiveKeyTable = LiveTable.NamespaceTable.Find( Namespace );

	// Find key table's entry.
	FStringEntry* LiveEntry = LiveKeyTable ? LiveKeyTable->Find( Key ) : NULL;

	// Entry is present.
	if( LiveEntry )
	{
		// If we're in some sort of development setting, source may have changed but localization resources have not, in which case the source should be used.
		const uint32 SourceStringHash = SourceString ? FCrc::StrCrc32(**SourceString) : 0;

		// If the source string (hash) is different, the local source has changed and should override - can't be localized.
		if( SourceStringHash != 0 && SourceStringHash != LiveEntry->SourceStringHash )
		{
			LiveEntry->SourceStringHash = SourceStringHash;
			LiveEntry->String.Get() = SourceString ? **SourceString : TEXT("");

#if ENABLE_LOC_TESTING
			if( (bShouldLEETIFYAll || bShouldLEETIFYUnlocalizedString) && SourceString )
			{
				FInternationalization::Leetify(*LiveEntry->String);
				if(*LiveEntry->String == *SourceString)
				{
					UE_LOG(LogTextLocalizationManager, Warning, TEXT("Leetify failed to alter a string (%s)."), **SourceString );
				}
			}
#endif

			UE_LOG(LogTextLocalizationManager, Verbose, TEXT("An attempt was made to get a localized string (Namespace:%s, Key:%s), but the source string hash does not match - the source string (%s) will be used."), *Namespace, *Key, **LiveEntry->String);

#if ENABLE_LOC_TESTING
			LiveEntry->bIsLocalized = bShouldLEETIFYAll;
#else
			LiveEntry->bIsLocalized = false;
#endif
		}

		return LiveEntry->String;
	}
	// Entry is absent.
	else
	{
		// Don't log warnings about unlocalized strings if the system hasn't been initialized - we simply don't have localization data yet.
		if( bIsInitialized )
		{
			UE_LOG(LogTextLocalizationManager, Verbose, TEXT("An attempt was made to get a localized string (Namespace:%s, Key:%s, Source:%s), but it did not exist."), *Namespace, *Key, SourceString ? **SourceString : TEXT(""));
		}

		const TSharedRef<FString> UnlocalizedString = MakeShareable( new FString( SourceString ? **SourceString : TEXT("") ) );

		// If live-culture-swap is enabled or the system is uninitialized - make entries so that they can be updated when system is initialized or a culture swap occurs.
		if( !(bIsInitialized) || ENABLE_LOC_TESTING )
		{
#if ENABLE_LOC_TESTING
			if( (bShouldLEETIFYAll || bShouldLEETIFYUnlocalizedString) && SourceString )
			{
				FInternationalization::Leetify(*UnlocalizedString);
				if(*UnlocalizedString == *SourceString)
				{
					UE_LOG(LogTextLocalizationManager, Warning, TEXT("Leetify failed to alter a string (%s)."), **SourceString );
				}
			}
#endif

			if ( UnlocalizedString->IsEmpty() )
			{
				if ( !bIsInitialized )
				{
					*(UnlocalizedString) = AccessedStringBeforeLocLoadedErrorMsg;
				}
			}

			FStringEntry NewEntry(
#if ENABLE_LOC_TESTING
				bShouldLEETIFYAll						/*bIsLocalized*/
#else
				false												/*bIsLocalized*/
#endif
				, TEXT("")
				, SourceString ? FCrc::StrCrc32(**SourceString) : 0	/*SourceStringHash*/
				, UnlocalizedString									/*String*/
				);

			if( !LiveKeyTable )
			{
				LiveKeyTable = &(LiveTable.NamespaceTable.Add( Namespace, FTextLookupTable::FKeyTable() ));
			}

			LiveKeyTable->Add( Key, NewEntry );
		}

		return UnlocalizedString;
	}
}

namespace
{
	TSharedPtr<FJsonObject> ReadJSONTextFile(const FString& InFilePath)
	{
		//read in file as string
		FString FileContents;
		if ( !FFileHelper::LoadFileToString(FileContents, *InFilePath) )
		{
			UE_LOG(LogTextLocalizationManager, Error,TEXT("Failed to load file %s."), *InFilePath);
			return NULL;
		}

		//parse as JSON
		TSharedPtr<FJsonObject> JSONObject;

		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( FileContents );

		if( !FJsonSerializer::Deserialize( Reader, JSONObject ) || !JSONObject.IsValid())
		{
			UE_LOG(LogTextLocalizationManager, Error,TEXT("Invalid JSON in file %s."), *InFilePath);
			return NULL;
		}

		return JSONObject;
	}
}

void FTextLocalizationManager::RegenerateResources(const FString& ConfigFilePath)
{
	FString SectionName = TEXT("RegenerateResources");

	// Get source path.
	FString SourcePath;
	if( !( GConfig->GetString( *SectionName, TEXT("SourcePath"), SourcePath, ConfigFilePath ) ) )
	{
		UE_LOG(LogTextLocalizationManager, Error, TEXT("No source path specified."));
		return;
	}

	// Get destination path.
	FString DestinationPath;
	if( !( GConfig->GetString( *SectionName, TEXT("DestinationPath"), DestinationPath, ConfigFilePath ) ) )
	{
		UE_LOG(LogTextLocalizationManager, Error, TEXT("No destination path specified."));
		return;
	}

	// Get manifest name.
	FString ManifestName;
	if( !( GConfig->GetString( *SectionName, TEXT("ManifestName"), ManifestName, ConfigFilePath ) ) )
	{
		UE_LOG(LogTextLocalizationManager, Error, TEXT("No manifest name specified."));
		return;
	}

	// Get resource name.
	FString ResourceName;
	if( !( GConfig->GetString( *SectionName, TEXT("ResourceName"), ResourceName, ConfigFilePath ) ) )
	{
		UE_LOG(LogTextLocalizationManager, Error, TEXT("No resource name specified."));
		return;
	}

	TArray<FString> LocaleNames;
	{
		const FString CultureName = FInternationalization::GetCurrentCulture()->GetName();
		LocaleNames.Add(CultureName);
		const FString BaseLanguageName = FInternationalization::GetCurrentCulture()->GetTwoLetterISOLanguageName();
		if(BaseLanguageName != CultureName)
		{
			LocaleNames.Add(BaseLanguageName);
		}
	}

	TArray<TArray<uint8>> BackingBuffers;
	BackingBuffers.SetNum(LocaleNames.Num());

	for(int32 i = 0; i < BackingBuffers.Num(); ++i)
	{
		TArray<uint8>& BackingBuffer = BackingBuffers[i];

		FMemoryWriter MemoryWriter(BackingBuffer, true);

		// Read the manifest file from the source path.
		FString ManifestFilePath = (SourcePath / ManifestName);
		ManifestFilePath = FPaths::ConvertRelativePathToFull(ManifestFilePath);
		TSharedPtr<FJsonObject> ManifestJSONObject = ReadJSONTextFile(ManifestFilePath);
		if( !(ManifestJSONObject.IsValid()) )
		{
			UE_LOG(LogTextLocalizationManager, Error, TEXT("No manifest found at %s."), *ManifestFilePath);
			return;
		}
		TSharedRef<FInternationalizationManifest> InternationalizationManifest = MakeShareable( new FInternationalizationManifest );
		{
			FInternationalizationManifestJsonSerializer ManifestSerializer;
			ManifestSerializer.DeserializeManifest(ManifestJSONObject.ToSharedRef(), InternationalizationManifest);
		}

		// Write resource.
		FTextLocalizationResourceGenerator::Generate(SourcePath, InternationalizationManifest, LocaleNames[i], &(MemoryWriter));

		MemoryWriter.Close();
	}

	// Prioritized array of localization entry trackers.
	TArray<FLocalizationEntryTracker> LocalizationEntryTrackers;
	for(int32 i = 0; i < BackingBuffers.Num(); ++i)
	{
		TArray<uint8>& BackingBuffer = BackingBuffers[i];

		FMemoryReader MemoryReader(BackingBuffer, true);

		const FString CulturePath = DestinationPath / LocaleNames[i];

		const FString ResourceFilePath = FPaths::ConvertRelativePathToFull(CulturePath / ResourceName);

		FLocalizationEntryTracker& CultureTracker = LocalizationEntryTrackers[LocalizationEntryTrackers.Add(FLocalizationEntryTracker())];
		CultureTracker.ReadFromArchive(MemoryReader, ResourceFilePath);
		CultureTracker.ReportCollisions();

		MemoryReader.Close();
	}

	UpdateLiveTable(LocalizationEntryTrackers, true);
}