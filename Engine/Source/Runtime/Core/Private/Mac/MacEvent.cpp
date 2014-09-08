// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "MacEvent.h"
#include "MacApplication.h"
#include "CocoaThread.h"
#include "CocoaWindow.h"

FMacEvent::FMacEvent(NSEvent* const Event)
: EventData(Event)
{
	check(Event);
	[Event retain];
	
	NSWindow* Window = FMacApplication::FindMacEventWindow(Event);
	if(Window)
	{
		MacWindow = FMacApplication::FindMacWindowByNSWindow((FCocoaWindow*)Window);
	}
	
	CGEventRef CGEvent = [Event CGEvent];
	CGPoint CGMouseLocation = CGEventGetLocation(CGEvent);
	MousePosition.X = CGMouseLocation.x;
	MousePosition.Y = CGMouseLocation.y;
}

FMacEvent::FMacEvent(NSNotification* const Notification, NSWindow* const Window)
: EventData(Notification)
{
	check(Notification);
	[Notification retain];
	
	if(Window)
	{
		MacWindow = FMacApplication::FindMacWindowByNSWindow((FCocoaWindow*)Window);
	}
	
	CGEventRef Event = CGEventCreate(NULL);
	CGPoint CursorPos = CGEventGetLocation(Event);
	MousePosition.X = CursorPos.x;
	MousePosition.Y = CursorPos.y;
	CFRelease(Event);
}

void FMacEvent::SendToGameRunLoop(FMacEvent const* const Event, EMacEventSendMethod SendMethod, NSString* SendMode)
{
	NSThread* GameThread = [NSThread gameThread];
	check(GameThread);
	
	NSRunLoop* GameRunLoop = [NSRunLoop gameRunLoop];
	check(GameRunLoop);
	
	dispatch_block_t Block = ^{ FMacApplication::ProcessEvent(Event); };
	const bool bWait = (SendMethod == EMacEventSendMethod::Sync);
	
	[GameThread performBlock: Block onRunLoop: GameRunLoop forMode: SendMode wait: bWait inMode: NSDefaultRunLoopMode];
}
	
void FMacEvent::SendToGameRunLoop(NSEvent* const Event, EMacEventSendMethod SendMethod, NSString* SendMode)
{
	if(MacApplication)
	{
		FMacEvent* MacEvent = new FMacEvent(Event);
		FMacEvent::SendToGameRunLoop(MacEvent, SendMethod, SendMode);
	}
}

void FMacEvent::SendToGameRunLoop(NSNotification* const Notification, NSWindow* const Window, EMacEventSendMethod SendMethod, NSString* SendMode)
{
	if(MacApplication)
	{
		FMacEvent* MacEvent = new FMacEvent(Notification, Window);
		FMacEvent::SendToGameRunLoop(MacEvent, SendMethod, SendMode);
	}
}

FMacEvent::~FMacEvent(void)
{
	if(EventData)
	{
		[EventData release];
		EventData = nil;
	}
}

NSEvent* FMacEvent::GetEvent(void) const
{
	if(EventData && [EventData isKindOfClass:[NSEvent class]])
	{
		return (NSEvent*)EventData;
	}
	else
	{
		return nil;
	}
}

NSNotification* FMacEvent::GetNotification(void) const
{
	if(EventData && [EventData isKindOfClass:[NSNotification class]])
	{
		return (NSNotification*)EventData;
	}
	else
	{
		return nil;
	}
}

TSharedPtr<FMacWindow> FMacEvent::GetWindow(void) const
{
	return MacWindow;
}

FVector2D FMacEvent::GetMousePosition(void) const
{
	return MousePosition;
}
