//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "TickableNotification.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "LevelEditor.h"

namespace Phonon
{
	FTickableNotification::FTickableNotification()
		: bIsTicking(false)
	{
	}

	void FTickableNotification::CreateNotification()
	{
		//FScopeLock Lock(&CriticalSection);

		FNotificationInfo Info(DisplayText);
		Info.bFireAndForget = false;
		Info.FadeOutDuration = 4.0f;
		Info.ExpireDuration = 0.0f;

		NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

		bIsTicking = true;
	}

	void FTickableNotification::DestroyNotification(const SNotificationItem::ECompletionState InFinalState)
	{
		//FScopeLock Lock(&CriticalSection);

		bIsTicking = false;
		FinalState = InFinalState;
	}

	void FTickableNotification::SetDisplayText(const FText& InDisplayText)
	{
		//FScopeLock Lock(&CriticalSection);

		DisplayText = InDisplayText;
	}

	void FTickableNotification::NotifyDestruction()
	{
		if (!NotificationPtr.Pin().IsValid())
		{
			return;
		}

		NotificationPtr.Pin()->SetText(DisplayText);
		NotificationPtr.Pin()->SetCompletionState(FinalState);
		NotificationPtr.Pin()->ExpireAndFadeout();
		NotificationPtr.Reset();
	}

	void FTickableNotification::Tick(float DeltaTime)
	{
		//FScopeLock Lock(&CriticalSection);

		if (bIsTicking && NotificationPtr.Pin().IsValid())
		{
			NotificationPtr.Pin()->SetText(DisplayText);
		}
		else
		{
			NotifyDestruction();
		}
	}

	bool FTickableNotification::IsTickable() const
	{
		return true;
	}

	TStatId FTickableNotification::GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FTickableNotification, STATGROUP_Tickables);
	}
}