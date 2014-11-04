// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"

#include "DiffUtils.h"
#include "EditorCategoryUtils.h"
#include "ObjectEditorUtils.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

static const UProperty* Resolve( const UStruct* Class, FName PropertyName )
{
	for (TFieldIterator<UProperty> PropertyIt(Class); PropertyIt; ++PropertyIt)
	{
		if( PropertyIt->GetFName() == PropertyName )
		{
			return *PropertyIt;
		}
	}

	return nullptr;
}

static FPropertySoftPathSet GetPropertyNameSet(const UObject* ForObj)
{
	return FPropertySoftPathSet(DiffUtils::GetVisiblePropertiesInOrderDeclared(ForObj));
}

FResolvedProperty FPropertySoftPath::Resolve(const UObject* Object) const
{
	// dig into the object, finding nested objects, etc:
	const UProperty* Property = ::Resolve(Object->GetClass(), PropertyChain[0]);
	// @todo: recurse for objects and structs:
	return FResolvedProperty(Object, Property);
}

FPropertyPath FPropertySoftPath::ResolvePath(const UObject* Object) const
{
	const UStruct* ClassOverride = nullptr;
	FPropertyPath Ret;
	for( const auto& Property : PropertyChain )
	{
		const UProperty* ResolvedProperty = ::Resolve(ClassOverride ? ClassOverride : Object->GetClass(), Property);
		if (const UObjectProperty* ObjectProperty = Cast<UObjectProperty>(ResolvedProperty))
		{
			// At the moment the cast of Object to void* just avoids an overzealous assert in ContainerPtrToValuePtr (for some of our
			// properties, the outer is a UStruct, not a UClass). At some point I want to support diffing of individual array elements, 
			// and that will necessitate the container be a void* anyway:
			const UObject* const* BaseObject = reinterpret_cast<const UObject* const*>( ObjectProperty->ContainerPtrToValuePtr<void>( static_cast<const void*>(Object)) );
			if( BaseObject && *BaseObject )
			{
				Object = *BaseObject;
			}
			ClassOverride = nullptr;
		}
		else if( const UStructProperty* StructProperty = Cast<UStructProperty>(ResolvedProperty) )
		{
			ClassOverride = StructProperty->Struct;
		}
		FPropertyInfo Info = { ResolvedProperty, -1 };
		Ret.AddProperty(Info);
	}
	return Ret;
}

const UObject* DiffUtils::GetCDO(const UBlueprint* ForBlueprint)
{
	if (!ForBlueprint
		|| !ForBlueprint->GeneratedClass)
	{
		return NULL;
	}

	return ForBlueprint->GeneratedClass->ClassDefaultObject;
}

void DiffUtils::CompareUnrelatedObjects(const UObject* A, UObject const* B, TArray<FPropertySoftPath>& OutDifferingProperties)
{
	FPropertySoftPathSet PropertiesInA = GetPropertyNameSet(A);
	FPropertySoftPathSet PropertiesInB = GetPropertyNameSet(B);

	// any properties in A that aren't in B are differing:
	OutDifferingProperties.Append(PropertiesInA.Difference(PropertiesInB).Array());

	// and the converse:
	OutDifferingProperties.Append(PropertiesInB.Difference(PropertiesInA).Array());

	// for properties in common, dig out the uproperties and determine if they're identical:
	if (A && B)
	{
		FPropertySoftPathSet Common = PropertiesInA.Intersect(PropertiesInB);
		for (const auto& PropertyName : Common)
		{
			FResolvedProperty AProp = PropertyName.Resolve(A);
			FResolvedProperty BProp = PropertyName.Resolve(B);

			check(AProp != FResolvedProperty() && BProp != FResolvedProperty());
			if (!DiffUtils::Identical(AProp, BProp))
			{
				OutDifferingProperties.Add(PropertyName);
			}
		}
	}
}

void DiffUtils::CompareUnrelatedSCS(const UBlueprint* A, const UBlueprint* B, TArray< FSCSDiffEntry >& OutDifferingEntries, TArray< int >* OptionalOutSortKeys)
{
	const auto FindObjectPropertyByName = [](const UObject* Instance, FName Name) -> UObject const*
	{
		for (TFieldIterator<UProperty> PropertyIter(Instance->GetClass()); PropertyIter; ++PropertyIter)
		{
			if (PropertyIter->GetFName() == Name)
			{
				const UObject* const* SCS = PropertyIter->ContainerPtrToValuePtr<const UObject*>(Instance);
				return SCS ? *SCS : nullptr;
			}
		}
		return nullptr;
	};

	FName SCS = TEXT("SimpleConstructionScript");
	UObject const* SCSA = FindObjectPropertyByName(A, SCS);
	UObject const* SCSB = FindObjectPropertyByName(B, SCS);

	// we get bitten by const shallowness here, avoid mutation:
	const TArray<USCS_Node*> NodesInA = A && A->SimpleConstructionScript ? A->SimpleConstructionScript->GetAllNodes() : TArray<USCS_Node*>();
	const TArray<USCS_Node*> NodesInB = B && B->SimpleConstructionScript ? B->SimpleConstructionScript->GetAllNodes() : TArray<USCS_Node*>();

	const auto FindByName = [](const TArray<USCS_Node*>& NodeList, FName Name) -> USCS_Node*
	{
		for (auto Node : NodeList)
		{
			if (Node->VariableName == Name)
			{
				return Node;
			}
		}
		return nullptr;
	};

	int32 SortKey = 0;
	for (auto NodeInA : NodesInA)
	{
		auto NodeInB = FindByName(NodesInB, NodeInA->VariableName);
		if (NodeInB)
		{
			// diff NodeA vs NodeB:
			TArray<FPropertySoftPath> DifferingProperties;

			DiffUtils::CompareUnrelatedObjects(A, B, DifferingProperties);
			for (auto Property : DifferingProperties)
			{
				FSCSDiffEntry Diff = { NodeInA->VariableName, Property.LastPropertyName() };
				OutDifferingEntries.Push(Diff);

				if (OptionalOutSortKeys)
				{
					OptionalOutSortKeys->Push(SortKey);
				}
			}
		}
		else
		{
			// Node A was added (removed from node B):
			FSCSDiffEntry Diff = { NodeInA->VariableName, FName() };
			OutDifferingEntries.Push(Diff);

			if (OptionalOutSortKeys)
			{
				OptionalOutSortKeys->Push(SortKey);
			}
		}

		++SortKey;
	}

	// add nodes that were added in B:
	for (auto NodeInB : NodesInB)
	{
		if (FindByName(NodesInA, NodeInB->VariableName) == nullptr)
		{
			// Node B was added:
			FSCSDiffEntry Diff = { NodeInB->VariableName, FName() };
			OutDifferingEntries.Push(Diff);

			if (OptionalOutSortKeys)
			{
				OptionalOutSortKeys->Push(SortKey);
			}
		}

		++SortKey;
	}
}

bool DiffUtils::Identical(const FResolvedProperty& AProp, const FResolvedProperty& BProp)
{
	const void* AValue = AProp.Property->ContainerPtrToValuePtr<void>(AProp.Object);
	const void* BValue = BProp.Property->ContainerPtrToValuePtr<void>(BProp.Object);

	return AProp.Property->Identical(AValue, BValue, PPF_DeepComparison);
}

TArray<FPropertySoftPath> DiffUtils::GetVisiblePropertiesInOrderDeclared(const UObject* ForObj, const TArray<FName>& Scope /*= TArray<FName>()*/)
{
	TArray<FPropertySoftPath> Ret;
	if (ForObj)
	{
		const UClass* Class = ForObj->GetClass();
		TSet<FString> HiddenCategories = FEditorCategoryUtils::GetHiddenCategories(Class);
		for (TFieldIterator<UProperty> PropertyIt(Class); PropertyIt; ++PropertyIt)
		{
			FName CategoryName = FObjectEditorUtils::GetCategoryFName(*PropertyIt);
			if (!HiddenCategories.Contains(CategoryName.ToString()))
			{
				if (PropertyIt->PropertyFlags&CPF_Edit)
				{
					TArray<FName> NewPath(Scope);
					NewPath.Push(PropertyIt->GetFName());
					if (const UObjectProperty* ObjectProperty = Cast<UObjectProperty>(*PropertyIt))
					{
						const UObject* const* BaseObject = reinterpret_cast<const UObject* const*>( ObjectProperty->ContainerPtrToValuePtr<void>(ForObj) );
						if (BaseObject && *BaseObject)
						{
							Ret.Append( GetVisiblePropertiesInOrderDeclared(*BaseObject, NewPath) );
						}
					}
					else
					{
						Ret.Push(NewPath);
					}
				}
			}
		}
	}
	return Ret;
}

TArray<FPropertyPath> DiffUtils::ResolveAll(const UObject* Object, const TArray<FPropertySoftPath>& InSoftProperties)
{
	TArray< FPropertyPath > Ret;
	for (const auto& Path : InSoftProperties)
	{
		Ret.Push(Path.ResolvePath(Object));
	}
	return Ret;
}
