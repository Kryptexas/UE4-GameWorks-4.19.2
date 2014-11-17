// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements a message serializer that serializes from and to Binary Json encoded data.
 */
class FBsonMessageSerializer
	: public ISerializeMessages
{
public:

	// ISerializeMessages interface

	virtual bool DeserializeMessage( FArchive& Archive, IMutableMessageContextRef& OutContext ) override
	{
		// not implemented yet
		return false;
	}

	virtual bool SerializeMessage( const IMessageContextRef& Context, FArchive& Archive ) override
	{
		// not implemented yet
		return false;
	}
};
