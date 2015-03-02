// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LinkerPlaceholderClass.h"

// Forward declarations
class FObjectInitializer;
class UObjectPropertyBase;

class ULinkerPlaceholderFunction : public UFunction, public FLinkerPlaceholderBase
{
public:
	DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR(ULinkerPlaceholderFunction, UFunction, /*TStaticFlags =*/0, CoreUObject, /*TStaticCastFlags =*/0, NO_API)

	ULinkerPlaceholderFunction(const FObjectInitializer& ObjectInitializer);
	virtual ~ULinkerPlaceholderFunction();

	// FPlaceholderBase 
	virtual int32 GetRefCount() const override;
	virtual UObject* GetPlaceholderAsUObject() override
	{
		return this;
	}
	// End of FPlaceholderBase

	int32 ReplaceTrackedReferences(UFunction* ReplacementClass);

	void AddReferencingScriptExpr(ULinkerPlaceholderFunction** ExpressionPtr);

private:

	TSet<UFunction**> ReferencingScriptExpressions;
};