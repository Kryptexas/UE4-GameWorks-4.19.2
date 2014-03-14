// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineNotificationHandler.h"

void FOnlineNotificationHandler::AddNotificationBinding(FName NotificationName, const FOnlineNotificationBinding& NewBinding)
{
	TArray<FOnlineNotificationBinding>& FoundBindings = BindingMap.FindOrAdd(NotificationName);

	if (!NewBinding.NotificationDelegate.IsBound())
	{
		UE_LOG(LogOnline,Error,TEXT("Adding empty notification binding for type %s"), *NotificationName.ToString());
		return;
	}

	for (int32 i = 0; i < FoundBindings.Num(); i++)
	{
		if (FoundBindings[i] == NewBinding)
		{
			UE_LOG(LogOnline,Error,TEXT("Adding identical notification binding for type %s"), *NotificationName.ToString());
			return;
		}
	}

	FoundBindings.Add(NewBinding);
}

void FOnlineNotificationHandler::SetDefaultNotificationBinding(const FOnlineNotificationBinding& NewBinding)
{
	if (!NewBinding.NotificationDelegate.IsBound())
	{
		UE_LOG(LogOnline,Error,TEXT("Adding empty default notification binding"));
		return;
	}

	DefaultBinding = NewBinding;
}

void FOnlineNotificationHandler::HandleNotification(const FOnlineNotification& Notification)
{
	EOnlineNotificationResult::Type CurrentResult = EOnlineNotificationResult::None;

	TArray<FOnlineNotificationBinding>* FoundBindings = BindingMap.Find(FName(*Notification.TypeStr));

	if (FoundBindings)
	{
		for (int32 i = 0; i < FoundBindings->Num(); i++)
		{
			const FOnlineNotificationBinding& Binding = (*FoundBindings)[i];
			
			if (Binding.NotificationDelegate.IsBound())
			{
				CurrentResult = Binding.NotificationDelegate.Execute(Notification);

				if (CurrentResult == EOnlineNotificationResult::Block)
				{
					// Done handling
					return;
				}
			}
		}
	}

	if (DefaultBinding.NotificationDelegate.IsBound())
	{
		CurrentResult = DefaultBinding.NotificationDelegate.Execute(Notification);
	}
}

void FOnlineNotificationHandler::ResetBindings()
{
	BindingMap.Empty();
	DefaultBinding = FOnlineNotificationBinding();
}