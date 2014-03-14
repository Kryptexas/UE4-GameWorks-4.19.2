// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FInternationalizationArchive;

/**
 * Interface for Internationalization archive serializers.
 */
class IInternationalizationArchiveSerializer
{
public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IInternationalizationArchiveSerializer( ) { }

	/**
	 * Deserializes a Internationalizationarchive from an archive.
	 *
	 * @param Archive - The archive to serialize from.
	 * @param InternationalizationArchive - Will hold the content of the deserialized Internationalizationarchive.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeArchive( FArchive& Archive, TSharedRef< FInternationalizationArchive > InternationalizationArchive ) = 0;

	/**
	 * Serializes a Internationalizationarchive into an archive.
	 *
	 * @param InternationalizationArchive - The Internationalizationarchive data to serialize.
	 * @param Archive - The archive to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeArchive( TSharedRef< const FInternationalizationArchive > InternationalizationArchive, FArchive& Archive ) = 0;

};