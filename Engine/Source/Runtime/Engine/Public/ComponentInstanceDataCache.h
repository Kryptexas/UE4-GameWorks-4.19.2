// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Base class for component instance cached data of a particular type. */
class FComponentInstanceDataBase
{
public:
	FComponentInstanceDataBase()
		: SourceComponentTypeSerializedIndex(-1)
	{}

	FComponentInstanceDataBase(const UActorComponent* SourceComponent);

	virtual ~FComponentInstanceDataBase()
	{}

	virtual bool MatchesComponent(const UActorComponent* Component) const;

protected:
	/** The name of the source component */
	FName SourceComponentName;

	/** The index of the source component in its owner's serialized array 
		when filtered to just that component type */
	int32 SourceComponentTypeSerializedIndex;
};

/** 
 *	Cache for component instance data.
 *	Note, does not collect references for GC, so is not safe to GC if the cache is only reference to a UObject.
 */
class ENGINE_API FComponentInstanceDataCache
{
public:
	FComponentInstanceDataCache() {}

	/** Constructor that also populatees cache from Actor */
	FComponentInstanceDataCache(AActor* InActor);

	/** Add some new instance data to the cache */
	void AddInstanceData(TSharedPtr<FComponentInstanceDataBase> NewData);

	/** Util to iterate over components and apply data to each */
	void ApplyToActor(AActor* Actor) const;

private:
	/** Map of data type name to data of that type */
	TMultiMap< FName, TSharedPtr<FComponentInstanceDataBase> >	TypeToDataMap;
};