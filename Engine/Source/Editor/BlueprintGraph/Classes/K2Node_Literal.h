// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Literal.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Literal : public UK2Node
{
	GENERATED_UCLASS_BODY()

private:
	/** If this is an object reference literal, keep a reference here so that it can be updated as objects move around */
	UPROPERTY()
	class UObject* ObjectRef;

public:

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool ShouldOverridePinNames() const OVERRIDE { return true; }
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual AActor* GetReferencedLevelActor() const OVERRIDE;
	virtual bool DrawNodeAsVariable() const OVERRIDE { return true; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual void PostReconstructNode() OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	/** Accessor for the value pin of the node */
	BLUEPRINTGRAPH_API UEdGraphPin* GetValuePin() const;

	/** Sets the LiteralValue for the pin, and changes the pin type, if necessary */
	BLUEPRINTGRAPH_API void SetObjectRef(UObject* NewValue);

	/** Gets the referenced object */
	BLUEPRINTGRAPH_API UObject* GetObjectRef() const { return ObjectRef; }

#endif
};

