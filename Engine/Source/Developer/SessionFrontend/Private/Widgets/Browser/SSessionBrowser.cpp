// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowser.cpp: Implements the SSessionBrowser class.
=============================================================================*/

#include "SessionFrontendPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionBrowser"


/* SSessionBrowser structors
 *****************************************************************************/

SSessionBrowser::~SSessionBrowser( )
{
	if (SessionManager.IsValid())
	{
		SessionManager->OnSelectedSessionChanged().RemoveAll(this);
		SessionManager->OnSessionsUpdated().RemoveAll(this);
	}
}


/* SSessionBrowser interface
 *****************************************************************************/

void SSessionBrowser::Construct( const FArguments& InArgs, ISessionManagerRef InSessionManager )
{
	SessionManager = InSessionManager;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.FillHeight(1.0)
			[
				SAssignNew(Splitter, SSplitter)

				+ SSplitter::Slot()
					.Value(0.4)
					[
						SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(0.0f)
							[
								// session list
								SAssignNew(SessionListView, SListView<ISessionInfoPtr>)
									.ItemHeight(24.0f)
									.ListItemsSource(&SessionList)
									.OnContextMenuOpening(this, &SSessionBrowser::HandleSessionListViewContextMenuOpening)
									.OnGenerateRow(this, &SSessionBrowser::HandleSessionListViewGenerateRow)
									.OnSelectionChanged(this, &SSessionBrowser::HandleSessionListViewSelectionChanged)
									.HeaderRow
									(
										SNew(SHeaderRow)

										+ SHeaderRow::Column("Status")
											.DefaultLabel(FText::FromString(" "))
											.FixedWidth(24.0f)

										+ SHeaderRow::Column("Name")
											.DefaultLabel(LOCTEXT("SessionListNameColumnTitle", "Session Name"))
											.FillWidth(0.4f)

										+ SHeaderRow::Column("Owner")
											.DefaultLabel(LOCTEXT("SessionListOwnerColumnTitle", "Owner"))
											.FillWidth(0.6f)
									)
							]
					]

				+ SSplitter::Slot()
					.Value(0.5)
					[
						SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(0.0f)
							[
								// instance list
								SAssignNew(InstanceListView, SSessionInstanceList, SessionManager.ToSharedRef())
									.SelectionMode(ESelectionMode::Multi)
							]
					]

			]
		/*
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				// command bar
				SAssignNew(CommandBar, SSessionBrowserCommandBar)
			]*/
	];

	SessionManager->OnSelectedSessionChanged().AddSP(this, &SSessionBrowser::HandleSessionManagerSelectedSessionChanged);
	SessionManager->OnSessionsUpdated().AddSP(this, &SSessionBrowser::HandleSessionManagerSessionsUpdated);

	ReloadSessions();
}


/* SWidget overrides
 *****************************************************************************/

void SSessionBrowser::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if ((AllottedGeometry.Size.X > 512.0f) && (AllottedGeometry.Size.X > AllottedGeometry.Size.Y))
	{
		Splitter->SetOrientation(Orient_Horizontal);
	}
	else
	{
		Splitter->SetOrientation(Orient_Vertical);
	}
}


/* SSessionBrowser implementation
 *****************************************************************************/

void SSessionBrowser::FilterSessions( )
{
	SessionList.Reset();

	for (int32 Index = 0; Index < AvailableSessions.Num(); ++Index)
	{
		const ISessionInfoPtr& Session = AvailableSessions[Index];

		if (Session->GetSessionOwner() == FPlatformProcess::UserName(false))
		{
			SessionList.Add(Session);
		}
	}

	SessionListView->RequestListRefresh();
	SessionListView->SetSelection(SessionManager->GetSelectedSession());
}


void SSessionBrowser::ReloadSessions( )
{
	SessionManager->GetSessions(AvailableSessions);
	FilterSessions();

	if ((SessionList.Num() > 0) && !SessionManager->GetSelectedSession().IsValid())
	{
		SessionManager->SelectSession(SessionList[0]);
	}
}


/* SSessionBrowser event handlers
 *****************************************************************************/

TSharedPtr<SWidget> SSessionBrowser::HandleSessionListViewContextMenuOpening( )
{
	const ISessionInfoPtr& SelectedSession = SessionManager->GetSelectedSession();

	if (SelectedSession.IsValid() && (SelectedSession->GetSessionOwner() == FPlatformProcess::UserName(false)))
	{
		return SNew(SSessionBrowserContextMenu, SessionManager.ToSharedRef());
	}		
	
	return NULL;
}


TSharedRef<ITableRow> SSessionBrowser::HandleSessionListViewGenerateRow( ISessionInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SSessionBrowserSessionListRow, OwnerTable)
		.SessionInfo(Item)
		.ToolTipText(LOCTEXT("SessionToolTipSessionId", "Session ID: ").ToString() + Item->GetSessionId().ToString(EGuidFormats::DigitsWithHyphensInBraces));
}


void SSessionBrowser::HandleSessionListViewSelectionChanged( ISessionInfoPtr Item, ESelectInfo::Type SelectInfo )
{
	if (!SessionManager->SelectSession(Item))
	{
		SessionListView->SetSelection(SessionManager->GetSelectedSession());
	}
}


void SSessionBrowser::HandleSessionManagerSelectedSessionChanged( const ISessionInfoPtr& SelectedSession )
{
	if (!SessionListView->IsItemSelected(SelectedSession))
	{
		SessionListView->SetSelection(SelectedSession);
	}
}


void SSessionBrowser::HandleSessionManagerSessionsUpdated( )
{
	ReloadSessions();
}


#undef LOCTEXT_NAMESPACE
