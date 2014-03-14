// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCultureListRow.h: Declares the SSessionLauncherCultureListRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherCultureListRow"


/**
 * Implements a row widget for map list.
 */
class SSessionLauncherCultureListRow
	: public SMultiColumnTableRow<TSharedPtr<FString> >
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherCultureListRow) { }
		SLATE_ATTRIBUTE(FString, HighlightString)
		SLATE_ARGUMENT(TSharedPtr<STableViewBase>, OwnerTableView)
		SLATE_ARGUMENT(TSharedPtr<FString>, CultureName)
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InProfileManager - The profile manager to use.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
	{
		HighlightString = InArgs._HighlightString;
		CultureName = InArgs._CultureName;
		Model = InModel;

		SMultiColumnTableRow<TSharedPtr<FString> >::Construct(FSuperRowType::FArguments(), InArgs._OwnerTableView.ToSharedRef());
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
		if (ColumnName == "Culture")
		{
			return SNew(SCheckBox)
				.IsChecked(this, &SSessionLauncherCultureListRow::HandleCheckBoxIsChecked)
				.OnCheckStateChanged(this, &SSessionLauncherCultureListRow::HandleCheckBoxCheckStateChanged)
				.Padding(FMargin(6.0, 2.0))
				[
					SNew(STextBlock)
						.Text(*CultureName)			
				];
		}

		return SNullWidget::NullWidget;
	}


private:

	// Callback for changing the checked state of the check box.
	void HandleCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			if (NewState == ESlateCheckBoxState::Checked)
			{
				SelectedProfile->AddCookedCulture(*CultureName);
			}
			else
			{
				SelectedProfile->RemoveCookedCulture(*CultureName);
			}
		}
	}

	// Callback for determining the checked state of the check box.
	ESlateCheckBoxState::Type HandleCheckBoxIsChecked( ) const
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid() && SelectedProfile->GetCookedCultures().Contains(*CultureName))
		{
			return ESlateCheckBoxState::Checked;
		}

		return ESlateCheckBoxState::Unchecked;
	}


private:

	// Holds the highlight string for the log message.
	TAttribute<FString> HighlightString;

	// Holds the map's name.
	TSharedPtr<FString> CultureName;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};


#undef LOCTEXT_NAMESPACE
