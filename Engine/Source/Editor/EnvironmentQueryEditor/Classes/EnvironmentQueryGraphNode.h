// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIGraphNode.h"
#include "EnvironmentQueryGraphNode.generated.h"

UCLASS()
class UEnvironmentQueryGraphNode : public UAIGraphNode
{
	GENERATED_BODY()
public:
	UEnvironmentQueryGraphNode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual class UEnvironmentQueryGraph* GetEnvironmentQueryGraph();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetDescription() const override;

	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const override;
};
