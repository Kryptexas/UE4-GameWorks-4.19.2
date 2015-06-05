// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "SGameplayTagQueryWidget.h"
#include "EditorUndoClient.h"

/** Customization for the gameplay tag query struct */
class FGameplayTagQueryCustomization : public IPropertyTypeCustomization, public FEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FGameplayTagQueryCustomization);
	}

	~FGameplayTagQueryCustomization();

	/** Overridden to show an edit button to launch the gameplay tag editor */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

private:
	/** Called when the edit button is clicked; Launches the gameplay tag editor */
	FReply OnEditButtonClicked();

	/** Overridden to do nothing */
	FText GetEditButtonText() const;

	void CloseWidgetWindow();

	/** Build List of Editable Queries */
	void BuildEditableQueryList();

	/** Cached property handle */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/** The array of queries this objects has */
	TArray<SGameplayTagQueryWidget::FEditableGameplayTagQueryDatum> EditableQueries;

	/** The Window for the GameplayTagWidget */
	TSharedPtr<SWindow> GameplayTagQueryWidgetWindow;
};

