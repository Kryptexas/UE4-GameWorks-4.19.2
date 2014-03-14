// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageForwardTask.h: Declares the FMessageForwardTask class.
=============================================================================*/

#pragma once


/**
 * Implements an asynchronous task for deserializing a message.
 */
class FMessageForwardTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageContext - The context of the message to forward.
	 * @param InBus - The message bus to forward the message to.
	 * @param InBridge - The sender that is forwarding the message.
	 */
	FMessageForwardTask( IMessageContextRef InMessageContext, IMessageBusRef InBus, ISendMessagesRef InSender )
		: BusPtr(InBus)
		, MessageContext(InMessageContext)
		, Sender(InSender)
	{ }

public:

	/**
	 * Performs the actual task.
	 *
	 * @param CurrentThread - The thread that this task is executing on.
	 * @param MyCompletionGraphEvent - The completion event.
	 */
	void DoTask( ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent )
	{
		IMessageBusPtr Bus = BusPtr.Pin();

		if (Bus.IsValid())
		{
			Bus->Forward(MessageContext, MessageContext->GetRecipients(), EMessageScope::Process, FTimespan::Zero(), Sender);
		}
	}
	
	/**
	 * Returns the name of the thread that this task should run on.
	 *
	 * @return Thread name.
	 */
	ENamedThreads::Type GetDesiredThread( )
	{
		return ENamedThreads::AnyThread;
	}

public:

	/**
	 * Gets the task's stats tracking identifier.
	 *
	 * @return Stats identifier.
	 */
	static TStatId GetStatId( );

	/**
	 * Gets the mode for tracking subsequent tasks.
	 *
	 * @return Always track subsequent tasks.
	 */
	static ESubsequentsMode::Type GetSubsequentsMode( ) 
	{ 
		return ESubsequentsMode::TrackSubsequents; 
	}

	/**
	 * Gets the name of this task.
	 *
	 * @return Task name.
	 */
	static const TCHAR* GetTaskName( )
	{
		return TEXT("FMessageForwardTask");
	}

private:

	// Holds a pointer to the message bus to forward the message to.
	IMessageBusWeakPtr BusPtr;

	// Holds the context of the message to forward.
	IMessageContextRef MessageContext;

	// Holds a reference to the message sender.
	ISendMessagesRef Sender;
};
