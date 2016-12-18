// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ActorSequenceComponentCustomization.h"

#include "ActorSequence.h"
#include "ActorSequenceComponent.h"
#include "EditorStyleSet.h"
#include "GameFramework/Actor.h"
#include "IDetailsView.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "SDockTab.h"
#include "SSCSEditor.h"
#include "BlueprintEditorTabs.h"
#include "ScopedTransaction.h"
#include "ISequencerModule.h"
#include "ActorSequenceEditorTabSummoner.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "ActorSequenceComponentCustomization"

FName SequenceTabId("EmbeddedSequenceID");

TSharedRef<IDetailCustomization> FActorSequenceComponentCustomization::MakeInstance()
{
	return MakeShared<FActorSequenceComponentCustomization>();
}

void FActorSequenceComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		// @todo: Display other common properties / information?
		return;
	}

	WeakSequenceComponent = Cast<UActorSequenceComponent>(Objects[0].Get());
	if (!WeakSequenceComponent.Get())
	{
		return;
	}

	const IDetailsView& DetailsView = DetailBuilder.GetDetailsView();
	TSharedPtr<FTabManager> HostTabManager = DetailsView.GetHostTabManager();

	DetailBuilder.HideProperty("Sequence");

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Sequence", FText(), ECategoryPriority::Important);

	if (HostTabManager.IsValid() && HostTabManager->CanSpawnTab(SequenceTabId))
	{
		WeakTabManager = HostTabManager;

		Category.AddCustomRow(FText())
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SequenceValueText", "Sequence"))
				.Font(DetailBuilder.GetDetailFont())
			]
			.ValueContent()
			[
				SNew(SButton)
				.OnClicked(this, &FActorSequenceComponentCustomization::InvokeSequencer)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OpenSequenceTabButtonText", "Open in Tab"))
					.Font(DetailBuilder.GetDetailFont())
				]
			];
	}

	// Only display an inline editor for non-blueprint sequences
	if (!GetActorSequence()->GetParentBlueprint())
	{
		TSharedRef<SActorSequenceEditorWidget> ActorSequenceEditorWidget = SNew(SActorSequenceEditorWidget, nullptr);
		ActorSequenceEditorWidget->AssignSequence(GetActorSequence());

		Category.AddCustomRow(FText())
		.WholeRowContent()
		.MaxDesiredWidth(TOptional<float>())
		[
			SAssignNew(InlineSequencer, SBox)
			.HeightOverride(300)
			[
				ActorSequenceEditorWidget
			]
		];
	}
}

FReply FActorSequenceComponentCustomization::InvokeSequencer()
{
	TSharedPtr<FTabManager> TabManager = WeakTabManager.Pin();
	if (TabManager.IsValid() && TabManager->CanSpawnTab(SequenceTabId))
	{
		if (InlineSequencer.IsValid())
		{	
			InlineSequencer->SetContent(SNullWidget::NullWidget);
			InlineSequencer->SetVisibility(EVisibility::Collapsed);
		}

		TSharedRef<SWidget> Content = TabManager->InvokeTab(SequenceTabId)->GetContent();
		StaticCastSharedRef<SActorSequenceEditorWidget>(Content)->AssignSequence(GetActorSequence());
	}

	return FReply::Handled();
}

UActorSequence* FActorSequenceComponentCustomization::GetActorSequence() const
{
	UActorSequenceComponent* SequenceComponent = WeakSequenceComponent.Get();
	return SequenceComponent ? SequenceComponent->GetSequence() : nullptr;
}

#undef LOCTEXT_NAMESPACE
