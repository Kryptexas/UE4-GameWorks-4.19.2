// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_CallFunction.generated.h"

UCLASS()
class BLUEPRINTGRAPH_API UK2Node_CallFunction : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Indicates that this is a call to a pure function */
	UPROPERTY()
	uint32 bIsPureFunc:1;

	/** Indicates that this is a call to a const function */
	UPROPERTY()
	uint32 bIsConstFunc:1;

	/** Indicates that during compile we want to create multiple exec pins from an enum param */
	UPROPERTY()
	uint32 bWantsEnumToExecExpansion:1;

	/** Indicates that this is a call to an interface function */
	UPROPERTY()
	uint32 bIsInterfaceCall:1;

	/** Indicates that this is a call to a final / superclass's function */
	UPROPERTY()
	uint32 bIsFinalFunction:1;

	/** Indicates that this is a 'bead' function with no fixed location; it is drawn between the nodes that it is wired to */
	UPROPERTY()
	uint32 bIsBeadFunction:1;

	/** The function to call */
	UPROPERTY()
	FMemberReference FunctionReference;

private:
	/** The name of the function to call */
	UPROPERTY()
	FName CallFunctionName_DEPRECATED;

	/** The class that the function is from. */
	UPROPERTY()
	TSubclassOf<class UObject> CallFunctionClass_DEPRECATED;

public:

#if WITH_EDITOR
	// UObject interface
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDescriptiveCompiledName() const OVERRIDE;
	virtual bool IsDeprecated() const OVERRIDE;
	virtual bool ShouldWarnOnDeprecation() const OVERRIDE;
	virtual FString GetDeprecationMessage() const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual bool IsNodePure() const OVERRIDE { return bIsPureFunc; }
	virtual bool HasExternalBlueprintDependencies() const OVERRIDE;
	virtual void PostReconstructNode() OVERRIDE;
	virtual bool ShouldDrawCompact() const OVERRIDE;
	virtual bool ShouldDrawAsBead() const OVERRIDE;
	virtual FString GetCompactNodeTitle() const OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE;
	virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const OVERRIDE;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual FName GetCornerIcon() const OVERRIDE;
	virtual FText GetToolTipHeading() const OVERRIDE;
	// End of UK2Node interface

	/** Returns the UFunction that this class is pointing to */
	UFunction* GetTargetFunction() const;

	/** Get the then output pin */
	UEdGraphPin* GetThenPin() const;
	/** Get the return value pin */
	UEdGraphPin* GetReturnValuePin() const;

	/** @return	true if the function is a latent operation */
	bool IsLatentFunction() const;

	/** @return true if this function can be called on multiple contexts at once */
	virtual bool AllowMultipleSelfs(bool bInputAsArray) const OVERRIDE;

	/**
	 * Creates a self pin for the graph, taking into account the scope of the function call
	 *
	 * @param	Function	The function to be called by the Node.
	 *
	 * @return	Pointer to the pin that was created
	 */
	virtual UEdGraphPin* CreateSelfPin(const UFunction* Function);
	
	/**
	 * Creates all of the pins required to call a particular UFunction.
	 *
	 * @param	Function	The function to be called by the Node.
	 *
	 * @return	true on success.
	 */
	bool CreatePinsForFunctionCall(const UFunction* Function);

	/** Create exec pins for this function. May be multiple is using 'expand enum as execs' */
	void CreateExecPinsForFunctionCall(const UFunction* Function);

	virtual void PostParameterPinCreated(UEdGraphPin *Pin) {}

	/** Gets the user-facing name for the function */
	static FString GetUserFacingFunctionName(const UFunction* Function);

	/** Gets the non-specific tooltip for the function */
	static FString GetDefaultTooltipForFunction(const UFunction* Function);
	/** Get default category for this function in action menu */
	static FString GetDefaultCategoryForFunction(const UFunction* Function, const FString& BaseCategory);
	/** Get keywords for this function in the action menu */
	static FString GetKeywordsForFunction(const UFunction* Function);
	/** Should be drawn compact for this function */
	static bool ShouldDrawCompact(const UFunction* Function);
	/** Get the compact name for this function */
	static FString GetCompactNodeTitle(const UFunction* Function);

	/** Get the string to use to explain the context for this function (used on node title) */
	virtual FString GetFunctionContextString() const;

	/** Set properties of this node from a supplied function (does not save ref to function) */
	virtual void SetFromFunction(const UFunction* Function);

	static void CallForEachElementInArrayExpansion(UK2Node* Node, UEdGraphPin* MultiSelf, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	/** Returns the graph for this function, if available */
	UEdGraph* GetFunctionGraph() const;
private:
	/** Helper function to check if the SelfPin is wired correctly */
	bool IsSelfPinCompatibleWithBlueprintContext (UEdGraphPin *SelfPin, UBlueprint* BlueprintObj) const;

	/* Looks at function metadata and properties to determine if this node should be using enum to exec expansion */
	void DetermineWantsEnumToExecExpansion(const UFunction* Function);

	/**
	 * Creates hover text for the specified pin.
	 * 
	 * @param   Pin				The pin you want hover text for (should belong to this node)
	 * @param   HoverTextOut	This will get filled out with the generated text
	 */
	void GeneratePinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const;

protected:
	/** Helper function to ensure function is called in our context */
	virtual void EnsureFunctionIsInBlueprint();

#endif
};

