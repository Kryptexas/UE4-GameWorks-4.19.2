// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "DockingPrivate.h"


const FVector2D FDockingConstants::MaxMinorTabSize(150.f, 50.0f);
const FVector2D FDockingConstants::MaxMajorTabSize(210.f, 50.f);

const FVector2D FDockingConstants::GetMaxTabSizeFor( ETabRole::Type TabRole )
{
	return (TabRole == ETabRole::MajorTab)
		? MaxMajorTabSize
		: MaxMinorTabSize;
}


void SDockingTabWell::Construct( const FArguments& InArgs )
{
	ForegroundTabIndex = INDEX_NONE;
	TabBeingDraggedPtr = TWeakPtr<SDockTab>(NULL);
	ChildBeingDraggedOffset = 0.0f;
	TabGrabOffsetFraction = FVector2D::ZeroVector;
		
	// We need a valid parent here. TabPanels must exist in a SDockingNode
	check( InArgs._ParentStackNode.Get().IsValid() );
	ParentTabStackPtr = InArgs._ParentStackNode.Get();
}

const TArray< TSharedRef<SDockTab> >& SDockingTabWell::GetTabs() const
{
	return Tabs;
}

int32 SDockingTabWell::GetNumTabs() const
{
	return Tabs.Num();
}

void SDockingTabWell::AddTab( const TSharedRef<SDockTab>& InTab, int32 AtIndex )
{
	// Add the tab and implicitly activate it.
	if (AtIndex == INDEX_NONE)
	{
		this->Tabs.Add( InTab );
		BringTabToFront( Tabs.Num()-1 );
	}
	else
	{
		AtIndex = FMath::Clamp( AtIndex, 0, Tabs.Num() );
		this->Tabs.Insert( InTab, AtIndex );
		BringTabToFront( AtIndex );
	}

	InTab->SetParent(SharedThis(this));

	const TSharedPtr<SDockingTabStack> ParentTabStack = ParentTabStackPtr.Pin();
	if (ParentTabStack.IsValid() && ParentTabStack->GetDockArea().IsValid())
	{
		ParentTabStack->GetDockArea()->GetTabManager()->GetPrivateApi().OnTabOpening( InTab );
	}
}


void SDockingTabWell::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	// The specialized TabWell is dedicated to arranging tabs.
	// Tabs have uniform sizing (all tabs the same size).
	// TabWell also ignores widget visibilit, as it is not really
	// relevant.


	// The tab that is being dragged by the user, if any.
	TSharedPtr<SDockTab> TabBeingDragged = TabBeingDraggedPtr.Pin();
		
	const int32 NumChildren = Tabs.Num();

	// Tabs have a uniform size.
	const FVector2D ChildSize = ComputeChildSize(AllottedGeometry);


	const float DraggedChildCenter = ChildBeingDraggedOffset + ChildSize.X / 2;

	// Arrange all the tabs left to right.
	float XOffset = 0;
	for( int32 TabIndex=0; TabIndex < NumChildren; ++TabIndex )
	{
		const TSharedRef<SDockTab> CurTab = Tabs[TabIndex];
		const float ChildWidthWithOverlap = ChildSize.X - CurTab->GetOverlapWidth();

		// Is this spot reserved from the tab that is being dragged?
		if ( TabBeingDragged.IsValid() && XOffset <= DraggedChildCenter && DraggedChildCenter < (XOffset + ChildWidthWithOverlap) )
		{
			// if so, leave some room to signify that this is where the dragged tab would end up
			XOffset += ChildWidthWithOverlap;
		}

		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild(CurTab, FVector2D(XOffset, 0), ChildSize) );

		XOffset += ChildWidthWithOverlap;
	}
		
	// Arrange the tab currently being dragged by the user, if any
	if ( TabBeingDragged.IsValid() )
	{
		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( TabBeingDragged.ToSharedRef(), FVector2D(ChildBeingDraggedOffset,0), ChildSize) );
	}
}
	

int32 SDockingTabWell::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// When we are dragging a tab, it must be painted on top of the other tabs, so we cannot
	// just reuse the Panel's default OnPaint.


	// The TabWell has no visualization of its own; it just visualizes its child tabs.
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren(AllottedGeometry, ArrangedChildren);

	// Because we paint multiple children, we must track the maximum layer id that they produced in case one of our parents
	// wants to an overlay for all of its contents.
	int32 MaxLayerId = LayerId;

	TSharedPtr<SDockTab> ForegroundTab = GetForegroundTab();
	FArrangedWidget* ForegroundTabGeometry = NULL;
	
	// Draw all inactive tabs first
	for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
		if (CurWidget.Widget == ForegroundTab)
		{
			ForegroundTabGeometry = &CurWidget;
		}
		else
		{
			FSlateRect ChildClipRect = MyClippingRect.IntersectionWith( CurWidget.Geometry.GetClippingRect() );
			const int32 CurWidgetsMaxLayerId = CurWidget.Widget->OnPaint( CurWidget.Geometry, ChildClipRect, OutDrawElements, MaxLayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
			MaxLayerId = FMath::Max( MaxLayerId, CurWidgetsMaxLayerId );
		}
	}

	// Draw active tab in front
	if (ForegroundTab != TSharedPtr<SDockTab>())
	{
		FSlateRect ChildClipRect = MyClippingRect.IntersectionWith( ForegroundTabGeometry->Geometry.GetClippingRect() );
		const int32 CurWidgetsMaxLayerId = ForegroundTabGeometry->Widget->OnPaint( ForegroundTabGeometry->Geometry, ChildClipRect, OutDrawElements, MaxLayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
		MaxLayerId = FMath::Max( MaxLayerId, CurWidgetsMaxLayerId );
	}

	return MaxLayerId;
}


FVector2D SDockingTabWell::ComputeDesiredSize() const
{
	FVector2D DesiredSizeResult(0,0);

	for ( int32 TabIndex=0; TabIndex < Tabs.Num(); ++TabIndex )
	{
		// Currently not respecting Visibility because tabs cannot be invisible.
		const TSharedRef<SDockTab>& SomeTab = Tabs[TabIndex];
		const FVector2D SomeTabDesiredSize = SomeTab->GetDesiredSize();
		DesiredSizeResult.X += SomeTabDesiredSize.X;
		DesiredSizeResult.Y = FMath::Max(SomeTabDesiredSize.Y, DesiredSizeResult.Y);
	}

	TSharedPtr<SDockTab> TabBeingDragged = TabBeingDraggedPtr.Pin();
	if ( TabBeingDragged.IsValid() )
	{
		const FVector2D SomeTabDesiredSize = TabBeingDragged->GetDesiredSize();
		DesiredSizeResult.X += SomeTabDesiredSize.X;
		DesiredSizeResult.Y = FMath::Max(SomeTabDesiredSize.Y, DesiredSizeResult.Y);
	}

	return DesiredSizeResult;
}


FChildren* SDockingTabWell::GetChildren()
{
	return &Tabs;
}


FVector2D SDockingTabWell::ComputeChildSize( const FGeometry& AllottedGeometry ) const
{
	const int32 NumChildren = Tabs.Num();

	/** Assume all tabs overlap the same amount. */
	const float OverlapWidth = (NumChildren > 0)
		? Tabs[0]->GetOverlapWidth()
		: 0.0f;

	// All children shall be the same size: evenly divide the alloted area.
	// If we are dragging a tab, don't forget to take it into account when dividing.
	const FVector2D ChildSize = TabBeingDraggedPtr.IsValid()
		? FVector2D( (AllottedGeometry.Size.X - OverlapWidth) / ( NumChildren + 1 ) + OverlapWidth, AllottedGeometry.Size.Y )
		: FVector2D( (AllottedGeometry.Size.X - OverlapWidth) / NumChildren + OverlapWidth, AllottedGeometry.Size.Y );

	// Major vs. Minor tabs have different tab sizes.
	// We will make our choice based on the first tab we encounter.
	TSharedPtr<SDockTab> FirstTab = (NumChildren > 0)
		? Tabs[0]
		: TabBeingDraggedPtr.Pin();

	// If there are no tabs in this tabwell, assume minor tabs.
	FVector2D MaxTabSize(0,0);
	if ( FirstTab.IsValid() )
	{
		const ETabRole::Type RoleToUse = FirstTab->IsNomadTabWithMajorTabStyle() ? ETabRole::MajorTab : FirstTab->GetTabRole();
		MaxTabSize = FDockingConstants::GetMaxTabSizeFor(RoleToUse);
	}
	else
	{
		MaxTabSize = FDockingConstants::MaxMinorTabSize;
	}

	// Don't let the tabs get too big, or they'll look ugly.
	return FVector2D (
		FMath::Min( ChildSize.X, MaxTabSize.X ),
		FMath::Min( ChildSize.Y, MaxTabSize.Y )
	);
}


float SDockingTabWell::ComputeDraggedTabOffset( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const FVector2D& InTabGrabOffsetFraction ) const
{
	const FVector2D ComputedChildSize = ComputeChildSize(MyGeometry);
	return MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).X - InTabGrabOffsetFraction .X * ComputedChildSize.X;
}




FReply SDockingTabWell::StartDraggingTab( TSharedRef<SDockTab> TabToStartDragging, FVector2D InTabGrabOffsetFraction, const FPointerEvent& MouseEvent )
{
	Tabs.Remove(TabToStartDragging);
	// We just removed the foreground tab.
	ForegroundTabIndex = INDEX_NONE;
	ParentTabStackPtr.Pin()->OnTabRemoved(TabToStartDragging->GetLayoutIdentifier());

	// Tha tab well keeps track of which tab we are dragging; we treat is specially during rendering and layout.
	TabBeingDraggedPtr = TabToStartDragging;
		
	TabGrabOffsetFraction = InTabGrabOffsetFraction;

	// We are about to start dragging a tab, so make sure its offset is correct
	this->ChildBeingDraggedOffset = ComputeDraggedTabOffset( MouseEvent.FindGeometry(SharedThis(this)), MouseEvent, InTabGrabOffsetFraction );

	// Start dragging.
	TSharedRef<FDockingDragOperation> DragDropOperation =
		FDockingDragOperation::New(
			TabToStartDragging,
			InTabGrabOffsetFraction,
			GetDockArea().ToSharedRef(),
			ParentTabStackPtr.Pin()->GetTabStackGeometry().Size
		);

	return FReply::Handled().BeginDragDrop( DragDropOperation );
}

void SDockingTabWell::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>(DragDropEvent.GetOperation());

		if (DragDropOperation->GetTabBeingDragged()->CanDockInNode(ParentTabStackPtr.Pin().ToSharedRef(), SDockTab::DockingViaTabWell))
		{
			// The user dragged a tab into this TabWell.

			// Update the state of the DragDropOperation to reflect this change.
			DragDropOperation->OnTabWellEntered( SharedThis(this) );

			// Preview the position of the tab in the TabWell
			this->TabBeingDraggedPtr = DragDropOperation->GetTabBeingDragged();
			this->TabGrabOffsetFraction = DragDropOperation->GetTabGrabOffsetFraction();
			
			// The user should see the contents of the tab that we're dragging.
			ParentTabStackPtr.Pin()->SetNodeContent(DragDropOperation->GetTabBeingDragged()->GetContent(), SNullWidget::NullWidget, SNullWidget::NullWidget);
		}
	}
}

void SDockingTabWell::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>( DragDropEvent.GetOperation() );
		// Check for TabBeingDraggedPtr validity as it may no longer be valid when dragging tabs in game
		if (this->TabBeingDraggedPtr.IsValid() && DragDropOperation->GetTabBeingDragged()->CanDockInNode(ParentTabStackPtr.Pin().ToSharedRef(), SDockTab::DockingViaTabWell))
		{
			// Update the DragAndDrop operation based on this change.
			const int32 LastForegroundTabIndex = Tabs.Find(this->TabBeingDraggedPtr.Pin().ToSharedRef());

			// The user is pulling a tab out of this TabWell.
			this->TabBeingDraggedPtr.Pin()->SetParent();

			TSharedPtr<SDockingTabStack> ParentDockNode = ParentTabStackPtr.Pin();

			// We are no longer dragging a tab in this tab well, so stop
			// showing it in the TabWell.
			this->TabBeingDraggedPtr.Reset();

			// Also stop showing its content; switch to the last tab that was active.
			BringTabToFront( FMath::Max(LastForegroundTabIndex-1, 0) );

			// We may have removed the last tab that this DockNode had.
			if (Tabs.Num() == 0 )
			{
				// Let the DockNode know that it is no longer needed.
				ParentDockNode->OnLastTabRemoved();
			}

			GetDockArea()->CleanUp( SDockingNode::TabRemoval_DraggedOut );

			const FGeometry& DockNodeGeometry = ParentDockNode->GetTabStackGeometry();
			DragDropOperation->OnTabWellLeft( SharedThis(this), DockNodeGeometry );
		}
	}
}


FReply SDockingTabWell::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>( DragDropEvent.GetOperation() );
		if (DragDropOperation->GetTabBeingDragged()->CanDockInNode(ParentTabStackPtr.Pin().ToSharedRef(), SDockTab::DockingViaTabWell))
		{
			// We are dragging the tab through a TabWell.
			// Update the position of the Tab that we are dragging in the panel.
			this->ChildBeingDraggedOffset = ComputeDraggedTabOffset(MyGeometry, DragDropEvent, TabGrabOffsetFraction);
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();	
}

FReply SDockingTabWell::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>( DragDropEvent.GetOperation().ToSharedRef() ) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>( DragDropEvent.GetOperation() );
		if (DragDropOperation->GetTabBeingDragged()->CanDockInNode(ParentTabStackPtr.Pin().ToSharedRef(), SDockTab::DockingViaTabWell))
		{
			if ( ensure( TabBeingDraggedPtr.IsValid() ) )
			{
				// We dropped a Tab into this TabWell.

				// Figure out where in this TabWell to drop the Tab.
				const float ChildWidth = ComputeChildSize(MyGeometry).X;
				const TSharedRef<SDockTab> TabBeingDragged = TabBeingDraggedPtr.Pin().ToSharedRef();
				const float ChildWidthWithOverlap = ChildWidth - TabBeingDragged->GetOverlapWidth();
				const float DraggedChildCenter = ChildBeingDraggedOffset + ChildWidth / 2;
				const int32 DropLocationIndex = FMath::Clamp( static_cast<int32>(DraggedChildCenter/ChildWidthWithOverlap), 0, Tabs.Num() );

				ensure( DragDropOperation->GetTabBeingDragged().ToSharedRef() == TabBeingDragged );

				// Actually insert the new tab.
				ParentTabStackPtr.Pin()->OpenTab(TabBeingDragged, DropLocationIndex);

				// We are no longer dragging a tab.
				TabBeingDraggedPtr.Reset();

				// We knew how to handled this drop operation!
				return FReply::Handled();
			}
		}
	}

	// Someone just dropped something in here, but we have no idea what to do with it.
	return FReply::Unhandled();
}

EWindowZone::Type SDockingTabWell::GetWindowZoneOverride() const
{
	// Pretend we are a title bar so the user can grab the area to move the window around
	return EWindowZone::TitleBar;
}


void SDockingTabWell::BringTabToFront( int32 TabIndexToActivate )
{
	const bool bActiveIndexChanging = TabIndexToActivate != ForegroundTabIndex;
	if ( bActiveIndexChanging )
	{
		const int32 LastForegroundTabIndex = FMath::Min(ForegroundTabIndex, Tabs.Num()-1);

		// For positive indexes, don't go out of bounds on the array.
		ForegroundTabIndex = FMath::Min(TabIndexToActivate, Tabs.Num()-1);

		TSharedPtr<SDockingArea> MyDockArea = GetDockArea();
		if ( Tabs.Num() > 0 && MyDockArea.IsValid() )
		{
			const TSharedPtr<SDockTab> PreviousForegroundTab = (LastForegroundTabIndex == INDEX_NONE)
				? TSharedPtr<SDockTab>()
				: Tabs[LastForegroundTabIndex];

			const TSharedPtr<SDockTab> NewForegroundTab = (ForegroundTabIndex == INDEX_NONE)
				? TSharedPtr<SDockTab>()
				: Tabs[ForegroundTabIndex];
			
			MyDockArea->GetTabManager()->GetPrivateApi().OnTabForegrounded(NewForegroundTab, PreviousForegroundTab);
		}
	}

	// Always force a refresh, even if we don't think the active index changed.
	RefreshParentContent();
}

/** Activate the tab specified by TabToActivate SDockTab. */
void SDockingTabWell::BringTabToFront( TSharedPtr<SDockTab> TabToActivate )
{
	if (Tabs.Num() > 0)
	{
		for (int32 TabIndex=0; TabIndex < Tabs.Num(); ++TabIndex )
		{
			if (Tabs[TabIndex] == TabToActivate)
			{
				BringTabToFront( TabIndex );
				return;
			}
		}
	}
}

/** Gets the currently active tab (or the currently dragged tab), or a null pointer if no tab is active. */
TSharedPtr<SDockTab> SDockingTabWell::GetForegroundTab() const
{
	if (TabBeingDraggedPtr.IsValid())
	{
		return TabBeingDraggedPtr.Pin();
	}
	return (Tabs.Num() > 0 && ForegroundTabIndex > INDEX_NONE) ? Tabs[ForegroundTabIndex] : TSharedPtr<SDockTab>();
}

/** Gets the index of the currently active tab, or INDEX_NONE if no tab is active or a tab is being dragged. */
int32 SDockingTabWell::GetForegroundTabIndex() const
{
	return (Tabs.Num() > 0) ? ForegroundTabIndex : INDEX_NONE;
}

void SDockingTabWell::RemoveAndDestroyTab(const TSharedRef<SDockTab>& TabToRemove, SDockingNode::ELayoutModification RemovalMethod)
{
	int32 TabIndex = Tabs.Find(TabToRemove);

	if (TabIndex != INDEX_NONE)
	{
		const TSharedPtr<SDockingTabStack> ParentTabStack = ParentTabStackPtr.Pin();

		// Remove the old tab from the list of tabs and activate the new tab.
		{
			BringTabToFront(TabIndex);
			Tabs.RemoveAt(TabIndex);
			// We no longer have a tab in the foreground.
			// This is important because BringTabToFront triggers notifications based on the difference in active tab indexes.
			ForegroundTabIndex = INDEX_NONE;

			// Now bring the last tab that we were on to the foreground
			BringTabToFront(FMath::Max(TabIndex-1, 0));
		}
		
		if ( ensure(ParentTabStack.IsValid()) )
		{
			ParentTabStack->OnTabClosed( TabToRemove );
			
			// We might be closing down an entire dock area, if this is a major tab.
			// Use this opportunity to save its layout
			if (RemovalMethod == SDockingNode::TabRemoval_Closed)
			{
				ParentTabStack->GetDockArea()->GetTabManager()->GetPrivateApi().OnTabClosing( TabToRemove );
			}

			if (Tabs.Num() == 0)
			{
				ParentTabStack->OnLastTabRemoved();
			}
			else
			{
				RefreshParentContent();
			}
		}


		GetDockArea()->CleanUp( RemovalMethod );
	}
}

void SDockingTabWell::RefreshParentContent()
{
	if (Tabs.Num() > 0 && ForegroundTabIndex != INDEX_NONE)
	{
		const TSharedRef<SDockTab>& ForegroundTab = Tabs[ForegroundTabIndex];
		FGlobalTabmanager::Get()->SetActiveTab( ForegroundTab );

		TSharedPtr<SWindow> ParentWindowPtr = ForegroundTab->GetParentWindow();
		if (ParentWindowPtr.IsValid() && ParentWindowPtr != FGlobalTabmanager::Get()->GetRootWindow())
		{
			ParentWindowPtr->SetTitle( ForegroundTab->GetTabLabel() );
		}

		ParentTabStackPtr.Pin()->SetNodeContent( ForegroundTab->GetContent(), ForegroundTab->GetLeftContent(), ForegroundTab->GetRightContent() );
	}
	else
	{
		ParentTabStackPtr.Pin()->SetNodeContent( SNullWidget::NullWidget, SNullWidget::NullWidget, SNullWidget::NullWidget );
	}
}

TSharedPtr<SDockingArea> SDockingTabWell::GetDockArea()
{
	return ParentTabStackPtr.IsValid() ? ParentTabStackPtr.Pin()->GetDockArea() : TSharedPtr<SDockingArea>();
}


TSharedPtr<SDockingTabStack> SDockingTabWell::GetParentDockTabStack()
{
	return ParentTabStackPtr.IsValid() ? ParentTabStackPtr.Pin() : TSharedPtr<SDockingTabStack>();
}
