// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeDecoratorGraphNode_Logic.generated.h"

UENUM()
namespace EDecoratorLogicMode
{
	enum Type
	{
		Sink,
		And,
		Or,
		Not,
	};
}

UCLASS()
class UBehaviorTreeDecoratorGraphNode_Logic : public UBehaviorTreeDecoratorGraphNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TEnumAsByte<EDecoratorLogicMode::Type> LogicMode;

	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE;
	virtual EBTDecoratorLogic::Type GetOperationType() const OVERRIDE;

	virtual void GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;

	bool CanRemovePins() const;
	bool CanAddPins() const;

	void AddInputPin();
	void RemoveInputPin(class UEdGraphPin* Pin);
};
