// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SBlueprintDiff.h"

class MERGE_API SBlueprintMerge : public SBlueprintDiff
{
public:
	SLATE_BEGIN_ARGS(SBlueprintMerge){}
	SLATE_ARGUMENT(class UBlueprint*, BlueprintLocal)
	SLATE_ARGUMENT( SBlueprintDiff::FArguments, BaseArgs )
	SLATE_ARGUMENT( TWeakPtr<SWindow>, OwningWindow )
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	TSharedRef<ITableRow> OnGenerateRow( TSharedPtr<struct FDiffSingleResult> ParamItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnDiffListSelectionChanged(TSharedPtr<struct FDiffSingleResult> Item, ESelectInfo::Type SelectionType);

protected:
	/** Overrides, see base class for documentation */
	virtual void ResetGraphEditors() override;
	virtual TSharedRef<SWidget> GenerateDiffWindow() override;
	virtual TSharedRef<SWidget> GenerateToolbar() override;
	virtual void HandleGraphChanged(const FString& GraphName) override;
	virtual void OnSelectionChanged(FGraphToDiff Item, ESelectInfo::Type SelectionType) override;
	virtual SGraphEditor* GetGraphEditorForGraph(UEdGraph* Graph) const override;
	
	/** Helper functions */
	TSharedRef< SWidget > GenerateDiffView(TArray<FDiffSingleResult>& Diffs, TSharedPtr< const FUICommandInfo > CommandNext, TSharedPtr< const FUICommandInfo > CommandPrev, TArray< TSharedPtr<FDiffSingleResult> >& SharedResults);

	/** Event handlers */
	FReply OnAcceptResultClicked();
	FReply OnCancelClicked();
	FReply OnTakeLocalClicked();
	FReply OnTakeRemoteClicked();
	FReply OnTakeBaseClicked();
	void StageBlueprint(UBlueprint const* DesiredBP);

	// Raw pointer?!
	UBlueprint* BlueprintResult;
	FDiffPanel PanelLocal;
	TSharedPtr<SBorder>		 EditorBorder;
	TWeakPtr<SWindow>		OwningWindow;

	// This has to be allocated here because SListView cannot own the list
	// that it is displaying. It also seems like the display list *has*
	// to be a list of TSharedPtrs.
	TArray< TSharedPtr<FDiffSingleResult> > LocalDiffResults;
	TArray< TSharedPtr<FDiffSingleResult> > RemoteDiffResults;
};
