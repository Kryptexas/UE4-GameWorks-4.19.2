// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "SBehaviorTreeDebuggerView.h"
#include "BehaviorTreeColors.h"

SBehaviorTreeDebuggerView::SBehaviorTreeDebuggerView()
{
	bShowCurrentState = true;
	bCanShowCurrentState = true;
}

void SBehaviorTreeDebuggerView::Construct( const FArguments& InArgs )
{
}

void SBehaviorTreeDebuggerView::UpdateBlackboard(class UBlackboardData* BlackboardAsset)
{
	MyContent = SNew(SVerticalBox);
	ChildSlot.Widget = MyContent.ToSharedRef();

	if (BlackboardAsset == NULL)
	{
		return;
	}

	// buttons for switching between current state and snapshot
	MyContent->AddSlot().AutoHeight().Padding(0, 0, 0, 10.0f)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "RadioButton")
			.Visibility( this, &SBehaviorTreeDebuggerView::GetStateSwitchVisibility)
			.IsChecked( this, &SBehaviorTreeDebuggerView::IsRadioChecked, EBTDebuggerBlackboard::Saved )
			.OnCheckStateChanged( this, &SBehaviorTreeDebuggerView::OnRadioChanged, EBTDebuggerBlackboard::Saved )
			[
				SNew(STextBlock).Text(FString("Execution snap"))
			]
		]
		+SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "RadioButton")
			.Visibility( this, &SBehaviorTreeDebuggerView::GetStateSwitchVisibility)
			.IsChecked( this, &SBehaviorTreeDebuggerView::IsRadioChecked, EBTDebuggerBlackboard::Current )
			.OnCheckStateChanged( this, &SBehaviorTreeDebuggerView::OnRadioChanged, EBTDebuggerBlackboard::Current )
			[
				SNew(STextBlock).Text(FString("Current state"))
			]
		]
	];

	TSharedRef<SVerticalBox> VBoxHeaders = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> VBoxValues = SNew(SVerticalBox);
	const int32 NumKeys = BlackboardAsset ? BlackboardAsset->GetNumKeys() : 0;

	VBoxHeaders->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(FString("Timestamp:"))
		.ColorAndOpacity(BehaviorTreeColors::Debugger::DescHeader)
		.Font(FEditorStyle::GetFontStyle("BoldFont"))
	];
	VBoxValues->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(this, &SBehaviorTreeDebuggerView::GetTimestampDesc)
	];

	VBoxHeaders->AddSlot().Padding(0, 0, 0, 10.0f).AutoHeight()
	[
		SNew(STextBlock).Text(FString("Blackboard:"))
		.ColorAndOpacity(BehaviorTreeColors::Debugger::DescHeader)
		.Font(FEditorStyle::GetFontStyle("BoldFont"))
	];
	VBoxValues->AddSlot().Padding(0, 0, 0, 10.0f).AutoHeight()
	[
		SNew(STextBlock).Text(GetNameSafe(BlackboardAsset))
	];

	for (int32 i = 0; i < NumKeys; i++)
	{
		FString KeyDesc = BlackboardAsset->GetKeyName(i).ToString().AppendChar(TEXT(':'));
		TSharedRef<SBorder> BorderHeader = SNew( SBorder ).BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
		[
			SNew(STextBlock).Text(KeyDesc)
			.ColorAndOpacity(BehaviorTreeColors::Debugger::DescKeys)
			.Font(FEditorStyle::GetFontStyle("BoldFont"))
		];

		TSharedRef<SBorder> BorderValue = SNew( SBorder ).BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
		[
			SNew(STextBlock).Text(this, &SBehaviorTreeDebuggerView::GetValueDesc, i)
		];

		if ((i % 2) == 0)
		{
			BorderHeader->SetBorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f));
			BorderValue->SetBorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f));
		}

		VBoxHeaders->AddSlot().AutoHeight()
		[
			BorderHeader
		];
		VBoxValues->AddSlot().AutoHeight()
		[
			BorderValue
		];
	}

	// use VBoxes
	MyContent->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot().AutoWidth()
		[
			VBoxHeaders
		]
		+SHorizontalBox::Slot().FillWidth(1.0f)
		[
			VBoxValues
		]
	];
}

EVisibility SBehaviorTreeDebuggerView::GetStateSwitchVisibility() const
{
	return bCanShowCurrentState ? EVisibility::Visible : EVisibility::Collapsed;
}

ESlateCheckBoxState::Type SBehaviorTreeDebuggerView::IsRadioChecked(EBTDebuggerBlackboard::Type ButtonId) const
{
	if (!bCanShowCurrentState)
	{
		return (ButtonId == EBTDebuggerBlackboard::Saved) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	return ((ButtonId == EBTDebuggerBlackboard::Current) && bShowCurrentState) || ((ButtonId == EBTDebuggerBlackboard::Saved) && !bShowCurrentState) ?
		ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SBehaviorTreeDebuggerView::OnRadioChanged(ESlateCheckBoxState::Type NewRadioState, EBTDebuggerBlackboard::Type ButtonId)
{
	if (NewRadioState == ESlateCheckBoxState::Checked)
	{
		bShowCurrentState = (ButtonId == EBTDebuggerBlackboard::Current);
	}
}

FString SBehaviorTreeDebuggerView::GetValueDesc(int32 ValueIdx) const
{
	return IsShowingCurrentState() ?
		CurrentValues.IsValidIndex(ValueIdx) ? CurrentValues[ValueIdx] : FString() :
		SavedValues.IsValidIndex(ValueIdx) ? SavedValues[ValueIdx] : FString();
}

FString SBehaviorTreeDebuggerView::GetTimestampDesc() const
{
	return FString::Printf(TEXT("%.3f"), IsShowingCurrentState() ? CurrentTimestamp : SavedTimestamp);
}
