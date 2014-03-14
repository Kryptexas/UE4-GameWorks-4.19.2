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
#include "PackageMapClient.generated.h"

UCLASS(transient)
class UPackageMapClient : public UPackageMap
{
    GENERATED_UCLASS_BODY()

	UPackageMapClient( const class FPostConstructInitializeProperties& PCIP, UNetConnection *InConnection, FNetObjectIsDynamic Delegate, UNetGUIDCache *InNetGUIDCache )
	: UPackageMap(PCIP, Delegate)
	, Connection(InConnection)
	{
		Cache = InNetGUIDCache;
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
	virtual void LogDebugInfo(FOutputDevice& Ar) OVERRIDE;
	virtual bool SupportsObject( const UObject* Object ) OVERRIDE;
	
	virtual UObject * NetGUIDAssign(FNetworkGUID NetGUID, FString Path, UObject *ObjOuter = NULL) OVERRIDE;

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

	virtual void HandleUnAssignedObject(const UObject* Obj) OVERRIDE;

	static void	AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	virtual void NotifyStreamingLevelUnload(UObject* UnloadedLevel) OVERRIDE;

	virtual bool PrintExportBatch() OVERRIDE;

protected:

	bool	ExportNetGUID( FNetworkGUID NetGUID, const UObject * Object, FString PathName, UObject * ObjOuter );
	void	ExportNetGUIDHeader();

	void	InternalGetOrAssignNetGUID(UObject* Obj, FNetworkGUID &NetGUID);
	bool	InternalIsMapped(UObject* Obj, FNetworkGUID &NetGUID);

	void	InternalWriteObject( FArchive& Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject* ObjectOuter );	
	void	InternalWriteObjectPath( FArchive& Ar, FNetworkGUID NetGUID, const UObject * Object, FString ObjectPathName, UObject* ObjectOuter );

	FNetworkGUID	InternalLoadObject( FArchive& Ar, UObject*& Object);
	bool			InternalLoadObjectPath( FArchive& Ar, UObject*& Object, FNetworkGUID NetGUID);

	bool	ShouldSendFullPath(const UObject* Object, const FNetworkGUID &NetGUID);

	virtual bool IsNetGUIDAuthority() OVERRIDE;

	bool Locked;
	bool IsExportingNetGUIDBunch;

	class UNetConnection* Connection;

	bool ObjectLevelHasFinishedLoading(UObject* Obj);

	virtual bool IsDynamicObject(const UObject* Object) OVERRIDE;

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
};
