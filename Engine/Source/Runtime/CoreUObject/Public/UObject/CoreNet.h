// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CoreNet.h: Core networking support.
=============================================================================*/

#pragma once

#include "ObjectBase.h"

DECLARE_DELEGATE_RetVal_OneParam( bool, FNetObjectIsDynamic, const UObject*);

//
// Information about a field.
//
class FFieldNetCache
{
public:
	UField* Field;
	int32 FieldNetIndex;
	FFieldNetCache()
	{}
	FFieldNetCache( UField* InField, int32 InFieldNetIndex )
	: Field(InField), FieldNetIndex(InFieldNetIndex)
	{}
};

//
// Information about a class, cached for network coordination.
//
class FClassNetCache
{
	friend class UPackageMap;
public:
	FClassNetCache();
	FClassNetCache( UClass* Class );
	int32 GetMaxIndex()
	{
		return FieldsBase+Fields.Num();
	}
	FFieldNetCache* GetFromField( UObject* Field )
	{
		FFieldNetCache* Result=NULL;
		for( FClassNetCache* C=this; C; C=C->Super )
		{
			if( (Result=C->FieldMap.FindRef(Field))!=NULL )
			{
				break;
			}
		}
		return Result;
	}
	FFieldNetCache* GetFromIndex( int32 Index )
	{
		for( FClassNetCache* C=this; C; C=C->Super )
		{
			if( Index>=C->FieldsBase && Index<C->FieldsBase+C->Fields.Num() )
			{
				return &C->Fields[Index-C->FieldsBase];
			}
		}
		return NULL;
	}
private:
	int32 FieldsBase;
	FClassNetCache* Super;
	UClass* Class;
	TArray<FFieldNetCache> Fields;
	TMap<UObject*,FFieldNetCache*> FieldMap;
};

//
// Ordered information of linker file requirements.
//
class COREUOBJECT_API FPackageInfo
{
public:
	// Variables.
	FName			PackageName;		// name of the package we need to request.
	UPackage*		Parent;				// The parent package.
	FGuid			Guid;				// Package identifier.
	int32				ObjectBase;			// Net index of first object.
	int32				ObjectCount;		// Number of objects, defined by server.
	int32				LocalGeneration;	// This machine's generation of the package.
	int32				RemoteGeneration;	// Remote machine's generation of the package.
	uint32			PackageFlags;		// Package flags.
	FName			ForcedExportBasePackageName; // for packages that were a forced export in another package (seekfree loading), the name of that base package, otherwise NAME_None
	uint8			LoadingPhase;		// indicates if package was loaded during a seamless loading operation (e.g. seamless level change) to aid client in determining when to process it
	FString			Extension;			// Extension of the package file, used so HTTP downloading can get the package
	FName			FileName;			// Name of the file this package was loaded from if it is not equal to the PackageName

	// Functions.
	FPackageInfo(UPackage* Package = NULL);
	friend COREUOBJECT_API FArchive& operator<<( FArchive& Ar, FPackageInfo& I );
};

/** maximum index that may ever be used to serialize an object over the network
 * this affects the number of bits used to send object references
 */
#define MAX_OBJECT_INDEX (uint32(1) << 31)

/** Stores an object with path associated with FNetworkGUID */
class FNetGuidCacheObject
{
public:
	TWeakObjectPtr< UObject >		Object;
	FString							FullPath;
};

class COREUOBJECT_API UNetGUIDCache : public UObject
{
	DECLARE_CLASS_INTRINSIC(UNetGUIDCache,UObject,CLASS_Transient|0,CoreUObject);

	virtual void PostInitProperties();

	// Fixme: use a single data structure for this
	TMap<FNetworkGUID, FNetGuidCacheObject >		ObjectLookup;
	TMap<TWeakObjectPtr<UObject>, FNetworkGUID>		NetGUIDLookup;

	int32	UniqueNetIDs[2];

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// History for debugging entries in the packagemap
	TMap<FNetworkGUID, FString>	History;
#endif
};


//
// Maps objects and names to and from indices for network communication.
//
class COREUOBJECT_API UPackageMap : public UObject
{
	DECLARE_CLASS_INTRINSIC(UPackageMap,UObject,CLASS_Transient|0,CoreUObject);

	
	UPackageMap( const class FPostConstructInitializeProperties& PCIP, FNetObjectIsDynamic Delegate ) :
		UObject(PCIP), ObjectIsDynamicDelegate(Delegate)
	{
	}

	// Begin UObject interface.
	virtual void	PostInitProperties();
	virtual void	Serialize( FArchive& Ar ) OVERRIDE;
	virtual void	FinishDestroy() OVERRIDE;
	static void		AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.
	
	// @todo document
	virtual bool CanSerializeObject(const UObject* Obj);
	
	// @todo document
	virtual bool SupportsPackage( UObject* InOuter );

	// @todo document
	virtual bool SupportsObject( const UObject* Obj );

	virtual bool WriteObject( FArchive& Ar, UObject* Outer, FNetworkGUID NetGUID, FString ObjName );

	// @todo document
	virtual bool SerializeObject( FArchive& Ar, UClass* Class, UObject*& Obj, FNetworkGUID *OutNetGUID = NULL );

	// @todo document
	virtual bool SerializeName( FArchive& Ar, FName& Name );

	/** get the cached field to index mappings for the given class */
	FClassNetCache* GetClassNetCache(UClass* Class);
	void	ClearClassNetCache();

	virtual UObject * NetGUIDAssign(FNetworkGUID NetGUID, FString Path, UObject *ObjOuter = NULL) { return NULL; }
	
	virtual void HandleUnAssignedObject(const UObject* Obj) { }

	virtual FNetworkGUID GetObjectNetGUID(const UObject* Obj);

	virtual UObject * GetObjectFromNetGUID(FNetworkGUID NetGUID);

	virtual void ResetPackageMap();

	/** Removes deleted objects from the package map lookup tables */
	virtual void CleanPackageMap();

	/**
	 * logs debug info (package list, etc) to the specified output device
	 * @param Ar - the device to log to
	 */
	virtual void LogDebugInfo(FOutputDevice& Ar);

	virtual void SetLocked(bool L) { }

	// Variables.

	virtual FNetworkGUID AssignNewNetGUID(const UObject* Object);
	virtual FNetworkGUID AssignNetGUID(const UObject* Object, FNetworkGUID NewNetworkGUID);

	void ResetUnAckedObject() { bSerializedUnAckedObject = false; }
	bool SerializedUnAckedObject() { return bSerializedUnAckedObject; }

	void ResetHasSerializedCDO() { bSerializedCDO = false; }
	bool HasSerializedCDO() const { return bSerializedCDO; }
	
	virtual bool SerializeNewActor(FArchive& Ar, class UActorChannel *Channel, class AActor*& Actor) { return false; }

	virtual bool NetGUIDHasBeenAckd(FNetworkGUID NetGUID) { return false; }
	virtual void ReceivedNak( const int32 NakPacketId ) { }
	virtual void ReceivedAck( const int32 AckPacketId ) { }
	virtual void NotifyBunchCommit( const int32 OutPacketId, const TArray< FNetworkGUID > & ExportNetGUIDs ) { }

	virtual void GetNetGUIDStats(int32 &AckCount, int32 &UnAckCount, int32 &PendingCount) { }

	UNetGUIDCache * GetNetGUIDCache() { return Cache; }

	FNetObjectIsDynamic ObjectIsDynamicDelegate;

	virtual void NotifyStreamingLevelUnload(UObject* UnloadedLevel) { }

	virtual bool PrintExportBatch() { return false; }

	void	SetDebugContextString(FString Str) { DebugContextString = Str; }
	void	ClearDebugContextString() { DebugContextString.Empty(); }

protected:

	virtual bool IsDynamicObject(const UObject* Object);
	virtual bool IsNetGUIDAuthority() { return true; }

	UNetGUIDCache * Cache;

	/** map from package names to their index in List */
	TMap<FName, int32> PackageListMap;

	// @todo document
	TMap<TWeakObjectPtr<UClass>, FClassNetCache*> ClassFieldIndices;

	bool	bSuppressLogs;
	bool	bShouldSerializeUnAckedObjects;
	bool	bSerializedUnAckedObject;
	bool	bSerializedCDO;

	FString	DebugContextString;
};

/** Represents a range of PacketIDs, inclusive */
struct FPacketIdRange
{
	FPacketIdRange(int32 _First, int32 _Last) : First(_First), Last(_Last) { }
	FPacketIdRange(int32 PacketId) : First(PacketId), Last(PacketId) { }
	FPacketIdRange() : First(INDEX_NONE), Last(INDEX_NONE) { }
	int32	First;
	int32	Last;

	bool	InRange(int32 PacketId)
	{
		return (First <= PacketId && PacketId <= Last);
	}
};

/** Information for tracking retirement and retransmission of a property. */
struct FPropertyRetirement
{
	FPropertyRetirement * Next;

	TSharedPtr<class INetDeltaBaseState> DynamicState;

	FPacketIdRange	OutPacketIdRange;
	int32			InPacketId;			// Packet received on, INDEX_NONE=none.

	uint32			Reliable		: 1;	// Whether it was sent reliably.
	uint32			CustomDelta		: 1;	// True if this property uses custom delta compression
	uint32			Config			: 1;

	FPropertyRetirement()
		:	Next ( NULL )
		,	DynamicState ( NULL )
		,	InPacketId	( INDEX_NONE )
		,   Reliable( 0 )
		,   CustomDelta( 0 )
		,	Config( 0 )
	{}
};

/** Secondary condition to check before considering the replication of a lifetime property. */
enum ELifetimeCondition
{
	COND_None				= 0,		// This property has no condition, and will send anytime it changes
	COND_InitialOnly		= 1,		// This property will only attempt to send on the initial bunch
	COND_OwnerOnly			= 2,		// This property will only send to the actor's owner
	COND_SkipOwner			= 3,		// This property send to every connection EXCEPT the owner
	COND_SimulatedOnly		= 4,		// This property will only send to simulated actors
	COND_AutonomousOnly		= 5,		// This property will only send to autonomous actors
	COND_SimulatedOrPhysics	= 6,		// This property will send to simulated OR bRepPhysics actors
	COND_InitialOrOwner		= 7,		// This property will send on the initial packet, or to the actors owner
	COND_Custom				= 8,		// This property has no particular condition, but wants the ability to toggle on/off via SetCustomIsActiveOverride
	COND_Max				= 9,
};

/** FLifetimeProperty
 *	This class is used to track a property that is marked to be replicated for the lifetime of the actor channel.
 *  This doesn't mean the property will necessarily always be replicated, it just means:
 *	"check this property for replication for the life of the actor, and I don't want to think about it anymore"
 *  A secondary condition can also be used to skip replication based on the condition results
 */
class FLifetimeProperty
{
public:
	uint16				RepIndex;
	ELifetimeCondition	Condition;

	FLifetimeProperty() : RepIndex( 0 ), Condition( COND_None ) {}
	FLifetimeProperty( int32 InRepIndex ) : RepIndex( InRepIndex ), Condition( COND_None ) { check( InRepIndex <= 65535 ); }
	FLifetimeProperty( int32 InRepIndex, ELifetimeCondition InCondition ) : RepIndex( InRepIndex ), Condition( InCondition ) { check( InRepIndex <= 65535 ); }

	inline bool operator==( const FLifetimeProperty & Other ) const
	{
		if ( RepIndex == Other.RepIndex )
		{
			check( Condition == Other.Condition );		// Can't have different conditions if the RepIndex matches, doesn't make sense
			return true;
		}

		return false;
	}
};

template <> struct TIsZeroConstructType<FLifetimeProperty> { enum { Value = true }; };

/**
 * FNetBitWriter
 *	A bit writer that serializes FNames and UObject* through
 *	a network packagemap.
 */
class COREUOBJECT_API FNetBitWriter : public FBitWriter
{
public:
	FNetBitWriter( UPackageMap * InPackageMap, int64 InMaxBits );
	FNetBitWriter( int64 InMaxBits );
	FNetBitWriter();

	class UPackageMap * PackageMap;

	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};

/**
 * FNetBitReader
 *	A bit reader that serializes FNames and UObject* through
 *	a network packagemap.
 */
class COREUOBJECT_API FNetBitReader : public FBitReader
{
public:
	FNetBitReader( UPackageMap* InPackageMap=NULL, uint8* Src=NULL, int64 CountBits=0 );
	FNetBitReader( int64 InMaxBits );

	class UPackageMap * PackageMap;

	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};


/**
 * INetDeltaBaseState
 *	An abstract interface for the base state used in net delta serialization. See notes in NetSerialization.h
 */
class INetDeltaBaseState
{
public:
	INetDeltaBaseState() { }
	virtual ~INetDeltaBaseState() { }

	virtual bool IsStateEqual(INetDeltaBaseState* Otherstate) = 0;

private:
};

class INetSerializeCB
{
public:
	INetSerializeCB() { }

	virtual void NetSerializeStruct( UStruct * Struct, FArchive & Ar, UPackageMap *	Map, void * Data, bool & bHasUnmapped ) = 0;
};

class IRepChangedPropertyTracker
{
public:
	IRepChangedPropertyTracker() { }

	virtual void SetCustomIsActiveOverride( const uint16 RepIndex, const bool bIsActive ) = 0;
};

/**
 * FNetDeltaSerializeInfo
 *  This is the parameter structure for delta serialization. It is kind of a dumping ground for anything custom implementations may need.
 */
struct FNetDeltaSerializeInfo
{
	FNetDeltaSerializeInfo()
	{
		OutBunch = NULL;
		NewState = NULL;
		OldState = NULL;
		Map = NULL;
		Data = NULL;

		Struct = NULL;
		InArchive = NULL;

		NumPotentialBits = 0;
		NumBytesTouched = 0;

		NetSerializeCB = NULL;
	}

	// Used for ggeneric TArray replication
	FBitWriter * OutBunch;
	
	TSharedPtr<INetDeltaBaseState> *NewState;		// SharedPtr to new base state created by NetDeltaSerialize.
	INetDeltaBaseState *  OldState;				// Pointer to the previous base state.
	UPackageMap *	Map;
	void * Data;

	// Only used for fast TArray replication
	UStruct *Struct;

	// Used for TArray replication reading
	FArchive *InArchive;

	// Number of bits touched during full state serialization
	int32 NumPotentialBits;
	// Number of bytes touched (will be larger usually due to forced byte alignment)
	int32 NumBytesTouched;

	INetSerializeCB * NetSerializeCB;

	// Debugging variables
	FString DebugName;
};

/**
 * Checksum macros for verifying archives stay in sync
 */
COREUOBJECT_API void SerializeChecksum(FArchive &Ar, uint32 x, bool ErrorOK);

#define NET_ENABLE_CHECKSUMS 0

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) && NET_ENABLE_CHECKSUMS

#define NET_CHECKSUM_OR_END(Ser) \
{ \
	SerializeChecksum(Ser,0xE282FA84, true); \
}

#define NET_CHECKSUM(Ser) \
{ \
	SerializeChecksum(Ser,0xE282FA84, false); \
}

#define NET_CHECKSUM_CUSTOM(Ser, x) \
{ \
	SerializeChecksum(Ser,x, false); \
}

// There are cases where a checksum failure is expected, but we still need to eat the next word (just dont without erroring)
#define NET_CHECKSUM_IGNORE(Ser) \
{ \
	uint32 Magic = 0; \
	Ser << Magic; \
}

#else

// No ops in shipping builds
#define NET_CHECKSUM(Ser)
#define NET_CHECKSUM_IGNORE(Ser)
#define NET_CHECKSUM_CUSTOM(Ser, x)
#define NET_CHECKSUM_OR_END(ser)


#endif

/**
 * Functions to assist in detecting errors during RPC calls
 */
COREUOBJECT_API void			RPC_ResetLastFailedReason();
COREUOBJECT_API void			RPC_ValidateFailed( const TCHAR * Reason );
COREUOBJECT_API const TCHAR *	RPC_GetLastFailedReason();
