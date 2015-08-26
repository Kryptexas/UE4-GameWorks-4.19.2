// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IFriendsNavigationService
{
public:

	virtual ~IFriendsNavigationService() {}

	virtual void SetOutgoingChatFriend(TSharedRef<class IFriendItem> FriendItem) = 0;
};

