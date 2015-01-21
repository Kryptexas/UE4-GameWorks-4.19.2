// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "LinkerPlaceholderClass.h"
#include "Blueprint/BlueprintSupport.h"

//------------------------------------------------------------------------------
ULinkerPlaceholderClass::ULinkerPlaceholderClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	, bResolvedReferences(false)
#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
{
}

//------------------------------------------------------------------------------
ULinkerPlaceholderClass::~ULinkerPlaceholderClass()
{
	// @TODO: should we assert when this happens, so we can figure out what 
	//        placeholder references are still around?
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	bool const bCircumventValidationChecks = !FBlueprintSupport::UseDeferredDependencyVerificationChecks();
	checkSlow(bCircumventValidationChecks || !HasReferences());
#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING

	// by this point, we really shouldn't have any properties left (they should 
	// have all got replaced), but just in case (so things don't blow up with a
	// missing class)...
	ReplaceTrackedReferences(UObject::StaticClass());
}

//------------------------------------------------------------------------------
IMPLEMENT_CORE_INTRINSIC_CLASS(ULinkerPlaceholderClass, UClass, 
	{
		Class->ClassAddReferencedObjects = &ULinkerPlaceholderClass::AddReferencedObjects;
		
		// @TODO: use the Class->Emit...() functions here to aid garbage 
		//        collection, so it has information on what class variables 
		//        hold onto UObject references
	}
);

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ULinkerPlaceholderClass* This = CastChecked<ULinkerPlaceholderClass>(InThis);
	//... 
	Super::AddReferencedObjects(InThis, Collector);
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Default__LinkerPlaceholderClass uses its own AddReferencedObjects function.
		ClassAddReferencedObjects = &ULinkerPlaceholderClass::AddReferencedObjects;
	}
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::Bind()
{
	ClassConstructor = InternalConstructor<ULinkerPlaceholderClass>;
	Super::Bind();

	ClassAddReferencedObjects = &ULinkerPlaceholderClass::AddReferencedObjects;
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::AddTrackedReference(UProperty* ReferencingProperty)
{
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	bool const bCircumventValidationChecks = !FBlueprintSupport::UseDeferredDependencyVerificationChecks();
	if (!bCircumventValidationChecks)
	{
		FObjectImport* PlaceholderImport = nullptr;
		if (ULinkerLoad* PropertyLinker = ReferencingProperty->GetLinker())
		{
			for (FObjectImport& Import : PropertyLinker->ImportMap)
			{
				if (Import.XObject == this)
				{
					PlaceholderImport = &Import;
					break;
				}
			}
			check(GetOuter() == PropertyLinker->LinkerRoot);
			check(PropertyLinker->LoadFlags & LOAD_DeferDependencyLoads);
		}
		// if this check hits, then we're adding dependencies after we've 
		// already resolved the placeholder (it won't be resolved again)
		check(!bResolvedReferences); 
	}	
#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING

	ReferencingProperties.Add(ReferencingProperty);
}

//------------------------------------------------------------------------------
bool ULinkerPlaceholderClass::HasReferences() const
{
	return (ReferencingProperties.Num() > 0);
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderClass::GetRefCount() const
{
	return ReferencingProperties.Num();
}

//------------------------------------------------------------------------------
bool ULinkerPlaceholderClass::HasBeenResolved() const
{
	return !HasReferences() && bResolvedReferences;
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::RemoveTrackedReference(UProperty* ReferencingProperty)
{
	//if (ReferencingProperty->IsValidLowLevel())
	{
		ReferencingProperties.Remove(ReferencingProperty);
	}
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderClass::ReplaceTrackedReferences(UClass* ReplacementClass)
{
	int32 ReplacementCount = 0;

	for (UProperty* Property : ReferencingProperties)
	{
		if (UObjectPropertyBase* ObjProperty = Cast<UObjectPropertyBase>(Property))
		{
			if (ObjProperty->PropertyClass == this)
			{
				ObjProperty->PropertyClass = ReplacementClass;
				++ReplacementCount;
			}

			UClassProperty* ClassProperty = Cast<UClassProperty>(ObjProperty);
			if ((ClassProperty != nullptr) && (ClassProperty->MetaClass == this))
			{
				ClassProperty->MetaClass = ReplacementClass;
				++ReplacementCount;
			}
		}	
		else if (UInterfaceProperty* InterfaceProp = Cast<UInterfaceProperty>(Property))
		{
			if (InterfaceProp->InterfaceClass == this)
			{
				InterfaceProp->InterfaceClass = ReplacementClass;
				++ReplacementCount;
			}
		}
	}
	ReferencingProperties.Empty();
	bResolvedReferences = true;

	return ReplacementCount;
}
