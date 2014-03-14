// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IInternationalizationArchiveSerializer.h"
#include "Json.h"

/**
 * Used to arrange Internationalization archive data in a hierarchy based on namespace prior to json serialization.
 */
struct FStructuredArchiveEntry
{
public:
	FStructuredArchiveEntry( const FString& InNamespace )
		: Namespace( InNamespace )
	{
	}

	const FString Namespace;
	TArray< TSharedPtr< FStructuredArchiveEntry > > SubNamespaces;
	TArray< TSharedPtr< class FArchiveEntry > > ArchiveEntries;
};

/**
 * Implements a InternationalizationArchive serializer that serializes to and from Json encoded data.
 */
class CORE_API FInternationalizationArchiveJsonSerializer : public IInternationalizationArchiveSerializer
{
public:
	FInternationalizationArchiveJsonSerializer() { }

	/**
	 * Deserializes a Internationalization archive from archive.
	 *
	 * @param Archive - The archive to serialize from.
	 * @param InternationalizationArchive - The populated Internationalization archive.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeArchive( FArchive& Archive, TSharedRef< FInternationalizationArchive > InternationalizationArchive ) OVERRIDE;

	/**
	 * Deserializes a Internationalization archive from a string.
	 *
	 * @param InStr - The string to serialize from.
	 * @param InternationalizationArchive - The populated Internationalization archive.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeArchive( const FString& InStr, TSharedRef< FInternationalizationArchive > InternationalizationArchive );

	/**
	 * Deserializes a Internationalization archive from a JSON object.
	 *
	 * @param InJsonObj - The JSON object to serialize from.
	 * @param InternationalizationArchive - The populated Internationalization archive.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeArchive( TSharedRef< FJsonObject > InJsonObj, TSharedRef< FInternationalizationArchive > InternationalizationArchive );

	/**
	 * Serializes a Internationalization archive into an archive.
	 *
	 * @param InternationalizationArchive - The Internationalization archive data to serialize.
	 * @param Archive - The archive to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeArchive( TSharedRef< const FInternationalizationArchive > InternationalizationArchive, FArchive& Archive ) OVERRIDE;

	/**
	 * Serializes a Internationalization archive to a string.
	 *
	 * @param InternationalizationArchive - The Internationalization archive data to serialize.
	 * @param Str - The string to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeArchive( TSharedRef< const FInternationalizationArchive > InternationalizationArchive, FString& Str );

	/**
	 * Serializes a Internationalization archive to a JSON object.
	 *
	 * @param InternationalizationArchive - The Internationalization archive data to serialize.
	 * @param JsonObj - The JSON object to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeArchive( TSharedRef< const FInternationalizationArchive > InternationalizationArchive, TSharedRef< FJsonObject > JsonObj );

protected:
	/**
	 * Convert a JSON object to a Internationalization archive.
	 *
	 * @param InJsonObj - The JSON object to serialize from.
	 * @param InternationalizationArchive - The Internationalization archive that will store the data.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	bool DeserializeInternal( TSharedRef< FJsonObject > InJsonObj, TSharedRef< FInternationalizationArchive > InternationalizationArchive );

	/**
	 * Convert a Internationalization archive to a JSON object.
	 *
	 * @param InInternationalizationArchive - The Internationalization archive object to serialize from.
	 * @param JsonObj - The Json object that will store the data.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	bool SerializeInternal( TSharedRef< const FInternationalizationArchive > InInternationalizationArchive, TSharedRef< FJsonObject > JsonObj );

	/**
	 * Recursive function that will traverse the JSON object and populate a Internationalization archive.
	 *
	 * @param InJsonObj - The JSON object.
	 * @param ParentNamespace - The namespace of the parent JSON object.
	 * @param InternationalizationArchive - The Internationalization archive that will store the data.
	 *
	 * @return true if successful, false otherwise.
	 */
	bool JsonObjToArchive( TSharedRef< FJsonObject > InJsonObj, FString ParentNamespace, TSharedRef< FInternationalizationArchive > InternationalizationArchive );

	/**
	 * Takes a Internationalization archive and arranges the data into a hierarchy based on namespace.
	 *
	 * @param InInternationalizationArchive - The Internationalization archive.
	 * @param RootElement - The root element of the structured data.
	 */
	void GenerateStructuredData( TSharedRef< const FInternationalizationArchive > InInternationalizationArchive, TSharedPtr< FStructuredArchiveEntry > RootElement );

	/**
	 * Goes through the structured, hierarchy based, archive data and does a non-culture specific sort on namespaces and default text.
	 *
	 * @param RootElement - The root element of the structured data.
	 */
	void SortStructuredData( TSharedPtr< FStructuredArchiveEntry > InElement );

	/**
	 * Populates a JSON object from Internationalization archive data that has been structured based on namespace.
	 *
	 * @param InElement - Internationalization archive data structured based on namespace.
	 * @param JsonObj - JSON object to be populated.
	 */
	void StructuredDataToJsonObj(TSharedPtr< const FStructuredArchiveEntry > InElement, TSharedRef< FJsonObject > JsonObj );


public:
	static const FString TAG_NAMESPACE;
	static const FString TAG_CHILDREN;
	static const FString TAG_SUBNAMESPACES;
	static const FString TAG_DEPRECATED_DEFAULTTEXT;
	static const FString TAG_DEPRECATED_TRANSLATEDTEXT;
	static const FString TAG_OPTIONAL;
	static const FString TAG_SOURCE;
	static const FString TAG_SOURCE_TEXT;
	static const FString TAG_TRANSLATION;
	static const FString TAG_TRANSLATION_TEXT;
	static const FString TAG_METADATA_KEY;
	static const FString NAMESPACE_DELIMITER;
};