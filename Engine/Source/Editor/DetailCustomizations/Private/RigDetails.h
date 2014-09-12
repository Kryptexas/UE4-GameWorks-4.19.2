// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"

class FRigDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:

	/** Generate a widget for a movie array element */
	void GenerateNodeArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout);
	void GenerateRigControlArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout);

	TSharedPtr<IPropertyHandle> NodesPropertyHandle;
	TSharedPtr<IPropertyHandle> RigControlsPropertyHandle;

	// array custom boxes - these will stay around as long as this window is up
	TArray<TSharedPtr<SEditableTextBox>>	DisplayNameTextBoxes;
	TArray<TArray<TSharedPtr<FString>>>		ParentSpaceOptionList;

	/** we only support one item */
	TWeakObjectPtr<UObject> ItemBeingEdited;

	// node display text handler
	void ValidErrorMessage(const FString & DisplayString, int32 ArrayIndex);
	FText GetDisplayName(TSharedRef<IPropertyHandle> DisplayNameProp) const;
	void OnDisplayNameChanged(const FText& Text, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex);
	void OnDisplayNameCommitted(const FText& Text, ETextCommit::Type CommitType, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex);

	// combo box handler
	void OnParentSpaceSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo, TSharedRef<IPropertyHandle> ParentSpacePropertyHandle);

	// button handlers
	FReply OnSetAllToWorld();
	FReply OnSetAllToParent();

	IDetailLayoutBuilder * DetailBuilderPtr;
};

