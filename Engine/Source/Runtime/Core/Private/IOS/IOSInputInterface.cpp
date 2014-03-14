// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "IOSInputInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogIOSInput, Log, All);

TArray<TouchInput> FIOSInputInterface::TouchInputStack = TArray<TouchInput>();
FCriticalSection FIOSInputInterface::CriticalSection;

TSharedRef< FIOSInputInterface > FIOSInputInterface::Create(  const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	return MakeShareable( new FIOSInputInterface( InMessageHandler ) );
}

FIOSInputInterface::FIOSInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
	: MessageHandler( InMessageHandler )
{
}

void FIOSInputInterface::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	MessageHandler = InMessageHandler;
}

void FIOSInputInterface::Tick( float DeltaTime )
{
}

void FIOSInputInterface::SendControllerEvents()
{
	FScopeLock Lock(&CriticalSection);

	for(int i = 0; i < FIOSInputInterface::TouchInputStack.Num(); ++i)
	{
		const TouchInput& Touch = FIOSInputInterface::TouchInputStack[i];

		// send input to handler
		if (Touch.Type == TouchBegan)
		{
			MessageHandler->OnTouchStarted( NULL, Touch.Position, Touch.Handle, 0);
		}
		else if (Touch.Type == TouchEnded)
		{
			MessageHandler->OnTouchEnded(Touch.Position, Touch.Handle, 0);
		}
		else
		{
			MessageHandler->OnTouchMoved(Touch.Position, Touch.Handle, 0);
		}
	}

	FIOSInputInterface::TouchInputStack.Empty(0);
}

void FIOSInputInterface::QueueTouchInput(TArray<TouchInput> InTouchEvents)
{
	FScopeLock Lock(&CriticalSection);

	FIOSInputInterface::TouchInputStack.Append(InTouchEvents);
}