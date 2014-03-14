// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQueryGraphNode.generated.h"

UCLASS()
class UEnvironmentQueryGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UObject* NodeInstance;

	/** subnode index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeIndex;

	/** Get the BT graph that owns this node */
	virtual class UEnvironmentQueryGraph* GetEnvironmentQueryGraph();

	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual void PostCopyNode();

	// @return the input pin for this state
	virtual UEdGraphPin* GetInputPin(int32 InputIndex=0) const;
	// @return the output pin for this state
	virtual UEdGraphPin* GetOutputPin(int32 InputIndex=0) const;
	//
	virtual UEdGraph* GetBoundGraph() const { return NULL; }

	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDescription() const;

	virtual void NodeConnectionListChanged() OVERRIDE;

	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const OVERRIDE;

	virtual bool IsSubNode() const { return false; }

	UClass* EnvQueryNodeClass;

protected:

	void ResetNodeOwner();
};

