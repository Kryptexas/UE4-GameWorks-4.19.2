// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SmartName.generated.h"

struct FSmartName;

USTRUCT()
struct ENGINE_API FSmartNameMapping
{
	GENERATED_USTRUCT_BODY();

	FSmartNameMapping();

	// ID type, should be used to access names as fundamental type may change.
	// NOTE: If changing this, also change the type of NextUid below. UHT ignores,
	// this typedef and needs to know the fundamental type to parse properties.
	// If this changes and UHT handles typedefs, set NextUid type to UID.
	typedef uint16 UID;
	// Max UID used for overflow checking
	static const UID MaxUID = MAX_uint16;

	// Add a name to the mapping, if it exists, get it.
	// @param Name - The name to add/get
	// @param OUT OutUid - The UID of the newly created or retrieved name
	// @return bool - true if the name was added, false if it existed (OutUid will be correctly set still in this case)
	bool AddOrFindName(FName Name, UID& OutUid, FGuid& OutGuid);

	// Get a name from the mapping
	// @param Uid - UID of the name to retrieve
	// @param OUT OutName - Retrieved name
	// @return bool - true if name existed and OutName is valid
	bool GetName(const UID& Uid, FName& OutName) const;
	bool GetNameByGuid(const FGuid& Guid, FName& OutName) const;
	// Fill an array with all used UIDs
	// @param Array - Array to fill
	void FillUidArray(TArray<UID>& Array) const;

	// Fill an array with all used names
	// @param Array - Array to fill
	void FillNameArray(TArray<FName>& Array) const;

	// Change a name
	// @param Uid - UID of the name to change
	// @param NewName - New name to set 
	// @return bool - true if the name was found and changed, false if the name wasn't present in the mapping
	bool Rename(const UID& Uid, FName NewName);

	// Remove a name from the mapping
	// @param Uid - UID of the name to remove
	// @return bool - true if the name was found and removed, false if the name wasn't present in the mapping
	bool Remove(const UID& Uid);

	// Return UID * if it finds it
	//  @param NewName - New name to set 
	// @return UID pointer - null if it doesn't find. pointer if it finds. 
	const FSmartNameMapping::UID* FindUID(const FName& Name) const;

	// Check whether a name already exists in the mapping
	// @param Uid - the UID to check
	// @return bool - whether the name was found
	bool Exists(const UID& Uid) const;

	// Check whether a name already exists in the mapping
	// @param Name - the name to check
	// @return bool - whether the name was found
	bool Exists(const FName& Name) const;

	// Get the number of names currently stored in this container
	int32 GetNumNames() const;

	// Find Or Add Smart Names
	bool FindOrAddSmartName(FName Name, FSmartName& OutName);
	bool FindSmartName(FName Name, FSmartName& OutName) const;
	bool FindSmartNameByUID(FSmartNameMapping::UID UID, FSmartName& OutName) const;

	// Serialize this to the provided archive; required for TMap serialization
	void Serialize(FArchive& Ar);

	friend FArchive& operator<<(FArchive& Ar, FSmartNameMapping& Elem);

private:
	UID NextUid;				// The next UID to use 
	TMap<UID, FName> UidMap;	// Mapping of UIDs and names. This is transient and built upon serialize - this is run-time data, and searching from UID to FName is important
	TMap<FName, FGuid> GuidMap;	// Mapping of GUIDs and names. This is the one serializing
};

USTRUCT()
struct ENGINE_API FSmartNameContainer
{
	GENERATED_USTRUCT_BODY();

	// Add a new smartname container with the provided name
	void AddContainer(FName NewContainerName);

	// Get a container by name	
	const FSmartNameMapping* GetContainer(FName ContainerName) const;

	// Serialize this to the provided archive; required for TMap serialization
	void Serialize(FArchive& Ar);

	friend FArchive& operator<<(FArchive& Ar, FSmartNameContainer& Elem);

	/** Only restricted classes can access the protected interface */
	friend class USkeleton;
protected:
	FSmartNameMapping* GetContainerInternal(const FName& ContainerName);

private:
	TMap<FName, FSmartNameMapping> NameMappings;	// List of smartname mappings
};

USTRUCT()
struct ENGINE_API FSmartName
{

	GENERATED_USTRUCT_BODY();

	// name 
	UPROPERTY()
	FName DisplayName;

	// UID - for faster access
	FSmartNameMapping::UID	UID;

	UPROPERTY()
	FGuid	Guid;

	FSmartName()
		: DisplayName(NAME_None)
		, UID(FSmartNameMapping::MaxUID)
	{}

	FSmartName(const FName& InDisplayName, FSmartNameMapping::UID InUID, const FGuid& InGuid)
		: DisplayName(InDisplayName)
		, UID(InUID)
		, Guid(InGuid)
	{}

	bool operator==(FSmartName const& Other) const
	{
		return (DisplayName == Other.DisplayName && UID == Other.UID && Guid == Other.Guid);
	}
	bool operator!=(const FSmartName& Other) const
	{
		return !(*this == Other);
	}
	/**
	* Serialize the SmartName
	*
	* @param Ar	Archive to serialize to
	*
	* @return True if the container was serialized
	*/
	bool Serialize(FArchive& Ar);

	friend FArchive& operator<<(FArchive& Ar, FSmartName& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	bool IsValid() const
	{
		return UID != FSmartNameMapping::MaxUID && Guid.IsValid(); 
	}
};

template<>
struct TStructOpsTypeTraits<FSmartName> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithSerializer = true,
		WithIdenticalViaEquality = true
	};
};