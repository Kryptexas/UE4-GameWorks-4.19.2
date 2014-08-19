// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "CocoaThread.h"

#define MAC_SEPARATE_GAME_THREAD 1 // Separate the main & game threads so that we better handle the interaction between the Cocoa's event delegates and UE4's event polling.

static FCocoaGameThread* GCocoaGameThread = nil;
static NSRunLoop* GCocoaGameRunLoop = nil;

@implementation NSThread (FCocoaThread)

+ (NSThread*) gameThread
{
	if(!GCocoaGameThread)
	{
		return [NSThread mainThread];
	}
	return GCocoaGameThread;
}

+ (bool) isGameThread
{
	bool const bIsGameThread = [[NSThread currentThread] isGameThread];
	return bIsGameThread;
}

- (bool) isGameThread
{
	bool const bIsGameThread = (self == GCocoaGameThread);
	return bIsGameThread;
}

- (void) performBlock:(dispatch_block_t)Block onRunLoop:(NSRunLoop*)RunLoop forMode:(NSString*)RunMode wait:(bool)bWait inMode:(NSString*)WaitMode
{
	check(RunLoop);
	if(RunLoop != [NSRunLoop currentRunLoop])
	{
		if(bWait)
		{
			__block bool bDone = false;
			[RunLoop performBlock:^{ Block(); bDone = true; } forMode:RunMode wake:true];
			while(!bDone)
			{
				CFRunLoopRunInMode((CFStringRef)WaitMode, 0.0, true);
			}
		}
		else
		{
			[RunLoop performBlock:Block forMode:RunMode wake:true];
		}
	}
	else
	{
		Block();
	}
}

@end

@implementation NSRunLoop (FCocoaRunLoop)

+ (NSRunLoop*)gameRunLoop
{
	if(!GCocoaGameRunLoop)
	{
		return [NSRunLoop mainRunLoop];
	}
	return GCocoaGameRunLoop;
}

- (void)performBlock:(dispatch_block_t)Block forMode:(NSString *)Mode wake:(bool)bWakeRunLoop
{
	CFRunLoopRef RunLoop = [self getCFRunLoop];
	check(RunLoop);
	CFRunLoopPerformBlock(RunLoop, (CFStringRef)Mode, Block);
	if(bWakeRunLoop)
	{
		CFRunLoopWakeUp(RunLoop);
	}
}

@end

@implementation FCocoaGameThread : NSThread

- (id)init
{
	id Self = [super init];
	if(Self)
	{
		GCocoaGameThread = Self;
	}
	return Self;
}

- (id)initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument
{
	id Self = [super initWithTarget:Target selector:Selector object:Argument];
	if(Self)
	{
		GCocoaGameThread = Self;
	}
	return Self;
}

- (void)dealloc
{
	GCocoaGameThread = nil;
	GCocoaGameRunLoop = nil;
	[super dealloc];
}

- (void)main
{
	GCocoaGameRunLoop = [NSRunLoop currentRunLoop];
	[super main];
}

@end

void PerformBlockOnRunLoop(dispatch_block_t Block, NSRunLoop* const RunLoop, bool const bWait)
{
	[[NSThread currentThread] performBlock:Block onRunLoop:RunLoop forMode:NSDefaultRunLoopMode wait:bWait inMode:NSDefaultRunLoopMode];
}

void MainThreadCall(dispatch_block_t Block, bool const bWait)
{
	[[NSThread currentThread] performBlock:Block onRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode wait:bWait inMode:NSDefaultRunLoopMode];
}

void GameThreadCall(dispatch_block_t Block, bool const bWait)
{
	[[NSThread currentThread] performBlock:Block onRunLoop:[NSRunLoop gameRunLoop] forMode:NSDefaultRunLoopMode wait:bWait inMode:NSDefaultRunLoopMode];
}

void RunGameThread(id Target, SEL Selector)
{
#if MAC_SEPARATE_GAME_THREAD
	// Create a separate game thread and set it to the stack size to be the same as the main thread default of 8MB ( http://developer.apple.com/library/mac/#qa/qa1419/_index.html )
	FCocoaGameThread* GameThread = [[FCocoaGameThread alloc] initWithTarget:Target selector:Selector object:nil];
	[GameThread setStackSize:8*1024*1024];
	[GameThread start];
#else
	[Target performSelector:Selector withObject:nil];
#endif
}

void ProcessGameThreadEvents(void)
{
#if MAC_SEPARATE_GAME_THREAD
	// Make one pass through the loop, processing all events
	CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
#else
	SCOPED_AUTORELEASE_POOL;
	while( NSEvent *Event = [NSApp nextEventMatchingMask: NSAnyEventMask untilDate: nil inMode: NSDefaultRunLoopMode dequeue: true] )
	{
		[NSApp sendEvent: Event];
	}
#endif
}
