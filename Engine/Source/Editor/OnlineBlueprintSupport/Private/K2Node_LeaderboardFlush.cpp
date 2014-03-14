// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "LeaderboardFlushCallbackProxy.h"
#include "K2Node_LeaderboardFlush.h"

UK2Node_LeaderboardFlush::UK2Node_LeaderboardFlush(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(ULeaderboardFlushCallbackProxy, CreateProxyObjectForFlush);
	ProxyFactoryClass = ULeaderboardFlushCallbackProxy::StaticClass();

	ProxyClass = ULeaderboardFlushCallbackProxy::StaticClass();
}

FString UK2Node_LeaderboardFlush::GetTooltip() const
{
	return TEXT("Flushes leaderboards for a session");
}

FString UK2Node_LeaderboardFlush::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Flush Leaderboards");
}

FString UK2Node_LeaderboardFlush::GetCategoryName()
{
	return TEXT("Online|Leaderboard");
}
