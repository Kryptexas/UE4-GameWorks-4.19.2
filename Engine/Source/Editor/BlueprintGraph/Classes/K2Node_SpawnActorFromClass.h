// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node.h"
#include "K2Node_SpawnActorFromClass.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SpawnActorFromClass : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual FString GetTooltip() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.SpawnActor_16x"); }
	// End UEdGraphNode interface.

	// Begin UK2Node interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const override;
	virtual void GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const override;
	// End UK2Node interface


	/** Create new pins to show properties on archetype */
	void CreatePinsForClass(UClass* InClass);
	/** See if this is a spawn variable pin, or a 'default' pin */
	BLUEPRINTGRAPH_API bool IsSpawnVarPin(UEdGraphPin* Pin);

	/** Get the then output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetThenPin() const;
	/** Get the blueprint input pin */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
	/** Get the world context input pin, can return NULL */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetWorldContextPin() const;
	/** Get the spawn transform input pin */	
	BLUEPRINTGRAPH_API UEdGraphPin* GetSpawnTransformPin() const;
	/** Get the spawn NoCollisionFail input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetNoCollisionFailPin() const;
	/** Get the result output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetResultPin() const;

	/** Get the class that we are going to spawn, if it's defined as default value */
	BLUEPRINTGRAPH_API UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;

private:
	/**
	 * Takes the specified "MutatablePin" and sets its 'PinToolTip' field (according
	 * to the specified description)
	 * 
	 * @param   MutatablePin	The pin you want to set tool-tip text on
	 * @param   PinDescription	A string describing the pin's purpose
	 */
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;

	/** Tooltip text for this node. */
	FString NodeTooltip;
};
