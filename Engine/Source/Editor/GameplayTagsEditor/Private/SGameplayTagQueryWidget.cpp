// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "SGameplayTagQueryWidget.h"
#include "ScopedTransaction.h"
#include "SScaleBox.h"

#include "SGameplayTagWidget.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"


#define LOCTEXT_NAMESPACE "GameplayTagQueryWidget"

void SGameplayTagQueryWidget::Construct(const FArguments& InArgs, const TArray<FEditableGameplayTagQueryDatum>& EditableTagQueries)
{
	ensure(EditableTagQueries.Num() > 0);
	TagQueries = EditableTagQueries;

	bReadOnly = InArgs._ReadOnly;
	OnSaveAndClose = InArgs._OnSaveAndClose;
	OnCancel = InArgs._OnCancel;

	// Tag the assets as transactional so they can support undo/redo
	for (int32 AssetIdx = 0; AssetIdx < TagQueries.Num(); ++AssetIdx)
	{
		UObject* TagQueryOwner = TagQueries[AssetIdx].TagQueryOwner.Get();
		if (TagQueryOwner)
		{
			TagQueryOwner->SetFlags(RF_Transactional);
		}
	}

	// build editable query object tree from the runtime query data
	UEditableGameplayTagQuery* const EQ = CreateEditableQuery(*TagQueries[0].TagQuery);
	EditableQuery = EQ;

	// create details view for the editable query object
	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = false;
	ViewArgs.bHideSelectionTip = true;
	ViewArgs.bShowActorLabel = false;
	
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Details = PropertyModule.CreateDetailView(ViewArgs);
	Details->SetObject(EQ);

	ChildSlot
	[
		SNew(SScaleBox)
		.HAlign(EHorizontalAlignment::HAlign_Left)
		.VAlign(EVerticalAlignment::VAlign_Top)
		.StretchDirection(EStretchDirection::DownOnly)
		.Stretch(EStretch::ScaleToFit)
		.Content()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.IsEnabled(!bReadOnly)
						.OnClicked(this, &SGameplayTagQueryWidget::OnSaveAndCloseClicked)
						.Text(LOCTEXT("GameplayTagQueryWidget_SaveAndClose", "Save and Close"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.OnClicked(this, &SGameplayTagQueryWidget::OnCancelClicked)
						.Text(LOCTEXT("GameplayTagQueryWidget_Cancel", "Close Without Saving"))
					]
				]
				// to delete!
				+ SVerticalBox::Slot()
				[
					Details.ToSharedRef()
				]
			]
		]
	];
}

UEditableGameplayTagQuery* SGameplayTagQueryWidget::CreateEditableQuery(FGameplayTagQuery& Q)
{
	UEditableGameplayTagQuery* const EditableQuery = Q.CreateEditableQuery();
	if (EditableQuery)
	{
		// prevent GC, will explicitly remove from root later
		EditableQuery->AddToRoot();
	}

	return EditableQuery;
}

SGameplayTagQueryWidget::~SGameplayTagQueryWidget()
{
	// clean up our temp editing uobjects
	UEditableGameplayTagQuery* const Q = EditableQuery.Get();
	if (Q)
	{
		Q->RemoveFromRoot();
	}
}

FReply SGameplayTagQueryWidget::OnSaveAndCloseClicked()
{
	// translate obj tree to token stream
	if (EditableQuery.IsValid() && !bReadOnly)
	{
		// write to all selected queries
		for (auto& TQ : TagQueries)
		{
			TQ.TagQuery->BuildFromEditableQuery(*EditableQuery.Get());
			TQ.TagQueryOwner->MarkPackageDirty();
		}
	}

	OnSaveAndClose.ExecuteIfBound();
	return FReply::Handled();
}

FReply SGameplayTagQueryWidget::OnCancelClicked()
{
	OnCancel.ExecuteIfBound();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
