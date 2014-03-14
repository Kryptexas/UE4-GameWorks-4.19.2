// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "SGameplayTagWidget.h"

/** Customization for the gameplay tag container struct */
class FGameplayTagContainerCustomization : public IStructCustomization, public FEditorUndoClient
{
public:
	static TSharedRef<IStructCustomization> MakeInstance()
	{
		return MakeShareable(new FGameplayTagContainerCustomization);
	}

	~FGameplayTagContainerCustomization();

	/** Overridden to show an edit button to launch the gameplay tag editor */
	virtual void CustomizeStructHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	
	/** Overridden to do nothing */
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE {}

	// Begin FEditorUndoClient Interface
	virtual void PostUndo( bool bSuccess ) OVERRIDE;
	virtual void PostRedo( bool bSuccess ) OVERRIDE;	
	// End FEditorUndoClient Interface

private:
	/** Called when the edit button is clicked; Launches the gameplay tag editor */
	FReply OnEditButtonClicked();

	/** Called when the clear all button is clicked; Clears all selected tags in the container*/
	FReply OnClearAllButtonClicked();

	/** Returns the selected tags list widget*/
	TSharedRef<SWidget> ActiveTags();

	/** Updates the list of selected tags*/
	void RefreshTagList();

	/** On Generate Row Delegate */
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Build List of Editable Containers */
	void BuildEditableContainerList();

	/** Delegate to close the Gameplay Tag Edit window if it loses focus, as the data it uses is volatile to outside changes */
	void OnGameplayTagWidgetWindowDeactivate();

	/** Cached property handle */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/** The array of containers this objects has */
	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableContainers;

	/** List of tag names selected in the tag containers*/
	TArray< TSharedPtr<FString> > TagNames;

	/** The TagList, kept as a member so we can update it later */
	TSharedPtr<SListView<TSharedPtr<FString>>> TagListView;

	/** The Window for the GameplayTagWidget */
	TSharedPtr<SWindow> GameplayTagWidgetWindow;
};

