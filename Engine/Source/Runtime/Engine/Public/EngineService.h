// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EngineService.h: Declares the FEngineService class.
=============================================================================*/

#pragma once


/**
 * Implements an application session service.
 */
class FEngineService
{
public:

	/**
	 * Default constructor.
	 */
	ENGINE_API FEngineService( );

protected:

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
	 * @param Context - The message context of the received Ping message.
	 */
	void SendPong( const IMessageContextRef& Context );

private:

	// Handles FEngineServiceAuthGrant messages.
	void HandleAuthGrantMessage( const FEngineServiceAuthGrant& Message, const IMessageContextRef& Context );

	// Handles FEngineServiceAuthGrant messages.
	void HandleAuthDenyMessage( const FEngineServiceAuthDeny& Message, const IMessageContextRef& Context );

	// Handles FEngineServiceExecuteCommand messages.
	void HandleExecuteCommandMessage( const FEngineServiceExecuteCommand& Message, const IMessageContextRef& Context );

	// Handles FEngineServicePing messages.
	void HandlePingMessage( const FEngineServicePing& Message, const IMessageContextRef& Context );

	// Handles FEngineServiceTerminate messages.
	void HandleTerminateMessage( const FEngineServiceTerminate& Message, const IMessageContextRef& Context );

private:

	// Holds the list of users that are allowed to interact with this session.
	TArray<FString> AuthorizedUsers;

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;
};
