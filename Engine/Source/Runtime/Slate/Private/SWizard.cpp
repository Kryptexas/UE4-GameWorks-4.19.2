// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SWizard.cpp: Implements the SWizard class.
=============================================================================*/

#include "Slate.h"


#define LOCTEXT_NAMESPACE "SWizard"


/* SWizard interface
 *****************************************************************************/

bool SWizard::CanShowPage( int32 PageIndex ) const
{
	if (Pages.IsValidIndex(PageIndex))
	{
		return Pages[PageIndex].CanShow();
	}

	return false;
}


void SWizard::Construct( const FArguments& InArgs )
{
	DesiredSize = InArgs._DesiredSize.Get();
	OnCanceled = InArgs._OnCanceled;
	OnFinished = InArgs._OnFinished;
	OnFirstPageBackClicked = InArgs._OnFirstPageBackClicked;

	TSharedPtr<SVerticalBox> PageListBox;
	TSharedPtr< SUniformGridPanel > ButtonGrid; 

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.FillHeight(1.0)			
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 20.0f, 0.0f)
					[
						SAssignNew(PageListBox, SVerticalBox)
							.Visibility(InArgs._ShowPageList ? EVisibility::Visible : EVisibility::Collapsed)
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						// widget switcher
						SAssignNew(WidgetSwitcher, SWidgetSwitcher)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SAssignNew(ButtonGrid, SUniformGridPanel)
				.SlotPadding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+SUniformGridPanel::Slot(0,0)
				[
					// 'Prev' button
					SNew(SButton)
					.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
					.IsEnabled(this, &SWizard::HandlePrevButtonIsEnabled)
					.OnClicked(this, &SWizard::HandlePrevButtonClicked)
					.Visibility(this, &SWizard::HandlePrevButtonVisibility)
					.ToolTipText(LOCTEXT("PrevButtonTooltip", "Go back to the previous step"))
					.Text(LOCTEXT("PrevButtonLabel", "< Back"))
				]
				+SUniformGridPanel::Slot(1,0)
				[
					// 'Next' button
					SNew(SButton)
					.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
					.IsEnabled(this, &SWizard::HandleNextButtonIsEnabled)
					.OnClicked(this, &SWizard::HandleNextButtonClicked)
					.Visibility(this, &SWizard::HandleNextButtonVisibility)
					.ToolTipText(LOCTEXT("NextButtonTooltip", "Go to the next step"))
					.Text(LOCTEXT("NextButtonLabel", "Next >"))
				]
				+SUniformGridPanel::Slot(2,0)
				[
					// 'Finish' button
					SNew(SButton)
					.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
					.IsEnabled(InArgs._CanFinish)
					.OnClicked(this, &SWizard::HandleFinishButtonClicked)
					.ToolTipText(InArgs._FinishButtonToolTip)
					.Text(InArgs._FinishButtonText)
				]
			]
	];

	if ( InArgs._ShowCancelButton )
	{
		ButtonGrid->AddSlot( 3, 0 )
		[
			// cancel button
			SNew(SButton)
			.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
			.OnClicked(this, &SWizard::HandleCancelButtonClicked)
			.ToolTipText(LOCTEXT("CancelButtonTooltip", "Cancel this wizard"))
			.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
		];
	}

	// populate wizard with pages
	for (int32 SlotIndex = 0; SlotIndex < InArgs.Slots.Num(); ++SlotIndex)
	{
		FWizardPage* Page = InArgs.Slots[SlotIndex];

		Pages.Add(Page);

		if (InArgs._ShowPageList)
		{
			PageListBox->AddSlot()
			.AutoHeight()
				[
					SNew(SCheckBox)
						.IsChecked(this, &SWizard::HandlePageButtonIsChecked, SlotIndex)
						.IsEnabled(this, &SWizard::HandlePageButtonIsEnabled, SlotIndex)
						.OnCheckStateChanged(this, &SWizard::HandlePageButtonCheckStateChanged, SlotIndex)
						.Padding(FMargin(8.0f, 4.0f, 24.0f, 4.0f))
						.Style(&FCoreStyle::Get().GetWidgetStyle< FCheckBoxStyle >("ToggleButtonCheckbox"))
						[
							Page->GetButtonContent()
						]
				];
		}

		WidgetSwitcher->AddSlot()
			[
				Page->GetPageContent()
			];
	}

	WidgetSwitcher->SetActiveWidgetIndex(InArgs._InitialPageIndex.Get());
}


void SWizard::ShowPage( int32 PageIndex )
{
	int32 ActivePageIndex = WidgetSwitcher->GetActiveWidgetIndex();

	if (Pages.IsValidIndex(ActivePageIndex))
	{
		Pages[ActivePageIndex].OnLeave().ExecuteIfBound();
	}

	if (Pages.IsValidIndex(PageIndex) && Pages[PageIndex].CanShow())
	{
		WidgetSwitcher->SetActiveWidgetIndex(PageIndex);

		// show the desired page
		Pages[PageIndex].OnEnter().ExecuteIfBound();
	}
	else if (Pages.IsValidIndex(0) && Pages[0].CanShow())
	{
		WidgetSwitcher->SetActiveWidgetIndex(0);

		// attempt to fall back to first page
		Pages[0].OnEnter().ExecuteIfBound();
	}
	else
	{
		WidgetSwitcher->SetActiveWidgetIndex(INDEX_NONE);
	}
}


/* SCompoundWidget overrides
 *****************************************************************************/

FVector2D SWizard::ComputeDesiredSize( ) const
{
	if (DesiredSize.IsZero())
	{
		return SCompoundWidget::ComputeDesiredSize();
	}

	return DesiredSize;
}


/* SWizard callbacks
 *****************************************************************************/

FReply SWizard::HandleCancelButtonClicked( )
{
	OnCanceled.ExecuteIfBound();

	return FReply::Handled();
}


FReply SWizard::HandleFinishButtonClicked( )
{
	OnFinished.ExecuteIfBound();

	return FReply::Handled();
}


FReply SWizard::HandleNextButtonClicked( )
{
	ShowPage(WidgetSwitcher->GetActiveWidgetIndex() + 1);

	return FReply::Handled();
}


bool SWizard::HandleNextButtonIsEnabled( ) const
{
	return CanShowPage(WidgetSwitcher->GetActiveWidgetIndex() + 1);
}


EVisibility SWizard::HandleNextButtonVisibility( ) const
{
	if (Pages.IsValidIndex(WidgetSwitcher->GetActiveWidgetIndex() + 1))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


void SWizard::HandlePageButtonCheckStateChanged( ESlateCheckBoxState::Type NewState, int32 PageIndex )
{
	if (NewState == ESlateCheckBoxState::Checked)
	{
		ShowPage(PageIndex);
	}
}


ESlateCheckBoxState::Type SWizard::HandlePageButtonIsChecked( int32 PageIndex ) const
{
	if (PageIndex == WidgetSwitcher->GetActiveWidgetIndex())
	{
		return ESlateCheckBoxState::Checked;
	}

	return ESlateCheckBoxState::Unchecked;
}


bool SWizard::HandlePageButtonIsEnabled( int32 PageIndex ) const
{
	return CanShowPage(PageIndex);
}


FReply SWizard::HandlePrevButtonClicked( )
{
	if ( WidgetSwitcher->GetActiveWidgetIndex() == 0 && OnFirstPageBackClicked.IsBound() )
	{
		return OnFirstPageBackClicked.Execute();
	}
	else
	{
		ShowPage(WidgetSwitcher->GetActiveWidgetIndex() - 1);
	}

	return FReply::Handled();
}


bool SWizard::HandlePrevButtonIsEnabled( ) const
{
	if ( WidgetSwitcher->GetActiveWidgetIndex() == 0 && OnFirstPageBackClicked.IsBound() )
	{
		return true;
	}

	return CanShowPage(WidgetSwitcher->GetActiveWidgetIndex() - 1);
}


EVisibility SWizard::HandlePrevButtonVisibility( ) const
{
	if (WidgetSwitcher->GetActiveWidgetIndex() > 0 || OnFirstPageBackClicked.IsBound())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


#undef LOCTEXT_NAMESPACE
