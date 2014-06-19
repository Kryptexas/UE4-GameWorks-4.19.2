// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * PackageMap implementation that is client/connection specific. This subclass implements all NetGUID Acking and interactions with a UConnection.
 *	On the server, each client will have their own instance of UPackageMapClient.
 *
 *	UObjects are first serialized as <NetGUID, Name/Path> pairs. UPackageMapClient tracks each NetGUID's usage and knows when a NetGUID has been ACKd.
 *	Once ACK'd, objects are just serialized as <NetGUID>. The result is higher bandwidth usage upfront for new clients, but minimal bandwidth once
 *	things gets going.
 *
 *	A further optimization is enabled by default. References will actually be serialized via:
 *	<NetGUID, <(Outer *), Object Name> >. Where Outer * is a reference to the UObject's Outer.
 *
 *	The main advantages from this are:
 *		-Flexibility. No precomputed net indices or large package lists need to be exchanged for UObject serialization.
 *		-Cross version communication. The name is all that is needed to exchange references.
 *		-Efficiency in that a very small % of UObjects will ever be serialized. Only Objects that serialized are assigned NetGUIDs.
 */

#pragma once
#include "Net/DataBunch.h"
#include "PackageMapClient.generated.h"

class FMarkSerializeNewActorScoped
{
public:
	FMarkSerializeNewActorScoped( bool & InIsSerializingNewActor ) : IsSerializingNewActorRef( InIsSerializingNewActor )
	{ 
		IsSerializingNewActorRef = true; 
	};

	~FMarkSerializeNewActorScoped()
	{
		IsSerializingNewActorRef = false;
	}

	bool & IsSerializingNewActorRef;
};
	
/** Stores an object with path associated with FNetworkGUID */
class FNetGuidCacheObject
{
public:
	FNetGuidCacheObject() : GuidSequence( 0 ), bNoLoad( 0 ), bIgnoreWhenMissing( 0 ), bIsPending( 0 ), bIsBroken( 0 )
	{
	}

	TWeakObjectPtr< UObject >	Object;

	// These fields are set when this guid is static
	FNetworkGUID				OuterGUID;
	FName						PathName;
	int32						GuidSequence;				// We remember the guid sequence this net guid was generated on so we can tell how old it is

	uint8						bNoLoad				: 1;	// Don't load this, only do a find
	uint8						bIgnoreWhenMissing	: 1;	// Don't warn when this asset can't be found or loaded
	uint8						bIsPending			: 1;	// This object is waiting to be fully loaded
	uint8						bIsBroken			: 1;	// If this object failed to load, then we set this to signify that we should stop trying
};

class ENGINE_API FNetGUIDCache
{
public:
	FNetGUIDCache( UNetDriver * InDriver );

	void			CleanReferences();
	bool			SupportsObject( const UObject * Object );
	bool			IsDynamicObject( const UObject * Object );
	bool			IsNetGUIDAuthority();
	FNetworkGUID	GetOrAssignNetGUID( const UObject * Object );
	FNetworkGUID	AssignNewNetGUID_Server( const UObject * Object );
	void			RegisterNetGUID_Internal( const FNetworkGUID & NetGUID, const FNetGuidCacheObject & CacheObject );
	void			RegisterNetGUID_Server( const FNetworkGUID & NetGUID, const UObject * Object );
	void			RegisterNetGUID_Client( const FNetworkGUID & NetGUID, const UObject * Object );
	void			RegisterNetGUIDFromPath_Client( const FNetworkGUID & NetGUID, const FString & PathName, const FNetworkGUID & OuterGUID, const bool bNoLoad, const bool bIgnoreWhenMissing );
	UObject *		GetObjectFromNetGUID( const FNetworkGUID & NetGUID );
	bool			ShouldIgnoreWhenMissing( const FNetworkGUID & NetGUID );
	UObject *		ResolvePath( const FString & PathName, UObject * ObjOuter, const bool bNoLoad );

	TMap< FNetworkGUID, FNetGuidCacheObject >		ObjectLookup;
	TMap< TWeakObjectPtr< UObject >, FNetworkGUID >	NetGUIDLookup;
	int32											UniqueNetIDs[2];

	int32											GuidSequence;

	UNetDriver *									Driver;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// History for debugging entries in the guid cache
	TMap<FNetworkGUID, FString>						History;
#endif
};

UCLASS(transient)
class UPackageMapClient : public UPackageMap
{
    GENERATED_UCLASS_BODY()

	UPackageMapClient( const class FPostConstructInitializeProperties & PCIP, UNetConnection * InConnection, TSharedPtr< FNetGUIDCache > InNetGUIDCache ) : 
		UPackageMap( PCIP )
	,	Connection(InConnection)
	{
		GuidCache				= InNetGUIDCache;
		ExportNetGUIDCount		= 0;
		IsExportingNetGUIDBunch = false;
		IsSerializingNewActor	= false;
	}

	virtual ~UPackageMapClient()
	{
		if (CurrentExportBunch)
		{
			delete CurrentExportBunch;
			CurrentExportBunch = NULL;
		}
	}

	
	// UPackageMap Interface
	virtual bool SerializeObject( FArchive& Ar, UClass* Class, UObject*& Obj, FNetworkGUID *OutNetGUID = NULL ) override;
	virtual bool SerializeNewActor( FArchive& Ar, class UActorChannel *Channel, class AActor*& Actor) override;
	
	virtual bool WriteObject( FArchive& Ar, UObject* Outer, FNetworkGUID NetGUID, FString ObjName ) override;

	// UPackageMapClient Connection specific methods

	bool NetGUIDHasBeenAckd(FNetworkGUID NetGUID);

	virtual void ReceivedNak( const int32 NakPacketId ) override;
	virtual void ReceivedAck( const int32 AckPacketId ) override;
	virtual void NotifyBunchCommit( const int32 OutPacketId, const TArray< FNetworkGUID > & ExportNetGUIDs ) override;
	virtual void GetNetGUIDStats(int32 &AckCount, int32 &UnAckCount, int32 &PendingCount) override;

	void ReceiveNetGUIDBunch( FInBunch &InBunch );
	bool AppendExportBunches(TArray<FOutBunch *>& OutgoingBunches);

	TMap<FNetworkGUID, int32>	NetGUIDExportCountMap;	// How many times we've exported each NetGUID on this connection. Public for ListNetGUIDExports 

	void HandleUnAssignedObject( const UObject * Obj );

	static void	AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	virtual void NotifyStreamingLevelUnload(UObject* UnloadedLevel) override;

	virtual bool PrintExportBatch() override;

	virtual void		LogDebugInfo( FOutputDevice & Ar) override;
	virtual UObject *	GetObjectFromNetGUID( const FNetworkGUID & NetGUID ) override;

protected:

	bool	ExportNetGUID( FNetworkGUID NetGUID, const UObject * Object, FString PathName, UObject * ObjOuter );
	void	ExportNetGUIDHeader();

	void			InternalWriteObject( FArchive& Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject * ObjectOuter );	
	FNetworkGUID	InternalLoadObject( FArchive & Ar, UObject *& Object, const bool bIsInnerLevel, int InternalLoadObjectRecursionCount );

	virtual UObject * ResolvePathAndAssignNetGUID( const FNetworkGUID & NetGUID, const FString & PathName ) override;

	bool	ShouldSendFullPath(const UObject* Object, const FNetworkGUID &NetGUID);
	
	bool IsNetGUIDAuthority();

	bool IsExportingNetGUIDBunch;

	class UNetConnection* Connection;

	bool ObjectLevelHasFinishedLoading(UObject* Obj);

	TSet< FNetworkGUID >				CurrentExportNetGUIDs;				// Current list of NetGUIDs being written to the Export Bunch.

	TMap< FNetworkGUID, int32 >			NetGUIDAckStatus;
	TArray< FNetworkGUID >				PendingAckGUIDs;					// Quick access to all GUID's that haven't been acked

	// Bunches of NetGUID/path tables to send with the current content bunch
	TArray<FOutBunch* >					ExportBunches;
	FOutBunch *							CurrentExportBunch;

	int32								ExportNetGUIDCount;

	TSharedPtr< FNetGUIDCache >			GuidCache;

	bool								IsSerializingNewActor;
};
