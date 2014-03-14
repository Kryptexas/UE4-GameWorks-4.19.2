// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowserInstanceListRow.h: Declares the SSessionBrowserInstanceListRow class.
=============================================================================*/

#pragma once


/**
 * Delegate type for changed instance check box state changes.
 *
 * The first parameter is the engine instance that was checked or unchecked.
 * The second parameter is the new checked state.
 */
DECLARE_DELEGATE_TwoParams(FOnSessionInstanceCheckStateChanged, const ISessionInstanceInfoPtr&, ESlateCheckBoxState::Type)


/**
 * Implements a row widget for the session instance list.
 */
class SSessionBrowserInstanceListRow
	: public SMultiColumnTableRow<ISessionInstanceInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(SSessionBrowserInstanceListRow) { }
		SLATE_ARGUMENT(ISessionInstanceInfoPtr, InstanceInfo)
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		InstanceInfo = InArgs._InstanceInfo;

		SMultiColumnTableRow<ISessionInstanceInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
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
		if (ColumnName == "Icons")
		{
			return SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1.0)
				[
					SNew(SImage)
						.Image(FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), *InstanceInfo->GetPlatformName())))
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1.0)
				[
					SNew(SImage)
						.Image(this, &SSessionBrowserInstanceListRow::HandleGetInstanceIcon)
				];
		}
		else if (ColumnName == "Instance")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 1.0f, 4.0f, 0.0f))
				.HAlign(HAlign_Left)
				[
					SNew(SBorder)
						.BorderBackgroundColor(this, &SSessionBrowserInstanceListRow::HandleBorderGetBackgroundColor)
						.BorderImage(this, &SSessionBrowserInstanceListRow::HandleBorderGetBrush)
						.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
						.Padding(FMargin(6.0, 4.0))
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Font(FEditorStyle::GetFontStyle("BoldFont"))
								.Text(InstanceInfo->GetInstanceName())							
						]
				];
		}
		else if (ColumnName == "Level")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0, 0.0))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SSessionBrowserInstanceListRow::HandleGetColorAndOpacity)
						.Text(this, &SSessionBrowserInstanceListRow::HandleGetLevelText)
				];
		}

		return SNullWidget::NullWidget;
	}


private:

	// Callback for getting the border color for this row.
	FSlateColor HandleBorderGetBackgroundColor( ) const
	{
		return FLinearColor((GetTypeHash(InstanceInfo->GetInstanceId()) & 0xff) * 360.0 / 256.0, 0.8, 0.3, 1.0).HSVToLinearRGB();
	}

	// Callback for getting the border brush for this row.
	const FSlateBrush* HandleBorderGetBrush( ) const
	{
		if (FDateTime::UtcNow() - InstanceInfo->GetLastUpdateTime() < FTimespan::FromSeconds(10.0))
		{
			return FEditorStyle::GetBrush("ErrorReporting.Box");
		}

		return FEditorStyle::GetBrush("ErrorReporting.EmptyBox");
	}

	// Callback for getting the foreground text color.
	FSlateColor HandleGetColorAndOpacity( ) const
	{
		if (FDateTime::UtcNow() - InstanceInfo->GetLastUpdateTime() < FTimespan::FromSeconds(10.0))
		{
			return FSlateColor::UseForeground();
		}

		return FSlateColor::UseSubduedForeground();
	}

	// Callback for getting the instance type icon.
	const FSlateBrush* HandleGetInstanceIcon( ) const
	{
		return FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Instance_%s"), *InstanceInfo->GetInstanceType()));
	}

	// Callback for getting the instance's current level.
	FString HandleGetLevelText( ) const
	{
		return InstanceInfo->GetCurrentLevel();
	}


private:

	// Holds a reference to the instance info that is displayed in this row.
	ISessionInstanceInfoPtr InstanceInfo;

	// Holds a delegate to be invoked when the check box state changed.
	FOnCheckStateChanged OnCheckStateChanged;
};
