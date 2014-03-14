// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_DynamicCast.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DynamicCast : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** The type that the input should try to be cast to */
	UPROPERTY()
	TSubclassOf<class UObject>  TargetType;


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Cast_16x"); }
	// End UEdGraphNode interface

	// UK2Node interface
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End of UK2Node interface

	/** Get the 'valid cast' exec pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetValidCastPin() const;

	/** Get the 'invalid cast' exec pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetInvalidCastPin() const;

	/** Get the cast result pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetCastResultPin() const;

	/** Get the input object to be casted pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetCastSourcePin() const;
#endif
};

