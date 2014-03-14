// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "DockingPrivate.h"


static const FVector2D ContextButtonTargetSize(24,24);
static const float TriggerAreaFraction = 0.24f;

void SDockingTabStack::Construct( const FArguments& InArgs, const TSharedRef<FTabManager::FStack>& PersistentNode  )
{
	Tabs = PersistentNode->Tabs;
	this->SetSizeCoefficient(PersistentNode->GetSizeCoefficient());

	bIsDocumentArea = InArgs._IsDocumentArea;

	InlineContentAreaLeft = NULL;
	InlineContentAreaRight = NULL;

	TitleAreaLeftSlot = NULL;
	TitleAreaRightSlot = NULL;

	this->TabStackGeometry = FGeometry();

	// Animation that toggles the tabs
	{
		ShowHideTabWell = FCurveSequence(0,0.15);
		if (PersistentNode->bHideTabWell)
		{
			ShowHideTabWell.JumpToStart();
		}
		else
		{
			ShowHideTabWell.JumpToEnd();
		}
		
	}

	// In TabStack mode we glue together a TabWell, two InlineContent areas and a ContentOverlay
	// that shows the content of the currently selected Tab.
	//                                         ________ TabWell
	//                                        |
	//  +-------------------------------------v-------------------------------+
	//  |                       +--------------------+                        |
	//  | InlineContentAreaLeft | Tab0 | Tab1 | Tab2 | InlineContentAreaRight | 
	//  +---------------------------------------------------------------------+
	//  |                                                                     |
	//  |                                                                     |  <-- Content area overlay
	//  |                                                                     |
	//  +---------------------------------------------------------------------+
	//

	const FButtonStyle* const UnhideTabWellButtonStyle = &FCoreStyle::Get().GetWidgetStyle< FButtonStyle >( "Docking.UnhideTabwellButton" );

	this->ChildSlot
	[
		SNew(SVerticalBox)
		.Visibility( EVisibility::SelfHitTestInvisible )
		+SVerticalBox::Slot()
		. AutoHeight()
		[
			// TAB WELL AREA
			SNew(SBorder)
			.Visibility( this, &SDockingTabStack::GetTabWellVisibility )
			.DesiredSizeScale( this, &SDockingTabStack::GetTabWellScale )
			.BorderImage( FCoreStyle::Get().GetBrush("NoBorder") )
			.VAlign(VAlign_Bottom)
			.OnMouseButtonDown( this, &SDockingTabStack::TabWellRightClicked )
			.Padding(0)
			[
				SNew(SVerticalBox)
				.Visibility( EVisibility::SelfHitTestInvisible )
				+SVerticalBox::Slot()
				.AutoHeight()
				[

					SNew(SHorizontalBox)
					.Visibility( EVisibility::SelfHitTestInvisible )

					// Left title area (if we have one)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Expose( TitleAreaLeftSlot )
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(SSpacer)	// Will be filled in later, in SetParentNode()
					]

					// Inline Content Left
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Expose( InlineContentAreaLeft )
					.Padding( 0,0,5,0 )				

					+SHorizontalBox::Slot() 
					.FillWidth(1)
					.VAlign(VAlign_Bottom)
					[
						SNew(SVerticalBox)
						.Visibility( EVisibility::SelfHitTestInvisible )
						+SVerticalBox::Slot()
						. AutoHeight()
						[
							SNew(SSpacer)
							.Visibility(this, &SDockingTabStack::GetMaximizeSpacerVisibility)
							.Size(FVector2D(0.0f, 10.0f))
						]
						+SVerticalBox::Slot()
						. AutoHeight()
						[
							// TabWell
							SAssignNew(TabWell, SDockingTabWell)
							.ParentStackNode( SharedThis(this) )
						]
					]

					// Inline Content Right
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Expose( InlineContentAreaRight )
					.Padding(5,0,0,0)

					// Right title area (if we have one)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Expose( TitleAreaRightSlot )
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(SSpacer)	// Will be filled in later, in SetParentNode()
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SImage)
					.Image( this, &SDockingTabStack::GetTabWellBrush )
				]
			]
		]
		+SVerticalBox::Slot()
		. FillHeight(1)
		[
			SAssignNew(OverlayManagement.ContentAreaOverlay, SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(ContentSlot, SBorder)
				. BorderImage( this, &SDockingTabStack::GetContentAreaBrush )
				. Padding( this, &SDockingTabStack::GetContentPadding )
				[
					// Selected CONTENT AREA
					SNew(STextBlock)
					. Text( NSLOCTEXT("DockTabStack", "EmptyTabMessage", "Empty Tab!") )
				]
			]
			+SOverlay::Slot()
			.Padding(0)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(SButton)
				.ButtonStyle( UnhideTabWellButtonStyle )
				.OnClicked( this, &SDockingTabStack::UnhideTabWell )
				.ContentPadding(0)
				.Visibility( this, &SDockingTabStack::GetUnhideButtonVisibility )
				.DesiredSizeScale( this, &SDockingTabStack::GetUnhideTabWellButtonScale )
				.ButtonColorAndOpacity( this, &SDockingTabStack::GetUnhideTabWellButtonOpacity )
				[
					// Button should be big enough to show its own image
					SNew(SSpacer)
					.Size( UnhideTabWellButtonStyle->Normal.ImageSize )
				]
			]
			
			#if DEBUG_TAB_MANAGEMENT
			+SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage( FCoreStyle::Get().GetBrush( "Docking.Border" ) )
				.BorderBackgroundColor(FLinearColor(1,0.5,0,0.75f))
				.Visibility(EVisibility::HitTestInvisible)
				[
					SNew(STextBlock)
					.Text(this, &SDockingTabStack::ShowPersistentTabs)
					.ShadowOffset(FVector2D::UnitVector)
				]
			]
			#endif
		]
	];

	if (bIsDocumentArea)
	{
		this->SetNodeContent( SDocumentAreaWidget::MakeDocumentAreaWidget(), SNullWidget::NullWidget, SNullWidget::NullWidget );
	}
}

void SDockingTabStack::OnLastTabRemoved()
{
	if (!bIsDocumentArea)
	{
		// Stop holding onto any meaningful window content.
		// The user should not see any content in this DockNode.
		this->SetNodeContent( SNullWidget::NullWidget, SNullWidget::NullWidget, SNullWidget::NullWidget );
	}
	else
	{
		this->SetNodeContent( SDocumentAreaWidget::MakeDocumentAreaWidget(), SNullWidget::NullWidget, SNullWidget::NullWidget );
	}
}

void SDockingTabStack::OnTabClosed( const TSharedRef<SDockTab>& ClosedTab )
{
	const FTabId& TabIdBeingClosed = ClosedTab->GetLayoutIdentifier();
	
	// Document-style tabs are positioned per use-case.
	const bool bIsTabPersistable = TabIdBeingClosed.IsTabPersistable();
	if (bIsTabPersistable)
	{
		ClosePersistentTab(TabIdBeingClosed);
	}
	else
	{
		RemovePersistentTab( TabIdBeingClosed );
	}	
}

void SDockingTabStack::OnTabRemoved( const FTabId& TabId )
{
	RemovePersistentTab( TabId );
}

void SDockingTabStack::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	TabStackGeometry = AllottedGeometry;
}

void SDockingTabStack::OpenTab( const TSharedRef<SDockTab>& InTab, int32 InsertLocationAmongActiveTabs )
{
	const int32 TabIndex = OpenPersistentTab( InTab->GetLayoutIdentifier(), InsertLocationAmongActiveTabs );
	AddTabWidget( InTab, TabIndex );
	// The tab may be a nomad tab, in which case it should inherit whichever tab manager it is being put into!
	InTab->SetTabManager( GetDockArea()->GetTabManager() );
	OnLiveTabAdded();
	TabWell->RefreshParentContent();
}

void SDockingTabStack::AddTabWidget( const TSharedRef<SDockTab>& InTab, int32 AtLocation )
{
	TabWell->AddTab( InTab, AtLocation );

	if ( IsTabWellHidden() && TabWell->GetNumTabs() > 1 )
	{
		SetTabWellHidden(false);
	}
	
	// We just added a tab, so if there was a cross up we no longer need it.
	HideCross();
	TSharedPtr<SDockingArea> ParentDockArea = GetDockArea();
	if (ParentDockArea.IsValid())
	{
		ParentDockArea->HideCross();
	}

}

const TArray< TSharedRef<SDockTab> >& SDockingTabStack::GetTabs() const
{
	return TabWell->GetTabs();
}

int32 SDockingTabStack::GetNumTabs() const
{
	return TabWell->GetNumTabs();
}

bool SDockingTabStack::HasTab( const struct FTabMatcher& TabMatcher ) const
{
	return Tabs.FindMatch( TabMatcher ) != INDEX_NONE;
}

FGeometry SDockingTabStack::GetTabStackGeometry() const
{
	return TabStackGeometry;
}

void SDockingTabStack::RemoveClosedTabsWithName( FName InName )
{
	for (int32 TabIndex=0; TabIndex < Tabs.Num();  )
	{
		const FTabManager::FTab& ThisTab = Tabs[TabIndex];
		if ( ThisTab.TabState == ETabState::ClosedTab && ThisTab.TabId == InName )
		{
			Tabs.RemoveAtSwap(TabIndex);
		}
		else
		{
			++TabIndex;
		}
	}
}

bool SDockingTabStack::IsShowingLiveTabs() const
{
	return this->TabWell->GetNumTabs() > 0;
}

void SDockingTabStack::BringToFront( const TSharedRef<SDockTab>& TabToBringToFront )
{
	TabWell->BringTabToFront(TabToBringToFront);
}

void SDockingTabStack::SetNodeContent( const TSharedRef<SWidget>& InContent, const TSharedRef<SWidget>& ContentLeft, const TSharedRef<SWidget>& ContentRight )
{
	ContentSlot->SetContent(InContent);
	(*InlineContentAreaLeft)[ContentLeft];
	(*InlineContentAreaRight)[ContentRight];
}

FReply SDockingTabStack::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>(DragDropEvent.GetOperation());
		
		if (DragDropOperation->GetTabBeingDragged()->CanDockInNode(SharedThis(this), SDockTab::DockingViaTarget))
		{
			FGeometry OverlayGeometry = this->FindChildGeometry( MyGeometry, OverlayManagement.ContentAreaOverlay.ToSharedRef() );
	
			if ( OverlayGeometry.IsUnderLocation( DragDropEvent.GetScreenSpacePosition() ) )
			{
				ShowCross();
			}
			else
			{
				HideCross();
			}

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SDockingTabStack::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>(DragDropEvent.GetOperation());
		HideCross();
	}
}

FReply SDockingTabStack::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>(DragDropEvent.GetOperation());
		HideCross();
	}

	return FReply::Unhandled();
}

void SDockingTabStack::OnKeyboardFocusChanging( const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath )
{
	
	const TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	if ( ForegroundTab.IsValid() )
	{
		const bool bIsForegroundTabActive = NewWidgetPath.ContainsWidget( SharedThis(this) );
	
		if (bIsForegroundTabActive)
		{
			// If a widget inside this tab stack got focused, activate this tab.
			FGlobalTabmanager::Get()->SetActiveTab( ForegroundTab );
		}
	}
}

FReply SDockingTabStack::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	const TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	if ( ForegroundTab.IsValid() && !ForegroundTab->IsActive() )
	{
		FGlobalTabmanager::Get()->SetActiveTab( ForegroundTab );
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}


FReply SDockingTabStack::OnUserAttemptingDock( SDockingNode::RelativeDirection Direction, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FDockingDragOperation>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FDockingDragOperation> DragDropOperation = StaticCastSharedPtr<FDockingDragOperation>(DragDropEvent.GetOperation());

		// We want to replace this placeholder  with whatever is being dragged.				
		CreateNewTabStackBySplitting( Direction )->OpenTab( DragDropOperation->GetTabBeingDragged().ToSharedRef() );

		HideCross();

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

TArray< TSharedRef<SDockTab> > SDockingTabStack::GetAllChildTabs() const
{
	return GetTabs();
}

void SDockingTabStack::CloseForegroundTab()
{
	TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	if (ForegroundTab.IsValid())
	{
		ForegroundTab->RequestCloseTab();
	}
}

void SDockingTabStack::CloseAllButForegroundTab(ETabsToClose TabsToClose)
{
	TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	if (ForegroundTab.IsValid())
	{
		int32 DestroyIndex = 0;
		while ((TabWell->GetNumTabs() > 1) && (DestroyIndex < TabWell->GetNumTabs()))
		{
			auto Tab = TabWell->GetTabs()[DestroyIndex];

			const bool bCanClose = (TabsToClose == CloseDocumentsAndTools) || (Tab->GetTabRole() == ETabRole::DocumentTab);

			if ((Tab == ForegroundTab) || !bCanClose)
			{
				++DestroyIndex;
			}
			else
			{
				Tab->RequestCloseTab();
			}
		}
	}
}

FReply SDockingTabStack::TabWellRightClicked( const FGeometry& TabWellGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		FSlateApplication::Get().PushMenu( SharedThis( this ), MakeContextMenu(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu );
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

SDockingNode::ECleanupRetVal SDockingTabStack::CleanUpNodes()
{
	if (TabWell->GetNumTabs() > 0)
	{
		return VisibleTabsUnderNode;
	}
	else if (Tabs.Num() > 0)
	{
		Visibility = EVisibility::Collapsed;
		return HistoryTabsUnderNode;
	}
	else
	{
		return NoTabsUnderNode;
	}
}

TSharedRef<SWidget> SDockingTabStack::MakeContextMenu()
{
	// Show a menu that allows users to toggle whether
	// a specific tab should hide if it is the sole tab
	// in its tab well.
	const bool bCloseAfterSelection = true;
	const bool bCloseSelfOnly = false;
	FMenuBuilder MenuBuilder( bCloseAfterSelection, NULL, TSharedPtr<FExtender>(), bCloseSelfOnly, &FCoreStyle::Get() );
	{
		MenuBuilder.BeginSection("DockingTabStackOptions", NSLOCTEXT("Docking", "TabOptionsHeading", "Options") );
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("Docking", "HideTabWell", "Hide Tab"),
				NSLOCTEXT("Docking", "HideTabWellTooltip", "Hide the tabs to save room."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &SDockingTabStack::ToggleTabWellVisibility ),
					FCanExecuteAction::CreateSP( this, &SDockingTabStack::CanHideTabWell )
				) 
			);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("DockingTabStackCloseTabs");
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("Docking", "CloseTab", "Close"),
				NSLOCTEXT("Docking", "CloseTabTooltil", "Close this tab."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &SDockingTabStack::CloseForegroundTab ),
					FCanExecuteAction()
				)
			);

			// If the active tab is a document tab, and there is more than one open in this tab well, offer to close the others
			auto ForegroundTabPtr = TabWell->GetForegroundTab();
			if (ForegroundTabPtr.IsValid() && (ForegroundTabPtr->GetTabRole() == ETabRole::DocumentTab) && (TabWell->GetNumTabs() > 1))
			{
				const ETabsToClose TabsToClose = CloseDocumentTabs;

				MenuBuilder.AddMenuEntry(
					NSLOCTEXT("Docking", "CloseOtherTabs", "Close Other Tabs"),
					NSLOCTEXT("Docking", "CloseOtherTabsTooltil", "Closes all tabs except for the active tab."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP( this, &SDockingTabStack::CloseAllButForegroundTab, TabsToClose ),
						FCanExecuteAction()
					)
				);
					
			}
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SDockingTabStack::ShowCross()
{
	const float DockTargetSize = 32.0f;

	if (!OverlayManagement.bShowingCross)
	{
		this->GetDockArea()->ShowCross();

		OverlayManagement.bShowingCross = true;
		OverlayManagement.ContentAreaOverlay->AddSlot()
		. HAlign(HAlign_Fill)
		. VAlign(VAlign_Fill)
		[
			
			SNew( SDockingCross, SharedThis(this) )

			//SNew(SBorder)
			//. BorderImage( FStyleDefaults::GetNoBrush() )
			//. Padding( FMargin(0,0,0,0))
			//. Content()
			//[
			//	SNew(SVerticalBox)
			//	+ SVerticalBox::Slot()
			//.AutoHeight()
			//	[
			//		// TOP ROW
			//		SNew(SHorizontalBox)
			//		+ SHorizontalBox::Slot() .FillWidth(1.0f)
			//		+ SHorizontalBox::Slot()
			//		[
			//			SNew(SDockingTarget)
			//			. OwnerNode( SharedThis(this) )
			//			. DockDirection( SDockingNode::Above )
			//		]
			//		+ SHorizontalBox::Slot() .FillWidth(1.0f)
			//	]
			//	+ SVerticalBox::Slot()
			//.AutoHeight()
			//	[
			//		// MIDDLE ROW
			//		SNew(SHorizontalBox)
			//		+ SHorizontalBox::Slot()
			//		.FillWidth(1.0f)
			//		[
			//			SNew(SDockingTarget)
			//			. OwnerNode( SharedThis(this) )
			//			. DockDirection( SDockingNode::LeftOf )
			//		]
			//		+ SHorizontalBox::Slot().AutoWidth()
			//		[
			//			// The center node is redundant with just moving the tab into place.
			//			// It was also confusing to many.
			//			SNew(SDockingTarget)
			//			. Visibility(EVisibility::Hidden)
			//			. OwnerNode( SharedThis(this) )
			//			. DockDirection( SDockingNode::Center )
			//		]
			//		+ SHorizontalBox::Slot()
			//		.FillWidth(1.0f)
			//		[
			//			SNew(SDockingTarget)
			//			. OwnerNode( SharedThis(this) )
			//			. DockDirection( SDockingNode::RightOf )
			//		]
			//	]
			//	+ SVerticalBox::Slot()
			//.AutoHeight()
			//	.HAlign(HAlign_Center)
			//	[
			//		// BOTTOM ROW
			//		SNew(SHorizontalBox)
			//		+ SHorizontalBox::Slot() .FillWidth(1.0f)
			//		+ SHorizontalBox::Slot().AutoWidth()
			//		[
			//			SNew(SDockingTarget)
			//			. OwnerNode( SharedThis(this) )
			//			. DockDirection( SDockingNode::Below )
			//		]
			//		+ SHorizontalBox::Slot() .FillWidth(1.0f)
			//	]
			//]
		];

	}
}

void SDockingTabStack::HideCross()
{
	if (OverlayManagement.bShowingCross)
	{
		OverlayManagement.ContentAreaOverlay->RemoveSlot();
		OverlayManagement.bShowingCross = false;
	}
}

TSharedPtr<FTabManager::FLayoutNode> SDockingTabStack::GatherPersistentLayout() const
{
	if( Tabs.Num() > 0 )
	{
		// Each live tab might want to save custom visual state.
		{
			const TArray< TSharedRef<SDockTab> >& MyTabs = this->GetTabs();
			for (int32 TabIndex=0; TabIndex < MyTabs.Num(); ++TabIndex)
			{
				MyTabs[TabIndex]->PersistVisualState();
			}
		}

		// Persist layout
		TSharedRef<FTabManager::FStack> PersistentStack =
			FTabManager::NewStack()
			->SetSizeCoefficient( this->GetSizeCoefficient() )
			->SetHideTabWell( this->IsTabWellHidden() );

		TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
		if(ForegroundTab.IsValid())
		{
			PersistentStack->SetForegroundTab(ForegroundTab->GetLayoutIdentifier());
		}

		for (int32 TabIndex=0; TabIndex < Tabs.Num(); ++TabIndex)
		{
			// We do not persist document tabs. Document tabs have a valid InstanceId in addition to a TabType.
			const bool bIsTabPersistable = Tabs[TabIndex].TabId.IsTabPersistable();
			if ( bIsTabPersistable )
			{
				PersistentStack->AddTab(Tabs[TabIndex].TabId, Tabs[TabIndex].TabState);
			}			
		}
		return PersistentStack;
	}
	else
	{
		return TSharedPtr<FTabManager::FLayoutNode>();
	}
}


TSharedRef< SDockingTabStack > SDockingTabStack::CreateNewTabStackBySplitting( const SDockingNode::RelativeDirection Direction )
{
	TSharedPtr<SDockingSplitter> ParentNode = ParentNodePtr.Pin();
	check(ParentNode.IsValid());
	
	TSharedRef<SDockingTabStack> NewStack = SNew(SDockingTabStack, FTabManager::NewStack());
	{
		NewStack->SetSizeCoefficient( this->GetSizeCoefficient() );
	}	
	
	ParentNode->PlaceNode( NewStack, Direction, SharedThis(this) );
	return NewStack;
}


void SDockingTabStack::SetParentNode( TSharedRef<class SDockingSplitter> InParent )
{
	SDockingNode::SetParentNode( InParent );

	// OK, if this docking area has a parent window, we'll assume the window was created with no title bar, and we'll
	// place the title bar widgets into our content instead!
	const TSharedPtr<SDockingArea>& DockArea = GetDockArea();
	if( DockArea.IsValid() && DockArea->GetParentWindow().IsValid() )
	{
		// @todo mainframe: Really we only want these to show up for tab stacks that are along the top of the window,
		//                  and only the first one!  Currently, all SDockingAreas with a parent window set will get
		//                  title area widgets added!
		const TSharedRef<SWindow>& ParentWindow = DockArea->GetParentWindow().ToSharedRef();

		TSharedPtr< SWidget > LeftContent, RightContent;
		TSharedPtr< SWidget > CenterContent;	// NOTE: We don't care about center title area content.  No space to put it!
		ParentWindow->MakeTitleBarContentWidgets( LeftContent, CenterContent, RightContent );
		
		TitleAreaLeftSlot->Widget = LeftContent.IsValid() ? LeftContent.ToSharedRef() : TSharedRef<SWidget>( SNew( SSpacer ) );
		TitleAreaRightSlot->Widget = RightContent.IsValid() ? RightContent.ToSharedRef() : TSharedRef<SWidget>( SNew( SSpacer ) );
	}
	else
	{
		TitleAreaLeftSlot->Widget = SNew( SSpacer );
		TitleAreaRightSlot->Widget = SNew( SSpacer );
	}
}


/** What should the content area look like for the current tab? */
const FSlateBrush* SDockingTabStack::GetContentAreaBrush() const
{
	TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	return (ForegroundTab.IsValid())
		? ForegroundTab->GetContentAreaBrush()
		: FStyleDefaults::GetNoBrush();
}

FMargin SDockingTabStack::GetContentPadding() const
{
	TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	return (ForegroundTab.IsValid())
		? ForegroundTab->GetContentPadding()
		: FMargin(5);
}

EVisibility SDockingTabStack::GetTabWellVisibility() const
{	
	const bool bTabWellVisible =
		// If we are playing, we're in transition, so tab is visible.
		ShowHideTabWell.IsPlaying() ||
		// Playing forward expands the tab, so it is always visible then as well.
		!ShowHideTabWell.IsInReverse();

	return (!bTabWellVisible)
		? EVisibility::Collapsed
		: EVisibility::SelfHitTestInvisible;	// Visible, but allow clicks to pass through self (but not children)
}

const FSlateBrush* SDockingTabStack::GetTabWellBrush() const
{
	TSharedPtr<SDockTab> ForegroundTab = TabWell->GetForegroundTab();
	return ( ForegroundTab.IsValid() )
		? ForegroundTab->GetTabWellBrush()
		: FStyleDefaults::GetNoBrush();
}

EVisibility SDockingTabStack::GetUnhideButtonVisibility() const
{
	const bool bShowUnhideButton =
		// If we are playing, we're in transition, so tab is visible.
		ShowHideTabWell.IsPlaying() ||
		// Playing forward expands the tab, so it is always visible then as well.
		ShowHideTabWell.IsInReverse();

	return (bShowUnhideButton)
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

void SDockingTabStack::ToggleTabWellVisibility()
{
	ShowHideTabWell.Reverse();
}

FReply SDockingTabStack::UnhideTabWell()
{
	SetTabWellHidden(false);

	return FReply::Handled();
}

bool SDockingTabStack::CanHideTabWell() const
{
	return GetNumTabs() == 1 && (GetTabs()[0]->GetTabRole() != ETabRole::MajorTab && !GetTabs()[0]->IsNomadTabWithMajorTabStyle());
}

SSplitter::ESizeRule SDockingTabStack::GetSizeRule() const
{
	if (this->GetNumTabs() == 1 && this->GetTabs()[0]->ShouldAutosize() )
	{
		// If there is a single tab in this stack and it is
		// sized to content, then the stack's cell shoudl size to Content.
		return SSplitter::SizeToContent;
	}
	else
	{
		return SSplitter::FractionOfParent;
	}
}

void SDockingTabStack::SetTabWellHidden( bool bShouldHideTabWell )
{
	if (bShouldHideTabWell)
	{
		ShowHideTabWell.PlayReverse();
	}
	else
	{
		ShowHideTabWell.Play();
	}
}

bool SDockingTabStack::IsTabWellHidden() const
{
	return ShowHideTabWell.IsInReverse();
}

FVector2D SDockingTabStack::GetTabWellScale() const
{
	return FVector2D(1,ShowHideTabWell.GetLerp());
}

FVector2D SDockingTabStack::GetUnhideTabWellButtonScale() const
{
	return FMath::Lerp(FVector2D::UnitVector, 8*FVector2D::UnitVector, ShowHideTabWell.GetLerp());
}

FSlateColor SDockingTabStack::GetUnhideTabWellButtonOpacity() const
{
	return FLinearColor( 1,1,1, 1.0f - ShowHideTabWell.GetLerp() );
}

int32 SDockingTabStack::OpenPersistentTab( const FTabId& TabId, int32 OpenLocationAmongActiveTabs )
{
	const int32 ExistingClosedTabIndex = Tabs.FindMatch( FTabMatcher(TabId, ETabState::ClosedTab) );

	if (OpenLocationAmongActiveTabs == INDEX_NONE)
	{						
		if (ExistingClosedTabIndex != INDEX_NONE)
		{
			// There's already a tab with that name; open it.
			Tabs[ExistingClosedTabIndex].TabState = ETabState::OpenedTab;
			return ExistingClosedTabIndex;
		}
		else
		{
			// This tab was never opened in the tab stack before; add it.
			Tabs.Add( FTabManager::FTab( TabId, ETabState::OpenedTab ) );
			return Tabs.Num()-1;
		}
	}
	else
	{
		// @TODO: This branch maybe needs to become a separate function: More like MoveOrAddTab

		// We need to open a tab in a specific location.

		// We have the index of the open tab where to insert. But we need the index in the persistent
		// array, which is an ordered list of all tabs ( both open and closed ).
		int32 OpenLocationInGlobalList=INDEX_NONE;
		for (int32 TabIndex = 0, OpenTabIndex=0; TabIndex < Tabs.Num() && OpenLocationInGlobalList == INDEX_NONE; ++TabIndex)
		{
			const bool bThisTabIsOpen = (Tabs[TabIndex].TabState == ETabState::OpenedTab);
			if ( bThisTabIsOpen )
			{
				if (OpenTabIndex == OpenLocationAmongActiveTabs)
				{
					OpenLocationInGlobalList = TabIndex;
				}
				++OpenTabIndex;
			}
		}

		if (OpenLocationInGlobalList == INDEX_NONE)
		{
			OpenLocationInGlobalList = Tabs.Num();
		}

		if ( ExistingClosedTabIndex == INDEX_NONE )
		{
			// Create a new tab.
			Tabs.Insert( FTabManager::FTab( TabId, ETabState::OpenedTab ), OpenLocationInGlobalList );
			return OpenLocationAmongActiveTabs;
		}
		else
		{
			// Move the existing closed tab to the new desired location
			FTabManager::FTab TabToMove = Tabs[ExistingClosedTabIndex];
			Tabs.RemoveAt( ExistingClosedTabIndex );

			// If the element we removed was before the insert location, subtract one since the index was shifted during the removal
			if ( ExistingClosedTabIndex <= OpenLocationInGlobalList )
			{
				OpenLocationInGlobalList--;
			}

			// Mark the tab opened
			TabToMove.TabState = ETabState::OpenedTab;

			Tabs.Insert( TabToMove, OpenLocationInGlobalList );
			return OpenLocationAmongActiveTabs;
		}
	}
}

int32 SDockingTabStack::ClosePersistentTab( const FTabId& TabId )
{
	const int32 TabIndex = Tabs.FindMatch( FTabMatcher(TabId, ETabState::OpenedTab) );
	if (TabIndex != INDEX_NONE)
	{
		Tabs[TabIndex].TabState = ETabState::ClosedTab;
	}
	return TabIndex;
}

void SDockingTabStack::RemovePersistentTab( const FTabId& TabId )
{
	const int32 TabIndex = Tabs.FindMatch( FTabMatcher(TabId) );
	Tabs.RemoveAtSwap(TabIndex);
}

EVisibility SDockingTabStack::GetMaximizeSpacerVisibility() const
{
	if(GetDockArea().IsValid() && GetDockArea()->GetParentWindow().IsValid())
	{
		if (GetDockArea()->GetParentWindow()->IsWindowMaximized())
		{
			return EVisibility::Collapsed;
		}
		else
		{
			return EVisibility::SelfHitTestInvisible;
		}
	}

	return EVisibility::Collapsed;
}


#if DEBUG_TAB_MANAGEMENT

FString SDockingTabStack::ShowPersistentTabs() const
{
	FString AllTabs;
	for (int32 TabIndex=0; TabIndex < Tabs.Num(); ++TabIndex)
	{
		AllTabs += (Tabs[TabIndex].TabState == ETabState::OpenedTab ) ? TEXT("[^]") : TEXT("[x]");
		AllTabs += Tabs[TabIndex].TabId.ToString();
		AllTabs += TEXT(" ");
	}
	return AllTabs;
}

#endif

