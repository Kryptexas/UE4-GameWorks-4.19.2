// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"

#ifdef __OBJC__

/* Custom run-loop modes for UE4 that process only certain kinds of events to simulate Windows event ordering. */
extern NSString* UE4NilEventMode; /* Process only mandatory events */
extern NSString* UE4ShowEventMode; /* Process only show window events */
extern NSString* UE4ResizeEventMode; /* Process only resize/move window events */
extern NSString* UE4FullscreenEventMode; /* Process only fullscreen mode events */
extern NSString* UE4CloseEventMode; /* Process only close window events */
extern NSString* UE4IMEEventMode; /* Process only input method events */

@interface NSThread (FCocoaThread)
+ (NSThread*) gameThread; // Returns the main game thread, or nil if has yet to be constructed.
+ (bool) isGameThread; // True if the current thread is the main game thread, else false.
- (bool) isGameThread; // True if this thread object is the main game thread, else false.
- (void) performBlock:(dispatch_block_t)Block onRunLoop:(NSRunLoop*)RunLoop forMode:(NSString*)RunMode wait:(bool)bWait inMode:(NSString*)WaitMode; // Perform Block on RunLoop in RunMode, which must not be the current thread's runloop and wait in WaitMode if bWait is true or return immediately after queueing to the RunLoop.
@end

@interface NSRunLoop (FCocoaRunLoop)
+ (NSRunLoop*)gameRunLoop; // The run loop for the main game thread, or nil if the main game thread has not been started.
- (void)performBlock:(dispatch_block_t)Block forMode:(NSString *)Mode wake:(bool)bWakeRunLoop; // Add Block to the run loop for processing in the given Mode, only waking the run loop if bWakeRunLoop is set otherwise processing occurs the next time the run-loop is processed.
- (NSString*)intendedMode;
@end

@interface FCocoaGameThread : NSThread
- (id)init; // Override that sets the variable backing +[NSThread gameThread], do not override in a subclass.
- (id)initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument; // Override that sets the variable backing +[NSThread gameThread], do not override in a subclass.
- (void)dealloc; // Override that clears the variable backing +[NSThread gameThread], do not override in a subclass.
- (void)main; // Override that sets the variable backing +[NSRunLoop gameRunLoop], do not override in a subclass.
@end

NSString* InGameRunLoopMode(NSArray* AllowedModes);

template<typename ReturnType>
ReturnType PerformBlockOnRunLoop(ReturnType (^Block)(void), NSRunLoop* const RunLoop, NSString* SendMode, NSString* WaitMode)
{
	__block ReturnType ReturnValue;
	[[NSThread currentThread] performBlock:^{ ReturnValue = Block(); } onRunLoop:RunLoop forMode:SendMode wait:true inMode:WaitMode];
	return ReturnValue;
}

void PerformBlockOnRunLoop(dispatch_block_t Block, NSRunLoop* const RunLoop, NSString* SendMode, NSString* WaitMode, bool const bWait);

template<typename ReturnType>
ReturnType MainThreadReturn(ReturnType (^Block)(void), NSString* WaitMode = NSDefaultRunLoopMode)
{
	return PerformBlockOnRunLoop(Block, [NSRunLoop mainRunLoop], NSDefaultRunLoopMode, WaitMode);
}

void MainThreadCall(dispatch_block_t Block, NSString* WaitMode = NSDefaultRunLoopMode, bool const bWait = true);

template<typename ReturnType>
ReturnType GameThreadReturn(ReturnType (^Block)(void), NSString* SendMode = NSDefaultRunLoopMode)
{
	return PerformBlockOnRunLoop(Block, [NSRunLoop gameRunLoop], SendMode, NSDefaultRunLoopMode);
}

void GameThreadCall(dispatch_block_t Block, NSString* SendMode = NSDefaultRunLoopMode, bool const bWait = true);

void RunGameThread(id Target, SEL Selector);

void ProcessGameThreadEvents(void);

#else

class NSThread;
class NSRunLoop;
class FCocoaGameThread;

#endif
