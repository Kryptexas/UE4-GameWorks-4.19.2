// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeEditorTypes.h"
#include "BehaviorTreeDecoratorGraphNode_Decorator.generated.h"

UCLASS()
class UBehaviorTreeDecoratorGraphNode_Decorator : public UBehaviorTreeDecoratorGraphNode 
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UObject* NodeInstance;

	struct FClassData ClassData;

	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;

	virtual EBTDecoratorLogic::Type GetOperationType() const OVERRIDE;

	virtual void PostEditImport() OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	void PostCopyNode();

private:

	void ResetNodeOwner();
};
