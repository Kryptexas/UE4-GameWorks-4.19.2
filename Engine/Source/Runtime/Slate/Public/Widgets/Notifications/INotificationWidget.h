// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SLATE_API INotificationWidget
{
public:
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) = 0;
	virtual TSharedRef< SWidget > AsWidget() = 0;
};