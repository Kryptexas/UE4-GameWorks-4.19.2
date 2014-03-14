// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FInternationalizationManifest;

/**
 * Interface for Internationalization manifest serializers.
 */
class IInternationalizationManifestSerializer
{
public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IInternationalizationManifestSerializer( ) { }

	/**
	 * Deserializes a manifest from an archive.
	 *
	 * @param Archive - The archive to serialize from.
	 * @param Manifest - Will hold the content of the deserialized manifest.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeManifest( FArchive& Archive, TSharedRef< FInternationalizationManifest > Manifest ) = 0;

	/**
	 * Serializes a manifest into an archive.
	 *
	 * @param Manifest - The manifest data to serialize.
	 * @param Archive - The archive to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeManifest( TSharedRef< const FInternationalizationManifest > Manifest, FArchive& Archive ) = 0;

};