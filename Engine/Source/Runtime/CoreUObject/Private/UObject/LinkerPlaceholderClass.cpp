// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "LinkerPlaceholderClass.h"
#include "Blueprint/BlueprintSupport.h"

/*******************************************************************************
 *FPlaceholderContainerTracker
 ******************************************************************************/

/**  */
struct FPlaceholderContainerTracker : TThreadSingleton<FPlaceholderContainerTracker>
{
	TArray<UObject*> PerspectiveReferencerStack;
};

//------------------------------------------------------------------------------
FScopedPlaceholderContainerTracker::FScopedPlaceholderContainerTracker(UObject* InPlaceholderContainerCandidate)
	: PlaceholderReferencerCandidate(InPlaceholderContainerCandidate)
{
	FPlaceholderContainerTracker::Get().PerspectiveReferencerStack.Add(InPlaceholderContainerCandidate);
}

//------------------------------------------------------------------------------
FScopedPlaceholderContainerTracker::~FScopedPlaceholderContainerTracker()
{
	UObject* StackTop = FPlaceholderContainerTracker::Get().PerspectiveReferencerStack.Pop();
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(StackTop == PlaceholderReferencerCandidate);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
}

/*******************************************************************************
 * FLinkerPlaceholderBase
 ******************************************************************************/

namespace LinkerPlaceholderObjectImpl
{
	/**
	 * Traces the specified property up its outer chain, all the way up to the 
	 * OwnerClass. Constructs a UProperty path along the way, so that we can use
	 * traverse a property tree down, from the class owner down.
	 * 
	 * @param  LeafProperty	The property you want a path for.
	 * @param  ChainOut		Output array that tracks the specified property back up its outer chain (first entry should be what was passed in LeafProperty). 
	 */
	static void BuildPropertyChain(const UProperty* LeafProperty, TArray<const UProperty*>& ChainOut);

	/**
	 * A recursive method that replaces all leaf references to this object with 
	 * the supplied ReplacementValue.
	 *
	 * This function recurses the property chain (from class owner down) because 
	 * at the time of AddReferencingPropertyValue() we cannot know/record the 
	 * address/index of array properties (as they may change during array 
	 * re-allocation or compaction). So we must follow the property chain and 
	 * check every UArrayProperty member for references to this (hence, the need
	 * for this recursive function).
	 * 
	 * @param  PropertyChain    An ascending outer chain, where the property at index zero is the leaf (referencer) property.
	 * @param  ChainIndex		An index into the PropertyChain that this call should start at and iterate DOWN to zero.
	 * @param  ValueAddress     The memory address of the value corresponding to the property at ChainIndex.
	 * @param  OldValue
	 * @param  ReplacementValue	The new object to replace all references to this with.
	 * @return The number of references that were replaced.
	 */
	static int32 ResolvePlaceholderValues(const TArray<const UProperty*>& PropertyChain, int32 ChainIndex, uint8* ValueAddress, UObject* OldValue, UObject* ReplacementValue);
}

//------------------------------------------------------------------------------
static void LinkerPlaceholderObjectImpl::BuildPropertyChain(const UProperty* LeafProperty, TArray<const UProperty*>& ChainOut)
{
	ChainOut.Empty();
	ChainOut.Add(LeafProperty);

	UClass* ClassOwner = LeafProperty->GetOwnerClass();

	UObject* PropertyOuter = LeafProperty->GetOuter();
	while ((PropertyOuter != nullptr) && (PropertyOuter != ClassOwner))
	{
		if (const UProperty* PropertyOwner = Cast<const UProperty>(PropertyOuter))
		{
			ChainOut.Add(PropertyOwner);
		}
		PropertyOuter = PropertyOuter->GetOuter();
	}
}

//------------------------------------------------------------------------------
static int32 LinkerPlaceholderObjectImpl::ResolvePlaceholderValues(const TArray<const UProperty*>& PropertyChain, int32 ChainIndex, uint8* ValueAddress, UObject* OldValue, UObject* ReplacementValue)
{
	int32 ReplacementCount = 0;

	for (int32 PropertyIndex = ChainIndex; PropertyIndex >= 0; --PropertyIndex)
	{
		const UProperty* Property = PropertyChain[PropertyIndex];
		if (PropertyIndex == 0)
		{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			check(Property->IsA<UObjectProperty>());
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

			const UObjectProperty* ReferencingProperty = (const UObjectProperty*)Property;
			if (ReferencingProperty->GetObjectPropertyValue(ValueAddress) == OldValue)
			{
				// @TODO: use an FArchiver with ReferencingProperty->SerializeItem() 
				//        so that we can utilize CheckValidObject()
				ReferencingProperty->SetObjectPropertyValue(ValueAddress, ReplacementValue);
				// @TODO: unfortunately, this is currently protected
				//ReferencingProperty->CheckValidObject(ValueAddress);

				++ReplacementCount;
			}
		}
		else if (const UArrayProperty* ArrayProperty = Cast<const UArrayProperty>(Property))
		{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			const UProperty* NextProperty = PropertyChain[PropertyIndex - 1];
			check(NextProperty == ArrayProperty->Inner);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

			// because we can't know which array entry was set with a reference 
			// to this object, we have to comb through them all
			FScriptArrayHelper ArrayHelper(ArrayProperty, ValueAddress);
			for (int32 ArrayIndex = 0; ArrayIndex < ArrayHelper.Num(); ++ArrayIndex)
			{
				uint8* MemberAddress = ArrayHelper.GetRawPtr(ArrayIndex);
				ReplacementCount += ResolvePlaceholderValues(PropertyChain, PropertyIndex - 1, MemberAddress, OldValue, ReplacementValue);
			}

			// the above recursive call chewed through the rest of the
			// PropertyChain, no need to keep on here
			break;
		}
		else
		{
			const UProperty* NextProperty = PropertyChain[PropertyIndex - 1];
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			check(NextProperty->GetOuter() == Property);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			ValueAddress = Property->ContainerPtrToValuePtr<uint8>(ValueAddress, /*ArrayIndex =*/0);
		}
	}

	return ReplacementCount;
}

/*******************************************************************************
 * ULinkerPlaceholderClass
 ******************************************************************************/

//------------------------------------------------------------------------------
ULinkerPlaceholderClass::ULinkerPlaceholderClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bResolvedReferences(false)
{
}

//------------------------------------------------------------------------------
ULinkerPlaceholderClass::~ULinkerPlaceholderClass()
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(!HasReferences());
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

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
void ULinkerPlaceholderClass::AddReferencingProperty(UProperty* ReferencingProperty)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
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
		check((PropertyLinker->LoadFlags & LOAD_DeferDependencyLoads) || FBlueprintSupport::IsResolvingDeferredDependenciesDisabled());
	}
	// if this check hits, then we're adding dependencies after we've 
	// already resolved the placeholder (it won't be resolved again)
	check(!bResolvedReferences); 	
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingProperties.Add(ReferencingProperty);
}

//------------------------------------------------------------------------------
bool ULinkerPlaceholderClass::AddReferencingPropertyValue(const UObjectProperty* ReferencingProperty, void* DataPtr)
{
	TArray<UObject*>& PossibleReferencers = FPlaceholderContainerTracker::Get().PerspectiveReferencerStack;

	UObject* FoundReferencer = nullptr;
	// iterate backwards because this is meant to act as a stack, where the last
	// entry is most likely the one we're looking for
	for (int32 CandidateIndex = PossibleReferencers.Num() - 1; CandidateIndex >= 0; --CandidateIndex)
	{
		UObject* ReferencerCandidate = PossibleReferencers[CandidateIndex];

		UClass* CandidateClass = ReferencerCandidate->GetClass();
		UClass* PropOwnerClass = ReferencingProperty->GetOwnerClass();

		if (CandidateClass->IsChildOf(PropOwnerClass))
		{
			FoundReferencer = ReferencerCandidate;
			break;
		}
	}

#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(ReferencingProperty->GetObjectPropertyValue(DataPtr) == this);
	check(FoundReferencer != nullptr);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	if (FoundReferencer != nullptr)
	{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
		// @TODO: verify that DataPtr belongs to FoundReferencer
// 		uint8* ContainerStart = (uint8*)FoundReferencer;
// 		uint8* ContainerEnd   = ContainerStart + FoundReferencer->GetClass()->GetStructureSize();
// 		// check that we picked the right container object, and that DataPtr exists within it 
// 		check(DataPtr >= ContainerStart && DataPtr <= ContainerEnd);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

		ReferencingContainers.FindOrAdd(FoundReferencer).Add(ReferencingProperty);
	}
	return (FoundReferencer != nullptr);
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderClass::ResolvePlaceholderPropertyValues(UObject* NewObjectValue)
{
	int32 ResolvedTotal = 0;

	UObject* ThisAsUObject = this;// GetPlaceholderAsUObject();
	for (auto& ReferencingPair : ReferencingContainers)
	{
		TWeakObjectPtr<UObject> ContainerPtr = ReferencingPair.Key;
		if (!ContainerPtr.IsValid())
		{
			continue;
		}
		UObject* Container = ContainerPtr.Get();

		for (const UObjectProperty* Property : ReferencingPair.Value)
		{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			check(Property->GetOwnerClass() == Container->GetClass());
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

			TArray<const UProperty*> PropertyChain;
			LinkerPlaceholderObjectImpl::BuildPropertyChain(Property, PropertyChain);
			const UProperty* OutermostProperty = PropertyChain.Last();

			uint8* OutermostAddress = OutermostProperty->ContainerPtrToValuePtr<uint8>((uint8*)Container, /*ArrayIndex =*/0);
			int32 ResolvedCount = LinkerPlaceholderObjectImpl::ResolvePlaceholderValues(PropertyChain, PropertyChain.Num() - 1, OutermostAddress, ThisAsUObject, NewObjectValue);
			ResolvedTotal += ResolvedCount;

#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
			// we expect that (because we have had ReferencingProperties added) 
			// there should be at least one reference that is resolved... if 
			// there were none, then a property could have changed its value 
			// after it was set to this
			// 
			// NOTE: this may seem it can be resolved by properties removing 
			//       themselves from ReferencingProperties, but certain 
			//       properties may be the inner of a UArrayProperty (meaning 
			//       there could be multiple references per property)... we'd 
			//       have to inc/decrement a property ref-count to resolve that 
			//       scenario
			check(ResolvedCount > 0);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
		}
	}

	return ResolvedTotal;
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::AddReferencingScriptExpr(ULinkerPlaceholderClass** ExpressionPtr)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(*ExpressionPtr == this);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingScriptExpressions.Add((UClass**)ExpressionPtr);
}

//------------------------------------------------------------------------------
bool ULinkerPlaceholderClass::HasReferences() const
{
	return (GetRefCount() > 0);
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderClass::GetRefCount() const
{
	return ReferencingProperties.Num() + ReferencingScriptExpressions.Num() + ReferencingContainers.Num();
}

//------------------------------------------------------------------------------
bool ULinkerPlaceholderClass::HasBeenResolved() const
{
	return !HasReferences() && bResolvedReferences;
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderClass::RemovePropertyReference(UProperty* ReferencingProperty)
{
	ReferencingProperties.Remove(ReferencingProperty);
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

	for (UClass** ScriptRefPtr : ReferencingScriptExpressions)
	{
		UClass*& ScriptRef = *ScriptRefPtr;
		if (ScriptRef == this)
		{
			ScriptRef = ReplacementClass;
			++ReplacementCount;
		}
	}
	ReferencingScriptExpressions.Empty();

	ReplacementCount += ResolvePlaceholderPropertyValues(ReplacementClass);
	ReferencingContainers.Empty();

	bResolvedReferences = true;
	return ReplacementCount;
}
