// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_StructMemberSet.h"
#include "K2Node_MakeStruct.generated.h"

// Pure kismet node that creates a struct with specified values for each member
UCLASS(MinimalAPI)
class UK2Node_MakeStruct : public UK2Node_StructMemberSet
{
	GENERATED_UCLASS_BODY()

	BLUEPRINTGRAPH_API static bool CanBeMade(const UScriptStruct* Struct);
	static bool CanBeExposed(const UProperty* Property);

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetTooltip() const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.MakeStruct_16x"); }
	// End  UEdGraphNode interface

	// Begin K2Node interface
	virtual bool IsNodePure() const override { return true; }
	virtual bool DrawNodeAsVariable() const override { return false; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const override;
	// End K2Node interface

protected:
	struct FMakeStructPinManager : public FStructOperationOptionalPinManager
	{
		const uint8* const SampleStructMemory;
	public:
		FMakeStructPinManager(const uint8* InSampleStructMemory);
	protected:
		virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, UProperty* Property) const override;
		virtual bool CanTreatPropertyAsOptional(UProperty* TestProperty) const override;
	};
};
