// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"
#include "DiffResults.h"
#include "GraphDiffControl.h"
#include "SDockTab.h"
#include "SMergeGraphView.h"
#include "SKismetInspector.h"

#define LOCTEXT_NAMESPACE "SMergeGraphView"

const FName MergeMyBluerpintTabId = FName(TEXT("MergeMyBluerpintTab"));
const FName MergeGraphTabId = FName(TEXT("MergeGraphTab"));

struct FGraphMergeConflict
{
	FGraphMergeConflict(FName InGraphName, const FDiffSingleResult& InRemoteDifference, const FDiffSingleResult& InLocalDifference )
		: GraphName(InGraphName)
		, RemoteDifference(InRemoteDifference)
		, LocalDifference(InLocalDifference)
	{
	}

	FName GraphName;

	// These are the diff results that we have judged to be conflicting:
	FDiffSingleResult RemoteDifference;
	FDiffSingleResult LocalDifference;
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
	TArray< TSharedPtr<FDiffSingleResult> > RemoteDifferences;
	TArray< TSharedPtr<FDiffSingleResult> > LocalDifferences;

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

	UEdGraph* Ret = nullptr;
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
			FGraphDiffControl::DiffGraphs(GraphOld ? *GraphOld : nullptr, GraphNew, Results);
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
							else if( RemoteDifference.Pin1 != nullptr && (RemoteDifference.Pin1 == LocalDifference.Pin1 ) )
							{
								// it's possible the users made the same change to the same pin, but given the wide
								// variety of changes that can be made to a pin it is difficult to identify the change 
								// as identical, for now I'm just flagging all changes to the same pin as a conflict:
								ConflictingDifference = &LocalDifference;
								break;
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
						// else: no conflict, we want to be able to automatically apply this remote change
						// to the local revision:
					}
				}

				bExistsInRemote = RemoteGraph != nullptr;
				bExistsInBase = BaseGraph != nullptr;
				bExistsInLocal = LocalGraph != nullptr;
			}

			FMergeGraphRowEntry NewEntry = {
				GraphName,
				ConcreteToShared(RemoteDifferences),
				ConcreteToShared(LocalDifferences),
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
	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	TabManager = FGlobalTabmanager::Get()->NewTabManager(MajorTab);

	TabManager->RegisterTabSpawner(MergeGraphTabId,
		FOnSpawnTab::CreateRaw(this, &SMergeGraphView::CreateGraphDiffViews))
		.SetDisplayName(LOCTEXT("MergeGraphsTabTitle", "Graphs"))
		.SetTooltipText(LOCTEXT("MergeGraphsTooltipText", "Differences in the various graphs present in the blueprint"));

	TabManager->RegisterTabSpawner(MergeMyBluerpintTabId,
		FOnSpawnTab::CreateRaw(this, &SMergeGraphView::CreateMyBlueprintsViews))
		.SetDisplayName(LOCTEXT("MergeMyBlueprintTabTitle", "My Blueprint"))
		.SetTooltipText(LOCTEXT("MergeMyBlueprintTooltipText", "Differences in the 'My Blueprints' attributes of the blueprint"));

	CurrentMergeConflict = INDEX_NONE;

	Data = InData;
	bViewsAreLocked = true;
	RemoteDiffResultsListData = nullptr;
	LocalDiffResultsListData = nullptr;

	TArray<FBlueprintRevPair> BlueprintsForDisplay;
	// EMergeParticipant::Remote
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintRemote, InData.RevisionRemote));
	// EMergeParticipant::Base
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintBase, InData.RevisionBase));
	// EMergeParticipant::Local
	BlueprintsForDisplay.Add(FBlueprintRevPair(InData.BlueprintLocal, FRevisionInfo()));
	
	const TSharedRef<FTabManager::FLayout> DefaultLayout = FTabManager::NewLayout("BlueprintMerge_Layout_v1")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->Split
		(
			FTabManager::NewStack()
			->AddTab(MergeMyBluerpintTabId, ETabState::OpenedTab)
			->AddTab(MergeGraphTabId, ETabState::OpenedTab)
		)
	);

	for (int32 i = 0; i < EMergeParticipant::Max_None; ++i)
	{
		DiffPanels.Add(FDiffPanel());
		FDiffPanel& NewPanel = DiffPanels[i];
		NewPanel.Blueprint = BlueprintsForDisplay[i].Blueprint;
		NewPanel.RevisionInfo = BlueprintsForDisplay[i].RevData;
		NewPanel.bShowAssetName = false;
	}

	auto GraphPanelContainer = TabManager->RestoreFrom(DefaultLayout, TSharedPtr<SWindow>()).ToSharedRef();

	for (auto& Panel : DiffPanels )
	{
		Panel.InitializeDiffPanel();
	}

	auto DetailsPanelContainer = SNew(SSplitter);
	for( auto& Panel : DiffPanels )
	{
		DetailsPanelContainer->AddSlot()
		[
			Panel.DetailsView.ToSharedRef()
		];
	}

	DifferencesFromBase = GenerateDiffListItems(BlueprintsForDisplay[EMergeParticipant::Remote]
												, BlueprintsForDisplay[EMergeParticipant::Base]
												, BlueprintsForDisplay[EMergeParticipant::Local]
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

	const auto DifferencesListGenerator = []( FText Title, TSharedPtr<SBox>& OutResultsContainer )
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(Title)
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				// this creates a line between the header and the list itself:
				SNew(SBorder)
				.Padding(FEditorStyle::GetMargin(TEXT("Menu.Separator.Padding")))
				.BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Separator")))
			]
		+ SVerticalBox::Slot()
			[
				SAssignNew(OutResultsContainer, SBox)
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
					.Value(.35f)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							DifferencesListGenerator(LOCTEXT("RemoteChangesLabel", "Remote Changes"), RemoteDiffResultsWidget)
						]
					]
					+ SSplitter::Slot()
					.Value(.35f)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							DifferencesListGenerator(LOCTEXT("LocalChangesLabel", "Local Changes"), LocalDiffResultsWidget)
						]
					]
				]
			]
			+ SSplitter::Slot()
				.Value(0.9f)
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot()
					.Value(.8f)
					[
						GraphPanelContainer
					]
					+ SSplitter::Slot()
					.Value(.2f)
					[
						DetailsPanelContainer
					]
				]
		];
}

void SMergeGraphView::NextDiff()
{
	check(	RemoteDiffResultList.Pin().IsValid() && RemoteDiffResultsListData || 
			LocalDiffResultList.Pin().IsValid() && LocalDiffResultsListData );
	if( RemoteDiffResultsListData && DiffWidgetUtils::HasNextDifference( *RemoteDiffResultList.Pin(), *RemoteDiffResultsListData) )
	{
		DiffWidgetUtils::SelectNextRow( *RemoteDiffResultList.Pin(), *RemoteDiffResultsListData );
	}
	else
	{
		DiffWidgetUtils::SelectNextRow(*LocalDiffResultList.Pin(), *LocalDiffResultsListData);
	}
}

void SMergeGraphView::PrevDiff()
{
	check(	RemoteDiffResultList.Pin().IsValid() && RemoteDiffResultsListData ||
			LocalDiffResultList.Pin().IsValid() && LocalDiffResultsListData);
	if (LocalDiffResultsListData && DiffWidgetUtils::HasPrevDifference(*LocalDiffResultList.Pin(), *LocalDiffResultsListData))
	{
		DiffWidgetUtils::SelectPrevRow(*LocalDiffResultList.Pin(), *LocalDiffResultsListData);
	}
	else
	{
		DiffWidgetUtils::SelectPrevRow(*RemoteDiffResultList.Pin(), *RemoteDiffResultsListData);
	}
}

bool SMergeGraphView::HasNextDifference() const
{
	if (HasNoDifferences())
	{
		return false;
	}

	check(	RemoteDiffResultList.Pin().IsValid() && RemoteDiffResultsListData ||
			LocalDiffResultList.Pin().IsValid() && LocalDiffResultsListData);
	return	(RemoteDiffResultsListData && DiffWidgetUtils::HasNextDifference(*RemoteDiffResultList.Pin(), *RemoteDiffResultsListData) ) ||
			(LocalDiffResultsListData && DiffWidgetUtils::HasNextDifference(*LocalDiffResultList.Pin(), *LocalDiffResultsListData) ) ;
}

bool SMergeGraphView::HasPrevDifference() const
{
	if (HasNoDifferences())
	{
		return false;
	}

	check(	RemoteDiffResultList.Pin().IsValid() && RemoteDiffResultsListData ||
			LocalDiffResultList.Pin().IsValid() && LocalDiffResultsListData);
	return DiffWidgetUtils::HasPrevDifference(*RemoteDiffResultList.Pin(), *RemoteDiffResultsListData) 
		|| DiffWidgetUtils::HasPrevDifference(*LocalDiffResultList.Pin(), *LocalDiffResultsListData);
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
	return  MergeConflicts.Num() != 0 && (MergeConflicts.Num() == 1 || CurrentMergeConflict + 1 < MergeConflicts.Num());
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
	if( UEdGraphPin* Pin = Conflict.RemoteDifference.Pin2 )
	{
		// then look for the diff panel and focus on the change:
		GetDiffPanelForNode(*Pin->GetOwningNode(), DiffPanels).FocusDiff(*Pin);
	}
	else if( UEdGraphNode* Node = Conflict.RemoteDifference.Node2 )
	{
		GetDiffPanelForNode(*Node, DiffPanels).FocusDiff(*Node);
	}
	
	// and the change made to the local graph:
	if (UEdGraphPin* Pin = Conflict.LocalDifference.Pin2)
	{
		GetDiffPanelForNode(*Pin->GetOwningNode(), DiffPanels).FocusDiff(*Pin);
	}
	else if (UEdGraphNode* Node = Conflict.LocalDifference.Node2)
	{
		GetDiffPanelForNode(*Node, DiffPanels).FocusDiff(*Node);
	}
}

bool SMergeGraphView::HasNoDifferences() const
{
	auto RemoteDiffResultListPtr = RemoteDiffResultList.Pin();
	auto LocalDiffResultListPtr = LocalDiffResultList.Pin();
	return	(!RemoteDiffResultListPtr.IsValid() || RemoteDiffResultListPtr->GetNumItemsBeingObserved() == 0) &&
			(!LocalDiffResultListPtr.IsValid() || LocalDiffResultListPtr->GetNumItemsBeingObserved() == 0);
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

	GetBasePanel().GeneratePanel(GraphBase, nullptr );
	GetRemotePanel().GeneratePanel(GraphRemote, GraphBase );
	GetLocalPanel().GeneratePanel(GraphLocal, GraphBase );

	LockViews(DiffPanels, bViewsAreLocked);

	RemoteDiffResultsListData = &(Item->RemoteDifferences);
	LocalDiffResultsListData = &(Item->LocalDifferences);

	const auto SetupDiffList = [this]( TArray< TSharedPtr<FDiffSingleResult> > *Differences, TWeakPtr < SListView< TSharedPtr< struct FDiffSingleResult> > >& OutWidget)
	{
		return SAssignNew(OutWidget, SListView< TSharedPtr<FDiffSingleResult> >)
			.ItemHeight(24)
			.ListItemsSource(Differences)
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
			.OnSelectionChanged(this, &SMergeGraphView::OnDiffListSelectionChanged);
	};

	RemoteDiffResultsWidget->SetContent(
		SetupDiffList( &Item->RemoteDifferences, RemoteDiffResultList)
	);

	LocalDiffResultsWidget->SetContent(
		SetupDiffList(&Item->LocalDifferences, LocalDiffResultList)
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

TSharedRef<SDockTab> SMergeGraphView::CreateGraphDiffViews(const FSpawnTabArgs& Args)
{
	auto PanelContainer = SNew(SSplitter);
	for (auto& Panel : DiffPanels)
	{
		PanelContainer->AddSlot()
		[
			SAssignNew(Panel.GraphEditorBorder, SBox)
			.VAlign(VAlign_Fill)
			[
				SBlueprintDiff::DefaultEmptyPanel()
			]
		];
	}

	return SNew(SDockTab)
	[
		PanelContainer
	];
}

TSharedRef<SDockTab> SMergeGraphView::CreateMyBlueprintsViews(const FSpawnTabArgs& Args)
{
	auto PanelContainer = SNew(SSplitter);
	for (auto& Panel : DiffPanels)
	{
		PanelContainer->AddSlot()
		[
			Panel.GenerateMyBlueprintPanel()
		];
	}

	return SNew(SDockTab)
	[
		PanelContainer
	];
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