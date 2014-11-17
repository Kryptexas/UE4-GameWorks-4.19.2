// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_MathExpression.generated.h"

/**
* This node type acts like our collapsed nodes, a single node that represents
* a larger sub-network of nodes (contained within a sub-graph). This node will
* take the expression it was named with, and attempt to convert it into a
* series of math node. If it is unsuccessful, then it generates a series of
* errors that are tacked onto the node for the user to see.
*/
UCLASS()
class UK2Node_MathExpression : public UK2Node_Composite
{
	GENERATED_UCLASS_BODY()

public:

	// The math expression to evaluate
	UPROPERTY(EditAnywhere, Category = Expression)
		FString Expression;

public:
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

	// UEdGraphNode interface
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const override;
	virtual void OnRenameNode(const FString& NewName) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PostPlacedNewNode() override;
	virtual void ReconstructNode() override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	// End of UK2Node interface

private:
	/**
	* Clears this node's sub-graph, and then takes the supplied string and
	* parses, and converts it into a series of new graph nodes.
	*
	* @param  NewExpression	The new string we wish to replace the current "Expression".
	*/
	void RebuildExpression(FString NewExpression);

	/**
	* Clears the cached expression string, deletes all generated node, clears
	* input pins, and resets the parser and graph generator (priming this node
	* for a new expression).
	*/
	void ClearExpression();

private:
	/** Cached so we don't have to regenerate it when the graph is recompiled */
	//UPROPERTY(Transient)
	TSharedPtr<class FCompilerResultsLog> CachedMessageLog;
};


