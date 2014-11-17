// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "LeaderboardFlushCallbackProxy.h"
#include "K2Node_LeaderboardFlush.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_LeaderboardFlush::UK2Node_LeaderboardFlush(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(ULeaderboardFlushCallbackProxy, CreateProxyObjectForFlush);
	ProxyFactoryClass = ULeaderboardFlushCallbackProxy::StaticClass();

	ProxyClass = ULeaderboardFlushCallbackProxy::StaticClass();
}

FString UK2Node_LeaderboardFlush::GetTooltip() const
{
	return LOCTEXT("K2Node_LeaderboardFlush_Tooltip", "Flushes leaderboards for a session").ToString();
}

FText UK2Node_LeaderboardFlush::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("FlushLeaderboards", "Flush Leaderboards");
}

FString UK2Node_LeaderboardFlush::GetCategoryName()
{
	return TEXT("Online|Leaderboard");
}

#undef LOCTEXT_NAMESPACE
