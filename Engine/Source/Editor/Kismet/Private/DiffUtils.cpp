// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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

void DiffUtils::CompareUnrelatedSCS(const UBlueprint* Old, const TArray< FSCSResolvedIdentifier >& OldHierarchy, const UBlueprint* New, const TArray< FSCSResolvedIdentifier >& NewHierarchy, FSCSDiffRoot& OutDifferingEntries )
{
	const auto FindEntry = [](TArray< FSCSResolvedIdentifier > const& InArray, const FSCSIdentifier* Value) -> const FSCSResolvedIdentifier*
	{
		for (const auto& Node : InArray)
		{
			if (Node.Identifier.Name == Value->Name )
			{
				return &Node;
			}
		}
		return nullptr;
	};

	for (const auto& OldNode : OldHierarchy)
	{
		const FSCSResolvedIdentifier* NewEntry = FindEntry(NewHierarchy, &OldNode.Identifier);

		if (NewEntry != nullptr)
		{
			// @todo doc: did properties change?
			TArray<FPropertySoftPath> DifferingProperties;
			DiffUtils::CompareUnrelatedObjects(OldNode.Object, NewEntry->Object, DifferingProperties);
			for (const auto& Property : DifferingProperties)
			{
				FSCSDiffEntry Diff = { OldNode.Identifier, ETreeDiffType::NODE_PROPERTY_CHANGED, Property };
				OutDifferingEntries.Entries.Push(Diff);
			}

			// did it move?


			// no change! Do nothing.
		}
		else
		{
			// not found in the new data, must have been deleted:
			FSCSDiffEntry Entry = { OldNode.Identifier, ETreeDiffType::NODE_REMOVED, FPropertySoftPath() };
			OutDifferingEntries.Entries.Push( Entry );
		}
	}

	for (const auto& NewNode : NewHierarchy)
	{
		const FSCSResolvedIdentifier* OldEntry = FindEntry(OldHierarchy, &NewNode.Identifier);

		if (OldEntry == nullptr)
		{
			FSCSDiffEntry Entry = { NewNode.Identifier, ETreeDiffType::NODE_ADDED, FPropertySoftPath() };
			OutDifferingEntries.Entries.Push( Entry );
		}
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
