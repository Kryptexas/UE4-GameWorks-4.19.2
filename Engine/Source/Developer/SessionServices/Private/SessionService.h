// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionService.h: Declares the FSessionService class.
=============================================================================*/

#pragma once


/**
 * Implements an application session service.
 */
class FSessionService
	: public FOutputDevice
	, public ISessionService
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageBus - The message bus to use.
	 */
	FSessionService( const IMessageBusRef& InMessageBus );

	/**
	 * Destructor.
	 */
	~FSessionService( );


public:

	// Begin FOutputDevice interface

	virtual void Serialize( const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE
	{
		SendLog(Data, Verbosity, Category);
	}

	// End FOutputDevice interface


public:

	// Begin ISessionService interface

	virtual bool IsRunning( ) OVERRIDE
	{
		return MessageEndpoint.IsValid();
	}

	virtual bool Start( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE;

	// End ISessionService interface


protected:

	/**
	 * Sends a log message to subscribed recipients.
	 *
	 * @param Data - The log message data.
	 * @param Verbosity - The verbosity type.
	 * @param Category - The log category.
	 */
	void SendLog( const TCHAR* Data, ELogVerbosity::Type Verbosity = ELogVerbosity::Log, const class FName& Category = "Log" );

	/**
	 * Sends a notification to the specified recipient.
	 *
	 * @param NotificationText - The notification text.
	 * @param Recipient - The recipient's message address.
	 */
	void SendNotification( const TCHAR* NotificationText, const FMessageAddress& Recipient );

	/**
	 * Publishes a ping response.
	 *
	 * @param Context - The context of the received Ping message.
	 */
	void SendPong( const IMessageContextRef& Context );


private:

	// Handles message bus shutdowns.
	void HandleMessageEndpointShutdown( );

	// Handles FSessionServiceLogSubscribe messages.
	void HandleSessionLogSubscribeMessage( const FSessionServiceLogSubscribe& Message, const IMessageContextRef& Context );

	// Handles FSessionServiceLogUnsubscribe messages.
	void HandleSessionLogUnsubscribeMessage( const FSessionServiceLogUnsubscribe& Message, const IMessageContextRef& Context );

	// Handles FSessionServicePing messages.
	void HandleSessionPingMessage( const FSessionServicePing& Message, const IMessageContextRef& Context );


private:

	// Holds the list of log subscribers.
	TArray<FMessageAddress> LogSubscribers;

	// Holds a critical section for the log subscribers array.
	FCriticalSection LogSubscribersLock;

	// Holds a weak pointer to the message bus.
	IMessageBusWeakPtr MessageBusPtr;

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;
};