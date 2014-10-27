// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"
#include "DiffResults.h"
#include "GraphDiffControl.h"
#include "SMergeGraphView.h"

#define LOCTEXT_NAMESPACE "SMergeGraphView"

struct FGraphMergeConflict
{
	FGraphMergeConflict(FName InGraphName, const FDiffSingleResult& InFirstResult, const FDiffSingleResult& InSecondResult )
		: GraphName(InGraphName)
		, FirstResult(InFirstResult)
		, SecondResult(InSecondResult)
	{
	}

	FName GraphName;
	FDiffSingleResult FirstResult;
	FDiffSingleResult SecondResult;
};

// This cludge is used to move arrays of structures into arrays of 
// shared ptrs of those structures. It is used because SListView
// does not support arrays of values:
template< typename T >
TArray< TSharedPtr<T> > ConcreteToShared(const TArray<T>& Values)
{
	TArray< TSharedPtr<T> > Ret;
	for (const auto& Value : Values)
	{
		Ret.Add(TSharedPtr<T>(new T(Value)));
	}
	return Ret;
}

struct FBlueprintRevPair
{
	FBlueprintRevPair(const UBlueprint* InBlueprint, const FRevisionInfo& InRevData)
	: Blueprint(InBlueprint)
	, RevData(InRevData)
	{
	}

	const UBlueprint* Blueprint;
	const FRevisionInfo& RevData;
};

struct FMergeGraphRowEntry
{
	FName GraphName;

	// These lists contain shared ptrs because they are displayed by a
	// SListView, and it currently does not support lists of values.
	TArray< TSharedPtr<FDiffSingleResult> > Differences;

	bool bExistsInRemote;
	bool bExistsInBase;
	bool bExistsInLocal;
	bool bDiffersInRemote;
	bool bDiffersInLocal;
};

static UEdGraph* FindGraphByName(UBlueprint const& FromBlueprint, const FName& GraphName)
{
	TArray<UEdGraph*> Graphs;
	FromBlueprint.GetAllGraphs(Graphs);

	UEdGraph* Ret = NULL;
	if (UEdGraph** Result = Graphs.FindByPredicate(FMatchFName(GraphName)))
	{
		Ret = *Result;
	}
	return Ret;
}

static const UEdGraphNode* FindNodeInUnrelatedGraph(UEdGraph* Graph, UEdGraphNode* TargetNode)
{
	check(TargetNode->GetGraph() != Graph); // purpose of this function is to find a node in a different version of its parent graph

	TArray< FGraphDiffControl::FNodeMatch > PriorMatches;
	FGraphDiffControl::FNodeMatch NodeMatch = FGraphDiffControl::FindNodeMatch(Graph, TargetNode, PriorMatches);
	return NodeMatch.NewNode;
}

static FDiffPanel InitializePanel(const FBlueprintRevPair& BlueprintRevPair)
{
	FDiffPanel Ret;
	Ret.Blueprint = BlueprintRevPair.Blueprint;
	SAssignNew(Ret.GraphEditorBorder, SBorder)
		.VAlign(VAlign_Fill)
		[
			SBlueprintDiff::DefaultEmptyPanel()
		];
	Ret.RevisionInfo = BlueprintRevPair.RevData;
	Ret.bShowAssetName = false;
	return Ret;
}

static TArray< TSharedPtr<FMergeGraphRowEntry> > GenerateDiffListItems(const FBlueprintRevPair& RemoteBlueprint, const FBlueprintRevPair& BaseBlueprint, const FBlueprintRevPair& LocalBlueprint,  TArray< TSharedPtr<FGraphMergeConflict> >& OutMergeConflicts )
{
	// Index all the graphs by name, we use the name of the graph as the 
	// basis of comparison between the various versions of the blueprint.
	TMap< FName, UEdGraph* > RemoteGraphMap, BaseGraphMap, LocalGraphMap;
	// We also want the set of all graph names in these blueprints, so that we 
	// can iterate over every graph.
	TSet< FName > AllGraphNames;
	{
		TArray<UEdGraph*> GraphsRemote, GraphsBase, GraphsLocal;
		RemoteBlueprint.Blueprint->GetAllGraphs(GraphsRemote);
		BaseBlueprint.Blueprint->GetAllGraphs(GraphsBase);
		LocalBlueprint.Blueprint->GetAllGraphs(GraphsLocal);

		const auto ToMap = [&AllGraphNames](const TArray<UEdGraph*>& InList, TMap<FName, UEdGraph*>& OutMap)
		{
			for (auto Graph : InList)
			{
				OutMap.Add(Graph->GetFName(), Graph);
				AllGraphNames.Add(Graph->GetFName());
			}
		};
		ToMap(GraphsRemote, RemoteGraphMap);
		ToMap(GraphsBase, BaseGraphMap);
		ToMap(GraphsLocal, LocalGraphMap);
	}

	TArray< TSharedPtr<FMergeGraphRowEntry> > Ret;
	{
		const auto GenerateDifferences = [](UEdGraph* GraphNew, UEdGraph** GraphOld)
		{
			TArray<FDiffSingleResult> Results;
			FGraphDiffControl::DiffGraphs(GraphOld ? *GraphOld : NULL, GraphNew, Results);
			return Results;
		};

		for (const auto& GraphName : AllGraphNames)
		{
			TArray< FDiffSingleResult > RemoteDifferences;
			TArray< FDiffSingleResult > LocalDifferences;
			bool bExistsInRemote, bExistsInBase, bExistsInLocal;

			{
				UEdGraph** RemoteGraph = RemoteGraphMap.Find(GraphName);
				UEdGraph** BaseGraph = BaseGraphMap.Find(GraphName);
				UEdGraph** LocalGraph = LocalGraphMap.Find(GraphName);

				if (RemoteGraph)
				{
					RemoteDifferences = GenerateDifferences(*RemoteGraph, BaseGraph);
				}

				if (LocalGraph)
				{
					LocalDifferences = GenerateDifferences(*LocalGraph, BaseGraph);
				}

				// 'join' the local differences and remote differences by noting changes
				// that affected the same common base:
				{
					for (const auto& RemoteDifference : RemoteDifferences)
					{
						const FDiffSingleResult* ConflictingDifference = nullptr;

						for (const auto& LocalDifference : LocalDifferences)
						{
							if (RemoteDifference.Node1 == LocalDifference.Node1 )
							{
								if( RemoteDifference.Diff == EDiffType::NODE_REMOVED || 
									LocalDifference.Diff == EDiffType::NODE_REMOVED ||
									RemoteDifference.Pin1 == LocalDifference.Pin1 )
								{
									ConflictingDifference = &LocalDifference;
									break;
								}
							}
						}

						if (ConflictingDifference != nullptr)
						{
							// For now, we don't want to create a hard conflict for changes that don't effect runtime behavior:
							if (RemoteDifference.Diff == EDiffType::NODE_MOVED ||
								RemoteDifference.Diff == EDiffType::NODE_COMMENT)
							{
								continue;
							}

							OutMergeConflicts.Push(MakeShareable( new FGraphMergeConflict( GraphName, RemoteDifference, *ConflictingDifference)) );
						}
						else
						{
							// no conflict, we want to be able to automatically apply this remote change
							// to the local revision:
							switch (RemoteDifference.Diff)
							{
							case EDiffType::NODE_REMOVED:
								{
									// Find the corresponding object in the local graph:
							
									const UEdGraphNode* TargetNode = FindNodeInUnrelatedGraph(*LocalGraph, RemoteDifference.Node1);
									check(TargetNode); // should have been a conflict!
								}
								break;
							case EDiffType::NODE_ADDED:


								break;
							case EDiffType::PIN_LINKEDTO_NUM_DEC:
								
								break;
							case EDiffType::PIN_LINKEDTO_NUM_INC:
								break;
							case EDiffType::PIN_DEFAULT_VALUE:
								break;
							case EDiffType::PIN_TYPE_CATEGORY:
								break;
							case EDiffType::PIN_TYPE_SUBCATEGORY:
								break;
							case EDiffType::PIN_TYPE_SUBCATEGORY_OBJECT:
								break;
							case EDiffType::PIN_TYPE_IS_ARRAY:
								break;
							case EDiffType::PIN_TYPE_IS_REF:
								break;
							case EDiffType::PIN_LINKEDTO_NODE:
								break;
							case EDiffType::NODE_MOVED:

								break;
							case EDiffType::TIMELINE_LENGTH:
							case EDiffType::TIMELINE_AUTOPLAY:
							case EDiffType::TIMELINE_LOOP:
							case EDiffType::TIMELINE_NUM_TRACKS:
							case EDiffType::TIMELINE_TRACK_MODIFIED:
								check(false); // doesn't apply to blueprint
								break;
							case EDiffType::NODE_PIN_COUNT:
								break;
							case EDiffType::NODE_COMMENT:
								break;
							case EDiffType::NODE_PROPERTY:
								break;
							case EDiffType::NO_DIFFERENCE:
								check(false);
							}
						}
					}
				}

				bExistsInRemote = RemoteGraph != NULL;
				bExistsInBase = BaseGraph != NULL;
				bExistsInLocal = LocalGraph != NULL;
			}

			TArray<FDiffSingleResult> AllDifferences(RemoteDifferences);
			AllDifferences.Append(LocalDifferences);

			FMergeGraphRowEntry NewEntry = {
				GraphName,
				ConcreteToShared(AllDifferences),
				bExistsInRemote,
				bExistsInBase,
				bExistsInLocal,
				RemoteDifferences.Num() != 0,
				LocalDifferences.Num() != 0
			};
			Ret.Add(TSharedPtr<FMergeGraphRowEntry>(new FMergeGraphRowEntry(NewEntry)));
		}
	}

	return Ret;
}

static void LockViews(TArray<FDiffPanel>& Views, bool bAreLocked)
{
	for (auto& Panel : Views)
	{
		auto GraphEditor = Panel.GraphEditor.Pin();
		if (GraphEditor.IsValid())
		{
			// lock this panel to ever other panel:
			for (auto& OtherPanel : Views)
			{
				auto OtherGraphEditor = OtherPanel.GraphEditor.Pin();
				if (OtherGraphEditor.IsValid() &&
					OtherGraphEditor != GraphEditor)
				{
					if (bAreLocked)
					{
						GraphEditor->LockToGraphEditor(OtherGraphEditor);
					}
					else
					{
						GraphEditor->UnlockFromGraphEditor(OtherGraphEditor);
					}
				}
			}
		}
	}
}

FDiffPanel& GetDiffPanelForNode(const UEdGraphNode& Node, TArray< FDiffPanel >& Panels)
{
	for (auto& Panel : Panels)
	{
		auto GraphEditor = Panel.GraphEditor.Pin();
		if (GraphEditor.IsValid())
		{
			if (Node.GetGraph() == GraphEditor->GetCurrentGraph())
			{
				return Panel;
			}
		}
	}
	checkf(false, TEXT("Looking for node %s but it cannot be found in provided panels"), *Node.GetName());
	static FDiffPanel Default;
	return Default;
}

// Some color constants used by the merge view:
const FLinearColor MergeBlue = FLinearColor(0.3f, 0.3f, 1.f);
const FLinearColor MergeRed = FLinearColor(1.0f, 0.2f, 0.3f);
const FLinearColor MergeYellow = FLinearColor::Yellow;
const FLinearColor MergeWhite = FLinearColor::White;

void SMergeGraphView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData )
{
	CurrentMergeConflict = INDEX_NONE;

	Data = InData;
	bViewsAreLocked = true;
	DiffResultsListData = NULL;

	TArray<FBlueprintRevPair> BlueprintsForDisplay;
	// EMergeParticipant::MERGE_PARTICIPANT_REMOTE
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintRemote, InData.RevisionRemote));
	// EMergeParticipant::MERGE_PARTICIPANT_BASE
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintBase, InData.RevisionBase));
	// EMergeParticipant::MERGE_PARTICIPANT_LOCAL
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintLocal, FRevisionInfo()));

	for (const auto& BlueprintRevPair : BlueprintsForDisplay)
	{
		DiffPanels.Add(InitializePanel(BlueprintRevPair));
	}

	TSharedRef<SSplitter> PanelContainer = SNew(SSplitter);
	for (const auto& Panel : DiffPanels)
	{
		PanelContainer->AddSlot()[Panel.GraphEditorBorder.ToSharedRef()];
	}

	DifferencesFromBase = GenerateDiffListItems(BlueprintsForDisplay[EMergeParticipant::MERGE_PARTICIPANT_REMOTE]
												, BlueprintsForDisplay[EMergeParticipant::MERGE_PARTICIPANT_BASE]
												, BlueprintsForDisplay[EMergeParticipant::MERGE_PARTICIPANT_LOCAL]
												, MergeConflicts);

	// This is the function we'll use to generate a row in the control that lists all the available graphs:
	const auto RowGenerator = [](TSharedPtr<FMergeGraphRowEntry> ParamItem, const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow>
	{
		// blue indicates added, red indicates changed, yellow indicates removed, white indicates no change:
		const auto ComputeColorForRevision = [](bool bExistsInBase, bool bExistsInThisRevision, bool bHasChangesInThisRevision) -> FLinearColor
		{
			if (bExistsInBase)
			{
				if (!bExistsInThisRevision)
				{
					return MergeYellow;
				}
				else if (bHasChangesInThisRevision)
				{
					return MergeRed;
				}
			}
			if (!bExistsInBase)
			{
				if (bExistsInThisRevision)
				{
					return MergeBlue;
				}
			}

			return MergeWhite;
		};

		FLinearColor RemoteColor = ComputeColorForRevision(ParamItem->bExistsInBase, ParamItem->bExistsInRemote, ParamItem->bDiffersInRemote);
		FLinearColor BaseColor = ComputeColorForRevision(ParamItem->bExistsInBase, ParamItem->bExistsInBase, false);
		FLinearColor LocalColor = ComputeColorForRevision(ParamItem->bExistsInBase, ParamItem->bExistsInLocal, ParamItem->bDiffersInLocal);

		const auto Box = [](bool bIsPresent, FLinearColor Color) -> SHorizontalBox::FSlot&
		{
			return SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.MaxWidth(8.0f)
				[
					SNew(SImage)
					.ColorAndOpacity(Color)
					.Image(bIsPresent ? FEditorStyle::GetBrush("BlueprintDif.HasGraph") : FEditorStyle::GetBrush("BlueprintDif.MissingGraph"))
				];
		};

		return SNew(STableRow< TSharedPtr<FMergeGraphRowEntry> >, OwnerTable)
			.ToolTipText(LOCTEXT("MergeGraphsDifferentText", "@todo doc"))
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.ColorAndOpacity(FLinearColor::White)
					.Text(ParamItem->GraphName.GetPlainNameString())
				]
				+ Box(ParamItem->bExistsInRemote, RemoteColor)
				+ Box(ParamItem->bExistsInBase, BaseColor)
				+ Box(ParamItem->bExistsInLocal, LocalColor)
			];
	};

	ChildSlot
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.1f)
			[
				// ToolPanel.GroupBorder is currently what gives this control its dark grey appearance:
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.Padding(FMargin(1.0f))
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot()
					.Value(.3f)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Left)
							[
								SNew(SButton)
								.OnClicked(this, &SMergeGraphView::OnToggleLockView)
								.Content()
								[
									SNew(SImage)
									.Image(this, &SMergeGraphView::GetLockViewImage)
									.ToolTipText(LOCTEXT("BPDifLock", "Lock the blueprint views?"))
								]
							]
							+ SVerticalBox::Slot()
								[
									SNew(SListView< TSharedPtr<FMergeGraphRowEntry> >)
									.ItemHeight(24)
									.ListItemsSource(&DifferencesFromBase)
									.OnGenerateRow(SListView< TSharedPtr<FMergeGraphRowEntry> >::FOnGenerateRow::CreateStatic(RowGenerator))
									.SelectionMode(ESelectionMode::Single)
									.OnSelectionChanged(this, &SMergeGraphView::OnGraphListSelectionChanged)
								]
						]
					]
					+ SSplitter::Slot()
						.Value(.7f)
						[
							SAssignNew(DiffResultsWidget, SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						]
				]
			]
			+ SSplitter::Slot()
				.Value(0.9f)
				[
					PanelContainer
				]
		];
}

void SMergeGraphView::NextDiff()
{
	check(DiffResultList.Pin().IsValid() && DiffResultsListData);
	DiffWidgetUtils::SelectNextRow( *DiffResultList.Pin(), *DiffResultsListData );
}

void SMergeGraphView::PrevDiff()
{
	check(DiffResultList.Pin().IsValid() && DiffResultsListData);
	DiffWidgetUtils::SelectPrevRow( *DiffResultList.Pin(), *DiffResultsListData );
}

bool SMergeGraphView::HasNextDifference() const
{
	if (HasNoDifferences())
	{
		return false;
	}

	check(DiffResultList.Pin().IsValid() && DiffResultsListData);
	return DiffWidgetUtils::HasNextDifference(*DiffResultList.Pin(), *DiffResultsListData);
}

bool SMergeGraphView::HasPrevDifference() const
{
	if (HasNoDifferences())
	{
		return false;
	}

	check(DiffResultList.Pin().IsValid() && DiffResultsListData);
	return DiffWidgetUtils::HasPrevDifference(*DiffResultList.Pin(), *DiffResultsListData);
}

void SMergeGraphView::HighlightNextConflict()
{
	if( CurrentMergeConflict + 1 < MergeConflicts.Num())
	{
		++CurrentMergeConflict;
	}
	HighlightConflict(*MergeConflicts[CurrentMergeConflict]);
}

void SMergeGraphView::HighlightPrevConflict()
{
	if (CurrentMergeConflict - 1 >= 0 )
	{
		--CurrentMergeConflict;
	}
	HighlightConflict(*MergeConflicts[CurrentMergeConflict]);
}

bool SMergeGraphView::HasNextConflict() const 
{
	// return true if we have one conflict so that users can reselect the conflict if they desire. If we 
	// return false when we already have selected this one and only conflict then there will be no way
	// to reselect it if the user wants to.
	return MergeConflicts.Num() == 1 || CurrentMergeConflict < MergeConflicts.Num();
}

bool SMergeGraphView::HasPrevConflict() const
{
	// note in HasNextConflict applies here as well.
	return MergeConflicts.Num() == 1 || CurrentMergeConflict > 0;
}

void SMergeGraphView::HighlightConflict(const FGraphMergeConflict& Conflict)
{
	for( const auto& MergeGraphEntry : DifferencesFromBase )
	{
		if( MergeGraphEntry->GraphName == Conflict.GraphName )
		{
			OnGraphListSelectionChanged(MergeGraphEntry, ESelectInfo::Direct);
			break;
		}
	}

	// highlight the change made to the remote graph:
	if( UEdGraphPin* Pin1 = Conflict.FirstResult.Pin2 )
	{
		// then look for the diff panel and focus on the change:
		GetDiffPanelForNode(*Pin1->GetOwningNode(), DiffPanels).FocusDiff(*Pin1);
	}
	else if( UEdGraphNode* Node1 = Conflict.FirstResult.Node2 )
	{
		GetDiffPanelForNode(*Node1, DiffPanels).FocusDiff(*Node1);
	}
	
	// and the change made to the local graph:
	if (UEdGraphPin* Pin1 = Conflict.SecondResult.Pin2)
	{
		GetDiffPanelForNode(*Pin1->GetOwningNode(), DiffPanels).FocusDiff(*Pin1);
	}
	else if (UEdGraphNode* Node1 = Conflict.SecondResult.Node2)
	{
		GetDiffPanelForNode(*Node1, DiffPanels).FocusDiff(*Node1);
	}
}

bool SMergeGraphView::HasNoDifferences() const
{
	auto DiffResultListPtr = DiffResultList.Pin();
	return !DiffResultListPtr.IsValid() || DiffResultListPtr->GetNumItemsBeingObserved() == 0;
}

void SMergeGraphView::OnGraphListSelectionChanged(TSharedPtr<FMergeGraphRowEntry> Item, ESelectInfo::Type SelectionType)
{
	if (( SelectionType != ESelectInfo::OnMouseClick &&
		  SelectionType != ESelectInfo::Direct) ||
		!Item.IsValid())
	{
		return;
	}

	FName GraphName = Item->GraphName;

	UEdGraph* GraphRemote = FindGraphByName(*GetRemotePanel().Blueprint, GraphName);
	UEdGraph* GraphBase = FindGraphByName(*GetBasePanel().Blueprint, GraphName);
	UEdGraph* GraphLocal = FindGraphByName(*GetLocalPanel().Blueprint, GraphName);

	GetBasePanel().GeneratePanel(GraphBase, NULL);
	GetRemotePanel().GeneratePanel(GraphRemote, GraphBase);
	GetLocalPanel().GeneratePanel(GraphLocal, GraphBase);

	LockViews(DiffPanels, bViewsAreLocked);

	DiffResultsListData = &(Item->Differences);
	DiffResultsWidget->SetContent(
		SAssignNew(DiffResultList, SListView< TSharedPtr<FDiffSingleResult> >)
		.ItemHeight(24)
		.ListItemsSource(&Item->Differences)
		.OnGenerateRow(SListView< TSharedPtr<FDiffSingleResult> >::FOnGenerateRow::CreateStatic(
			[](TSharedPtr<FDiffSingleResult> ParamItem, const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow>
			{
				FDiffResultItem WidgetGenerator(*ParamItem);
				return SNew(STableRow< TSharedPtr<FDiffSingleResult> >, OwnerTable)
					.Content()
					[
						WidgetGenerator.GenerateWidget()
					];
			}
		))
		.SelectionMode(ESelectionMode::Single)
		.OnSelectionChanged(this, &SMergeGraphView::OnDiffListSelectionChanged)
	);
}

void SMergeGraphView::OnDiffListSelectionChanged(TSharedPtr<FDiffSingleResult> Item, ESelectInfo::Type SelectionType)
{
	if (!Item.IsValid())
	{
		return;
	}

	// Clear graph selection:
	for (auto& Panel : DiffPanels)
	{
		auto GraphEdtiorPtr = Panel.GraphEditor.Pin();
		if (GraphEdtiorPtr.IsValid())
		{
			GraphEdtiorPtr->ClearSelectionSet();
		}
	}

	//focus the graph onto the diff that was clicked on
	if (Item->Pin1)
	{
		GetDiffPanelForNode(*Item->Pin1->GetOwningNode(), DiffPanels).FocusDiff(*Item->Pin1);

		if (Item->Pin2)
		{
			GetDiffPanelForNode(*Item->Pin2->GetOwningNode(), DiffPanels).FocusDiff(*Item->Pin2);
		}
	}
	else if (Item->Node1)
	{
		GetDiffPanelForNode(*Item->Node1, DiffPanels).FocusDiff(*Item->Node1);

		if (Item->Node2)
		{
			GetDiffPanelForNode(*Item->Node2, DiffPanels).FocusDiff(*Item->Node2);
		}
	}
}

FReply SMergeGraphView::OnToggleLockView()
{
	bViewsAreLocked = !bViewsAreLocked;

	LockViews(DiffPanels, bViewsAreLocked);
	return FReply::Handled();
}

const FSlateBrush* SMergeGraphView::GetLockViewImage() const
{
	return bViewsAreLocked ? FEditorStyle::GetBrush("GenericLock") : FEditorStyle::GetBrush("GenericUnlock");
}

#undef LOCTEXT_NAMESPACE