// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraScriptGraph.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraGraph.h"
#include "NiagaraNode.h"

#include "AssetEditorManager.h"
#include "GraphEditor.h"
#include "EditorStyleSet.h"
#include "SBorder.h"
#include "SBoxPanel.h"
#include "TextLayout.h"
#include "SErrorText.h"
#include "STextBlock.h"


#define LOCTEXT_NAMESPACE "NiagaraScriptGraph"

void SNiagaraScriptGraph::Construct(const FArguments& InArgs, TSharedRef<FNiagaraScriptGraphViewModel> InViewModel)
{
	ViewModel = InViewModel;
	ViewModel->GetSelection()->OnSelectedObjectsChanged().AddSP(this, &SNiagaraScriptGraph::ViewModelSelectedNodesChanged);
	ViewModel->OnNodesPasted().AddSP(this, &SNiagaraScriptGraph::NodesPasted);
	ViewModel->OnGraphChanged().AddSP(this, &SNiagaraScriptGraph::GraphChanged);
	bUpdatingGraphSelectionFromViewModel = false;

	GraphTitle = InArgs._GraphTitle;

	GraphEditor = ConstructGraphEditor();

	ChildSlot
	[
		GraphEditor.ToSharedRef()
	];
}

TSharedRef<SGraphEditor> SNiagaraScriptGraph::ConstructGraphEditor()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "NIAGARA");

	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 3.0f, 0.0f)
			[
				SNew(SErrorText)
				.Visibility(ViewModel.ToSharedRef(), &FNiagaraScriptGraphViewModel::GetGraphErrorTextVisible)
				.BackgroundColor(ViewModel.ToSharedRef(), &FNiagaraScriptGraphViewModel::GetGraphErrorColor)
				.ToolTipText(ViewModel.ToSharedRef(), &FNiagaraScriptGraphViewModel::GetGraphErrorMsgToolTip)
				.ErrorText(ViewModel->GetGraphErrorText())
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ViewModel.ToSharedRef(), &FNiagaraScriptGraphViewModel::GetDisplayName)
				.TextStyle(FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
				.Justification(ETextJustify::Center)
			]
		];

	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &SNiagaraScriptGraph::GraphEditorSelectedNodesChanged);
	Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SNiagaraScriptGraph::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(ViewModel->GetCommands())
		.Appearance(AppearanceInfo)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(ViewModel->GetGraph())
		.GraphEvents(Events);
}

void SNiagaraScriptGraph::ViewModelSelectedNodesChanged()
{
	if (FNiagaraEditorUtilities::SetsMatch(GraphEditor->GetSelectedNodes(), ViewModel->GetSelection()->GetSelectedObjects()) == false)
	{
		bUpdatingGraphSelectionFromViewModel = true;
		GraphEditor->ClearSelectionSet();
		for (UObject* SelectedNode : ViewModel->GetSelection()->GetSelectedObjects())
		{
			UEdGraphNode* GraphNode = Cast<UEdGraphNode>(SelectedNode);
			if (GraphNode != nullptr)
			{
				GraphEditor->SetNodeSelection(GraphNode, true);
			}
		}
		bUpdatingGraphSelectionFromViewModel = false;
	}
}

void SNiagaraScriptGraph::GraphEditorSelectedNodesChanged(const TSet<UObject*>& SelectedNodes)
{
	if (bUpdatingGraphSelectionFromViewModel == false)
	{
		ViewModel->GetSelection()->SetSelectedObjects(SelectedNodes);
	}
}

void SNiagaraScriptGraph::OnNodeDoubleClicked(UEdGraphNode* ClickedNode)
{
	UNiagaraNode* NiagaraNode = Cast<UNiagaraNode>(ClickedNode);
	if (NiagaraNode != nullptr)
	{
		UObject* ReferencedAsset = NiagaraNode->GetReferencedAsset();
		if (ReferencedAsset != nullptr)
		{
			FAssetEditorManager::Get().OpenEditorForAsset(ReferencedAsset);
		}
	}
}

void SNiagaraScriptGraph::NodesPasted(const TSet<UEdGraphNode*>& PastedNodes)
{
	if (PastedNodes.Num() != 0)
	{
		PositionPastedNodes(PastedNodes);
		GraphEditor->NotifyGraphChanged();
	}
}

void SNiagaraScriptGraph::PositionPastedNodes(const TSet<UEdGraphNode*>& PastedNodes)
{
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (UEdGraphNode* PastedNode : PastedNodes)
	{
		AvgNodePosition.X += PastedNode->NodePosX;
		AvgNodePosition.Y += PastedNode->NodePosY;
	}

	float InvNumNodes = 1.0f / float(PastedNodes.Num());
	AvgNodePosition.X *= InvNumNodes;
	AvgNodePosition.Y *= InvNumNodes;

	FVector2D PasteLocation = GraphEditor->GetPasteLocation();
	for (UEdGraphNode* PastedNode : PastedNodes)
	{
		PastedNode->NodePosX = (PastedNode->NodePosX - AvgNodePosition.X) + PasteLocation.X;
		PastedNode->NodePosY = (PastedNode->NodePosY - AvgNodePosition.Y) + PasteLocation.Y;

		PastedNode->SnapToGrid(16);
	}
}

void SNiagaraScriptGraph::GraphChanged()
{
	GraphEditor = ConstructGraphEditor();

	ChildSlot
	[
		GraphEditor.ToSharedRef()
	];
}

#undef LOCTEXT_NAMESPACE // "NiagaraScriptGraph"