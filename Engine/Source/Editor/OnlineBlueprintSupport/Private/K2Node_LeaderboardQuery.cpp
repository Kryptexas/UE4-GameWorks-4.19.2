// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "LeaderboardQueryCallbackProxy.h"
#include "K2Node_LeaderboardQuery.h"

UK2Node_LeaderboardQuery::UK2Node_LeaderboardQuery(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(ULeaderboardQueryCallbackProxy, CreateProxyObjectForIntQuery);
	ProxyFactoryClass = ULeaderboardQueryCallbackProxy::StaticClass();

	ProxyClass = ULeaderboardQueryCallbackProxy::StaticClass();
}

FString UK2Node_LeaderboardQuery::GetTooltip() const
{
	return TEXT("Queries a leaderboard for an integer value");
}

FString UK2Node_LeaderboardQuery::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Read Leaderboard Integer");
}

FString UK2Node_LeaderboardQuery::GetCategoryName()
{
	return TEXT("Online|Leaderboard");
}
