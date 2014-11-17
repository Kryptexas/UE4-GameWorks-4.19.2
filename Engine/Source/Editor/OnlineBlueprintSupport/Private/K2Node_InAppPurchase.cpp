// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineBlueprintSupportPrivatePCH.h"
#include "InAppPurchaseCallbackProxy.h"
#include "K2Node_InAppPurchase.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_InAppPurchase::UK2Node_InAppPurchase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UInAppPurchaseCallbackProxy, CreateProxyObjectForInAppPurchase);
	ProxyFactoryClass = UInAppPurchaseCallbackProxy::StaticClass();

	ProxyClass = UInAppPurchaseCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
