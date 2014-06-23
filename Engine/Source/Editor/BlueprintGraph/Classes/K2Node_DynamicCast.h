// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node.h"
#include "K2Node_DynamicCast.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DynamicCast : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** The type that the input should try to be cast to */
	UPROPERTY()
	TSubclassOf<class UObject>  TargetType;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.Cast_16x"); }
	// End UEdGraphNode interface

	// UK2Node interface
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	virtual bool HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	// End of UK2Node interface

	/** Get the 'valid cast' exec pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetValidCastPin() const;

	/** Get the 'invalid cast' exec pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetInvalidCastPin() const;

	/** Get the cast result pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetCastResultPin() const;

	/** Get the input object to be casted pin */
	BLUEPRINTGRAPH_API virtual UEdGraphPin* GetCastSourcePin() const;
};

