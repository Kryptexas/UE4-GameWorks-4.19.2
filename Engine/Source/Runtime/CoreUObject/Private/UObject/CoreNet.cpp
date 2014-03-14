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
bool UPackageMap::CanSerializeObject(const UObject* Obj)
{
	return SupportsObject(Obj);
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

FClassNetCache* UPackageMap::GetClassNetCache( UClass* Class )
{
	FClassNetCache* Result = ClassFieldIndices.FindRef(Class);
	if( !Result && SupportsObject(Class) )
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
			if( SupportsObject(Field ) )
			{
				int32 ThisIndex      = Result->GetMaxIndex();
 				new(Result->Fields)FFieldNetCache( Field, ThisIndex );
			}
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

void UPackageMap::LogDebugInfo(FOutputDevice& Ar)
{
	for (auto It = Cache->ObjectLookup.CreateIterator(); It; ++It)
	{
		FNetworkGUID NetGUID = It.Key();
		UObject *Obj = It.Value().Object.Get();
		UE_LOG(LogCoreNet, Log, TEXT("%s - %s"), *NetGUID.ToString(), Obj ? *Obj->GetPathName() : TEXT("NULL"));
	}
}

bool UPackageMap::SupportsObject( const UObject* Object )
{
	/** In this PackageMap implementation, all Objects are supported.*/
	return true;
}
bool UPackageMap::SupportsPackage( UObject* InOuter )
{
	/** In this PackageMap implementation, all Packages are supported.*/
	return true;
}

void UPackageMap::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << Cache;
}

void UPackageMap::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UPackageMap * This = CastChecked<UPackageMap>( InThis );

	if ( This->Cache != NULL )
	{
		Collector.AddReferencedObject( This->Cache, This );		
	}

	return Super::AddReferencedObjects( InThis, Collector );
}

FNetworkGUID UPackageMap::GetObjectNetGUID(const UObject *Object)
{
	FNetworkGUID NetGUID;
	if (!Object || !SupportsObject(Object))
	{
		return NetGUID;
	}
	
	NetGUID = Cache->NetGUIDLookup.FindRef(Object);
	if (!NetGUID.IsValid())
	{
		NetGUID = AssignNewNetGUID(Object);
	}
	return NetGUID;
}

UObject * UPackageMap::GetObjectFromNetGUID( FNetworkGUID NetGUID )
{
	if ( !ensure( NetGUID.IsValid() ) )
	{
		return NULL;
	}

	if ( !ensure( !NetGUID.IsDefault() ) )
	{
		return NULL;
	}

	FNetGuidCacheObject * CacheObjectPtr = Cache->ObjectLookup.Find( NetGUID );

	if ( CacheObjectPtr == NULL )
	{
		// We don't have the object mapped yet
		return NULL;
	}

	UObject * Object = CacheObjectPtr->Object.Get();

	if ( Object != NULL )
	{
		check( Object->GetPathName() == CacheObjectPtr->FullPath );
		return Object;
	}

	if ( CacheObjectPtr->FullPath.IsEmpty() )
	{
		// This probably isn't possible, but making it a warning
		UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: No full path for %s" ), *NetGUID.ToString() );
		return NULL;
	}

	if ( NetGUID.IsDynamic() )
	{
		// Dynamic objects don't have stable names, so we can't possibly reload the same object
		UE_LOG( LogNetPackageMap, VeryVerbose, TEXT( "GetObjectFromNetGUID: Cannot re-create dynamic object after GC <%s, %s>" ), *NetGUID.ToString(), *CacheObjectPtr->FullPath );
		return NULL;
	}

	//
	// The object was previously mapped, but we GC'd it
	// We need to reload it now
	//

	Object = StaticFindObject( UObject::StaticClass(), NULL, *CacheObjectPtr->FullPath, false );

	if ( Object == NULL )
	{
		if ( IsNetGUIDAuthority() )
		{
			// The authority shouldn't be loading resources on demand, at least for now.
			// This could be garbage or a security risk
			// Another possibility is in dynamic property replication if the server reads the previously serialized state
			// that has a now destroyed actor in it.
			UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Could not find Object for: NetGUID <%s, %s> (and IsNetGUIDAuthority())" ), *NetGUID.ToString(), *CacheObjectPtr->FullPath );
			return NULL;
		}
		else
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "GetObjectFromNetGUID: Could not find Object for: NetGUID <%s, %s>"), *NetGUID.ToString(), *CacheObjectPtr->FullPath );
		}

		Object = StaticLoadObject( UObject::StaticClass(), NULL, *CacheObjectPtr->FullPath, NULL, LOAD_NoWarn );

		if ( Object )
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "GetObjectFromNetGUID: StaticLoadObject. Found: %s" ), Object ? *Object->GetName() : TEXT( "NULL" ) );
		}
		else
		{
			Object = LoadPackage( NULL, *CacheObjectPtr->FullPath, LOAD_None );
			UE_LOG( LogNetPackageMap, Log, TEXT( "GetObjectFromNetGUID: LoadPackage. Found: %s" ), Object ? *Object->GetName() : TEXT( "NULL" ) );
		}
	}
	else
	{
		// If we actually found the object, we probably shouldn't have GC'd it, so that's odd
		UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Object should not be found after GC: <%s, %s>" ), *NetGUID.ToString(), *CacheObjectPtr->FullPath );
	}

	if ( Object == NULL )
	{
		// If we get here, that means we have an invalid path, which shouldn't be possible, but making it a warning
		UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: FAILED to re-create object after GC: <%s, %s>" ), *NetGUID.ToString(), *CacheObjectPtr->FullPath );
	}
	else
	{
		// Reassign the object pointer for quick access next time
		CacheObjectPtr->Object = Object;		
		// Make sure the object is in the GUIDToObjectLookup.
		Cache->NetGUIDLookup.Add( Object, NetGUID );
	}

	return Object;
}

/**
 *	Generate a new NetGUID for this object and assign it.
 */
FNetworkGUID UPackageMap::AssignNewNetGUID(const UObject* Object)
{
	if (!IsNetGUIDAuthority())
	{
		// We cannot make new NetGUIDs
		return FNetworkGUID::GetDefault();
	}

	// Generate new NetGUID and assign it
	int32 idx = 0;
	if (!IsDynamicObject(Object))
	{
		idx = 1;
	}

	return AssignNetGUID(Object, (++Cache->UniqueNetIDs[idx] << 1) | idx);
}

/**
 *	Assigns the given UObject the give NetNetworkGUID.
 */
FNetworkGUID UPackageMap::AssignNetGUID(const UObject* Object, FNetworkGUID NewNetworkGUID)
{
	if (NewNetworkGUID.IsDefault())
	{
		// This is the default NetGUID, look up or generate a real one
		NewNetworkGUID = Cache->NetGUIDLookup.FindRef(Object);
		if (!NewNetworkGUID.IsValid())
		{
			NewNetworkGUID = AssignNewNetGUID(Object);
		}
		HandleUnAssignedObject(Object);
		return NewNetworkGUID;
	}

	UE_LOG(LogNetPackageMap, Log, TEXT("Assigning %s NetGUID <%s>"), Object ? *Object->GetName() : TEXT("NULL"), *NewNetworkGUID.ToString() );
	
	// Check for reassignment: this can happen during seamless travel, so leaving this off for now, but is useful for debugging purposes, so leaving it in.
#if 0
	{
		FNetworkGUID OldNetGUID = Cache->NetGUIDLookup.FindRef(Object);
		if (OldNetGUID.IsValid() && !OldNetGUID.IsDefault())
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Attempting to reassign NetGUID to %s (was previously <%s>. Now is <%s>"), Object ? *Object->GetName() : TEXT("NULL"), *OldNetGUID.ToString(),  *NewNetworkGUID.ToString() );
		}

		UObject *OldObject = Cache->ObjectLookup.FindRef(NewNetworkGUID).Get();
		if (OldObject)
		{
			UE_LOG(LogNetPackageMap, Warning, TEXT("Attempting to reassign NetGUID <%s> to %s (It is already assigned to object %s"), *NewNetworkGUID.ToString(), Object ? *Object->GetName() : TEXT("NULL"), *OldNetGUID.ToString(), *OldObject->GetName() );
		}
	}
#endif

	// Actually assign
	FNetGuidCacheObject CacheObject;

	CacheObject.Object = Object;
	CacheObject.FullPath = Object->GetPathName();

	// Remove any existing entries
	{
		FNetGuidCacheObject* ExistingCacheObjectPtr = Cache->ObjectLookup.Find( NewNetworkGUID );
		if ( ExistingCacheObjectPtr )
		{
			const FNetGuidCacheObject& ExistingCacheObject = *ExistingCacheObjectPtr;
			Cache->NetGUIDLookup.Remove( ExistingCacheObject.Object );
		}

		FNetworkGUID* ExistingNetworkGUIDPtr = Cache->NetGUIDLookup.Find( Object );
		if ( ExistingNetworkGUIDPtr )
		{
			const FNetworkGUID& ExistingNetworkGUID = *ExistingNetworkGUIDPtr;
			Cache->ObjectLookup.Remove( ExistingNetworkGUID );
		}
	}

	Cache->ObjectLookup.Add( NewNetworkGUID, CacheObject );
	Cache->NetGUIDLookup.Add(Object, NewNetworkGUID);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	Cache->History.Add(NewNetworkGUID, Object ? Object->GetPathName() : TEXT("NULL"));
#endif

	return NewNetworkGUID;
}

bool UPackageMap::IsDynamicObject(const UObject* Object)
{
	check(ObjectIsDynamicDelegate.IsBound());
	return ObjectIsDynamicDelegate.Execute(Object);
}

void UPackageMap::FinishDestroy()
{
	ClearClassNetCache();
	Super::FinishDestroy();
}

void UPackageMap::ResetPackageMap()
{
	Cache->ObjectLookup.Empty();
	Cache->NetGUIDLookup.Empty();
	Cache->UniqueNetIDs[0] = Cache->UniqueNetIDs[1] = 0;
}

void UPackageMap::CleanPackageMap()
{
	for ( auto It = Cache->ObjectLookup.CreateIterator(); It; ++It )
	{
		if ( !It.Value().Object.IsValid() )
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "Cleaning reference to NetGUID: <%s> (ObjectLookup)" ), *It.Key().ToString() );
			It.RemoveCurrent();
			continue;
		}

		// Make sure the path matches (which can change during seamless travel)
		It.Value().FullPath = It.Value().Object->GetPathName();
	}

	for ( auto It = Cache->NetGUIDLookup.CreateIterator(); It; ++It )
	{
		if ( !It.Key().IsValid() )
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "Cleaning reference to NetGUID: <%s> (NetGUIDLookup)" ), *It.Value().ToString() );
			It.RemoveCurrent();
		}
	}

	// Sanity check
	for ( auto It = Cache->ObjectLookup.CreateIterator(); It; ++It )
	{
		check( It.Value().Object.IsValid() );
		checkf( Cache->NetGUIDLookup.FindRef( It.Value().Object ) == It.Key(), TEXT("Failed to validate ObjectLookup map in UPackageMap. Object '%s' was not in the NetGUIDLookup map with with value '%s'."), *It.Value().Object.Get()->GetPathName(), *It.Key().ToString());
	}

	for ( auto It = Cache->NetGUIDLookup.CreateIterator(); It; ++It )
	{
		check( It.Key().IsValid() );
		checkf( Cache->ObjectLookup.FindRef( It.Value() ).Object == It.Key(), TEXT("Failed to validate NetGUIDLookup map in UPackageMap. GUID '%s' was not in the ObjectLookup map with with object '%s'."), *It.Value().ToString(), *It.Key().Get()->GetPathName());
	}
}

void UPackageMap::PostInitProperties()
{
	Super::PostInitProperties();
	bShouldSerializeUnAckedObjects = true;
	bSerializedUnAckedObject = false;

	// Create the NetGUIDCache now if we don't have one. The pattern should be that the MasterMap (a plain UPackageMap) will always
	// create one here, but UPackageMapClient and other subclasses will take thier Cache in as a parameter so that is it shared 
	// (e.g., one 1 UNetGUIDCache exists on the server and is referenced be each client connection's package map).
	// 
	// This also keeps subclasses from <i>requiring</i> a master map to pass in their UNetGUIDCache; if we don't pass one in on construction
	// they can use their own and still work. This may be useful for 'sideways' connections or other backend communication
	if ( !Cache && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		Cache = new UNetGUIDCache (FPostConstructInitializeProperties());
	}
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UPackageMap, UObject,
	{
	}
);

// ----------------------------------------------------------------

void UNetGUIDCache::PostInitProperties()
{
	Super::PostInitProperties();
	UniqueNetIDs[0] = UniqueNetIDs[1] = 0;
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UNetGUIDCache, UObject,
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
