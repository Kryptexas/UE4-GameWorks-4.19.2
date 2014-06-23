// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorToolbar.h"
#include "BehaviorTreeEditor.h"
#include "WorkflowOrientedApp/SModeWidget.h"
#include "BehaviorTreeEditorCommands.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeEditorToolbar"

class SBehaviorTreeModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SBehaviorTreeModeSeparator) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FEditorStyle::GetBrush("BlueprintEditor.PipelineSeparator"))
			.Padding(0.0f)
			);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize() const override
	{
		const float Height = 20.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};

void FBehaviorTreeEditorToolbar::AddModesToolbar(TSharedPtr<FExtender> Extender)
{
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BehaviorTreeEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBehaviorTreeEditorToolbar::FillModesToolbar ) );
}

void FBehaviorTreeEditorToolbar::AddDebuggerToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();

	// setup toolbar
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<FBehaviorTreeEditor> BehaviorTreeEditor)
		{
			TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();

			const bool bCanShowDebugger = BehaviorTreeEditorPtr->IsDebuggerReady();
			if (bCanShowDebugger)
			{
				TSharedRef<SWidget> SelectionBox = SNew(SComboButton)
					.OnGetMenuContent( BehaviorTreeEditorPtr.Get(), &FBehaviorTreeEditor::OnGetDebuggerActorsMenu )
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText( LOCTEXT("SelectDebugActor", "Pick actor to debug") )
						.Text(BehaviorTreeEditorPtr.Get(), &FBehaviorTreeEditor::GetDebuggerActorDesc )
					];

				ToolbarBuilder.BeginSection("CachedState");
				{
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().BackOver);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().BackInto);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ForwardInto);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ForwardOver);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().StepOut);
				}
				ToolbarBuilder.EndSection();
				ToolbarBuilder.BeginSection("World");
				{
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().PausePlaySession);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ResumePlaySession);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().StopPlaySession);
					ToolbarBuilder.AddSeparator();
					ToolbarBuilder.AddWidget(SelectionBox);
				}
				ToolbarBuilder.EndSection();
			}
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, BehaviorTreeEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar, BehaviorTreeEditor ));
	BehaviorTreeEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FBehaviorTreeEditorToolbar::FillModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();

	TAttribute<FName> GetActiveMode(BehaviorTreeEditorPtr.ToSharedRef(), &FBehaviorTreeEditor::GetCurrentMode);
	FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(BehaviorTreeEditorPtr.ToSharedRef(), &FBehaviorTreeEditor::SetCurrentMode);

	// Left side padding
	BehaviorTreeEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

	BehaviorTreeEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FBehaviorTreeEditor::GetLocalizedMode( FBehaviorTreeEditor::BehaviorTreeMode ), FBehaviorTreeEditor::BehaviorTreeMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(BehaviorTreeEditorPtr.Get(), &FBehaviorTreeEditor::CanAccessBehaviorTreeMode)
		.ToolTipText(LOCTEXT("BehaviorTreeModeButtonTooltip", "Switch to Behavior Tree Mode"))
		.IconImage(FEditorStyle::GetBrush("BTEditor.SwitchToBehaviorTreeMode"))
		.SmallIconImage(FEditorStyle::GetBrush("BTEditor.SwitchToBehaviorTreeMode.Small"))
	);

	BehaviorTreeEditorPtr->AddToolbarWidget(SNew(SBehaviorTreeModeSeparator));

	BehaviorTreeEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FBehaviorTreeEditor::GetLocalizedMode( FBehaviorTreeEditor::BlackboardMode ), FBehaviorTreeEditor::BlackboardMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(BehaviorTreeEditorPtr.Get(), &FBehaviorTreeEditor::CanAccessBlackboardMode)
		.ToolTipText(LOCTEXT("BlackboardModeButtonTooltip", "Switch to Blackboard Mode"))
		.IconImage(FEditorStyle::GetBrush("BTEditor.SwitchToBlackboardMode"))
		.SmallIconImage(FEditorStyle::GetBrush("BTEditor.SwitchToBlackboardMode.Small"))
	);
		
	// Right side padding
	BehaviorTreeEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
}

#undef LOCTEXT_NAMESPACE
