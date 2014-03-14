// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageDeserializeTask.h: Declares the FMessageDeserializeTask class.
=============================================================================*/

#pragma once


/**
 * Implements an asynchronous task for deserializing a message.
 */
class FMessageDeserializeTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * InMessageTypeInfo - The message's type information (may be nullptr if the type information hasn't been discovered yet).
	 * @param InSerializer - The serializer to use.
	 * @param InThread - The name of the thread to deserialize the message on.
	 */
	FMessageDeserializeTask( TWeakObjectPtr<UScriptStruct> InMessageTypeInfo, ISerializeMessagesRef InSerializer, ENamedThreads::Type InThread )
		: MessageTypeInfoPtr(InMessageTypeInfo)
		, Serializer(InSerializer)
		, Thread(InThread)
	{ }

public:

	/**
	 * Performs the actual task.
	 *
	 * @param CurrentThread - The thread that this task is executing on.
	 * @param MyCompletionGraphEvent - The completion event.
	 *
	 * @todo gmp: implement asynchronous message deserialization
	 */
	void DoTask( ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent )
	{
/*		UScriptStruct* MessageTypeInfo = MessageTypeInfoPtr.Get();


		MessageTypeInfo = FindObjectSafe<UScriptStruct>(ANY_PACKAGE, );

		if (MessageTypeInfo != nullptr)
		{

		}
		else if (FTaskGraphInterface::Get().GetCurrentThreadIfKnown() == ENamedThreads::GameThread)
		{

		}*/
	}
	
	/**
	 * Returns the name of the thread that this task should run on.
	 *
	 * @return Thread name.
	 */
	ENamedThreads::Type GetDesiredThread( )
	{
		return Thread;
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
		return TEXT("FMessageDeserializeTask");
	}

private:

	// Holds the message's type information.
	TWeakObjectPtr<UScriptStruct> MessageTypeInfoPtr;

	// Holds a reference to the serializer to use.
	ISerializeMessagesRef Serializer;

	// Holds the name of the thread that deserialization should be run on.
	ENamedThreads::Type Thread;
};
