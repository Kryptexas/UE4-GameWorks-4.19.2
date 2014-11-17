// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "InAppPurchaseQueryCallbackProxy.h"
#include "K2Node_InAppPurchaseQuery.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_InAppPurchaseQuery::UK2Node_InAppPurchaseQuery(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UInAppPurchaseQueryCallbackProxy, CreateProxyObjectForInAppPurchaseQuery);
	ProxyFactoryClass = UInAppPurchaseQueryCallbackProxy::StaticClass();

	ProxyClass = UInAppPurchaseQueryCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
