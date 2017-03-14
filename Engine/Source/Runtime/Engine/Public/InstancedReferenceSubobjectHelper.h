// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"

/** 
 * Meant to represent a specific object property that is setup to reference a 
 * instanced sub-object. Tracks the property hierarchy used to reach the 
 * property, so that we can easily retrieve instanced sub-objects from a 
 * container object.
 */
struct FInstancedPropertyPath
{
private:
	struct FPropertyLink
	{
		FPropertyLink(const UProperty* Property, int32 ArrayIndexIn = INDEX_NONE)
			: PropertyPtr(Property), ArrayIndex(ArrayIndexIn)
		{}

		const UProperty* PropertyPtr;
		int32            ArrayIndex;
	};

public:
	//--------------------------------------------------------------------------
	FInstancedPropertyPath(UProperty* RootProperty)
	{
		PropertyChain.Add(FPropertyLink(RootProperty));
	}

	//--------------------------------------------------------------------------
	void Push(const UProperty* Property, int32 ArrayIndex = INDEX_NONE)
	{
		PropertyChain.Add(FPropertyLink(Property, ArrayIndex));		
	}

	//--------------------------------------------------------------------------
	void Pop()
	{
 		PropertyChain.RemoveAt(PropertyChain.Num() - 1);
	}

	//--------------------------------------------------------------------------
	const UProperty* Head() const
	{
		return PropertyChain.Last().PropertyPtr;
	}

	//--------------------------------------------------------------------------
	UObject* Resolve(const UObject* Container) const
	{
		UStruct* CurrentContainerType = Container->GetClass();

		const TArray<FPropertyLink>& PropChainRef = PropertyChain;
		auto GetProperty = [&CurrentContainerType, &PropChainRef](int32 ChainIndex)->UProperty*
		{
			const UProperty* SrcProperty = PropChainRef[ChainIndex].PropertyPtr;
			return FindField<UProperty>(CurrentContainerType, SrcProperty->GetFName());
		};

		UProperty* CurrentProp = GetProperty(0);
		const uint8* ValuePtr = (CurrentProp) ? CurrentProp->ContainerPtrToValuePtr<uint8>(Container) : nullptr;

		for (int32 ChainIndex = 1; CurrentProp && ChainIndex < PropertyChain.Num(); ++ChainIndex)
		{
			if (const UArrayProperty* ArrayProperty = Cast<UArrayProperty>(CurrentProp))
			{
				check(PropertyChain[ChainIndex].PropertyPtr == ArrayProperty->Inner);

				int32 TargetIndex = PropertyChain[ChainIndex].ArrayIndex;
				check(TargetIndex != INDEX_NONE);

				FScriptArrayHelper ArrayHelper(ArrayProperty, ValuePtr);
				if (TargetIndex >= ArrayHelper.Num())
				{
					CurrentProp = nullptr;
					break;
				}

				CurrentProp = ArrayProperty->Inner;
				ValuePtr    = ArrayHelper.GetRawPtr(TargetIndex);
			}
			else if (ensure(PropertyChain[ChainIndex].ArrayIndex <= 0))
			{
				if (const UStructProperty* StructProperty = Cast<UStructProperty>(CurrentProp))
				{
					CurrentContainerType = StructProperty->Struct;
				}

				CurrentProp = GetProperty(ChainIndex);
				ValuePtr    = (CurrentProp) ? CurrentProp->ContainerPtrToValuePtr<uint8>(ValuePtr) : nullptr;
			}
		}

		const UObjectProperty* TargetPropety = Cast<UObjectProperty>(CurrentProp);
		if (TargetPropety && TargetPropety->HasAnyPropertyFlags(CPF_InstancedReference))
		{ 
			return TargetPropety->GetObjectPropertyValue(ValuePtr);
		}
		return nullptr;
	}

private:
	TArray<FPropertyLink> PropertyChain;
};

/** 
 * Can be used as a raw sub-object pointer, but also contains a 
 * FInstancedPropertyPath to identify the property that this sub-object is 
 * referenced by. Paired together for ease of use (so API users don't have to manage a map).
 */
struct FInstancedSubObjRef
{
	FInstancedSubObjRef(UObject* SubObj, const FInstancedPropertyPath& PropertyPathIn)
		: SubObjInstance(SubObj)
		, PropertyPath(PropertyPathIn)
	{}

	//--------------------------------------------------------------------------
	operator UObject*() const
	{
		return SubObjInstance;
	}

	//--------------------------------------------------------------------------
	UObject* operator->() const
	{
		return SubObjInstance;
	}

	//--------------------------------------------------------------------------
	friend uint32 GetTypeHash(const FInstancedSubObjRef& SubObjRef)
	{
		return GetTypeHash((UObject*)SubObjRef);
	}

	UObject* SubObjInstance;
	FInstancedPropertyPath PropertyPath;
};
 
/** 
 * Contains a set of utility functions useful for searching out and identifying
 * instanced sub-objects contained within a specific outer object.
 */
class ENGINE_API FFindInstancedReferenceSubobjectHelper
{
public:
	template<typename T>
	static void GetInstancedSubObjects(const UObject* Container, T& OutObjects)
	{
		const UClass* ContainerClass = Container->GetClass();
		for (UProperty* Prop = ContainerClass->RefLink; Prop; Prop = Prop->NextRef)
		{
			FInstancedPropertyPath RootPropertyPath(Prop);
			GetInstancedSubObjects_Inner(RootPropertyPath, reinterpret_cast<const uint8*>(Container), [&OutObjects](const FInstancedSubObjRef& Ref){ OutObjects.Add(Ref); });
		}
	}

	static void Duplicate(UObject* OldObject, UObject* NewObject, TMap<UObject*, UObject*>& ReferenceReplacementMap, TArray<UObject*>& DuplicatedObjects);

private:
	static void GetInstancedSubObjects_Inner(FInstancedPropertyPath& PropertyPath, const uint8* ContainerAddress, TFunctionRef<void(const FInstancedSubObjRef& Ref)> OutObjects);
};
