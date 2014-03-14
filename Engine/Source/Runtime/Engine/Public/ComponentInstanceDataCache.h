// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Base class for component instance cached data of a particular type. */
class FComponentInstanceDataBase
{
public:
	virtual ~FComponentInstanceDataBase()
	{}

	/** Return the type name of this data */
	virtual FName GetDataTypeName() const = 0;
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
	FComponentInstanceDataCache(AActor* InActor)
	{
		GetFromActor(InActor, *this);
	}

	/** Add some new instance data to the cache */
	void AddInstanceData(TSharedPtr<FComponentInstanceDataBase> NewData);

	/** Get all instance data of a particular type */
	void GetInstanceDataOfType(FName TypeName, TArray< TSharedPtr<FComponentInstanceDataBase> >& OutData) const;

	/** Util to iterate over components and get data from each */
	static void GetFromActor(AActor* Actor, FComponentInstanceDataCache& Cache);

	/** Util to iterate over components and apply data to each */
	void ApplyToActor(AActor* Actor);

private:
	/** Map of data type name to data of that type */
	TMultiMap< FName, TSharedPtr<FComponentInstanceDataBase> >	TypeToDataMap;
};