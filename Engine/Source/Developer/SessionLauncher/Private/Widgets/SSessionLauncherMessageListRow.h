// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherTaskListRow.h: Declares the SSessionLauncherTaskListRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherMessageListRow"

struct FLauncherMessage
{
	FString Message;
	ELogVerbosity::Type Verbosity;

	FLauncherMessage(const FString& NewMessage, ELogVerbosity::Type InVerbosity)
		: Message(NewMessage)
		, Verbosity(InVerbosity)
	{
	}
};


/**
 * Implements a row widget for the launcher's task list.
 */
class SSessionLauncherMessageListRow
	: public SMultiColumnTableRow<TSharedPtr<FLauncherMessage>>
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherMessageListRow) { }
		SLATE_ARGUMENT(TSharedPtr<FLauncherMessage>, Message)
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InDeviceProxyManager - The device proxy manager to use.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		Message = InArgs._Message;

		SMultiColumnTableRow<TSharedPtr<FLauncherMessage>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}


public:

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName - The name of the column to generate the widget for.
	 *
	 * @return The widget.
	 */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == "Status")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0, 0.0))
				.VAlign((VAlign_Center))
				[
					SNew(STextBlock)
						.ColorAndOpacity(HandleGetTextColor())
						.Text(Message->Message)
				];
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION


private:

	// Callback for getting the task's status text.
	FSlateColor HandleGetTextColor( ) const
	{
		if ((Message->Verbosity == ELogVerbosity::Error) ||
			(Message->Verbosity == ELogVerbosity::Fatal))
		{
			return FLinearColor::Red;
		}
		else if (Message->Verbosity == ELogVerbosity::Warning)
		{
			return FLinearColor::Yellow;
		}
		else
		{
			return FSlateColor::UseForeground();
		}
	}



private:

	// Holds a pointer to the task that is displayed in this row.
	TSharedPtr<FLauncherMessage> Message;
};


#undef LOCTEXT_NAMESPACE
