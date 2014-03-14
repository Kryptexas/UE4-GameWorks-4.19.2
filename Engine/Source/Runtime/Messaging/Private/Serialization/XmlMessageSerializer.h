// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	XmlMessageSerializer.h: Declares the FXmlMessageSerializer structure.
=============================================================================*/

#pragma once


/**
 * Implements a message serializer that serializes from and to XML encoded data.
 */
class FXmlMessageSerializer
	: public ISerializeMessages
{
	// Begin ISerializeMessages interface

	virtual bool DeserializeMessage( FArchive& Archive, IMutableMessageContextRef& OutContext ) OVERRIDE
	{
		// not implemented yet
		return false;
	}

	virtual bool SerializeMessage( const IMessageContextRef& Context, FArchive& Archive ) OVERRIDE
	{
		// not implemented yet
		return false;
	}

	// End ISerializeMessages interface
};
