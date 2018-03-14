// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "UObject/WeakObjectPtr.h"
#include "DerivedDataCacheInterface.h"
#include "IDDCNotifications.h"

class SNotificationItem;

/**
* Implementation of Editor DDC performance notifications
**/
class FDDCNotifications : public IDDCNotifications
{
public:
	FDDCNotifications();
	virtual ~FDDCNotifications();

private: 

	/** DDC data put notification handler */
	void OnDDCNotificationEvent(FDerivedDataCacheInterface::EDDCNotification DDCNotification);
	
	/** Update DDC notification event subscriptions */
	void UpdateDDCSubscriptions(bool bSubscribe = false);	

	/** Manually clear any presented DDC notifications */
	void ClearSharedDDCNotification();

	/** Whether or not to show the Shared DDC notification */
	bool bShowSharedDDCNotification;

	/** Valid when a DDC notification item is being presented */
	TSharedPtr<SNotificationItem> SharedDDCNotification;
};

