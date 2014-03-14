// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_LiveEditObject.generated.h"

UCLASS(DependsOn=ULiveEditorTypes,MinimalAPI)
class UK2Node_LiveEditObject : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) OVERRIDE;
	// End UEdGraphNode interface.

	// Begin UK2Node interface
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	/** Create new pins to show LiveEditable properties on archetype */
	void CreatePinsForClass(UClass* InClass);
	/** See if this is a spawn variable pin, or a 'default' pin */
	bool IsSpawnVarPin(UEdGraphPin* Pin);

	/** Get the class that we are going to spawn */
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;

	FString GetEventName() const;

public:
	static FString LiveEditableVarPinCategory;

private:

	/** Get the blueprint input pin */
	UEdGraphPin* GetBlueprintPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
	UEdGraphPin *GetBaseClassPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;

	UEdGraphPin *GetThenPin() const;
	UEdGraphPin *GetVariablePin() const;
	UEdGraphPin *GetDescriptionPin() const;
	UEdGraphPin *GetPermittedBindingsPin() const;
	UEdGraphPin *GetOnMidiInputPin() const;
	UEdGraphPin *GetDeltaMultPin() const;
	UEdGraphPin *GetShouldClampPin() const;
	UEdGraphPin *GetClampMinPin() const;
	UEdGraphPin *GetClampMaxPin() const;
	UFunction *GetEventMIDISignature() const;

	/**
	 * Takes the specified "MutatablePin" and sets its 'PinToolTip' field (according
	 * to the specified description)
	 * 
	 * @param   MutatablePin	The pin you want to set tool-tip text on
	 * @param   PinDescription	A string describing the pin's purpose
	 */
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;
#endif
};
