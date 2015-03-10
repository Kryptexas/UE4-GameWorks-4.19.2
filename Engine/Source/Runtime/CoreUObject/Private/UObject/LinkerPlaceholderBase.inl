// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*******************************************************************************
 * TLinkerImportPlaceholder<>
 ******************************************************************************/

//------------------------------------------------------------------------------
template<class PlaceholderType>
int32 TLinkerImportPlaceholder<PlaceholderType>::ResolveAllPlaceholderReferences(UObject* ReplacementObj)
{
	PlaceholderType* TypeCheckedReplacement = CastChecked<PlaceholderType>(ReplacementObj, ECastCheckedType::NullAllowed);

	int32 ReplacementCount = ResolvePropertyReferences(TypeCheckedReplacement);
	ReplacementCount += ResolveScriptReferences(TypeCheckedReplacement);
	ReplacementCount += FLinkerPlaceholderBase::ResolveAllPlaceholderReferences(ReplacementObj);

	return ReplacementCount;
}

//------------------------------------------------------------------------------
template<class PlaceholderType>
bool TLinkerImportPlaceholder<PlaceholderType>::HasKnownReferences() const
{
	return FLinkerPlaceholderBase::HasKnownReferences() || (ReferencingProperties.Num() > 0) || (ReferencingScriptExpressions.Num() > 0);
}

//------------------------------------------------------------------------------
template<class PlaceholderType>
void TLinkerImportPlaceholder<PlaceholderType>::AddReferencingProperty(UProperty* ReferencingProperty)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	UObject* ThisAsObject = GetPlaceholderAsUObject();
	check(ThisAsObject != nullptr);

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
		check(ThisAsObject->GetOutermost() == PropertyLinker->LinkerRoot);
		check((PropertyLinker->LoadFlags & LOAD_DeferDependencyLoads) || FBlueprintSupport::IsResolvingDeferredDependenciesDisabled());
	}
	// if this check hits, then we're adding dependencies after we've 
	// already resolved the placeholder (it won't be resolved again)
	check(!IsMarkedResolved());
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingProperties.Add(ReferencingProperty);
}

//------------------------------------------------------------------------------
template<class PlaceholderType>
void TLinkerImportPlaceholder<PlaceholderType>::RemoveReferencingProperty(UProperty* ReferencingProperty)
{
	ReferencingProperties.Remove(ReferencingProperty);
}

//------------------------------------------------------------------------------
template<class PlaceholderType>
void TLinkerImportPlaceholder<PlaceholderType>::AddReferencingScriptExpr(PlaceholderType** ExpressionPtr)
{
#if USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS
	check(*ExpressionPtr == GetPlaceholderAsUObject());
#endif // USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS

	ReferencingScriptExpressions.Add(ExpressionPtr);
}

//------------------------------------------------------------------------------
template<class PlaceholderType>
int32 TLinkerImportPlaceholder<PlaceholderType>::ResolvePropertyReferences(PlaceholderType* ReplacementObj) 
	// requires template specialization (technically not "pure virtual"):
	PURE_VIRTUAL(TLinkerImportPlaceholder<PlaceholderType>::ResolvePropertyReferences, return 0;)

//------------------------------------------------------------------------------
template<class PlaceholderType>
int32 TLinkerImportPlaceholder<PlaceholderType>::ResolveScriptReferences(PlaceholderType* ReplacementObj)
{
	int32 ReplacementCount = 0;
	PlaceholderType* PlaceholderObj = CastChecked<PlaceholderType>(GetPlaceholderAsUObject());

	for (PlaceholderType** ScriptRefPtr : ReferencingScriptExpressions)
	{
		PlaceholderType*& ScriptRef = *ScriptRefPtr;
		if (ScriptRef == PlaceholderObj)
		{
			ScriptRef = ReplacementObj;
			++ReplacementCount;
		}
	}

	ReferencingScriptExpressions.Empty();
	return ReplacementCount;
}
