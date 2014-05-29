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

#define USE_NEW_DYNAMIC_AND_SUPPORT_CHECKS 1

/**
 * Don't allow infinite recursion of InternalLoadObject - an attacker could
 * send malicious packets that cause a stack overflow on the server.
 */
static const int INTERNAL_LOAD_OBJECT_RECURSION_LIMIT = 16;

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

		bSerializedUnAckedObject = false;
		FNetworkGUID NetGUID = InternalGetOrAssignNetGUID( Object );
		bool Mapped = InternalIsMapped(Object, NetGUID);

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

#if 0
		if ( Object != NULL && Object->HasAnyFlags( RF_ClassDefaultObject ) )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::SerializeObject: Sent CDO: %s, %s" ), *Object->GetFullName(), *Class->GetFullName() );
			bSerializedCDO = true;
		}
#endif

		return Mapped;
	}
	else if (Ar.IsLoading())
	{
		// ----------------	
		// Read NetGUID from stream and resolve object
		// ----------------	
		FNetworkGUID NetGUID = InternalLoadObject( Ar, Object, false, 0 );

		// Write out NetGUID to caller if necessary
		if (OutNetGUID)
		{
			*OutNetGUID = NetGUID;
		}	

		// ----------------	
		// Final Checks/verification
		// ----------------	

		// NULL if we haven't finished loading the objects level yet
		if (!ObjectLevelHasFinishedLoading(Object))
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Using None instead of replicated reference to %s because the level it's in has not been made visible"), *Object->GetFullName());
			Object = NULL;
		}

		// Check that we got the right class
		if( Object && !Object->IsA(Class) )
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("Forged object: got %s, expecting %s"),*Object->GetFullName(),*Class->GetFullName());
			Object = NULL;
		}

		if ( NetGUID.IsValid() && Object == NULL )
		{
			bLoadedUnmappedObject = true;
			LastUnmappedNetGUID = NetGUID;
		}

		UE_CLOG(!bSuppressLogs, LogNetPackageMap, Log, TEXT("UPackageMapClient::SerializeObject Serialized Object %s as <%s>"), Object ? *Object->GetPathName() : TEXT("NULL"), *NetGUID.ToString() );

#if 0
		if ( Object != NULL && Object->HasAnyFlags( RF_ClassDefaultObject ) )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::SerializeObject: Received CDO: %s, %s" ), *Object->GetFullName(), *Class->GetFullName() );
			bSerializedCDO = true;
		}
#endif

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
	UE_LOG(LogNetPackageMap, VeryVerbose, TEXT("SerializeNewActor START"));

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

			check( Actor->NeedsLoadForClient() );			// We have no business sending this unless the client can load
			check( Archetype->NeedsLoadForClient() );		// We have no business sending this unless the client can load

			Location = Actor->GetRootComponent() ? Actor->GetActorLocation() : FVector::ZeroVector;
			Rotation = Actor->GetRootComponent() ? Actor->GetActorRotation() : FRotator::ZeroRotator;
			Velocity = Actor->GetVelocity();
		}

		FNetworkGUID ArchetypeNetGUID;
		SerializeObject(Ar, UObject::StaticClass(), Archetype, &ArchetypeNetGUID );

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

		if (Ar.IsLoading())
		{
			// Spawn actor if necessary (we may have already found it if it was dormant)
			if (Actor == NULL)
			{
				if (Archetype)
				{
					FActorSpawnParameters SpawnInfo;
					SpawnInfo.Template = Cast<AActor>(Archetype);
					SpawnInfo.bNoCollisionFail = true;
					SpawnInfo.bRemoteOwned = true;
					SpawnInfo.bNoFail = true;
					Actor = Connection->Driver->GetWorld()->SpawnActor(Archetype->GetClass(), &Location, &Rotation, SpawnInfo );
					Actor->PostNetReceiveVelocity(Velocity);

					AssignNetGUID(Actor, NetGUID);
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
		// Generate net guids for every subobject that has a stable name, this way they can safely be referenced over the network
		//

		// The idea here is to assume both the server and client will generate the same list when calling GetSubobjectsWithStableNamesForNetworking
		// We can take advantage of this by only sending the first GUID, and then having the client generate the same guids deterministically from there
		// The only caveat is that some guids have already been assigned before getting here (they might be referenced, and were pre-assigned)
		// To get around this, we cherry pick these guids, and send them anyhow, skipping over them when doing the range assign

		TArray< UObject * > Subobjs;

		Actor->GetSubobjectsWithStableNamesForNetworking( Subobjs );

		if ( Ar.IsSaving() )
		{
			uint32 Checksum = 0;

			uint16 NumSubobjects = Subobjs.Num();
			Ar << NumSubobjects;

			bool bRangeStartSend = false;

			for ( UObject * Obj : Subobjs )
			{
				FNetworkGUID SubobjNetGUID = Cache->NetGUIDLookup.FindRef( Obj );

				uint8 bForceSendGuid = SubobjNetGUID.IsValid() ? 1 : 0;

				if ( !bForceSendGuid )
				{
					SubobjNetGUID = InternalGetOrAssignNetGUID( Obj );
				}

				check( SubobjNetGUID.IsDynamic() );		// Make sure some assumptions are correct

				Ar.SerializeBits( &bForceSendGuid, 1 );

				if ( bForceSendGuid || !bRangeStartSend )
				{
					Ar << SubobjNetGUID;				// Save guid if forced, or if we haven't sent the start of the range

					if ( !bForceSendGuid )
					{
						// If this is the start of the range, we don't need to send it again
						bRangeStartSend = true;
					}
				}

				// Evolve checksum so we can sanity check
				Checksum = FCrc::MemCrc32( &SubobjNetGUID.Value, sizeof( SubobjNetGUID.Value ), Checksum );
				Checksum = FCrc::StrCrc32( *Obj->GetName(), Checksum );

				check( !PendingAckGUIDs.Contains( SubobjNetGUID ) );

				UE_LOG( LogNetPackageMap, Log, TEXT( "SerializeNewActor: [Saving] Assigned NetGUID <%s> to subobject %s" ), *SubobjNetGUID.ToString(), *Obj->GetPathName() );
			}

			if ( NumSubobjects > 0 )
			{
				Ar << Checksum;
			}
		}
		else
		{
			uint16 NumSubobjects = 0;
			Ar << NumSubobjects;

			if ( NumSubobjects > 0 )
			{
				FNetworkGUID LastRangedNetGUID;

				uint32 Checksum = 0;

				TArray< FNetworkGUID > SubObjGuids;

				for ( int32 i = 0; i < NumSubobjects; i++ )
				{
					FNetworkGUID LocalNetGUID;

					uint8 bForceSendGuid = false;
					Ar.SerializeBits( &bForceSendGuid, 1 );

					if ( bForceSendGuid || !LastRangedNetGUID.IsValid() )
					{
						// Server only saves out the very first guid (or guid that aren't part of the range), we derive the rest
						Ar << LocalNetGUID;
					}
					else
					{
						// Derive the next guid from the last one
						LocalNetGUID.Value = ( ( LastRangedNetGUID.Value >> 1 ) + 1 ) << 1;
					}

					if ( !bForceSendGuid )
					{
						LastRangedNetGUID = LocalNetGUID;
					}

					SubObjGuids.Add( LocalNetGUID );

					if ( i < Subobjs.Num() )
					{
						UObject * Obj = Subobjs[i];

						// Evolve checksum so we can sanity check
						Checksum = FCrc::MemCrc32( &LocalNetGUID.Value, sizeof( LocalNetGUID.Value ), Checksum );
						Checksum = FCrc::StrCrc32( *Obj->GetName(), Checksum );
					}
				}

				uint32 ServerChecksum = 0;
				Ar << ServerChecksum;

				// Sanity check the results
				if ( NumSubobjects != Subobjs.Num() )
				{
					UE_LOG( LogNetPackageMap, Error, TEXT( "SerializeNewActor: Invalid number of sub-objects: %s (%i != %i)" ), *Actor->GetFullName(), NumSubobjects, Subobjs.Num() );
				}
				else if ( Checksum != ServerChecksum )
				{
					UE_LOG( LogNetPackageMap, Error, TEXT( "SerializeNewActor: Subobject checksum FAILED: %s" ), *Actor->GetFullName() );
				}
				else
				{
					// Everything worked, assign the guids
					check( SubObjGuids.Num() == Subobjs.Num() );

					for ( int32 i = 0; i < Subobjs.Num(); i++ )
					{
						AssignNetGUID( Subobjs[i], SubObjGuids[i] );
						UE_LOG( LogNetPackageMap, Log, TEXT( "SerializeNewActor: [Loading] Assigned NetGUID <%s> to subobject %s" ), *SubObjGuids[i].ToString(), *Subobjs[i]->GetPathName() );
					}
				}
			}
		}
	}

	UE_LOG(LogNetPackageMap, Log, TEXT("SerializeNewActor END: Finished Serializing Actor %s <%s> on Channel %d"), Actor ? *Actor->GetName() : TEXT("NULL"), *NetGUID.ToString(), Channel->ChIndex );

	return SpawnedActor;
}

//--------------------------------------------------------------------
//
//	Assignment of NetGUIDs
//
//--------------------------------------------------------------------

/** Gets or assigns a new NetGUID to this object. Returns whether the object is fully mapped or not */
FNetworkGUID UPackageMapClient::InternalGetOrAssignNetGUID( const UObject * Object )
{
	if (!Object || !SupportsObject(Object))
	{
		// Null of unsupported object, leave as default NetGUID and just return mapped=true
		return FNetworkGUID();
	}

	// ----------------
	// Assign NetGUID if necessary
	// ----------------
	FNetworkGUID NetGUID = Cache->NetGUIDLookup.FindRef(Object);
	if (!NetGUID.IsValid())
	{
		NetGUID = AssignNewNetGUID(Object);
	}

	return NetGUID;
}

bool UPackageMapClient::InternalIsMapped(UObject* Object, FNetworkGUID &NetGUID)
{
	if (!Object)
	{
		// Null of unsupported object, leave as default NetGUID and just return mapped=true
		return true;
	}

	if (!NetGUID.IsValid())
	{
		// This will happen with subobjects that will replicate but who have not been assigned NetGUIDs yet.
		if (bShouldSerializeUnAckedObjects)
		{
			// We should serialize unacked objects, so just note that we did it by setting this flag to true.
			UE_LOG(LogNetPackageMap, Log, TEXT("Object %s is not supported, so unmapped"), *Object->GetName() );
			bSerializedUnAckedObject = true;
		}
		else
		{
			// We arent suppose to serialize unacked objects now, reset the serialized NetGUID
			UE_LOG(LogNetPackageMap, Log, TEXT("Object %s is not supported, so unmapped. Reseting"), *Object->GetName() );
			NetGUID.Reset();
		}
		return false;
	}

	// ----------------
	// Make sure we can serialize reference, determine Mapped
	//	-Actor is not mapped if actor channel is not ackd
	//	-Static objects are not mapped if streaming level not loaded
	// ----------------
	AActor *Actor = Cast<AActor>(Object);
	UObject *TestObj = Object;
	if (Actor == NULL && Cast<AActor>(Object->GetOuter()))
	{
		Actor = Cast<AActor>(Object->GetOuter());
		TestObj = Actor;
	}

	bool IsDynamic = IsDynamicObject(TestObj);
	bool Mapped = true;
	if (IsDynamic)
	{
		check(Actor);
		UActorChannel* Ch = Connection->ActorChannels.FindRef(Actor);
		if (Actor->bTearOff)
		{
			// Assume tear off actors are mapped, but this may not be 100% accurate.
			// We may need to track 'actors that were succesfully torn off per client' on the server.
			Mapped = true;
		}
		else if ( Ch )
		{
			// Actor has open channel - mapped if Channel is OpenAcked
			Mapped = Ch->OpenAcked;
		}
		else if (Connection->DormantActors.Contains(Actor) || Connection->RecentlyDormantActors.Contains(Actor))
		{
			// Actor is dormant - must be mapped
			Mapped = true;
		}
		else
		{
			// Actor has no channel and not dormant - its not mapped
			Mapped = false;
		}
			
		// If this is an unmapped object 
		if (!Mapped)
		{
			if (bShouldSerializeUnAckedObjects)
			{
				// We should serialize unacked objects, so just note that we did it by setting this flag to true.
				bSerializedUnAckedObject = true;
			}
			else
			{
				// We arent suppose to serialize unacked objects now, reset the serialized NetGUID
				NetGUID.Reset();
			}
		}

		// If this is an actor component whose channel has not been fully ack'd
		if (TestObj != Object && !Mapped)
		{
			if (NetGUID.IsStatic() && !NetGUIDHasBeenAckd(NetGUID))
			{
				// We cannot serialize a reference to this until the channel has been fully ack'd.
				//	We can't allow a reference like this to go out since we can't garuntee the exported
				//	NetGUID bunch containing it will be handled after the Actor Bunch that contains the parent actor.
				//	Everything in the NetGUID bunch must, 100%, must be resolved on client side, otherwise the NetGUID'
				//	ACKing system would not work. PackageMapClient assumes that if the NetGUID bunch packet ID is ACK'd, all
				//	NetGUIDs inside of it are ACK'd.
				NetGUID.Reset();
			}
		}
	}
	else if(Connection->Driver->IsServer() && !Connection->ClientHasInitializedLevelFor(Object))
	{
		// Static actor the client has not initialized its level yet, always serialize these as NULL
		NetGUID.Reset();
		Mapped = false;
	}

	return Mapped;
}

/**	
 *	Finds objects from PathName and ObjOuter, and assigns it a NetGUID. Called on the client after serialziing a
 *	<NetGUID, Path> pair.
 */
UObject * UPackageMapClient::ResolvePathAndAssignNetGUID( FNetworkGUID & InOutNetGUID, FString PathName, UObject *ObjOuter)
{
	return ResolvePathAndAssignNetGUID( InOutNetGUID, PathName, ObjOuter, false, false );
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
			uint8 bHasPath		: 1;
			uint8 bIsLevel		: 1;
			uint8 bIsPackage	: 1;
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

		ExportFlags.bIsLevel	= Cast< const ULevel >( Object ) != NULL ? 1 : 0;
		ExportFlags.bIsPackage	= Cast< const UPackage >( Object ) != NULL ? 1 : 0;

		Ar << ExportFlags.Value;
	}

	if ( ExportFlags.bHasPath )
	{
		InternalWriteObjectPath( Ar, NetGUID, Object, ObjectPathName, ObjectOuter );
	}
	else
	{
		UE_CLOG(!bSuppressLogs, LogNetPackageMap, Log, TEXT("UPackageMapClient::InternalWriteObjectPath Serialized ObjectPathName %s as <%s>)"), *ObjectPathName, *NetGUID.ToString() );
	}
}

/** Actually writes the ObjectPath, given a FString ObjectPathName and Outer* */
void UPackageMapClient::InternalWriteObjectPath( FArchive & Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject * ObjectOuter )
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
	FNetworkGUID OuterNetGUID = InternalGetOrAssignNetGUID( ObjectOuter );

	InternalWriteObject( Ar, OuterNetGUID, ObjectOuter, TEXT( "" ), NULL );

	GEngine->NetworkRemapPath(Connection->Driver->GetWorld(), ObjectPathName, false);

	// Serialize Name of object
	Ar << ObjectPathName;

	if ( IsExportingNetGUIDBunch )
	{
		CurrentExportNetGUIDs.Add( NetGUID );

		int32& Count = NetGUIDExportCountMap.FindOrAdd(NetGUID);
		Count++;
	}

	UE_CLOG(!bSuppressLogs, LogNetPackageMap, Log, TEXT("UPackageMapClient::InternalWriteObjectPath Serialized Object %s as <%s,%s>)"), *ObjectPathName, *NetGUID.ToString(), *ObjectPathName );
}

//--------------------------------------------------------------------
//
//	Loading
//
//--------------------------------------------------------------------

static void SanityCheckExport( const UObject * Object, const FNetworkGUID & NetGUID, const FString & ExpectedPathName, const UObject * ExpectedOuter )
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
}

/** Loads a UObject from an FArchive stream. Reads object path if there, and tries to load object if its not already loaded */
FNetworkGUID UPackageMapClient::InternalLoadObject( FArchive & Ar, UObject *& Object, const bool bIsOuterLevel, const int InternalLoadObjectRecursionCount )
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

		FNetworkGUID OuterGUID = InternalLoadObject( Ar, ObjOuter, ExportFlags.bIsLevel || bIsOuterLevel, InternalLoadObjectRecursionCount + 1 );

		FString PathName;
		Ar << PathName;

		if ( Ar.IsError() )
		{
			UE_LOG( LogNetPackageMap, Error, TEXT( "InternalLoadObject: Failed to load path name" ) );
			Object = NULL;
			return NetGUID;
		}

		GEngine->NetworkRemapPath( Connection->Driver->GetWorld(), PathName, true );

		if ( Object == NULL )
		{
			// Try to resolve the name (we may have already loaded it), but DON'T load yet if we haven't already
			Object = ResolvePathAndAssignNetGUID( NetGUID, PathName, OuterGUID, ExportFlags.bIsPackage, false );
		}

		if ( Object != NULL )
		{
			// If we already have the object, just do some sanity checking and return
			SanityCheckExport( Object, NetGUID, PathName, ObjOuter );
			return NetGUID;
		}

		// If this is a level or our outer is being deferred, then defer this guid
		const bool bCanDefer = false;//!NetGUID.IsDefault() && ( ExportFlags.bIsLevel || bIsOuterLevel || DeferredResolvePaths.Contains( OuterGUID ) );

		if ( bCanDefer )
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "InternalLoadObject: Deferring resolve. Path: %s, NetGUID: %s, Outer: %s" ), *PathName, *NetGUID.ToString(), ObjOuter != NULL ? *ObjOuter->GetFullName() : TEXT( "NULL" ) );
			ResolvePathAndAssignNetGUID_Deferred( NetGUID, PathName, OuterGUID, ExportFlags.bIsPackage );
		}
		else
		{
			Object = ResolvePathAndAssignNetGUID( NetGUID, PathName, OuterGUID, ExportFlags.bIsPackage, false );
		}
	}

	if ( Object == NULL && NetGUID.IsStatic() && !DeferredResolvePaths.Contains( NetGUID ) )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "InternalLoadObject: Unable to resolve object. NetGUID: %s, HasPath: %s" ), *NetGUID.ToString(), ExportFlags.bHasPath ? TEXT( "TRUE" ) : TEXT( "FALSE" ) );
	}

	return NetGUID;
}

void UPackageMapClient::ResolvePathAndAssignNetGUID_Deferred( const FNetworkGUID & NetGUID, const FString & PathName, const FNetworkGUID & OuterGUID, const bool bIsPackage )
{
	check( !NetGUID.IsDefault() );		// We can't defer default guids, doesn't make sense

	if ( !DeferredResolvePaths.Contains( NetGUID ) )
	{
		FDeferredResolvePath DeferredResolvePath;

		DeferredResolvePath.OuterGUID	= OuterGUID;
		DeferredResolvePath.PathName	= FName( *PathName );

		DeferredResolvePaths.Add( NetGUID, DeferredResolvePath );
	}
	else
	{
		// Sanity check
		FDeferredResolvePath & DeferredResolvePath = DeferredResolvePaths.FindChecked( NetGUID );

		if ( DeferredResolvePath.OuterGUID != OuterGUID )
		{
			UE_LOG( LogNetPackageMap, Warning, TEXT( "ResolvePathAndAssignNetGUID_Deferred: Deferred Outer mismatch. NetGUID: %s, Path: %s, Current: %s, Expected: %s" ), *NetGUID.ToString(), *PathName, *DeferredResolvePath.OuterGUID.ToString(), *OuterGUID.ToString() );
		}
	}
}

UObject * UPackageMapClient::ResolvePathAndAssignNetGUID( FNetworkGUID & InOutNetGUID, const FString & PathName, UObject * ObjOuter, const bool bIsPackage, const bool bNoLoad )
{
	if ( InOutNetGUID.IsDynamic() )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::ResolvePathAndAssignNetGUID. Trying to load dynamic NetGUID by path: <%s,%s>. Outer: %s" ), *InOutNetGUID.ToString(), *PathName, ObjOuter ? *ObjOuter->GetName() : TEXT( "NULL" ) );
	}

	// We don't have the object mapped yet
	UObject * Object = StaticFindObject( UObject::StaticClass(), ObjOuter, *PathName, false );

	if ( !Object && !bNoLoad )
	{
		if ( IsNetGUIDAuthority() )
		{
			// The authority shouldn't be loading resources on demand, at least for now.
			// This could be garbage or a security risk
			// Another possibility is in dynamic property replication if the server reads the previously serialized state
			// that has a now destroyed actor in it.
			UE_LOG( LogNetPackageMap, Warning, TEXT( "Could not find Object for: NetGUID <%s, %s> (and IsNetGUIDAuthority())" ), *InOutNetGUID.ToString(), *PathName );
			return NULL;
		}
		else
		{
			UE_LOG( LogNetPackageMap, Log, TEXT( "Could not find Object for: NetGUID <%s, %s>" ), *InOutNetGUID.ToString(), *PathName );
		}

		Object = StaticLoadObject( UObject::StaticClass(), ObjOuter, *PathName, NULL, LOAD_NoWarn );

		if ( Object )
		{
			UE_LOG(LogNetPackageMap, Log, TEXT("  StaticLoadObject. Found: %s" ), Object ? *Object->GetName() : TEXT("NULL") );
		}
		else
		{
			if ( bIsPackage )
			{
				Object = LoadPackage( Cast< UPackage >( ObjOuter ), *PathName, LOAD_None );
				UE_LOG( LogNetPackageMap, Log, TEXT( "UPackageMapClient::ResolvePathAndAssignNetGUID  LoadPackage %s. Found: %s" ), *PathName, Object ? *Object->GetName() : TEXT( "NULL" ) );
			}
			else
			{
				UE_LOG( LogNetPackageMap, Warning, TEXT( "Outer %s is not a package for object %s" ), ObjOuter ? *ObjOuter->GetName() : TEXT( "NULL" ), *PathName );
			}
		}
	}

	if ( Object )
	{
		InOutNetGUID = AssignNetGUID( Object, InOutNetGUID );
	}

	return Object;
}

UObject * UPackageMapClient::ResolvePathAndAssignNetGUID( FNetworkGUID & InOutNetGUID, const FString & PathName, const FNetworkGUID & OuterGUID, const bool bIsPackage, const bool bNoLoad )
{
	UObject * ObjOuter = OuterGUID.IsValid() ? GetObjectFromNetGUID( OuterGUID ) : NULL;

	if ( OuterGUID.IsValid() && ObjOuter == NULL )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "ResolvePathAndAssignNetGUID: Unable resolve outer. Path: %s, NetGUID: %s, OuterGUID: %s " ), *PathName, *InOutNetGUID.ToString(), *OuterGUID.ToString() );
		return NULL;
	}

	UObject * Object = ResolvePathAndAssignNetGUID( InOutNetGUID, PathName, ObjOuter, bIsPackage, bNoLoad );

	if ( Object == NULL )
	{
		UE_LOG( LogNetPackageMap, Warning, TEXT( "ResolvePathAndAssignNetGUID: Unable to resolve object. Path: %s, NetGUID: %s, Outer: %s" ), *PathName, *InOutNetGUID.ToString(), ObjOuter != NULL ? *ObjOuter->GetPathName() : TEXT( "NULL" ) );
	}

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
		UObject *Obj;
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

	FNetworkGUID NetGUID = InternalGetOrAssignNetGUID( Obj );

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
 *	Dynamic objects are actors that were spawned in the world at run time, and therefor cannot be
 *	referenced with a path name to the client.
 */
bool UPackageMapClient::IsDynamicObject(const UObject* Object)
{
#if USE_NEW_DYNAMIC_AND_SUPPORT_CHECKS == 1
	check( Object != NULL );
	check( Object->IsSupportedForNetworking() );

	// Any non net addressable object is dynamic
	return !Object->IsFullNameStableForNetworking();
#else
	const AActor * Actor = Cast<const AActor>(Object);
	bool NonAddressableSubobject = false;
	if (!Actor)
	{
		Actor = Cast<AActor>(Object->GetOuter());
		if (Actor)
		{
			const UActorComponent* ActorComponent = Cast<const UActorComponent>(Object);
			if (ActorComponent)
			{
				// Actor compnents define if they are NetAddressable via UActorComponent::IsNameStableForNetworking()
				if (!ActorComponent->IsNameStableForNetworking())
				{
					NonAddressableSubobject  = true;
				}
			}
			// Non actor component subobjects are net addressable if they are default subobjects or RF_WasLoaded
			else if (!Object->IsDefaultSubobject() && !Object->HasAnyFlags(RF_WasLoaded))
			{
				NonAddressableSubobject = true;
			}
		}
	}
	
	if (!Actor || Actor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || (Actor->IsNetStartupActor() && !NonAddressableSubobject))
	{
		return false;
	}

	return true;
#endif
}

/**
 *	Prints debug info about this package map's state
 */
void UPackageMapClient::LogDebugInfo(FOutputDevice& Ar)
{
	for (auto It = Cache->NetGUIDLookup.CreateIterator(); It; ++It)
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
		FString Str = FString::Printf(TEXT("%s [%s] [%s] - %s"), *NetGUID.ToString(), *Status, IsDynamicObject(Obj) ? TEXT("Dynamic") : TEXT("Static") , Obj ? *Obj->GetPathName() : TEXT("NULL"));
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
	return !(Connection && Connection->Driver && Connection->Driver->ServerConnection == Connection);
}

bool UPackageMapClient::SupportsObject( const UObject* Object )
{
#if USE_NEW_DYNAMIC_AND_SUPPORT_CHECKS == 1
	// NULL is always supported
	if ( !Object )
	{
		return true;
	}

	// If we already gave it a NetGUID, its supported.
	// This should happen for dynamic subobjects.
	FNetworkGUID NetGUID = Cache->NetGUIDLookup.FindRef(Object);
	if (NetGUID.IsValid())
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

	UE_LOG( LogNetPackageMap, Warning, TEXT( "UPackageMapClient::SupportsObject: %s NOT Supported." ), *Object->GetFullName() );
	UE_LOG( LogNetPackageMap, Warning, TEXT( "   %s"), *DebugContextString );

	return false;
#else
	// NULL is always supported
	if (!Object)
	{
		return true;
	}

	// Non actor UObjects
	if (Cast<const AActor>(Object) == NULL && Cast<const ULevel>(Object) == NULL )
	{
		// If we already gave it a NetGUID, its supported.
		// This should happen for dynamic subobjects.
		FNetworkGUID NetGUID = Cache->NetGUIDLookup.FindRef(Object);
		if (NetGUID.IsValid())
		{
			return true;
		}
		
		// Some special rules for actor components
		const UActorComponent *ActorComponent = Cast<const UActorComponent>(Object);
		if (ActorComponent)
		{
			// NetAddressable actor components are supported even before they have a NetGUID
			// So things like an actor's static mesh component can be referenced even if the SMC doesn't have replicated properties itself
			if (ActorComponent->IsNameStableForNetworking())
			{
				return true;
			}

			// This case handles a dynamically spawned actor component who doesn't have a NetGUID yet, but probably will have one soon.
			// For now, it is unsupported. Once it is assigned a netguid by the owning actor channel, it will be supported.
			if (ActorComponent->GetIsReplicated())
			{
				// This is ok. Don't warn.
				return false;
			}
		}

		// Everything else
		UObject * ThisOuter = Object->GetOuter();
		while(ThisOuter)
		{
			// If the UObject lives in a world, we can only serialize it if it was loaded (not if it was dynamically spawned)
			if (Cast<UWorld>(ThisOuter))
			{
				// This object lives in a world, it can only be supported if RF_WasLoaded is true
				if (Object->HasAnyFlags(RF_WasLoaded) == false)
				{
					UE_LOG(LogNetPackageMap, Warning, TEXT("UPackageMapClient::SupportsObject %s NOT Supported.Not RF_WasLoaded. NetGUID: <%s>"), *Object->GetPathName(), *NetGUID.ToString() );
					UE_LOG(LogNetPackageMap, Warning, TEXT("   %s"), *DebugContextString);
					return false;
				}

				break;
			}
			ThisOuter = ThisOuter->GetOuter();
		}
	}

	// Everything else is now supported
	return true;
#endif
}

/** Resets packagemap state for server travel */
void UPackageMapClient::ResetPackageMap()
{
	UPackageMap::ResetPackageMap();
	
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
	for (auto It = Cache->History.CreateIterator(); It; ++It)
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
