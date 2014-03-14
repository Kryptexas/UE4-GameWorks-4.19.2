// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "K2Node_BaseMCDelegate.generated.h"

UCLASS(MinimalAPI, abstract)
class UK2Node_BaseMCDelegate : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Reference to delegate */
	UPROPERTY()
	FMemberReference	DelegateReference;

public:
	
	// UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return false; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual bool AllowMultipleSelfs(bool bInputAsArray) const OVERRIDE { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End of UK2Node interface

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End of UEdGraphNode interface

	BLUEPRINTGRAPH_API void SetFromProperty(const UProperty* Property, bool bSelfContext)
	{
		DelegateReference.SetFromField<UProperty>(Property, bSelfContext);
	}

	BLUEPRINTGRAPH_API UProperty* GetProperty() const
	{
		return DelegateReference.ResolveMember<UProperty>(this);
	}

	BLUEPRINTGRAPH_API FName GetPropertyName() const
	{
		return DelegateReference.GetMemberName();
	}

	BLUEPRINTGRAPH_API UFunction* GetDelegateSignature(bool bForceNotFromSkelClass = false) const;

	BLUEPRINTGRAPH_API UEdGraphPin* GetDelegatePin() const;

	/** Is the delegate BlueprintAuthorityOnly */
	BLUEPRINTGRAPH_API bool IsAuthorityOnly() const;
};
