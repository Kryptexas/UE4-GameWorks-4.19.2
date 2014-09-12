// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BuildPatchManifest.cpp: Implements the manifest classes.
=============================================================================*/

#include "BuildPatchServicesPrivatePCH.h"

#define LOCTEXT_NAMESPACE "BuildPatchManifest"

/**
 * Helper functions that convert generic types to and from string blobs for use with JSON parsing.
 * It's kind of horrible but guarantees no loss of data as the JSON reader/writer only supports float functionality 
 * which would result in data loss with high int32 values, and we'll be using uint64.
 */
template< typename DataType >
bool FromStringBlob( const FString& StringBlob, DataType& ValueOut )
{
	void* AsBuffer = &ValueOut;
	return FString::ToBlob( StringBlob, static_cast< uint8* >( AsBuffer ), sizeof( DataType ) );
}
template< typename DataType >
FString ToStringBlob( const DataType& DataVal )
{
	const void* AsBuffer = &DataVal;
	return FString::FromBlob( static_cast<const uint8*>( AsBuffer ), sizeof( DataType ) );
}

/* EBuildPatchAppManifestVersion implementation
*****************************************************************************/
const EBuildPatchAppManifestVersion::Type EBuildPatchAppManifestVersion::GetLatestVersion()
{
	return static_cast<EBuildPatchAppManifestVersion::Type>(LatestPlusOne - 1);
}

const FString& EBuildPatchAppManifestVersion::GetChunkSubdir(const EBuildPatchAppManifestVersion::Type ManifestVersion)
{
	static const FString ChunksV1 = TEXT("Chunks");
	static const FString ChunksV2 = TEXT("ChunksV2");
	static const FString ChunksV3 = TEXT("ChunksV3");
	return ManifestVersion < DataFileRenames ? ChunksV1
		: ManifestVersion < ChunkCompressionSupport ? ChunksV2
		: ChunksV3;
}

const FString& EBuildPatchAppManifestVersion::GetFileSubdir(const EBuildPatchAppManifestVersion::Type ManifestVersion)
{
	static const FString FilesV1 = TEXT("Files");
	static const FString FilesV2 = TEXT("FilesV2");
	return ManifestVersion < DataFileRenames ? FilesV1
		: FilesV2;
}

/* FBuildPatchCustomField implementation
*****************************************************************************/
FBuildPatchCustomField::FBuildPatchCustomField( const FString& Value )
	: CustomValue( Value )
{
}

const FString FBuildPatchCustomField::AsString() const
{
	return CustomValue;
}

const double FBuildPatchCustomField::AsDouble() const
{
	// The Json parser currently only supports float so we have to decode string blob instead
	double Rtn;
	if( FromStringBlob( CustomValue, Rtn ) )
	{
		return Rtn;
	}
	return 0;
}

const int64 FBuildPatchCustomField::AsInteger() const
{
	// The Json parser currently only supports float so we have to decode string blob instead
	int64 Rtn;
	if( FromStringBlob( CustomValue, Rtn ) )
	{
		return Rtn;
	}
	return 0;
}

/* FBuildPatchFileManifest implementation
*****************************************************************************/

FBuildPatchFileManifest::FBuildPatchFileManifest()
	: Filename( TEXT( "" ) )
#if PLATFORM_MAC
	, bIsUnixExecutable( false )
	, SymlinkTarget( TEXT( "" ) )
#endif
	, FileSize( INDEX_NONE )
{
}

const int64& FBuildPatchFileManifest::GetFileSize()
{
	if( FileSize == INDEX_NONE )
	{
		FileSize = 0;
		for( auto FileChunkPartIt = FileChunkParts.CreateConstIterator(); FileChunkPartIt; ++FileChunkPartIt)
		{
			FileSize += (*FileChunkPartIt).Size;
		}
	}
	return FileSize;
}

/* FBuildPatchAppManifest implementation
*****************************************************************************/

FBuildPatchAppManifest::FBuildPatchAppManifest()
	: ManifestFileVersion( EBuildPatchAppManifestVersion::GetLatestVersion() )
{
}

FBuildPatchAppManifest::FBuildPatchAppManifest( const uint32& InAppID, const FString& AppName )
	: ManifestFileVersion( EBuildPatchAppManifestVersion::GetLatestVersion() )
	, AppID( InAppID )
	, AppNameString( AppName )
	, TotalBuildSize( INDEX_NONE )
{
}

FBuildPatchAppManifest::~FBuildPatchAppManifest()
{
	// Make sure the hash data is cleaned up
	DestroyData();
}

void FBuildPatchAppManifest::DestroyData()
{
	// Clear Manifest data
	AppNameString = "";
	BuildVersionString = "";
	LaunchExeString = "";
	LaunchCommandString = "";
	PrereqNameString = "";
	PrereqPathString = "";
	PrereqArgsString = "";
	FileManifestList.Empty();
	ChunkHashList.Empty();
}

bool FBuildPatchAppManifest::SaveToFile(FString Filename)
{
	bool bSuccess = true;
	FArchive* FileOut = IFileManager::Get().CreateFileWriter(*Filename);
	bSuccess &= FileOut != NULL;
	if (bSuccess)
	{
		FString JSONOutput;
		SerializeToJSON(JSONOutput);
		FileOut->Serialize(TCHAR_TO_ANSI(*JSONOutput), JSONOutput.Len());
		FileOut->Close();
		delete FileOut;
	}
	return bSuccess;
}

bool FBuildPatchAppManifest::LoadFromFile( FString Filename )
{
	FString JSONString;
	bool bSuccess = FFileHelper::LoadFileToString( JSONString, *Filename );
	if( bSuccess )
	{
		bSuccess &= DeserializeFromJSON( JSONString );
	}
	return bSuccess;
}

void FBuildPatchAppManifest::SerializeToJSON( FString& JSONOutput )
{
#if UE_BUILD_DEBUG // We'll use this to switch between human readable JSON
	TSharedRef< TJsonWriter< TCHAR, TPrettyJsonPrintPolicy< TCHAR > > > Writer = TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy< TCHAR > >::Create(&JSONOutput);
#else
	TSharedRef< TJsonWriter< TCHAR, TCondensedJsonPrintPolicy< TCHAR > > > Writer = TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy< TCHAR > >::Create( &JSONOutput );
#endif //ALLOW_DEBUG_FILES

	Writer->WriteObjectStart();
	{
		// Write general data
		Writer->WriteValue( TEXT("ManifestFileVersion"), ToStringBlob( static_cast< int32 >( ManifestFileVersion ) ) );
		Writer->WriteValue( TEXT("bIsFileData"), bIsFileData );
		Writer->WriteValue( TEXT("AppID"), ToStringBlob( AppID ) );
		Writer->WriteValue( TEXT("AppNameString"), AppNameString );
		Writer->WriteValue( TEXT("BuildVersionString"), BuildVersionString );
		Writer->WriteValue( TEXT("LaunchExeString"), LaunchExeString );
		Writer->WriteValue( TEXT("LaunchCommand"), LaunchCommandString );
		Writer->WriteValue( TEXT("PrereqName"), PrereqNameString );
		Writer->WriteValue( TEXT("PrereqPath"), PrereqPathString );
		Writer->WriteValue( TEXT("PrereqArgs"), PrereqArgsString );
		// Write file manifest data
		Writer->WriteArrayStart( TEXT("FileManifestList") );
		for( auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt; ++FileManifestIt )
		{
			const TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestIt.Value();
			Writer->WriteObjectStart();
			{
				Writer->WriteValue( TEXT("Filename"), FileManifest->Filename );
				Writer->WriteValue( TEXT("FileHash"), FString::FromBlob( FileManifestIt.Value()->FileHash.Hash, FSHA1::DigestSize ) );
#if PLATFORM_MAC
				if( FileManifest->bIsUnixExecutable )
				{
					Writer->WriteValue( TEXT("bIsUnixExecutable"), FileManifest->bIsUnixExecutable );
				}
				const bool bIsSymlink = !FileManifest->SymlinkTarget.IsEmpty();
				if( bIsSymlink )
				{
					Writer->WriteValue( TEXT("SymlinkTarget"), FileManifest->SymlinkTarget );
				}
				else
#endif
				{
					Writer->WriteArrayStart( TEXT("FileChunkParts") );
					{
						for( auto FileChunkPartIt = FileManifestIt.Value()->FileChunkParts.CreateConstIterator(); FileChunkPartIt; ++FileChunkPartIt )
						{
							const FChunkPart& FileChunkPart = *FileChunkPartIt;
							Writer->WriteObjectStart();
							{
								Writer->WriteValue( TEXT("Guid"), FileChunkPart.Guid.ToString() );
								Writer->WriteValue( TEXT("Offset"), ToStringBlob( FileChunkPart.Offset ) );
								Writer->WriteValue( TEXT("Size"), ToStringBlob( FileChunkPart.Size ) );
							}
							Writer->WriteObjectEnd();
						}
					}
					Writer->WriteArrayEnd();
				}
			}
			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();
		// Write chunk hash list
		Writer->WriteObjectStart( TEXT("ChunkHashList") );
		for( auto ChunkHashListIt = ChunkHashList.CreateConstIterator(); ChunkHashListIt; ++ChunkHashListIt )
		{
			const FGuid& ChunkGuid = ChunkHashListIt.Key();
			const uint64& ChunkHash = ChunkHashListIt.Value();
			Writer->WriteValue( ChunkGuid.ToString(), ToStringBlob( ChunkHash ) );
		}
		Writer->WriteObjectEnd();
		// Write data group list
		Writer->WriteObjectStart( TEXT("DataGroupList") );
		for( auto DataGroupListIt = DataGroupList.CreateConstIterator(); DataGroupListIt; ++DataGroupListIt )
		{
			const FGuid& DataGuid = DataGroupListIt.Key();
			const uint8& DataGroup = DataGroupListIt.Value();
			Writer->WriteValue( DataGuid.ToString(), ToStringBlob( DataGroup ) );
		}
		Writer->WriteObjectEnd();
		// Write chunk size list
		Writer->WriteObjectStart(TEXT("ChunkFilesizeList"));
		for (auto ChunkFilesizeListIt = ChunkFilesizeList.CreateConstIterator(); ChunkFilesizeListIt; ++ChunkFilesizeListIt)
		{
			const FGuid& ChunkGuid = ChunkFilesizeListIt.Key();
			const int64& ChunkSize = ChunkFilesizeListIt.Value();
			Writer->WriteValue(ChunkGuid.ToString(), ToStringBlob(ChunkSize));
		}
		Writer->WriteObjectEnd();
		// Write custom fields
		Writer->WriteObjectStart( TEXT("CustomFields") );
		for( auto CustomFieldsIt = CustomFields.CreateConstIterator(); CustomFieldsIt; ++CustomFieldsIt )
		{
			Writer->WriteValue( CustomFieldsIt.Key(), CustomFieldsIt.Value() );
		}
		Writer->WriteObjectEnd();
	}
	Writer->WriteObjectEnd();

	Writer->Close();
}

// @TODO LSwift: Perhaps replace FromBlob and ToBlob usage with hexadecimal notation instead
bool FBuildPatchAppManifest::DeserializeFromJSON( const FString& JSONInput )
{
	bool bSuccess = true;
	TSharedPtr< FJsonObject > JSONManifestObject;
	TSharedRef< TJsonReader< TCHAR > > Reader = TJsonReaderFactory< TCHAR >::Create( JSONInput );

	// Clear current data
	DestroyData();

	// Attempt to deserialize JSON
	if( !FJsonSerializer::Deserialize( Reader, JSONManifestObject ) || !JSONManifestObject.IsValid() )
	{
		return false;
	}

	// Get the values map
	TMap< FString, TSharedPtr< FJsonValue > >& JsonValueMap = JSONManifestObject->Values;

	// Get the app and version strings
	TSharedPtr< FJsonValue > JsonAppID = JsonValueMap.FindRef( TEXT("AppID") );
	TSharedPtr< FJsonValue > JsonAppNameString = JsonValueMap.FindRef( TEXT("AppNameString") );
	TSharedPtr< FJsonValue > JsonBuildVersionString = JsonValueMap.FindRef( TEXT("BuildVersionString") );
	TSharedPtr< FJsonValue > JsonLaunchExe = JsonValueMap.FindRef( TEXT("LaunchExeString") );
	TSharedPtr< FJsonValue > JsonLaunchCommand = JsonValueMap.FindRef( TEXT("LaunchCommand") );
	TSharedPtr< FJsonValue > JsonPrereqName = JsonValueMap.FindRef( TEXT("PrereqName") );
	TSharedPtr< FJsonValue > JsonPrereqPath = JsonValueMap.FindRef( TEXT("PrereqPath") );
	TSharedPtr< FJsonValue > JsonPrereqArgs = JsonValueMap.FindRef( TEXT("PrereqArgs") );
	bSuccess = bSuccess && JsonAppID.IsValid();
	if( bSuccess )
	{
		bSuccess = bSuccess && FromStringBlob( JsonAppID->AsString(), AppID );
	}
	bSuccess = bSuccess && JsonAppNameString.IsValid();
	if( bSuccess )
	{
		AppNameString = JsonAppNameString->AsString();
	}
	bSuccess = bSuccess && JsonBuildVersionString.IsValid();
	if( bSuccess )
	{
		BuildVersionString = JsonBuildVersionString->AsString();
	}
	bSuccess = bSuccess && JsonLaunchExe.IsValid();
	if( bSuccess )
	{
		LaunchExeString = JsonLaunchExe->AsString();
	}
	bSuccess = bSuccess && JsonLaunchCommand.IsValid();
	if( bSuccess )
	{
		LaunchCommandString = JsonLaunchCommand->AsString();
	}

	// Get the prerequisites installer info.  These are optional entries.
	PrereqNameString = JsonPrereqName.IsValid() ? JsonPrereqName->AsString() : FString();
	PrereqPathString = JsonPrereqPath.IsValid() ? JsonPrereqPath->AsString() : FString();
	PrereqArgsString = JsonPrereqArgs.IsValid() ? JsonPrereqArgs->AsString() : FString();

	// Manifest version did not always exist
	int32 ManifestFileVersionInt = 0;
	TSharedPtr< FJsonValue > JsonManifestFileVersion = JsonValueMap.FindRef( TEXT("ManifestFileVersion") );
	if( JsonManifestFileVersion.IsValid() && FromStringBlob( JsonManifestFileVersion->AsString(), ManifestFileVersionInt ) )
	{
		ManifestFileVersion = static_cast< EBuildPatchAppManifestVersion::Type >( ManifestFileVersionInt );
	}
	else
	{
		// Then we presume version just before we started outputting the version
		ManifestFileVersion = static_cast< EBuildPatchAppManifestVersion::Type >( EBuildPatchAppManifestVersion::StartStoringVersion - 1 );
	}

	// Store a list of all data GUID for later use
	TSet< FGuid > AllDataGuids;

	// Get the FileManifestList
	TSharedPtr< FJsonValue > JsonFileManifestList = JsonValueMap.FindRef( TEXT("FileManifestList") );
	bSuccess = bSuccess && JsonFileManifestList.IsValid();
	if( bSuccess )
	{
		TArray< TSharedPtr< FJsonValue > > JsonFileManifestArray = JsonFileManifestList->AsArray();
		for ( auto JsonFileManifestIt = JsonFileManifestArray.CreateConstIterator(); JsonFileManifestIt && bSuccess; ++JsonFileManifestIt )
		{
			TSharedPtr< FJsonObject > JsonFileManifest = (*JsonFileManifestIt)->AsObject();
			TSharedRef< FBuildPatchFileManifest > FileManifest = MakeShareable( new FBuildPatchFileManifest() );
			FileManifest->Filename = JsonFileManifest->GetStringField( TEXT("Filename") );
#if PLATFORM_MAC
			FileManifest->bIsUnixExecutable = JsonFileManifest->HasField( TEXT("bIsUnixExecutable") ) && JsonFileManifest->GetBoolField( TEXT("bIsUnixExecutable") );
			FileManifest->SymlinkTarget = JsonFileManifest->HasField( TEXT("SymlinkTarget") ) ? JsonFileManifest->GetStringField( TEXT("SymlinkTarget") ) : TEXT( "" );
#endif
			FileManifestList.Add( FileManifest->Filename, FileManifest );
			bSuccess = bSuccess && FString::ToBlob( JsonFileManifest->GetStringField( TEXT("FileHash") ), FileManifest->FileHash.Hash, FSHA1::DigestSize );
			TArray< TSharedPtr< FJsonValue > > JsonChunkPartArray = JsonFileManifest->GetArrayField( TEXT("FileChunkParts") );
			for ( auto JsonChunkPartIt = JsonChunkPartArray.CreateConstIterator(); JsonChunkPartIt && bSuccess; ++JsonChunkPartIt )
			{
				FileManifest->FileChunkParts.Add( FChunkPart() );
				TSharedPtr< FJsonObject > JsonChunkPart = (*JsonChunkPartIt)->AsObject();
				FChunkPart& FileChunkPart = FileManifest->FileChunkParts.Top();
				bSuccess = bSuccess && FGuid::Parse( JsonChunkPart->GetStringField( TEXT("Guid") ), FileChunkPart.Guid );
				bSuccess = bSuccess && FromStringBlob( JsonChunkPart->GetStringField( TEXT("Offset") ), FileChunkPart.Offset );
				bSuccess = bSuccess && FromStringBlob( JsonChunkPart->GetStringField( TEXT("Size") ), FileChunkPart.Size );
				AllDataGuids.Add( FileChunkPart.Guid );
			}
		}
	}

	// Get the ChunkHashList
	TSharedPtr< FJsonValue > JsonChunkHashList = JsonValueMap.FindRef( TEXT("ChunkHashList") );
	bSuccess = bSuccess && JsonChunkHashList.IsValid();
	if( bSuccess )
	{
		TSharedPtr< FJsonObject > JsonChunkHashListObj = JsonChunkHashList->AsObject();
		for( auto ChunkHashIt = JsonChunkHashListObj->Values.CreateConstIterator(); ChunkHashIt && bSuccess; ++ChunkHashIt )
		{
			FGuid ChunkGuid;
			uint64 ChunkHash = 0;
			bSuccess = bSuccess && FGuid::Parse( ChunkHashIt.Key(), ChunkGuid );
			bSuccess = bSuccess && FromStringBlob( ChunkHashIt.Value()->AsString(), ChunkHash );
			ChunkHashList.Add( ChunkGuid, ChunkHash );
			AllDataGuids.Add( ChunkGuid );
		}
	}

	// Get the bIsFileData value. The variable will exist in versions of StoresIfChunkOrFileData or later, otherwise the previous method is to check
	// if ChunkHashList is empty.
	TSharedPtr< FJsonValue > JsonIsFileData = JsonValueMap.FindRef( TEXT("bIsFileData") );
	if( JsonIsFileData.IsValid() && JsonIsFileData->Type == EJson::Boolean )
	{
		bIsFileData = JsonIsFileData->AsBool();
	}
	else
	{
		bIsFileData = ChunkHashList.Num() == 0;
	}

	// Get the DataGroupList
	TSharedPtr< FJsonValue > JsonDataGroupList = JsonValueMap.FindRef( TEXT("DataGroupList") );
	if( JsonDataGroupList.IsValid() )
	{
		TSharedPtr< FJsonObject > JsonDataGroupListObj = JsonDataGroupList->AsObject();
		for( auto DataGroupIt = JsonDataGroupListObj->Values.CreateConstIterator(); DataGroupIt && bSuccess; ++DataGroupIt )
		{
			FGuid DataGuid;
			uint8 DataGroup;
			// If the list exists, we must be able to parse it ok otherwise error
			bSuccess = bSuccess && FGuid::Parse( DataGroupIt.Key(), DataGuid );
			bSuccess = bSuccess && FromStringBlob( DataGroupIt.Value()->AsString(), DataGroup );
			DataGroupList.Add( DataGuid, DataGroup );
		}
	}
	else if( bSuccess )
	{
		// If the list did not exist in the manifest then the grouping is the deprecated crc functionality, as long
		// as there are no previous parsing errors we can build the group list from the AllDataGuids list.
		for( auto AllDataGuidsIt = AllDataGuids.CreateConstIterator(); AllDataGuidsIt; ++AllDataGuidsIt )
		{
			const FGuid& DataGuid = *AllDataGuidsIt;
			uint8 DataGroup = FCrc::MemCrc_DEPRECATED( &DataGuid, sizeof( FGuid ) ) % 100;
			DataGroupList.Add( DataGuid, DataGroup );
		}
	}

	// Get the ChunkFilesizeList
	TSharedPtr< FJsonValue > JsonChunkFilesizeList = JsonValueMap.FindRef(TEXT("ChunkFilesizeList"));
	if (JsonChunkFilesizeList.IsValid())
	{
		TSharedPtr< FJsonObject > JsonChunkFilesizeListObj = JsonChunkFilesizeList->AsObject();
		for (auto ChunkFilesizeIt = JsonChunkFilesizeListObj->Values.CreateConstIterator(); ChunkFilesizeIt; ++ChunkFilesizeIt)
		{
			FGuid ChunkGuid;
			int64 ChunkSize = 0;
			if (FGuid::Parse(ChunkFilesizeIt.Key(), ChunkGuid))
			{
				FromStringBlob(ChunkFilesizeIt.Value()->AsString(), ChunkSize);
				ChunkFilesizeList.Add(ChunkGuid, ChunkSize);
			}
		}
	}

	// Get the custom fields. This is optional, and should not fail if it does not exist
	TSharedPtr< FJsonValue > JsonCustomFields = JsonValueMap.FindRef( TEXT( "CustomFields" ) );
	if( JsonCustomFields.IsValid() )
	{
		TSharedPtr< FJsonObject > JsonCustomFieldsObj = JsonCustomFields->AsObject();
		for( auto CustomFieldIt = JsonCustomFieldsObj->Values.CreateConstIterator(); CustomFieldIt && bSuccess; ++CustomFieldIt )
		{
			CustomFields.Add( CustomFieldIt.Key(), CustomFieldIt.Value()->AsString() );
		}
	}

	// If this is file data, fill out the guid to filename lookup
	if( bIsFileData )
	{
		for( auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt; ++FileManifestIt )
		{
			const TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestIt.Value();
			check( FileManifest->FileChunkParts.Num() == 1 );
			FileNameReference.Add( FileManifest->FileChunkParts[ 0 ].Guid, FileManifest->Filename );
		}
	}

	// Calculate build size
	TotalBuildSize = 0;
	for (auto& File : FileManifestList)
	{
		if (File.Value.IsValid())
		{
			TotalBuildSize += File.Value->GetFileSize();
		}
	}

	// Make sure we don't have any half loaded data
	if( !bSuccess )
	{
		DestroyData();
	}

	return bSuccess;
}

const EBuildPatchAppManifestVersion::Type FBuildPatchAppManifest::GetManifestVersion() const
{
	return ManifestFileVersion;
}

void FBuildPatchAppManifest::GetChunksRequiredForFiles( const TArray< FString >& FileList, TArray< FGuid >& RequiredChunks, bool bAddUnique ) const
{
	for( auto FileIt = FileList.CreateConstIterator(); FileIt ; ++FileIt )
	{
		TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestList.FindRef( *FileIt );
		if( FileManifest.IsValid() )
		{
			for( auto ChunkPartIt = FileManifest->FileChunkParts.CreateConstIterator(); ChunkPartIt; ++ChunkPartIt )
			{
				const FChunkPart& ChunkPart = *ChunkPartIt;
				if( bAddUnique )
				{
					RequiredChunks.AddUnique( ChunkPart.Guid );
				}
				else
				{
					RequiredChunks.Add( ChunkPart.Guid );
				}
			}
		}
	}
}

const int64 FBuildPatchAppManifest::GetDownloadSize() const
{
	// If this is not a chunked build, then return build size
	if (bIsFileData)
	{
		return GetBuildSize();
	}
	else
	{
		TArray<FGuid> ChunkList;
		ChunkHashList.GetKeys(ChunkList);
		return GetDataSize(ChunkList);
	}
}

const int64 FBuildPatchAppManifest::GetBuildSize() const
{
	return TotalBuildSize;
}

const int64 FBuildPatchAppManifest::GetFileSize( const TArray< FString >& Filenames ) const
{
	int64 TotalSize = 0;
	for( int32 FileIdx = 0; FileIdx < Filenames.Num(); ++FileIdx )
	{
		TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestList.FindRef( Filenames[ FileIdx ] );
		if( FileManifest.IsValid() )
		{
			TotalSize += FileManifest->GetFileSize();
		}
	}
	return TotalSize;
}

const int64 FBuildPatchAppManifest::GetFileSize( const FString& Filename ) const
{
	TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestList.FindRef( Filename );
	if( FileManifest.IsValid() )
	{
		return FileManifest->GetFileSize();
	}
	return 0;
}

const int64 FBuildPatchAppManifest::GetDataSize(const FGuid& DataGuid) const
{
	if (bIsFileData)
	{
		// For file data, the file must exist in the list
		check(FileNameReference.Contains(DataGuid));
		return GetFileSize(FileNameReference[DataGuid]);
	}
	else if (ChunkFilesizeList.Contains(DataGuid))
	{
		// Chunk data sizes are stored in our map
		return ChunkFilesizeList[DataGuid];
	}
	else
	{
		// Default chunk size to be the data size. Inaccurate, but represents original behavior.
		return FBuildPatchData::ChunkDataSize;
	}
}

const int64 FBuildPatchAppManifest::GetDataSize(const TArray< FGuid >& DataGuids) const
{
	int64 TotalSize = 0;
	for (auto DataGuidsIt = DataGuids.CreateConstIterator(); DataGuidsIt; ++DataGuidsIt)
	{
		TotalSize += GetDataSize(*DataGuidsIt);
	}
	return TotalSize;
}

const uint32 FBuildPatchAppManifest::GetNumFiles() const
{
	return FileManifestList.Num();
}

void FBuildPatchAppManifest::GetFileList( TArray< FString >& Filenames ) const
{
	FileManifestList.GetKeys( Filenames );
}

void FBuildPatchAppManifest::GetDataList(TArray< FGuid >& DataGuids) const
{
	ChunkHashList.GetKeys( DataGuids );
}

const TSharedPtr< FBuildPatchFileManifest > FBuildPatchAppManifest::GetFileManifest( const FString& Filename )
{
	return FileManifestList.FindRef( Filename );
}

const bool FBuildPatchAppManifest::IsFileDataManifest() const
{
	return bIsFileData;
}

const bool FBuildPatchAppManifest::GetChunkHash( const FGuid& ChunkGuid, uint64& OutHash ) const
{
	const uint64* ChunkHash = ChunkHashList.Find( ChunkGuid );
	if( ChunkHash != NULL )
	{
		OutHash = *ChunkHash;
		return true;
	}
	return false;
}

const bool FBuildPatchAppManifest::GetFileDataHash( const FGuid& FileGuid, FSHAHash& OutHash ) const
{
	// File data hash is the same as the hash for the file it holds, so we can grab it from there
	for( auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt; ++FileManifestIt )
	{
		if( FileManifestIt.Value().IsValid() )
		{
			const FBuildPatchFileManifest& FileManifest = *FileManifestIt.Value().Get();
			for( auto FileChunkPartsIt = FileManifest.FileChunkParts.CreateConstIterator(); FileChunkPartsIt; ++FileChunkPartsIt )
			{
				const FChunkPart& FileDataPart = *FileChunkPartsIt;
				if( FileDataPart.Guid == FileGuid )
				{
					OutHash = FileManifest.FileHash;
					return true;
				}
			}
		}
	}
	return false;
}

const uint32& FBuildPatchAppManifest::GetAppID() const
{
	return AppID;
}

const FString& FBuildPatchAppManifest::GetAppName() const
{
	return AppNameString;
}

const FString& FBuildPatchAppManifest::GetVersionString() const
{
	return BuildVersionString;
}

const FString& FBuildPatchAppManifest::GetLaunchExe() const
{
	return LaunchExeString;
}

const FString& FBuildPatchAppManifest::GetLaunchCommand() const
{
	return LaunchCommandString;
}

const FString& FBuildPatchAppManifest::GetPrereqName() const
{
	return PrereqNameString;
}

const FString& FBuildPatchAppManifest::GetPrereqPath() const
{
	return PrereqPathString;
}

const FString& FBuildPatchAppManifest::GetPrereqArgs() const
{
	return PrereqArgsString;
}

IBuildManifestRef FBuildPatchAppManifest::Duplicate() const
{
	return MakeShareable(new FBuildPatchAppManifest(*this));
}

const IManifestFieldPtr FBuildPatchAppManifest::GetCustomField( const FString& FieldName ) const
{
	IManifestFieldPtr Return;
	if( CustomFields.Contains( FieldName ) )
	{
		Return = MakeShareable( new FBuildPatchCustomField( CustomFields.FindRef( FieldName ) ) );
	}
	return Return;
}

const IManifestFieldPtr FBuildPatchAppManifest::SetCustomField( const FString& FieldName, const FString& Value )
{
	return MakeShareable( new FBuildPatchCustomField( CustomFields.Add( FieldName, Value ) ) );
}

const IManifestFieldPtr FBuildPatchAppManifest::SetCustomField( const FString& FieldName, const double& Value )
{
	return MakeShareable( new FBuildPatchCustomField( CustomFields.Add( FieldName, ToStringBlob( Value ) ) ) );
}

const IManifestFieldPtr FBuildPatchAppManifest::SetCustomField( const FString& FieldName, const int64& Value )
{
	return MakeShareable( new FBuildPatchCustomField( CustomFields.Add( FieldName, ToStringBlob( Value ) ) ) );
}

void FBuildPatchAppManifest::EnumerateProducibleChunks( const FString& InstallDirectory, const TArray< FGuid >& ChunksRequired, TArray< FGuid >& ChunksAvailable ) const
{
	// A struct that will store byte ranges
	struct FChunkRange
	{
		// The inclusive min byte
		uint32 Min;
		// The inclusive max byte
		uint32 Max;
	};
	// A struct that will sort an FChunkRange array by Min
	struct FCompareFChunkRangeByMin
	{
		FORCEINLINE bool operator()( const FChunkRange& A, const FChunkRange& B ) const
		{
			return A.Min < B.Min;
		}
	};
	// The first thing we need is an inventory of FFileChunkParts that we can get which refer to our required chunks
	TMap< FGuid, TArray< FFileChunkPart > > ChunkPartInventory;
	EnumerateChunkPartInventory( ChunksRequired, ChunkPartInventory );

	// For each chunk that we found a part for, check that the union of chunk parts we have for it, contains all bytes of the chunk
	for( auto ChunkPartInventoryIt = ChunkPartInventory.CreateConstIterator(); ChunkPartInventoryIt; ++ChunkPartInventoryIt )
	{
		// Create an array of FChunkRanges that describes the chunk data that we have available
		const FGuid& ChunkGuid = ChunkPartInventoryIt.Key();
		const TArray< FFileChunkPart >& ChunkParts = ChunkPartInventoryIt.Value();
		if( ChunksAvailable.Contains( ChunkGuid ) )
		{
			continue;
		}
		TArray< FChunkRange > ChunkRanges;
		for( auto ChunkPartIt = ChunkParts.CreateConstIterator(); ChunkPartIt; ++ChunkPartIt )
		{
			const FFileChunkPart& FileChunkPart = *ChunkPartIt;
			const int64 SourceFilesize = IFileManager::Get().FileSize( *(InstallDirectory / FileChunkPart.Filename) );
			const int64 LastRequiredByte = FileChunkPart.FileOffset + FileChunkPart.ChunkPart.Size;
			if( SourceFilesize == GetFileSize( FileChunkPart.Filename ) && SourceFilesize >= LastRequiredByte )
			{
				const uint32 Min = FileChunkPart.ChunkPart.Offset;
				const uint32 Max = Min + FileChunkPart.ChunkPart.Size - 1;
				// We will store our ranges using inclusive values
				FChunkRange NextRange;
				NextRange.Min = Min;
				NextRange.Max = Max;
				ChunkRanges.Add( NextRange );
			}
		}
		ChunkRanges.Sort( FCompareFChunkRangeByMin() );
		// Now we have a sorted array of ChunkPart ranges, we walk this array to find a gap in available data
		// which would mean we cannot generate this chunk
		uint32 ByteCount = 0;
		for( auto ChunkRangeIt = ChunkRanges.CreateConstIterator(); ChunkRangeIt; ++ChunkRangeIt )
		{
			const FChunkRange& ChunkRange = *ChunkRangeIt;
			if( ChunkRange.Min <= ByteCount && ChunkRange.Max >= ByteCount )
			{
				ByteCount = ChunkRange.Max;
			}
			else
			{
				break;
			}
		}
		// If we can make the chunk, add it to the list
		const bool bCanMakeChunk = ByteCount == ( FBuildPatchData::ChunkDataSize - 1 );
		if( bCanMakeChunk )
		{
			ChunksAvailable.AddUnique( ChunkGuid );
		}
	}
}

bool FBuildPatchAppManifest::VerifyAgainstDirectory( const FString& VerifyDirectory, TArray < FString >& OutDatedFiles, FBuildPatchFloatDelegate ProgressDelegate, FBuildPatchBoolRetDelegate ShouldPauseDelegate, double& TimeSpentPaused )
{
	bool bAllCorrect = true;
	OutDatedFiles.Empty();
	TimeSpentPaused = 0;

	// Setup progress tracking
	double TotalBuildSize = GetBuildSize();
	double ProcessedBytes = 0;
	struct FLocalProgress
	{
		double CurrentBuildPercentage;
		double CurrentFileWeight;
		FBuildPatchFloatDelegate ProgressDelegate;
		void PerFileProgress( float Progress )
		{
			const double PercentComplete = CurrentBuildPercentage + ( Progress * CurrentFileWeight );
			ProgressDelegate.ExecuteIfBound( PercentComplete );
		}
	} LocalProgress;
	LocalProgress.CurrentBuildPercentage = 0;
	LocalProgress.ProgressDelegate = ProgressDelegate;

	// For all files in FileManifestList, check that they produce the correct SHA1 hash, adding any that don't to the list
	for( auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt && !FBuildPatchInstallError::HasFatalError(); ++FileManifestIt )
	{
		// Pause if necessary
		const double PrePauseTime = FPlatformTime::Seconds();
		double PostPauseTime = PrePauseTime;
		bool bShouldPause = ShouldPauseDelegate.IsBound() && ShouldPauseDelegate.Execute();
		while( bShouldPause )
		{
			FPlatformProcess::Sleep( 0.1f );
			bShouldPause = ShouldPauseDelegate.Execute();
			PostPauseTime = FPlatformTime::Seconds();
		}
		// Count up pause time
		TimeSpentPaused += PostPauseTime - PrePauseTime;
		// Get file details and construct
		TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestIt.Value();
		FString FullFilename = VerifyDirectory / FileManifest->Filename;
		const int64 CurrentFileSize = FileManifest->GetFileSize();
		LocalProgress.CurrentFileWeight = CurrentFileSize / TotalBuildSize;
		const bool bSameFile = FBuildPatchUtils::VerifyFile(FullFilename, FileManifest->FileHash, FileManifest->FileHash, FBuildPatchFloatDelegate::CreateRaw(&LocalProgress, &FLocalProgress::PerFileProgress), ShouldPauseDelegate, TimeSpentPaused) != 0;
		ProcessedBytes += CurrentFileSize;
		LocalProgress.CurrentBuildPercentage = ProcessedBytes / TotalBuildSize;
		bAllCorrect &= bSameFile;
		if( !bSameFile )
		{
			OutDatedFiles.Add( FileManifest->Filename );
		}
	}

	return bAllCorrect && !FBuildPatchInstallError::HasFatalError();
}

void FBuildPatchAppManifest::EnumerateChunkPartInventory( const TArray< FGuid >& ChunksRequired, TMap< FGuid, TArray< FFileChunkPart > >& ChunkPartsAvailable ) const
{
	ChunkPartsAvailable.Empty();
	// For each file in the manifest, check what chunks it is made out of, and grab details for the ones in ChunksRequired
	for (auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt && !FBuildPatchInstallError::HasFatalError(); ++FileManifestIt)
	{
		const TSharedPtr< FBuildPatchFileManifest > FileManifest = FileManifestIt.Value();
		uint64 FileOffset = 0;
		for (auto ChunkPartIt = FileManifest->FileChunkParts.CreateConstIterator(); ChunkPartIt && !FBuildPatchInstallError::HasFatalError(); ++ChunkPartIt)
		{
			const FChunkPart& ChunkPart = *ChunkPartIt;
			if( ChunksRequired.Contains( ChunkPart.Guid ) )
			{
				TArray< FFileChunkPart >& FileChunkParts = ChunkPartsAvailable.FindOrAdd( ChunkPart.Guid );
				FFileChunkPart FileChunkPart;
				FileChunkPart.Filename = FileManifest->Filename;
				FileChunkPart.ChunkPart = ChunkPart;
				FileChunkPart.FileOffset = FileOffset;
				FileChunkParts.Add( FileChunkPart );
			}
			FileOffset += ChunkPart.Size;
		}
	}
}

void FBuildPatchAppManifest::GetRemovableFiles(FBuildPatchAppManifestRef OldManifest, FBuildPatchAppManifestRef NewManifest, TArray< FString >& RemovableFiles)
{
	NewManifest->GetRemovableFiles(OldManifest, RemovableFiles);
}

void FBuildPatchAppManifest::GetRemovableFiles(IBuildManifestRef InOldManifest, TArray< FString >& RemovableFiles) const
{
	// Cast manifest parameters
	FBuildPatchAppManifestRef OldManifest = StaticCastSharedRef< FBuildPatchAppManifest >(InOldManifest);
	// Simply put, any files that exist in the OldManifest file list, but do not in this manifest's file list, are assumed
	// to be files no longer required by the build
	for (auto OldFilesIt = OldManifest->FileManifestList.CreateConstIterator(); OldFilesIt; ++OldFilesIt)
	{
		const FString& OldFile = OldFilesIt.Key();
		if (!this->FileManifestList.Contains(OldFile))
		{
			RemovableFiles.Add(OldFile);
		}
	}
}

void FBuildPatchAppManifest::GetRemovableFiles(const TCHAR* InstallPath, TArray< FString >& RemovableFiles) const
{
	TArray<FString> AllFiles;
	IFileManager::Get().FindFilesRecursive(AllFiles, InstallPath, TEXT("*"), true, false);
	
	FString BasePath = InstallPath;

#if PLATFORM_MAC
	// On Mac paths in manifest start with app bundle name
	if (BasePath.EndsWith(".app"))
	{
		BasePath = FPaths::GetPath(BasePath) + "/";
	}
#endif
	
	for (int32 FileIndex = 0; FileIndex < AllFiles.Num(); ++FileIndex)
	{
		const FString& FileName = AllFiles[FileIndex].RightChop(BasePath.Len());
		if (!FileManifestList.Contains(*FileName))
		{
			RemovableFiles.Add(AllFiles[FileIndex]);
		}
	}
}

void FBuildPatchAppManifest::GetOutdatedFiles( FBuildPatchAppManifestPtr OldManifest, FBuildPatchAppManifestRef NewManifest, const FString& InstallDirectory, TArray< FString >& OutDatedFiles )
{
	if( !OldManifest.IsValid() )
	{
		// All files are outdated if no OldManifest
		NewManifest->FileManifestList.GetKeys( OutDatedFiles );
	}
	else
	{
		// Enumerate files in the NewManifest file list, that do not exist, or have different hashes in the OldManifest
		// to be files no longer required by the build
		for( auto NewFilesIt = NewManifest->FileManifestList.CreateConstIterator(); NewFilesIt; ++NewFilesIt )
		{
			const FString& NewFile = NewFilesIt.Key();
			const int64 ExistingFileSize = IFileManager::Get().FileSize( *(InstallDirectory / NewFile) );
			const TSharedPtr< FBuildPatchFileManifest > NewFileManifest = NewFilesIt.Value();
			const TSharedPtr< FBuildPatchFileManifest > OldFileManifest = OldManifest->FileManifestList.FindRef( NewFile );
			// Check changed
			if( IsFileOutdated( OldManifest.ToSharedRef(), NewManifest, NewFile ) )
			{
				OutDatedFiles.Add( NewFile );
			}
			// Double check an unchanged file is not missing (size will be -1) or is incorrect size
			else if( ExistingFileSize != NewFileManifest->GetFileSize() )
			{
				OutDatedFiles.Add( NewFile );
			}
		}
	}
}

bool FBuildPatchAppManifest::IsFileOutdated( FBuildPatchAppManifestRef OldManifest, FBuildPatchAppManifestRef NewManifest, const FString& Filename )
{
	// If both app manifests are the same, return false as only repair would touch the file.
	if( OldManifest == NewManifest )
	{
		return false;
	}
	// Get file manifests
	TSharedPtr< FBuildPatchFileManifest > OldFile = OldManifest->GetFileManifest( Filename );
	TSharedPtr< FBuildPatchFileManifest > NewFile = NewManifest->GetFileManifest( Filename );
	// Out of date if not in either manifest
	if( !OldFile.IsValid() || !NewFile.IsValid() )
	{
		return true;
	}
	// Different hash means different file
	if( OldFile->FileHash != NewFile->FileHash )
	{
		return true;
	}
	return false;
}

const uint32 FBuildPatchAppManifest::GetNumberOfChunkReferences( const FGuid& ChunkGuid ) const
{
	uint32 RefCount = 0;
	// For each file in the manifest, check if it has references to this chunk
	for( auto FileManifestIt = FileManifestList.CreateConstIterator(); FileManifestIt; ++FileManifestIt )
	{
		const TSharedPtr< FBuildPatchFileManifest >& FileManifest = FileManifestIt.Value();
		for( auto ChunkPartIt = FileManifest->FileChunkParts.CreateConstIterator(); ChunkPartIt; ++ChunkPartIt )
		{
			const FChunkPart& ChunkPart = *ChunkPartIt;
			if( ChunkPart.Guid == ChunkGuid )
			{
				++RefCount;
			}
		}
	}
	return RefCount;
}

#undef LOCTEXT_NAMESPACE
