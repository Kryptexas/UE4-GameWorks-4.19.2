// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DataChannel.cpp: Unreal datachannel implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"

#include "Net/NetworkProfiler.h"
#include "Net/DataChannel.h"

DEFINE_LOG_CATEGORY(LogNet);
DEFINE_LOG_CATEGORY(LogNetPlayerMovement);
DEFINE_LOG_CATEGORY(LogNetTraffic);
DEFINE_LOG_CATEGORY(LogNetDormancy);
DEFINE_LOG_CATEGORY_STATIC(LogNetPartialBunch, Warning, All);

extern FAutoConsoleVariable CVarDoReplicationContextString;

/*-----------------------------------------------------------------------------
	UChannel implementation.
-----------------------------------------------------------------------------*/

UChannel::UChannel(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UChannel::Init( UNetConnection* InConnection, int32 InChIndex, bool InOpenedLocally )
{
	// if child connection then use its parent
	if (InConnection->GetUChildConnection() != NULL)
	{
		Connection = ((UChildConnection*)InConnection)->Parent;
	}
	else
	{
		Connection = InConnection;
	}
	ChIndex			= InChIndex;
	OpenedLocally	= InOpenedLocally;
	OpenPacketId	= FPacketIdRange();
	NegotiatedVer	= InConnection->NegotiatedVer;
}


void UChannel::SetClosingFlag()
{
	Closing = 1;
}


void UChannel::Close()
{
	check(Connection->Channels[ChIndex]==this);
	if
	(	!Closing
	&&	(Connection->State==USOCK_Open || Connection->State==USOCK_Pending) )
	{
		UE_LOG(LogNetDormancy, Verbose, TEXT("Channel[%d] '%s' Closing. Dormant: %d"), ChIndex, *Describe(), Dormant );

		// Send a close notify, and wait for ack.
		FOutBunch CloseBunch( this, 1 );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		CloseBunch.DebugString = FString::Printf(TEXT("%.2f Close: %s"), Connection->Driver->Time, *Describe());
#endif
		check(!CloseBunch.IsError());
		check(CloseBunch.bClose);
		CloseBunch.bReliable = 1;
		CloseBunch.bDormant = Dormant;
		SendBunch( &CloseBunch, 0 );
	}
}


void UChannel::CleanUp()
{
	checkSlow(Connection != NULL);
	checkSlow(Connection->Channels[ChIndex] == this);

	// if this is the control channel, make sure we properly killed the connection
	if (ChIndex == 0 && !Closing)
	{
		UE_LOG(LogNet, Log, TEXT("NetConnection::Close() [%s] from UChannel::Cleanup()"), Connection->Driver ? *Connection->Driver->NetDriverName.ToString() : TEXT("NULL"));
		Connection->Close();
	}

	// remember sequence number of first non-acked outgoing reliable bunch for this slot
	if (OutRec != NULL)
	{
		Connection->PendingOutRec[ChIndex] = OutRec->ChSequence;
		//UE_LOG(LogNetTraffic, Log, TEXT("%i save pending out bunch %i"),ChIndex,Connection->PendingOutRec[ChIndex]);
	}
	// Free any pending incoming and outgoing bunches.
	for (FOutBunch* Out = OutRec, *NextOut; Out != NULL; Out = NextOut)
	{
		NextOut = Out->Next;
		delete Out;
	}
	for (FInBunch* In = InRec, *NextIn; In != NULL; In = NextIn)
	{
		NextIn = In->Next;
		delete In;
	}
	if (InPartialBunch != NULL)
	{
		delete InPartialBunch;
		InPartialBunch = NULL;
	}

	// Remove from connection's channel table.
	verifySlow(Connection->OpenChannels.Remove(this) == 1);
	Connection->Channels[ChIndex] = NULL;
	Connection = NULL;
}


void UChannel::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ConditionalCleanUp();
	}
	
	Super::BeginDestroy();
}


void UChannel::ReceivedAcks()
{
	check(Connection->Channels[ChIndex]==this);

	/*
	// Verify in sequence.
	for( FOutBunch* Out=OutRec; Out && Out->Next; Out=Out->Next )
		check(Out->Next->ChSequence>Out->ChSequence);
	*/

	// Release all acknowledged outgoing queued bunches.
	bool DoClose = 0;
	while( OutRec && OutRec->ReceivedAck )
	{
		if (OutRec->bOpen)
		{
			bool OpenFinished = true;
			if (OutRec->bPartial)
			{
				// Partial open bunches: check that all open bunches have been ACKd before trashing them
				FOutBunch*	OpenBunch = OutRec;
				while (OpenBunch)
				{
					UE_LOG(LogNet, VeryVerbose, TEXT("   Channel %i open partials %d ackd %d final %d "), ChIndex, OpenBunch->PacketId, OpenBunch->ReceivedAck, OpenBunch->bPartialFinal);

					if (!OpenBunch->ReceivedAck)
					{
						OpenFinished = false;
						break;
					}
					if(OpenBunch->bPartialFinal)
					{
						break;
					}

					OpenBunch = OpenBunch->Next;
				}
			}
			if (OpenFinished)
			{
				UE_LOG(LogNet, VeryVerbose, TEXT("Channel %i is fully acked. PacketID: %d"), ChIndex, OutRec->PacketId );
				OpenAcked = 1;
			}
			else
			{
				// Don't delete this bunch yet until all open bunches are Ackd.
				break;
			}
		}

		DoClose = DoClose || !!OutRec->bClose;
		FOutBunch* Release = OutRec;
		OutRec = OutRec->Next;
		delete Release;
		NumOutRec--;
	}

	// If a close has been acknowledged in sequence, we're done.
	if( DoClose || (OpenTemporary && OpenAcked) )
	{
		UE_LOG(LogNetDormancy, Verbose, TEXT("Channel[%d] '%s' DoClose. Dormant: %d"), ChIndex, *Describe(), Dormant );		

		check(!OutRec);
		ConditionalCleanUp();
	}

}


int32 UChannel::MaxSendBytes()
{
	int32 ResultBits
	=	Connection->MaxPacket*8
	-	(Connection->Out.GetNumBits() ? 0 : MAX_PACKET_HEADER_BITS)
	-	Connection->Out.GetNumBits()
	-	MAX_PACKET_TRAILER_BITS
	-	MAX_BUNCH_HEADER_BITS;
	return FMath::Max( 0, ResultBits/8 );
}


void UChannel::Tick()
{
	checkSlow(Connection->Channels[ChIndex]==this);
	if (bPendingDormancy && ReadyForDormancy())
	{
		BecomeDormant();
	}
}


void UChannel::AssertInSequenced()
{
#if DO_CHECK
	// Verify that buffer is in order with no duplicates.
	for( FInBunch* In=InRec; In && In->Next; In=In->Next )
		check(In->Next->ChSequence>In->ChSequence);
#endif
}


bool UChannel::ReceivedSequencedBunch( FInBunch& Bunch )
{
	// Handle a regular bunch.
	if ( !Closing )
	{
		ReceivedBunch( Bunch );
	}

	// We have fully received the bunch, so process it.
	if( Bunch.bClose )
	{
		Dormant = Bunch.bDormant;

		// Handle a close-notify.
		if( InRec )
		{
			ensureMsgf(false, TEXT("Close Anomaly %i / %i"), Bunch.ChSequence, InRec->ChSequence );
		}
		UE_LOG(LogNetTraffic, Log, TEXT("      Channel %i got close-notify"), ChIndex );
		ConditionalCleanUp();
		return 1;
	}
	return 0;
}

void UChannel::ReceivedRawBunch( FInBunch & Bunch, bool & bOutSkipAck )
{
	// Immediately consume the NetGUID portion of this bunch, regardless if it is partial or reliable.
	if (Bunch.bHasGUIDs)
	{
		Cast<UPackageMapClient>(Connection->PackageMap)->ReceiveNetGUIDBunch(Bunch);

		if ( Bunch.IsError() )
		{
			UE_LOG( LogNetTraffic, Error, TEXT( "UChannel::ReceivedRawBunch: Bunch.IsError() after ReceiveNetGUIDBunch" ) );
			return;
		}
	}


	check(Connection->Channels[ChIndex]==this);

	if ( Bunch.bReliable && Bunch.ChSequence != Connection->InReliable[ChIndex] + 1 )
	{
		// If this bunch has a dependency on a previous unreceived bunch, buffer it.
		checkSlow(!Bunch.bOpen);

		// Verify that UConnection::ReceivedPacket has passed us a valid bunch.
		check(Bunch.ChSequence>Connection->InReliable[ChIndex]);

		// Find the place for this item, sorted in sequence.
		UE_LOG(LogNetTraffic, Log, TEXT("      Queuing bunch with unreceived dependency: %d / %d"), Bunch.ChSequence, Connection->InReliable[ChIndex]+1 );
		FInBunch** InPtr;
		for( InPtr=&InRec; *InPtr; InPtr=&(*InPtr)->Next )
		{
			if( Bunch.ChSequence==(*InPtr)->ChSequence )
			{
				// Already queued.
				return;
			}
			else if( Bunch.ChSequence<(*InPtr)->ChSequence )
			{
				// Stick before this one.
				break;
			}
		}
		FInBunch* New = new FInBunch(Bunch);
		New->Next     = *InPtr;
		*InPtr        = New;
		NumInRec++;

		if ( NumInRec >= RELIABLE_BUFFER )
		{
			Bunch.SetError();
			UE_LOG( LogNetTraffic, Error, TEXT( "UChannel::ReceivedRawBunch: Too many reliable messages queued up" ) );
			return;
		}

		checkSlow(NumInRec<=RELIABLE_BUFFER);
		//AssertInSequenced();
	}
	else
	{
		bool bDeleted = ReceivedNextBunch( Bunch, bOutSkipAck );

		if ( Bunch.IsError() )
		{
			UE_LOG( LogNetTraffic, Error, TEXT( "UChannel::ReceivedRawBunch: Bunch.IsError() after ReceivedNextBunch 1" ) );
			return;
		}

		if (bDeleted)
		{
			return;
		}
		
		// Dispatch any waiting bunches.
		while( InRec )
		{
			if( InRec->ChSequence!=Connection->InReliable[ChIndex]+1 )
				break;
			UE_LOG(LogNetTraffic, Log, TEXT("      Channel %d Unleashing queued bunch"), ChIndex );
			FInBunch* Release = InRec;
			InRec = InRec->Next;
			NumInRec--;
			
			// Just keep a local copy of the bSkipAck flag, since these have already been acked and it doesn't make sense on this context
			// Definitely want to warn when this happens, since it's really not possible
			bool bLocalSkipAck = false;

			bDeleted = ReceivedNextBunch( *Release, bLocalSkipAck );

			if ( bLocalSkipAck )
			{
				UE_LOG( LogNetTraffic, Warning, TEXT( "UChannel::ReceivedRawBunch: bLocalSkipAck == true for already acked packet" ) );
			}

			if ( Bunch.IsError() )
			{
				UE_LOG( LogNetTraffic, Error, TEXT( "UChannel::ReceivedRawBunch: Bunch.IsError() after ReceivedNextBunch 2" ) );
				return;
			}

			delete Release;
			if (bDeleted)
			{
				return;
			}
			//AssertInSequenced();
		}
	}
}

bool UChannel::ReceivedNextBunch( FInBunch & Bunch, bool & bOutSkipAck )
{
	// We received the next bunch. Basically at this point:
	//	-We know this is in order if reliable
	//	-We dont know if this is partial or not
	// If its not a partial bunch, of it completes a partial bunch, we can call ReceivedSequencedBunch to actually handle it
	
	// Note this bunch's retirement.
	if( Bunch.bReliable )
		Connection->InReliable[Bunch.ChIndex] = Bunch.ChSequence;

	FInBunch* HandleBunch = &Bunch;
	if (Bunch.bPartial)
	{
		HandleBunch = NULL;
		if (Bunch.bPartialInitial)
		{
			// Create new InPartialBunch if this is the initial bunch of a new sequence.

			if (InPartialBunch != NULL)
			{
				if (!InPartialBunch->bPartialFinal)
				{
					if ( InPartialBunch->bReliable )
					{
						check( !Bunch.bReliable );		// FIXME: Disconnect client in this case
						UE_LOG(LogNetPartialBunch, Log, TEXT( "Unreliable partial trying to destroy reliable partial 1") );
						bOutSkipAck = true;
						return false;
					}

					// We didn't complete the last partial bunch - this isn't fatal since they can be unreliable, but may want to log it.
					UE_LOG(LogNetPartialBunch, Verbose, TEXT("Incomplete partial bunch. Channel: %d ChSequence: %d"), InPartialBunch->ChIndex, InPartialBunch->ChSequence);
				}
				
				delete InPartialBunch;
				InPartialBunch = NULL;
			}

			InPartialBunch = new FInBunch(Bunch, false);
			if (Bunch.GetBitsLeft() > 0)
			{
				check( Bunch.GetBitsLeft() % 8 == 0); // Starting partial bunches should always be byte aligned.

				InPartialBunch->AppendDataFromChecked( Bunch.GetDataPosChecked(), Bunch.GetBitsLeft() );
				UE_LOG(LogNetPartialBunch, Verbose, TEXT("Received New partial bunch. Channel: %d ChSequence: %d. NumBits Total: %d. NumBits LefT: %d.  Reliable: %d"), InPartialBunch->ChIndex, InPartialBunch->ChSequence, InPartialBunch->GetNumBits(),  Bunch.GetBytesLeft(), Bunch.bReliable);
			}
			else
			{
				UE_LOG(LogNetPartialBunch, Verbose, TEXT("Received New partial bunch. It only contained NetGUIDs.  Channel: %d ChSequence: %d. Reliable: %d"), InPartialBunch->ChIndex, InPartialBunch->ChSequence, Bunch.bReliable);
			}
		}
		else
		{
			// Merge in next partial bunch to InPartialBunch if:
			//	-We have a valid InPartialBunch
			//	-The current InPartialBunch wasn't already complete
			//  -ChSequence is next in partial sequence
			//	-Reliability flag matches

			if (InPartialBunch && !InPartialBunch->bPartialFinal && InPartialBunch->ChSequence+1 == Bunch.ChSequence && InPartialBunch->bReliable == Bunch.bReliable)
			{
				// Merge.
				UE_LOG(LogNetPartialBunch, Verbose, TEXT("Merging Partial Bunch: %d Bytes"), Bunch.GetBytesLeft() );

				if (Bunch.GetBitsLeft() > 0)
				{
					InPartialBunch->AppendDataFromChecked( Bunch.GetDataPosChecked(), Bunch.GetBitsLeft() );
				}

				// Only the final partial bunch should ever be non byte aligned. This is enforced during partial bunch creation
				// This is to ensure fast copies/appending of partial bunches. The final partial bunch may be non byte aligned.
				check( Bunch.bPartialFinal || Bunch.GetBitsLeft() % 8 == 0);

				InPartialBunch->ChSequence++;

				if (Bunch.bPartialFinal)
				{
					if (UE_LOG_ACTIVE(LogNetPartialBunch,Verbose)) // Don't want to call appMemcrc unless we need to
					{
						UE_LOG(LogNetPartialBunch, Verbose, TEXT("Completed Partial Bunch: Channel: %d ChSequence: %d. Num: %d Rel: %d CRC 0x%X"), InPartialBunch->ChIndex, InPartialBunch->ChSequence, InPartialBunch->GetNumBits(), Bunch.bReliable, FCrc::MemCrc_DEPRECATED(InPartialBunch->GetData(), InPartialBunch->GetNumBytes()));
					}

					HandleBunch = InPartialBunch;
					InPartialBunch->bPartialFinal = true;
					InPartialBunch->bClose = Bunch.bClose;
					InPartialBunch->bDormant = Bunch.bDormant;
					if (InPartialBunch->bOpen)
					{
						UE_LOG(LogNetPartialBunch, Verbose, TEXT("Channel: %d is now Open! [%d] - [%d]"), ChIndex, OpenPacketId.First, OpenPacketId.Last );

						// If the open bunch was partial, set the ending range to the final partial bunch id
						OpenPacketId.First = InPartialBunch->PacketId;
						OpenPacketId.Last = Bunch.PacketId;
						OpenAcked = true;
					}
				}
				else
				{
					if (UE_LOG_ACTIVE(LogNetPartialBunch,Verbose)) // Don't want to call appMemcrc unless we need to
					{
						UE_LOG(LogNetPartialBunch, Verbose, TEXT("Received Partial Bunch: Channel: %d ChSequence: %d. Num: %d Rel: %d CRC 0x%X"), InPartialBunch->ChIndex, InPartialBunch->ChSequence, InPartialBunch->GetNumBits(), Bunch.bReliable, FCrc::MemCrc_DEPRECATED(InPartialBunch->GetData(), InPartialBunch->GetNumBytes()));
					}
				}
			}
			else
			{
				// Merge problem - delete InPartialBunch. This is mainly so that in the unlikely chance that ChSequence wraps around, we wont merge two completely separate partial bunches.
				bOutSkipAck = true;

				if ( InPartialBunch && InPartialBunch->bReliable )
				{
					check( !Bunch.bReliable );		// FIXME: Disconnect client in this case
					UE_LOG( LogNetPartialBunch, Log, TEXT( "Unreliable partial trying to destroy reliable partial 2" ) );
					bOutSkipAck = true;
					return false;
				}

				if (InPartialBunch)
				{
					delete InPartialBunch;
					InPartialBunch = NULL;
				}

				if (UE_LOG_ACTIVE(LogNetPartialBunch,Verbose)) // Don't want to call appMemcrc unless we need to
				{
					UE_LOG(LogNetPartialBunch, Verbose, TEXT("Received Partial Bunch Out of Sequence: Channel: %d ChSequence: %d/%d. Num: %d Rel: %d CRC 0x%X"), InPartialBunch->ChIndex, InPartialBunch->ChSequence, Bunch.ChSequence, InPartialBunch->GetNumBits(), Bunch.bReliable, FCrc::MemCrc_DEPRECATED(InPartialBunch->GetData(), InPartialBunch->GetNumBytes()));
				}
			}
		}

		// Fairly large number, and probably a bad idea to even have a bunch this size, but want to be safe for now and not throw out legitimate data
		static const int32 MAX_CONSTRUCTED_PARTIAL_SIZE_IN_BYTES = 1024 * 64;		

		if ( InPartialBunch != NULL && InPartialBunch->GetNumBytes() > MAX_CONSTRUCTED_PARTIAL_SIZE_IN_BYTES )
		{
			UE_LOG( LogNetPartialBunch, Error, TEXT( "Final partial bunch too large" ) );
			Bunch.SetError();
			return false;
		}
	}
	
	if (HandleBunch!=NULL)
	{
		// Receive it in sequence.
		return ReceivedSequencedBunch( *HandleBunch );
	}

	return false;
}

FPacketIdRange UChannel::SendBunch( FOutBunch* Bunch, bool Merge )
{
	check(!Closing);
	check(Connection->Channels[ChIndex]==this);
	check(!Bunch->IsError());

	// Set bunch flags.
	if( OpenPacketId.First==INDEX_NONE && OpenedLocally )
	{
		Bunch->bOpen = 1;
		OpenTemporary = !Bunch->bReliable;
	}

	// If channel was opened temporarily, we are never allowed to send reliable packets on it.
	check(!OpenTemporary || !Bunch->bReliable);

	// This is the max number of bits we can have in a single bunch
	static const int64 MAX_SINGLE_BUNCH_SIZE_BITS  = Connection->MaxPacket*8-MAX_BUNCH_HEADER_BITS-MAX_PACKET_TRAILER_BITS-MAX_PACKET_HEADER_BITS;

	// Max bytes we'll put in a partial bunch
	static const int64 MAX_SINGLE_BUNCH_SIZE_BYTES = MAX_SINGLE_BUNCH_SIZE_BITS / 8;

	// Max bits will put in a partial bunch (byte aligned, we dont want to deal with partial bytes in the partial bunches)
	static const int64 MAX_PARTIAL_BUNCH_SIZE_BITS = MAX_SINGLE_BUNCH_SIZE_BYTES * 8;

	TArray<FOutBunch *> OutgoingBunches;

	// Let the package map add any outgoing bunches it needs to send
	if (Cast<UPackageMapClient>(Connection->PackageMap)->AppendExportBunches(OutgoingBunches))
	{
		// Dont merge if we have multiple bunches to send.
		Merge = false;
	}

	//-----------------------------------------------------
	// Contemplate merging.
	//-----------------------------------------------------
	int32 PreExistingBits = 0;
	FOutBunch* OutBunch = NULL;
	if
	(	Merge
	&&	Connection->LastOut.ChIndex == Bunch->ChIndex
	&&	Connection->AllowMerge
	&&	Connection->LastEnd.GetNumBits()
	&&	Connection->LastEnd.GetNumBits()==Connection->Out.GetNumBits()
	&&	Connection->Out.GetNumBytes()+Bunch->GetNumBytes()+(MAX_BUNCH_HEADER_BITS+MAX_PACKET_TRAILER_BITS+7)/8<=Connection->MaxPacket )
	{
		// Merge.
		check(!Connection->LastOut.IsError());
		PreExistingBits = Connection->LastOut.GetNumBits();
		Connection->LastOut.SerializeBits( Bunch->GetData(), Bunch->GetNumBits() );
		Connection->LastOut.bReliable |= Bunch->bReliable;
		Connection->LastOut.bOpen     |= Bunch->bOpen;
		Connection->LastOut.bClose    |= Bunch->bClose;
		OutBunch                       = Connection->LastOutBunch;
		Bunch                          = &Connection->LastOut;
		check(!Bunch->IsError());
		Connection->LastStart.Pop( Connection->Out );
		Connection->Driver->OutBunches--;
	}

	//-----------------------------------------------------
	// Possibly split large bunch into list of smaller partial bunches
	//-----------------------------------------------------
	if( Bunch->GetNumBits() > MAX_SINGLE_BUNCH_SIZE_BITS )
	{
		uint8 *data = Bunch->GetData();
		int64 bitsLeft = Bunch->GetNumBits();
		Merge = false;
		
		while(bitsLeft > 0)
		{
			FOutBunch * PartialBunch = new FOutBunch(this, false);
			int64 bitsThisBunch = FMath::Min<int64>(bitsLeft, MAX_PARTIAL_BUNCH_SIZE_BITS);
			PartialBunch->SerializeBits(data, bitsThisBunch);
			OutgoingBunches.Add(PartialBunch);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			PartialBunch->DebugString = FString::Printf(TEXT("Partial[%d]: %s"), OutgoingBunches.Num(), *Bunch->DebugString);
#endif
		
			bitsLeft -= bitsThisBunch;
			data += (bitsThisBunch >> 3);

			UE_LOG(LogNetPartialBunch, Log, TEXT("	Making partial bunch from content bunch. bitsThisBunch: %d bitsLeft: %d"), bitsThisBunch, bitsLeft );
			
			ensure(bitsLeft == 0 || bitsThisBunch % 8 == 0); // Byte aligned or it was the last bunch
		}
	}
	
	else
	{
		OutgoingBunches.Add(Bunch);
	}

	//-----------------------------------------------------
	// Send all the bunches we need to
	//	Note: this is done all at once. We could queue this up somewhere else before sending to Out.
	//-----------------------------------------------------
	FPacketIdRange PacketIdRange;

	if (Bunch->bReliable && (NumOutRec + OutgoingBunches.Num() >= RELIABLE_BUFFER + Bunch->bClose))
	{
		UE_LOG(LogNetPartialBunch, Warning, TEXT("Channel[%d] - %s. Reliable partial bunch overflows reliable buffer!"), ChIndex, *Describe() );
		UE_LOG(LogNetPartialBunch, Warning, TEXT("   Num OutgoingBunches: %d. NumOutRec: %d"), OutgoingBunches.Num(), NumOutRec );
		PrintReliableBunchBuffer();

		// Bail out, we can't recover from this (without increasing RELIABLE_BUFFER)
		FString ErrorMsg = NSLOCTEXT("NetworkErrors", "ClientReliableBufferOverflow", "Outgoing reliable buffer overflow").ToString();
		FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
		Connection->FlushNet(true);
		Connection->Close();
	
		return PacketIdRange;
	}

	UE_CLOG((OutgoingBunches.Num() > 1), LogNetPartialBunch, Log, TEXT("Sending %d Bunches. Channel: %d "), OutgoingBunches.Num(), Bunch->ChIndex);
	for( int32 PartialNum = 0; PartialNum < OutgoingBunches.Num(); ++PartialNum)
	{
		FOutBunch * NextBunch = OutgoingBunches[PartialNum];

		NextBunch->bReliable = Bunch->bReliable;
		NextBunch->bOpen = Bunch->bOpen;
		NextBunch->bClose = Bunch->bClose;
		NextBunch->bDormant = Bunch->bDormant;
		NextBunch->ChIndex = Bunch->ChIndex;
		NextBunch->ChType = Bunch->ChType;

		if (OutgoingBunches.Num() > 1)
		{
			NextBunch->bPartial = 1;
			NextBunch->bPartialInitial = (PartialNum == 0 ? 1: 0);
			NextBunch->bPartialFinal = (PartialNum == OutgoingBunches.Num() - 1 ? 1: 0);
			NextBunch->bOpen &= (PartialNum == 0);											// Only the first bunch should have the bOpen bit set
			NextBunch->bClose = (Bunch->bClose && (OutgoingBunches.Num()-1 == PartialNum)); // Only last bunch should have bClose bit set
		}

		FOutBunch *ThisOutBunch = PrepBunch(NextBunch, OutBunch, Merge); // This handles queueing reliable bunches into the ack list
			
		if (ThisOutBunch->bPartial && !ThisOutBunch->bReliable)
		{
			// If we are reliable, then partial bunches just use the reliable sequences
			// if not reliable, we hijack ChSequence and use Connection->PartialPackedId to sequence these packets
			ThisOutBunch->ChSequence = Connection->PartialPackedId++;
		}

		if (UE_LOG_ACTIVE(LogNetPartialBunch,Verbose) && (OutgoingBunches.Num() > 1)) // Don't want to call appMemcrc unless we need to
		{
			UE_LOG(LogNetPartialBunch, Verbose, TEXT("	Bunch[%d]: Bytes: %d Bits: %d ChSequence: %d 0x%X"), PartialNum, ThisOutBunch->GetNumBytes(), ThisOutBunch->GetNumBits(), ThisOutBunch->ChSequence, FCrc::MemCrc_DEPRECATED(ThisOutBunch->GetData(), ThisOutBunch->GetNumBytes()));
		}

		NETWORK_PROFILER(GNetworkProfiler.TrackSendBunch(ThisOutBunch,ThisOutBunch->GetNumBits()-PreExistingBits));

		// Update Packet Range
		int32 PacketId = SendRawBunch(ThisOutBunch, Merge);
		if (PartialNum == 0)
		{
			PacketIdRange = FPacketIdRange(PacketId);
		}
		else
		{
			PacketIdRange.Last = PacketId;
		}

		// Update channel sequence count.
		Connection->LastOut = *ThisOutBunch;
		Connection->LastEnd	= FBitWriterMark(Connection->Out);
	}

	// Update open range if necessary
	if (Bunch->bOpen)
	{
		OpenPacketId = PacketIdRange;		
	}


	// Destroy outgoing bunches now that they are sent, except the one that was passed into ::SendBunch
	//	This is because the one passed in ::SendBunch is the responsibility of the caller, the other bunches in OutgoingBunches
	//	were either allocated in this function for partial bunches, or taken from the package map, which expects us to destroy them.
	for (auto It = OutgoingBunches.CreateIterator(); It; ++It)
	{
		FOutBunch *DeleteBunch = *It;
		if (DeleteBunch != Bunch)
			delete DeleteBunch;
	}

	return PacketIdRange;
}

/** This returns a pointer to Bunch, but it may either be a direct pointer, or a pointer to a copied instance of it */

// OUtbunch is a bunch that was new'd by the network system or NULL. It should never be one created on the stack
FOutBunch* UChannel::PrepBunch(FOutBunch* Bunch, FOutBunch* OutBunch, bool Merge)
{
	// Find outgoing bunch index.
	if( Bunch->bReliable )
	{
		// Find spot, which was guaranteed available by FOutBunch constructor.
		if( OutBunch==NULL )
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (!(NumOutRec<RELIABLE_BUFFER-1+Bunch->bClose))
			{
				UE_LOG(LogNetTraffic, Warning, TEXT("Reliable buffer overflow! %s"), *Describe());
				PrintReliableBunchBuffer();
			}
#else
			check(NumOutRec<RELIABLE_BUFFER-1+Bunch->bClose);
#endif

			Bunch->Next	= NULL;
			Bunch->ChSequence = ++Connection->OutReliable[ChIndex];
			NumOutRec++;
			OutBunch = new FOutBunch(*Bunch);
			FOutBunch** OutLink = &OutRec;
			while(*OutLink) // This was rewritten from a single-line for loop due to compiler complaining about empty body for loops (-Wempty-body)
			{
				OutLink=&(*OutLink)->Next;
			}
			*OutLink = OutBunch;
		}
		else
		{
			Bunch->Next = OutBunch->Next;
			*OutBunch = *Bunch;
		}
		Connection->LastOutBunch = OutBunch;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.Reliable.Debug"));
		if (CVar)
		{
			if (CVar->GetValueOnGameThread() == 1)
			{
				UE_LOG(LogNetTraffic, Warning, TEXT("%s. Reliable: %s"), *Describe(), *Bunch->DebugString);
			}
			if (CVar->GetValueOnGameThread() == 2)
			{
				UE_LOG(LogNetTraffic, Warning, TEXT("%s. Reliable: %s"), *Describe(), *Bunch->DebugString);
				PrintReliableBunchBuffer();
				UE_LOG(LogNetTraffic, Warning, TEXT(""));
			}
		}
#endif

	}
	else
	{
		OutBunch = Bunch;
		Connection->LastOutBunch = NULL;//warning: Complex code, don't mess with this!
	}

	return OutBunch;
}

int32 UChannel::SendRawBunch(FOutBunch* OutBunch, bool Merge)
{
	// Send the raw bunch.
	OutBunch->ReceivedAck = 0;
	int32 PacketId = Connection->SendRawBunch(*OutBunch, Merge);
	if( OpenPacketId.First==INDEX_NONE && OpenedLocally )
		OpenPacketId = FPacketIdRange(PacketId);
	if( OutBunch->bClose )
		SetClosingFlag();

	return PacketId;
}

FString UChannel::Describe()
{
	return FString(TEXT("State=")) + (Closing ? TEXT("closing") : TEXT("open") );
}


int32 UChannel::IsNetReady( bool Saturate )
{
	// If saturation allowed, ignore queued byte count.
	if( NumOutRec>=RELIABLE_BUFFER-1 )
	{
		return 0;
	}
	return Connection->IsNetReady( Saturate );
}


bool UChannel::IsKnownChannelType( int32 Type )
{
	return Type>=0 && Type<CHTYPE_MAX && ChannelClasses[Type];
}


void UChannel::ReceivedNak( int32 NakPacketId )
{
	for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
	{
		// Retransmit reliable bunches in the lost packet.
		if( Out->PacketId==NakPacketId && !Out->ReceivedAck )
		{
			check(Out->bReliable);
			UE_LOG(LogNetTraffic, Log, TEXT("      Channel %i nak); resending %i..."), Out->ChIndex, Out->ChSequence );
			Connection->SendRawBunch( *Out, 0 );
		}
	}
}

void UChannel::PrintReliableBunchBuffer()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
	{
		UE_LOG(LogNetTraffic, Warning, TEXT("Out: %s"), *Out->DebugString );
	}
	UE_LOG(LogNetTraffic, Warning, TEXT("-------------------------\n"));
#endif
}


UClass* UChannel::ChannelClasses[CHTYPE_MAX]={0,0,0,0,0,0,0,0};

/*-----------------------------------------------------------------------------
	UControlChannel implementation.
-----------------------------------------------------------------------------*/

const TCHAR* FNetControlMessageInfo::Names[256];

// control channel message implementation
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Hello);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Welcome);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Upgrade);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Challenge);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Netspeed);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Login);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Failure);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Uses);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Have);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Join);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(JoinSplit);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Skip);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Abort);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(Unload);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(PCSwap);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(ActorChannelFailure);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(DebugText);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(BeaconWelcome);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(BeaconJoin);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(BeaconAssignGUID);
IMPLEMENT_CONTROL_CHANNEL_MESSAGE(BeaconNetGUIDAck);

void UControlChannel::Init( UNetConnection* InConnection, int32 InChannelIndex, bool InOpenedLocally )
{
	Super::Init( InConnection, InChannelIndex, InOpenedLocally );

	// If we are opened as a server connection, do the endian checking
	// The client assumes that the data will always have the correct byte order
	if (!InOpenedLocally)
	{
		// Mark this channel as needing endianess determination
		bNeedsEndianInspection = true;
	}
}


bool UControlChannel::CheckEndianess(FInBunch& Bunch)
{
	// Assume the packet is bogus and the connection needs closing
	bool bConnectionOk = false;
	// Get pointers to the raw packet data
	const uint8* HelloMessage = Bunch.GetData();
	// Check for a packet that is big enough to look at (message ID (1 byte) + platform identifier (1 byte))
	if (Bunch.GetNumBytes() >= 2)
	{
		if (HelloMessage[0] == NMT_Hello)
		{
			// Get platform id
			uint8 OtherPlatformIsLittle = HelloMessage[1];
			checkSlow(OtherPlatformIsLittle == !!OtherPlatformIsLittle); // should just be zero or one, we use check slow because we don't want to crash in the wild if this is a bad value.
			uint8 IsLittleEndian = uint8(PLATFORM_LITTLE_ENDIAN);
			check(IsLittleEndian == !!IsLittleEndian); // should only be one or zero

			UE_LOG(LogNet, Log, TEXT("Remote platform little endian=%d"), int32(OtherPlatformIsLittle));
			UE_LOG(LogNet, Log, TEXT("This platform little endian=%d"), int32(IsLittleEndian));
			// Check whether the other platform needs byte swapping by
			// using the value sent in the packet. Note: we still validate it
			if (OtherPlatformIsLittle ^ IsLittleEndian)
			{
				// Client has opposite endianess so swap this bunch
				// and mark the connection as needing byte swapping
				Bunch.SetByteSwapping(true);
				Connection->bNeedsByteSwapping = true;
			}
			else
			{
				// Disable all swapping
				Bunch.SetByteSwapping(false);
				Connection->bNeedsByteSwapping = false;
			}
			// We parsed everything so keep the connection open
			bConnectionOk = true;
			bNeedsEndianInspection = false;
		}
	}
	return bConnectionOk;
}


void UControlChannel::ReceivedBunch( FInBunch& Bunch )
{
	check(!Closing);
	// If this is a new client connection inspect the raw packet for endianess
	if (bNeedsEndianInspection && !CheckEndianess(Bunch))
	{
		// Send close bunch and shutdown this connection
		UE_LOG(LogNet, Log, TEXT("NetConnection::Close() [%s] from CheckEndianess()"), Connection->Driver ? *Connection->Driver->NetDriverName.ToString() : TEXT("NULL"));
		Connection->Close();
		return;
	}

	// Process the packet
	while (!Bunch.AtEnd() && Connection != NULL && Connection->State != USOCK_Closed) // if the connection got closed, we don't care about the rest
	{
		uint8 MessageType;
		Bunch << MessageType;
		if (Bunch.IsError())
		{
			break;
		}
		int32 Pos = Bunch.GetPosBits();
		
		// we handle Actor channel failure notifications ourselves
		if (MessageType == NMT_ActorChannelFailure)
		{
			if (Connection->Driver->ServerConnection == NULL)
			{
				UE_LOG(LogNet, Log, TEXT("Server connection received: %s"), FNetControlMessageInfo::GetName(MessageType));
				int32 ChannelIndex;
				FNetControlMessage<NMT_ActorChannelFailure>::Receive(Bunch, ChannelIndex);
				if (ChannelIndex < ARRAY_COUNT(Connection->Channels))
				{
					UActorChannel* ActorChan = Cast<UActorChannel>(Connection->Channels[ChannelIndex]);
					if (ActorChan != NULL && ActorChan->Actor != NULL)
					{
						// if the client failed to initialize the PlayerController channel, the connection is broken
						if (ActorChan->Actor == Connection->PlayerController)
						{
							UE_LOG(LogNet, Log, TEXT("NetConnection::Close() [%s] from failed to initialize the PlayerController channel"), Connection->Driver ? *Connection->Driver->NetDriverName.ToString() : TEXT("NULL"));
							Connection->Close();
						}
						else if (Connection->PlayerController != NULL)
						{
							Connection->PlayerController->NotifyActorChannelFailure(ActorChan);
						}
					}
				}
			}
		}
		else
		{
			// Process control message on client/server connection
			Connection->Driver->Notify->NotifyControlMessage(Connection, MessageType, Bunch);
		}

		// if the message was not handled, eat it ourselves
		if (Pos == Bunch.GetPosBits() && !Bunch.IsError())
		{
			switch (MessageType)
			{
				case NMT_Hello:
					FNetControlMessage<NMT_Hello>::Discard(Bunch);
					break;
				case NMT_Welcome:
					FNetControlMessage<NMT_Welcome>::Discard(Bunch);
					break;
				case NMT_Upgrade:
					FNetControlMessage<NMT_Upgrade>::Discard(Bunch);
					break;
				case NMT_Challenge:
					FNetControlMessage<NMT_Challenge>::Discard(Bunch);
					break;
				case NMT_Netspeed:
					FNetControlMessage<NMT_Netspeed>::Discard(Bunch);
					break;
				case NMT_Login:
					FNetControlMessage<NMT_Login>::Discard(Bunch);
					break;
				case NMT_Failure:
					FNetControlMessage<NMT_Failure>::Discard(Bunch);
					break;
				case NMT_Uses:
					FNetControlMessage<NMT_Uses>::Discard(Bunch);
					break;
				case NMT_Have:
					FNetControlMessage<NMT_Have>::Discard(Bunch);
					break;
				case NMT_Join:
					//FNetControlMessage<NMT_Join>::Discard(Bunch);
					break;
				case NMT_JoinSplit:
					FNetControlMessage<NMT_JoinSplit>::Discard(Bunch);
					break;
				case NMT_Skip:
					FNetControlMessage<NMT_Skip>::Discard(Bunch);
					break;
				case NMT_Abort:
					FNetControlMessage<NMT_Abort>::Discard(Bunch);
					break;
				case NMT_Unload:
					FNetControlMessage<NMT_Unload>::Discard(Bunch);
					break;
				case NMT_PCSwap:
					FNetControlMessage<NMT_PCSwap>::Discard(Bunch);
					break;
				case NMT_ActorChannelFailure:
					FNetControlMessage<NMT_ActorChannelFailure>::Discard(Bunch);
					break;
				case NMT_DebugText:
					FNetControlMessage<NMT_DebugText>::Discard(Bunch);
					break;
				case NMT_NetGUIDAssign:
					FNetControlMessage<NMT_NetGUIDAssign>::Discard(Bunch);
				case NMT_BeaconWelcome:
					//FNetControlMessage<NMT_BeaconWelcome>::Discard(Bunch);
					break;
				case NMT_BeaconJoin:
					FNetControlMessage<NMT_BeaconJoin>::Discard(Bunch);
					break;
				case NMT_BeaconAssignGUID:
					FNetControlMessage<NMT_BeaconAssignGUID>::Discard(Bunch);
					break;
				case NMT_BeaconNetGUIDAck:
					FNetControlMessage<NMT_BeaconNetGUIDAck>::Discard(Bunch);
					break;
				default:
					check(!FNetControlMessageInfo::IsRegistered(MessageType)); // if this fails, a case is missing above for an implemented message type

					UE_LOG(LogNet, Error, TEXT("Received unknown control channel message"));
					ensureMsgf(false, TEXT("Failed to read control channel message %i"), int32(MessageType));
					Connection->Close();
					return;
			}
		}
		if ( Bunch.IsError() )
		{
			UE_LOG( LogNet, Error, TEXT( "Failed to read control channel message '%s'" ), FNetControlMessageInfo::GetName( MessageType ) );
			break;
		}
	}

	if ( Bunch.IsError() )
	{
		UE_LOG( LogNet, Error, TEXT( "UControlChannel::ReceivedBunch: Failed to read control channel message" ) );
		Connection->Close();
	}
}

void UControlChannel::QueueMessage(const FOutBunch* Bunch)
{
	if (QueuedMessages.Num() >= MAX_QUEUED_CONTROL_MESSAGES)
	{
		// we're out of room in our extra buffer as well, so kill the connection
		UE_LOG(LogNet, Log, TEXT("Overflowed control channel message queue, disconnecting client"));
		// intentionally directly setting State as the messaging in Close() is not going to work in this case
		Connection->State = USOCK_Closed;
	}
	else
	{
		int32 Index = QueuedMessages.AddZeroed();
		QueuedMessages[Index].AddUninitialized(Bunch->GetNumBytes());
		FMemory::Memcpy(QueuedMessages[Index].GetTypedData(), Bunch->GetData(), Bunch->GetNumBytes());
	}
}

FPacketIdRange UControlChannel::SendBunch(FOutBunch* Bunch, bool Merge)
{
	// if we already have queued messages, we need to queue subsequent ones to guarantee proper ordering
	if (QueuedMessages.Num() > 0 || NumOutRec >= RELIABLE_BUFFER - 1 + Bunch->bClose)
	{
		QueueMessage(Bunch);
		return FPacketIdRange(INDEX_NONE);
	}
	else
	{
		if (!Bunch->IsError())
		{
			return Super::SendBunch(Bunch, Merge);
		}
		else
		{
			// an error here most likely indicates an unfixable error, such as the text using more than the maximum packet size
			// so there is no point in queueing it as it will just fail again
			UE_LOG(LogNet, Error, TEXT("Control channel bunch overflowed"));
			ensureMsgf(false, TEXT("Control channel bunch overflowed"));
			Connection->Close();
			return FPacketIdRange(INDEX_NONE);
		}
	}
}

void UControlChannel::Tick()
{
	Super::Tick();

	if( !OpenAcked )
	{
		int32 Count = 0;
		for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
			if( !Out->ReceivedAck )
				Count++;
		if ( Count > 8 )
			return;
		// Resend any pending packets if we didn't get the appropriate acks.
		for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
		{
			if( !Out->ReceivedAck )
			{
				float Wait = Connection->Driver->Time-Out->Time;
				checkSlow(Wait>=0.f);
				if( Wait>1.f )
				{
					UE_LOG(LogNetTraffic, Log, TEXT("Channel %i ack timeout); resending %i..."), ChIndex, Out->ChSequence );
					check(Out->bReliable);
					Connection->SendRawBunch( *Out, 0 );
				}
			}
		}
	}
	else
	{
		// attempt to send queued messages
		while (QueuedMessages.Num() > 0 && !Closing)
		{
			FControlChannelOutBunch Bunch(this, 0);
			if (Bunch.IsError())
			{
				break;
			}
			else
			{
				Bunch.bReliable = 1;
				Bunch.Serialize(QueuedMessages[0].GetData(), QueuedMessages[0].Num());
				if (!Bunch.IsError())
				{
					Super::SendBunch(&Bunch, 1);
					QueuedMessages.RemoveAt(0, 1);
				}
				else
				{
					// an error here most likely indicates an unfixable error, such as the text using more than the maximum packet size
					// so there is no point in queueing it as it will just fail again
					ensureMsgf(false, TEXT("Control channel bunch overflowed"));
					UE_LOG(LogNet, Error, TEXT("Control channel bunch overflowed"));
					Connection->Close();
					break;
				}
			}
		}
	}
}


FString UControlChannel::Describe()
{
	return FString(TEXT("Text ")) + UChannel::Describe();
}

/*-----------------------------------------------------------------------------
	UActorChannel.
-----------------------------------------------------------------------------*/

void UActorChannel::Init( UNetConnection* InConnection, int32 InChannelIndex, bool InOpenedLocally )
{
	Super::Init( InConnection, InChannelIndex, InOpenedLocally );

	RelevantTime	= Connection->Driver->Time;
	LastUpdateTime	= Connection->Driver->Time - Connection->Driver->SpawnPrioritySeconds;
	bActorMustStayDirty = false;
	bActorStillInitial = false;
}

void UActorChannel::SetClosingFlag()
{
	if( Actor )
		Connection->ActorChannels.Remove( Actor );
	UChannel::SetClosingFlag();
}

void UActorChannel::Close()
{
	UChannel::Close();
	if (Actor != NULL)
	{
		UE_LOG(LogNetTraffic, Log, TEXT("Close[%d]: Actor: %s ActorClass: %s. %s"), ChIndex, Actor ? *Actor->GetPathName() : TEXT("NULL"), ActorClass ? *ActorClass->GetName() : TEXT("NULL"), *GetName() );

		bool bKeepReplicators = false;		// If we keep replicators around, we can use them to determine if the actor changed since it went dormant

		if ( Dormant )
		{
			check( Actor->NetDormancy > DORM_Awake ); // Dormancy should have been canceled if game code changed NetDormancy
			Connection->DormantActors.Add(Actor);

			// Validation checking
			static const auto ValidateCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.DormancyValidate"));
			if ( ValidateCVar && ValidateCVar->GetValueOnGameThread() > 0 )
			{
				bKeepReplicators = true;		// We need to keep the replicators around so we can use
			}
		}

		// SetClosingFlag() might have already done this, but we need to make sure as that won't get called if the connection itself has already been closed
		Connection->ActorChannels.Remove( Actor );

		Actor = NULL;
		CleanupReplicators( bKeepReplicators );
	}
}

void UActorChannel::CleanupReplicators( const bool bKeepReplicators )
{
	// Cleanup or save replicators
	for ( auto CompIt = ReplicationMap.CreateIterator(); CompIt; ++CompIt )
	{
		if ( bKeepReplicators )
		{
			// If we want to keep the replication state of the actor/sub-objects around, transfer ownership to the connection
			// This way, if this actor opens another channel on this connection, we can reclaim or use this replicator to compare state, etc.
			// For example, we may want to see if any state changed since the actor went dormant, and is now active again. 
			check( Connection->DormantReplicatorMap.Find( CompIt.Value()->GetObject() ) == NULL );
			Connection->DormantReplicatorMap.Add( CompIt.Value()->GetObject(), CompIt.Value() );
			CompIt.Value()->StopReplicating( this );		// Stop replicating on this channel
		}
		else
		{
			CompIt.Value()->CleanUp();
		}
	}

	ReplicationMap.Empty();

	ActorReplicator = NULL;
	ActorClass		= NULL;
}

void UActorChannel::CleanUp()
{
	const bool IsServer = (Connection->Driver->ServerConnection == NULL);

	// Remove from hash and stuff.
	SetClosingFlag();

	CleanupReplicators();

	// If we're the client, destroy this actor.
	if (!IsServer)
	{
		check(Actor == NULL || Actor->IsValidLowLevel());
		checkSlow(Connection != NULL);
		checkSlow(Connection->IsValidLowLevel());
		checkSlow(Connection->Driver != NULL);
		checkSlow(Connection->Driver->IsValidLowLevel());
		if (Actor != NULL)
		{
			if (Actor->bTearOff)
			{
				Actor->Role = ROLE_Authority;
				Actor->SetReplicates(false);
			}
			else if (Dormant)
			{
				Connection->DormantActors.Add(Actor);
			}
			else if (!Actor->bNetTemporary && Actor->GetWorld() != NULL && !GIsRequestingExit)
			{
				UE_LOG(LogNetDormancy, Verbose, TEXT("Channel[%d] '%s' Destroying Actor."), ChIndex, *Describe() );
				Actor->Destroy( true );
			}
		}
	}
	else if (Actor && !OpenAcked)
	{
		// Resend temporary actors if nak'd.
		Connection->SentTemporaries.Remove(Actor);
	}

	Super::CleanUp();
}

void UActorChannel::ReceivedNak( int32 NakPacketId )
{
	UChannel::ReceivedNak(NakPacketId);	
	for (auto CompIt = ReplicationMap.CreateIterator(); CompIt; ++CompIt)
	{
		CompIt.Value()->ReceivedNak(NakPacketId);
	}

	// Reset any subobject RepKeys that were sent on this packetId
	FPacketRepKeyInfo * Info = SubobjectNakMap.Find(NakPacketId % SubobjectRepKeyBufferSize);
	if (Info)
	{
		if (Info->PacketID == NakPacketId)
		{
			UE_LOG(LogNetTraffic, Verbose, TEXT("ActorChannel[%d]: Reseting object keys due to Nak: %d"), ChIndex, NakPacketId);
			for (auto It = Info->ObjKeys.CreateIterator(); It; ++It)
			{
				SubobjectRepKeyMap.FindOrAdd(*It) = INDEX_NONE;
				UE_LOG(LogNetTraffic, Verbose, TEXT("    %d"), *It);
			}
		}
	}
}

void UActorChannel::SetChannelActor( AActor* InActor )
{
	check(!Closing);
	check(Actor==NULL);

	// Set stuff.
	Actor                      = InActor;
	ActorClass                 = Actor->GetClass();
	FClassNetCache* ClassCache = Connection->PackageMap->GetClassNetCache( ActorClass );

	UE_LOG(LogNetTraffic, VeryVerbose, TEXT("SetChannelActor[%d]: Actor: %s ActorClass: %s. %s"), ChIndex, Actor ? *Actor->GetPathName() : TEXT("NULL"), ActorClass ? *ActorClass->GetName() : TEXT("NULL"), *GetName() );

	if ( Connection->PendingOutRec[ChIndex] > 0 )
	{
		// send empty reliable bunches to synchronize both sides
		// UE_LOG(LogNetTraffic, Log, TEXT("%i Actor %s WILL BE sending %i vs first %i"), ChIndex, *Actor->GetName(), Connection->PendingOutRec[ChIndex],Connection->OutReliable[ChIndex]);
		int32 RealOutReliable = Connection->OutReliable[ChIndex];
		Connection->OutReliable[ChIndex] = Connection->PendingOutRec[ChIndex] - 1;
		while ( Connection->PendingOutRec[ChIndex] <= RealOutReliable )
		{
			// UE_LOG(LogNetTraffic, Log, TEXT("%i SYNCHRONIZING by sending %i"), ChIndex, Connection->PendingOutRec[ChIndex]);

			FOutBunch Bunch( this, 0 );
			if( !Bunch.IsError() )
			{
				Bunch.bReliable = true;
				SendBunch( &Bunch, 0 );
				Connection->PendingOutRec[ChIndex]++;
			}
		}

		Connection->OutReliable[ChIndex] = RealOutReliable;
		Connection->PendingOutRec[ChIndex] = 0;
	}


	// Add to map.
	Connection->ActorChannels.Add( Actor, this );

	check( !ReplicationMap.Contains( Actor ) );

	// Create the actor replicator, and store a quick access pointer to it
	ActorReplicator = &FindOrCreateReplicator( Actor );

	// Remove from connection's dormancy lists
	Connection->DormantActors.Remove( InActor );
	Connection->RecentlyDormantActors.Remove( InActor );
}

void UActorChannel::SetChannelActorForDestroy( FActorDestructionInfo *DestructInfo )
{
	check(Connection->Channels[ChIndex]==this);
	if
	(	!Closing
	&&	(Connection->State==USOCK_Open || Connection->State==USOCK_Pending) )
	{
		// Send a close notify, and wait for ack.
		FOutBunch CloseBunch( this, 1 );
		check(!CloseBunch.IsError());
		check(CloseBunch.bClose);
		CloseBunch.bReliable = 1;
		CloseBunch.bDormant = 0;

		// Serialize DestructInfo
		NET_CHECKSUM(CloseBunch); // This is to mirror the Checksum in UPackageMapClient::SerializeNewActor
		Connection->PackageMap->WriteObject( CloseBunch, DestructInfo->ObjOuter.Get(), DestructInfo->NetGUID, DestructInfo->PathName );

		UE_LOG(LogNetTraffic, Log, TEXT("SetChannelActorForDestroy: Channel %d. NetGUID <%s> Path: %s. Bits: %d"), ChIndex, *DestructInfo->NetGUID.ToString(), *DestructInfo->PathName, CloseBunch.GetNumBits() );
		UE_LOG(LogNetDormancy, Verbose, TEXT("SetChannelActorForDestroy: Channel %d. NetGUID <%s> Path: %s. Bits: %d"), ChIndex, *DestructInfo->NetGUID.ToString(), *DestructInfo->PathName, CloseBunch.GetNumBits() );

		SendBunch( &CloseBunch, 0 );
	}
}

void UActorChannel::ReceivedBunch( FInBunch& Bunch )
{
	check(!Closing);
	if ( Broken || bTornOff )
	{
		return;
	}

	const bool bIsServer = (Connection->Driver->ServerConnection == NULL);

	FReplicationFlags RepFlags;

	// ------------------------------------------------------------
	// Initialize client if first time through.
	// ------------------------------------------------------------
	bool SpawnedNewActor = false;
	if( Actor == NULL )
	{
		if( !Bunch.bOpen )
		{
			UE_LOG(LogNetTraffic, Log, TEXT("New actor channel received non-open packet: %i/%i/%i"),Bunch.bOpen,Bunch.bClose,Bunch.bReliable);
			return;
		}

		AActor * NewChannelActor = NULL;
		SpawnedNewActor = Connection->PackageMap->SerializeNewActor(Bunch, this, NewChannelActor);

		// We are unsynchronized. Instead of crashing, let's try to recover.
		if (!NewChannelActor)
		{
			UE_LOG(LogNet, Log, TEXT("Received invalid actor class on channel %i"), ChIndex);
			Broken = 1;
			FNetControlMessage<NMT_ActorChannelFailure>::Send(Connection, ChIndex);
			return;
		}

		UE_LOG(LogNetTraffic, Log, TEXT("      Channel Actor %s:"), *NewChannelActor->GetFullName() );
		SetChannelActor( NewChannelActor );

		Actor->OnActorChannelOpen(Bunch, Connection);

		RepFlags.bNetInitial = true;
	}
	else
	{
		UE_LOG(LogNetTraffic, Log, TEXT("      Actor %s:"), *Actor->GetFullName() );
	}

	// Owned by connection's player?
	UNetConnection* ActorConnection = Actor->GetNetConnection();
	if (ActorConnection == Connection || (ActorConnection != NULL && ActorConnection->IsA(UChildConnection::StaticClass()) && ((UChildConnection*)ActorConnection)->Parent == Connection))
	{
		RepFlags.bNetOwner = true;
	}

	// ----------------------------------------------
	//	Read chunks of actor content
	// ----------------------------------------------
	while ( !Bunch.AtEnd() )
	{
		UObject * RepObj = ReadContentBlockHeader( Bunch );

		if ( Bunch.IsError() )
		{
			UE_LOG( LogNet, Error, TEXT( "ReceivedBunch: ReadContentBlockHeader FAILED. Bunch.IsError() == TRUE.  Closing connection.") );
			Connection->Close();
			return;
		}

		if ( !RepObj )
		{
			continue;
		}
		
		FObjectReplicator & Replicator = FindOrCreateReplicator( RepObj );

		if ( !Replicator.ReceivedBunch( Bunch, RepFlags ) )
		{
			UE_LOG( LogNet, Error, TEXT( "ReceivedBunch: Replicator.ReceivedBunch failed.  Closing connection.") );
			Connection->Close();
			return;
		}
	}
	
	for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
	{
		RepComp.Value()->PostReceivedBunch();
	}

	// After all properties have been initialized, call PostNetInit. This should call BeginPlay() so initialization can be done with proper starting values.
	if (Actor && SpawnedNewActor)
	{
		Actor->PostNetInit();
	}

	// Tear off an actor on the client-side
	if ( Actor && Actor->bTearOff && Connection->Driver->GetNetMode() == NM_Client )
	{
		Actor->Role = ROLE_Authority;
		Actor->SetReplicates(false);
		bTornOff = true;
		Connection->ActorChannels.Remove( Actor );
		Actor->TornOff();
		Actor = NULL;
	}
}

bool UActorChannel::ReplicateActor()
{
	SCOPE_CYCLE_COUNTER(STAT_NetReplicateActorsTime);

	checkSlow(Actor);
	checkSlow(!Closing);

	// Time how long it takes to replicate this particular actor
	STAT( FScopeCycleCounterUObject FunctionScope(Actor) );

	bool WroteSomethingImportant = false;

	// triggering replication of an Actor while already in the middle of replication can result in invalid data being sent and is therefore illegal
	if (bIsReplicatingActor)
	{
		FString Error(FString::Printf(TEXT("Attempt to replicate '%s' while already replicating that Actor!"), *Actor->GetName()));
		UE_LOG(LogNet, Log, TEXT("%s"), *Error);
		ensureMsg(false,*Error);
		return false;
	}

	// Create an outgoing bunch, and skip this actor if the channel is saturated.
	FOutBunch Bunch( this, 0 );
	if( Bunch.IsError() )
	{
		return false;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.Reliable.Debug"));
	if ( CVar && CVar->GetValueOnGameThread() > 0 )
	{
		Bunch.DebugString = FString::Printf(TEXT("%.2f ActorBunch: %s"), Connection->Driver->Time, *Actor->GetName() );
	}
#endif

	bIsReplicatingActor = true;
	FReplicationFlags RepFlags;

	// Send initial stuff.
	if( OpenPacketId.First != INDEX_NONE )
	{
		if( !SpawnAcked && OpenAcked )
		{
			// After receiving ack to the spawn, force refresh of all subsequent unreliable packets, which could
			// have been lost due to ordering problems. Note: We could avoid this by doing it in FActorChannel::ReceivedAck,
			// and avoid dirtying properties whose acks were received *after* the spawn-ack (tricky ordering issues though).
			SpawnAcked = 1;
			for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
			{
				RepComp.Value()->ForceRefreshUnreliableProperties();
			}

		}
	}
	else
	{
		RepFlags.bNetInitial = true;
		Bunch.bClose = Actor->bNetTemporary;
		Bunch.bReliable = !Actor->bNetTemporary;
	}

	// Owned by connection's player?
	UNetConnection* OwningConnection = Actor->GetNetConnection();
	RepFlags.bNetOwner = (OwningConnection == Connection) ? true : false;

	// ----------------------------------------------------------
	// If initial, send init data.
	// ----------------------------------------------------------
	if( RepFlags.bNetInitial && OpenedLocally )
	{
		Connection->PackageMap->SerializeNewActor(Bunch, this, Actor);
		WroteSomethingImportant = true;

		Actor->OnSerializeNewActor(Bunch);
	}

	// Save out the actor's RemoteRole, and downgrade it if necessary.
	ENetRole const ActualRemoteRole = Actor->GetRemoteRole();
	if (ActualRemoteRole == ROLE_AutonomousProxy)
	{
		if (!RepFlags.bNetOwner)
		{
			Actor->SetAutonomousProxy(false);
		}
	}

	RepFlags.bNetSimulated	= ( Actor->GetRemoteRole() == ROLE_SimulatedProxy );
	RepFlags.bRepPhysics	= Actor->ReplicatedMovement.bRepPhysics;

	RepFlags.bNetInitial = RepFlags.bNetInitial || bActorStillInitial; // for replication purposes, bNetInitial stays true until all properties sent
	bActorMustStayDirty = false;
	bActorStillInitial = false;

	UE_LOG(LogNetTraffic, Log, TEXT("Replicate %s, bNetInitial: %d, bNetOwner: %d"), *Actor->GetName(), RepFlags.bNetInitial, RepFlags.bNetOwner );

	FMemMark	MemMark(FMemStack::Get());	// The calls to ReplicateProperties will allocate memory on FMemStack::Get(), and use it in ::PostSendBunch. we free it below

	bool FilledUp = false;	// For now, we cant be filled up since we do partial bunches, but there is some logic below I want to preserve in case we ever do need to cut off partial bunch size

	// ----------------------------------------------------------
	// Replicate Actor and Component properties and RPCs
	// ----------------------------------------------------------

#if USE_NETWORK_PROFILER 
	const uint32 ActorReplicateStartTime = GNetworkProfiler.IsTrackingEnabled() ? FPlatformTime::Cycles() : 0;
#endif

	// The Actor
	WroteSomethingImportant |= ActorReplicator->ReplicateProperties( Bunch, RepFlags );

	// The SubObjects
	WroteSomethingImportant |= Actor->ReplicateSubobjects(this, &Bunch, &RepFlags);

	// Look for deleted subobjects
	for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
	{
		if (!RepComp.Key().IsValid())
		{
			// Write a deletion content header:
			//	-Deleted object's NetGUID
			//	-Invalid NetGUID (interpretted as delete)
			Bunch << RepComp.Value()->ObjectNetGUID;

			FNetworkGUID InvalidNetGUID;
			InvalidNetGUID.Reset();
			Bunch << InvalidNetGUID;

			WroteSomethingImportant = true;
			Bunch.bReliable = true;

			RepComp.Value()->CleanUp();
			RepComp.RemoveCurrent();			
		}
	}


	NETWORK_PROFILER(GNetworkProfiler.TrackReplicateActor(Actor, RepFlags, FPlatformTime::Cycles() - ActorReplicateStartTime ));

	// -----------------------------
	// Send if necessary
	// -----------------------------
	bool SentBunch = false;
	if( WroteSomethingImportant )
	{
		FPacketIdRange PacketRange = SendBunch( &Bunch, 1 );
		for (auto RepComp = ReplicationMap.CreateIterator(); RepComp; ++RepComp)
		{
			RepComp.Value()->PostSendBunch( PacketRange, Bunch.bReliable );
		}

		// If there were any subobject keys pending, add them to the NakMap
		if (PendingObjKeys.Num() >0)
		{
			// For the packet range we just sent over
			for(int32 PacketId = PacketRange.First; PacketId <= PacketRange.Last; ++PacketId)
			{
				// Get the existing set (its possible we send multiple bunches back to back and they end up on the same packet)
				FPacketRepKeyInfo &Info = SubobjectNakMap.FindOrAdd(PacketId % SubobjectRepKeyBufferSize);
				if (Info.PacketID != PacketId)
				{
					UE_LOG(LogNetTraffic, Verbose, TEXT("ActorChannel[%d]: Clearing out PacketRepKeyInfo for new packet: %d"), ChIndex, PacketId);
					Info.ObjKeys.Empty(Info.ObjKeys.Num());
				}
				Info.PacketID = PacketId;
				Info.ObjKeys.Append(PendingObjKeys);

				FString VerboseString;
				for (auto KeyIt = PendingObjKeys.CreateIterator(); KeyIt; ++KeyIt)
				{
					VerboseString += FString::Printf(TEXT(" %d"), *KeyIt);
				}
				
				UE_LOG(LogNetTraffic, Verbose, TEXT("ActorChannel[%d]: Sending ObjKeys: %s"), ChIndex, *VerboseString);
			}
		}
		
		SentBunch = true;
		if( Actor->bNetTemporary )
		{
			Connection->SentTemporaries.Add( Actor );
		}
	}

	PendingObjKeys.Empty();
	

	// If we evaluated everything, mark LastUpdateTime, even if nothing changed.
	if ( FilledUp )
	{
		UE_LOG(LogNetTraffic, Log, TEXT("Filled packet up before finishing %s still initial %d"),*Actor->GetName(),bActorStillInitial);
	}
	else
	{
		LastUpdateTime = Connection->Driver->Time;
	}

	bActorStillInitial = RepFlags.bNetInitial && (FilledUp || (!Actor->bNetTemporary && bActorMustStayDirty));

	// Reset temporary net info.
	if (Actor->GetRemoteRole() != ActualRemoteRole)
	{
		Actor->SetReplicates(ActualRemoteRole != ROLE_None);
		if (ActualRemoteRole == ROLE_AutonomousProxy)
		{
			Actor->SetAutonomousProxy(true);
		}
	}

	MemMark.Pop();

	bIsReplicatingActor = false;

	return SentBunch;
}


FString UActorChannel::Describe()
{
	if( Closing || !Actor )
		return FString(TEXT("Actor=None ")) + UChannel::Describe();
	else
		return FString::Printf(TEXT("Actor=%s (Role=%i RemoteRole=%i) ActorClass: %s"), *Actor->GetFullName(), (int32)Actor->Role, (int32)Actor->GetRemoteRole(), ActorClass ? *ActorClass->GetName() : TEXT("NULL") ) + UChannel::Describe();
}


void UActorChannel::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UActorChannel* This = CastChecked<UActorChannel>(InThis);	
	Super::AddReferencedObjects( This, Collector );
}


void UActorChannel::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	
	if (Ar.IsCountingMemory())
	{
		for (auto MapIt = ReplicationMap.CreateIterator(); MapIt; ++MapIt)
		{
			MapIt.Value()->Serialize(Ar);
		}
	}
}

void UActorChannel::QueueRemoteFunctionBunch( UObject * CallTarget, UFunction* Func, FOutBunch &Bunch )
{
	FindOrCreateReplicator(CallTarget).QueueRemoteFunctionBunch( Func, Bunch );
}

void UActorChannel::BecomeDormant()
{
	UE_LOG(LogNetDormancy, Verbose, TEXT("Channel[%d] '%s' BecomeDormant()"), ChIndex, *Describe() );
	bPendingDormancy = false;
	Dormant = true;
	Close();
}

bool UActorChannel::ReadyForDormancy(bool suppressLogs)
{
	for (auto MapIt = ReplicationMap.CreateIterator(); MapIt; ++MapIt)
	{
		if (!MapIt.Value()->ReadyForDormancy(suppressLogs))
		{
			return false;
		}
	}
	return true;
}

void UActorChannel::StartBecomingDormant()
{
	UE_LOG(LogNetDormancy, Verbose, TEXT("Channel[%d] '%s' StartBecomingDormant()"), ChIndex, *Describe() );

	for (auto MapIt = ReplicationMap.CreateIterator(); MapIt; ++MapIt)
	{
		MapIt.Value()->StartBecomingDormant();
	}
	bPendingDormancy = 1;
}

void UActorChannel::BeginContentBlock( UObject * Obj, FOutBunch &Bunch )
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDoReplicationContextString->GetInt() > 0)
	{
		Connection->PackageMap->SetDebugContextString( FString::Printf(TEXT("Content Header for object: %s (Class: %s)"), *Obj->GetPathName(), *Obj->GetClass()->GetPathName() ) );
	}
#endif

	check(Obj);
	Bunch << Obj;
	NET_CHECKSUM(Bunch);

	UClass *ObjClass = Obj->GetClass();
	Bunch << ObjClass;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDoReplicationContextString->GetInt() > 0)
	{
		Connection->PackageMap->ClearDebugContextString();
	}
#endif
}

void UActorChannel::EndContentBlock( UObject *Obj, FOutBunch &Bunch, FClassNetCache* ClassCache )
{
	check(Obj);
	if (!ClassCache)
	{
		ClassCache = Connection->PackageMap->GetClassNetCache( Obj->GetClass() );
	}

	// Write max int to signify done
	Bunch.WriteIntWrapped(ClassCache->GetMaxIndex(), ClassCache->GetMaxIndex()+1);
}

UObject * UActorChannel::ReadContentBlockHeader( FInBunch & Bunch )
{
	const bool IsServer = ( Connection->Driver->ServerConnection == NULL );

	// Note this heavily mirrors what happens in UPackageMapClient::SerializeNewActor
	FNetworkGUID NetGUID;
	UObject *RepObj = NULL;

	// Manually serialize the object so that we can get the NetGUID (in order to assign it if we spawn the object here)
	Connection->PackageMap->SerializeObject(Bunch, UObject::StaticClass(), RepObj, &NetGUID);
	NET_CHECKSUM_OR_END(Bunch);

	if ( Bunch.IsError() || Bunch.AtEnd() )
	{
		Bunch.SetError();
		return NULL;
	}

	if ( RepObj != NULL && RepObj != Actor && !RepObj->IsIn( Actor ) )
	{
		UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Object %s not in parent actor 1 %s" ), *RepObj->GetName(), *Actor->GetName() );
		Bunch.SetError();
		return NULL;
	}

	// Serialize the class incase we have to spawn it.
	// Manually serialize the object so that we can get the NetGUID (in order to assign it if we spawn the object here)
	FNetworkGUID ClassNetGUID;
	UObject *CastObj = NULL;
	Connection->PackageMap->SerializeObject(Bunch, UObject::StaticClass(), CastObj, &ClassNetGUID);

	// Delete subobject
	if ( !ClassNetGUID.IsValid() )
	{
		if ( IsServer )
		{
			UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Client attempted to delete subobject %s" ), *Actor->GetName() );
			Bunch.SetError();
			return NULL;
		}

		if ( RepObj )
		{
			if ( Cast<AActor>( RepObj ) != NULL )
			{
				UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Attempting to delete actor while deleting subobject: %s" ), *Actor->GetName() );
				Bunch.SetError();
				return NULL;
			}

			Actor->OnSubobjectDestroyFromReplication(RepObj);
			RepObj->MarkPendingKill();
		}
		return NULL;
	}

	UClass * RepClass = Cast<UClass>(CastObj);

	// Valid NetGUID but no class was resolved - this is an error
	if ( RepClass == NULL )
	{
		UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Unable to read subobject class for actor %s" ), *Actor->GetName() );
		Bunch.SetError();
		return NULL;
	}

	if ( RepClass == UObject::StaticClass() )
	{
		UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: RepClass == UObject::StaticClass() %s" ), *Actor->GetName() );
		Bunch.SetError();
		return NULL;
	}

	if ( !RepObj )
	{
		if ( IsServer )
		{
			UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Client attempted to create subobject %s" ), *Actor->GetName() );
			Bunch.SetError();
			return NULL;
		}

		// Construct the class
		UE_LOG( LogNetTraffic, Log, TEXT( "ReadContentBlockHeader: Instantiating subobject for actor %s. Class: %s" ), *Actor->GetName(), *RepClass->GetName());
		RepObj = ConstructObject<UObject>(RepClass, Actor);
		check( RepObj );
		Actor->OnSubobjectCreatedFromReplication(RepObj);
		Connection->PackageMap->AssignNetGUID(RepObj, NetGUID);
	}

	if ( RepObj != Actor && !RepObj->IsIn( Actor ) )
	{
		UE_LOG( LogNetTraffic, Error, TEXT( "ReadContentBlockHeader: Object %s not in parent actor 2 %s" ), *RepObj->GetName(), *Actor->GetName() );
		Bunch.SetError();
		return NULL;
	}

	return RepObj;
}

FObjectReplicator & UActorChannel::GetActorReplicationData()
{
	return ReplicationMap.FindChecked(Actor).Get();
}

FObjectReplicator & UActorChannel::FindOrCreateReplicator( UObject * Obj )
{
	// First, try to find it on the channel replication map
	TSharedRef<FObjectReplicator> * ReplicatorRefPtr = ReplicationMap.Find( Obj );

	if ( ReplicatorRefPtr == NULL )
	{
		// Didn't find it. 
		// Try to find in the dormancy map
		ReplicatorRefPtr = Connection->DormantReplicatorMap.Find( Obj );

		if ( ReplicatorRefPtr == NULL )
		{
			// Still didn't find one, need to create
			UE_LOG( LogNetTraffic, Log, TEXT( "Creating Replicator for %s" ), *Obj->GetName() );

			ReplicatorRefPtr = new TSharedRef<FObjectReplicator>( new FObjectReplicator() );
			ReplicatorRefPtr->Get().InitWithObject( Obj, Connection, true );
		}

		// Add to the replication map
		ReplicationMap.Add( Obj, *ReplicatorRefPtr );

		// Remove from dormancy map in case we found it there
		Connection->DormantReplicatorMap.Remove( Obj );

		// Start replicating with this replicator
		ReplicatorRefPtr->Get().StartReplicating( this );
	}

	return ReplicatorRefPtr->Get();
}

bool UActorChannel::ObjectHasReplicator(UObject *Obj)
{
	return ReplicationMap.Contains(Obj);
}

bool UActorChannel::KeyNeedsToReplicate(int32 ObjID, int32 RepKey)
{
	int32 &MapKey = SubobjectRepKeyMap.FindOrAdd(ObjID);
	if (MapKey == RepKey)
		return false;

	MapKey = RepKey;
	PendingObjKeys.Add(ObjID);
	return true;
}

bool UActorChannel::ReplicateSubobject(UObject *Obj, FOutBunch &Bunch, const FReplicationFlags &RepFlags)
{
	// Hack for now: subobjects are SupportsObject==false until they are replicated via ::ReplicateSUbobject, and then we make them supported
	// here, by forcing the packagemap to give them a NetGUID.
	//
	// Once we can lazily handle unmapped references on the client side, this can be simplified.
	if (!Connection->PackageMap->SupportsObject(Obj))
	{
		FNetworkGUID NetGUID = Connection->PackageMap->AssignNewNetGUID(Obj);	//Make sure he gets a NetGUID so that he is now 'supported'
	}

	bool NewSubobject = false;
	if (!ObjectHasReplicator(Obj))
	{
		// This is the first time replicating this subobject
		// This bunch should be reliable and we should always return true
		// even if the object properties did not diff from the CDO
		// (this will ensure the content header chunk is sent which is all we care about
		// to spawnt his on the client).
		Bunch.bReliable = true;
		NewSubobject = true;
	}
	bool WroteSomething = FindOrCreateReplicator(Obj).ReplicateProperties(Bunch, RepFlags);
	if (NewSubobject && !WroteSomething)
	{
		BeginContentBlock( Obj, Bunch );
		WroteSomething= true;
		EndContentBlock( Obj, Bunch );
	}

	return WroteSomething;
}

//------------------------------------------------------

static void	DebugNetGUIDs( UWorld* InWorld )
{
	UNetDriver *NetDriver = InWorld->NetDriver;
	if (!NetDriver)
	{
		return;
	}

	UNetConnection * Connection = (NetDriver->ServerConnection ? NetDriver->ServerConnection : (NetDriver->ClientConnections.Num() > 0 ? NetDriver->ClientConnections[0] : NULL));
	if (!Connection)
	{
		return;
	}

	Connection->PackageMap->LogDebugInfo(*GLog);
}

FAutoConsoleCommandWithWorld DormantActorCommand(
	TEXT("net.ListNetGUIDs"), 
	TEXT( "Lists NetGUIDs for actors" ), 
	FConsoleCommandWithWorldDelegate::CreateStatic(DebugNetGUIDs)
	);

//------------------------------------------------------

static void	ListOpenActorChannels( UWorld* InWorld )
{
	UNetDriver *NetDriver = InWorld->NetDriver;
	if (!NetDriver)
	{
		return;
	}

	UNetConnection * Connection = (NetDriver->ServerConnection ? NetDriver->ServerConnection : (NetDriver->ClientConnections.Num() > 0 ? NetDriver->ClientConnections[0] : NULL));
	if (!Connection)
	{
		return;
	}

	TMap<UClass*, int32> ClassMap;
	
	for (auto It = Connection->ActorChannels.CreateIterator(); It; ++It)
	{
		UActorChannel* Chan = It.Value();
		UClass *ThisClass = Chan->Actor->GetClass();
		while( Cast<UBlueprintGeneratedClass>(ThisClass))
		{
			ThisClass = ThisClass->GetSuperClass();
		}

		UE_LOG(LogNet, Warning, TEXT("Chan[%d] %s "), Chan->ChIndex, *Chan->Actor->GetFullName() );

		int32 &cnt = ClassMap.FindOrAdd(ThisClass);
		cnt++;
	}

	// Sort by the order in which categories were edited
	struct FCompareActorClassCount
	{
		FORCEINLINE bool operator()( int32 A, int32 B ) const
		{
			return A < B;
		}
	};
	ClassMap.ValueSort( FCompareActorClassCount() );

	UE_LOG(LogNet, Warning, TEXT("-----------------------------") );

	for (auto It = ClassMap.CreateIterator(); It; ++It)
	{
		UE_LOG(LogNet, Warning, TEXT("%4d - %s"), It.Value(), *It.Key()->GetName() );
	}
}

FAutoConsoleCommandWithWorld ListOpenActorChannelsCommand(
	TEXT("net.ListActorChannels"), 
	TEXT( "Lists open actor channels" ), 
	FConsoleCommandWithWorldDelegate::CreateStatic(ListOpenActorChannels)
	);

//------------------------------------------------------

static void	DeleteDormantActor( UWorld* InWorld )
{
	UNetDriver *NetDriver = InWorld->NetDriver;
	if (!NetDriver)
	{
		return;
	}

	UNetConnection * Connection = (NetDriver->ServerConnection ? NetDriver->ServerConnection : (NetDriver->ClientConnections.Num() > 0 ? NetDriver->ClientConnections[0] : NULL));
	if (!Connection)
	{
		return;
	}

	for (auto It = Connection->DormantActors.CreateIterator(); It; ++It)
	{
		AActor * ThisActor = const_cast<AActor*>(*It);

		UE_LOG(LogNet, Warning, TEXT("Deleting actor %s"), *ThisActor->GetName());

		FBox Box = ThisActor->GetComponentsBoundingBox();
		
		DrawDebugBox( InWorld, Box.GetCenter(), Box.GetExtent(), FQuat::Identity, FColor::Red, true, 30 );

		ThisActor->Destroy();

		break;
	}
}

FAutoConsoleCommandWithWorld DeleteDormantActorCommand(
	TEXT("net.DeleteDormantActor"), 
	TEXT( "Lists open actor channels" ), 
	FConsoleCommandWithWorldDelegate::CreateStatic(DeleteDormantActor)
	);

//------------------------------------------------------
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static void	FindNetGUID( const TArray<FString>& Args, UWorld* InWorld )
{
	for (FObjectIterator ObjIt(UNetGUIDCache::StaticClass()); ObjIt; ++ObjIt)
	{
		UNetGUIDCache *Cache = Cast<UNetGUIDCache>(*ObjIt);
		if (Cache->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			continue;

		if (Args.Num() <= 0)
		{
			// Display all
			for (auto It = Cache->History.CreateIterator(); It; ++It)
			{
				FString Str = It.Value();
				FNetworkGUID NetGUID = It.Key();
				UE_LOG(LogNet, Warning, TEXT("<%s> - %s"), *NetGUID.ToString(), *Str);
			}
		}
		else
		{
			uint32 GUIDValue=0;
			TTypeFromString<uint32>::FromString(GUIDValue, *Args[0]);
			FNetworkGUID NetGUID(GUIDValue);

			// Search
			FString Str = Cache->History.FindRef(NetGUID);

			if (!Str.IsEmpty())
			{
				UE_LOG(LogNet, Warning, TEXT("Found: %s"), *Str);
			}
			else
			{
				UE_LOG(LogNet, Warning, TEXT("No matches") );
			}
		}
	}
}

FAutoConsoleCommandWithWorldAndArgs FindNetGUIDCommand(
	TEXT("net.Packagemap.FindNetGUID"), 
	TEXT( "Looks up object that was assigned a given NetGUID" ), 
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(FindNetGUID)
	);
#endif

//------------------------------------------------------

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static void	TestObjectRefSerialize( const TArray<FString>& Args, UWorld* InWorld )
{
	if (!InWorld || Args.Num() <= 0)
		return;

	UObject * Object = StaticFindObject( UObject::StaticClass(), NULL, *Args[0], false );
	if (!Object)
	{
		Object = StaticLoadObject( UObject::StaticClass(), NULL, *Args[0], NULL, LOAD_NoWarn );
	}

	if (!Object)
	{
		UE_LOG(LogNet, Warning, TEXT("Couldn't find object: %s"), *Args[0]);
		return;
	}

	UE_LOG(LogNet, Warning, TEXT("Repping reference to: %s"), *Object->GetName());

	UNetDriver *NetDriver = InWorld->GetNetDriver();

	for (int32 i=0; i < NetDriver->ClientConnections.Num(); ++i)
	{
		if ( NetDriver->ClientConnections[i] && NetDriver->ClientConnections[i]->PackageMap )
		{
			FBitWriter TempOut( 1024 * 10, true);
			NetDriver->ClientConnections[i]->PackageMap->SerializeObject(TempOut, UObject::StaticClass(), Object, NULL);
		}
	}

	/*
	for( auto Iterator = InWorld->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		PlayerController->ClientRepObjRef(Object);
	}
	*/
}

FAutoConsoleCommandWithWorldAndArgs TestObjectRefSerializeCommand(
	TEXT("net.TestObjRefSerialize"), 
	TEXT( "Attempts to replicate an object reference to all clients" ), 
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(TestObjectRefSerialize)
	);
#endif
