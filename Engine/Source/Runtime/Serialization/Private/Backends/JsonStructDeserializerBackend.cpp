// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SerializationPrivatePCH.h"
#include "Json.h"
#include "JsonStructDeserializerBackend.h"


/* Internal helpers
 *****************************************************************************/

namespace JsonStructDeserializerBackend
{
	// Clears the value of the given property.
	void ClearPropertyValue( UProperty* Property, UProperty* Outer, void* Data, int32 ArrayIndex )
	{
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Outer);

		if (ArrayProperty != nullptr)
		{
			check(ArrayProperty->Inner == Property)

			FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->template ContainerPtrToValuePtr<void>(Data));
			ArrayIndex = ArrayHelper.AddValue();
		}

		Property->ClearValue_InContainer(Data, ArrayIndex);
	}

	// Sets the value of the given property.
	template<typename UPropertyType, typename PropertyType>
	void SetPropertyValue( UProperty* Property, UProperty* Outer, void* Data, int32 ArrayIndex, const PropertyType& Value )
	{
		PropertyType* ValuePtr = nullptr;
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Outer);

		if (ArrayProperty != nullptr)
		{
			check(ArrayProperty->Inner == Property)

			FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->template ContainerPtrToValuePtr<void>(Data));
			int32 Index = ArrayHelper.AddValue();
		
			ValuePtr = (PropertyType*)ArrayHelper.GetRawPtr(Index);
		}
		else
		{
			UPropertyType* TypedProperty = Cast<UPropertyType>(Property);
			check(TypedProperty)

			ValuePtr = TypedProperty->template ContainerPtrToValuePtr<PropertyType>(Data, ArrayIndex);
		}

		*ValuePtr = Value;
	}
}


/* IStructDeserializerBackend interface
 *****************************************************************************/

const FString& FJsonStructDeserializerBackend::GetCurrentPropertyName( ) const
{
	return JsonReader->GetIdentifier();
}


FString FJsonStructDeserializerBackend::GetDebugString( ) const
{
	return FString::Printf(TEXT("Line: %u, Ch: %u"), JsonReader->GetLineNumber(), JsonReader->GetCharacterNumber());
}


const FString& FJsonStructDeserializerBackend::GetLastErrorMessage( ) const
{
	return JsonReader->GetErrorMessage();
}


bool FJsonStructDeserializerBackend::GetNextToken( EStructDeserializerBackendTokens& OutToken )
{
	if (!JsonReader->ReadNext(LastNotation))
	{
		return false;
	}

	switch (LastNotation)
	{
	case EJsonNotation::ArrayEnd:
		OutToken = EStructDeserializerBackendTokens::ArrayEnd;
		break;

	case EJsonNotation::ArrayStart:
		OutToken = EStructDeserializerBackendTokens::ArrayStart;
		break;

	case EJsonNotation::Boolean:
	case EJsonNotation::Null:
	case EJsonNotation::Number:
	case EJsonNotation::String:
		{
			OutToken = EStructDeserializerBackendTokens::Property;
		}
		break;

	case EJsonNotation::Error:
		OutToken = EStructDeserializerBackendTokens::Error;
		break;

	case EJsonNotation::ObjectEnd:
		OutToken = EStructDeserializerBackendTokens::StructureEnd;
		break;

	case EJsonNotation::ObjectStart:
		OutToken = EStructDeserializerBackendTokens::StructureStart;
		break;

	default:
		OutToken = EStructDeserializerBackendTokens::None;
	}

	return true;
}


bool FJsonStructDeserializerBackend::ReadProperty( UProperty* Property, UProperty* Outer, void* Data, int32 ArrayIndex )
{
	using namespace JsonStructDeserializerBackend;

	switch (LastNotation)
	{
	// boolean values
	case EJsonNotation::Boolean:
		{
			bool BoolValue = JsonReader->GetValueAsBoolean();

			if (Property->GetClass() == UBoolProperty::StaticClass())
			{
				SetPropertyValue<UBoolProperty, bool>(Property, Outer, Data, ArrayIndex, BoolValue);
			}
			else
			{
				UE_LOG(LogSerialization, Verbose, TEXT("Boolean field %s with value '%s' is not supported in UProperty type %s (%s)"), *Property->GetFName().ToString(), BoolValue ? *(GTrue.ToString()) : *(GFalse.ToString()), *Property->GetClass()->GetName(), *GetDebugString());

				return false;
			}
		}
		break;

	// numeric values
	case EJsonNotation::Number:
		{
			float NumericValue = JsonReader->GetValueAsNumber();

			if (Property->GetClass() == UByteProperty::StaticClass())
			{
				SetPropertyValue<UByteProperty, int8>(Property, Outer, Data, ArrayIndex, (int8)NumericValue);
			}
			else if (Property->GetClass() == UDoubleProperty::StaticClass())
			{
				SetPropertyValue<UDoubleProperty, double>(Property, Outer, Data, ArrayIndex, NumericValue);
			}
			else if (Property->GetClass() == UFloatProperty::StaticClass())
			{
				SetPropertyValue<UFloatProperty, float>(Property, Outer, Data, ArrayIndex, NumericValue);
			}
			else if (Property->GetClass() == UIntProperty::StaticClass())
			{
				SetPropertyValue<UIntProperty, int32>(Property, Outer, Data, ArrayIndex, (int32)NumericValue);
			}
			else if (Property->GetClass() == UUInt32Property::StaticClass())
			{
				SetPropertyValue<UUInt32Property, uint32>(Property, Outer, Data, ArrayIndex, (uint32)NumericValue);
			}
			else if (Property->GetClass() == UInt16Property::StaticClass())
			{
				SetPropertyValue<UInt16Property, int16>(Property, Outer, Data, ArrayIndex, (int16)NumericValue);
			}
			else if (Property->GetClass() == UUInt16Property::StaticClass())
			{
				SetPropertyValue<UUInt16Property, uint16>(Property, Outer, Data, ArrayIndex, (uint16)NumericValue);
			}
			else if (Property->GetClass() == UInt64Property::StaticClass())
			{
				SetPropertyValue<UInt64Property, int64>(Property, Outer, Data, ArrayIndex, (int64)NumericValue);
			}
			else if (Property->GetClass() == UUInt64Property::StaticClass())
			{
				SetPropertyValue<UUInt64Property, uint64>(Property, Outer, Data, ArrayIndex, (uint64)NumericValue);
			}
			else if (Property->GetClass() == UInt8Property::StaticClass())
			{
				SetPropertyValue<UInt8Property, int8>(Property, Outer, Data, ArrayIndex, (int8)NumericValue);
			}
			else
			{
				UE_LOG(LogSerialization, Verbose, TEXT("Numeric field %s with value '%f' is not supported in UProperty type %s (%s)"), *Property->GetFName().ToString(), NumericValue, *Property->GetClass()->GetName(), *GetDebugString());

				return false;
			}
		}
		break;

	// null values
	case EJsonNotation::Null:
		ClearPropertyValue(Property, Outer, Data, ArrayIndex);
		break;

	// strings, names & enumerations
	case EJsonNotation::String:
		{
			const FString& StringValue = JsonReader->GetValueAsString();

			if (Property->GetClass() == UStrProperty::StaticClass())
			{
				SetPropertyValue<UStrProperty, FString>(Property, Outer, Data, ArrayIndex, StringValue);
			}
			else if (Property->GetClass() == UNameProperty::StaticClass())
			{
				SetPropertyValue<UNameProperty, FName>(Property, Outer, Data, ArrayIndex, *StringValue);
			}
			else if (Property->GetClass() == UByteProperty::StaticClass())
			{
				UByteProperty* ByteProperty = Cast<UByteProperty>(Property);
				int32 Index = ByteProperty->Enum->FindEnumIndex(*StringValue);

				if (Index != INDEX_NONE)
				{
					SetPropertyValue<UByteProperty, uint8>(Property, Outer, Data, ArrayIndex, (uint8)Index);
				}
			}
			else if (Property->GetClass() == UClassProperty::StaticClass())
			{
				SetPropertyValue<UClassProperty, UClass*>(Property, Outer, Data, ArrayIndex, LoadObject<UClass>(NULL, *StringValue, NULL, LOAD_NoWarn));
			}
			else
			{
				UE_LOG(LogSerialization, Verbose, TEXT("String field %s with value '%s' is not supported in UProperty type %s (%s)"), *Property->GetFName().ToString(), *StringValue, *Property->GetClass()->GetName(), *GetDebugString());

				return false;
			}
		}
		break;
	}

	return true;
}


void FJsonStructDeserializerBackend::SkipArray( ) 
{
	JsonReader->SkipArray();
}


void FJsonStructDeserializerBackend::SkipStructure( )
{
	JsonReader->SkipObject();
}
