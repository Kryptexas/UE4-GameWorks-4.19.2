// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingHistory.cpp: Implements the SMessagingHistory class.
=============================================================================*/

#include "MessagingDebuggerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMessagingHistory"


/* SMessagingHistory structors
 *****************************************************************************/

SMessagingHistory::~SMessagingHistory( )
{
	if (Model.IsValid())
	{
		Model->OnMessageVisibilityChanged().RemoveAll(this);
	}

	if (Tracer.IsValid())
	{
		Tracer->OnMessageAdded().RemoveAll(this);
		Tracer->OnMessagesReset().RemoveAll(this);
	}
}


/* SMessagingHistory interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMessagingHistory::Construct( const FArguments& InArgs, const FMessagingDebuggerModelRef& InModel, const TSharedRef<ISlateStyle>& InStyle, const IMessageTracerRef& InTracer )
{
	Filter = MakeShareable(new FMessagingDebuggerMessageFilter());
	Model = InModel;
	ShouldScrollToLast = true;
	Style = InStyle;
	Tracer = InTracer;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(0.0f)
					[
						// filter bar
						SNew(SMessagingHistoryFilterBar, Filter.ToSharedRef())
					]
			]

		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(0.0f)
					[
						// message list
						SAssignNew(MessageListView, SListView<FMessageTracerMessageInfoPtr>)
							.ItemHeight(24.0f)
							.ListItemsSource(&MessageList)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SMessagingHistory::HandleMessageListGenerateRow)
							.OnItemScrolledIntoView(this, &SMessagingHistory::HandleMessageListItemScrolledIntoView)
							.OnMouseButtonDoubleClick(this, &SMessagingHistory::HandleMessageListItemDoubleClick)
							.OnSelectionChanged(this, &SMessagingHistory::HandleMessageListSelectionChanged)
							.HeaderRow
							(
								SNew(SHeaderRow)

								+ SHeaderRow::Column("SendType")
									.DefaultLabel( FText::FromString(TEXT(" ")))
									.FixedWidth(20.0f)

								+ SHeaderRow::Column("TimeSent")
									.DefaultLabel(LOCTEXT("MessageListTimeSentColumnHeader", "Time Sent"))
									.FixedWidth(112.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)

								+ SHeaderRow::Column("MessageType")
									.DefaultLabel(LOCTEXT("MessageListMessageTypeColumnHeader", "Message Type"))
									.FillWidth(1.0f)

								+ SHeaderRow::Column("Scope")
									.DefaultLabel(LOCTEXT("MessageListScopeColumnHeader", "Scope"))
									.FixedWidth(64.0f)

								+ SHeaderRow::Column("RouteLatency")
									.DefaultLabel(LOCTEXT("MessageListRouteLatencyColumnHeader", "Routing Latency"))
									.FixedWidth(112.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)

								+ SHeaderRow::Column("DispatchLatency")
									.DefaultLabel(LOCTEXT("MessageListDispatchLatencyColumnHeader", "Dispatch Latency"))
									.FixedWidth(112.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)

								+ SHeaderRow::Column("HandleTime")
									.DefaultLabel(LOCTEXT("MessageListHandleTimeColumnHeader", "Handle Time"))
									.FixedWidth(80.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)

								+ SHeaderRow::Column("Flag")
									.DefaultLabel( FText::FromString(TEXT(" ")))
									.FixedWidth(20.0f)
							)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						// status bar
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
									.Text(this, &SMessagingHistory::HandleStatusBarText)
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.HAlign(HAlign_Left)
							[
								SNew(SHyperlink)
									.OnNavigate(this, &SMessagingHistory::HandleShowHiddenHyperlinkNavigate)
									.Text(LOCTEXT("ShowHiddenHyperlinkText", "show all"))
									.ToolTipText(LOCTEXT("NoCulturesHyperlinkTooltip", "Reset endpoint and message type visibility filters to make all messages visible."))
									.Visibility(this, &SMessagingHistory::HandleShowHiddenHyperlinkVisibility)
							]
					]
			]
	];

	Filter->OnChanged().AddRaw(this, &SMessagingHistory::HandleFilterChanged);
	Model->OnMessageVisibilityChanged().AddRaw(this, &SMessagingHistory::HandleModelMessageVisibilityChanged);
	Tracer->OnMessageAdded().AddRaw(this, &SMessagingHistory::HandleTracerMessageAdded);
	Tracer->OnMessagesReset().AddRaw(this, &SMessagingHistory::HandleTracerMessagesReset);

	ReloadMessages();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SMessagingHistory implementation
 *****************************************************************************/

void SMessagingHistory::AddMessage( const FMessageTracerMessageInfoRef& MessageInfo )
{
	++TotalMessages;

	if (!Model->IsMessageVisible(MessageInfo))
	{
		return;
	}

	if (!Filter->FilterEndpoint(MessageInfo))
	{
		return;
	}

	MessageList.Add(MessageInfo);
	MessageListView->RequestListRefresh();
}


void SMessagingHistory::ReloadMessages( )
{
	MessageList.Reset();
	TotalMessages = 0;
	
	TArray<FMessageTracerMessageInfoPtr> Messages;

	if (Tracer->GetMessages(Messages) > 0)
	{
		for (TArray<FMessageTracerMessageInfoPtr>::TConstIterator It(Messages); It; ++It)
		{
			AddMessage(It->ToSharedRef());
		}
	}

	MessageListView->RequestListRefresh();
}


/* SMessagingHistory callbacks
 *****************************************************************************/

void SMessagingHistory::HandleFilterChanged( )
{
	ReloadMessages();
}


TSharedRef<ITableRow> SMessagingHistory::HandleMessageListGenerateRow( FMessageTracerMessageInfoPtr MessageInfo, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SMessagingHistoryTableRow, OwnerTable)
		.HighlightText(this, &SMessagingHistory::HandleMessageListGetHighlightText)
		.MessageInfo(MessageInfo)
		.Style(Style);
}


FText SMessagingHistory::HandleMessageListGetHighlightText( ) const
{
	return FText::GetEmpty();
	//return FilterBar->GetFilterText();
}


void SMessagingHistory::HandleMessageListItemDoubleClick( FMessageTracerMessageInfoPtr Item )
{

}


void SMessagingHistory::HandleMessageListItemScrolledIntoView( FMessageTracerMessageInfoPtr Item, const TSharedPtr<ITableRow>& TableRow )
{
	if (MessageList.Num() > 0)
	{
		ShouldScrollToLast = MessageListView->IsItemVisible(MessageList.Last());
	}
	else
	{
		ShouldScrollToLast = true;
	}
}


void SMessagingHistory::HandleMessageListSelectionChanged( FMessageTracerMessageInfoPtr InItem, ESelectInfo::Type SelectInfo )
{
	Model->SelectMessage(InItem);
}


void SMessagingHistory::HandleModelMessageVisibilityChanged( )
{
	ReloadMessages();
}


void SMessagingHistory::HandleShowHiddenHyperlinkNavigate( )
{
	Model->ClearVisibilities();
}


EVisibility SMessagingHistory::HandleShowHiddenHyperlinkVisibility( ) const
{
	if (TotalMessages > MessageList.Num())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


FString SMessagingHistory::HandleStatusBarText( ) const
{
	FString Result;
	int32 VisibleMessages = MessageList.Num();

	if (VisibleMessages > 0)
	{
		Result = FString::Printf(*LOCTEXT("StatusBarNumMessages", "%i messages").ToString(), MessageList.Num());

		if (MessageListView->GetNumItemsSelected() > 0)
		{
			Result += FString::Printf(*LOCTEXT("StatusBarNumSelected", ", %i selected").ToString(), MessageListView->GetNumItemsSelected());
		}

		if (VisibleMessages < TotalMessages)
		{
			Result += FString::Printf(*LOCTEXT("StatusBarNumHidden", ", %i hidden - ").ToString(), (TotalMessages - VisibleMessages));
		}
	}
	else
	{
		Result = TEXT("Press the 'Start' button to trace messages");
	}

	return Result;
}


void SMessagingHistory::HandleTracerMessageAdded( FMessageTracerMessageInfoRef MessageInfo )
{
	AddMessage(MessageInfo);

	if (ShouldScrollToLast && !Tracer->IsBreaking() && (MessageList.Num() > 0))
	{
		MessageListView->RequestScrollIntoView(MessageList.Last(0));
	}
}


void SMessagingHistory::HandleTracerMessagesReset( )
{
	ReloadMessages();
}


#undef LOCTEXT_NAMESPACE
