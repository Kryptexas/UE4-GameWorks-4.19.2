// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "UnrealEd.h"
#include "BlueprintUtilities.h"
#include "Editor/UnrealEd/Public/EdGraphUtilities.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditor.h"
#include "SBlueprintDiff.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistryModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "GraphEditor.h"
#include "SGraphTitleBar.h"
#include "GraphDiffControl.h"

#define LOCTEXT_NAMESPACE "SBlueprintDif"

/** Individual Diff item shown in the list of diffs */
struct FDiffResultItem: public TSharedFromThis<FDiffResultItem>
{
	FDiffResultItem(FDiffSingleResult InResult): Result(InResult){}

	FDiffSingleResult Result;

	TSharedRef<SWidget>	GenerateWidget() const
	{
		FString ToolTip = Result.ToolTip;
		FLinearColor Color = Result.DisplayColor;
		FString Text = Result.DisplayString;
		if(Text.Len() == 0)
		{
			Text = LOCTEXT("DIF_UnknownDiff", "Unknown Diff").ToString();
			ToolTip = LOCTEXT("DIF_Confused", "There is an unspecified difference").ToString();
		}
		return SNew(STextBlock)
				.ToolTipText(ToolTip)
				.ColorAndOpacity(Color)
				.Text(Text);
	}
};

FListItemGraphToDiff::FListItemGraphToDiff( class SBlueprintDiff* InDiff, class UEdGraph* InGraphOld, class UEdGraph* InGraphNew, const FRevisionInfo& InRevisionOld, const FRevisionInfo& InRevisionNew )
	: GraphOld(InGraphOld), GraphNew(InGraphNew), RevisionOld(InRevisionOld), RevisionNew(InRevisionNew), Diff(InDiff)
{
	check(InGraphOld || InGraphNew); //one of them needs to exist

	//need to know when it is modified
	if(InGraphNew)
	{
		InGraphNew->AddOnGraphChangedHandler( FOnGraphChanged::FDelegate::CreateRaw(this, &FListItemGraphToDiff::OnGraphChanged));
	}
}

FListItemGraphToDiff::~FListItemGraphToDiff()
{
	if(GraphNew)
	{
		GraphNew->RemoveOnGraphChangedHandler( FOnGraphChanged::FDelegate::CreateRaw(this, &FListItemGraphToDiff::OnGraphChanged));
	}
}

TSharedRef<SWidget> FListItemGraphToDiff::GenerateWidget() 
{
	auto Graph = GraphOld ? GraphOld : GraphNew;
	
	FLinearColor Color = (GraphOld && GraphNew) ? FLinearColor::White : FLinearColor(0.3f,0.3f,1.f);

	BuildDiffSourceArray();

	const bool bHasDiffs = DiffListSource.Num() > 0;

	//give it a red color to indicate that the graph contains differences
	if(bHasDiffs)
	{
		Color =  FLinearColor(1.0f,0.2f,0.3f);
	}

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.MaxWidth(8.0f)
		[
			SNew(SImage)
			.ColorAndOpacity(Color)
			.Image(GetIconForGraph(GraphOld))

		]
	+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(Color)
			.Text(Graph->GetName())

		]
	+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.MaxWidth(8.0f)
		[
			SNew(SImage)
			.ColorAndOpacity(Color)
			.Image(GetIconForGraph(GraphNew))
		];

}

//////////////////////////////////////////////////////////////////////////
// FDiffListCommands

class FDiffListCommands : public TCommands<FDiffListCommands>
{
public:
	/** Constructor */
	FDiffListCommands() 
		: TCommands<FDiffListCommands>("DiffList", NSLOCTEXT("Diff", "Blueprint", "Blueprint Diff"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	/** Go to previous difference */
	TSharedPtr<FUICommandInfo> Previous;

	/** Go to next difference */
	TSharedPtr<FUICommandInfo> Next;

	/** Initialize commands */
	virtual void RegisterCommands() OVERRIDE;
};

void FDiffListCommands::RegisterCommands()
{
	UI_COMMAND(Previous, "Prev", "Go to previous difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7, EModifierKey::Control));
	UI_COMMAND(Next, "Next", "Go to next difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7));
}


TSharedRef<SWidget> FListItemGraphToDiff::GenerateDiffListWidget() 
{
	BuildDiffSourceArray();
	if(DiffListSource.Num() > 0)
	{
		struct SortDiff
		{
			bool operator () (const FSharedDiffOnGraph& A, const FSharedDiffOnGraph& B) const
			{
				return A->Result.Diff < B->Result.Diff;
			}
		};

		Sort(DiffListSource.GetTypedData(),DiffListSource.Num(), SortDiff());

		
		const FDiffListCommands& Commands = FDiffListCommands::Get();

		// Map commands through UI
		KeyCommands = MakeShareable(new FUICommandList );

		KeyCommands->MapAction( Commands.Previous, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::PrevDiff));
		KeyCommands->MapAction( Commands.Next, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::NextDiff));

		FToolBarBuilder ToolbarBuilder( KeyCommands.ToSharedRef(), FMultiBoxCustomization::None );
		ToolbarBuilder.AddToolBarButton(Commands.Previous, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.PrevDiff"));
		ToolbarBuilder.AddToolBarButton(Commands.Next, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.NextDiff"));

		TSharedPtr<SListViewType> DiffListRef;
		TSharedRef<SHorizontalBox> Result =	SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			.MaxWidth(350.f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(0.f)
				.AutoHeight()
				[
					ToolbarBuilder.MakeWidget()
				]
				+SVerticalBox::Slot()
				.Padding(0.f)
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
					.Padding(FMargin(2.0f))
					.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
					.ToolTipText(LOCTEXT("BlueprintDifDifferencesToolTip", "List of differences found between revisions, click to select"))
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RevisionDifferences", "Revision Differences"))
					]
				]
				+SVerticalBox::Slot()
				.Padding(1.f)
				.FillHeight(1.f)
				[
					SAssignNew(DiffList, SListViewType)
					.ItemHeight(24)
					.ListItemsSource( &DiffListSource )
					.OnGenerateRow( this, &FListItemGraphToDiff::OnGenerateRow )
					.SelectionMode( ESelectionMode::Single )
					.OnSelectionChanged(this, &FListItemGraphToDiff::OnSelectionChanged)
				]
			];
		return Result;
	}
	else
	{
		return SNew(SBorder).Visibility(EVisibility::Hidden);
	}
		
	
}

TSharedRef<ITableRow> FListItemGraphToDiff::OnGenerateRow( FSharedDiffOnGraph ParamItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return	SNew( STableRow< FSharedDiffOnGraph >, OwnerTable )
		.Content()
		[
			ParamItem->GenerateWidget()
		];
}

void FListItemGraphToDiff::OnSelectionChanged( FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType )
{
	if(Item.IsValid())
	{
		Diff->OnDiffListSelectionChanged(Item, this);
	}
}

const FSlateBrush* FListItemGraphToDiff::GetIconForGraph( UEdGraph* Graph ) 
{
	return Graph? FEditorStyle::GetBrush("BlueprintDif.HasGraph") : FEditorStyle::GetBrush("BlueprintDif.MissingGraph");
}

FText FListItemGraphToDiff::GetToolTip() 
{
	if ( GraphOld && GraphNew )
	{
		BuildDiffSourceArray();
		if ( DiffListSource.Num() > 0 )
		{
			return LOCTEXT("ContainsDifferences", "Revisions are different");
		}
		else
		{
			return LOCTEXT("GraphsIdentical", "Revisions appear to be identical");
		}
	}
	else
	{
		auto GoodGraph = GraphOld ? GraphOld : GraphNew;
		const FRevisionInfo& Revision = GraphNew ? RevisionOld : RevisionNew;
		FText RevisionText = LOCTEXT("CurrentRevision", "Current Revision");

		if ( Revision.Revision >= 0 )
		{
			RevisionText = FText::Format( LOCTEXT("Revision Number", "Revision {0}") , FText::AsNumber( Revision.Revision, NULL, FInternationalization::GetInvariantCulture() ) );
		}

		return FText::Format( LOCTEXT("MissingGraph", "Graph '{0}' missing from {1}"), FText::FromString( GoodGraph->GetName() ), RevisionText );
	}
}

void FListItemGraphToDiff::BuildDiffSourceArray()
{
	TArray<FDiffSingleResult> FoundDiffs;
	FGraphDiffControl::DiffGraphs(GraphOld, GraphNew, FoundDiffs);

	DiffListSource.Empty();
	for (auto DiffIt(FoundDiffs.CreateIterator()); DiffIt; ++DiffIt)
	{
		DiffListSource.Add(FSharedDiffOnGraph(new FDiffResultItem(*DiffIt)));
	}
}

void FListItemGraphToDiff::NextDiff()
{
	int32 Index = (GetCurrentDiffIndex() + 1) % DiffListSource.Num();

	DiffList->SetSelection(DiffListSource[Index]);
}

void FListItemGraphToDiff::PrevDiff()
{
	int32 Index = GetCurrentDiffIndex();
	if(Index == 0)
	{
		Index = DiffListSource.Num() - 1;
	}
	else
	{
		Index = (Index - 1) % DiffListSource.Num();
	}

	DiffList->SetSelection(DiffListSource[Index]);
}

int32 FListItemGraphToDiff::GetCurrentDiffIndex() 
{
	auto Selected = DiffList->GetSelectedItems();
	if(Selected.Num() == 1)
	{	
		int32 Index = 0;
		for(auto It(DiffListSource.CreateIterator());It;++It,Index++)
		{
			if(*It == Selected[0])
			{
				return Index;
			}
		}
	}
	return 0;
}

void FListItemGraphToDiff::OnGraphChanged( const FEdGraphEditAction& Action )
{
	Diff->OnGraphChanged(this);
}

void FListItemGraphToDiff::KeyWasPressed( const FKeyboardEvent& InKeyboardEvent )
{
	if ( KeyCommands.IsValid() )
	{
		KeyCommands->ProcessCommandBindings(InKeyboardEvent);
	}
}


SBlueprintDiff::FDiffPanel::FDiffPanel()
{
	Blueprint = NULL;
}

//////////////////////////////////////////////////////////////////////////
// SBlueprintDiff

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlueprintDiff::Construct( const FArguments& InArgs)
{
	LastPinTarget = NULL;
	LastOtherPinTarget = NULL;
	FDiffListCommands::Register();
	check(InArgs._BlueprintOld && InArgs._BlueprintNew);
	PanelOld.Blueprint = InArgs._BlueprintOld;
	PanelNew.Blueprint = InArgs._BlueprintNew;

	PanelOld.RevisionInfo = InArgs._OldRevision;
	PanelNew.RevisionInfo = InArgs._NewRevision;

	// sometimes we want to clearly identify the assets being diffed (when it's
	// not the same asset in each panel)
	PanelOld.bShowAssetName = InArgs._ShowAssetNames;
	PanelNew.bShowAssetName = InArgs._ShowAssetNames;

	OpenInDefaults = InArgs._OpenInDefaults;

	bLockViews = true;

	TArray<UEdGraph*> GraphsOld,GraphsNew;
	PanelOld.Blueprint->GetAllGraphs(GraphsOld);
	PanelNew.Blueprint->GetAllGraphs(GraphsNew);

	// Add handler for blueprint changing.
	PanelNew.Blueprint->OnChanged().AddSP( this, &SBlueprintDiff::OnBlueprintChanged );
	
	//Add Graphs that exist in both blueprints, or in blueprint 1 only
	for(auto It(GraphsOld.CreateConstIterator());It;It++)
	{
		UEdGraph* GraphOld = *It;
		UEdGraph* GraphNew = NULL;
		for(auto It2(GraphsNew.CreateIterator());It2;It2++)
		{
			UEdGraph* TestGraph = *It2;
			if(TestGraph && GraphOld->GetName() == TestGraph->GetName())
			{
				GraphNew = TestGraph;
				*It2 = NULL;
				break;
			}
		}
		CreateGraphEntry(GraphOld,GraphNew);
	}

	//Add graphs that only exist in 2nd(new) blueprint
	for(auto It2(GraphsNew.CreateIterator());It2;It2++)
	{
		UEdGraph* GraphNew = *It2;
		if(GraphNew != NULL)
		{
			CreateGraphEntry(NULL,GraphNew);
		}
	}
	auto DefaultEmptyPanel = SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintDifGraphsToolTip", "Select Graph to Diff").ToString())
		];

	this->ChildSlot
		[	
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Content()
			[
				SNew(SBorder)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SNew(SSplitter)
						+SSplitter::Slot()
						.Value(0.2f)
						[
							SNew(SBorder)
							[
								SNew(SSplitter)
								.Orientation(Orient_Vertical)
								+SSplitter::Slot()
								.Value(0.3f)
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot()
									.AutoHeight()
									.HAlign(HAlign_Left)
									[
										//toggle lock button
										SNew(SButton)
										.OnClicked(this, &SBlueprintDiff::OnToggleLockView)
										.Content()
										[
											SNew(SImage)
											.Image(this, &SBlueprintDiff::GetLockViewImage)
											.ToolTipText(LOCTEXT("BPDifLock", "Lock the blueprint views?").ToString())
										]
									]
									+SVerticalBox::Slot()
									.FillHeight(1.f)
									[
										//graph selection 
										SNew(SBorder)
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.AutoHeight()
											[
												SNew(SBorder)
												.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
												.Padding(FMargin(2.0f))
												.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
												.ToolTipText(LOCTEXT("BlueprintDifGraphsToolTip", "Select Graph to Diff").ToString())
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(LOCTEXT("BlueprintDifGraphs", "Graphs").ToString())
												]
											]
											+SVerticalBox::Slot()
											.FillHeight(1.f)
											[
												SAssignNew(GraphsToDiff, SListViewType)
												.ItemHeight(24)
												.ListItemsSource( &Graphs )
												.OnGenerateRow( this, &SBlueprintDiff::OnGenerateRow )
												.SelectionMode( ESelectionMode::Single )
												.OnSelectionChanged(this, &SBlueprintDiff::OnSelectionChanged)
											]
										]
									]
								]
								+SSplitter::Slot()
								.Value(0.7f)
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot()
									.AutoHeight()
									[
										//open in p4dif tool
										SNew(SButton)
										.OnClicked(this, &SBlueprintDiff::OnOpenInDefaults)
										.Content()
										[
											SNew(STextBlock)
											.Text(LOCTEXT("DifBlueprintDefaultsToolTip", "Diff Blueprint Defaults").ToString())
										]
									]
									+SVerticalBox::Slot()
									.FillHeight(1.f)
									[
										SAssignNew(DiffListBorder, SBorder)
									]
								]
							]
						]
						+SSplitter::Slot()
						.Value(0.8f)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.FillWidth(1.f)
							[
								//dif window
								SNew(SSplitter)
								+SSplitter::Slot()
								.Value(0.5f)
								[
									//left blueprint
									SAssignNew(PanelOld.GraphEditorBorder, SBorder)
									.VAlign(VAlign_Fill)
									[
										DefaultEmptyPanel
									]
								]
								+SSplitter::Slot()
								.Value(0.5f)
								[
									//right blueprint
									SAssignNew(PanelNew.GraphEditorBorder, SBorder)
									.VAlign(VAlign_Fill)
									[
										DefaultEmptyPanel
									]
								]
							]
						]
					]
				]
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ITableRow> SBlueprintDiff::OnGenerateRow( FGraphToDiff ParamItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return	SNew( STableRow< FGraphToDiff >, OwnerTable )
			.ToolTipText(ParamItem->GetToolTip())
			.Content()
			[
				ParamItem->GenerateWidget()
			];
}

void SBlueprintDiff::CreateGraphEntry( class UEdGraph* GraphOld, class UEdGraph* GraphNew )
{
	Graphs.Add(FGraphToDiff(new FListItemGraphToDiff(this, GraphOld, GraphNew, PanelOld.RevisionInfo, PanelNew.RevisionInfo)));
}

void SBlueprintDiff::OnSelectionChanged( FGraphToDiff Item, ESelectInfo::Type SelectionType) 
{
	if(SelectionType != ESelectInfo::OnMouseClick || !Item.IsValid())
	{
		return;
	}

	FocusOnGraphRevisions(Item->GetGraphOld(), Item->GetGraphNew(), Item.Get());

}

void SBlueprintDiff::OnGraphChanged(FListItemGraphToDiff* Diff)
{
	if(PanelNew.GraphEditor.IsValid() && PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Diff->GetGraphNew())
	{
		FocusOnGraphRevisions(Diff->GetGraphOld(), Diff->GetGraphNew(), Diff);
	}
}

void SBlueprintDiff::FocusOnGraphRevisions( class UEdGraph* GraphOld, class UEdGraph* GraphNew, FListItemGraphToDiff* Diff )
{
	DisablePinDiffFocus();
	PanelOld.GeneratePanel(GraphOld,GraphNew);
	PanelNew.GeneratePanel(GraphNew, GraphOld);

	ResetGraphEditors();

	//set new diff list
	DiffListBorder->SetContent(Diff->GenerateDiffListWidget());
}

void SBlueprintDiff::OnDiffListSelectionChanged(const TSharedPtr<struct FDiffResultItem>& TheDiff, FListItemGraphToDiff* GraphDiffer )
{
	DisablePinDiffFocus();

	//focus the graph onto the diff that was clicked on
	FDiffSingleResult Result = TheDiff->Result;
	if(Result.Pin1)
	{
		PanelNew.GraphEditor.Pin()->ClearSelectionSet();
		PanelOld.GraphEditor.Pin()->ClearSelectionSet();
	
		if(Result.Pin1)
		{
			LastPinTarget = Result.Pin1;
			Result.Pin1->bIsDiffing = true;
			GetGraphEditorForGraph(Result.Pin1->GetOwningNode()->GetGraph())->JumpToPin(Result.Pin1);
		}

		if(Result.Pin2)
		{
			LastOtherPinTarget = Result.Pin2;
			Result.Pin2->bIsDiffing = true;
			GetGraphEditorForGraph(Result.Pin2->GetOwningNode()->GetGraph())->JumpToPin(Result.Pin2);
		}
	}
	else if(Result.Node1)
	{
		PanelNew.GraphEditor.Pin()->ClearSelectionSet();
		PanelOld.GraphEditor.Pin()->ClearSelectionSet();

		if(Result.Node2)
		{
			GetGraphEditorForGraph(Result.Node2->GetGraph())->JumpToNode(Result.Node2, false);
		}
		GetGraphEditorForGraph(Result.Node1->GetGraph())->JumpToNode(Result.Node1, false);
	}

}

FReply	SBlueprintDiff::OnToggleLockView()
{
	bLockViews = !bLockViews;
	ResetGraphEditors();
	return FReply::Handled();
}

const FSlateBrush* SBlueprintDiff::GetLockViewImage() const
{
	return bLockViews ? FEditorStyle::GetBrush("GenericLock") : FEditorStyle::GetBrush("GenericUnlock");
}

void SBlueprintDiff::ResetGraphEditors()
{
	if(PanelOld.GraphEditor.IsValid() && PanelNew.GraphEditor.IsValid())
	{
		if(bLockViews)
		{
			PanelOld.GraphEditor.Pin()->LockToGraphEditor(PanelNew.GraphEditor);
			PanelNew.GraphEditor.Pin()->LockToGraphEditor(PanelOld.GraphEditor);
		}
		else
		{
			PanelOld.GraphEditor.Pin()->LockToGraphEditor(TWeakPtr<SGraphEditor>());
			PanelNew.GraphEditor.Pin()->LockToGraphEditor(TWeakPtr<SGraphEditor>());
		}	
	}
}

void SBlueprintDiff::FDiffPanel::GeneratePanel(UEdGraph* Graph, UEdGraph* GraphToDiff)
{
	TSharedPtr<SWidget> Widget = SNew(SBorder)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text( LOCTEXT("BPDifPanelNoGraphTip", "Graph does not exist in this revision").ToString())
								];
	if(Graph)
	{
		SGraphEditor::FGraphEditorEvents InEvents;

		FGraphAppearanceInfo AppearanceInfo;
		AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_BlueprintDif", "DIFF").ToString();

		if ( !GraphEditorCommands.IsValid() )
		{
			GraphEditorCommands = MakeShareable( new FUICommandList() );

			GraphEditorCommands->MapAction( FGenericCommands::Get().Copy,
				FExecuteAction::CreateRaw( this, &FDiffPanel::CopySelectedNodes ),
				FCanExecuteAction::CreateRaw( this, &FDiffPanel::CanCopyNodes )
				);
		}

		auto Editor = SNew(SGraphEditor)
			.AdditionalCommands(GraphEditorCommands)
			.GraphToEdit(Graph)
			.GraphToDiff(GraphToDiff)
			.IsEditable(false)
			.TitleBarEnabledOnly(false)
			.TitleBar(SNew(SBorder).HAlign(HAlign_Center)
						[
							SNew(STextBlock).Text(GetTitle())
						])
			.Appearance(AppearanceInfo)
			.GraphEvents(InEvents);

		GraphEditor = Editor;
		Widget = Editor;
	}

	GraphEditorBorder->SetContent(Widget.ToSharedRef());
}

FGraphPanelSelectionSet SBlueprintDiff::FDiffPanel::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GraphEditor.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void SBlueprintDiff::FDiffPanel::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);
}


FString SBlueprintDiff::FDiffPanel::GetTitle() const
{
	FString Title = LOCTEXT("CurrentRevision", "Current Revision").ToString();

	// if this isn't the current working version being displayed
	if (RevisionInfo.Revision >= 0)
	{
		FString DateString = RevisionInfo.Date.ToString(TEXT("%m/%d/%Y"));
		if (bShowAssetName)
		{
			FString AssetName = Blueprint->GetName();

			FText LocalizedFormat = LOCTEXT("NamedRevisionDiff", "%s - Revision %i, CL %i, %s");
			Title = FString::Printf(*LocalizedFormat.ToString(), *AssetName, RevisionInfo.Revision, RevisionInfo.Changelist, *DateString);
		}
		else
		{
			FText LocalizedFormat = LOCTEXT("PreviousRevisionDif", "Revision %i, CL %i, %s");
			Title = FString::Printf(*LocalizedFormat.ToString(), RevisionInfo.Revision, RevisionInfo.Changelist, *DateString);
		}
	}
	else if (bShowAssetName)
	{
		FString AssetName = Blueprint->GetName();

		FText LocalizedFormat = LOCTEXT("NamedCurrentRevision", "%s - Current Revision");
		Title = FString::Printf(*LocalizedFormat.ToString(), *AssetName);
	}

	return Title;
}

bool SBlueprintDiff::FDiffPanel::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

FReply SBlueprintDiff::OnOpenInDefaults()
{
	OpenInDefaults.ExecuteIfBound(PanelOld.Blueprint, PanelNew.Blueprint);
	return FReply::Handled();
}

SGraphEditor* SBlueprintDiff::GetGraphEditorForGraph( UEdGraph* Graph ) const
{
	if(PanelOld.GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return PanelOld.GraphEditor.Pin().Get();
	}
	else if(PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return PanelNew.GraphEditor.Pin().Get();
	}
	checkNoEntry();
	return NULL;
}

FReply SBlueprintDiff::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) 
{
	TArray< FGraphToDiff>  Selected = GraphsToDiff->GetSelectedItems();
	for(auto It(Selected.CreateIterator());It;++It)
	{
		FGraphToDiff& Diff = *It;
		Diff->KeyWasPressed(InKeyboardEvent);
	}
	return FReply::Handled();
}

void SBlueprintDiff::DisablePinDiffFocus()
{
	if(LastPinTarget)
	{
		LastPinTarget->bIsDiffing = false;
	}
	if(LastOtherPinTarget)
	{
		LastOtherPinTarget->bIsDiffing = false;
	}
}

void SBlueprintDiff::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	DisablePinDiffFocus();

 	TArray<UEdGraph*> GraphsOld,GraphsNew;
 	PanelOld.Blueprint->GetAllGraphs(GraphsOld);
 	PanelNew.Blueprint->GetAllGraphs(GraphsNew);
 	PanelOld.GeneratePanel(GraphsOld[0],GraphsNew[0]);
  	PanelNew.GeneratePanel(GraphsNew[0], GraphsOld[0]);
 
 	ResetGraphEditors();
}

#undef LOCTEXT_NAMESPACE

