// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"

#define LOCTEXT_NAMESPACE "AddContentDialog"

void SAddContentDialog::Construct(const FArguments& InArgs)
{
	SAssignNew(AddContentWidget, SAddContentWidget);

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("AddContentDialogTitle", "Add Content to the Project"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(900, 700))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(15))
			[
				SNew(SVerticalBox)

				// Add content widget.
				+ SVerticalBox::Slot()
				.Padding(FMargin(0, 0, 0, 15))
				[
					AddContentWidget.ToSharedRef()
				]

				// Add button.
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(EHorizontalAlignment::HAlign_Right)
				[
					SNew(SButton)
					.OnClicked(this, &SAddContentDialog::AddButtonClicked)
					.IsEnabled_Lambda([this]()->bool
					{
						return AddContentWidget->GetContentSourcesToAdd()->Num() > 0;
					})
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AddSelectedContentToProject", "Add the selected content to the project"))
					]
				]
			]
		]);
}

FReply SAddContentDialog::AddButtonClicked()
{
	for (TSharedPtr<IContentSource> ContentSource : *AddContentWidget->GetContentSourcesToAdd())
	{
		ContentSource->InstallToProject("/Game");
	}
	RequestDestroyWindow();
	return FReply::Handled();
}

SAddContentDialog::~SAddContentDialog()
{
}

#undef LOCTEXT_NAMESPACE