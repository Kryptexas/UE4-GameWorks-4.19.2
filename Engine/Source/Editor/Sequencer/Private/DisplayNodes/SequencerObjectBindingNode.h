// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerDisplayNode.h"


/**
 * A node for displaying an object binding
 */
class FSequencerObjectBindingNode
	: public FSequencerDisplayNode
{
public:
	/**
	 * Create and initialize a new instance.
	 * 
	 * @param InNodeName The name identifier of then node.
	 * @param InObjectName The name of the object we're binding to.
	 * @param InObjectBinding Object binding guid for associating with live objects.
	 * @param InParentNode The parent of this node, or nullptr if this is a root node.
	 * @param InParentTree The tree this node is in.
	 */
	FSequencerObjectBindingNode(FName NodeName, const FString& InObjectName, const FGuid& InObjectBinding, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree)
		: FSequencerDisplayNode(NodeName, InParentNode, InParentTree)
		, ObjectBinding(InObjectBinding)
		, DefaultDisplayName(FText::FromString(InObjectName))
	{ }

public:


	/** @return The object binding on this node */
	const FGuid& GetObjectBinding() const
	{
		return ObjectBinding;
	}
	
public:

	// FSequencerDisplayNode interface

	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder) override;
	virtual TSharedRef<SWidget> GenerateEditWidgetForOutliner() override;
	virtual FText GetDisplayName() const override;
	virtual float GetNodeHeight() const override;
	virtual FNodePadding GetNodePadding() const override;
	virtual ESequencerNode::Type GetType() const override;

private:

	TSharedRef<SWidget> OnGetAddTrackMenuContent();

	void AddPropertyMenuItemsWithCategories(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*>> KeyablePropertyPath);

	void AddPropertyMenuItems(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*>> KeyableProperties, int32 PropertyNameIndexStart = 0, int32 PropertyNameIndexEnd = -1);

	void AddTrackForProperty(TArray<UProperty*> PropertyPath);

	/** Get class for object binding */
	const UClass* GetClassForObjectBinding();

private:

	/** The binding to live objects */
	FGuid ObjectBinding;

	/** The default display name of the object which is used if the binding manager doesn't provide one. */
	FText DefaultDisplayName;
};
