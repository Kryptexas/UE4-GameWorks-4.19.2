// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphPalette.h"
#include "EdGraph/EdGraphSchema.h"
#include "SBehaviorTreeBlackboardView.h"

/** Displays and edits blackboard entries */
class SBehaviorTreeBlackboardEditor : public SBehaviorTreeBlackboardView
{
public:
	SLATE_BEGIN_ARGS( SBehaviorTreeBlackboardEditor ) {}

		SLATE_EVENT(FOnEntrySelected, OnEntrySelected)
		SLATE_EVENT(FOnGetDebugKeyValue, OnGetDebugKeyValue)
		SLATE_EVENT(FOnGetDisplayCurrentState, OnGetDisplayCurrentState)
		SLATE_EVENT(FOnIsDebuggerReady, OnIsDebuggerReady)
		SLATE_EVENT(FOnIsDebuggerPaused, OnIsDebuggerPaused)
		SLATE_EVENT(FOnGetDebugTimeStamp, OnGetDebugTimeStamp)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FUICommandList> ToolkitCommands, UBlackboardData* InBlackboardData);

private:
	/** Fill the context menu with edit options */
	virtual void FillContextMenu(FMenuBuilder& MenuBuilder) const override;

	/** Fill the toolbar with edit options */
	void FillToolbar(FToolBarBuilder& ToolbarBuilder) const;

	/** Extend the toolbar */
	virtual TSharedPtr<FExtender> GetToolbarExtender(TSharedRef<FUICommandList> ToolkitCommands) const override;

	/** Handle deleting the currently selected entry */
	void HandleDeleteEntry();

	/** Handle renaming the currently selected entry */
	void HandleRenameEntry();

	/** Create the menu used to create a new blackboard entry */
	TSharedRef<SWidget> HandleCreateNewEntryMenu() const;

	/** Create a new blackboard entry when a class is picked */
	void HandleKeyClassPicked(UClass* InClass);

	/** Delegate handler that disallows creating a new entry while the debugger is running */
	bool CanCreateNewEntry() const;
};