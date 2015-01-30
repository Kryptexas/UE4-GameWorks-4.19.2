// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Wraps a details panel customized for viewing actors
 */
class SActorDetails : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SActorDetails){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FName TabIdentifier);
	~SActorDetails();

	/**
	 * Sets the objects to be viewed by the details panel
	 *
	 * @param InObjects	The objects to set
	 */
	void SetObjects(const TArray<UObject*>& InObjects);

	/** FEditorUndoClient Interface */
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:
	void OnEditorSelectionChanged(UObject* Object);
	AActor* GetSelectedActor() const;
	void OnSCSEditorRootSelected(AActor* Actor);
	void OnSCSEditorTreeViewSelectionChanged(const TArray<TSharedPtr<class FSCSEditorTreeNode> >& SelectedNodes);
	void UpdateComponentTreeFromEditorSelection();

private:
	TSharedPtr<SSplitter> DetailsSplitter;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<SBox> ComponentsBox;
	TSharedPtr<class SSCSEditor> SCSEditor;

	bool bSelectionGuard;

	// True if the actor "root" node in the SCS editor is currently shown as selected
	bool bShowingRootActorNodeSelected;
};