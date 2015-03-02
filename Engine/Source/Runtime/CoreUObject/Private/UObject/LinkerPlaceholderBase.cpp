// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "LinkerPlaceholderBase.h"
#include "Blueprint/BlueprintSupport.h"

//------------------------------------------------------------------------------
void FLinkerPlaceholderBase::AddReferencingProperty(UProperty* ReferencingProperty)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	UObject* ThisAsObject = GetPlaceholderAsUObject();
	check(ThisAsObject);
	FObjectImport* PlaceholderImport = nullptr;
	if (ULinkerLoad* PropertyLinker = ReferencingProperty->GetLinker())
	{
		for (FObjectImport& Import : PropertyLinker->ImportMap)
		{
			if (Import.XObject == ThisAsObject)
			{
				PlaceholderImport = &Import;
				break;
			}
		}
		check(ThisAsObject->GetOuter() == PropertyLinker->LinkerRoot);
		check((PropertyLinker->LoadFlags & LOAD_DeferDependencyLoads) || FBlueprintSupport::IsResolvingDeferredDependenciesDisabled());
	}
	// if this check hits, then we're adding dependencies after we've 
	// already resolved the placeholder (it won't be resolved again)
	check(!bResolvedReferences);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingProperties.Add(ReferencingProperty);
}

//------------------------------------------------------------------------------
bool FLinkerPlaceholderBase::HasReferences() const
{
	return (GetRefCount() > 0);
}

//------------------------------------------------------------------------------
int32 FLinkerPlaceholderBase::GetRefCount() const
{
	return ReferencingProperties.Num();
}

//------------------------------------------------------------------------------
bool FLinkerPlaceholderBase::HasBeenResolved() const
{
	return !HasReferences() && bResolvedReferences;
}

//------------------------------------------------------------------------------
void FLinkerPlaceholderBase::RemovePropertyReference(UProperty* ReferencingProperty)
{
	ReferencingProperties.Remove(ReferencingProperty);
}
