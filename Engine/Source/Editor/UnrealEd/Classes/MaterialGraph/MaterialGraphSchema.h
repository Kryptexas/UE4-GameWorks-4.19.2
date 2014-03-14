// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialGraphSchema.generated.h"

/** Action to add an expression node to the graph */
USTRUCT()
struct UNREALED_API FMaterialGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Class of expression we want to create */
	UPROPERTY()
	class UClass* MaterialExpressionClass;

	// Simple type info
	static FString StaticGetTypeId() {static FString Type = TEXT("FMaterialGraphSchemaAction_NewNode"); return Type;}
	virtual FString GetTypeId() const OVERRIDE { return StaticGetTypeId(); } 

	FMaterialGraphSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, MaterialExpressionClass(NULL)
	{}

	FMaterialGraphSchemaAction_NewNode(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
		, MaterialExpressionClass(NULL)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to add a Material Function call to the graph */
USTRUCT()
struct UNREALED_API FMaterialGraphSchemaAction_NewFunctionCall : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Path to function that we want to call */
	UPROPERTY()
	FString FunctionPath;

	// Simple type info
	static FString StaticGetTypeId() {static FString Type = TEXT("FMaterialGraphSchemaAction_NewFunctionCall"); return Type;}
	virtual FString GetTypeId() const OVERRIDE { return StaticGetTypeId(); } 

	FMaterialGraphSchemaAction_NewFunctionCall() 
		: FEdGraphSchemaAction()
	{}

	FMaterialGraphSchemaAction_NewFunctionCall(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to add a comment node to the graph */
USTRUCT()
struct UNREALED_API FMaterialGraphSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	// Simple type info
	static FString StaticGetTypeId() {static FString Type = TEXT("FMaterialGraphSchemaAction_NewComment"); return Type;}
	virtual FString GetTypeId() const OVERRIDE { return StaticGetTypeId(); } 

	FMaterialGraphSchemaAction_NewComment() 
		: FEdGraphSchemaAction()
	{}

	FMaterialGraphSchemaAction_NewComment(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to paste clipboard contents into the graph */
USTRUCT()
struct UNREALED_API FMaterialGraphSchemaAction_Paste : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	// Simple type info
	static FString StaticGetTypeId() {static FString Type = TEXT("FMaterialGraphSchemaAction_Paste"); return Type;}
	virtual FString GetTypeId() const OVERRIDE { return StaticGetTypeId(); } 

	FMaterialGraphSchemaAction_Paste() 
		: FEdGraphSchemaAction()
	{}

	FMaterialGraphSchemaAction_Paste(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

UCLASS(MinimalAPI)
class UMaterialGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	// Allowable PinType.PinCategory values
	UPROPERTY()
	FString PC_Mask;

	UPROPERTY()
	FString PC_Required;

	UPROPERTY()
	FString PC_Optional;

	UPROPERTY()
	FString PC_MaterialInput;

	// Common PinType.PinSubCategory values
	UPROPERTY()
	FString PSC_Red;

	UPROPERTY()
	FString PSC_Green;

	UPROPERTY()
	FString PSC_Blue;

	UPROPERTY()
	FString PSC_Alpha;

	// Color of certain pins/connections
	UPROPERTY()
	FLinearColor ActivePinColor;

	UPROPERTY()
	FLinearColor InactivePinColor;

	UPROPERTY()
	FLinearColor AlphaPinColor;

	/**
	 * Get menu for breaking links to specific nodes
	 *
	 * @param	MenuBuilder	MenuBuilder we are populating
	 * @param	InGraphPin	Pin with links to break
	 */
	void GetBreakLinkToSubMenuActions(class FMenuBuilder& MenuBuilder, class UEdGraphPin* InGraphPin);

	/**
	 * Connect a pin to one of the Material Function's outputs
	 *
	 * @param	InGraphPin	Pin we are connecting
	 * @param	InFuncPin	Desired output pin
	 */
	void OnConnectToFunctionOutput(UEdGraphPin* InGraphPin, UEdGraphPin* InFuncPin);

	/**
	 * Connect a pin to one of the Material's inputs
	 *
	 * @param	InGraphPin	Pin we are connecting
	 * @param	ConnIndex	Index of the Material input to connect to
	 */
	void OnConnectToMaterial(UEdGraphPin* InGraphPin, int32 ConnIndex);

	UNREALED_API void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName, bool bMaterialFunction) const;

	/** Check whether connecting these pins would cause a loop */
	bool ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const;

	/** Check whether the types of pins are compatible */
	bool ArePinsCompatible(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin, FText& ResponseMessage) const;

	// Begin UEdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const OVERRIDE;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const OVERRIDE;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const OVERRIDE;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const OVERRIDE { return true; }
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const OVERRIDE;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const OVERRIDE;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const OVERRIDE;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) OVERRIDE;
	virtual void DroppedAssetsOnGraph(const TArray<class FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const OVERRIDE;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const OVERRIDE;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const OVERRIDE;
	// End UEdGraphSchema interface

private:
	/** Adds actions for all Material Functions */
	void GetMaterialFunctionActions(FGraphActionMenuBuilder& ActionMenuBuilder) const;
	/** Adds action for creating a comment */
	void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = NULL) const;
};
