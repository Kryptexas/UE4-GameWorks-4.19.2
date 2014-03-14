// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_MathExpression.generated.h"

UCLASS()
class UK2Node_MathExpression : public UK2Node_Composite
{
	GENERATED_UCLASS_BODY()

public:

	// The math expression to evaluate
	UPROPERTY(EditAnywhere, Category=Expression)
	FString Expression;

	//@TODO: Debug output - the parsing results
	UPROPERTY(VisibleAnywhere, Category=Expression)
	FString ParseResults;
public:
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void ReconstructNode() OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UK2Node interface

protected:
	void RebuildExpression();
};


