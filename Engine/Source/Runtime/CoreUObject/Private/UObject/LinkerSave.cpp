// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

/*----------------------------------------------------------------------------
	ULinkerSave.
----------------------------------------------------------------------------*/

/** A mapping of package name to generated script SHA keys */
TMap<FString, TArray<uint8> > ULinkerSave::PackagesToScriptSHAMap;

ULinkerSave::ULinkerSave( const class FPostConstructInitializeProperties& PCIP, UPackage* InParent, const TCHAR* InFilename, bool bForceByteSwapping, bool bInSaveUnversioned )
:	ULinker(PCIP, InParent, InFilename )
,	Saver( NULL )
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	if (FPlatformProperties::HasEditorOnlyData())
	{
		// Create file saver.
		Saver = IFileManager::Get().CreateFileWriter( InFilename, 0 );
		if( !Saver )
		{
			UE_LOG(LogLinker, Fatal, TEXT("%s"), *FString::Printf( TEXT("Error opening file '%s'."), InFilename ) );
		}

		// Set main summary info.
		Summary.Tag           = PACKAGE_FILE_TAG;
		Summary.SetFileVersions( (int32)VER_LAST_ENGINE_UE3, GPackageFileUE4Version, GPackageFileLicenseeUE4Version, bInSaveUnversioned );
		Summary.EngineVersion =	GEngineVersion;
		Summary.PackageFlags  = Cast<UPackage>(LinkerRoot) ? Cast<UPackage>(LinkerRoot)->PackageFlags : 0;

		UPackage *Package = Cast<UPackage>(LinkerRoot);
		if (Package)
		{
			Summary.FolderName = Package->GetFolderName().ToString();
			Summary.ChunkIDs = Package->GetChunkIDs();
		}

		// Set status info.
		ArIsSaving				= 1;
		ArIsPersistent			= 1;
		ArForceByteSwapping		= bForceByteSwapping;
	
		// Allocate indices.
		NameIndices  .AddZeroed( FName::GetMaxNames() );
	}
}

ULinkerSave::ULinkerSave(const class FPostConstructInitializeProperties& PCIP, UPackage* InParent, bool bForceByteSwapping, bool bInSaveUnversioned )
:	ULinker( PCIP, InParent,TEXT("$$Memory$$") )
,	Saver( NULL )
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	if (FPlatformProperties::HasEditorOnlyData())
	{
		// Create file saver.
		Saver = new FBufferArchive();
		check(Saver);

		// Set main summary info.
		Summary.Tag           = PACKAGE_FILE_TAG;
		Summary.SetFileVersions( (int32)VER_LAST_ENGINE_UE3, GPackageFileUE4Version, GPackageFileLicenseeUE4Version, bInSaveUnversioned );
		Summary.EngineVersion =	GEngineVersion;
		Summary.PackageFlags  = Cast<UPackage>(LinkerRoot) ? Cast<UPackage>(LinkerRoot)->PackageFlags : 0;

		UPackage *Package = Cast<UPackage>(LinkerRoot);
		if (Package)
		{
			Summary.FolderName = Package->GetFolderName().ToString();
			Summary.ChunkIDs = Package->GetChunkIDs();
		}

		// Set status info.
		ArIsSaving				= 1;
		ArIsPersistent			= 1;
		ArForceByteSwapping		= bForceByteSwapping;

		NameIndices  .AddZeroed( FName::GetMaxNames() );
	}
}
/**
 * Detaches file saver and hence file handle.
 */
void ULinkerSave::Detach()
{
	if( Saver )
	{
		delete Saver;
	}
	Saver = NULL;
}

void ULinkerSave::BeginDestroy()
{
	// Detach file saver/ handle.
	Detach();
	Super::BeginDestroy();
}

// FArchive interface.
int32 ULinkerSave::MapName( const FName* Name ) const
{
	return NameIndices[Name->GetIndex()];
}

FPackageIndex ULinkerSave::MapObject( const UObject* Object ) const
{
	if (Object)
	{
		const FPackageIndex *Found = ObjectIndicesMap.Find(Object);
		if (Found)
		{
			return *Found;
		}
	}
	return FPackageIndex();
}

void ULinkerSave::Seek( int64 InPos )
{
	Saver->Seek( InPos );
}

int64 ULinkerSave::Tell()
{
	return Saver->Tell();
}

void ULinkerSave::Serialize( void* V, int64 Length )
{
	Saver->Serialize( V, Length );
}
	
FArchive& ULinkerSave::operator<<( UObject*& Obj )
{
	FPackageIndex Save;
	if (Obj)
	{
		Save = MapObject(Obj);
	}
	return *this << Save;
}

FArchive& ULinkerSave::operator<<( FLazyObjectPtr& LazyObjectPtr)
{
	FUniqueObjectGuid ID;
	ID = LazyObjectPtr.GetUniqueID();
	return *this << ID;
}

FArchive& ULinkerSave::operator<<( FAssetPtr& AssetPtr)
{
	FStringAssetReference ID;
	UObject *Object = AssetPtr.Get();

	if (Object)
	{
		// Use object in case name has changed. 
		ID = FStringAssetReference(Object);
	}
	else
	{
		ID = AssetPtr.GetUniqueID();
	}
	return *this << ID;
}

IMPLEMENT_CORE_INTRINSIC_CLASS(ULinkerSave, ULinker,
	{
	}
);
