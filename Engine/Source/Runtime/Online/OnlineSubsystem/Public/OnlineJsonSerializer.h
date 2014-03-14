// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Json.h"
#include "OnlineKeyValuePair.h"

/**
 * Macros used to generate a serialization function for a class derived from FJsonSerializable
 */
 #define BEGIN_ONLINE_JSON_SERIALIZER \
	virtual void Serialize(FOnlineJsonSerializerBase& Serializer) OVERRIDE \
	{ \
		Serializer.StartObject();

#define END_ONLINE_JSON_SERIALIZER \
		Serializer.EndObject(); \
	}

#define ONLINE_JSON_SERIALIZE(JsonName, JsonValue) \
		Serializer.Serialize(TEXT(JsonName), JsonValue)

#define ONLINE_JSON_SERIALIZE_ARRAY(JsonName, JsonArray) \
	    Serializer.SerializeArray(TEXT(JsonName), JsonArray)

#define ONLINE_JSON_SERIALIZE_MAP(JsonName, JsonMap) \
		Serializer.SerializeMap(TEXT(JsonName), JsonMap)

#define ONLINE_JSON_SERIALIZE_SERIALIZABLE(JsonName, JsonValue) \
		JsonValue.Serialize(Serializer)

/** Array of string data */
typedef TArray<FString> FJsonSerializableArray;

/** Maps a key to a value */
typedef FOnlineKeyValuePairs<FString, FString> FJsonSerializableKeyValueMap;
typedef FOnlineKeyValuePairs<FString, int32> FJsonSerializableKeyValueMapInt;

/**
 * Base interface used to serialize to/from JSON. Hides the fact there are separate read/write classes
 */
struct FOnlineJsonSerializerBase
{
	virtual bool IsLoading() const = 0;
	virtual bool IsSaving() const = 0;
	virtual void StartObject() = 0;
	virtual void StartObject(const FString& Name) = 0;
	virtual void EndObject() = 0;
	virtual void Serialize(const TCHAR* Name, int32& Value) = 0;
	virtual void Serialize(const TCHAR* Name, uint32& Value) = 0;
	virtual void Serialize(const TCHAR* Name, bool& Value) = 0;
	virtual void Serialize(const TCHAR* Name, FString& Value) = 0;
	virtual void Serialize(const TCHAR* Name, float& Value) = 0;
	virtual void SerializeArray(FJsonSerializableArray& Array) = 0;
	virtual void SerializeArray(const TCHAR* Name, FJsonSerializableArray& Value) = 0;
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMap& Map) = 0;
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMapInt& Map) = 0;
};

/**
 * Implements the abstract serializer interface hiding the underlying writer object
 */
class FOnlineJsonSerializerWriter :
	public FOnlineJsonSerializerBase
{
	/** The object to write the JSON output to */
	TSharedRef<TJsonWriter<> > JsonWriter;

public:

	/**
	 * Initializes the writer object
	 *
	 * @param InJsonWriter the object to write the JSON output to
	 */
	FOnlineJsonSerializerWriter(TSharedRef<TJsonWriter<> > InJsonWriter) :
		JsonWriter(InJsonWriter)
	{
	}

    /** Is the JSON being read from */
	virtual bool IsLoading() const OVERRIDE { return false; }
    /** Is the JSON being written to */
	virtual bool IsSaving() const OVERRIDE { return true; }

	/**
	 * Starts a new object "{"
	 */
	virtual void StartObject() OVERRIDE
	{
		JsonWriter->WriteObjectStart();
	}

	/**
	 * Starts a new object "{"
	 */
	virtual void StartObject(const FString& Name) OVERRIDE
	{
		JsonWriter->WriteObjectStart(Name);
	}
	/**
	 * Completes the definition of an object "}"
	 */
	virtual void EndObject() OVERRIDE
	{
		JsonWriter->WriteObjectEnd();
	}
	/**
	 * Writes the field name and the corresponding value to the JSON data
	 *
	 * @param Name the field name to write out
	 * @param Value the value to write out
	 */
	virtual void Serialize(const TCHAR* Name, int32& Value) OVERRIDE
	{
		JsonWriter->WriteValue(Name, (const float)Value);
	}
	/**
	 * Writes the field name and the corresponding value to the JSON data
	 *
	 * @param Name the field name to write out
	 * @param Value the value to write out
	 */
	virtual void Serialize(const TCHAR* Name, uint32& Value) OVERRIDE
	{
		JsonWriter->WriteValue(Name, (const float)Value);
	}
	/**
	 * Writes the field name and the corresponding value to the JSON data
	 *
	 * @param Name the field name to write out
	 * @param Value the value to write out
	 */
	virtual void Serialize(const TCHAR* Name, bool& Value) OVERRIDE
	{
		JsonWriter->WriteValue(Name, (const bool)Value);
	}
	/**
	 * Writes the field name and the corresponding value to the JSON data
	 *
	 * @param Name the field name to write out
	 * @param Value the value to write out
	 */
	virtual void Serialize(const TCHAR* Name, FString& Value) OVERRIDE
	{
		JsonWriter->WriteValue(Name, Value);
	}
	/**
	 * Writes the field name and the corresponding value to the JSON data
	 *
	 * @param Name the field name to write out
	 * @param Value the value to write out
	 */
	virtual void Serialize(const TCHAR* Name, float& Value) OVERRIDE
	{
		JsonWriter->WriteValue(Name, (const float)Value);
	}
	/**
	 * Serializes an array of values
	 *
	 * @param Name the name of the property to serialize
	 * @param Array the array to serialize
	 */
	virtual void SerializeArray(FJsonSerializableArray& Array) OVERRIDE
	{
		JsonWriter->WriteArrayStart();
		// Iterate all of values
		for (FJsonSerializableArray::TIterator ArrayIt(Array); ArrayIt; ++ArrayIt)
		{
			JsonWriter->WriteValue(*ArrayIt);
		}
		JsonWriter->WriteArrayEnd();
	}
	/**
	 * Serializes an array of values with an identifier
	 *
	 * @param Name the name of the property to serialize
	 * @param Array the array to serialize
	 */
	virtual void SerializeArray(const TCHAR* Name, FJsonSerializableArray& Array) OVERRIDE
	{
		JsonWriter->WriteArrayStart(Name);
		// Iterate all of values
		for (FJsonSerializableArray::TIterator ArrayIt(Array); ArrayIt; ++ArrayIt)
		{
			JsonWriter->WriteValue(*ArrayIt);
		}
		JsonWriter->WriteArrayEnd();
	}
	/**
	 * Serializes the keys & values for map
	 *
	 * @param Name the name of the property to serialize
	 * @param Map the map to serialize
	 */
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMap& Map) OVERRIDE
	{
		JsonWriter->WriteObjectStart(Name);
		// Iterate all of the keys and their values
		for (FJsonSerializableKeyValueMap::TIterator KeyValueIt(Map); KeyValueIt; ++KeyValueIt)
		{
			Serialize(*(KeyValueIt.Key()), KeyValueIt.Value());
		}
		JsonWriter->WriteObjectEnd();
	}
	/**
	 * Serializes the keys & values for map
	 *
	 * @param Name the name of the property to serialize
	 * @param Map the map to serialize
	 */
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMapInt& Map) OVERRIDE
	{
		JsonWriter->WriteObjectStart(Name);
		// Iterate all of the keys and their values
		for (FJsonSerializableKeyValueMapInt::TIterator KeyValueIt(Map); KeyValueIt; ++KeyValueIt)
		{
			Serialize(*(KeyValueIt.Key()), KeyValueIt.Value());
		}
		JsonWriter->WriteObjectEnd();
	}
};

/**
 * Implements the abstract serializer interface hiding the underlying reader object
 */
class FOnlineJsonSerializerReader :
	public FOnlineJsonSerializerBase
{
	/** The object that holds the parsed JSON data */
	TSharedPtr<FJsonObject> JsonObject;

public:
	/**
	 * Inits the base JSON object that is being read from
	 *
	 * @param InJsonObject the JSON object to serialize from
	 */
	FOnlineJsonSerializerReader(TSharedPtr<FJsonObject> InJsonObject) :
		JsonObject(InJsonObject)
	{
	}

    /** Is the JSON being read from */
	virtual bool IsLoading() const OVERRIDE { return true; }
    /** Is the JSON being written to */
	virtual bool IsSaving() const OVERRIDE { return false; }

	/** Ignored */
	virtual void StartObject() OVERRIDE
	{
		// Empty on purpose
	}
	/** Ignored */
	virtual void StartObject(const FString& Name) OVERRIDE
	{
		// Empty on purpose
	}
	/** Ignored */
	virtual void EndObject() OVERRIDE
	{
		// Empty on purpose
	}
	/**
	 * If the underlying json object has the field, it is read into the value
	 *
	 * @param Name the name of the field to read
	 * @param Value the out value to read the data into
	 */
	virtual void Serialize(const TCHAR* Name, int32& Value) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			Value = FMath::Trunc(JsonObject->GetNumberField(Name));
		}
	}
	/**
	 * If the underlying json object has the field, it is read into the value
	 *
	 * @param Name the name of the field to read
	 * @param Value the out value to read the data into
	 */
	virtual void Serialize(const TCHAR* Name, uint32& Value) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			Value = FMath::Trunc(JsonObject->GetNumberField(Name));
		}
	}
	/**
	 * If the underlying json object has the field, it is read into the value
	 *
	 * @param Name the name of the field to read
	 * @param Value the out value to read the data into
	 */
	virtual void Serialize(const TCHAR* Name, bool& Value) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			Value = JsonObject->GetBoolField(Name);
		}
	}
	/**
	 * If the underlying json object has the field, it is read into the value
	 *
	 * @param Name the name of the field to read
	 * @param Value the out value to read the data into
	 */
	virtual void Serialize(const TCHAR* Name, FString& Value) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			Value = JsonObject->GetStringField(Name);
		}
	}
	/**
	 * If the underlying json object has the field, it is read into the value
	 *
	 * @param Name the name of the field to read
	 * @param Value the out value to read the data into
	 */
	virtual void Serialize(const TCHAR* Name, float& Value) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			Value = JsonObject->GetNumberField(Name);
		}
	}
	/**
	 * Serializes an array of values
	 *
	 * @param Name the name of the property to serialize
	 * @param Array the array to serialize
	 */
	virtual void SerializeArray(FJsonSerializableArray& Array) OVERRIDE
	{
		// @todo - higher level serialization is expecting a Json Object
		check(0 && TEXT("Not implemented"));
	}
	/**
	 * Serializes an array of values with an identifier
	 *
	 * @param Name the name of the property to serialize
	 * @param Array the array to serialize
	 */
	virtual void SerializeArray(const TCHAR* Name, FJsonSerializableArray& Array) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			TArray< TSharedPtr<FJsonValue> > JsonArray = JsonObject->GetArrayField(Name);
			// Iterate all of the keys and their values
			for (auto ArrayIt = JsonArray.CreateConstIterator(); ArrayIt; ++ArrayIt)
			{
				Array.Add((*ArrayIt)->AsString());
			}
		}
	}
	/**
	 * Serializes the keys & values for map
	 *
	 * @param Name the name of the property to serialize
	 * @param Map the map to serialize
	 */
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMap& Map) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			TSharedPtr<FJsonObject> JsonMap = JsonObject->GetObjectField(Name);
			// Iterate all of the keys and their values
			for (auto KeyValueIt = JsonMap->Values.CreateConstIterator(); KeyValueIt; ++KeyValueIt)
			{
				FString Value = JsonMap->GetStringField(KeyValueIt.Key());
				Map.Add(KeyValueIt.Key(), Value);
			}
		}
	}

	/**
	 * Serializes the keys & values for map
	 *
	 * @param Name the name of the property to serialize
	 * @param Map the map to serialize
	 */
	virtual void SerializeMap(const TCHAR* Name, FJsonSerializableKeyValueMapInt& Map) OVERRIDE
	{
		if (JsonObject->HasField(Name))
		{
			TSharedPtr<FJsonObject> JsonMap = JsonObject->GetObjectField(Name);
			// Iterate all of the keys and their values
			for (auto KeyValueIt = JsonMap->Values.CreateConstIterator(); KeyValueIt; ++KeyValueIt)
			{
				int32 Value = JsonMap->GetNumberField(KeyValueIt.Key());
				Map.Add(KeyValueIt.Key(), Value);
			}
		}
	}
};

/**
 * Base class for a JSON serializable object
 */
struct FOnlineJsonSerializable
{
	/**
	 * Used to allow serialization of a const ref
	 *
	 * @return the corresponding json string
	 */
	inline const FString ToJson() const
	{
		// Strip away const, because we use a single method that can read/write which requires non-const semantics
		// Otherwise, we'd have to have 2 separate macros for declaring const to json and non-const from json
		return ((FOnlineJsonSerializable*)this)->ToJson();
	}
	/**
	 * Serializes this object to its JSON string form
	 *
	 * @return the corresponding json string
	 */
	virtual const FString ToJson()
	{
		FString JsonStr;
		TSharedRef<TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(&JsonStr);
		FOnlineJsonSerializerWriter Serializer(JsonWriter);
		Serialize(Serializer);
		JsonWriter->Close();
		return JsonStr;
	}
	/**
	 * Serializes the contents of a JSON string into this object
	 *
	 * @param Json the JSON data to serialize from
	 */
	virtual bool FromJson(const FString& Json)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(Json);
		if (FJsonSerializer::Deserialize(JsonReader,JsonObject) &&
			JsonObject.IsValid())
		{
			FOnlineJsonSerializerReader Serializer(JsonObject);
			Serialize(Serializer);
			return true;
		}
		return false;
	}
	virtual bool FromJson(TSharedPtr<FJsonObject> JsonObject)
	{
		if (JsonObject.IsValid())
		{
			FOnlineJsonSerializerReader Serializer(JsonObject);
			Serialize(Serializer);
			return true;
		}
		return false;
	}

	/**
	 * Abstract method that needs to be supplied using the macros
	 *
	 * @param Serializer the object that will perform serialization in/out of JSON
	 */
	virtual void Serialize(FOnlineJsonSerializerBase& Serializer) = 0;
};

