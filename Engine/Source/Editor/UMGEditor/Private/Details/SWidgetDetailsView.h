// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

#include "WidgetTemplate.h"

/**
 * The details view used in the designer section of the widget blueprint editor.
 */
class SWidgetDetailsView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SWidgetDetailsView ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor);
	virtual ~SWidgetDetailsView();
	
	TSharedPtr<class IDetailsView> GetPropertyView() const { return PropertyView; }
	
private:
	void RegisterCustomizations();

	void OnEditorSelectionChanging();
	void OnEditorSelectionChanged();

	void ClearFocusIfOwned();

	EVisibility GetNameAreaVisibility() const;

	const FSlateBrush* GetNameIcon() const;

	FText GetNameText() const;

	bool HandleVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

	void HandleNameTextChanged(const FText& Text);
	void HandleNameTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	ESlateCheckBoxState::Type GetIsVariable() const;

	void HandleIsVariableChanged(ESlateCheckBoxState::Type CheckState);

private:
	/** The editor that owns this details view */
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;

	TSharedPtr<SEditableTextBox> NameTextBox;
	
	/** Property viewing widget */
	TSharedPtr<class IDetailsView> PropertyView;

	/** Selected objects for this detail view */
	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
};
