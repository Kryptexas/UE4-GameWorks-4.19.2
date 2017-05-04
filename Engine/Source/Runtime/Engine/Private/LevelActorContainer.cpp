// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Level.cpp: Level-related functions
=============================================================================*/

#include "Engine/LevelActorContainer.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "UObject/FastReferenceCollector.h"
#include "UObject/UObjectArray.h"
#include "UObject/Package.h"
#include "UObject/UObjectClusters.h"

DEFINE_LOG_CATEGORY_STATIC(LogLevelActorContainer, Log, All);

/**
* Pool for reducing allocations when constructing clusters
*/
class FActorClusterArrayPool
{
public:

	/**
	* Gets the singleton instance of the FObjectArrayPool
	* @return Pool singleton.
	*/
	FORCEINLINE static FActorClusterArrayPool& Get()
	{
		static FActorClusterArrayPool Singleton;
		return Singleton;
	}

	/**
	* Gets an event from the pool or creates one if necessary.
	*
	* @return The array.
	* @see ReturnToPool
	*/
	FORCEINLINE TArray<UObject*>* GetArrayFromPool()
	{
		TArray<UObject*>* Result = Pool.Pop();
		if (!Result)
		{
			Result = new TArray<UObject*>();
		}
		check(Result);
#if UE_BUILD_DEBUG
		NumberOfUsedArrays.Increment();
#endif // UE_BUILD_DEBUG
		return Result;
	}

	/**
	* Returns an array to the pool.
	*
	* @param Array The array to return.
	* @see GetArrayFromPool
	*/
	FORCEINLINE void ReturnToPool(TArray<UObject*>* Array)
	{
#if UE_BUILD_DEBUG
		const int32 CheckUsedArrays = NumberOfUsedArrays.Decrement();
		checkSlow(CheckUsedArrays >= 0);
#endif // UE_BUILD_DEBUG
		check(Array);
		Array->Reset();
		Pool.Push(Array);
	}

	/** Performs memory cleanup */
	void Cleanup()
	{
#if UE_BUILD_DEBUG
		const int32 CheckUsedArrays = NumberOfUsedArrays.GetValue();
		checkSlow(CheckUsedArrays == 0);
#endif // UE_BUILD_DEBUG

		uint32 FreedMemory = 0;
		TArray< TArray<UObject*>* > AllArrays;
		Pool.PopAll(AllArrays);
		for (TArray<UObject*>* Array : AllArrays)
		{
			FreedMemory += Array->GetAllocatedSize();
			delete Array;
		}
		UE_LOG(LogLevelActorContainer, Log, TEXT("Freed %ub from %d cluster array pools."), FreedMemory, AllArrays.Num());
	}

#if UE_BUILD_DEBUG
	void CheckLeaks()
	{
		// This function is called after cluster has been created so at this point there should be no
		// arrays used by cluster creation code and all should be returned to the pool
		const int32 LeakedPoolArrays = NumberOfUsedArrays.GetValue();
		checkSlow(LeakedPoolArrays == 0);
	}
#endif

private:

	/** Holds the collection of recycled arrays. */
	TLockFreePointerListLIFO< TArray<UObject*> > Pool;

#if UE_BUILD_DEBUG
	/** Number of arrays currently acquired from the pool by clusters */
	FThreadSafeCounter NumberOfUsedArrays;
#endif // UE_BUILD_DEBUG
};

/**
* Handles UObject references found by TFastReferenceCollector
*/
class FActorClusterReferenceProcessor
{
	int32 ClusterRootIndex;
	FUObjectCluster& Cluster;
	ULevel* ParentLevel;
	UPackage* ParentLevelPackage;
	volatile bool bIsRunningMultithreaded;
public:

	FActorClusterReferenceProcessor(int32 InClusterRootIndex, FUObjectCluster& InCluster, ULevel* InParentLevel)
		: ClusterRootIndex(InClusterRootIndex)
		, Cluster(InCluster)
		, ParentLevel(InParentLevel)
		, bIsRunningMultithreaded(false)
	{
		ParentLevelPackage = ParentLevel->GetOutermost();
	}

	FORCEINLINE int32 GetMinDesiredObjectsPerSubTask() const
	{
		// We're not running the processor in parallel when creating clusters
		return 0;
	}

	FORCEINLINE volatile bool IsRunningMultithreaded() const
	{
		// This should always be false
		return bIsRunningMultithreaded;
	}

	FORCEINLINE void SetIsRunningMultithreaded(bool bIsParallel)
	{
		check(!bIsParallel);
		bIsRunningMultithreaded = bIsParallel;
	}

	void UpdateDetailedStats(UObject* CurrentObject, uint32 DeltaCycles)
	{
	}

	void LogDetailedStatsSummary()
	{
	}

	FORCENOINLINE bool CanAddToCluster(UObject* Object)
	{
		if (!Object->IsIn(ParentLevelPackage))
		{
			// No external references are allowed in level clusters
			return false;
		}
		else if (!Object->IsIn(ParentLevel))
		{
			// If the object is in the same package but is not in the level we don't want it either.
			return false;
		}
		if (Object->IsA(ULevel::StaticClass()) || Object->IsA(UWorld::StaticClass()))
		{
			// And generally, no levels or worlds
			return false;
		}
		return Object->CanBeInCluster();
	}

	/**
	* Adds an object to cluster (if possible)
	*
	* @param ObjectIndex UObject index in GUObjectArray
	* @param ObjectItem UObject's entry in GUObjectArray
	* @param Obj The object to add to cluster
	* @param ObjectsToSerialize An array of remaining objects to serialize (Obj must be added to it if Obj can be added to cluster)
	* @param bOuterAndClass If true, the Obj's Outer and Class will also be added to the cluster
	*/
	void AddObjectToCluster(int32 ObjectIndex, FUObjectItem* ObjectItem, UObject* Obj, TArray<UObject*>& ObjectsToSerialize, bool bOuterAndClass)
	{
		// If we haven't finished loading, we can't be sure we know all the references
		check(!Obj->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad));
		check(ObjectItem->GetOwnerIndex() == 0 || ObjectItem->GetOwnerIndex() == ClusterRootIndex || ObjectIndex == ClusterRootIndex);
		check(Obj->CanBeInCluster());
		if (ObjectIndex != ClusterRootIndex && ObjectItem->GetOwnerIndex() == 0 && !GUObjectArray.IsDisregardForGC(Obj))
		{
			ObjectsToSerialize.Add(Obj);
			check(!ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot));
			ObjectItem->SetOwnerIndex(ClusterRootIndex);
			Cluster.Objects.Add(ObjectIndex);

			if (bOuterAndClass)
			{
				UObject* ObjOuter = Obj->GetOuter();
				if (CanAddToCluster(ObjOuter))
				{
					HandleTokenStreamObjectReference(ObjectsToSerialize, Obj, ObjOuter, INDEX_NONE, true);
				}
				else
				{
					Cluster.MutableObjects.AddUnique(GUObjectArray.ObjectToIndex(ObjOuter));
				}
				if (!Obj->GetClass()->HasAllClassFlags(CLASS_Native))
				{
					UObject* ObjectClass = Obj->GetClass();
					HandleTokenStreamObjectReference(ObjectsToSerialize, Obj, ObjectClass, INDEX_NONE, true);
					UObject* ObjectClassOuter = Obj->GetClass()->GetOuter();
					HandleTokenStreamObjectReference(ObjectsToSerialize, Obj, ObjectClassOuter, INDEX_NONE, true);
				}
			}
		}
	}

	/**
	* Handles UObject reference from the token stream. Performance is critical here so we're FORCEINLINING this function.
	*
	* @param ObjectsToSerialize An array of remaining objects to serialize (Obj must be added to it if Obj can be added to cluster)
	* @param ReferencingObject Object referencing the object to process.
	* @param TokenIndex Index to the token stream where the reference was found.
	* @param bAllowReferenceElimination True if reference elimination is allowed (ignored when constructing clusters).
	*/
	FORCEINLINE void HandleTokenStreamObjectReference(TArray<UObject*>& ObjectsToSerialize, UObject* ReferencingObject, UObject*& Object, const int32 TokenIndex, bool bAllowReferenceElimination)
	{
		if (Object)
		{
			// If we haven't finished loading, we can't be sure we know all the references
			check(!Object->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad));

			FUObjectItem* ObjectItem = GUObjectArray.ObjectToObjectItem(Object);

			// Add encountered object reference to list of to be serialized objects if it hasn't already been added.
			if (ObjectItem->GetOwnerIndex() != ClusterRootIndex)
			{
				if (ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot) || ObjectItem->GetOwnerIndex() != 0)
				{
					// Simply reference this cluster and all clusters it's referencing
					const int32 OtherClusterRootIndex = ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot) ? GUObjectArray.ObjectToIndex(Object) : ObjectItem->GetOwnerIndex();
					FUObjectItem* OtherClusterRootItem = GUObjectArray.IndexToObject(OtherClusterRootIndex);
					FUObjectCluster& OtherCluster = GUObjectClusters[OtherClusterRootItem->GetClusterIndex()];
					Cluster.ReferencedClusters.AddUnique(OtherClusterRootIndex);

					OtherCluster.ReferencedByClusters.AddUnique(ClusterRootIndex);

					for (int32 OtherClusterReferencedCluster : OtherCluster.ReferencedClusters)
					{
						if (OtherClusterReferencedCluster != ClusterRootIndex)
						{
							Cluster.ReferencedClusters.AddUnique(OtherClusterReferencedCluster);
						}
					}
					for (int32 OtherClusterReferencedMutableObjectIndex : OtherCluster.MutableObjects)
					{
						Cluster.MutableObjects.AddUnique(OtherClusterReferencedMutableObjectIndex);
					}
				}
				else if (!GUObjectArray.IsDisregardForGC(Object)) // Objects that can create clusters themselves and haven't been postloaded yet should be excluded
				{
					check(ObjectItem->GetOwnerIndex() == 0);

					// New object, add it to the cluster.
					if (CanAddToCluster(Object) && !Object->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
					{
						AddObjectToCluster(GUObjectArray.ObjectToIndex(Object), ObjectItem, Object, ObjectsToSerialize, true);
					}
					else
					{
						checkf(!Object->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad), TEXT("%s is being added to cluster but hasn't finished loading yet"), *Object->GetFullName());
						Cluster.MutableObjects.AddUnique(GUObjectArray.ObjectToIndex(Object));
					}
				}
			}
		}
	}
};

/**
* Specialized FReferenceCollector that uses FActorClusterReferenceProcessor to construct the cluster
*/
template <class TProcessor>
class TActorClusterCollector : public FReferenceCollector
{
	TProcessor& Processor;
	TArray<UObject*>& ObjectArray;

public:
	TActorClusterCollector(TProcessor& InProcessor, TArray<UObject*>& InObjectArray)
		: Processor(InProcessor)
		, ObjectArray(InObjectArray)
	{
	}
	virtual void HandleObjectReference(UObject*& Object, const UObject* ReferencingObject, const UProperty* ReferencingProperty) override
	{
		Processor.HandleTokenStreamObjectReference(ObjectArray, const_cast<UObject*>(ReferencingObject), Object, INDEX_NONE, false);
	}
	virtual void HandleObjectReferences(UObject** InObjects, const int32 ObjectNum, const UObject* ReferencingObject, const UProperty* InReferencingProperty) override
	{
		for (int32 ObjectIndex = 0; ObjectIndex < ObjectNum; ++ObjectIndex)
		{
			UObject*& Object = InObjects[ObjectIndex];
			Processor.HandleTokenStreamObjectReference(ObjectArray, const_cast<UObject*>(ReferencingObject), Object, INDEX_NONE, false);
		}
	}
	virtual bool IsIgnoringArchetypeRef() const override
	{
		return false;
	}
	virtual bool IsIgnoringTransient() const override
	{
		return false;
	}
};

void ULevelActorContainer::CreateCluster()
{

	int32 ContainerInternalIndex = GUObjectArray.ObjectToIndex(this);
	FUObjectItem* RootItem = GUObjectArray.IndexToObject(ContainerInternalIndex);
	if (RootItem->GetOwnerIndex() != 0 || RootItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
	{
		return;
	}

	// If we haven't finished loading, we can't be sure we know all the references
	check(!HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad));

	// Create a new cluster, reserve an arbitrary amount of memory for it.
	const int32 ClusterIndex = GUObjectClusters.AllocateCluster(GUObjectArray.ObjectToIndex(this));
	FUObjectCluster& Cluster = GUObjectClusters[ClusterIndex];
	Cluster.Objects.Reserve(64);

	// Collect all objects referenced by cluster root and by all objects it's referencing
	FActorClusterReferenceProcessor Processor(ContainerInternalIndex, Cluster, CastChecked<ULevel>(GetOuter()));
	TFastReferenceCollector<false, FActorClusterReferenceProcessor, TActorClusterCollector<FActorClusterReferenceProcessor>, FActorClusterArrayPool, true> ReferenceCollector(Processor, FActorClusterArrayPool::Get());
	TArray<UObject*> ObjectsToProcess;
	ObjectsToProcess.Add(static_cast<UObject*>(this));
	ReferenceCollector.CollectReferences(ObjectsToProcess);
#if UE_BUILD_DEBUG
	FActorClusterArrayPool::Get().CheckLeaks();
#endif

	if (Cluster.Objects.Num())
	{
		// Sort all objects and set up the cluster root
		Cluster.Objects.Sort();
		Cluster.ReferencedClusters.Sort();
		Cluster.MutableObjects.Sort();
		check(RootItem->GetOwnerIndex() == 0);
		RootItem->SetClusterIndex(ClusterIndex);
		RootItem->SetFlags(EInternalObjectFlags::ClusterRoot);

		UE_LOG(LogLevelActorContainer, Log, TEXT("Created LevelActorCluster (%d) for %s with %d objects, %d referenced clusters and %d mutable objects."),
			ClusterIndex, *GetOuter()->GetPathName(), Cluster.Objects.Num(), Cluster.ReferencedClusters.Num(), Cluster.MutableObjects.Num());

#if UE_GCCLUSTER_VERBOSE_LOGGING
		DumpClusterToLog(Cluster, true, false);
#endif
	}
	else
	{
		check(RootItem->GetOwnerIndex() == 0);
		RootItem->SetClusterIndex(ClusterIndex);
		GUObjectClusters.FreeCluster(ClusterIndex);
	}
}

void ULevelActorContainer::OnClusterMarkedAsPendingKill()
{
	ULevel* Level = CastChecked<ULevel>(GetOuter());
	Level->ActorsForGC.Append(Actors);
	Actors.Reset();

	Super::OnClusterMarkedAsPendingKill();
}
