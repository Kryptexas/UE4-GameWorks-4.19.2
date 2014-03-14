// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	JsonMessageSerializer.h: Declares the FJsonMessageSerializer structure.
=============================================================================*/

#pragma once


/**
 * Implements a message serializer that serializes from and to Json encoded data.
 */
class FJsonMessageSerializer
	: public ISerializeMessages
{
public:

	// Begin ISerializeMessages interface

	virtual bool DeserializeMessage( FArchive& Archive, IMutableMessageContextRef& OutContext ) OVERRIDE;

	virtual bool SerializeMessage( const IMessageContextRef& Context, FArchive& Archive ) OVERRIDE;

	// End ISerializeMessages interface


protected:

	/**
	 * Deserializes a structure.
	 *
	 */
	bool DeserializeStruct( IMutableMessageContextRef& OutContext, const TSharedRef<TJsonReader<UCS2CHAR> >& Reader ) const;

	/**
	 * Serializes a structure.
	 *
	 * @param Data - The structured data to serialize.
	 * @param TypeInfo - The data's type information.
	 * @param Writer - The Json writer to write to.
	 */
	bool SerializeStruct( const void* Data, UStruct& TypeInfo, const TSharedRef<TJsonWriter<UCS2CHAR> >& Writer );
};
