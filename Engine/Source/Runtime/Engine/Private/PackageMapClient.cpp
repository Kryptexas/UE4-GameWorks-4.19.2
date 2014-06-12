// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "Net/NetworkProfiler.h"
#include "Net/DataChannel.h"
#include "Net/DataReplication.h"
#include "Engine/ActorChannel.h"

// ( OutPacketId == GUID_PACKET_NOT_ACKED ) == NAK'd		(this GUID is not acked, and is not pending either, so sort of waiting)
// ( OutPacketId == GUID_PACKET_ACKED )		== FULLY ACK'd	(this GUID is fully acked, and we no longer need to send full path)
// ( OutPacketId > GUID_PACKET_ACKED )		== PENDING		(this GUID needs to be acked, it has been recently reference, and path was sent)

static const int GUID_PACKET_NOT_ACKED	= -2;		
static const int GUID_PACKET_ACKED		= -1;		

/**
 * Don't allow infinite recursion of InternalLoadObject - an attacker could
 * send malicious packets that cause a stack overflow on the server.
 */
static const int INTERNAL_LOAD_OBJECT_RECURSION_LIMIT = 16;

#define GET_NET_GUID_INDEX( GUID )					( GUID.Value >> 1 )
#define GET_NET_GUID_STATIC_MOD( GUID )				( GUID.Value & 1 )
#define COMPOSE_NET_GUID( Index, IsStatic )			( ( ( Index ) << 1 ) | ( IsStatic ) )
#define COMPOSE_RELATIVE_NET_GUID( GUID, Index )	( COMPOSE_NET_GUID( GET_NET_GUID_INDEX( GUID ) + Index, GET_NET_GUID_STATIC_MOD( GUID ) ) )

#define STRESS_PENDING_GUIDS ( 0 )

/*-----------------------------------------------------------------------------
	UPackageMapClient implementation.
-----------------------------------------------------------------------------*/
UPackageMapClient::UPackageMapClient(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/**
 *	This is the meat of the PackageMap class which serializes a reference to Object.
 */
bool UPackageMapClient::SerializeObject( FArchive& Ar, UClass* Class, UObject*& Object, FNetworkGUID *OutNetGUID)
{
	SCOPE_CYCLE_COUNTER(STAT_PackageMap_SerializeObjectTime);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static IConsoleVariable* DebugObjectCvar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.PackageMap.DebugObject"));
	if (DebugObjectCvar && !DebugObjectCvar->GetString().IsEmpty() && Object && Object->GetName().Contains(DebugObjectCvar->GetString()))
	{
		UE_LOG(LogNetPackageMap, Log, TEXT("Serialized Object %s"), *Object->GetName());
	}
#endif

	if (Ar.IsSaving())
	{
		// If pending kill, just serialize as NULL.
		// TWeakObjectPtrs of PendingKill objects will behave strangely with TSets and TMaps
		//	PendingKill objects will collide with each other and with NULL objects in those data structures.
		if (Object && Object->IsPendingKill())
		{
			UObject * NullObj = NULL;
			return SerializeObject( Ar, Class, NullObj, OutNetGUID);
		}

		FNetworkGUID NetGUID = GuidCache->GetOrAssignNetGUID( Object );

		// Write out NetGUID to caller if necessary
		if (OutNetGUID)
		{
			*OutNetGUID = NetGUID;
		}

		// Write object NetGUID to the given FArchive
		InternalWriteObject( Ar, NetGUID, Object, TEXT( "" ), NULL );

		// If we need to export this GUID (its new or hasnt been ACKd, do so here)
		if (!NetGUID.IsDefault() && ShouldSendFullPath(Object, NetGUID))
		{
			check(IsNetGUIDAuthority());
			if ( !ExportNetGUID( NetGUID, Object, TEXT(""), NULL ) )
			{
				UE_LOG( LogNetPackageMap, Verbose, TEXT( "Failed to export in ::SerializeObject %s"), *Object->GetName() );
			}
		}

		return true;
	}
	else if (Ar.IsLoading())
	{
		// ----------------	
		// Read NetGUID from stream and resolve object
		// ----------------	
		FNetworkGUID NetGUID = InternalLoadObject( Ar, Object, false, 0 );

		// Write out NetGUID to caller if necessary
		if ( OutNetGUID )
		{
			*OutNetGUID = NetGUID;
		}	

		// ----------------	
		// Final Checks/verification
		// ----------------	

		// NULL if we haven't finished loading the objects level yet
		if ( !ObjectLevelHasFinishedLoading( Object ) )
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Using None instead of replicated reference to %s because the level it's in has not been made visible"), *Object->GetFullName());
			Object = NULL;
		}

		// Check that we got the right class
		if ( Object && !Object->IsA( Class ) )
		{
			UE_LOG(LogNetPackageMap, Warning, TEXT("Forged object: got %s, expecting %s"),*Object->GetFullName(),*Class->GetFullName());
			Object = NULL;
		}

		if ( NetGUID.IsValid() && Object == NULL )
		{
			bLoadedUnmappedObject = true;
			LastUnmappedNetGUID = NetGUID;
		}

		UE_CLOG(!bSuppressLogs, LogNetPackageMap, Log, TEXT("UPackageMapClient::SerializeObject Serialized Object %s as <%s>"), Object ? *Object->GetPathName() : TEXT("NULL"), *NetGUID.ToString() );

		// reference is mapped if it was not NULL (or was explicitly null)
		return (Object != NULL || !NetGUID.IsValid());
	}

	return true;
}

/**
 *	Slimmed down version of SerializeObject, that writes an object reference given a NetGUID and name
 *	(e.g, it does not require the actor to actually exist anymore to serialize the reference).
 *	This must be kept in sync with UPackageMapClient::SerializeObject.
 */
bool UPackageMapClient::WriteObject( FArchive& Ar, UObject* ObjOuter, FNetworkGUID NetGUID, FString ObjName )
{
	Ar << NetGUID;
	NET_CHECKSUM(Ar);

	UE_LOG(LogNetPackageMap, Log, TEXT("WroteObject %s NetGUID <%s>"), *ObjName, *NetGUID.ToString() );

	if (NetGUID.IsStatic() && !NetGUID.IsDefault() && !NetGUIDHasBeenAckd(NetGUID))
	{
		if ( !ExportNetGUID( NetGUID, NULL, ObjName, ObjOuter ) )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "Failed to export in ::WriteObject %s" ), *ObjName );
		}
	}

	return true;
}

/**
 *	Standard method of serializing a new actor.
 *		For static actors, this will just be a single call to SerializeObject, since they can be referenced by their path name.
 *		For dynamic actors, first the actor's reference is serialized but will not resolve on clients since they haven't spawned the actor yet.
 *			The actor archetype is then serialized along with the starting location, rotation, and velocity.
 *			After reading this information, the client spawns this actor via GWorld and assigns it the NetGUID it read at the top of the function.
 *
 *		returns true if a new actor was spawned. false means an existing actor was found for the netguid.
 */
bool UPackageMapClient::SerializeNewActor(FArchive& Ar, class UActorChannel *Channel, class AActor*& Actor)
{
	UE_LOG( LogNetPackageMap, VeryVerbose, TEXT( "SerializeNewActor START" ) );

	// This will mark IsSerializingNewActor true for the scope of the function
	FMarkSerializeNewActorScoped SerializeNewActorScoped( IsSerializingNewActor );

	uint8 bIsClosingChannel = 0;

	if (Ar.IsLoading() )
	{
		FInBunch* InBunch = (FInBunch*)&Ar;
		bIsClosingChannel = InBunch->bClose;		// This is so we can determine that this channel was opened/closed for destruction
		UE_LOG(LogNetPackageMap, Log, TEXT("UPackageMapClient::SerializeNewActor BitPos: %d"), InBunch->GetPosBits() );
	}

	NET_CHECKSUM(Ar);

	FNetworkGUID NetGUID;
	UObject *NewObj = Actor;
	SerializeObject(Ar, AActor::StaticClass(), NewObj, &NetGUID);

	if ( Ar.IsError() )
	{
		UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::SerializeNewActor: Ar.IsError after SerializeObject 1" ) );
		return false;
	}

	Actor = Cast<AActor>(NewObj);

	bool SpawnedActor = false;

	if ( Ar.AtEnd() && NetGUID.IsDynamic() )
	{
		// This must be a destruction info coming through or something is wrong
		// If so, we should be both closing the channel and we should find the actor
		// This can happen when dormant actors that don't have channels get destroyed
		if ( bIsClosingChannel == 0 || Actor == NULL )
		{
			UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::SerializeNewActor: bIsClosingChannel == 0 || Actor == NULL : %s" ), Actor ? *Actor->GetName() : TEXT( "NULL" ) );
			Ar.SetError();
			return false;
		}

		UE_LOG( LogNetPackageMap, Log, TEXT( "UPackageMapClient::SerializeNewActor:  Skipping full read because we are deleting dynamic actor: %s" ), Actor ? *Actor->GetName() : TEXT( "NULL" ) );
		return true;
	}

	if (NetGUID.IsDynamic())
	{
		UObject* Archetype = NULL;
		FVector_NetQuantize10 Location;
		FVector_NetQuantize10 Velocity;
		FRotator Rotation;
		bool SerSuccess;

		if (Ar.IsSaving())
		{
			Archetype = Actor->GetArchetype();

			check( Archetype != NULL );
			check( Actor->NeedsLoadForClient() );			// We have no business sending this unless the client can load
			check( Archetype->NeedsLoadForClient() );		// We have no business sending this unless the client can load

			Location = Actor->GetRootComponent() ? Actor->GetActorLocation() : FVector::ZeroVector;
			Rotation = Actor->GetRootComponent() ? Actor->GetActorRotation() : FRotator::ZeroRotator;
			Velocity = Actor->GetVelocity();
		}

		FNetworkGUID ArchetypeNetGUID;
		SerializeObject( Ar, UObject::StaticClass(), Archetype, &ArchetypeNetGUID );

		if ( ArchetypeNetGUID.IsValid() && Archetype == NULL )
		{
			const FNetGuidCacheObject * ExistingCacheObjectPtr = GuidCache->ObjectLookup.Find( ArchetypeNetGUID );

			if ( ExistingCacheObjectPtr != NULL )
			{
				UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::SerializeNewActor. Unresolved Archetype GUID. Path: %s, NetGUID: %s." ), *ExistingCacheObjectPtr->PathName.ToString(), *ArchetypeNetGUID.ToString() );
			}
			else
			{
				UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::SerializeNewActor. Unresolved Archetype GUID. Guid not registered! NetGUID: %s." ), *ArchetypeNetGUID.ToString() );
			}
		}

		// SerializeCompressedInitial
		{			
			// read/write compressed location
			Location.NetSerialize( Ar, this, SerSuccess );
			Rotation.NetSerialize( Ar, this, SerSuccess );
			Velocity.NetSerialize( Ar, this, SerSuccess );

			if ( Channel && Ar.IsSaving() )
			{
				FObjectReplicator * RepData = &Channel->GetActorReplicationData();
				uint8 * Recent = RepData && RepData->RepState != NULL && RepData->RepState->StaticBuffer.Num() ? RepData->RepState->StaticBuffer.GetTypedData() : NULL;
				if ( Recent )
				{
					((AActor*)Recent)->ReplicatedMovement.Location = Location;
					((AActor*)Recent)->ReplicatedMovement.Rotation = Rotation;
					((AActor*)Recent)->ReplicatedMovement.LinearVelocity = Velocity;
				}
			}
		}

		if ( Ar.IsLoading() )
		{
			// Spawn actor if necessary (we may have already found it if it was dormant)
			if ( Actor == NULL )
			{
				if ( Archetype )
				{
					FActorSpawnParameters SpawnInfo;
					SpawnInfo.Template = Cast<AActor>(Archetype);
					SpawnInfo.bNoCollisionFail = true;
					SpawnInfo.bRemoteOwned = true;
					SpawnInfo.bNoFail = true;
					Actor = Connection->Driver->GetWorld()->SpawnActor(Archetype->GetClass(), &Location, &Rotation, SpawnInfo );
					Actor->PostNetReceiveVelocity(Velocity);

					GuidCache->RegisterNetGUID_Client( NetGUID, Actor );
					SpawnedActor = true;
				}
				else
				{
					UE_LOG(LogNetPackageMap, Warning, TEXT("UPackageMapClient::SerializeNewActor Unable to read Archetype for NetGUID %s / %s"), *NetGUID.ToString(), *ArchetypeNetGUID.ToString() );
					ensure(Archetype);
				}
			}
		}

		//
		// Serialize Initially replicating sub-objects here
		//

		//
		// We expect that all sub-objects that have stable network names have already been assigned guids, which are relative to their actor owner
		// Because of this, we can expect the client to deterministically generate the same guids, and we just need to verify with checksum
		//

		TArray< UObject * > Subobjs;

		Actor->GetSubobjectsWithStableNamesForNetworking( Subobjs );

		if ( Ar.IsSaving() )
		{
			// We're not really saving anything necessary here except a checksum to make sure we're in sync
			// It might be a worth thinking about turning this off in shipping, and trust this is working as intended to save bandwidth if needed
			uint32 Checksum = 0;

			for ( int32 i = 0; i < Subobjs.Num(); i++ )
			{
				UObject * SubObj = Subobjs[i];

				FNetworkGUID SubobjNetGUID = GuidCache->NetGUIDLookup.FindRef( SubObj );

				// Make sure these sub objects have net guids that are relative to this owning actor
				check( SubobjNetGUID.Value == COMPOSE_RELATIVE_NET_GUID( NetGUID, i + 1 ) );

				// Evolve checksum so we can sanity check
				Checksum = FCrc::MemCrc32( &SubobjNetGUID.Value, sizeof( SubobjNetGUID.Value ), Checksum );
				Checksum = FCrc::StrCrc32( *SubObj->GetName(), Checksum );

				check( !PendingAckGUIDs.Contains( SubobjNetGUID ) );
			}

			Ar << Checksum;
		}
		else
		{
			// Load the server checksum, then generate our own, and make sure they match
			uint32 ServerChecksum = 0;
			uint32 LocalChecksum = 0;
			Ar << ServerChecksum;

			TArray< FNetworkGUID > SubObjGuids;

			for ( int32 i = 0; i < Subobjs.Num(); i++ )
			{
				// Generate a guid that is relative to our owning actor
				SubObjGuids.Add( COMPOSE_RELATIVE_NET_GUID( NetGUID, i + 1 ) );

				// Evolve checksum so we can sanity check
				LocalChecksum = FCrc::MemCrc32( &SubObjGuids[i].Value, sizeof( SubObjGuids[i].Value ), LocalChecksum );
				LocalChecksum = FCrc::StrCrc32( *Subobjs[i]->GetName(), LocalChecksum );

				UE_LOG( LogNetPackageMap, Log, TEXT( "SerializeNewActor: [Loading] Assigned NetGUID <%s> to subobject %s" ), *SubObjGuids[i].ToString(), *Subobjs[i]->GetPathName() );
			}

			if ( LocalChecksum != ServerChecksum )
			{
				UE_LOG( LogNetPackageMap, Error, TEXT( "SerializeNewActor: Subobject checksum FAILED: %s" ), *Actor->GetFullName() );
			}
			else
			{
				// Everything worked, assign the guids
				for ( int32 i = 0; i < Subobjs.Num(); i++ )
				{
					FNetworkGUID OldNetGUID = GuidCache->NetGUIDLookup.FindRef( Subobjs[i] );

					if ( OldNetGUID.IsValid() )
					{
						if ( OldNetGUID != SubObjGuids[i] )
						{
							UE_LOG( LogNetPackageMap, Warning, TEXT( "SerializeNewActor: Changing NetGUID on subobject. Name: %s, OldGUID: %s, NewGUID: %s" ), *Subobjs[i]->GetPathName(), *OldNetGUID.ToString(), *SubObjGuids[i].ToString() );
						}
					}

					GuidCache->RegisterNetGUID_Client( SubObjGuids[i], Subobjs[i] );

					UE_LOG( LogNetPackageMap, Log, TEXT( "SerializeNewActor: [Loading] Assigned NetGUID <%s> to subobject %s" ), *SubObjGuids[i].ToString(), *Subobjs[i]->GetPathName() );
				}
			}
		}
	}

	UE_LOG(LogNetPackageMap, Log, TEXT("SerializeNewActor END: Finished Serializing Actor %s <%s> on Channel %d"), Actor ? *Actor->GetName() : TEXT("NULL"), *NetGUID.ToString(), Channel->ChIndex );

	return SpawnedActor;
}

//--------------------------------------------------------------------
//
//	Writing
//
//--------------------------------------------------------------------

struct FExportFlags
{
	union
	{
		struct
		{
			uint8 bHasPath			: 1;
			uint8 bIsLevel			: 1;
			uint8 bIsPackage		: 1;
		};

		uint8	Value;
	};

	FExportFlags()
	{
		Value = 0;
	}
};

/** Writes an object NetGUID given the NetGUID and either the object itself, or FString full name of the object. Appends full name/path if necessary */
void UPackageMapClient::InternalWriteObject( FArchive & Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject * ObjectOuter )
{
	Ar << NetGUID;
	NET_CHECKSUM( Ar );

	if ( !NetGUID.IsValid() )
	{
		// We're done writing
		return;
	}

	// Write export flags
	//   note: Default NetGUID is implied to always send path
	FExportFlags ExportFlags;

	if ( NetGUID.IsDefault() )
	{
		ExportFlags.bHasPath = 1;
	}		
	else if ( IsExportingNetGUIDBunch )
	{
		if ( Object != NULL )
		{
			ExportFlags.bHasPath = ShouldSendFullPath( Object, NetGUID ) ? 1 : 0;
		}
		else
		{
			ExportFlags.bHasPath = ObjectPathName.IsEmpty() ? 0 : 1;
		}

		ExportFlags.bIsLevel	= ( Cast< const ULevel >( Object ) != NULL ) ? 1 : 0;
		ExportFlags.bIsPackage	= ( Cast< const UPackage >( Object ) != NULL ) ? 1 : 0;

		Ar << ExportFlags.Value;
	}

	if ( ExportFlags.bHasPath )
	{
		if ( Object != NULL )
		{
			// If the object isn't NULL, expect an empty path name, then fill it out with the actual info
			check( ObjectOuter == NULL );
			check( ObjectPathName.IsEmpty() );
			ObjectPathName = Object->GetName();
			ObjectOuter = Object->GetOuter();
		}
		else
		{
			// If we don't have an object, expect an already filled out path name
			check( ObjectOuter != NULL );
			check( !ObjectPathName.IsEmpty() );
		}

		// Serialize reference to outer. This is basically a form of compression.
		FNetworkGUID OuterNetGUID = GuidCache->GetOrAssignNetGUID( ObjectOuter );

		InternalWriteObject( Ar, OuterNetGUID, ObjectOuter, TEXT( "" ), NULL );

		GEngine->NetworkRemapPath(Connection->Driver->GetWorld(), ObjectPathName, false);

		// Serialize Name of object
		Ar << ObjectPathName;

		if ( IsExportingNetGUIDBunch )
		{
			CurrentExportNetGUIDs.Add( NetGUID );

			int32 & Count = NetGUIDExportCountMap.FindOrAdd( NetGUID );
			Count++;
		}
	}
}

//--------------------------------------------------------------------
//
//	Loading
//
//--------------------------------------------------------------------

static void SanityCheckExport( const UObject * Object, const FNetworkGUID & NetGUID, const FString & ExpectedPathName, const UObject * ExpectedOuter, const FExportFlags & ExportFlags )
{
	if ( Object->GetName() != ExpectedPathName )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "SanityCheckExport: Name mismatch. NetGUID: %s, Object: %s, Expected: %s" ), *NetGUID.ToString(), *Object->GetPathName(), *ExpectedPathName );
	}

	if ( Object->GetOuter() != ExpectedOuter )
	{
		const FString CurrentOuterName	= Object->GetOuter() != NULL ? *Object->GetOuter()->GetName() : TEXT( "NULL" );
		const FString ExpectedOuterName = ExpectedOuter != NULL ? *ExpectedOuter->GetName() : TEXT( "NULL" );
		UE_LOG( LogNetPackageMap, Warning, TEXT( "SanityCheckExport: Outer mismatch. Object: %s, NetGUID: %s, Current: %s, Expected: %s" ), *Object->GetPathName(), *NetGUID.ToString(), *CurrentOuterName, *ExpectedOuterName );
	}

	if ( ExportFlags.bIsPackage )
	{
		const UPackage * Package = Cast< const UPackage >( Object );

		if ( Package == NULL )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "SanityCheckExport: Expected package. Object:%s, NetGUID: %s" ), *Object->GetPathName(), *NetGUID.ToString() );
		}
	}
}

/** Loads a UObject from an FArchive stream. Reads object path if there, and tries to load object if its not already loaded */
FNetworkGUID UPackageMapClient::InternalLoadObject( FArchive & Ar, UObject *& Object, const bool bIsInnerLevel, const int InternalLoadObjectRecursionCount )
{
	if ( InternalLoadObjectRecursionCount > INTERNAL_LOAD_OBJECT_RECURSION_LIMIT ) 
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "InternalLoadObject: Hit recursion limit." ) );
		Ar.SetError(); 
		Object = NULL;
		return FNetworkGUID(); 
	} 

	// ----------------	
	// Read the NetGUID
	// ----------------	
	FNetworkGUID NetGUID;
	Ar << NetGUID;
	NET_CHECKSUM_OR_END( Ar );

	if ( Ar.IsError() )
	{
		Object = NULL;
		return NetGUID;
	}

	if ( !NetGUID.IsValid() )
	{
		Object = NULL;
		return NetGUID;
	}

	// ----------------	
	// Try to resolve NetGUID
	// ----------------	
	if ( NetGUID.IsValid() && !NetGUID.IsDefault() )
	{
		Object = GetObjectFromNetGUID( NetGUID );

		UE_CLOG( !bSuppressLogs, LogNetPackageMap, Log, TEXT( "InternalLoadObject loaded %s from NetGUID <%s>" ), Object ? *Object->GetFullName() : TEXT( "NULL" ), *NetGUID.ToString() );
	}

	// ----------------	
	// Read the full if its there
	// ----------------	
	FExportFlags ExportFlags;

	if ( NetGUID.IsDefault() )
	{
		ExportFlags.bHasPath = 1;
	}
	else if ( IsExportingNetGUIDBunch )
	{
		Ar << ExportFlags.Value;

		if ( Ar.IsError() )
		{
			Object = NULL;
			return NetGUID;
		}
	}

	if ( ExportFlags.bHasPath )
	{
		UObject * ObjOuter = NULL;

		FNetworkGUID OuterGUID = InternalLoadObject( Ar, ObjOuter, ExportFlags.bIsLevel || bIsInnerLevel, InternalLoadObjectRecursionCount + 1 );

		FString PathName;
		Ar << PathName;

		if ( Ar.IsError() )
		{
			UE_LOG( LogNetPackageMap, Error, TEXT( "InternalLoadObject: Failed to load path name" ) );
			Object = NULL;
			return NetGUID;
		}

		// Remap name for PIE
		GEngine->NetworkRemapPath( Connection->Driver->GetWorld(), PathName, true );

		if ( Object != NULL )
		{
			// If we already have the object, just do some sanity checking and return
			SanityCheckExport( Object, NetGUID, PathName, ObjOuter, ExportFlags );
			return NetGUID;
		}

		if ( NetGUID.IsDefault() )
		{
			// This should be from the client
			// If we get here, we want to go ahead and assign a network guid, 
			// then export that to the client at the next available opportunity
			check( IsNetGUIDAuthority() );

			Object = GuidCache->ResolvePath( PathName, ObjOuter, false );

			if ( Object == NULL )
			{
				UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::InternalLoadObject: Unable to resolve default guid from client: PathName: %s, ObjOuter: %s " ), *PathName, ObjOuter != NULL ? *ObjOuter->GetPathName() : TEXT( "NULL" ) );
				return NetGUID;
			}

			// Assign the guid to the object
			NetGUID = GuidCache->GetOrAssignNetGUID( Object );

			// Let this client know what guid we assigned
			HandleUnAssignedObject( Object );

			return NetGUID;
		}

		// If we are the server, we should have found the object by now
		if ( IsNetGUIDAuthority() )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::InternalLoadObject: Server could not resolve non default guid from client. PathName: %s, ObjOuter: %s " ), *PathName, ObjOuter != NULL ? *ObjOuter->GetPathName() : TEXT( "NULL" ) );
			return NetGUID;
		}

		// At this point, only the client gets this far
		const bool bIsForLevel			= ExportFlags.bIsLevel || bIsInnerLevel;			// Make note if this is for a level (or any of our inners are)
		const bool bNoLoad				= bIsForLevel;
		const bool bIgnoreWhenMissing	= bIsForLevel || GuidCache->ShouldIgnoreWhenMissing( OuterGUID );

		// Register this path and outer guid combo with the net guid
		GuidCache->RegisterNetGUIDFromPath_Client( NetGUID, PathName, OuterGUID, bNoLoad, bIgnoreWhenMissing );

		// Try again now that we've registered the path
		Object = GuidCache->GetObjectFromNetGUID( NetGUID );

		if ( Object == NULL && !GuidCache->ShouldIgnoreWhenMissing( NetGUID ) )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "InternalLoadObject: Unable to resolve object from path. Path: %s, Outer: %s, NetGUID: %s" ), *PathName, ObjOuter ? *ObjOuter->GetPathName() : TEXT( "NULL" ), *NetGUID.ToString() );
		}
	}
	else if ( Object == NULL && NetGUID.IsStatic() && !GuidCache->ShouldIgnoreWhenMissing( NetGUID ) )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "InternalLoadObject: Unable to resolve object. NetGUID: %s, HasPath: %s" ), *NetGUID.ToString(), ExportFlags.bHasPath ? TEXT( "TRUE" ) : TEXT( "FALSE" ) );
	}

	return NetGUID;
}

UObject * UPackageMapClient::ResolvePathAndAssignNetGUID( const FNetworkGUID & NetGUID, const FString & PathName )
{
	if ( NetGUID.IsDynamic() )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::ResolvePathAndAssignNetGUID. Trying to load dynamic NetGUID by path. Path: %s, NetGUID: %s" ), *PathName, *NetGUID.ToString() );
	}

	if ( NetGUID.IsDefault() )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::ResolvePathAndAssignNetGUID. Trying to load default NetGUID by path. Path: %s, NetGUID: %s" ), *PathName, *NetGUID.ToString() );
		return NULL;
	}

	UObject * Object = GuidCache->ResolvePath( PathName, NULL, false );

	if ( Object == NULL )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::ResolvePathAndAssignNetGUID. Could not resolve path. Path: %s, NetGUID: %s" ), *PathName, *NetGUID.ToString() );
		return NULL;
	}

	GuidCache->RegisterNetGUID_Client( NetGUID, Object );

	return Object;
}

//--------------------------------------------------------------------
//
//	Network - NetGUID Bunches (Export Table)
//
//	These functions deal with exporting new NetGUIDs in separate, discrete bunches.
//	These bunches are appended to normal 'content' bunches. You can think of it as an
//	export table that is prepended to bunches.
//
//--------------------------------------------------------------------

/** Exports the NetGUID and paths needed to the CurrentExportBunch */
bool UPackageMapClient::ExportNetGUID( FNetworkGUID NetGUID, const UObject * Object, FString PathName, UObject * ObjOuter )
{
	check( NetGUID.IsValid() );
	check( ( Object == NULL ) == !PathName.IsEmpty() );
	check( !NetGUID.IsDefault() );
	check( Object == NULL || ShouldSendFullPath( Object, NetGUID ) );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Warn if dynamic, non actor component, NetGUID is being exported.
	if ( NetGUID.IsDynamic() && ( !PathName.IsEmpty() || ( Object && Cast<const UActorComponent>( Object ) == NULL ) ) )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "ExportNetGUID: Exporting Dynamic NetGUID. NetGUID: <%s> Object: %s" ), *NetGUID.ToString(), Object ? *Object->GetName() : *PathName );
	}
#endif

	// Two passes are used to export this net guid:
	// 1. Attempt to append this net guid to current bunch
	// 2. If step 1 fails, append to fresh new bunch
	for ( int32 NumTries = 0; NumTries < 2; NumTries++ )
	{
		if ( !CurrentExportBunch )
		{
			check( ExportNetGUIDCount == 0 );

			CurrentExportBunch = new FOutBunch(this, Connection->MaxPacket*8-MAX_BUNCH_HEADER_BITS-MAX_PACKET_TRAILER_BITS-MAX_PACKET_HEADER_BITS );
			CurrentExportBunch->SetAllowResize(false);
			CurrentExportBunch->bHasGUIDs = true;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			CurrentExportBunch->DebugString = TEXT("NetGUIDs");
#endif

			ExportNetGUIDCount = 0;
			*CurrentExportBunch << ExportNetGUIDCount;
			NET_CHECKSUM(*CurrentExportBunch);
		}

		if ( CurrentExportNetGUIDs.Num() != 0 )
		{
			UE_LOG( LogNetPackageMap, Fatal, TEXT( "ExportNetGUID - CurrentExportNetGUIDs.Num() != 0 (%s)." ), Object ? *Object->GetName() : *PathName );
			return false;
		}

		// Push our current state in case we overflow with this export and have to pop it off.
		FBitWriterMark LastExportMark;
		LastExportMark.Init( *CurrentExportBunch );

		IsExportingNetGUIDBunch = true;

		InternalWriteObject( *CurrentExportBunch, NetGUID, Object, PathName, ObjOuter );

		IsExportingNetGUIDBunch = false;

		if ( CurrentExportNetGUIDs.Num() == 0 )
		{
			// Some how we failed to export this GUID 
			// This means no path names were written, which means we possibly are incorrectly not writing paths out, or we shouldn't be here in the first place
			UE_LOG( LogNetPackageMap, Warning, TEXT( "ExportNetGUID - InternalWriteObject no GUID's were exported: %s " ), Object ? *Object->GetName() : *PathName );
			LastExportMark.Pop( *CurrentExportBunch );
			return false;
		}
	
		if ( !CurrentExportBunch->IsError() )
		{
			// Success, append these exported guid's to the list going out on this bunch
			CurrentExportBunch->ExportNetGUIDs.Append( CurrentExportNetGUIDs.Array() );
			CurrentExportNetGUIDs.Empty();		// Done with this
			ExportNetGUIDCount++;
			return true;
		}

		// Overflowed, wrap up the currently pending bunch, and start a new one
		LastExportMark.Pop( *CurrentExportBunch );

		// Make sure we reset this so it doesn't persist into the next batch
		CurrentExportNetGUIDs.Empty();

		if ( ExportNetGUIDCount == 0 || NumTries == 1 )
		{
			// This means we couldn't serialize this NetGUID into a single bunch. The path could be ridiculously big (> ~512 bytes) or something else is very wrong
			UE_LOG( LogNetPackageMap, Fatal, TEXT( "ExportNetGUID - Failed to serialize NetGUID into single bunch. (%s)" ), Object ? *Object->GetName() : *PathName  );
			return false;
		}

		for ( auto It = CurrentExportNetGUIDs.CreateIterator(); It; ++It )
		{
			int32 & Count = NetGUIDExportCountMap.FindOrAdd( *It );
			Count--;
		}

		// Export current bunch, create a new one, and try again.
		ExportNetGUIDHeader();
	}

	check( 0 );		// Shouldn't get here

	return false;
}

/** Called when an export bunch is finished. It writes how many NetGUIDs are contained in the bunch and finalizes the book keeping so we know what NetGUIDs are in the bunch */
void UPackageMapClient::ExportNetGUIDHeader()
{
	check(CurrentExportBunch);
	FOutBunch& Out = *CurrentExportBunch;

	UE_LOG(LogNetPackageMap, Log, TEXT("	UPackageMapClient::ExportNetGUID. Bytes: %d Bits: %d ExportNetGUIDCount: %d"), CurrentExportBunch->GetNumBytes(), CurrentExportBunch->GetNumBits(), ExportNetGUIDCount);

	// Rewrite how many NetGUIDs were exported.
	FBitWriterMark Reset;
	FBitWriterMark Restore(Out);
	Reset.PopWithoutClear(Out);
	Out << ExportNetGUIDCount;
	Restore.PopWithoutClear(Out);

	// If we've written new NetGUIDs to the 'bunch' set (current+1)
	if (UE_LOG_ACTIVE(LogNetPackageMap,Verbose))
	{
		UE_LOG(LogNetPackageMap, Verbose, TEXT("ExportNetGUIDHeader: "));
		for (auto It = CurrentExportBunch->ExportNetGUIDs.CreateIterator(); It; ++It)
		{
			UE_LOG(LogNetPackageMap, Verbose, TEXT("  NetGUID: %s"), *It->ToString());
		}
	}

	// CurrentExportBunch *should* always have NetGUIDs to export. If it doesn't warn. This is a bug.
	if ( CurrentExportBunch->ExportNetGUIDs.Num() != 0 )	
	{
		ExportBunches.Add( CurrentExportBunch );
	}
	else
	{
		UE_LOG(LogNetPackageMap, Warning, TEXT("Attempted to export a NetGUID Bunch with no NetGUIDs!"));
	}
	
	CurrentExportBunch = NULL;
	ExportNetGUIDCount = 0;
}

void UPackageMapClient::ReceiveNetGUIDBunch( FInBunch &InBunch )
{
	IsExportingNetGUIDBunch = true;
	check(InBunch.bHasGUIDs);

	int32 NumGUIDsInBunch = 0;
	InBunch << NumGUIDsInBunch;
	
	static const int32 MAX_GUID_COUNT = 2048;

	if ( NumGUIDsInBunch > MAX_GUID_COUNT )
	{
		UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::ReceiveNetGUIDBunch: NumGUIDsInBunch > MAX_GUID_COUNT (%i)" ), NumGUIDsInBunch );
		InBunch.SetError();
		return;
	}

	NET_CHECKSUM(InBunch);

	UE_LOG(LogNetPackageMap, Log, TEXT("UPackageMapClient::ReceiveNetGUIDBunch %d NetGUIDs. PacketId %d. ChSequence %d. ChIndex %d"), NumGUIDsInBunch, InBunch.PacketId, InBunch.ChSequence, InBunch.ChIndex );

	int32 NumGUIDsRead = 0;
	while( NumGUIDsRead < NumGUIDsInBunch )
	{
		UObject * Obj = NULL;
		InternalLoadObject( InBunch, Obj, false, 0 );

		if ( InBunch.IsError() )
		{
			UE_LOG( LogNetPackageMap, Error, TEXT( "UPackageMapClient::ReceiveNetGUIDBunch: InBunch.IsError() after InternalLoadObject" ) );
			return;
		}
		NumGUIDsRead++;
	}

	InBunch.EatByteAlign();

	UE_LOG(LogNetPackageMap, Log, TEXT("UPackageMapClient::ReceiveNetGUIDBunch end. BitPos: %d"), InBunch.GetPosBits() );
	IsExportingNetGUIDBunch = false;
}

bool UPackageMapClient::AppendExportBunches(TArray<FOutBunch *>& OutgoingBunches)
{
	// Finish current in progress bunch if necessary
	if (ExportNetGUIDCount > 0)
	{
		ExportNetGUIDHeader();
	}

	// Append the bunches we've made to the passed in list reference
	if (ExportBunches.Num() > 0)
	{
		if (UE_LOG_ACTIVE(LogNetPackageMap,Log))
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("AppendExportBunches. %d Bunches  %d"), ExportBunches.Num(), ExportNetGUIDCount);
			for (auto It=ExportBunches.CreateIterator(); It; ++It)
			{
				UE_LOG(LogNetPackageMap, Log, TEXT("   Bunch[%d]. NumBytes: %d NumBits: %d"), It.GetIndex(), (*It)->GetNumBytes(), (*It)->GetNumBits() );
			}
		}

		for (auto It = ExportBunches.CreateIterator(); It; ++It)
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Bunch[%d] Num ExportNetGUIDs: %d"), It.GetIndex(), (*It)->ExportNetGUIDs.Num() );
		}


		OutgoingBunches.Append(ExportBunches);
		ExportBunches.Empty();
		return true;
	}

	return false;
}

//--------------------------------------------------------------------
//
//	Network - ACKing
//
//--------------------------------------------------------------------

/**
 *	Called when a bunch is committed to the connection's Out buffer.
 *	ExportNetGUIDs is the list of GUIDs stored on the bunch that we use to update the expected sequence for those exported GUIDs
 */
void UPackageMapClient::NotifyBunchCommit( const int32 OutPacketId, const TArray< FNetworkGUID > & ExportNetGUIDs )
{
	check( OutPacketId > GUID_PACKET_ACKED );	// Assumptions break if this isn't true ( We assume ( OutPacketId > GUID_PACKET_ACKED ) == PENDING )
	check( ExportNetGUIDs.Num() > 0 );			// We should have never created this bunch if there are not exports

#if 1
	// Not sure we actually need to do this.  
	// Assuming we are acking/nacking property, and those systems are working correctly, I don't see the need.
	// Keeping it for now though, maybe I'm missing something.
	if ( Locked )
	{
		return;
	}
#endif

	for ( int32 i = 0; i < ExportNetGUIDs.Num(); i++ )
	{
		if ( !NetGUIDAckStatus.Contains( ExportNetGUIDs[i] ) )
		{
			NetGUIDAckStatus.Add( ExportNetGUIDs[i], GUID_PACKET_NOT_ACKED );
		}

		int32 & ExpectedPacketIdRef = NetGUIDAckStatus.FindChecked( ExportNetGUIDs[i] );

		// Only update expected sequence if this guid was previously nak'd
		// If we always update to the latest packet id, we risk prolonging the ack for no good reason
		// (GUID information doesn't change, so updating to the latest expected sequence is unnecessary)
		if ( ExpectedPacketIdRef == GUID_PACKET_NOT_ACKED )
		{
			ExpectedPacketIdRef = OutPacketId;
			check( !PendingAckGUIDs.Contains( ExportNetGUIDs[i] ) );	// If we hit this assert, this means the lists are out of sync
			PendingAckGUIDs.AddUnique( ExportNetGUIDs[i] );
		}
	}
}

/**
 *	Called by the PackageMap's UConnection after a receiving an ack
 *	Updates the respective GUIDs that were acked by this packet
 */
void UPackageMapClient::ReceivedAck( const int32 AckPacketId )
{
	for ( int32 i = PendingAckGUIDs.Num() - 1; i >= 0; i-- )
	{
		int32 & ExpectedPacketIdRef = NetGUIDAckStatus.FindChecked( PendingAckGUIDs[i] );

		check( ExpectedPacketIdRef > GUID_PACKET_ACKED );		// Make sure we really are pending, since we're on the list

		if ( ExpectedPacketIdRef > GUID_PACKET_ACKED && ExpectedPacketIdRef <= AckPacketId )
		{
			ExpectedPacketIdRef = GUID_PACKET_ACKED;	// Fully acked
			PendingAckGUIDs.RemoveAt( i );				// Remove from pending list, since we're now acked
		}
	}
}

/**
 *	Handles a NACK for given packet id. If this packet ID contained a NetGUID reference, we redirty the NetGUID by setting
 *	its entry in NetGUIDAckStatus to GUID_PACKET_NOT_ACKED.
 */
void UPackageMapClient::ReceivedNak( const int32 NakPacketId )
{
	for ( int32 i = PendingAckGUIDs.Num() - 1; i >= 0; i-- )
	{
		int32 & ExpectedPacketIdRef = NetGUIDAckStatus.FindChecked( PendingAckGUIDs[i] );

		check( ExpectedPacketIdRef > GUID_PACKET_ACKED );		// Make sure we aren't acked, since we're on the list

		if ( ExpectedPacketIdRef == NakPacketId )
		{
			ExpectedPacketIdRef = GUID_PACKET_NOT_ACKED;
			// Remove from pending list since we're no longer pending
			// If we send another reference to this GUID, it will get added back to this list to hopefully get acked next time
			PendingAckGUIDs.RemoveAt( i );	
		}
	}
}

/**
 *	Returns true if this PackageMap's connection has ACK'd the given NetGUID.
 */
bool UPackageMapClient::NetGUIDHasBeenAckd(FNetworkGUID NetGUID)
{
	if (!NetGUID.IsValid())
	{
		// Invalid NetGUID == NULL obect, so is ack'd by default
		return true;
	}

	if (NetGUID.IsDefault())
	{
		// Default NetGUID is 'unassigned' but valid. It is never Ack'd
		return false;
	}

	if (!IsNetGUIDAuthority())
	{
		// We arent the ones assigning NetGUIDs, so yes this is fully ackd
		return true;
	}

	// If brand new, add it to map with GUID_PACKET_NOT_ACKED
	if ( !NetGUIDAckStatus.Contains( NetGUID ) )
	{
		NetGUIDAckStatus.Add( NetGUID, GUID_PACKET_NOT_ACKED );
	}

	int32 & AckPacketId = NetGUIDAckStatus.FindChecked( NetGUID );	

	if ( AckPacketId == GUID_PACKET_ACKED )
	{
		// This GUID has been fully Ackd
		UE_LOG( LogNetPackageMap, Verbose, TEXT("NetGUID <%s> is fully ACKd (AckPacketId: %d <= Connection->OutAckPacketIdL %d) "), *NetGUID.ToString(), AckPacketId, Connection->OutAckPacketId );
		return true;
	}
	else if ( AckPacketId == GUID_PACKET_NOT_ACKED )
	{
		
	}

	return false;
}

/** Immediately export an Object's NetGUID. This will */
void UPackageMapClient::HandleUnAssignedObject(const UObject* Obj)
{
	check( Obj != NULL );

	FNetworkGUID NetGUID = GuidCache->GetOrAssignNetGUID( Obj );

	if ( !NetGUID.IsDefault() && ShouldSendFullPath( Obj, NetGUID ) )
	{
		if ( !ExportNetGUID( NetGUID, Obj, TEXT( "" ), NULL ) )
		{
			UE_LOG( LogNetPackageMap, Verbose, TEXT( "Failed to export in ::HandleUnAssignedObject %s" ), Obj ? *Obj->GetName() : TEXT("NULL") );
		}
	}
}

//--------------------------------------------------------------------
//
//	Misc
//
//--------------------------------------------------------------------

/** Do we need to include the full path of this object for the client to resolve it? */
bool UPackageMapClient::ShouldSendFullPath(const UObject* Object, const FNetworkGUID &NetGUID)
{
	if ( !Connection )
	{
		return false;
	}

	// NetGUID is already exported
	if ( CurrentExportBunch != NULL && CurrentExportBunch->ExportNetGUIDs.Contains( NetGUID ) )
	{
		return false;
	}

	if ( !NetGUID.IsValid() )
	{
		return false;
	}

	if ( NetGUID.IsDynamic() )
	{
		return false;		// We only export fully stably named objects
	}

	if ( NetGUID.IsDefault() )
	{
		return true;
	}

	return Locked || !NetGUIDHasBeenAckd( NetGUID );
}

/**
 *	Prints debug info about this package map's state
 */
void UPackageMapClient::LogDebugInfo( FOutputDevice & Ar )
{
	for ( auto It = GuidCache->NetGUIDLookup.CreateIterator(); It; ++It )
	{
		FNetworkGUID NetGUID = It.Value();

		FString Status = TEXT("Unused");
		if ( NetGUIDAckStatus.Contains( NetGUID ) )
		{
			const int32 PacketId = NetGUIDAckStatus.FindRef(NetGUID);
			if ( PacketId == GUID_PACKET_NOT_ACKED )
			{
				Status = TEXT("UnAckd");
			}
			else if ( PacketId == GUID_PACKET_ACKED )
			{
				Status = TEXT("Ackd");
			}
			else
			{
				Status = TEXT("Pending");
			}
		}

		UObject *Obj = It.Key().Get();
		FString Str = FString::Printf(TEXT("%s [%s] [%s] - %s"), *NetGUID.ToString(), *Status, NetGUID.IsDynamic() ? TEXT("Dynamic") : TEXT("Static") , Obj ? *Obj->GetPathName() : TEXT("NULL"));
		Ar.Logf(*Str);
		UE_LOG(LogNetPackageMap, Log, TEXT("%s"), *Str);
	}
}

/**
 *	Returns true if Object's outer level has completely finished loading.
 */
bool UPackageMapClient::ObjectLevelHasFinishedLoading(UObject* Object)
{
	if (Object != NULL && Connection!= NULL && Connection->Driver != NULL && Connection->Driver->GetWorld() != NULL)
	{
		// get the level for the object
		ULevel* Level = NULL;
		for (UObject* Obj = Object; Obj != NULL; Obj = Obj->GetOuter())
		{
			Level = Cast<ULevel>(Obj);
			if (Level != NULL)
			{
				break;
			}
		}
		
		if (Level != NULL && Level != Connection->Driver->GetWorld()->PersistentLevel)
		{
			return Level->bIsVisible;
		}
	}

	return true;
}

/**
 * Return false if our connection is the netdriver's server connection
 *  This is ugly but probably better than adding a shadow variable that has to be
 *  set/cleared at the net driver level.
 */
bool UPackageMapClient::IsNetGUIDAuthority()
{
	return GuidCache->IsNetGUIDAuthority();
}

/** Resets packagemap state for server travel */
void UPackageMapClient::ResetPackageMap()
{
	NetGUIDExportCountMap.Empty();
	CurrentExportNetGUIDs.Empty();
	NetGUIDAckStatus.Empty();
	PendingAckGUIDs.Empty();
}

/**	
 *	Returns stats for NetGUID usage
 */
void UPackageMapClient::GetNetGUIDStats(int32 &AckCount, int32 &UnAckCount, int32 &PendingCount)
{
	AckCount = UnAckCount = PendingCount = 0;
	for ( auto It = NetGUIDAckStatus.CreateIterator(); It; ++It )
	{
		// Sanity check that we're in sync
		check( ( It.Value() > GUID_PACKET_ACKED ) == PendingAckGUIDs.Contains( It.Key() ) );

		if ( It.Value() == GUID_PACKET_NOT_ACKED )
		{
			UnAckCount++;
		}
		else if ( It.Value() == GUID_PACKET_ACKED )
		{
			AckCount++;
		}
		else
		{
			PendingCount++;
		}
	}

	// Sanity check that we're in sync
	check( PendingAckGUIDs.Num() == PendingCount );
}

void UPackageMapClient::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	return Super::AddReferencedObjects(InThis, Collector);
}

void UPackageMapClient::NotifyStreamingLevelUnload(UObject* UnloadedLevel)
{
}

bool UPackageMapClient::PrintExportBatch()
{
	if ( ExportNetGUIDCount <= 0 && CurrentExportBunch == NULL )
	{
		return false;
	}

	// Print the whole thing for reference
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	for (auto It = GuidCache->History.CreateIterator(); It; ++It)
	{
		FString Str = It.Value();
		FNetworkGUID NetGUID = It.Key();
		UE_LOG(LogNetPackageMap, Warning, TEXT("<%s> - %s"), *NetGUID.ToString(), *Str);
	}
#endif

	UE_LOG(LogNetPackageMap, Warning, TEXT("\n\n"));
	if ( CurrentExportBunch != NULL )
	{
		for (auto It = CurrentExportBunch->ExportNetGUIDs.CreateIterator(); It; ++It)
		{
			UE_LOG(LogNetPackageMap, Warning, TEXT("  CurrentExportBunch->ExportNetGUIDs: %s"), *It->ToString());
		}
	}

	UE_LOG(LogNetPackageMap, Warning, TEXT("\n"));
	for (auto It = CurrentExportNetGUIDs.CreateIterator(); It; ++It)
	{
		UE_LOG(LogNetPackageMap, Warning, TEXT("  CurrentExportNetGUIDs: %s"), *It->ToString());
	}

	return true;
}

UObject * UPackageMapClient::GetObjectFromNetGUID( const FNetworkGUID & NetGUID )
{
	return GuidCache->GetObjectFromNetGUID( NetGUID );
}

//----------------------------------------------------------------------------------------
//	FNetGUIDCache
//----------------------------------------------------------------------------------------

FNetGUIDCache::FNetGUIDCache( UNetDriver * InDriver ) : Driver( InDriver )
{
	UniqueNetIDs[0] = UniqueNetIDs[1] = 0;
}

void FNetGUIDCache::Reset()
{
	ObjectLookup.Empty();
	NetGUIDLookup.Empty();
	UniqueNetIDs[0] = UniqueNetIDs[1] = 0;
}

class FArchiveCountMemGUID : public FArchive
{
public:
	FArchiveCountMemGUID() : Mem( 0 ) { ArIsCountingMemory = true; }
	void CountBytes( SIZE_T InNum, SIZE_T InMax ) { Mem += InMax; }
	SIZE_T Mem;
};

void FNetGUIDCache::CleanReferences()
{
	// NOTE - Quick fix for some package map issues where client was cleaning up guids, which was causing issues.
	// For now, we no longer clean these up.  The client can sometimes cleanup guids that the server doesn't which causes issues.
	// The guid memory will leak for now until we find a better way.

	// Free any dynamic guids that refer to objects that are dead
	for ( auto It = ObjectLookup.CreateIterator(); It; ++It )
	{
		if ( It.Value().Object.IsValid() )
		{
			continue;
		}

		if ( It.Key().IsDynamic() )
		{
			It.RemoveCurrent();
		}
	}

	for ( auto It = NetGUIDLookup.CreateIterator(); It; ++It )
	{
		if ( !It.Key().IsValid() )
		{
			It.RemoveCurrent();
		}
	}

	// Sanity check
	for ( auto It = ObjectLookup.CreateIterator(); It; ++It )
	{
		check( It.Value().Object.IsValid() || It.Key().IsStatic() );

		if ( It.Value().Object.IsValid() )
		{
			checkf( !It.Value().Object.IsValid() || NetGUIDLookup.FindRef( It.Value().Object ) == It.Key(), TEXT("Failed to validate ObjectLookup map in UPackageMap. Object '%s' was not in the NetGUIDLookup map with with value '%s'."), *It.Value().Object.Get()->GetPathName(), *It.Key().ToString());
		}
		else
		{
			for ( auto It2 = NetGUIDLookup.CreateIterator(); It2; ++It2 )
			{
				checkf( It2.Value() != It.Key(), TEXT("Failed to validate ObjectLookup map in UPackageMap. Object '%s' was in the NetGUIDLookup map with with value '%s'."), *It.Value().Object.Get()->GetPathName(), *It.Key().ToString());
			}
		}
	}

	for ( auto It = NetGUIDLookup.CreateIterator(); It; ++It )
	{
		check( It.Key().IsValid() );
		checkf( ObjectLookup.FindRef( It.Value() ).Object == It.Key(), TEXT("Failed to validate NetGUIDLookup map in UPackageMap. GUID '%s' was not in the ObjectLookup map with with object '%s'."), *It.Value().ToString(), *It.Key().Get()->GetPathName());
	}

	FArchiveCountMemGUID CountBytesAr;

	ObjectLookup.CountBytes( CountBytesAr );
	NetGUIDLookup.CountBytes( CountBytesAr );

	// Make this a warning to be a constant reminder that we need to ultimately free this memory when we find a solution
	UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::CleanReferences: ObjectLookup: %i, NetGUIDLookup: %i, Mem: %i kB" ), ObjectLookup.Num(), NetGUIDLookup.Num(), ( CountBytesAr.Mem / 1024 ) );
}

bool FNetGUIDCache::SupportsObject( const UObject * Object )
{
	// NULL is always supported
	if ( !Object )
	{
		return true;
	}

	// If we already gave it a NetGUID, its supported.
	// This should happen for dynamic subobjects.
	FNetworkGUID NetGUID = NetGUIDLookup.FindRef( Object );

	if ( NetGUID.IsValid() )
	{
		return true;
	}

	if ( Object->IsFullNameStableForNetworking() )
	{
		// If object is fully net addressable, it's definitely supported
		return true;
	}

	if ( Object->IsSupportedForNetworking() )
	{
		// This means the server will explicitly tell the client to spawn and assign the id for this object
		return true;
	}

	UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::SupportsObject: %s NOT Supported." ), *Object->GetFullName() );
	//UE_LOG( LogNetPackageMap, Warning, TEXT( "   %s"), *DebugContextString );

	return false;
}

/**
 *	Dynamic objects are actors that were spawned in the world at run time, and therefor cannot be
 *	referenced with a path name to the client.
 */
bool FNetGUIDCache::IsDynamicObject( const UObject * Object )
{
	check( Object != NULL );
	check( Object->IsSupportedForNetworking() );

	// Any non net addressable object is dynamic
	return !Object->IsFullNameStableForNetworking();
}

bool FNetGUIDCache::IsNetGUIDAuthority()
{
	return Driver == NULL || Driver->IsServer();
}

/** Gets or assigns a new NetGUID to this object. Returns whether the object is fully mapped or not */
FNetworkGUID FNetGUIDCache::GetOrAssignNetGUID( const UObject * Object )
{
	if ( !Object || !SupportsObject( Object ) )
	{
		// Null of unsupported object, leave as default NetGUID and just return mapped=true
		return FNetworkGUID();
	}

	// ----------------
	// Assign NetGUID if necessary
	// ----------------
	FNetworkGUID NetGUID = NetGUIDLookup.FindRef( Object );

	if ( NetGUID.IsValid() )
	{
		return NetGUID;
	}

	if ( !IsNetGUIDAuthority() )
	{
		// We cannot make or assign new NetGUIDs
		// Generate a default GUID, which signifies we write the full path
		// The server should detect this, and assign a full-time guid, and send that back to us
		return FNetworkGUID::GetDefault();
	}

	return AssignNewNetGUID_Server( Object );
}

static const AActor * GetOuterActor( const UObject * Object )
{
	Object = Object->GetOuter();

	while ( Object != NULL )
	{
		const AActor * Actor = Cast< const AActor >( Object );

		if ( Actor != NULL )
		{
			return Actor;
		}

		Object = Object->GetOuter();
	}

	return NULL;
}

/**
 *	Generate a new NetGUID for this object and assign it.
 */
FNetworkGUID FNetGUIDCache::AssignNewNetGUID_Server( const UObject * Object )
{
	check( IsNetGUIDAuthority() );

	const AActor * Actor = Cast< const AActor >( Object );

	// So what is going on here, is we want to make sure the network guid's of stably named sub-objects get 
	// initialized in sync with their owning actor. What this allows us to do, is not consume any extra bandwidth 
	// when assigning these guid's to the sub-objects on each client
	if ( Actor == NULL )
	{
		const AActor * OuterActor = GetOuterActor( Object );

		if ( OuterActor != NULL && IsDynamicObject( OuterActor ) )
		{
			TArray< UObject * > Subobjs;
			const_cast< AActor * >( OuterActor )->GetSubobjectsWithStableNamesForNetworking( Subobjs );

			if ( Subobjs.Contains( const_cast< UObject * >( Object ) ) )
			{
				// Since we always assign guids to components when the owning actor gets assigned, we assume that if we get here
				// the owning actor must not have been assigned yet
				check( !NetGUIDLookup.Contains( OuterActor ) );

				// Assign our owning actor a guid first (which should also assign our net guid if things are working correctly)
				AssignNewNetGUID_Server( OuterActor );

				return NetGUIDLookup.FindChecked( Object );
			}
		}
	}

#define ALLOC_NEW_NET_GUID( IsStatic ) ( COMPOSE_NET_GUID( ++UniqueNetIDs[ IsStatic ], IsStatic ) )

	// Generate new NetGUID and assign it
	const int32 IsStatic = IsDynamicObject( Object ) ? 0 : 1;

	const FNetworkGUID NewNetGuid( ALLOC_NEW_NET_GUID( IsStatic ) );

	RegisterNetGUID_Server( NewNetGuid, Object );

	// If this is a dynamic actor, assign all of our stably named sub-objects their net guids now as well (they come as a group)
	if ( Actor != NULL && !IsStatic )
	{
		TArray< UObject * > Subobjs;

		const_cast< AActor * >( Actor )->GetSubobjectsWithStableNamesForNetworking( Subobjs );

		for ( int32 i = 0; i < Subobjs.Num(); i++ )
		{
			UObject * SubObj = Subobjs[i];

			check( GetOuterActor( SubObj ) == Actor );

			if ( NetGUIDLookup.Contains( SubObj ) )		// We should NOT have a guid yet
			{
				UE_LOG( LogNetPackageMap, Error, TEXT( "AssignNewNetGUID_Server: Sub object already registered: Actor: %s, SubObj: %s, NetGuid: %s" ), *Actor->GetPathName(), *SubObj->GetPathName(), *NetGUIDLookup.FindChecked( SubObj ).ToString() );
			}
			const int32 SubIsStatic = IsDynamicObject( SubObj ) ? 0 : 1;
			check( SubIsStatic == IsStatic );
			const FNetworkGUID SubobjNetGUID( ALLOC_NEW_NET_GUID( SubIsStatic ) );
			check( SubobjNetGUID.Value == COMPOSE_RELATIVE_NET_GUID( NewNetGuid, i + 1 ) );
			RegisterNetGUID_Server( SubobjNetGUID, SubObj );
		}
	}

	return NewNetGuid;
}

void FNetGUIDCache::RegisterNetGUID_Internal( const FNetworkGUID & NetGUID, const FNetGuidCacheObject & CacheObject )
{
	// We're pretty strict in this function, we expect everything to have been handled before we get here
	check( !ObjectLookup.Contains( NetGUID ) );

	ObjectLookup.Add( NetGUID, CacheObject );

	if ( CacheObject.Object != NULL )
	{
		check( !NetGUIDLookup.Contains( CacheObject.Object ) );

		// If we have an object, associate it with this guid now
		NetGUIDLookup.Add( CacheObject.Object, NetGUID );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		History.Add( NetGUID, CacheObject.Object->GetPathName() );
#endif
	}
	else
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		History.Add( NetGUID, CacheObject.PathName.ToString() );
#endif
	}
}

/**
 *	Associates a net guid directly with an object
 *  This function is only called on server
 */
void FNetGUIDCache::RegisterNetGUID_Server( const FNetworkGUID & NetGUID, const UObject * Object )
{
	check( IsNetGUIDAuthority() );				// Only the server should call this
	check( !Object->IsPendingKill() );
	check( !NetGUID.IsDefault() );
	check( !ObjectLookup.Contains( NetGUID ) );	// Server should never add twice

	FNetGuidCacheObject CacheObject;

	CacheObject.Object = Object;

	RegisterNetGUID_Internal( NetGUID, CacheObject );
}

/**
 *	Associates a net guid directly with an object
 *  This function is only called on clients with dynamic guids
 */
void FNetGUIDCache::RegisterNetGUID_Client( const FNetworkGUID & NetGUID, const UObject * Object )
{
	check( !IsNetGUIDAuthority() );			// Only clients should be here
	check( !Object->IsPendingKill() );
	check( !NetGUID.IsDefault() );
	check( NetGUID.IsDynamic() );	// Clients should only assign dynamic guids through here (static guids go through RegisterNetGUIDFromPath_Client)

	UE_LOG( LogNetPackageMap, Log, TEXT( "RegisterNetGUID_Client: Assigning %s NetGUID <%s>" ), Object ? *Object->GetName() : TEXT( "NULL" ), *NetGUID.ToString() );
	
	//
	// If we have an existing entry, make sure things match up properly
	// We also completely disassociate anything so that RegisterNetGUID_Internal can be fairly strict
	//

	const FNetGuidCacheObject * ExistingCacheObjectPtr = ObjectLookup.Find( NetGUID );

	if ( ExistingCacheObjectPtr )
	{
		if ( ExistingCacheObjectPtr->PathName != NAME_None )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "RegisterNetGUID_Client: Guid with pathname. Path: %s, NetGUID: %s" ), *ExistingCacheObjectPtr->PathName.ToString(), *NetGUID.ToString() );
		}

		// If this net guid was found but the old object is NULL, this can happen due to:
		//	1. Actor channel was closed locally (but we don't remove the net guid entry, since we can't know for sure if it will be referenced again)
		//		a. Then when we re-create a channel, and assign this actor, we will find the old guid entry here
		//	2. Dynamic object was locally GC'd, but then exported again from the server
		//
		// If this net guid was found and the objects match, we don't care. This can happen due to:
		//	1. Same thing above can happen, but if we for some reason didn't destroy the actor/object we will see this case
		//
		// If the object pointers are different, this can be a problem, 
		//	since this should only be possible if something gets out of sync during the net guid exchange code

		const UObject * OldObject = ExistingCacheObjectPtr->Object.Get();

		if ( OldObject != NULL && OldObject != Object )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "RegisterNetGUID_Client: Reassigning NetGUID <%s> to %s (was assigned to object %s)" ), *NetGUID.ToString(), Object ? *Object->GetPathName() : TEXT( "NULL" ), OldObject ? *OldObject->GetPathName() : TEXT( "NULL" ) );
		}
		else
		{
			UE_LOG( LogNetPackageMap, Verbose, TEXT( "RegisterNetGUID_Client: Reassigning NetGUID <%s> to %s (was assigned to object %s)" ), *NetGUID.ToString(), Object ? *Object->GetPathName() : TEXT( "NULL" ), OldObject ? *OldObject->GetPathName() : TEXT( "NULL" ) );
		}

		NetGUIDLookup.Remove( ExistingCacheObjectPtr->Object );
		ObjectLookup.Remove( NetGUID );
	}

	const FNetworkGUID * ExistingNetworkGUIDPtr = NetGUIDLookup.Find( Object );

	if ( ExistingNetworkGUIDPtr )
	{
		// This shouldn't happen on dynamic guids
		UE_LOG( LogNetPackageMap, Warning, TEXT( "Changing NetGUID on object %s from <%s:%s> to <%s:%s>" ), Object ? *Object->GetPathName() : TEXT( "NULL" ), *ExistingNetworkGUIDPtr->ToString(), ExistingNetworkGUIDPtr->IsDynamic() ? TEXT("TRUE") : TEXT("FALSE"), *NetGUID.ToString(), NetGUID.IsDynamic() ? TEXT("TRUE") : TEXT("FALSE") );
		ObjectLookup.Remove( *ExistingNetworkGUIDPtr );
		NetGUIDLookup.Remove( Object );
	}

	FNetGuidCacheObject CacheObject;

	CacheObject.Object = Object;

	RegisterNetGUID_Internal( NetGUID, CacheObject );
}

/**
 *	Associates a net guid with a path, that can be loaded or found later
 *  This function is only called on the client
 */
void FNetGUIDCache::RegisterNetGUIDFromPath_Client( const FNetworkGUID & NetGUID, const FString & PathName, const FNetworkGUID & OuterGUID, const bool bNoLoad, const bool bIgnoreWhenMissing )
{
	check( !IsNetGUIDAuthority() );		// Server never calls this locally
	check( !NetGUID.IsDefault() );
	check( !NetGUID.IsDynamic() );		// It only makes sense to do this for static guids

	const FNetGuidCacheObject * ExistingCacheObjectPtr = ObjectLookup.Find( NetGUID );

	// If we find this guid, make sure nothing changes
	if ( ExistingCacheObjectPtr != NULL )
	{
		if ( ExistingCacheObjectPtr->PathName.ToString() != PathName )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::RegisterNetGUIDFromPath_Client: Path mismatch. Path: %s, Expected: %s, NetGUID: %s" ), *PathName, *ExistingCacheObjectPtr->PathName.ToString(), *NetGUID.ToString() );
		}

		if ( ExistingCacheObjectPtr->OuterGUID != OuterGUID )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::RegisterNetGUIDFromPath_Client: Outer mismatch. Path: %s, Outer: %s, Expected: %s, NetGUID: %s" ), *PathName, *OuterGUID.ToString(), *ExistingCacheObjectPtr->OuterGUID.ToString(), *NetGUID.ToString() );
		}

		if ( ExistingCacheObjectPtr->Object != NULL )
		{
			FNetworkGUID CurrentNetGUID = NetGUIDLookup.FindRef( ExistingCacheObjectPtr->Object );

			if ( CurrentNetGUID != NetGUID )
			{
				UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::RegisterNetGUIDFromPath_Client: Netguid mismatch. Path: %s, NetGUID: %s, Expected: %s" ), *PathName, *NetGUID.ToString(), *CurrentNetGUID.ToString() );
			}
		}

		ObjectLookup.Remove( NetGUID );
	}

	// Register a new guid with this path
	FNetGuidCacheObject CacheObject;

	CacheObject.PathName			= FName( *PathName );
	CacheObject.OuterGUID			= OuterGUID;
	CacheObject.bNoLoad				= bNoLoad;
	CacheObject.bIgnoreWhenMissing	= bIgnoreWhenMissing;

	RegisterNetGUID_Internal( NetGUID, CacheObject );
}

UObject * FNetGUIDCache::GetObjectFromNetGUID( const FNetworkGUID & NetGUID )
{
	if ( !ensure( NetGUID.IsValid() ) )
	{
		return NULL;
	}

	if ( !ensure( !NetGUID.IsDefault() ) )
	{
		return NULL;
	}

	FNetGuidCacheObject * CacheObjectPtr = ObjectLookup.Find( NetGUID );

	if ( CacheObjectPtr == NULL )
	{
		// This net guid has never been registered
		return NULL;
	}

	UObject * Object = CacheObjectPtr->Object.Get();

	if ( Object != NULL )
	{
		// Either the name should match, or this is dynamic, or we're on the server
		check( Object->GetFName() == CacheObjectPtr->PathName || NetGUID.IsDynamic() || IsNetGUIDAuthority() );
		return Object;
	}

	if ( IsNetGUIDAuthority() )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Guid with no object on server. Path: %s, NetGUID: %s" ), *CacheObjectPtr->PathName.ToString(), *NetGUID.ToString() );
		return NULL;
	}

	if ( NetGUID.IsDynamic() )
	{
		// Dynamic objects can never load from their names (name is not stable)
		return NULL;
	}

	if ( CacheObjectPtr->PathName == NAME_None )
	{
		// Static guids should have a path
		UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Static guid on client with no path. NetGUID: %s" ), *NetGUID.ToString() );
		return NULL;
	}

	// First, resolve the outer
	UObject * ObjOuter = NULL;
	
	if ( CacheObjectPtr->OuterGUID.IsValid() )
	{
		// We should have an outer, resolve it
		ObjOuter = GetObjectFromNetGUID( CacheObjectPtr->OuterGUID );

		// If we can't resolve the outer, we need to stop
		if ( ObjOuter == NULL )
		{
			// If the outer is not pending, we need to warn
			if ( !ShouldIgnoreWhenMissing( CacheObjectPtr->OuterGUID ) )
			{
				UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Failed to find outer. Path: %s, NetGUID: %s" ), *CacheObjectPtr->PathName.ToString(), *NetGUID.ToString() );
			}
			return NULL;
		}
	}

	// Resolve the path of this object relative to outer
	Object = ResolvePath( CacheObjectPtr->PathName.ToString(), ObjOuter, CacheObjectPtr->bNoLoad || CacheObjectPtr->bIsPending );

	if ( Object == NULL )
	{
		// If we can't find an object, and it's not pending, warn
		if ( !CacheObjectPtr->bIgnoreWhenMissing )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "GetObjectFromNetGUID: Failed to resolve path. Path: %s, NetGUID: %s" ), *CacheObjectPtr->PathName.ToString(), *NetGUID.ToString() );
		}

		return NULL;
	}

	// Assign the resolved object to this guid
	CacheObjectPtr->Object = Object;		

	if ( NetGUIDLookup.Contains( Object ) )
	{
		// This can happen to static guids on the client. The server deleted this object, but for some reason the client did not
		// When this happens, the server will give the object a different guid, and we will just conform
		ObjectLookup.Remove( NetGUIDLookup.FindRef( Object ) );
	}

	// Make sure the object is in the GUIDToObjectLookup.
	NetGUIDLookup.Add( Object, NetGUID );

	return Object;
}

bool FNetGUIDCache::ShouldIgnoreWhenMissing( const FNetworkGUID & NetGUID )
{
	if ( IsNetGUIDAuthority() )
	{
		return false;		// Server never ignores when missing, always warns
	}

	const FNetGuidCacheObject * CacheObject = ObjectLookup.Find( NetGUID );

	if ( CacheObject != NULL )
	{
		// Ignore warnings when we explicitly are told to, or if we're pending load (which could also be asynchronously loading)
		return CacheObject->bIgnoreWhenMissing || CacheObject->bIsPending;
	}

	return false;
}

UObject * FNetGUIDCache::ResolvePath( const FString & PathName, UObject * ObjOuter, const bool bNoLoad )
{
	UObject * Object = StaticFindObject( UObject::StaticClass(), ObjOuter, *PathName, false );

	if ( bNoLoad || Object != NULL )
	{
		return Object;
	}

	if ( IsNetGUIDAuthority() )
	{
		// The authority shouldn't be loading resources on demand, at least for now.
		// This could be garbage or a security risk
		// Another possibility is in dynamic property replication if the server reads the previously serialized state
		// that has a now destroyed actor in it.
		UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::ResolvePath: Server can't load objects from client. Path: %s" ), *PathName );
		return NULL;
	}

	Object = StaticLoadObject( UObject::StaticClass(), ObjOuter, *PathName, NULL, LOAD_NoWarn );

	if ( Object )
	{
		UE_LOG( LogNetPackageMap, Log, TEXT( "FNetGUIDCache::ResolvePath: StaticLoadObject. Found: %s" ), Object ? *Object->GetName() : TEXT( "NULL" ) );
	}
	else
	{
		//
		// If we failed to load it as an object, try to load it as a package
		//

		UPackage * PackageOuter = Cast< UPackage >( ObjOuter );

		// If we have an outer at this point, it only makes sense for it to be a package (or NULL)
		if ( PackageOuter != NULL || ObjOuter == NULL )
		{
			Object = LoadPackage( PackageOuter, *PathName, LOAD_None );
			UE_LOG( LogNetPackageMap, Log, TEXT( "FNetGUIDCache::ResolvePath: LoadPackage: %s. Found: %s" ), *PathName, Object ? *Object->GetName() : TEXT( "NULL" ) );
		} 
		else
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "FNetGUIDCache::ResolvePath: Outer %s is not a package for object %s" ), ObjOuter ? *ObjOuter->GetName() : TEXT( "NULL" ), *PathName );
		}
	}

	return Object;
}

//------------------------------------------------------
// Debug command to see how many times we've exported each NetGUID
// Used for measuring inefficiencies. Some duplication is unavoidable since we cannot garuntee atomicicity across multiple channels.
// (for example if you have 100 actor channels of the same actor class go out at once, each will have to export the actor's class path in 
// order to be safey resolved... until the NetGUID is ACKd and then new actor channels will not have to export it again).
//
//------------------------------------------------------

static void	ListNetGUIDExports()
{
	struct FCompareNetGUIDCount
	{
		FORCEINLINE bool operator()( const int32& A, const int32& B ) const { return A > B; }
	};

	for (TObjectIterator<UPackageMapClient> PmIt; PmIt; ++PmIt)
	{
		UPackageMapClient *PackageMap = *PmIt;

		
		PackageMap->NetGUIDExportCountMap.ValueSort(FCompareNetGUIDCount());


		UE_LOG(LogNetPackageMap, Warning, TEXT("-----------------------"));
		for (auto It = PackageMap->NetGUIDExportCountMap.CreateIterator(); It; ++It)
		{
			UE_LOG(LogNetPackageMap, Warning, TEXT("NetGUID <%s> - %d"), *(It.Key().ToString()), It.Value() );	
		}
		UE_LOG(LogNetPackageMap, Warning, TEXT("-----------------------"));
	}			
}

FAutoConsoleCommand	ListNetGUIDExportsCommand(
	TEXT("net.ListNetGUIDExports"), 
	TEXT( "Lists open actor channels" ), 
	FConsoleCommandDelegate::CreateStatic(ListNetGUIDExports)
	);



// ----------------------------------------------------------------
