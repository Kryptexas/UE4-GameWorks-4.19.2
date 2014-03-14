// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "GraphEditor.h"


/**
 * Graph editor public interface
 */
class FGraphEditorModule : public IModuleInterface
{

public:
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** Delegates to be called to extend the graph menus */
	
	DECLARE_DELEGATE_RetVal_FiveParams( TSharedRef<FExtender>, FGraphEditorMenuExtender_SelectedNode, const TSharedRef<FUICommandList>, const UEdGraph*, const UEdGraphNode*, const UEdGraphPin*, bool);
	virtual TArray<FGraphEditorMenuExtender_SelectedNode>& GetAllGraphEditorContextMenuExtender() {return GraphEditorContextMenuExtender;}

private:
	friend class SGraphEditor;

	/**
	 * DO NOT CALL THIS METHOD. Use SNew(SGraphEditor) to make instances of SGraphEditor.
	 *
	 * @return A GraphEditor implementation.
	 */
	virtual TSharedRef<SGraphEditor> PRIVATE_MakeGraphEditor(
		const TSharedPtr<FUICommandList>& InAdditionalCommands,
		const TAttribute<bool>& InIsEditable,
		TAttribute<FGraphAppearanceInfo> Appearance,
		TSharedPtr<SWidget> InTitleBar,
		const TAttribute<bool>& InTitleBarEnabledOnly,
		UEdGraph* InGraphToEdit,
		SGraphEditor::FGraphEditorEvents GraphEvents,
		bool InAutoExpandActionMenu,
		UEdGraph* InGraphToDiff,
		FSimpleDelegate InOnNavigateHistoryBack,
		FSimpleDelegate InOnNavigateHistoryForward,
		bool InShowPIENotification);

private:
	/** All extender delegates for the graph menus */
	TArray<FGraphEditorMenuExtender_SelectedNode> GraphEditorContextMenuExtender;
};
