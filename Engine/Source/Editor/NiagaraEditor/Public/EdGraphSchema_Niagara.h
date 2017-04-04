// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_Niagara.generated.h"

class UEdGraph;
struct FNiagaraVariable;
struct FNiagaraVariableInfo;
struct FNiagaraTypeDefinition;
enum class ENiagaraDataType : uint8;

/** Action to add a node to the graph */
USTRUCT()
struct NIAGARAEDITOR_API FNiagaraSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Template of node we want to create */
	UPROPERTY()
	class UNiagaraNode* NodeTemplate;


	FNiagaraSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, NodeTemplate(nullptr)
	{}

	FNiagaraSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		, NodeTemplate(nullptr)
	{}

	//~ Begin FEdGraphSchemaAction Interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	//~ End FEdGraphSchemaAction Interface

	template <typename NodeType>
	static NodeType* SpawnNodeFromTemplate(class UEdGraph* ParentGraph, NodeType* InTemplateNode, const FVector2D Location, bool bSelectNewNode = true)
	{
		FNiagaraSchemaAction_NewNode Action;
		Action.NodeTemplate = InTemplateNode;

		return Cast<NodeType>(Action.PerformAction(ParentGraph, NULL, Location, bSelectNewNode));
	}
};

UCLASS()
class NIAGARAEDITOR_API UEdGraphSchema_Niagara : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	// Allowable PinType.PinCategory values
	static const FString PinCategoryType;
	static const FString PinCategoryMisc;
	static const FString PinCategoryClass;

	//~ Begin EdGraphSchema Interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;

	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) override;
	//~ End EdGraphSchema Interface

	FNiagaraVariable PinToNiagaraVariable(const UEdGraphPin* Pin, bool bNeedsValue=false)const;
	FNiagaraTypeDefinition PinToTypeDefinition(const UEdGraphPin* Pin)const;
	FEdGraphPinType TypeDefinitionToPinType(FNiagaraTypeDefinition TypeDef)const;
	
	bool IsSystemConstant(const FNiagaraVariable& Variable)const;

	FNiagaraTypeDefinition GetTypeDefForProperty(const UProperty* Property)const;

	static const FLinearColor NodeTitleColor_Attribute;
	static const FLinearColor NodeTitleColor_Constant;
	static const FLinearColor NodeTitleColor_SystemConstant;
	static const FLinearColor NodeTitleColor_FunctionCall;
	static const FLinearColor NodeTitleColor_Event;

private:
	void GetBreakLinkToSubMenuActions(class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin);
	void GetNumericConversionToSubMenuActions(class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin);
	void ConvertNumericPinToType(UEdGraphPin* InPin, FNiagaraTypeDefinition TypeDef);

};

