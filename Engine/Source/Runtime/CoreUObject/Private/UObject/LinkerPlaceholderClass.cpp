// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "LinkerPlaceholderFunction.h"

//------------------------------------------------------------------------------
ULinkerPlaceholderClass::ULinkerPlaceholderClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
#if WITH_HOT_RELOAD_CTORS
	ClassVTableHelperCtorCaller = InternalVTableHelperCtorCaller<ULinkerPlaceholderClass>;
#endif // WITH_HOT_RELOAD_CTORS
	Super::Bind();

	ClassAddReferencedObjects = &ULinkerPlaceholderClass::AddReferencedObjects;
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

	bResolvedReferences = true;
	return ReplacementCount;
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderClass::GetRefCount() const
{
	return FLinkerPlaceholderBase::GetRefCount() + ReferencingScriptExpressions.Num();
}
