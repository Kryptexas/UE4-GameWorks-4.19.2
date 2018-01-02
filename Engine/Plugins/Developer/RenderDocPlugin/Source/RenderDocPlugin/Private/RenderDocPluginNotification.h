// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "NotificationManager.h"
#include "TickableEditorObject.h"

class FRenderDocPluginNotification : public FTickableEditorObject
{
public:
	static FRenderDocPluginNotification& Get()
	{
		static FRenderDocPluginNotification Instance;
		return Instance;
	}

	void ShowNotification(const FText& Message);
	void HideNotification();

protected:
	/** FTickableEditorObject interface */
	virtual void Tick(float DeltaTime);
	virtual ETickableTickType GetTickableTickType() const
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override;

private:
	FRenderDocPluginNotification();
	FRenderDocPluginNotification(FRenderDocPluginNotification const& Notification);
	void operator=(FRenderDocPluginNotification const&);

	/** The source code symbol query in progress message */
	TWeakPtr<SNotificationItem> RenderDocNotificationPtr;
	double LastEnableTime;
};

#endif
