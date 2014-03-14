// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_SpawnActor.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SpawnActor : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual bool IsDeprecated() const OVERRIDE;
	virtual bool ShouldWarnOnDeprecation() const OVERRIDE;
	virtual FString GetDeprecationMessage() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.SpawnActor_16x"); }
	// End UEdGraphNode interface.

	// Begin UK2Node interface
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual bool HasExternalBlueprintDependencies() const OVERRIDE;
	// End UK2Node interface


	/** Create new pins to show properties on archetype */
	void CreatePinsForClass(UClass* InClass);
	/** See if this is a spawn variable pin, or a 'default' pin */
	BLUEPRINTGRAPH_API bool IsSpawnVarPin(UEdGraphPin* Pin);

	/** Get the then output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetThenPin() const;
	/** Get the blueprint input pin */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetBlueprintPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
	/** Get the world context input pin, can return NULL */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetWorldContextPin() const;
	/** Get the spawn transform input pin */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetSpawnTransformPin() const;
	/** Get the no collision fail input pin */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetNoCollisionFailPin() const;
	/** Get the result output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetResultPin() const;

private:
	/** Get the class that we are going to spawn */
	BLUEPRINTGRAPH_API UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;

	/** Tooltip text for this node. */
	FString NodeTooltip;
#endif
};
