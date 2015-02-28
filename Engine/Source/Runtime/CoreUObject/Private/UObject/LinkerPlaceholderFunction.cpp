// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "LinkerPlaceholderFunction.h"

//------------------------------------------------------------------------------
ULinkerPlaceholderFunction::ULinkerPlaceholderFunction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//------------------------------------------------------------------------------
ULinkerPlaceholderFunction::~ULinkerPlaceholderFunction()
{
}

//------------------------------------------------------------------------------
IMPLEMENT_CORE_INTRINSIC_CLASS(ULinkerPlaceholderFunction, UFunction,
	{
	}
);

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderFunction::ReplaceTrackedReferences(UFunction* ReplacementObj)
{
	int32 ReplacementCount = 0;
	for (UProperty* Property : ReferencingProperties)
	{
		if (auto DelegateProperty = Cast<UDelegateProperty>(Property))
		{
			if (DelegateProperty->SignatureFunction == this)
			{
				DelegateProperty->SignatureFunction = ReplacementObj;
				++ReplacementCount;
			}
		}
		else if (auto MulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Property))
		{
			MulticastDelegateProperty->SignatureFunction = ReplacementObj;
			++ReplacementCount;
		}
		else
		{
			ensure(false);
		}
	}
	ReferencingProperties.Empty();

	for (UFunction** ScriptRefPtr : ReferencingScriptExpressions)
	{
		UFunction*& ScriptRef = *ScriptRefPtr;
		if (ScriptRef == this)
		{
			ScriptRef = ReplacementObj;
			++ReplacementCount;
		}
	}
	ReferencingScriptExpressions.Empty();

	bResolvedReferences = true;
	return ReplacementCount;
}

//------------------------------------------------------------------------------
void ULinkerPlaceholderFunction::AddReferencingScriptExpr(ULinkerPlaceholderFunction** ExpressionPtr)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(*ExpressionPtr == this);
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingScriptExpressions.Add((UFunction**)ExpressionPtr);
}

//------------------------------------------------------------------------------
int32 ULinkerPlaceholderFunction::GetRefCount() const
{
	return FLinkerPlaceholderBase::GetRefCount() + ReferencingScriptExpressions.Num();
}