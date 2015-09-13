// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MessagingDebuggerPrivatePCH.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "Json.h"
#include "JsonStructSerializerBackend.h"
#include "PropertyEditorModule.h"
#include "StructSerializer.h"


#define LOCTEXT_NAMESPACE "SMessagingMessageData"


/* SMessagingMessageData structors
 *****************************************************************************/

SMessagingMessageData::~SMessagingMessageData()
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
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = this;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	FStructureDetailsViewArgs StructureViewArgs;
	{
		StructureViewArgs.bShowObjects = false;
		StructureViewArgs.bShowAssets = true;
		StructureViewArgs.bShowClasses = true;
		StructureViewArgs.bShowInterfaces = false;
	}

	StructureDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor")
		.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr, LOCTEXT("MessageData", "Message Data"));

	StructureDetailsView->GetDetailsView().SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &SMessagingMessageData::HandleDetailsViewIsPropertyEditable));
	StructureDetailsView->GetDetailsView().SetVisibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SMessagingMessageData::HandleDetailsViewVisibility)));

	ChildSlot
	[
		StructureDetailsView->GetWidget().ToSharedRef()
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

bool SMessagingMessageData::HandleDetailsViewIsPropertyEditable() const
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (!SelectedMessage.IsValid() || !SelectedMessage->Context.IsValid())
	{
		return false;
	}

	return (SelectedMessage->TimeRouted == 0.0);
}


EVisibility SMessagingMessageData::HandleDetailsViewVisibility() const
{
	if (Model->GetSelectedMessage().IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


void SMessagingMessageData::HandleModelSelectedMessageChanged()
{
	FMessageTracerMessageInfoPtr SelectedMessage = Model->GetSelectedMessage();

	if (SelectedMessage.IsValid() && SelectedMessage->Context.IsValid())
	{
		UScriptStruct* MessageTypeInfo = SelectedMessage->Context->GetMessageTypeInfo().Get();

		if (MessageTypeInfo != nullptr)
		{
			StructureDetailsView->SetStructureData(MakeShareable(new FStructOnScope(MessageTypeInfo, (uint8*)SelectedMessage->Context->GetMessage())));
		}
		else
		{
			StructureDetailsView->SetStructureData(nullptr);
		}
	}
	else
	{
		StructureDetailsView->SetStructureData(nullptr);
	}
}


#undef LOCTEXT_NAMESPACE