// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "NotificationManager.h"

namespace NotificationManagerConstants
{
	// Offsets from the bottom-left corner of the work area
	const FVector2D NotificationOffset( 15.0f, 15.0f );
}

FSlateNotificationManager::FSlateNotificationManager() :
	AnchorReferenceRect( 0.0f, 0.0f, 0.0f, 0.0f )
{
}

FSlateNotificationManager& FSlateNotificationManager::Get()
{
	static FSlateNotificationManager* Instance = NULL;
	if( Instance == NULL )
	{
		static FCriticalSection CriticalSection;
		FScopeLock Lock(&CriticalSection);
		if( Instance == NULL )
		{
			Instance = new FSlateNotificationManager;
		}
	}
	return *Instance;
}

void FSlateNotificationManager::SetRootWindow( const TSharedRef<SWindow> InRootWindow )
{
	RootWindowPtr = InRootWindow;
}

void FSlateNotificationManager::RecalculateAnchorRect( const bool bForce )
{
	// Do we need to switch the anchor point because the focus has switched to another
	// desktop area?
	// Only switch if there are no items at the moment (or we are forced)
	if( NotificationLists.Num() == 0 || bForce )
	{
		AnchorReferenceRect = FSlateApplication::Get().GetPreferredWorkArea();
	}
}

TSharedPtr<SNotificationItem> FSlateNotificationManager::AddNotification(const FNotificationInfo& Info)
{
	check(IsInGameThread() || !"FSlateNotificationManager::AddNotification must be called on game thread. Use QueueNotification if necessary.");

	// Early calls of this function can happen before Slate is initialized.
	if( FSlateApplication::IsInitialized() )
	{
		RecalculateAnchorRect( false );

		// create the list & notification
		TSharedRef<SNotificationList> NotificationList = SNew(SNotificationList);
		TSharedPtr<SNotificationItem> NewItem = NotificationList->AddNotification(Info);

		TSharedRef<SWindow> NotificationWindow = SWindow::MakeNotificationWindow();
		NotificationWindow->SetContent( 
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				NotificationList
			]
		);

		NotificationList->ParentWindowPtr = NotificationWindow;
		NotificationLists.Add( NotificationList );

		if( RootWindowPtr.IsValid() )
		{
			FSlateApplication::Get().AddWindowAsNativeChild( NotificationWindow, RootWindowPtr.Pin().ToSharedRef() );
		}
		else
		{
			FSlateApplication::Get().AddWindow( NotificationWindow );
		}

		if( !FSlateApplication::Get().GetActiveModalWindow().IsValid() )
		{
			NotificationWindow->BringToFront();
		}

		return NewItem;
	}

	return NULL;
}

void FSlateNotificationManager::QueueNotification(FNotificationInfo* Info)
{
	PendingNotifications.Push(Info);
}

void FSlateNotificationManager::GetWindows(TArray< TSharedRef<SWindow> >& OutWindows) const
{
	for( auto Iter(NotificationLists.CreateConstIterator()); Iter; Iter++ )
	{
		TSharedPtr<SNotificationList> NotificationList = *Iter;
		TSharedPtr<SWindow> PinnedWindow = NotificationList->ParentWindowPtr.Pin();
		if( PinnedWindow.IsValid() )
		{
			OutWindows.Add(PinnedWindow.ToSharedRef());
		}
	}
}

void FSlateNotificationManager::Tick()
{
	while (auto* Notification = PendingNotifications.Pop())
	{
		AddNotification(*Notification);
		delete Notification;
	}

	// Check notifications to see if any have timed out & need to be removed.
	// We need to do this here as we cant remove their windows in the normal
	// window-tick callstack (as the SlateWindows array gets corrupted)
	int32 RemovedCount = 0;
	do 
	{
		RemovedCount = 0;
		for( TArray< TSharedPtr<SNotificationList> >::TIterator Iter = NotificationLists.CreateIterator(); Iter; Iter++ )
		{
			TSharedPtr<SNotificationList> NotificationList = *Iter;
			if( NotificationList->bDone )
			{
				RemovedCount = NotificationLists.Remove(NotificationList);
				TSharedPtr<SWindow> PinnedWindow = NotificationList->ParentWindowPtr.Pin();
				if( PinnedWindow.IsValid() )
				{
					PinnedWindow->RequestDestroyWindow();
				}
				break;
			}
		}
	} while(RemovedCount > 0);

	ArrangeNotifications();
}

void FSlateNotificationManager::ArrangeNotifications()
{
	// We may need to reacquire the anchor rect here in case the desktop is resized
	// while a notification is displayed
	const FSlateRect AnchorRect = FSlateApplication::Get().GetWorkArea( AnchorReferenceRect );
	if( AnchorReferenceRect != AnchorRect )
	{
		RecalculateAnchorRect( true );
	}

	// stack items so the newest is at the bottom of the pile
	const FVector2D InitialPoint( AnchorReferenceRect.Right - NotificationManagerConstants::NotificationOffset.X, AnchorReferenceRect.Bottom - NotificationManagerConstants::NotificationOffset.Y );
	FVector2D CurrentPoint = InitialPoint;
	for(int32 Index = NotificationLists.Num() - 1; Index >= 0; --Index )
	{
		TSharedPtr<SNotificationList> NotificationList = NotificationLists[Index];
		TSharedPtr<SWindow> PinnedWindow = NotificationList->ParentWindowPtr.Pin();
		if( PinnedWindow.IsValid() )
		{
			FVector2D DesiredSize = PinnedWindow->GetDesiredSize();
			CurrentPoint.Y -= DesiredSize.Y;
			FVector2D NewPosition( InitialPoint.X - DesiredSize.X, CurrentPoint.Y );
			if( NewPosition != PinnedWindow->GetPositionInScreen() && DesiredSize != PinnedWindow->GetSizeInScreen() )
			{
				PinnedWindow->ReshapeWindow( NewPosition, DesiredSize );
			}
			else if( NewPosition != PinnedWindow->GetPositionInScreen() )
			{
				PinnedWindow->MoveWindowTo( NewPosition );
			}
		}
	}
}

void FSlateNotificationManager::ForceNotificationsInFront( const TSharedRef<SWindow> InWindow )
{
	// check to see if this is a re-entrant call from one of our windows
	for( TArray< TSharedPtr<SNotificationList> >::TIterator Iter = NotificationLists.CreateIterator(); Iter; Iter++ )
	{
		TSharedPtr<SNotificationList> NotificationList = *Iter;
		TSharedPtr<SWindow> PinnedWindow = NotificationList->ParentWindowPtr.Pin();
		if( PinnedWindow.IsValid() )
		{
			if( InWindow == PinnedWindow )
			{
				return;
			}
		}
	}

	// now bring all of our windows back to the front
	for( TArray< TSharedPtr<SNotificationList> >::TIterator Iter = NotificationLists.CreateIterator(); Iter; Iter++ )
	{
		TSharedPtr<SNotificationList> NotificationList = *Iter;
		TSharedPtr<SWindow> PinnedWindow = NotificationList->ParentWindowPtr.Pin();
		if( PinnedWindow.IsValid() && !FSlateApplication::Get().GetActiveModalWindow().IsValid() )
		{
			PinnedWindow->BringToFront();
		}
	}
}
