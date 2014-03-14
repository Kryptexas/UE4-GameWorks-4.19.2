// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Online.h"

//////////////////////////////////////////////////////////////////////////
// FPlayerMuteList

// Tell the server to mute a given player
void FPlayerMuteList::ServerMutePlayer(APlayerController* OwningPC, const FUniqueNetIdRepl& MuteId)
{
	const TSharedPtr<FUniqueNetId>& PlayerId = MuteId.GetUniqueNetId();

	FUniqueNetIdMatcher PlayerMatch(*PlayerId);

	// Don't reprocess if they are already muted
	if (VoiceMuteList.FindMatch(PlayerMatch) == INDEX_NONE)
	{
		VoiceMuteList.Add(PlayerId.ToSharedRef());
	}
	
	// Add them to the packet filter list if not already on it
	if (VoicePacketFilter.FindMatch(PlayerMatch) == INDEX_NONE)
	{
		VoicePacketFilter.Add(PlayerId.ToSharedRef());
	}

	// Replicate mute state to client
	OwningPC->ClientMutePlayer(MuteId);

	// Find the muted player's player controller so it can be notified
	APlayerController* OtherPC = GetPlayerControllerFromNetId(OwningPC->GetWorld(), *PlayerId);
	if (OtherPC != NULL)
	{
		// Update their packet filter list too
		OtherPC->MuteList.ClientMutePlayer(OtherPC, OwningPC->PlayerState->UniqueId);

		// Tell the other PC to mute this one
		OtherPC->ClientMutePlayer(OwningPC->PlayerState->UniqueId);
	}
}

// Tell the server to unmute a given player
void FPlayerMuteList::ServerUnmutePlayer(APlayerController* OwningPC, const FUniqueNetIdRepl& UnmuteId)
{
	const TSharedPtr<FUniqueNetId>& PlayerId = UnmuteId.GetUniqueNetId();

	FUniqueNetIdMatcher PlayerMatch(*PlayerId);

	// If the player was found, remove them from our explicit list
	int32 RemoveIndex = VoiceMuteList.FindMatch(PlayerMatch);
	if (RemoveIndex != INDEX_NONE)
	{
		VoiceMuteList.RemoveAtSwap(RemoveIndex);
	}

	// Find the muted player's player controller so it can be notified
	APlayerController* OtherPC = GetPlayerControllerFromNetId(OwningPC->GetWorld(), *PlayerId);
	if (OtherPC != NULL)
	{
		FUniqueNetIdMatcher OtherMatch(*OwningPC->PlayerState->UniqueId);

		// Make sure this player isn't muted for gameplay reasons
		if (GameplayVoiceMuteList.FindMatch(PlayerMatch) == INDEX_NONE &&
			// And make sure they didn't mute us
			OtherPC->MuteList.VoiceMuteList.FindMatch(OtherMatch) == INDEX_NONE)
		{
			OwningPC->ClientUnmutePlayer(UnmuteId);
		}

		// If the other player doesn't have this player muted
		if (OtherPC->MuteList.VoiceMuteList.FindMatch(OtherMatch) == INDEX_NONE &&
			OtherPC->MuteList.GameplayVoiceMuteList.FindMatch(OtherMatch) == INDEX_NONE)
		{
			// Remove them from the packet filter list
			RemoveIndex = VoicePacketFilter.FindMatch(PlayerMatch);
			if (RemoveIndex != INDEX_NONE)
			{
				VoicePacketFilter.RemoveAtSwap(RemoveIndex);
			}
			// If found, remove so packets flow to that client too
			RemoveIndex = OtherPC->MuteList.VoicePacketFilter.FindMatch(OtherMatch);
			if (RemoveIndex != INDEX_NONE)
			{
				OtherPC->MuteList.VoicePacketFilter.RemoveAtSwap(RemoveIndex);
			}

			// Tell the other PC to unmute this one
			OtherPC->ClientUnmutePlayer(OwningPC->PlayerState->UniqueId);
		}
	}
}

// Tell the client to mute a given player
void FPlayerMuteList::ClientMutePlayer(APlayerController* OwningPC, const FUniqueNetIdRepl& MuteId)
{
	const TSharedPtr<FUniqueNetId>& PlayerId = MuteId.GetUniqueNetId();

	FUniqueNetIdMatcher PlayerMatch(*PlayerId);

	// Add to the filter list on clients (used for peer to peer voice)
	if (VoicePacketFilter.FindMatch(PlayerMatch) == INDEX_NONE)
	{
		VoicePacketFilter.Add(PlayerId.ToSharedRef());
	}

	// Use the local player to determine the controller id
	ULocalPlayer* LP = Cast<ULocalPlayer>(OwningPC->Player);
	if (LP != NULL)
	{
		IOnlineVoicePtr VoiceInt = Online::GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			// Have the voice subsystem mute this player
			VoiceInt->MuteRemoteTalker(LP->ControllerId, *PlayerId, false);
		}
	}
}

// Tell the client to unmute a given player
void FPlayerMuteList::ClientUnmutePlayer(APlayerController* OwningPC, const FUniqueNetIdRepl& UnmuteId)
{
	const TSharedPtr<FUniqueNetId>& PlayerId = UnmuteId.GetUniqueNetId();

	FUniqueNetIdMatcher PlayerMatch(*PlayerId);

	// It's safe to remove them from the filter list on clients (used for peer to peer voice)
	int32 RemoveIndex = VoicePacketFilter.FindMatch(PlayerMatch);
	if (RemoveIndex != INDEX_NONE)
	{
		VoicePacketFilter.RemoveAtSwap(RemoveIndex);
	}

	// Use the local player to determine the controller id
	ULocalPlayer* LP = Cast<ULocalPlayer>(OwningPC->Player);
	if (LP != NULL)
	{
		IOnlineVoicePtr VoiceInt = Online::GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			// Have the voice subsystem mute this player
			VoiceInt->UnmuteRemoteTalker(LP->ControllerId, *PlayerId, false);
		}
	}
}

// Is a given player currently muted?
bool FPlayerMuteList::IsPlayerMuted(const FUniqueNetId& PlayerId)
{
	FUniqueNetIdMatcher PlayerMatch(PlayerId);
	return VoicePacketFilter.FindMatch(PlayerMatch) != INDEX_NONE;
}
