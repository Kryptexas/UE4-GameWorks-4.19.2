// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingMessageData.cpp: Implements the SMessagingMessageData class.
=============================================================================*/

#include "MessagingDebuggerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMessagingMessageData"


/* SMessagingMessageData structors
 *****************************************************************************/

SMessagingMessageData::~SMessagingMessageData( )
{
	if (Model.IsValid())
	{
		Model->OnSelectedMessageChanged().RemoveAll(this);
	}
}


/* SMessagingMessageData interface
 *****************************************************************************/

void SMessagingMessageData::Construct( const FArguments& InArgs, const FMessagingDebuggerModelRef& InModel, const TSharedRef<ISlateStyle>& InStyle )
{
	Model = InModel;
	Style = InStyle;

	// initialize details view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bObjectsUseNameArea = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = this;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	DetailsView->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SMessagingMessageData::HandleDetailsViewEnabled)));
	DetailsView->SetVisibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SMessagingMessageData::HandleDetailsViewVisibility)));

	ChildSlot
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			//DetailsView.ToSharedRef()
			SNew(STextBlock)
				.Text(LOCTEXT("NotImplementedYet", "Not implemented yet"))
		];

	Model->OnSelectedMessageChanged().AddRaw(this, &SMessagingMessageData::HandleModelSelectedMessageChanged);
}


/* FNotifyHook interface
 *****************************************************************************/

void SMessagingMessageData::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged )
{
}


/* SMessagingMessageData callbacks
 *****************************************************************************/

bool SMessagingMessageData::HandleDetailsViewEnabled( ) const
{
	return true;
}


EVisibility SMessagingMessageData::HandleDetailsViewVisibility( ) const
{
	if (Model->GetSelectedMessage().IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


void SMessagingMessageData::HandleModelSelectedMessageChanged( )
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->Context.IsValid())
	{
		// @todo gmp: add support for displaying UScriptStructs in details view
	}
	else
	{
		DetailsView->SetObject(nullptr);
	}
}


#undef LOCTEXT_NAMESPACE