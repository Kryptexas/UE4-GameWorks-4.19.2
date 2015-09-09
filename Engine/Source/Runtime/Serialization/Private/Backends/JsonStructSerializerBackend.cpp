// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SerializationPrivatePCH.h"
#include "JsonStructSerializerBackend.h"


/* Internal helpers
 *****************************************************************************/

namespace JsonStructSerializerBackend
{
	// Writes a property value to the serialization output.
	template<typename ValueType>
	void WritePropertyValue( const TSharedRef<TJsonWriter<UCS2CHAR>> JsonWriter, UProperty* Property, const ValueType& Value )
	{
		if ((Property == nullptr) || (Property->ArrayDim > 1) || (Property->GetOuter()->GetClass() == UArrayProperty::StaticClass()))
		{
			JsonWriter->WriteValue(Value);
		}
		else
		{
			JsonWriter->WriteValue(Property->GetName(), Value);
		}
	}

	// Writes a null value to the serialization output.
	void WriteNull( const TSharedRef<TJsonWriter<UCS2CHAR>> JsonWriter, UProperty* Property )
	{
		if ((Property == nullptr) || (Property->ArrayDim > 1) || (Property->GetOuter()->GetClass() == UArrayProperty::StaticClass()))
		{
			JsonWriter->WriteNull();
		}
		else
		{
			JsonWriter->WriteNull(Property->GetName());
		}
	}
}


/* IStructSerializerBackend interface
 *****************************************************************************/

void FJsonStructSerializerBackend::BeginArray(const FStructSerializerState& State)
{
	UObject* Outer = State.ValueProperty->GetOuter();

	if ((Outer != nullptr) && (Outer->GetClass() == UArrayProperty::StaticClass()))
	{
		JsonWriter->WriteArrayStart();
	}
	else
	{
		JsonWriter->WriteArrayStart(State.ValueProperty->GetName());
	}
}


void FJsonStructSerializerBackend::BeginStructure(const FStructSerializerState& State)
{
	if (State.ValueProperty != nullptr)
	{
		UObject* Outer = State.ValueProperty->GetOuter();

		if ((Outer != nullptr) && (Outer->GetClass() == UArrayProperty::StaticClass()))
		{
			JsonWriter->WriteObjectStart();
		}
		else
		{
			JsonWriter->WriteObjectStart(State.ValueProperty->GetName());
		}
	}
	else
	{
		JsonWriter->WriteObjectStart();
	}
}


void FJsonStructSerializerBackend::EndArray(const FStructSerializerState& /*State*/)
{
	JsonWriter->WriteArrayEnd();
}


void FJsonStructSerializerBackend::EndStructure(const FStructSerializerState& /*State*/)
{
	JsonWriter->WriteObjectEnd();
}


void FJsonStructSerializerBackend::WriteComment(const FString& Comment)
{
	// Json does not support comments
}


void FJsonStructSerializerBackend::WriteProperty(const FStructSerializerState& State, int32 ArrayIndex)
{
	using namespace JsonStructSerializerBackend;

	// booleans
	if (State.ValueType == UBoolProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UBoolProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}

	// unsigned bytes & enumerations
	else if (State.ValueType == UByteProperty::StaticClass())
	{
		UByteProperty* ByteProperty = Cast<UByteProperty>(State.ValueProperty);

		if (ByteProperty->IsEnum())
		{
			WritePropertyValue(JsonWriter, State.ValueProperty, ByteProperty->Enum->GetEnumName(ByteProperty->GetPropertyValue_InContainer(State.ValueData, ArrayIndex)));
		}
		else
		{
			WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UByteProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
		}			
	}

	// floating point numbers
	else if (State.ValueType == UDoubleProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UDoubleProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UFloatProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UFloatProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}

	// signed integers
	else if (State.ValueType == UIntProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UIntProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UInt8Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UInt8Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UInt16Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UInt16Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UInt64Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UInt64Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}

	// unsigned integers
	else if (State.ValueType == UUInt16Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UUInt16Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UUInt32Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UUInt32Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UUInt64Property::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, (double)Cast<UUInt64Property>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}

	// names, strings & text
	else if (State.ValueType == UNameProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UNameProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex).ToString());
	}
	else if (State.ValueType == UStrProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UStrProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex));
	}
	else if (State.ValueType == UTextProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UTextProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex).ToString());
	}

	// classes & objects
	else if (State.ValueType == UClassProperty::StaticClass())
	{
		WritePropertyValue(JsonWriter, State.ValueProperty, Cast<UClassProperty>(State.ValueProperty)->GetPropertyValue_InContainer(State.ValueData, ArrayIndex)->GetPathName());
	}
	else if (State.ValueType == UObjectProperty::StaticClass())
	{
		WriteNull(JsonWriter, State.ValueProperty);
	}
	
	// unsupported property type
	else
	{
		UE_LOG(LogSerialization, Verbose, TEXT("FJsonStructSerializerBackend: Property %s cannot be serialized, because its type (%s) is not supported"), *State.ValueProperty->GetFName().ToString(), *State.ValueType->GetFName().ToString());
	}
}
