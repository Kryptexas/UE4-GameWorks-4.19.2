// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageProcessor.cpp: Implements the FUdpMessageProcessor class.
=============================================================================*/

#include "UdpMessagingPrivatePCH.h"


/* FUdpMessageHelloSender static initialization
 *****************************************************************************/

const int32 FUdpMessageProcessor::DeadHelloIntervals = 5;


/* FUdpMessageProcessor structors
 *****************************************************************************/

FUdpMessageProcessor::FUdpMessageProcessor( FSocket* InSocket, const FGuid& InNodeId, const FIPv4Endpoint& InMulticastEndpoint )
	: Beacon(NULL)
	, LastSentMessage(-1)
	, LocalNodeId(InNodeId)
	, MulticastEndpoint(InMulticastEndpoint)
	, Sender(NULL)
	, Socket(InSocket)
	, Stopping(false)
{
	WorkEvent = FPlatformProcess::CreateSynchEvent();
	Thread = FRunnableThread::Create(this, TEXT("FUdpMessageProcessor"), false, false, 128 * 1024, TPri_AboveNormal);

	const UUdpMessagingSettings& Settings = *GetDefault<UUdpMessagingSettings>();

	for (int32 StaticEndpointIndex = 0; StaticEndpointIndex < Settings.StaticEndpoints.Num(); ++StaticEndpointIndex)
	{
		FIPv4Endpoint Endpoint;

		if (FIPv4Endpoint::Parse(Settings.StaticEndpoints[StaticEndpointIndex], Endpoint))
		{
			FNodeInfo& NodeInfo = StaticNodes.FindOrAdd(Endpoint);
			NodeInfo.Endpoint = Endpoint;
		}
		else
		{
			GLog->Logf(TEXT("Warning: Invalid UDP Messaging StaticNode '%s'"), *Settings.StaticEndpoints[StaticEndpointIndex]);
		}
	}
}


FUdpMessageProcessor::~FUdpMessageProcessor( )
{
	Thread->Kill(true);
		
	delete Thread;
	delete WorkEvent;
}


/* FUdpMessageProcessor interface
 *****************************************************************************/

bool FUdpMessageProcessor::EnqueueInboundSegment( const FArrayReaderPtr& Data, const FIPv4Endpoint& InSender ) 
{
	if (!InboundSegments.Enqueue(FInboundSegment(Data, InSender)))
	{
		return false;
	}

	WorkEvent->Trigger();

	return true;
}


bool FUdpMessageProcessor::EnqueueOutboundMessage( const IMessageDataRef& Data, const FGuid& Recipient )
{
	if (!OutboundMessages.Enqueue(FOutboundMessage(Data, Recipient)))
	{
		return false;
	}

	Data->OnStateChanged().BindRaw(this, &FUdpMessageProcessor::HandleMessageDataStateChanged);

	if (Data->GetState() != EMessageDataState::Incomplete)
	{
		WorkEvent->Trigger();
	}

	return true;
}


/* FRunnable interface
 *****************************************************************************/

bool FUdpMessageProcessor::Init( )
{
	Beacon = new FUdpMessageBeacon(Socket, LocalNodeId, MulticastEndpoint);
	Sender = new FUdpSocketSender(Socket, TEXT("FUdpMessageProcessor.Sender"));

	return true;
}


uint32 FUdpMessageProcessor::Run( )
{
	while (!Stopping)
	{
		if (WorkEvent->Wait(CalculateWaitTime()))
		{
			CurrentTime = FDateTime::UtcNow();

			ConsumeInboundSegments();
			ConsumeOutboundMessages();

			UpdateKnownNodes();
			UpdateStaticNodes();
		}
	}
	
	delete Beacon;
	Beacon = NULL;

	delete Sender;
	Sender = NULL;

	return 0;
}


void FUdpMessageProcessor::Stop( )
{
	Stopping = true;

	WorkEvent->Trigger();
}


/* FUdpMessageProcessor implementation
 *****************************************************************************/

void FUdpMessageProcessor::AcknowledgeReceipt( int32 MessageId, const FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FHeader Header;

	Header.RecipientNodeId = NodeInfo.NodeId;
	Header.SenderNodeId = LocalNodeId;
	Header.ProtocolVersion = UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION;
	Header.SegmentType = EUdpMessageSegments::Acknowledge;

	FUdpMessageSegment::FAcknowledgeChunk AcknowledgeChunk;

	AcknowledgeChunk.MessageId = MessageId;

	FArrayWriter Writer;

	Writer << Header;
	Writer << AcknowledgeChunk;

	int32 Sent;

	Socket->SendTo(Writer.GetData(), Writer.Num(), Sent, *NodeInfo.Endpoint.ToInternetAddr());
}


FTimespan FUdpMessageProcessor::CalculateWaitTime( ) const
{
	return FTimespan::FromMilliseconds(10);
}


void FUdpMessageProcessor::ConsumeInboundSegments( )
{
	FInboundSegment Segment;

	while (InboundSegments.Dequeue(Segment))
	{
		FUdpMessageSegment::FHeader Header;

		// quick hack for TTP# 247103
		if (!Segment.Data.IsValid())
		{
			continue;
		}

		*Segment.Data << Header;

		if (FilterSegment(Header, Segment.Data, Segment.Sender))
		{
			FNodeInfo& NodeInfo = KnownNodes.FindOrAdd(Header.SenderNodeId);

			NodeInfo.Endpoint = Segment.Sender;
			NodeInfo.NodeId = Header.SenderNodeId;

			switch (Header.SegmentType)
			{
			case EUdpMessageSegments::Abort:
				ProcessAbortSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Acknowledge:
				ProcessAcknowledgeSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Bye:
				ProcessByeSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Data:			
				ProcessDataSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Hello:
				ProcessHelloSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Retransmit:
				ProcessRetransmitSegment(Segment, NodeInfo);
				break;

			case EUdpMessageSegments::Timeout:
				ProcessTimeoutSegment(Segment, NodeInfo);
				break;

			default:
				ProcessUnknownSegment(Segment, NodeInfo, Header.SegmentType);
			}

			NodeInfo.LastSegmentReceivedTime = CurrentTime;
		}
	}
}


void FUdpMessageProcessor::ConsumeOutboundMessages( )
{
	FOutboundMessage OutboundMessage;

	while (OutboundMessages.Dequeue(OutboundMessage))
	{
		++LastSentMessage;

		FNodeInfo& RecipientNodeInfo = KnownNodes.FindOrAdd(OutboundMessage.RecipientId);

		if (!OutboundMessage.RecipientId.IsValid())
		{
			RecipientNodeInfo.Endpoint = MulticastEndpoint;

			for (TMap<FIPv4Endpoint, FNodeInfo>::TIterator It(StaticNodes); It; ++It)
			{
				FNodeInfo& NodeInfo = It.Value();
				NodeInfo.Segmenters.Add(LastSentMessage, MakeShareable(new FUdpMessageSegmenter(OutboundMessage.MessageData.ToSharedRef(), 1024)));
			}
		}

		RecipientNodeInfo.Segmenters.Add(LastSentMessage, MakeShareable(new FUdpMessageSegmenter(OutboundMessage.MessageData.ToSharedRef(), 1024)));
	}
}


bool FUdpMessageProcessor::FilterSegment( const FUdpMessageSegment::FHeader& Header, const FArrayReaderPtr& Data, const FIPv4Endpoint& InSender )
{
	// filter unsupported protocol versions
	if (Header.ProtocolVersion != UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION)
	{
		return false;
	}

	// filter locally generated segments
	if (Header.SenderNodeId == LocalNodeId)
	{
		return false;
	}

	return true;
}


void FUdpMessageProcessor::ProcessAbortSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FAbortChunk AbortChunk;

	*Segment.Data << AbortChunk;

	NodeInfo.Segmenters.Remove(AbortChunk.MessageId);
}


void FUdpMessageProcessor::ProcessAcknowledgeSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FAcknowledgeChunk AcknowledgeChunk;

	*Segment.Data << AcknowledgeChunk;

	NodeInfo.Segmenters.Remove(AcknowledgeChunk.MessageId);
}


void FUdpMessageProcessor::ProcessByeSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FGuid RemoteNodeId;

	*Segment.Data << RemoteNodeId;

	if (RemoteNodeId.IsValid())
	{
		if (NodeInfo.NodeId == RemoteNodeId)
		{
			RemoveKnownNode(RemoteNodeId);
		}
	}
}


void FUdpMessageProcessor::ProcessDataSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FDataChunk DataChunk;

	*Segment.Data << DataChunk;

	// Discard late segments for sequenced messages
	if ((DataChunk.Sequence != 0) && (DataChunk.Sequence < NodeInfo.Resequencer.GetNextSequence()))
	{
		return;
	}

	FReassembledUdpMessagePtr& Message = NodeInfo.ReassembledMessages.FindOrAdd(DataChunk.MessageId);

	// Reassemble message
	if (!Message.IsValid())
	{
		Message = MakeShareable(new FReassembledUdpMessage(DataChunk.MessageSize, DataChunk.TotalSegments, DataChunk.Sequence, Segment.Sender));
	}

	Message->Reassemble(DataChunk.SegmentNumber, DataChunk.SegmentOffset, DataChunk.Data, CurrentTime);

	// Deliver or re-sequence message
	if (Message->IsComplete())
	{
		AcknowledgeReceipt(DataChunk.MessageId, NodeInfo);

		if (Message->GetSequence() == 0)
		{
			FMemoryReader HackReader(Message->GetData());

			if (NodeInfo.NodeId.IsValid())
			{
				MessageReceivedDelegate.ExecuteIfBound(HackReader, NULL, NodeInfo.NodeId);
			}
		}
		else if (NodeInfo.Resequencer.Resequence(Message))
		{
			FReassembledUdpMessagePtr ResequencedMessage;
			FMemoryReader HackReader(ResequencedMessage->GetData());

			while (NodeInfo.Resequencer.Pop(ResequencedMessage))
			{
				if (NodeInfo.NodeId.IsValid())
				{
					MessageReceivedDelegate.ExecuteIfBound(HackReader, NULL, NodeInfo.NodeId);
				}
			}
		}

		NodeInfo.ReassembledMessages.Remove(DataChunk.MessageId);
	}
}


void FUdpMessageProcessor::ProcessHelloSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FGuid RemoteNodeId;

	*Segment.Data << RemoteNodeId;

	if (RemoteNodeId.IsValid())
	{
		NodeInfo.ResetIfRestarted(RemoteNodeId);
	}
}


void FUdpMessageProcessor::ProcessRetransmitSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FRetransmitChunk RetransmitChunk;

	*Segment.Data << RetransmitChunk;

	TSharedPtr<FUdpMessageSegmenter> Segmenter = NodeInfo.Segmenters.FindRef(RetransmitChunk.MessageId);

	if (Segmenter.IsValid())
	{
		Segmenter->MarkForRetransmission(RetransmitChunk.Segments);
	}
}


void FUdpMessageProcessor::ProcessTimeoutSegment( FInboundSegment& Segment, FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FTimeoutChunk TimeoutChunk;

	*Segment.Data << TimeoutChunk;

	TSharedPtr<FUdpMessageSegmenter> Segmenter = NodeInfo.Segmenters.FindRef(TimeoutChunk.MessageId);

	if (Segmenter.IsValid())
	{
		Segmenter->MarkForRetransmission();
	}
}


void FUdpMessageProcessor::ProcessUnknownSegment( FInboundSegment& Segment, FNodeInfo& EndpointInfo, uint8 SegmentType )
{
	GLog->Logf(TEXT("FUdpMessageProcessor: Received unknown segment type '%i' from %s"), SegmentType, *Segment.Sender.ToText().ToString());
}


void FUdpMessageProcessor::RemoveKnownNode( const FGuid& NodeId )
{
	NodeLostDelegate.ExecuteIfBound(NodeId);

	KnownNodes.Remove(NodeId);
}


void FUdpMessageProcessor::UpdateKnownNodes( )
{
	// remove dead remote endpoints
	FTimespan DeadHelloTimespan = DeadHelloIntervals * Beacon->GetBeaconInterval();

	TArray<FGuid> NodesToRemove;

	for (TMap<FGuid, FNodeInfo>::TIterator It(KnownNodes); It; ++It)
	{
		FNodeInfo& NodeInfo = It.Value();

		if ((It.Key().IsValid()) && ((NodeInfo.LastSegmentReceivedTime + DeadHelloTimespan) <= CurrentTime))
		{
			NodesToRemove.Add(It.Key());
		}
		else
		{
			UpdateSegmenters(NodeInfo);
		}
	}

	for (int32 Index = 0; Index < NodesToRemove.Num(); ++Index)
	{
		// @todo gmp: put this back in after testing
		//RemoveKnownEndpoint(EndpointsToRemove(Index));
	}

	Beacon->SetEndpointCount(KnownNodes.Num() + 1);
}


void FUdpMessageProcessor::UpdateSegmenters( FNodeInfo& NodeInfo )
{
	FUdpMessageSegment::FHeader Header;

	Header.RecipientNodeId = NodeInfo.NodeId;
	Header.SenderNodeId = LocalNodeId;
	Header.ProtocolVersion = UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION;
	Header.SegmentType = EUdpMessageSegments::Data;

	for (TMap<int32, TSharedPtr<FUdpMessageSegmenter> >::TIterator It2(NodeInfo.Segmenters); It2; ++It2)
	{
		TSharedPtr<FUdpMessageSegmenter>& Segmenter = It2.Value();

		Segmenter->Initialize();

		if (Segmenter->IsInitialized())
		{
			FUdpMessageSegment::FDataChunk DataChunk;

			while (Segmenter->GetNextPendingSegment(DataChunk.Data, DataChunk.SegmentNumber))
			{
				DataChunk.MessageId = It2.Key();
				DataChunk.MessageSize = Segmenter->GetMessageSize();
				DataChunk.SegmentOffset = 1024 * DataChunk.SegmentNumber;
				DataChunk.Sequence = 0;
				DataChunk.TotalSegments = Segmenter->GetSegmentCount();

				FArrayWriter Writer;

				Writer << Header;
				Writer << DataChunk;

				if (Sender->Send(MakeShareable(new TArray<uint8>(Writer)), NodeInfo.Endpoint))
				{
					Segmenter->MarkAsSent(DataChunk.SegmentNumber);
				}
				else
				{
					return;
				}
			}

			It2.RemoveCurrent();
		}
		else if (Segmenter->IsInvalid())
		{
			It2.RemoveCurrent();
		}
	}
}


void FUdpMessageProcessor::UpdateStaticNodes( )
{
	for (TMap<FIPv4Endpoint, FNodeInfo>::TIterator It(StaticNodes); It; ++It)
	{
		UpdateSegmenters(It.Value());
	}
}


/* FUdpMessageProcessor callbacks
 *****************************************************************************/

void FUdpMessageProcessor::HandleMessageDataStateChanged( )
{
	WorkEvent->Trigger();
}
