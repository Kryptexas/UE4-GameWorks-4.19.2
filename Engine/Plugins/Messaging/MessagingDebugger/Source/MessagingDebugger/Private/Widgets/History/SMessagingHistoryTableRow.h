// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingHistoryTableRow.h: Declares the SMessagingHistoryTableRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SMessagingHistoryTableRow"


/**
 * Implements a row widget for the message history list.
 */
class SMessagingHistoryTableRow
	: public SMultiColumnTableRow<FMessageTracerMessageInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(SMessagingHistoryTableRow) { }
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_ARGUMENT(FMessageTracerMessageInfoPtr, MessageInfo)
		SLATE_ARGUMENT(TSharedPtr<ISlateStyle>, Style)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InOwnerTableView - The table view that owns this row.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		check(InArgs._MessageInfo.IsValid());
		check(InArgs._Style.IsValid());

		MaxDispatchLatency = -1.0;
		MaxHandlingTime = -1.0;
		HighlightText = InArgs._HighlightText;
		MessageInfo = InArgs._MessageInfo;
		Style = InArgs._Style;

		SMultiColumnTableRow<FMessageTracerMessageInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

public:

	// Begin SWidget overrides

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE
	{
		SWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		MaxDispatchLatency = -1.0;
		MaxHandlingTime = -1.0;

		for (TMap<FMessageTracerEndpointInfoPtr, FMessageTracerDispatchStatePtr>::TConstIterator It(MessageInfo->DispatchStates); It; ++It)
		{
			const FMessageTracerDispatchStatePtr& DispatchState = It.Value();

			if (MessageInfo->TimeRouted > 0.0)
			{
				MaxDispatchLatency = FMath::Max(MaxDispatchLatency, DispatchState->DispatchLatency);
			}

			if (DispatchState->TimeHandled > 0.0)
			{
				MaxHandlingTime = FMath::Max(MaxHandlingTime, DispatchState->TimeHandled - DispatchState->TimeDispatched);
			}
		}
	}

	// End SWidget overrides

public:

	// Begin SMultiColumnTableRow interface

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == "DispatchLatency")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SMessagingHistoryTableRow::HandleDispatchLatencyText)
				];
		}
		else if (ColumnName == "Flag")
		{
			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.Image(this, &SMessagingHistoryTableRow::HandleFlagImage)
						.ToolTipText(LOCTEXT("DeadMessageTooltip", "No valid recipients (dead letter)"))
				];
		}
		else if (ColumnName == "HandleTime")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SMessagingHistoryTableRow::HandleTextColorAndOpacity)
						.Text(this, &SMessagingHistoryTableRow::HandleHandlingTimeText)
				];
		}
		else if (ColumnName == "MessageType")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SMessagingHistoryTableRow::HandleTextColorAndOpacity)
						.HighlightText(HighlightText)
						.Text(MessageInfo->Context->GetMessageType().ToString())
				];
		}
		else if (ColumnName == "RouteLatency")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SMessagingHistoryTableRow::HandleRouteLatencyColorAndOpacity)
						.Text(this, &SMessagingHistoryTableRow::HandleRouteLatencyText)
				];
		}
		else if (ColumnName == "Scope")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SMessagingHistoryTableRow::HandleScopeText)
				];
		}
		else if (ColumnName == "SendType")
		{
			bool Forwarded;
			int32 NumRecipients;

			if (MessageInfo->Context.IsValid())
			{
				Forwarded = MessageInfo->Context->IsForwarded();
				NumRecipients = MessageInfo->Context->GetRecipients().Num();
			}
			else
			{
				Forwarded = false;
				NumRecipients = 0;
			}
			
			bool Published = (NumRecipients == 0);

			FFormatNamedArguments Args;
			Args.Add(TEXT("NumRecipients"), NumRecipients);

			FText ToolTipText;
			FName BrushImageName;

			if (Forwarded)
			{
				ToolTipText = FText::Format(LOCTEXT("ForwaredMessageTooltip", "Forwarded Message ({NumRecipients} recipients)"), Args);
				BrushImageName = "ForwardedMessage";
			}
			else if (Published)
			{
				ToolTipText = LOCTEXT("PublishedMessageTooltip", "Published Message");
				BrushImageName = "PublishedMessage";
			}
			else
			{
				ToolTipText = FText::Format(LOCTEXT("ForwaredMessageTooltip", "Sent Message ({NumRecipients} recipients)"), Args);
				BrushImageName = "SentMessage";
			}

			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(Style->GetBrush(BrushImageName))
					.ToolTipText(ToolTipText)
				];
		}
		else if (ColumnName == "TimeSent")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SMessagingHistoryTableRow::HandleTextColorAndOpacity)
						.HighlightText(HighlightText)
						.Text(FText::AsNumber(MessageInfo->TimeSent - GStartTime))
				];
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	// End SMultiColumnTableRow interface

protected:

	/**
	 * Converts the given time span in seconds to a human readable string.
	 *
	 * @param Seconds - The time span to convert.
	 *
	 * @return The string representation.
	 *
	 * @todo gmp: refactor this into FText::AsTimespan or something like that
	 */
	FText TimespanToReadableText( double Seconds ) const
	{
		if (Seconds < 0.0)
		{
			return LOCTEXT("Zero Length Timespan", "-");
		}

		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 1;
		Options.MaximumFractionalDigits = 1;

		if (Seconds < 0.0001)
		{
			return FText::Format(LOCTEXT("Seconds < 0.0001 Length Timespan", "{0} us"), FText::AsNumber(Seconds * 1000000, &Options));
		}

		if (Seconds < 0.1)
		{
			return FText::Format(LOCTEXT("Seconds < 0.1 Length Timespan", "{0} ms"), FText::AsNumber(Seconds * 1000, &Options));
		}

		if (Seconds < 60.0)
		{
			return FText::Format(LOCTEXT("Seconds < 60.0 Length Timespan", "{0} s"), FText::AsNumber(Seconds, &Options));
		}
		
		return LOCTEXT("> 1 minute Length Timespan", "> 1 min");
	}

private:

	// Callback for getting the text olor of the DispatchLatency column.
	FSlateColor HandleDispatchLatencyColorAndOpacity( ) const
	{
		if (MaxDispatchLatency >= 0.01)
		{
			return FLinearColor::Red;
		}

		if (MaxDispatchLatency >= 0.001)
		{
			return FLinearColor(1.0f, 1.0f, 0.0f);
		}

		if (MaxDispatchLatency >= 0.0001)
		{
			return FLinearColor::Yellow;
		}

		return FSlateColor::UseForeground();
	}

	// Callback for getting the text of the DispatchLatency column.
	FText HandleDispatchLatencyText( ) const
	{
		return TimespanToReadableText(MaxDispatchLatency);
	}

	const FSlateBrush* HandleFlagImage( ) const
	{
		if ((MessageInfo->TimeRouted > 0.0) && (MessageInfo->DispatchStates.Num() == 0))
		{
			return Style->GetBrush("DeadMessage");
		}

		return NULL;
	}

	// Callback for getting the text of the HandlingTime column.
	FText HandleHandlingTimeText( ) const
	{
		return TimespanToReadableText(MaxHandlingTime);
	}

	// Callback for getting the text color of the RouteLatency column.
	FSlateColor HandleRouteLatencyColorAndOpacity( ) const
	{
		double RouteLatency = MessageInfo->TimeRouted - MessageInfo->TimeSent;

		if (RouteLatency >= 0.01)
		{
			return FLinearColor::Red;
		}

		if (RouteLatency >= 0.001)
		{
			return FLinearColor(1.0f, 1.0f, 0.0f);
		}

		if (RouteLatency >= 0.0001)
		{
			return FLinearColor::Yellow;
		}

		return FSlateColor::UseForeground();
	}

	// Callback for getting the text of the RouteLatency column.
	FText HandleRouteLatencyText( ) const
	{
		if (MessageInfo->TimeRouted > 0.0)
		{
			return TimespanToReadableText(MessageInfo->TimeRouted - MessageInfo->TimeSent);
		}
		
		return FText::GetEmpty();
	}

	// Callback for getting the text of the Scope column.
	FText HandleScopeText( ) const
	{
		if (MessageInfo->Context.IsValid() && MessageInfo->Context->IsForwarded() || (MessageInfo->Context->GetRecipients().Num() == 0))
		{
			EMessageScope::Type Scope = MessageInfo->Context->GetScope();

			switch (Scope)
			{
			case EMessageScope::Thread:
				return LOCTEXT("ScopeThread", "Thread");
				break;

			case EMessageScope::Process:
				return LOCTEXT("ScopeProcess", "Process");
				break;

			case EMessageScope::Network:
				return LOCTEXT("ScopeNetwork", "Network");
				break;

			case EMessageScope::All:
				return LOCTEXT("ScopeAll", "All");
				break;

			default:
				return LOCTEXT("ScopeUnknown", "Unknown");
			}
		}

		return LOCTEXT("NoScope", "-");
	}

	// Callback for getting the text color of various columns.
	FSlateColor HandleTextColorAndOpacity( ) const
	{
		if (MessageInfo->TimeRouted == 0.0)
		{
			return FSlateColor::UseSubduedForeground();
		}

		return FSlateColor::UseForeground();
	}

private:

	// Holds the highlight string for the message.
	TAttribute<FText> HighlightText;

	// Holds message's debug information.
	FMessageTracerMessageInfoPtr MessageInfo;

	// Holds the maximum dispatch latency.
	double MaxDispatchLatency;

	// Holds the maximum time that was needed to handle the message.
	double MaxHandlingTime;

	// Holds the widget's visual style.
	TSharedPtr<ISlateStyle> Style;
};


#undef LOCTEXT_NAMESPACE
