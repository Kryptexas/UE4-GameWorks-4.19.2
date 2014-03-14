// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node.h"
#include "GameplayTagsK2Node_LiteralGameplayTag.generated.h"

UCLASS()
class UGameplayTagsK2Node_LiteralGameplayTag : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface
#endif

};
