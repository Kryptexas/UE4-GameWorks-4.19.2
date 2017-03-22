//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "TickableEditorObject.h"
#include "SNotificationList.h"

namespace Phonon
{
	class FTickableNotification : public FTickableEditorObject
	{
	public:
		FTickableNotification();

		void CreateNotification();
		void DestroyNotification(const SNotificationItem::ECompletionState FinalState = SNotificationItem::CS_Success);
		void SetDisplayText(const FText& DisplayText);

	protected:
		virtual void Tick(float DeltaTime) override;
		virtual bool IsTickable() const override;
		virtual TStatId GetStatId() const override;

	private:
		void NotifyDestruction();

		TWeakPtr<SNotificationItem> NotificationPtr;
		
		FCriticalSection CriticalSection;
		FText DisplayText;
		bool bIsTicking;
		SNotificationItem::ECompletionState FinalState;
	};
}