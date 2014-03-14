// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_StructOperation.generated.h"

UCLASS(MinimalAPI, abstract)
class UK2Node_StructOperation : public UK2Node_Variable
{
	GENERATED_UCLASS_BODY()

	/** Class that this variable is defined in.  */
	UPROPERTY()
	UScriptStruct* StructType;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	//virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End UEdGraphNode interface

	// UK2Node interface
	//virtual bool DrawNodeAsVariable() const OVERRIDE { return true; }
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE {}
	// End of UK2Node interface

protected:
	// Updater for subclasses that allow hiding pins
	struct FStructOperationOptionalPinManager : public FOptionalPinManager
	{
		// FOptionalPinsUpdater interface
		virtual void GetRecordDefaults(UProperty* TestProperty, FOptionalPinFromProperty& Record) const OVERRIDE
		{
			Record.bCanToggleVisibility = true;
			Record.bShowPin = true;
		}
		// End of FOptionalPinsUpdater interfac
	};

#endif
};

