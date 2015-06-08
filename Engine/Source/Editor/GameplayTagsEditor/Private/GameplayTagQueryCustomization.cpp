// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "GameplayTagQueryCustomization.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "MainFrame.h"
#include "SGameplayTagQueryWidget.h"
#include "GameplayTagContainer.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "GameplayTagQueryCustomization"

void FGameplayTagQueryCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	BuildEditableQueryList();

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(512)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(this, &FGameplayTagQueryCustomization::GetEditButtonText)
				.OnClicked(this, &FGameplayTagQueryCustomization::OnEditButtonClicked)
			]
		]
	];

	GEditor->RegisterForUndo(this);
}

FText FGameplayTagQueryCustomization::GetEditButtonText() const
{
	bool const bReadOnly = StructPropertyHandle->GetProperty()->HasAnyPropertyFlags(CPF_EditConst);
	return
		bReadOnly
		? LOCTEXT("GameplayTagQueryCustomization_View", "View...")
		: LOCTEXT("GameplayTagQueryCustomization_Edit", "Edit...");
}

FReply FGameplayTagQueryCustomization::OnEditButtonClicked()
{
	if (GameplayTagQueryWidgetWindow.IsValid())
	{
		// already open, just show it
		GameplayTagQueryWidgetWindow->BringToFront(true);
	}
	else
	{
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);

		bool const bReadOnly = StructPropertyHandle->GetProperty()->HasAnyPropertyFlags(CPF_EditConst);

		FText Title;
		if (OuterObjects.Num() > 1)
		{
			FText const AssetName = FText::Format(LOCTEXT("GameplayTagDetailsBase_MultipleAssets", "{0} Assets"), FText::AsNumber(OuterObjects.Num()));
			FText const PropertyName = StructPropertyHandle->GetPropertyDisplayName();
			Title = FText::Format(LOCTEXT("GameplayTagQueryCustomization_BaseWidgetTitle", "Tag Editor: {0} {1}"), PropertyName, AssetName);
		}
		else if (OuterObjects.Num() > 0 && OuterObjects[0])
		{
			FText const AssetName = FText::FromString(OuterObjects[0]->GetName());
			FText const PropertyName = StructPropertyHandle->GetPropertyDisplayName();
			Title = FText::Format(LOCTEXT("GameplayTagQueryCustomization_BaseWidgetTitle", "Tag Editor: {0} {1}"), PropertyName, AssetName);
		}


		GameplayTagQueryWidgetWindow = SNew(SWindow)
			.Title(Title)
			.HasCloseButton(false)
			.ClientSize(FVector2D(600, 400))
			[
				SNew(SGameplayTagQueryWidget, EditableQueries)
				.OnSaveAndClose(this, &FGameplayTagQueryCustomization::CloseWidgetWindow)
				.OnCancel(this, &FGameplayTagQueryCustomization::CloseWidgetWindow)
				.ReadOnly(bReadOnly)
			];

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		if (MainFrameModule.GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(GameplayTagQueryWidgetWindow.ToSharedRef(), MainFrameModule.GetParentWindow().ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(GameplayTagQueryWidgetWindow.ToSharedRef());
		}
	}

	return FReply::Handled();
}

FGameplayTagQueryCustomization::~FGameplayTagQueryCustomization()
{
	if( GameplayTagQueryWidgetWindow.IsValid() )
	{
		GameplayTagQueryWidgetWindow->RequestDestroyWindow();
	}
	GEditor->UnregisterForUndo(this);
}

void FGameplayTagQueryCustomization::BuildEditableQueryList()
{	
	EditableQueries.Empty();

	if( StructPropertyHandle.IsValid() )
	{
		TArray<void*> RawStructData;
		StructPropertyHandle->AccessRawData(RawStructData);

		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);

		ensure(RawStructData.Num() == OuterObjects.Num());
		for (int32 Idx = 0; Idx < RawStructData.Num() && Idx < OuterObjects.Num(); ++Idx)
		{
			EditableQueries.Add(SGameplayTagQueryWidget::FEditableGameplayTagQueryDatum(OuterObjects[Idx], (FGameplayTagQuery*)RawStructData[Idx]));
		}
	}	
}

void FGameplayTagQueryCustomization::CloseWidgetWindow()
{
 	if( GameplayTagQueryWidgetWindow.IsValid() )
 	{
 		GameplayTagQueryWidgetWindow->RequestDestroyWindow();
		GameplayTagQueryWidgetWindow = nullptr;
 	}
}

#undef LOCTEXT_NAMESPACE
