// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnCoreNet.cpp: Core networking support.
=============================================================================*/

#include "CoreUObjectPrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoreNet, Log, All);

DEFINE_STAT(STAT_NetSerializeFast_Array);

/*-----------------------------------------------------------------------------
	FPackageInfo implementation.
-----------------------------------------------------------------------------*/

//
// FPackageInfo constructor.
//
FPackageInfo::FPackageInfo(UPackage* Package)
:	PackageName		(Package != NULL ? Package->GetFName() : NAME_None)
,	Parent			(Package)
,	Guid			(Package != NULL ? Package->GetGuid() : FGuid(0,0,0,0))
,	ObjectBase		( INDEX_NONE )
,	ObjectCount		( 0 )
,	LocalGeneration	( 0)
,	RemoteGeneration( 0 )
,	PackageFlags	(Package != NULL ? Package->PackageFlags : 0)
,	ForcedExportBasePackageName(NAME_None)
,	FileName		(Package != NULL ? Package->FileName : NAME_None)
{
	// if we have a pacakge, find it's source file so that we can send the extension of the file
	if (Package != NULL)
	{
		FString PackageFile;
		if (FPackageName::DoesPackageExist(Package->GetName(), NULL, &PackageFile))
		{
			Extension = FPaths::GetExtension(PackageFile);
		}
	}
}

//
// FPackageInfo serializer.
//
FArchive& operator<<( FArchive& Ar, FPackageInfo& I )
{
	return Ar << I.Parent;
}

/*-----------------------------------------------------------------------------
	FClassNetCache implementation.
-----------------------------------------------------------------------------*/

FClassNetCache::FClassNetCache()
{}
FClassNetCache::FClassNetCache( UClass* InClass )
: Class( InClass )
{}

/*-----------------------------------------------------------------------------
	UPackageMap implementation.
-----------------------------------------------------------------------------*/

bool UPackageMap::SerializeName(FArchive& Ar, FName& Name)
{
	if (Ar.IsLoading())
	{
		uint8 bHardcoded = 0;
		Ar.SerializeBits(&bHardcoded, 1);
		if (bHardcoded)
		{
			// replicated by hardcoded index
			uint32 NameIndex;
			Ar.SerializeInt(NameIndex, MAX_NETWORKED_HARDCODED_NAME + 1);
			Name = EName(NameIndex);
			// hardcoded names never have a Number
		}
		else
		{
			// replicated by string
			FString InString;
			int32 InNumber;
			Ar << InString << InNumber;
			Name = FName(*InString, InNumber);
		}
	}
	else if (Ar.IsSaving())
	{
		uint8 bHardcoded = Name.GetIndex() <= MAX_NETWORKED_HARDCODED_NAME;
		Ar.SerializeBits(&bHardcoded, 1);
		if (bHardcoded)
		{
			// send by hardcoded index
			checkSlow(Name.GetNumber() <= 0); // hardcoded names should never have a Number
			uint32 NameIndex = uint32(Name.GetIndex());
			Ar.SerializeInt(NameIndex, MAX_NETWORKED_HARDCODED_NAME + 1);
		}
		else
		{
			// send by string
			FString OutString = Name.GetPlainNameString();
			int32 OutNumber = Name.GetNumber();
			Ar << OutString << OutNumber;
		}
	}
	return true;
}

bool UPackageMap::SerializeObject( FArchive& Ar, UClass* Class, UObject*& Object, FNetworkGUID *OutNetGUID )
{
	// For now this is unsupported, but we could implement this serialization in a way that is compatible with UPackageMapClient,
	// it would just always serialize objects as <NetGUID, FullPath>.
	UE_LOG(LogCoreNet, Fatal,TEXT("Unexpected UPackageMap::SerializeObject"));
	return true;
}

bool UPackageMap::WriteObject( FArchive& Ar, UObject* Outer, FNetworkGUID NetGUID, FString ObjName )
{
	UE_LOG(LogCoreNet, Fatal,TEXT("Unexpected UPackageMap::WriteObject"));
	return true;
}

void UPackageMap::LogDebugInfo( FOutputDevice & Ar)
{
	UE_LOG(LogCoreNet, Fatal,TEXT("Unexpected UPackageMap::LogDebugInfo"));
}

UObject * UPackageMap::GetObjectFromNetGUID( const FNetworkGUID & NetGUID )
{
	UE_LOG(LogCoreNet, Fatal,TEXT("Unexpected UPackageMap::GetObjectFromNetGUID"));
	return NULL;
}

FClassNetCache* UPackageMap::GetClassNetCache( UClass* Class )
{
	FClassNetCache* Result = ClassFieldIndices.FindRef(Class);
	if( !Result )
	{
		Result                       = ClassFieldIndices.Add( Class, new FClassNetCache(Class) );
		Result->Super                = NULL;
		Result->FieldsBase           = 0;
		if( Class->GetSuperClass() )
		{
			Result->Super		         = GetClassNetCache(Class->GetSuperClass());
			Result->FieldsBase           = Result->Super->GetMaxIndex();
		}

		Result->Fields.Empty( Class->NetFields.Num() );
		for( int32 i=0; i<Class->NetFields.Num(); i++ )
		{
			// Add sandboxed items to net cache.  
			UField* Field = Class->NetFields[i];
			int32 ThisIndex	= Result->GetMaxIndex();
			new(Result->Fields)FFieldNetCache( Field, ThisIndex );
		}

		Result->Fields.Shrink();
		for( TArray<FFieldNetCache>::TIterator It(Result->Fields); It; ++It )
		{
			Result->FieldMap.Add( It->Field, &*It );
		}
	}
	return Result;
}

void UPackageMap::ClearClassNetCache()
{
	for( auto It = ClassFieldIndices.CreateIterator(); It; ++It)
	{
		delete It.Value();
	}
	ClassFieldIndices.Empty();
}

void UPackageMap::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
}

void UPackageMap::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UPackageMap * This = CastChecked<UPackageMap>( InThis );
	return Super::AddReferencedObjects( InThis, Collector );
}

void UPackageMap::FinishDestroy()
{
	ClearClassNetCache();
	Super::FinishDestroy();
}

void UPackageMap::ResetPackageMap()
{
	UE_LOG(LogCoreNet, Fatal,TEXT("Unexpected UPackageMap::ResetPackageMap"));
}

void UPackageMap::PostInitProperties()
{
	Super::PostInitProperties();
	bShouldSerializeUnAckedObjects = true;
	bSerializedUnAckedObject = false;
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UPackageMap, UObject,
	{
	}
);

// ----------------------------------------------------------------

void SerializeChecksum(FArchive &Ar, uint32 x, bool ErrorOK)
{
	if (Ar.IsLoading() )
	{
		uint32 Magic = 0;
		Ar << Magic;
		if((!ErrorOK || !Ar.IsError()) && !ensure(Magic==x))
		{
			UE_LOG(LogCoreNet, Warning, TEXT("%d == %d"), Magic, x );
		}
		
	}
	else
	{
		uint32 Magic = x;
		Ar << Magic;
	}
}

// ----------------------------------------------------------------
//	FNetBitWriter
// ----------------------------------------------------------------
FNetBitWriter::FNetBitWriter()
:	FBitWriter(0)
{}

FNetBitWriter::FNetBitWriter( int64 InMaxBits )
:	FBitWriter (InMaxBits, true)
,	PackageMap( NULL )
{

}

FNetBitWriter::FNetBitWriter( UPackageMap * InPackageMap, int64 InMaxBits )
:	FBitWriter (InMaxBits, true)
,	PackageMap( InPackageMap )
{

}

FArchive& FNetBitWriter::operator<<( class FName& N )
{
	PackageMap->SerializeName( *this, N );
	return *this;
}

FArchive& FNetBitWriter::operator<<( UObject*& Object )
{
	PackageMap->SerializeObject( *this, UObject::StaticClass(), Object );
	return *this;
}


// ----------------------------------------------------------------
//	FNetBitReader
// ----------------------------------------------------------------
FNetBitReader::FNetBitReader( UPackageMap *InPackageMap, uint8* Src, int64 CountBits )
:	FBitReader	(Src, CountBits)
,   PackageMap  ( InPackageMap )
{
}

FArchive& FNetBitReader::operator<<( UObject*& Object )
{
	PackageMap->SerializeObject( *this, UObject::StaticClass(), Object );
	return *this;
}

FArchive& FNetBitReader::operator<<( class FName& N )
{
	PackageMap->SerializeName( *this, N );
	return *this;
}

static const TCHAR * GLastRPCFailedReason = NULL;

void RPC_ResetLastFailedReason()
{
	GLastRPCFailedReason = NULL;
}
void RPC_ValidateFailed( const TCHAR * Reason )
{
	GLastRPCFailedReason = Reason;
}

const TCHAR * RPC_GetLastFailedReason()
{
	return GLastRPCFailedReason;
}
