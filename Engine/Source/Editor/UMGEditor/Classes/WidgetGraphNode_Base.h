// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetGraphNode_Base.generated.h"

UCLASS(Abstract, MinimalAPI)
class UWidgetGraphNode_Base : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PinOptions, EditFixedSize)
	TArray<FOptionalPinFromProperty> ShowPinForProperties;

	//// UObject interface
	UMGEDITOR_API virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	//// End of UObject interface

	//// UEdGraphNode interface
	UMGEDITOR_API virtual void AllocateDefaultPins() OVERRIDE;
	UMGEDITOR_API virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	//UMGEDITOR_API virtual FString GetDocumentationLink() const OVERRIDE;
	//UMGEDITOR_API virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const OVERRIDE;
	UMGEDITOR_API virtual bool ShowPaletteIconOnNode() const OVERRIDE { return false; }
	//	// End of UEdGraphNode interface


	// UK2Node interface
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual bool CanPlaceBreakpoints() const OVERRIDE { return false; }
	UMGEDITOR_API virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	UMGEDITOR_API virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const OVERRIDE;
	UMGEDITOR_API virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UK2Node interface

	// UWidgetGraphNode_Base interface

	// Gets the menu category this node belongs in
	UMGEDITOR_API virtual FString GetNodeCategory() const;

	// Create any output pins necessary for this node
	UMGEDITOR_API virtual void CreateOutputPins();

	UMGEDITOR_API void GetPinAssociatedProperty(UScriptStruct* NodeType, UEdGraphPin* InputPin, UProperty*& OutProperty, int32& OutIndex);

	// customize pin data based on the input
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const;

	/** Get the animation blueprint to which this node belongs */
	UWidgetBlueprint* GetWidgetBlueprint() const { return CastChecked<UWidgetBlueprint>(GetBlueprint()); }

	// UWidgetGraphNode_Base

protected:
	friend class FWidgetBlueprintCompiler;

	// Gets the animation FNode type represented by this ed graph node
	UMGEDITOR_API UScriptStruct* GetFNodeType() const;

	// Gets the animation FNode property represented by this ed graph node
	UMGEDITOR_API UStructProperty* GetFNodeProperty() const;

	void InternalPinCreation(TArray<UEdGraphPin*>* OldPins);

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> CreateDefaultMenuEntry(FGraphContextMenuBuilder& ContextMenuBuilder) const;
};
