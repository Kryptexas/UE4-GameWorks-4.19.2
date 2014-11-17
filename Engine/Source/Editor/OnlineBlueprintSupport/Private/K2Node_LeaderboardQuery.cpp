// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "LeaderboardQueryCallbackProxy.h"
#include "K2Node_LeaderboardQuery.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_LeaderboardQuery::UK2Node_LeaderboardQuery(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(ULeaderboardQueryCallbackProxy, CreateProxyObjectForIntQuery);
	ProxyFactoryClass = ULeaderboardQueryCallbackProxy::StaticClass();

	ProxyClass = ULeaderboardQueryCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
