// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "MacWindow.h"

#ifndef __OBJC__

class NSEvent;
class NSNotification;
class NSObject;

#endif

enum EMacEventSendMethod
{
	Async,
	Sync
};

class FMacEvent
{
	// Constructor for an NSEvent based FMacEvent
	FMacEvent(NSEvent* const Event);
	
	// Constructor for an NSNotification based FMacEvent
	FMacEvent(NSNotification* const Notification, NSWindow* const Window);
	
	// Send the event to the game run loop for processing using the specified SendMethod.
	static void SendToGameRunLoop(FMacEvent const* const Event, EMacEventSendMethod SendMethod);
	
public:
	// Send an NSEvent to the Game run loop as an FMacEvent using the specified SendMethod
	static void SendToGameRunLoop(NSEvent* const Event, EMacEventSendMethod SendMethod);
	
	// Send an NSNotification to the Game run loop as an FMacEvent using the specified SendMethod
	static void SendToGameRunLoop(NSNotification* const Notification, NSWindow* const Window, EMacEventSendMethod SendMethod);
	
	// Destructor
	~FMacEvent(void);
	
	// Get the NSEvent for this FMacEvent, nil if not an NSEvent.
	NSEvent* GetEvent(void) const;
	
	// Get the NSNotification for this FMacEvent, nil if not an NSNotification.
	NSNotification* GetNotification(void) const;
	
	// Get the FMacWindow for this FMacEvent, may be nullptr if not a window event, or for a non-FMacWindow.
	TSharedPtr<FMacWindow> GetWindow(void) const;
	
	// Get the mouse position for this FMacEvent at the time of posting on the AppKit Main Thread.
	FVector2D GetMousePosition(void) const;
	
private:
	TSharedPtr<FMacWindow> MacWindow; // The retained UE4 window for this event, or nullptr
	NSObject* EventData; // The retained NSEvent or NSNotification for this event or nil.
	FVector2D MousePosition; // The mouse position for the event in UE4 coordinates.
};


