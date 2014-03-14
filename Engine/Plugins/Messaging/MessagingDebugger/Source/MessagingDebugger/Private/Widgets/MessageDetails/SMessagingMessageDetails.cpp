// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingMessageDetails.cpp: Implements the SMessagingMessageDetails class.
=============================================================================*/

#include "MessagingDebuggerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMessagingMessageDetails"


/* SMessagingMessageDetails structors
 *****************************************************************************/

SMessagingMessageDetails::~SMessagingMessageDetails( )
{
	if (Model.IsValid())
	{
		Model->OnSelectedMessageChanged().RemoveAll(this);
	}
}


/* SMessagingMessageDetails interface
 *****************************************************************************/

void SMessagingMessageDetails::Construct( const FArguments& InArgs, const FMessagingDebuggerModelRef& InModel, const TSharedRef<ISlateStyle>& InStyle )
{
	Model = InModel;
	Style = InStyle;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 2.0f)
			[
				SNew(SGridPanel)
					.FillColumn(1, 1.0f)

				// Message type
				+ SGridPanel::Slot(0, 0)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("MessageTypeLabel", "Message Type:").ToString())
					]

				+ SGridPanel::Slot(1, 0)
					.HAlign(HAlign_Right)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(this, &SMessagingMessageDetails::HandleMessageTypeText)
					]

				// Sender
				+ SGridPanel::Slot(0, 1)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("SenderLabel", "Sender:").ToString())
					]

				+ SGridPanel::Slot(1, 1)
					.HAlign(HAlign_Right)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(this, &SMessagingMessageDetails::HandleSenderText)
					]

				// Sender thread
				+ SGridPanel::Slot(0, 2)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SenderThreadLabel", "Sender Thread:").ToString())
					]

				+ SGridPanel::Slot(1, 2)
					.HAlign(HAlign_Right)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(this, &SMessagingMessageDetails::HandleSenderThreadText)
					]

				// Time sent
				+ SGridPanel::Slot(0, 3)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("TimestampLabel", "Timestamp:").ToString())
					]

				+ SGridPanel::Slot(1, 3)
					.HAlign(HAlign_Right)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(this, &SMessagingMessageDetails::HandleTimestampText)
					]

				// Expiration
				+ SGridPanel::Slot(0, 4)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ExpirationLabel", "Expiration:").ToString())
					]

				+ SGridPanel::Slot(1, 4)
					.HAlign(HAlign_Right)
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
							.Text(this, &SMessagingMessageDetails::HandleExpirationText)
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
						SAssignNew(DispatchStateListView, SListView<FMessageTracerDispatchStatePtr>)
							.ItemHeight(24.0f)
							.ListItemsSource(&DispatchStateList)
							.SelectionMode(ESelectionMode::None)
							.OnGenerateRow(this, &SMessagingMessageDetails::HandleDispatchStateListGenerateRow)
							.HeaderRow
							(
								SNew(SHeaderRow)

								+ SHeaderRow::Column("Type")
									.DefaultLabel( FText::FromString(TEXT(" ")))
									.FixedWidth(20.0f)

								+ SHeaderRow::Column("Recipient")
									.DefaultLabel(LOCTEXT("DispatchStateListRecipientColumnHeader", "Recipient"))
									.FillWidth(1.0f)

								+ SHeaderRow::Column("DispatchLatency")
									.DefaultLabel(LOCTEXT("DispatchStateListDispatchedColumnHeader", "Dispatch Latency"))
									.FixedWidth(112.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)

								+ SHeaderRow::Column("HandleTime")
									.DefaultLabel(LOCTEXT("DispatchStateListHandledColumnHeader", "Handle Time"))
									.FixedWidth(80.0f)
									.HAlignCell(HAlign_Right)
									.HAlignHeader(HAlign_Right)
							)
					]
			]
	];

	Model->OnSelectedMessageChanged().AddRaw(this, &SMessagingMessageDetails::HandleModelSelectedMessageChanged);
}


/* SMessagingMessageDetails implementation
 *****************************************************************************/

void SMessagingMessageDetails::RefreshDetails( )
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid())
	{
		SelectedMessage->DispatchStates.GenerateValueArray(DispatchStateList);
	}
	else
	{
		DispatchStateList.Reset();
	}

	DispatchStateListView->RequestListRefresh();
}


/* SMessagingMessageDetails event handlers
 *****************************************************************************/

TSharedRef<ITableRow> SMessagingMessageDetails::HandleDispatchStateListGenerateRow( FMessageTracerDispatchStatePtr DispatchState, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SMessagingDispatchStateTableRow, OwnerTable, Model.ToSharedRef())
		.DispatchState(DispatchState)
		.Style(Style);
}


FString SMessagingMessageDetails::HandleExpirationText( ) const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->Context.IsValid())
	{
		const FDateTime& Expiration = SelectedMessage->Context->GetExpiration();
		
		if (Expiration == FDateTime::MaxValue())
		{
			return LOCTEXT("ExpirationNever", "Never").ToString();
		}

		return Expiration.ToString();
	}

	return FString();
}


FString SMessagingMessageDetails::HandleMessageTypeText( ) const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->TypeInfo.IsValid())
	{
		return SelectedMessage->TypeInfo->TypeName.ToString();
	}

	return FString();
}


void SMessagingMessageDetails::HandleModelSelectedMessageChanged( )
{
	RefreshDetails();	
}


FString SMessagingMessageDetails::HandleSenderText( ) const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->SenderInfo.IsValid())
	{
		return SelectedMessage->SenderInfo->Name.ToString();
	}

	return FString();
}


FString SMessagingMessageDetails::HandleSenderThreadText( ) const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->Context.IsValid())
	{
		ENamedThreads::Type Thread = SelectedMessage->Context->GetSenderThread();

		switch (Thread)
		{
		case ENamedThreads::AnyThread:
			return TEXT("AnyThread");
			break;

		case ENamedThreads::GameThread:
			return TEXT("GameThread");
			break;

		case ENamedThreads::ActualRenderingThread:
			return TEXT("ActualRenderingThread");
			break;

		default:
			return LOCTEXT("UnknownThread", "Unknown").ToString();
		}
	}

	return FString();
}


FString SMessagingMessageDetails::HandleTimestampText( ) const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->Context.IsValid())
	{
		return SelectedMessage->Context->GetTimeSent().ToString();
	}

	return FString();
}


#undef LOCTEXT_NAMESPACE
