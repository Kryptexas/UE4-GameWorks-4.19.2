// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
/*=============================================================================
	IGameAndPartyService.h: Interface for game and party services.
	The game and party service is used to inform when a party action has been performed.
	e.g. friend joined your party
=============================================================================*/

#pragma once

class IGameAndPartyService
{
public:

	DECLARE_EVENT_ThreeParams(IGameAndPartyService, FOnFriendsJoinPartyEvent, const FUniqueNetId& /*SenderId*/, const TSharedRef<class IOnlinePartyJoinInfo>& /*PartyJoinInfo*/, bool /*bIsFromInvite*/)
	virtual FOnFriendsJoinPartyEvent& OnFriendsJoinParty() = 0;
};
