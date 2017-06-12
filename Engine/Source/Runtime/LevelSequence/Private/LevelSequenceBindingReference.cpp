// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceBindingReference.h"
#include "LevelSequenceLegacyObjectReference.h"
#include "UObject/Package.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneFwd.h"
#include "PackageName.h"
#include "Engine/World.h"

FLevelSequenceBindingReference::FLevelSequenceBindingReference(UObject* InObject, UObject* InContext)
{
	check(InContext && InObject);

	if (!InContext->IsA<UWorld>() && InObject->IsIn(InContext))
	{
		ObjectPath = InObject->GetPathName(InContext);
	}
	else
	{
		UPackage* ObjectPackage = InObject->GetOutermost();
		if (!ensure(ObjectPackage))
		{
			return;
		}

		PackageName = ObjectPackage->GetName();
	#if WITH_EDITORONLY_DATA
		if (ObjectPackage->PIEInstanceID != INDEX_NONE)
		{
			FString PIEPrefix = FString::Printf(PLAYWORLD_PACKAGE_PREFIX TEXT("_%d_"), ObjectPackage->PIEInstanceID);
			PackageName.ReplaceInline(*PIEPrefix, TEXT(""));
		}
	#endif

		ObjectPath = InObject->GetPathName(ObjectPackage);
	}
}

UObject* FLevelSequenceBindingReference::Resolve(UObject* InContext) const
{
	if (PackageName.Len() == 0)
	{
		return FindObject<UObject>(InContext, *ObjectPath, false);
	}
	else
	{
		const TCHAR* SearchWithinPackage = *PackageName;
		FString FixupPIEPackageName;

	#if WITH_EDITORONLY_DATA
		int32 PIEInstanceID = InContext ? InContext->GetOutermost()->PIEInstanceID : INDEX_NONE;
		if (PIEInstanceID != INDEX_NONE)
		{
			const FString ShortPackageOuterAndName = FPackageName::GetLongPackageAssetName(PackageName);
			if (ensureMsgf(!ShortPackageOuterAndName.StartsWith(PLAYWORLD_PACKAGE_PREFIX), TEXT("Detected PIE world prefix in level sequence binding - this should not happen")))
			{
				FixupPIEPackageName = FPackageName::GetLongPackagePath(PackageName) / FString::Printf(PLAYWORLD_PACKAGE_PREFIX TEXT("_%d_"), PIEInstanceID) + ShortPackageOuterAndName;
				SearchWithinPackage = *FixupPIEPackageName;
			}
		}
	#endif

		UPackage* Package = FindPackage(nullptr, SearchWithinPackage);
		return Package ? FindObject<UObject>(Package, *ObjectPath, false) : nullptr;
	}
}

UObject* FLevelSequenceLegacyObjectReference::Resolve(UObject* InContext) const
{
	if (ObjectId.IsValid() && InContext != nullptr)
	{
		int32 PIEInstanceID = InContext->GetOutermost()->PIEInstanceID;
		FUniqueObjectGuid FixedUpId = PIEInstanceID == -1 ? ObjectId : ObjectId.FixupForPIE(PIEInstanceID);

		FLazyObjectPtr LazyPtr;
		LazyPtr = FixedUpId;

		if (UObject* FoundObject = LazyPtr.Get())
		{
			return FoundObject;
		}
	}

	if (!ObjectPath.IsEmpty())
	{
		if (UObject* FoundObject = FindObject<UObject>(InContext, *ObjectPath, false))
		{
			return FoundObject;
		}

		if (UObject* FoundObject = FindObject<UObject>(ANY_PACKAGE, *ObjectPath, false))
		{
			return FoundObject;
		}
	}

	return nullptr;
}

bool FLevelSequenceObjectReferenceMap::Serialize(FArchive& Ar)
{
	int32 Num = Map.Num();
	Ar << Num;

	if (Ar.IsLoading())
	{
		while(Num-- > 0)
		{
			FGuid Key;
			Ar << Key;

			FLevelSequenceLegacyObjectReference Value;
			Ar << Value;

			Map.Add(Key, Value);
		}
	}
	else if (Ar.IsSaving() || Ar.IsCountingMemory() || Ar.IsObjectReferenceCollector())
	{
		for (auto& Pair : Map)
		{
			Ar << Pair.Key;
			Ar << Pair.Value;
		}
	}
	return true;
}

bool FLevelSequenceBindingReferences::HasBinding(const FGuid& ObjectId) const
{
	return BindingIdToReferences.Contains(ObjectId);
}

void FLevelSequenceBindingReferences::AddBinding(const FGuid& ObjectId, UObject* InObject, UObject* InContext)
{
	BindingIdToReferences.FindOrAdd(ObjectId).References.Emplace(InObject, InContext);
}

void FLevelSequenceBindingReferences::RemoveBinding(const FGuid& ObjectId)
{
	BindingIdToReferences.Remove(ObjectId);
}

void FLevelSequenceBindingReferences::ResolveBinding(const FGuid& ObjectId, UObject* InContext, TArray<UObject*, TInlineAllocator<1>>& OutObjects) const
{
	const FLevelSequenceBindingReferenceArray* ReferenceArray = BindingIdToReferences.Find(ObjectId);
	if (!ReferenceArray)
	{
		return;
	}

	for (const FLevelSequenceBindingReference& Reference : ReferenceArray->References)
	{
		UObject* ResolvedObject = Reference.Resolve(InContext);

		// if the resolved object does not have a valid world (e.g. world is being torn down), dont resolve
		if (ResolvedObject && ResolvedObject->GetWorld())
		{
			OutObjects.Add(ResolvedObject);
		}
	}
}


UObject* FLevelSequenceObjectReferenceMap::ResolveBinding(const FGuid& ObjectId, UObject* InContext) const
{
	const FLevelSequenceLegacyObjectReference* Reference = Map.Find(ObjectId);
	UObject* ResolvedObject = Reference ? Reference->Resolve(InContext) : nullptr;
	if (ResolvedObject != nullptr)
	{
		// if the resolved object does not have a valid world (e.g. world is being torn down), dont resolve
		return ResolvedObject->GetWorld() != nullptr ? ResolvedObject : nullptr;
	}
	return nullptr;
}