// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class IFriendsApplicationViewModel
{
public:

	// IFriendsApplicationViewModel Interface
	virtual bool IsAppJoinable() = 0;
	virtual void LaunchFriendApp(const FString& Commandline) = 0;
};
