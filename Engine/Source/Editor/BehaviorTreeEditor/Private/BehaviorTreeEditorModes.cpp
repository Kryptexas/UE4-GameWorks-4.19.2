// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorModes.h"
#include "BehaviorTreeEditorTabs.h"

FBehaviorTreeEditorApplicationMode::FBehaviorTreeEditorApplicationMode(TSharedPtr<class FBehaviorTreeEditor> InBehaviorTreeEditor)
	: FApplicationMode(FBehaviorTreeEditor::BehaviorTreeMode)
{
	BehaviorTreeEditor = InBehaviorTreeEditor;

	BehaviorTreeEditorTabFactories.RegisterFactory(MakeShareable(new FBehaviorTreeDetailsSummoner(InBehaviorTreeEditor)));
	BehaviorTreeEditorTabFactories.RegisterFactory(MakeShareable(new FBehaviorTreeSearchSummoner(InBehaviorTreeEditor)));

	TabLayout = FTabManager::NewLayout( "Standalone_BehaviorTree_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->AddTab(InBehaviorTreeEditor->GetToolbarTabId(), ETabState::OpenedTab) 
			->SetHideTabWell(true) 
		)
		->Split
		(
			FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(FBehaviorTreeEditorTabs::GraphEditorID, ETabState::ClosedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab(FBehaviorTreeEditorTabs::GraphDetailsID, ETabState::OpenedTab)
				->AddTab(FBehaviorTreeEditorTabs::SearchID, ETabState::ClosedTab)
			)
		)
	);
	
	InBehaviorTreeEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
	InBehaviorTreeEditor->GetToolbarBuilder()->AddDebuggerToolbar(ToolbarExtender);
}

void FBehaviorTreeEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();
	
	BehaviorTreeEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	BehaviorTreeEditorPtr->PushTabFactories(BehaviorTreeEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FBehaviorTreeEditorApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();
	
	BehaviorTreeEditorPtr->SaveEditedObjectState();
}

void FBehaviorTreeEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();
	BehaviorTreeEditorPtr->RestoreBehaviorTree();

	FApplicationMode::PostActivateMode();
}

FBlackboardEditorApplicationMode::FBlackboardEditorApplicationMode(TSharedPtr<class FBehaviorTreeEditor> InBehaviorTreeEditor)
	: FApplicationMode(FBehaviorTreeEditor::BlackboardMode)
{
	BehaviorTreeEditor = InBehaviorTreeEditor;
	
	BlackboardTabFactories.RegisterFactory(MakeShareable(new FBlackboardDetailsSummoner(InBehaviorTreeEditor)));

	TabLayout = FTabManager::NewLayout( "Standalone_BlackboardEditor_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell( true )
			->AddTab(InBehaviorTreeEditor->GetToolbarTabId(), ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewSplitter()
			->Split
			(
				FTabManager::NewStack()
				->AddTab(FBehaviorTreeEditorTabs::BlackboardDetailsID, ETabState::OpenedTab)
			)
		)
	);

	InBehaviorTreeEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
}

void FBlackboardEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();
	
	BehaviorTreeEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	BehaviorTreeEditorPtr->PushTabFactories(BlackboardTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FBlackboardEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the BT was last saved
	check(BehaviorTreeEditor.IsValid());
	TSharedPtr<FBehaviorTreeEditor> BehaviorTreeEditorPtr = BehaviorTreeEditor.Pin();
//	BehaviorTreeEditorPtr->StartEditingDefaults();

	FApplicationMode::PostActivateMode();
}
