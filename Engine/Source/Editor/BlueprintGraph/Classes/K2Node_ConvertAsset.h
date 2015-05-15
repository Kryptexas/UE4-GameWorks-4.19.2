// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node.h"
#include "K2Node_ConvertAsset.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ConvertAsset : public UK2Node
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TSubclassOf<class UObject>  TargetType;

	UPROPERTY()
	bool bIsAssetClass;

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const override { return true; }
	virtual bool ShouldDrawCompact() const override { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual FText GetCompactNodeTitle() const override;
	// End of UK2Node interface
};