// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorUndoClient.h"
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"
#include "STableRow.h"
#include "STableViewBase.h"
#include "STreeView.h"
#include "NiagaraSystemViewModel.h"
#include "TickableEditorObject.h"
#include "WeakObjectPtrTemplates.h"
#include "NiagaraDataSet.h"
#include "SharedPointer.h"
#include "Map.h"
#include "SCheckBox.h"
#include "SSearchBox.h"
#include "SComboButton.h"
#include "SMultiLineEditableTextBox.h"

class SNiagaraGeneratedCodeView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNiagaraGeneratedCodeView)
	{}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraSystemViewModel> InSystemViewModel);
	virtual ~SNiagaraGeneratedCodeView();

	void OnCodeCompiled();

protected:

	void UpdateUI();

	struct TabInfo
	{
		FText UsageName;
		FText Hlsl;
		ENiagaraScriptUsage Usage;
		FGuid UsageId;

		TArray<FString> HlslByLines;
		TSharedPtr<SMultiLineEditableText> Text;
		TSharedPtr<SScrollBar> HorizontalScrollBar;
		TSharedPtr<SScrollBar> VerticalScrollBar;
		TSharedPtr<SVerticalBox> Container;
	};

	TArray<TabInfo> GeneratedCode;
	TSharedPtr<SComboButton> ScriptNameCombo;
	TSharedPtr<SHorizontalBox> ScriptNameContainer;
	TSharedPtr<SVerticalBox> TextBodyContainer;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STextBlock> SearchFoundMOfNText;
	TArray<FTextLocation> ActiveFoundTextEntries;
	int32 CurrentFoundTextEntry;

	void SetSearchMofN();

	void SelectedEmitterHandlesChanged();
	void OnTabChanged(uint32 Tab);
	bool GetTabCheckedState(uint32 Tab) const;
	EVisibility GetViewVisibility(uint32 Tab) const;
	bool TabHasScriptData() const;
	FReply OnCopyPressed();
	void OnSearchTextChanged(const FText& InFilterText);
	void OnSearchTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
	FReply SearchUpClicked();
	FReply SearchDownClicked();
	TSharedRef<SWidget> MakeScriptMenu();
	void DoSearch(const FText& InFilterText);
	FText GetCurrentScriptNameText() const;

	FText GetSearchText() const;

	uint32 TabState;

	TSharedPtr<FNiagaraSystemViewModel> SystemViewModel;

	UEnum* ScriptEnum;
};


