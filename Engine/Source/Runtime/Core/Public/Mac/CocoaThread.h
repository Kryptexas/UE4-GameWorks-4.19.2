// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"

#ifdef __OBJC__

@interface NSThread (FCocoaThread)
+ (NSThread*) gameThread; // Returns the main game thread, or nil if has yet to be constructed.
+ (bool) isGameThread; // True if the current thread is the main game thread, else false.
- (bool) isGameThread; // True if this thread object is the main game thread, else false.
- (void) performBlock:(dispatch_block_t)Block onRunLoop:(NSRunLoop*)RunLoop forMode:(NSString*)RunMode wait:(bool)bWait inMode:(NSString*)WaitMode; // Perform Block on RunLoop in RunMode, which must not be the current thread's runloop and wait in WaitMode if bWait is true or return immediately after queueing to the RunLoop.
@end

@interface NSRunLoop (FCocoaRunLoop)
+ (NSRunLoop*)gameRunLoop; // The run loop for the main game thread, or nil if the main game thread has not been started.
- (void)performBlock:(dispatch_block_t)Block forMode:(NSString *)Mode wake:(bool)bWakeRunLoop; // Add Block to the run loop for processing in the given Mode, only waking the run loop if bWakeRunLoop is set otherwise processing occurs the next time the run-loop is processed.
@end

@interface FCocoaGameThread : NSThread
- (id)init; // Override that sets the variable backing +[NSThread gameThread], do not override in a subclass.
- (id)initWithTarget:(id)Target selector:(SEL)Selector object:(id)Argument; // Override that sets the variable backing +[NSThread gameThread], do not override in a subclass.
- (void)dealloc; // Override that clears the variable backing +[NSThread gameThread], do not override in a subclass.
- (void)main; // Override that sets the variable backing +[NSRunLoop gameRunLoop], do not override in a subclass.
@end

template<typename ReturnType>
ReturnType PerformBlockOnRunLoop(ReturnType (^Block)(void), NSRunLoop* const RunLoop)
{
	__block ReturnType ReturnValue;
	[[NSThread currentThread] performBlock:^{ ReturnValue = Block(); } onRunLoop:RunLoop forMode:NSDefaultRunLoopMode wait:true inMode:NSDefaultRunLoopMode];
	return ReturnValue;
}

void PerformBlockOnRunLoop(dispatch_block_t Block, NSRunLoop* const RunLoop, bool const bWait);

template<typename ReturnType>
ReturnType MainThreadReturn(ReturnType (^Block)(void))
{
	return PerformBlockOnRunLoop(Block, [NSRunLoop mainRunLoop]);
}

void MainThreadCall(dispatch_block_t Block, bool const bWait);

template<typename ReturnType>
ReturnType GameThreadReturn(ReturnType (^Block)(void))
{
	return PerformBlockOnRunLoop(Block, [NSRunLoop gameRunLoop]);
}

void GameThreadCall(dispatch_block_t Block, bool const bWait);

void RunGameThread(id Target, SEL Selector);

void ProcessGameThreadEvents(void);

#else

class NSThread;
class NSRunLoop;
class FCocoaGameThread;

#endif
