// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageBridge.cpp: Implements the FMessageBridge class.
=============================================================================*/

#include "MessagingPrivatePCH.h"


/* FMessageBridge structors
 *****************************************************************************/

FMessageBridge::FMessageBridge( const FMessageAddress InAddress, const IMessageBusRef& InBus, const ISerializeMessagesRef& InSerializer, const ITransportMessagesRef& InTransport )
	: Address(InAddress)
	, Bus(InBus)
	, Enabled(false)
	, Id(FGuid::NewGuid())
	, Serializer(InSerializer)
	, Transport(InTransport)
{
	Bus->OnShutdown().AddRaw(this, &FMessageBridge::HandleMessageBusShutdown);
	Transport->OnNodeLost().BindRaw(this, &FMessageBridge::HandleTransportNodeLost);
	Transport->OnMessageReceived().BindRaw(this, &FMessageBridge::HandleTransportMessageReceived);
}


FMessageBridge::~FMessageBridge( )
{
	Shutdown();

	if (Bus.IsValid())
	{
		Bus->OnShutdown().RemoveAll(this);

		TArray<FMessageAddress> RemovedAddresses;

		for (int32 AddressIndex = 0; AddressIndex < RemovedAddresses.Num(); ++AddressIndex)
		{
			Bus->Unregister(RemovedAddresses[AddressIndex]);
		}
	}
}


/* IMessageBridge interface
 *****************************************************************************/

void FMessageBridge::Disable( ) 
{
	if (!Enabled)
	{
		return;
	}

	// disable subscription & transport
	if (MessageSubscription.IsValid())
	{
		MessageSubscription->Disable();
	}

	if (Transport.IsValid())
	{
		Transport->StopTransport();
	}

	Enabled = false;
}


void FMessageBridge::Enable( )
{
	if (Enabled || !Bus.IsValid() || !Transport.IsValid())
	{
		return;
	}

	// enable subscription & transport
	if (!Transport->StartTransport())
	{
		return;
	}

	if (MessageSubscription.IsValid())
	{
		MessageSubscription->Enable();
	}
	else
	{
		MessageSubscription = Bus->Subscribe(AsShared(), NAME_All, FMessageScopeRange::AtLeast(EMessageScope::Network));
	}

	Enabled = true;
}


/* IReceiveMessages interface
 *****************************************************************************/

void FMessageBridge::ReceiveMessage( const IMessageContextRef& Context )
{
	if (!Enabled)
	{
		return;
	}

	TArray<FGuid> TransportNodes;

	if (Context->GetRecipients().Num() > 0)
	{
		TransportNodes = AddressBook.GetNodesFor(Context->GetRecipients());

		if (TransportNodes.Num() == 0)
		{
			return;
		}
	}

	// forward message to transport nodes
	FMessageDataRef MessageData = MakeShareable(new FMessageData());
	TGraphTask<FMessageSerializeTask>::CreateTask().ConstructAndDispatchWhenReady(Context, MessageData, Serializer.ToSharedRef());
	Transport->TransportMessage(MessageData, Context->GetAttachment(), TransportNodes);
}


/* ISendMessages interface
 *****************************************************************************/

void FMessageBridge::NotifyMessageError( const IMessageContextRef& Context, const FString& Error )
{
	// deprecated
}


/* FMessageBridge implementation
 *****************************************************************************/

void FMessageBridge::Shutdown( )
{
	Disable();

	if (Transport.IsValid())
	{
		Transport->OnMessageReceived().Unbind();
		Transport->OnNodeLost().Unbind();
	}
}


/* FMessageBridge callbacks
 *****************************************************************************/

void FMessageBridge::HandleMessageBusShutdown( )
{
	Shutdown();
	Bus.Reset();
}


void FMessageBridge::HandleTransportMessageReceived( FArchive& MessageData, const IMessageAttachmentPtr& Attachment, const FGuid& NodeId )
{
	if (!Enabled || !Bus.IsValid())
	{
		return;
	}

	IMutableMessageContextRef MessageContext = MakeShareable(new FMessageContext());

	if (Serializer->DeserializeMessage(MessageData, MessageContext))
	{
		if (MessageContext->GetExpiration() >= FDateTime::UtcNow())
		{
			// register newly discovered endpoints
			if (!AddressBook.Contains(MessageContext->GetSender()))
			{
				AddressBook.Add(MessageContext->GetSender(), NodeId);

				Bus->Register(MessageContext->GetSender(), AsShared());
			}

			// forward the message to the internal bus
			MessageContext->SetAttachment(Attachment);

			Bus->Forward(MessageContext, MessageContext->GetRecipients(), EMessageScope::Process, FTimespan::Zero(), AsShared());
		}
	}
}


void FMessageBridge::HandleTransportNodeLost( const FGuid& LostNodeId )
{
	TArray<FMessageAddress> RemovedAddresses;

	// update address book
	AddressBook.RemoveNode(LostNodeId, RemovedAddresses);

	// unregister endpoints
	if (Bus.IsValid())
	{
		for (int32 AddressIndex = 0; AddressIndex < RemovedAddresses.Num(); ++AddressIndex)
		{
			Bus->Unregister(RemovedAddresses[AddressIndex]);
		}
	}
}
