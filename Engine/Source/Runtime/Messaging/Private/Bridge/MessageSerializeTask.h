// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageSerializeTask.h: Declares the FMessageSerializeTask class.
=============================================================================*/

#pragma once


/**
 * Implements an asynchronous task for serializing a message.
 */
class FMessageSerializeTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageContext - The context of the message to serialize.
	 * @param InMessageData - Will hold the serialized message data.
	 * @param InSerializer - The serializer to use.
	 */
	FMessageSerializeTask( IMessageContextRef InMessageContext, FMessageDataRef InMessageData, ISerializeMessagesRef InSerializer )
		: MessageContext(InMessageContext)
		, MessageData(InMessageData)
		, SerializerPtr(InSerializer)
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
		ISerializeMessagesPtr Serializer = SerializerPtr.Pin();

		if (Serializer.IsValid())
		{
			if (Serializer->SerializeMessage(MessageContext, *MessageData))
			{
				MessageData->UpdateState(EMessageDataState::Complete);
			}
			else
			{
				MessageData->UpdateState(EMessageDataState::Invalid);
			}
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
		return TEXT("FMessageSerializeTask");
	}

private:

	// Holds the context of the message to serialize.
	IMessageContextRef MessageContext;

	// Holds a reference to the message data.
	FMessageDataRef MessageData;

	// Holds a reference to the serializer to use.
	TWeakPtr<ISerializeMessages, ESPMode::ThreadSafe> SerializerPtr;
};
