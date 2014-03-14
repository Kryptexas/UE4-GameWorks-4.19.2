// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_CastToInterface.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CastToInterface : public UK2Node_DynamicCast
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	
	// UEdGraphNode interface
	//virtual void AllocateDefaultPins() OVERRIDE;
	//virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	//virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End of UK2Node interface

	/** Get the 'valid interface' pin */
	//BLUEPRINTGRAPH_API UEdGraphPin* GetValidInterfacePin() const;
	/** Get the 'invalid interface' pin */
	//BLUEPRINTGRAPH_API UEdGraphPin* GetInvalidInterfacePin() const;
	/** Get the interface result pin */
	//BLUEPRINTGRAPH_API UEdGraphPin* GetInterfaceResultPin() const;
#endif
};

