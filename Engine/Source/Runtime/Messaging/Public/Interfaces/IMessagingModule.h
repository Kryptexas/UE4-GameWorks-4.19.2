// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessagingModule.h: Declares the IMessagingModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for messaging modules.
 */
class IMessagingModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a new message bridge.
	 *
	 * Message bridges translate messages between a message bus and another means of
	 * message transportation, such as network sockets.
	 *
	 * @param Address - The bridge's address on the message bus.
	 * @param Bus - The message bus to attach the bridge to.
	 * @param Serializer - The message serializer to use.
	 * @param Transport - The message transport technology to use.
	 *
	 * @return The new message bridge, or NULL if the bridge couldn't be created.
	 *
	 * @see CreateBus
	 */
	virtual IMessageBridgePtr CreateBridge( const FMessageAddress& Address, const IMessageBusRef& Bus, const ISerializeMessagesRef& Serializer, const ITransportMessagesRef& Transport ) = 0;

	/**
	 * Creates a new message bus.
	 *
	 * @param RecipientAuthorizer - An optional recipient authorizer.
	 *
	 * @return The new message bus, or NULL if the bus couldn't be created.
	 *
	 * @see CreateBridge
	 */
	virtual IMessageBusPtr CreateBus( const IAuthorizeMessageRecipientsPtr& RecipientAuthorizer ) = 0;

	/**
	 * Creates a Json message serializer (deprecated).
	 *
	 * @return A new serializer.
	 */
	virtual ISerializeMessagesPtr CreateJsonMessageSerializer( ) = 0;

	/**
	 * Gets the default message bus if it has been initialized.
	 *
	 * @return The default bus.
	 */
	virtual IMessageBusPtr GetDefaultBus( ) const = 0;

public:

	/**
	 * Gets a reference to the messaging module instance.
	 *
	 * @todo gmp - better implementation using dependency injection.
	 *
	 * @return A reference to the Messaging module.
	 */
	static IMessagingModule& Get( )
	{
		return FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");
	}

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessagingModule( ) { }
};
