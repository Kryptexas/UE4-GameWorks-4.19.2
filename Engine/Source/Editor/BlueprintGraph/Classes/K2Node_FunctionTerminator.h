// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_FunctionTerminator.generated.h"

UCLASS(abstract, MinimalAPI)
class UK2Node_FunctionTerminator : public UK2Node_EditablePinBase
{
	GENERATED_UCLASS_BODY()

	/** 
	 * The source class that defines the signature, if it is getting that from elsewhere (e.g. interface, base class etc). 
	 * If NULL, this is a newly created function.
	 */
	UPROPERTY()
	TSubclassOf<class UObject>  SignatureClass;

	/** The name of the signature function. */
	UPROPERTY()
	FName SignatureName;


	// Begin UEdGraphNode interface
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	// End UK2Node interface
};

