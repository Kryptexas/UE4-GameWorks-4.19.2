// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * PackageMap implementation that is client/connection specific. This subclass implements all NetGUID Acking and interactions with a UConnection.
 *	On the server, each client will have their own instance of UPackageMapClient.
 *
 *	UObjects are first serialized as <NetGUID, Name/Path> pairs. UPackageMapClient tracks each NetGUID's usage and knows when a NetGUID has been ACKd.
 *	Once ACK'd, objects are just serialized as <NetGUID>. The result is higher bandwidth usage upfront for new clients, but minimal bandwidth once
 *	things gets going.
 *
 *	A further optimization is enabled by default with the SerializeOuter variable. When true, references will actualy be serialized via:
 *	<NetGUID, <(Outer *), Object Name> >. Where Outer * is a reference to the UObject's Outer.
 *
 *	The main advantages from this are:
 *		-Flexibility. No precomputed net indices or large package lists need to be exchanged for UObject serialization.
 *		-Cross version communication. The name is all that is needed to exchange references.
 *		-Effeciency in that a very small % of UObjects will ever be serialized. Only Objects that serialized are assigned NetGUIDs.
 */

#pragma once
#include "Net/DataBunch.h"
#include "PackageMapClient.generated.h"

class FDeferredResolvePath
{
public:
	FDeferredResolvePath() {}

	FName			PathName;
	FString			FilenameOverride;
	FNetworkGUID	OuterGUID;
};

/** Stores an object with path associated with FNetworkGUID */
class FNetGuidCacheObject
{
public:
	TWeakObjectPtr< UObject >		Object;
	FString							FullPath;
};

class ENGINE_API FNetGUIDCache
{
public:
	FNetGUIDCache( UNetDriver * InDriver );

	void			Reset();
	void			CleanReferences();
	bool			SupportsObject( const UObject * Object );
	bool			IsDynamicObject( const UObject * Object );
	bool			IsNetGUIDAuthority();
	FNetworkGUID	GetOrAssignNetGUID( const UObject * Object );
	FNetworkGUID	AssignNewNetGUID( const UObject * Object );
	void			AssignNetGUID( const UObject * Object, const FNetworkGUID & NewNetworkGUID );
	UObject *		GetObjectFromNetGUID( const FNetworkGUID & NetGUID );

	TMap< FNetworkGUID, FNetGuidCacheObject >		ObjectLookup;
	TMap< TWeakObjectPtr< UObject >, FNetworkGUID >	NetGUIDLookup;
	int32											UniqueNetIDs[2];

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
		GuidCache = InNetGUIDCache;
		SerializeOuter = true;
		Locked = false;
		ExportNetGUIDCount = 0;
		IsExportingNetGUIDBunch = false;
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
	virtual bool SerializeObject( FArchive& Ar, UClass* Class, UObject*& Obj, FNetworkGUID *OutNetGUID = NULL ) OVERRIDE;
	virtual bool SerializeNewActor( FArchive& Ar, class UActorChannel *Channel, class AActor*& Actor) OVERRIDE;
	
	virtual bool WriteObject( FArchive& Ar, UObject* Outer, FNetworkGUID NetGUID, FString ObjName ) OVERRIDE;

	virtual void ResetPackageMap() OVERRIDE;

	virtual void SetLocked(bool L) { Locked = L; }

	// UPackageMapClient Connection specific methods

	virtual bool NetGUIDHasBeenAckd(FNetworkGUID NetGUID) OVERRIDE;

	virtual void ReceivedNak( const int32 NakPacketId ) OVERRIDE;
	virtual void ReceivedAck( const int32 AckPacketId ) OVERRIDE;
	virtual void NotifyBunchCommit( const int32 OutPacketId, const TArray< FNetworkGUID > & ExportNetGUIDs ) OVERRIDE;
	virtual void GetNetGUIDStats(int32 &AckCount, int32 &UnAckCount, int32 &PendingCount) OVERRIDE;

	void ReceiveNetGUIDBunch( FInBunch &InBunch );
	bool AppendExportBunches(TArray<FOutBunch *>& OutgoingBunches);

	TMap<FNetworkGUID, int32>	NetGUIDExportCountMap;	// How many times we've exported each NetGUID on this connection. Public for ListNetGUIDExports 

	void HandleUnAssignedObject( const UObject * Obj );

	static void	AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	virtual void NotifyStreamingLevelUnload(UObject* UnloadedLevel) OVERRIDE;

	virtual bool PrintExportBatch() OVERRIDE;

	virtual void		LogDebugInfo( FOutputDevice & Ar) OVERRIDE;
	virtual UObject *	GetObjectFromNetGUID( const FNetworkGUID & NetGUID ) OVERRIDE;

protected:

	bool	ExportNetGUID( FNetworkGUID NetGUID, const UObject * Object, FString PathName, UObject * ObjOuter );
	void	ExportNetGUIDHeader();

	void			InternalWriteObject( FArchive& Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject * ObjectOuter );	
	FNetworkGUID	InternalLoadObject( FArchive & Ar, UObject *& Object, const bool bIsOuterLevel, int InternalLoadObjectRecursionCount );

	void		ResolvePathAndAssignNetGUID_Deferred( const FNetworkGUID & NetGUID, const FString & PathName, const FString & FilenameOverride, const FNetworkGUID & OuterGUID );
	UObject *	ResolvePathAndAssignNetGUID( FNetworkGUID & InOutNetGUID, const FString & PathName, const FString & FilenameOverride, const FNetworkGUID & OuterGUID, const bool bNoLoad = false );

	virtual UObject * ResolvePathAndAssignNetGUID( FNetworkGUID & InOutNetGUID, const FString & PathName, const FString & FilenameOverride, UObject * ObjOuter, const bool bNoLoad = false ) OVERRIDE;

	bool	ShouldSendFullPath(const UObject* Object, const FNetworkGUID &NetGUID);
	
	bool	IsNetGUIDAuthority();

	bool Locked;
	bool IsExportingNetGUIDBunch;

	class UNetConnection* Connection;

	bool ObjectLevelHasFinishedLoading(UObject* Obj);

	TSet< FNetworkGUID >			CurrentExportNetGUIDs;				// Current list of NetGUIDs being written to the Export Bunch.

	TMap< FNetworkGUID, int32 >		NetGUIDAckStatus;
	TArray< FNetworkGUID >			PendingAckGUIDs;					// Quick access to all GUID's that haven't been acked

	// -----------------

	
	// Enables optimization to send <outer reference + Name> instead of <full path name> when serializing unackd objects
	bool							SerializeOuter;

	// Bunches of NetGUID/path tables to send with the current content bunch
	TArray<FOutBunch* >				ExportBunches;
	FOutBunch *						CurrentExportBunch;

	int32							ExportNetGUIDCount;

	TMap< FNetworkGUID, FDeferredResolvePath >	DeferredResolvePaths;

	TSharedPtr< FNetGUIDCache >		GuidCache;
};
