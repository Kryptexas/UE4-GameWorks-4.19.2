// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerPrivatePCH.h"
#include "Sequencer.h"
#include "SListPanel.h"

static FName TrackAreaName = "TrackArea";

/** Structure used to cache physical geometry for a particular node */
struct FCachedGeometry
{
	FCachedGeometry(FDisplayNodeRef InNode, float InPhysicalTop, float InPhysicalHeight)
		: Node(MoveTemp(InNode)), PhysicalTop(InPhysicalTop), PhysicalHeight(InPhysicalHeight)
	{}

	FDisplayNodeRef Node;
	float PhysicalTop, PhysicalHeight;
};

/** Construct function for this widget */
void SSequencerTreeViewRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView, const FDisplayNodeRef& InNode)
{
	Node = InNode;
	OnGenerateWidgetForColumn = InArgs._OnGenerateWidgetForColumn;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), OwnerTableView);
}

TSharedRef<SWidget> SSequencerTreeViewRow::GenerateWidgetForColumn(const FName& ColumnId)
{
	auto PinnedNode = Node.Pin();
	if (PinnedNode.IsValid())
	{
		return OnGenerateWidgetForColumn.Execute(PinnedNode.ToSharedRef(), ColumnId, SharedThis(this));
	}

	return SNullWidget::NullWidget;
}

TSharedPtr<FSequencerDisplayNode> SSequencerTreeViewRow::GetDisplayNode() const
{
	return Node.Pin();
}

const FGeometry& SSequencerTreeViewRow::GetCachedGeometry() const
{
	return RowGeometry;
}

void SSequencerTreeViewRow::AddTrackAreaReference(const TSharedPtr<SSequencerTrackLane>& Lane)
{
	TrackLaneReference = Lane;
}

void SSequencerTreeViewRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	RowGeometry = AllottedGeometry;
}

void SSequencerTreeView::Construct(const FArguments& InArgs, const TSharedRef<FSequencerNodeTree>& InNodeTree, const TSharedRef<SSequencerTrackArea>& InTrackArea)
{
	SequencerNodeTree = InNodeTree;
	TrackArea = InTrackArea;

	// We 'leak' these delegates (they'll get cleaned up automatically when the invocation list changes)
	// It's not safe to attempt their removal in ~SSequencerTreeView because SequencerNodeTree->GetSequencer() may not be valid
	FSequencer& Sequencer = InNodeTree->GetSequencer();
	Sequencer.GetSettings()->GetOnShowCurveEditorChanged().AddSP(this, &SSequencerTreeView::UpdateTrackArea);
	Sequencer.GetSelection().GetOnOutlinerNodeSelectionChanged().AddSP(this, &SSequencerTreeView::OnSequencerSelectionChangedExternally);

	HeaderRow = SNew(SHeaderRow).Visibility(EVisibility::Collapsed);

	SetupColumns(InArgs);

	STreeView::Construct
	(
		STreeView::FArguments()
		.TreeItemsSource(&RootNodes)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SSequencerTreeView::OnGenerateRow)
		.OnGetChildren(this, &SSequencerTreeView::OnGetChildren)
		.HeaderRow(HeaderRow)
		.ExternalScrollbar(InArgs._ExternalScrollbar)
		.OnExpansionChanged(this, &SSequencerTreeView::OnExpansionChanged)
		.AllowOverscroll(EAllowOverscroll::No)
	);
}

void SSequencerTreeView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	STreeView::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	CachedTreeGeometry = AllottedGeometry;
	UpdateCachedVerticalGeometry();
}

void SSequencerTreeView::UpdateCachedVerticalGeometry()
{
	FChildren* Children = ItemsPanel->GetChildren();

	CachedGeometry.Empty(Children->Num());
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		TSharedRef<SSequencerTreeViewRow> TreeViewRow = StaticCastSharedRef<SSequencerTreeViewRow>(Children->GetChildAt(Index));
		TSharedPtr<FSequencerDisplayNode> Node = TreeViewRow->GetDisplayNode();
		if (Node.IsValid())
		{
			CachedGeometry.Emplace(Node.ToSharedRef(), TreeViewRow->GetCachedGeometry().Position.Y, TreeViewRow->GetCachedGeometry().Size.Y);
		}
	}
}

float SSequencerTreeView::PhysicalToVirtual(float InPhysical) const
{
	for (auto& Geometry : CachedGeometry)
	{
		if (InPhysical >= Geometry.PhysicalTop && InPhysical <= Geometry.PhysicalTop + Geometry.PhysicalHeight)
		{
			const float FractionalHeight = (InPhysical - Geometry.PhysicalTop) / Geometry.PhysicalHeight;
			return Geometry.Node->VirtualTop + (Geometry.Node->VirtualBottom - Geometry.Node->VirtualTop) * FractionalHeight;
		}
	}

	if (CachedGeometry.Num())
	{
		auto Last = CachedGeometry.Last();
		return Last.Node->VirtualTop + (InPhysical - Last.PhysicalTop);
	}

	return InPhysical;
}

float SSequencerTreeView::VirtualToPhysical(float InVirtual) const
{
	for (auto& Geometry : CachedGeometry)
	{
		if (InVirtual >= Geometry.Node->VirtualTop && InVirtual <= Geometry.Node->VirtualBottom)
		{
			const float FractionalHeight = (InVirtual - Geometry.Node->VirtualTop) / (Geometry.Node->VirtualBottom - Geometry.Node->VirtualTop);
			return Geometry.PhysicalTop + Geometry.PhysicalHeight * FractionalHeight;
		}
	}
	
	if (CachedGeometry.Num())
	{
		auto Last = CachedGeometry.Last();
		return Last.PhysicalTop + (InVirtual - Last.Node->VirtualTop);
	}

	return InVirtual;
}

void SSequencerTreeView::SetupColumns(const FArguments& InArgs)
{
	FSequencer& Sequencer = SequencerNodeTree->GetSequencer();

	// Define a column for the Outliner
	auto GenerateOutliner = [=](const FDisplayNodeRef& InNode, const TSharedRef<SSequencerTreeViewRow>& InRow)
	{
		return InNode->GenerateContainerWidgetForOutliner(InRow);
	};

	Columns.Add("Outliner", FSequencerTreeViewColumn(GenerateOutliner, 1.f));

	// Now populate the header row with the columns
	for (auto& Pair : Columns)
	{
		if (Pair.Key != TrackAreaName || !Sequencer.GetSettings()->GetShowCurveEditor())
		{
			HeaderRow->AddColumn(
				SHeaderRow::Column(Pair.Key)
				.FillWidth(Pair.Value.Width)
			);
		}
	}
}

void SSequencerTreeView::UpdateTrackArea()
{
	FSequencer& Sequencer = SequencerNodeTree->GetSequencer();

	// Add or remove the column
	if (Sequencer.GetSettings()->GetShowCurveEditor())
	{
		HeaderRow->RemoveColumn(TrackAreaName);
	}
	else if (const auto* Column = Columns.Find(TrackAreaName))
	{
		HeaderRow->AddColumn(
			SHeaderRow::Column(TrackAreaName)
			.FillWidth(Column->Width)
		);
	}
}

void SSequencerTreeView::OnSequencerSelectionChangedExternally()
{
	Private_ClearSelection();

	FSequencer& Sequencer = SequencerNodeTree->GetSequencer();
	for (auto& Node : Sequencer.GetSelection().GetSelectedOutlinerNodes())
	{
		Private_SetItemSelection(Node, true, false);
	}

	Private_SignalSelectionChanged(ESelectInfo::Direct);
}

void SSequencerTreeView::Private_SignalSelectionChanged(ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		FSequencer& Sequencer = SequencerNodeTree->GetSequencer();
		FSequencerSelection& Selection = Sequencer.GetSelection();
		Selection.EmptySelectedOutlinerNodes();
		for (auto& Item : GetSelectedItems())
		{
			Selection.AddToSelection(Item);
		}
	}

	STreeView::Private_SignalSelectionChanged(SelectInfo);
}

void SSequencerTreeView::Refresh()
{
	RootNodes.Reset(SequencerNodeTree->GetRootNodes().Num());

	for (const auto& RootNode : SequencerNodeTree->GetRootNodes())
	{
		if (RootNode->IsExpanded())
		{
			SetItemExpansion(RootNode, true);
		}

		if (!RootNode->IsHidden())
		{
			RootNodes.Add(RootNode);
		}
	}

	RequestTreeRefresh();
}

void SSequencerTreeView::ScrollByDelta(float DeltaInSlateUnits)
{
	ScrollBy( CachedTreeGeometry, DeltaInSlateUnits, EAllowOverscroll::No );
}

template<typename T>
bool ShouldExpand(const T& InContainer, ETreeRecursion Recursion)
{
	bool bAllExpanded = true;
	for (auto& Item : InContainer)
	{
		bAllExpanded &= Item->IsExpanded();
		if (Recursion == ETreeRecursion::Recursive)
		{
			Item->TraverseVisible_ParentFirst([&](FSequencerDisplayNode& InNode){
				bAllExpanded &= InNode.IsExpanded();
				return true;
			});
		}
	}
	return !bAllExpanded;
}

void SSequencerTreeView::ToggleSelectedNodeExpansion(ETreeRecursion Recursion)
{
	FSequencer& Sequencer = SequencerNodeTree->GetSequencer();

	const TSet< FDisplayNodeRef >& SelectedNodes = Sequencer.GetSelection().GetSelectedOutlinerNodes();

	if (SelectedNodes.Num() != 0)
	{
		const bool bExpand = ShouldExpand(SelectedNodes, Recursion);
		for (auto& Item : SelectedNodes)
		{
			ExpandCollapseNode(Item, bExpand, Recursion);
		}
	}
	else
	{
		const bool bExpand = ShouldExpand(SequencerNodeTree->GetRootNodes(), Recursion);
		for (auto& Item : SequencerNodeTree->GetRootNodes())
		{
			ExpandCollapseNode(Item, bExpand, Recursion);
		}
	}
}

void SSequencerTreeView::ExpandCollapseNode(const FDisplayNodeRef& InNode, bool bExpansionState, ETreeRecursion Recursion)
{
	SetItemExpansion(InNode, bExpansionState);

	if (Recursion == ETreeRecursion::Recursive)
	{
		for (auto& Node : InNode->GetChildNodes())
		{
			ExpandCollapseNode(Node, bExpansionState, ETreeRecursion::Recursive);
		}
	}
}

void SSequencerTreeView::OnExpansionChanged(FDisplayNodeRef InItem, bool bIsExpanded)
{
	InItem->SetExpansionState(bIsExpanded);
	
	// Expand any children that are also expanded
	for (auto& Child : InItem->GetChildNodes())
	{
		if (Child->IsExpanded())
		{
			SetItemExpansion(Child, true);
		}
	}

	InItem->UpdateCachedShotFilteredVisibility();
}

void SSequencerTreeView::OnGetChildren(FDisplayNodeRef InParent, TArray<FDisplayNodeRef>& OutChildren) const
{
	for (const auto& Node : InParent->GetChildNodes())
	{
		if (!Node->IsHidden())
		{
			OutChildren.Add(Node);
		}
	}
}

TSharedRef<ITableRow> SSequencerTreeView::OnGenerateRow(FDisplayNodeRef InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SSequencerTreeViewRow> Row =
		SNew(SSequencerTreeViewRow, OwnerTable, InDisplayNode)
		.OnGenerateWidgetForColumn(this, &SSequencerTreeView::GenerateWidgetForColumn);

	// Ensure the track area is kept up to date with the virtualized scroll of the tree view
	TSharedPtr<FSequencerDisplayNode> SectionAuthority = InDisplayNode->GetSectionAreaAuthority();
	if (SectionAuthority.IsValid())
	{
		TSharedPtr<SSequencerTrackLane> TrackLane = TrackArea->FindTrackSlot(SectionAuthority.ToSharedRef());

		if (!TrackLane.IsValid())
		{
			// Add a track slot for the row
			TAttribute<TRange<float>> ViewRange = FAnimatedRange::WrapAttribute( TAttribute<FAnimatedRange>::Create(TAttribute<FAnimatedRange>::FGetter::CreateSP(&SequencerNodeTree->GetSequencer(), &FSequencer::GetViewRange)) );	

			TrackLane = SNew(SSequencerTrackLane, SectionAuthority.ToSharedRef(), SharedThis(this))
			[
				SectionAuthority->GenerateWidgetForSectionArea(ViewRange)
			];

			TrackArea->AddTrackSlot(SectionAuthority.ToSharedRef(), TrackLane);
		}

		if (ensure(TrackLane.IsValid()))
		{
			Row->AddTrackAreaReference(TrackLane);
		}
	}

	return Row;
}

TSharedRef<SWidget> SSequencerTreeView::GenerateWidgetForColumn(const FDisplayNodeRef& InNode, const FName& ColumnId, const TSharedRef<SSequencerTreeViewRow>& Row) const
{
	const auto* Definition = Columns.Find(ColumnId);

	if (ensureMsgf(Definition, TEXT("Invalid column name specified")))
	{
		return Definition->Generator(InNode, Row);
	}

	return SNullWidget::NullWidget;
}