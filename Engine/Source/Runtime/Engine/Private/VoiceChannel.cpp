// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VoiceChannel.cpp: Unreal voice traffic implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "Net/NetworkProfiler.h"

/** Cleans up any voice data remaining in the queue */
void UVoiceChannel::CleanUp()
{
	// Clear out refs to any voice packets so we don't leak
	VoicePackets.Empty();
	// Route to the parent class for their cleanup
	Super::CleanUp();
}

/**
 * Processes the in bound bunch to extract the voice data
 *
 * @param Bunch the voice data to process
 */
void UVoiceChannel::ReceivedBunch(FInBunch& Bunch)
{
	IOnlineVoicePtr VoiceInt = Online::GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		while (!Bunch.AtEnd())
		{
			// Give the data to the local voice processing
			TSharedPtr<FVoicePacket> VoicePacket = VoiceInt->SerializeRemotePacket(Bunch);
			if (VoicePacket.IsValid())
			{
				if (Connection->Driver->ServerConnection == NULL)
				{
					// Possibly replicate the data to other clients
					Connection->Driver->ReplicateVoicePacket(VoicePacket, Connection);
				}
#if STATS
				// Increment the number of voice packets we've received
				Connection->Driver->VoicePacketsRecv++;
				Connection->Driver->VoiceBytesRecv += VoicePacket->GetBufferSize();
#endif
			}
		}
	}
}

/**
 * Performs any per tick update of the VoIP state
 */
void UVoiceChannel::Tick()
{
	// If the handshaking hasn't completed throw away all voice data
	if (Connection->PlayerController)
	{
		// Try to append each packet in turn
		for (int32 Index = 0; Index < VoicePackets.Num(); Index++)
		{
			FOutBunch Bunch(this, 0);
			// Don't want reliable delivery. The bunch will be lost if the connection is saturated
			// First send needs to be reliable
			Bunch.bReliable = OpenAcked == false;

			TSharedPtr<FVoicePacket> Packet = VoicePackets[Index];
			// Append the packet data (copies into the bunch)
			Packet->Serialize(Bunch);

#if STATS
			// Increment the number of voice packets we've sent
			Connection->Driver->VoicePacketsSent++;
			Connection->Driver->VoiceBytesSent += Packet->GetBufferSize();
#endif
			// Don't submit the bunch if something went wrong
			if (Bunch.IsError() == false)
			{
				// Submit the bunching with merging on
				SendBunch(&Bunch, 1);
			}
			// If the network is saturated, throw away any remaining packets
			if (Connection->IsNetReady(0) == false)
			{
				// Empty early
				VoicePackets.Empty();
			}
		}
	}

	// Let the packets free itself if no longer in use
	VoicePackets.Empty();
}

/**
 * Adds the voice packet to the list to send for this channel
 *
 * @param VoicePacket the voice packet to send
 */
void UVoiceChannel::AddVoicePacket(TSharedPtr<FVoicePacket> VoicePacket)
{
	if (VoicePacket.IsValid())
	{
		VoicePackets.Add(VoicePacket);

		UE_LOG(LogNet, VeryVerbose, TEXT("AddVoicePacket: %s [%s] to=%s from=%s"),
			*Connection->PlayerId->ToDebugString(),
			*Connection->Driver->GetDescription(),
			*Connection->LowLevelDescribe(),
			*VoicePacket->GetSender()->ToDebugString());
	}
}