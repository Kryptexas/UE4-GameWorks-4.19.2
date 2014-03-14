// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageBridgeBuilder.h: Declares the FMessageBridgeBuilder class.
=============================================================================*/

#pragma once


/**
 * Implements a message bridge builder.
 */
class FMessageBridgeBuilder
{
public:

	/**
	 * Default constructor.
	 */
	FMessageBridgeBuilder( )
		: Address(FMessageAddress::NewGuid())
		, BusPtr(IMessagingModule::Get().GetDefaultBus())
		, Disabled(false)
		, Transport(NULL)
	{ }

	/*
	 * Creates and initializes a new instance.
	 *
	 * @param InBus - The message bus to attach the bridge to.
	 */
	FMessageBridgeBuilder( const IMessageBusRef& Bus )
		: Address(FMessageAddress::NewGuid())
		, BusPtr(Bus)
		, Disabled(false)
		, Transport(NULL)
	{ }

public:

	/**
	 * Disables the bridge.
	 *
	 * @return This instance (for method chaining).
	 */
	FMessageBridgeBuilder& ThatIsDisabled( )
	{
		Disabled = true;

		return *this;
	}

	/**
	 * Configures the bridge to use a custom message serializer.\
	 *
	 * @param CustomSerializer - The custom serializer.
	 *
	 * @return This instance (for method chaining).
	 */
	FMessageBridgeBuilder& UsingCustomSerializer( const ISerializeMessagesRef& CustomSerializer )
	{
		Serializer = CustomSerializer;

		return *this;
	}

	/**
	 * Configures the bridge to use a specific message transport technology.
	 *
	 * @param InTransport - The transport technology to use.
	 *
	 * @return This instance (for method chaining).
	 */
	FMessageBridgeBuilder& UsingTransport( const ITransportMessagesRef& InTransport )
	{
		Transport = InTransport;

		return *this;
	}

	/**
	 * Configures the bridge to use a binary message serializer.
	 *
	 * @return This instance (for method chaining).
	 */
	FMessageBridgeBuilder& UsingJsonSerializer( )
	{
		Serializer = IMessagingModule::Get().CreateJsonMessageSerializer();

		return *this;
	}

	/**
	 * Sets the bridge's address.
	 *
	 * If no address is specified, one will be generated automatically.
	 *
	 * @param InAddress - The address to set.
	 *
	 * @return This instance (for method chaining).
	 */
	FMessageBridgeBuilder& WithAddress( const FMessageAddress& InAddress )
	{
		Address = InAddress;

		return *this;
	}

public:

	/**
	 * Builds the message bridge as configured.
	 *
	 * @return A new message bridge, or NULL if it couldn't be built.
	 */
	IMessageBridgePtr Build( )
	{
		IMessageBridgePtr Bridge;

		check(Transport.IsValid());

		IMessageBusPtr Bus = BusPtr.Pin();

		if (Bus.IsValid())
		{
			Bridge = IMessagingModule::Get().CreateBridge(Address, Bus.ToSharedRef(), Serializer.ToSharedRef(), Transport.ToSharedRef());

			if (Bridge.IsValid())
			{
				if (Disabled)
				{
					Bridge->Disable();
				}
				else
				{
					Bridge->Enable();
				}
			}
		}

		return Bridge;
	}

	/**
	 * Implicit conversion operator to build the message bridge as configured.
	 *
	 * @return A new message bridge, or NULL if it couldn't be built.
	 */
	operator IMessageBridgePtr( )
	{
		return Build();
	}

private:

	// Holds the bridge's address.
	FMessageAddress Address;

	// Holds a weak pointer to the message bus to attach to.
	IMessageBusWeakPtr BusPtr;

	// Holds a flag indicating whether the bridge should be disabled.
	bool Disabled;

	// Holds a reference to the message serializer.
	ISerializeMessagesPtr Serializer;

	// Holds a reference to the message transport technology.
	ITransportMessagesPtr Transport;
};
