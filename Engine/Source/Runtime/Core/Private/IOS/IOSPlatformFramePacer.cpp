// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"


// Collection of events listening for this trigger.
static TArray<FEvent*> ListeningEvents;


/*******************************************************************
 * FIOSFramePacer implementation
 *******************************************************************/

@interface FIOSFramePacer : NSObject
{
    @public
	FEvent *FramePacerEvent;
}

-(void)run:(id)param;
-(void)signal:(id)param;

@end


@implementation FIOSFramePacer

-(void)run:(id)param
{
	NSRunLoop *runloop = [NSRunLoop currentRunLoop];
	CADisplayLink *displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(signal:)];
	displayLink.frameInterval = FIOSPlatformRHIFramePacer::FrameInterval;
    
	[displayLink addToRunLoop:runloop forMode:NSDefaultRunLoopMode];
	[runloop run];
}


-(void)signal:(id)param
{
    for( auto& NextEvent : ListeningEvents )
    {
        NextEvent->Trigger();
    }
}

@end



/*******************************************************************
 * FIOSPlatformRHIFramePacer implementation
 *******************************************************************/

uint32 FIOSPlatformRHIFramePacer::FrameInterval = 1;
FIOSFramePacer* FIOSPlatformRHIFramePacer::FramePacer = nil;

bool FIOSPlatformRHIFramePacer::IsEnabled()
{
    bool bIsRHIFramePacerEnabled = false;
	GConfig->GetBool( TEXT( "PlatformFramePacer" ), TEXT( "IsEnabled" ), bIsRHIFramePacerEnabled, GEngineIni );
    
    
    return bIsRHIFramePacerEnabled;
}


void FIOSPlatformRHIFramePacer::InitWithEvent(FEvent* TriggeredEvent, uint32 InFrameInterval)
{
    FrameInterval = InFrameInterval;
    
    // Create display link thread
	FramePacer = [[FIOSFramePacer alloc] init];
	[NSThread detachNewThreadSelector:@selector(run:) toTarget:FramePacer withObject:nil];
    
    // Only one supported for now, we may want more eventually.
    ListeningEvents.Add( TriggeredEvent );
}


void FIOSPlatformRHIFramePacer::Destroy()
{
    if( FramePacer != nil )
    {
        [FramePacer release];
        FramePacer = nil;
    }
}