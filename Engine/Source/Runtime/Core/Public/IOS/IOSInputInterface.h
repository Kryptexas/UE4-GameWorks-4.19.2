// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

enum TouchType
{
	TouchBegan,
	TouchMoved,
	TouchEnded,
};

struct TouchInput
{
	int Handle;
	TouchType Type;
	FVector2D LastPosition;
	FVector2D Position;
};

/**
 * Interface class for IOS input devices
 */
class FIOSInputInterface
{
public:

	static TSharedRef< FIOSInputInterface > Create(  const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

public:

	~FIOSInputInterface() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	/** Tick the interface (i.e check for new controllers) */
	void Tick( float DeltaTime );

	/**
	 * Poll for controller state and send events if needed
	 */
	void SendControllerEvents();

	static void QueueTouchInput(TArray<TouchInput> InTouchEvents);

private:

	FIOSInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );
private:

	static TArray<TouchInput> TouchInputStack;

	// protects the input stack used on 2 threads
	static FCriticalSection CriticalSection;

	TSharedRef< FGenericApplicationMessageHandler > MessageHandler;

};