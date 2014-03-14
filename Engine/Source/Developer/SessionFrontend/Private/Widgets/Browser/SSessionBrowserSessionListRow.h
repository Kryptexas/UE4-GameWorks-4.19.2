// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowserSessionListRow.h: Declares the SSessionBrowserSessionListRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionBrowserSessionListRow"


/**
 * Implements a row widget for the session browser list.
 */
class SSessionBrowserSessionListRow
	: public SMultiColumnTableRow<ISessionInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(SSessionBrowserSessionListRow) { }
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_ARGUMENT(TSharedPtr<STableViewBase>, OwnerTableView)
		SLATE_ARGUMENT(ISessionInfoPtr, SessionInfo)
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		HighlightText = InArgs._HighlightText;
		SessionInfo = InArgs._SessionInfo;

		SMultiColumnTableRow<ISessionInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}


public:

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName - The name of the column to generate the widget for.
	 *
	 * @return The widget.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == "Name")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.HighlightText(HighlightText)
						.Text(this, &SSessionBrowserSessionListRow::HandleGetSessionName)
				];
		}
		else if (ColumnName == "Owner")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(SessionInfo->GetSessionOwner())
				];
		}
		
		return SNullWidget::NullWidget;
	}


protected:

	// Callback for getting the name of the session.
	FString HandleGetSessionName( ) const
	{
		const FString& SessionName = SessionInfo->GetSessionName();

		// return name of a launched session
		if (!SessionInfo->IsStandalone() || !SessionName.IsEmpty())
		{
			return SessionName;
		}

		// generate name for a standalone session


		TArray<ISessionInstanceInfoPtr> Instances;
		SessionInfo->GetInstances(Instances);

		if (Instances.Num() > 0)
		{
			const ISessionInstanceInfoPtr& FirstInstance = Instances[0];

			if ((Instances.Num() == 1) && (FirstInstance->GetInstanceId() == FApp::GetInstanceId()))
			{
				return LOCTEXT("ThisApplicationSessionText", "This Application").ToString();
			}

			if (FirstInstance->GetDeviceName() == FPlatformProcess::ComputerName())
			{
				return LOCTEXT("UnnamedLocalSessionText", "Unnamed Session (Local)").ToString();
			}

			return LOCTEXT("UnnamedRemoteSessionText", "Unnamed Session (Remote)").ToString();
		}

		return LOCTEXT("UnnamedSessionText", "Unnamed Session").ToString();
	}


private:

	// Holds the highlight string for the session name.
	TAttribute<FText> HighlightText;

	// Holds a reference to the session info that is displayed in this row.
	ISessionInfoPtr SessionInfo;
};


#undef LOCTEXT_NAMESPACE