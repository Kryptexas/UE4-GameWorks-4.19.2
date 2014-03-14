// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_FunctionEntry.generated.h"

UCLASS(MinimalAPI)
class UK2Node_FunctionEntry : public UK2Node_FunctionTerminator
{
	GENERATED_UCLASS_BODY()

	/** If specified, the function that is created for this entry point will have this name.  Otherwise, it will have the function signature's name */
	UPROPERTY()
	FName CustomGeneratedFunctionName;

	/** Any extra flags that the function may need */
	UPROPERTY()
	int32 ExtraFlags;

	/** Function metadata */
	UPROPERTY()
	struct FKismetUserDeclaredFunctionMetadata MetaData;


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual bool IsDeprecated() const OVERRIDE;
	virtual FString GetDeprecationMessage() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool DrawNodeAsEntry() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	// Begin UK2Node_EditablePinBase interface
	virtual bool CanUseRefParams() const OVERRIDE { return true; }
	// End UK2Node_EditablePinBase interface

	// Begin K2Node_FunctionTerminator interface
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) OVERRIDE;
	// End K2Node_FunctionTerminator interface

	// Removes an output pin from the node
	BLUEPRINTGRAPH_API void RemoveOutputPin(UEdGraphPin* PinToRemove);
#endif
};

