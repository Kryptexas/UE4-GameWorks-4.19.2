// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationPrivatePCH.h"
#include "ActorAnimationObjectReference.h"

FActorAnimationObjectReference::FActorAnimationObjectReference(UObject* InObject, UObject* InContext)
{
	if (InObject->IsA<AActor>() || InObject->IsA<UActorComponent>())
	{
		ObjectId = FLazyObjectPtr(InObject).GetUniqueID();
		ObjectPath = InObject->GetPathName(InContext);
	}
}

UObject* FActorAnimationObjectReference::Resolve(UObject* InContext) const
{
	int32 PIEInstanceID = InContext->GetOutermost()->PIEInstanceID;
	FUniqueObjectGuid FixedUpId = PIEInstanceID == -1 ? ObjectId : ObjectId.FixupForPIE(PIEInstanceID);

	FLazyObjectPtr LazyPtr;
	LazyPtr = FixedUpId;

	if (UObject* FoundObject = LazyPtr.Get())
	{
		return FoundObject;
	}
	else if (!ObjectPath.IsEmpty())
	{
		return FindObject<UObject>(InContext, *ObjectPath, false);
	}

	return nullptr;
}

bool FActorAnimationObjectReferenceMap::Serialize(FArchive& Ar)
{
	int32 Num = Map.Num();
	Ar << Num;

	if (Ar.IsLoading())
	{
		while(Num-- > 0)
		{
			FGuid Key;
			Ar << Key;

			FActorAnimationObjectReference Value;
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

bool FActorAnimationObjectReferenceMap::HasBinding(const FGuid& ObjectId) const
{
	if (const FActorAnimationObjectReference* Reference = Map.Find(ObjectId))
	{
		return Reference->IsValid();
	}
	return false;
}

void FActorAnimationObjectReferenceMap::CreateBinding(const FGuid& ObjectId, UObject* InObject, UObject* InContext)
{
	FActorAnimationObjectReference NewReference(InObject, InContext);

	if (ensureMsgf(NewReference.IsValid(), TEXT("Unable to generate a reference for the specified object and context")))
	{
		Map.FindOrAdd(ObjectId) = NewReference;
	}
}

void FActorAnimationObjectReferenceMap::RemoveBinding(const FGuid& ObjectId)
{
	Map.Remove(ObjectId);
}

UObject* FActorAnimationObjectReferenceMap::ResolveBinding(const FGuid& ObjectId, UObject* InContext) const
{
	const FActorAnimationObjectReference* Reference = Map.Find(ObjectId);
	return Reference ? Reference->Resolve(InContext) : nullptr;
}

FGuid FActorAnimationObjectReferenceMap::FindBindingId(UObject* InObject, UObject* InContext) const
{
	for (const auto& Pair : Map)
	{
		if (Pair.Value.Resolve(InContext) == InObject)
		{
			return Pair.Key;
		}
	}
	return FGuid();
}