// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageHandler.h: Declares the IMessageHandler interface and related helper templates.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageHandler.
 */
typedef TSharedPtr<class IMessageHandler, ESPMode::ThreadSafe> IMessageHandlerPtr;

/**
 * Type definition for shared references to instances of IMessageHandler.
 */
typedef TSharedRef<class IMessageHandler, ESPMode::ThreadSafe> IMessageHandlerRef;


/**
 * Interface for message handlers.
 */
class IMessageHandler
{
public:

	/**
	 * Gets the message type handled by this handler.
	 *
	 * @return The name of the message type.
	 */
	virtual const FName GetHandledMessageType( ) const = 0;

	/**
	 * Handles the specified message.
	 *
	 * @param Context - The context of the message to handle.
	 */
	virtual void HandleMessage( const IMessageContextRef& Context ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageHandler( ) { }
};


/**
 * Template for message handler function pointer types.
 *
 * @param MessageType - The type of message to handle.
 * @param HandlerType - The type of the handler class.
 */
template<typename MessageType, typename HandlerType>
struct TMessageHandlerFunc
{
	typedef void (HandlerType::*Type)( const MessageType&, const IMessageContextRef& );
};


/**
 * Template for message handlers.
 *
 * @param MessageType - The type of message to handle.
 * @param HandlerType - The type of the handler class.
 */
template<typename MessageType, typename HandlerType>
class TMessageHandler
	: public IMessageHandler
{
public:

	/**
	 * Creates and initializes a new message handler.
	 *
	 * @param InHandler - The object that will handle the messages.
	 * @param InHandlerFunc - The object's message handling function.
	 */
	TMessageHandler( HandlerType* InHandler, typename TMessageHandlerFunc<MessageType, HandlerType>::Type InHandlerFunc )
		: Handler(InHandler)
		, HandlerFunc(InHandlerFunc)
	{
		check(InHandler != NULL);
	}

public:

	// Begin IMessageHandler interface
	
	virtual const FName GetHandledMessageType( ) const OVERRIDE
	{
		return MessageType::StaticStruct()->GetFName();
	}

	virtual void HandleMessage( const IMessageContextRef& Context ) OVERRIDE
	{
		(Handler->*HandlerFunc)(*static_cast<const MessageType*>(Context->GetMessage()), Context);
	}
	
	// End IMessageHandler interface
	
private:

	// Holds a pointer to the object handling the messages.
	HandlerType* Handler;

	// Holds a pointer to the actual handler function.
	typename TMessageHandlerFunc<MessageType, HandlerType>::Type HandlerFunc;
};
