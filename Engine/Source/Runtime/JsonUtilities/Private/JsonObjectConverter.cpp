// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "JsonUtilitiesPrivatePCH.h"
#include "JsonUtilities.h"

FString FJsonObjectConverter::StandardizeCase(const FString &StringIn)
{
	FString FixedString = StringIn;

	FixedString[0] = FChar::ToLower(FixedString[0]); // json classes/variable start lower case
	FixedString.ReplaceInline(TEXT("ID"), TEXT("Id"), ESearchCase::CaseSensitive); // Id is standard instead of ID, some of our fnames use ID

	return FixedString;
}

static TSharedPtr<FJsonValue> UPropertyToJsonValue(UProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags)
{
	if (UNumericProperty *NumericProperty = Cast<UNumericProperty>(Property))
	{
		// We want to export numbers as numbers
		if (NumericProperty->IsFloatingPoint())
		{
			return MakeShareable(new FJsonValueNumber(NumericProperty->GetFloatingPointPropertyValue(Value)));
		}
		else if (NumericProperty->IsInteger())
		{
			return MakeShareable(new FJsonValueNumber(NumericProperty->GetSignedIntPropertyValue(Value)));
		}

		// fall through to default
	}		
	else if (UBoolProperty *BoolProperty = Cast<UBoolProperty>(Property))
	{
		// Export bools as bools
		return MakeShareable(new FJsonValueBoolean(BoolProperty->GetPropertyValue(Value)));
	}
	else if (UStrProperty *StringProperty = Cast<UStrProperty>(Property))
	{
		return MakeShareable(new FJsonValueString(StringProperty->GetPropertyValue(Value)));
	}
	else if (UArrayProperty *ArrayProperty = Cast<UArrayProperty>(Property))
	{
		TArray< TSharedPtr<FJsonValue> > Out;
		FScriptArrayHelper Helper(ArrayProperty, Value);
		for (int32 i=0, n=Helper.Num(); i<n; ++i)
		{
			TSharedPtr<FJsonValue> Elem = UPropertyToJsonValue(ArrayProperty->Inner, Helper.GetRawPtr(i), CheckFlags, SkipFlags);
			if (Elem.IsValid())
			{
				// add to the array
				Out.Push(Elem);
			}
		}
		return MakeShareable(new FJsonValueArray(Out));
	}
	else if (UStructProperty *StructProperty = Cast<UStructProperty>(Property))
	{
		TSharedRef<FJsonObject> Out = MakeShareable(new FJsonObject());
		if (FJsonObjectConverter::UStructToJsonObject(StructProperty->Struct, Value, Out, CheckFlags, SkipFlags))
		{
			return MakeShareable(new FJsonValueObject(Out));
		}
		// fall through to default
	}
	else
	{
		// Default to export as string for everything else
		FString StringValue;
		Property->ExportTextItem(StringValue, Value, NULL, NULL, 0);
		return MakeShareable(new FJsonValueString(StringValue));
	}

	// invalid
	return TSharedPtr<FJsonValue>();
}

bool FJsonObjectConverter::UStructToJsonObject(const UStruct* StructDefinition, const void* Struct, TSharedRef<FJsonObject> OutJsonObject, int64 CheckFlags, int64 SkipFlags)
{
	for(TFieldIterator<UProperty> It(StructDefinition); It; ++It)
	{
		UProperty* Property = *It;

		// Check to see if we should ignore this property
		if (CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
		{
			continue;
		}
		if (Property->HasAnyPropertyFlags(SkipFlags))
		{
			continue;
		}

		FString VariableName = StandardizeCase(Property->GetName());
		const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);

		// convert the property to a FJsonValue
		TSharedPtr<FJsonValue> JsonValue = UPropertyToJsonValue(Property, Value, CheckFlags, SkipFlags);
		if (!JsonValue.IsValid())
		{
			UClass* PropClass = Property->GetClass();
			UE_LOG(LogJson, Error, TEXT("UStructToJsonObject - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
			return false;
		}

		// set the value on the output object
		OutJsonObject->SetField(VariableName, JsonValue);
	}

	return true;
}

bool FJsonObjectConverter::UStructToJsonObjectString(const UStruct* StructDefinition, const void* Struct, FString& OutJsonString, int64 CheckFlags, int64 SkipFlags)
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable( new FJsonObject() );
	if (UStructToJsonObject(StructDefinition, Struct, JsonObject, CheckFlags, SkipFlags))
	{
		TSharedRef<TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(&OutJsonString);

		if (FJsonSerializer::Serialize( JsonObject, JsonWriter ))
		{
			JsonWriter->Close();
			return true;
		}
		else
		{
			UE_LOG(LogJson, Warning, TEXT("UStructToJsonObjectString - Unable to write out json"));
			JsonWriter->Close();
		}
	}

	return false;
}

static bool JsonValueToUProperty(const TSharedPtr<FJsonValue> JsonValue, UProperty* Property, void* OutValue, int64 CheckFlags, int64 SkipFlags)
{
	if (!JsonValue.IsValid())
	{
		UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Invalid value JSON key"));
		return false;
	}

	if (UNumericProperty *NumericProperty = Cast<UNumericProperty>(Property))
	{
		// We want to export numbers as numbers
		if (NumericProperty->IsFloatingPoint())
		{
			// AsNumber will log an error for completely inappropriate types (then give us a default)
			NumericProperty->SetFloatingPointPropertyValue(OutValue, JsonValue->AsNumber());
		}
		else if (NumericProperty->IsInteger())
		{
			if (JsonValue->Type == EJson::String)
			{
				// parse string -> int64 ourselves so we don't lose any precision going through AsNumber (aka double)
				NumericProperty->SetIntPropertyValue(OutValue, FCString::Atoi64(*JsonValue->AsString()));
			}
			else
			{
				// AsNumber will log an error for completely inappropriate types (then give us a default)
				NumericProperty->SetIntPropertyValue(OutValue, (int64)JsonValue->AsNumber());
			}
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Unable to set numeric property type %s"), *Property->GetClass()->GetName());
			return false;
		}
	}		
	else if (UBoolProperty *BoolProperty = Cast<UBoolProperty>(Property))
	{
		// AsBool will log an error for completely inappropriate types (then give us a default)
		BoolProperty->SetPropertyValue(OutValue, JsonValue->AsBool());
	}
	else if (UStrProperty *StringProperty = Cast<UStrProperty>(Property))
	{
		// AsString will log an error for completely inappropriate types (then give us a default)
		StringProperty->SetPropertyValue(OutValue, JsonValue->AsString());
	}
	else if (UArrayProperty *ArrayProperty = Cast<UArrayProperty>(Property))
	{
		if (JsonValue->Type == EJson::Array)
		{
			TArray< TSharedPtr<FJsonValue> > ArrayValue = JsonValue->AsArray();
			int32 ArrLen = ArrayValue.Num(); 

			// make the output array size match
			FScriptArrayHelper Helper(ArrayProperty, OutValue);
			Helper.Resize(ArrLen);

			// set the property values
			for (int32 i=0;i<ArrLen;++i)
			{
				if (!JsonValueToUProperty(ArrayValue[i], ArrayProperty->Inner, Helper.GetRawPtr(i), CheckFlags, SkipFlags))
				{
					UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Unable to deserialize array element [%d]"), i);
					return false;
				}
			}
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Attempted to import TArray from non-array JSON key"));
			return false;
		}
	}
	else if (UStructProperty *StructProperty = Cast<UStructProperty>(Property))
	{
		if (JsonValue->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();
			if (Obj.IsValid()) // should normally always be true
			{
				if (!FJsonObjectConverter::JsonObjectToUStruct(Obj.ToSharedRef(), StructProperty->Struct, OutValue, CheckFlags, SkipFlags))
				{
					// error message should have already been logged
					return false;
				}
			}
			else
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Attempted to import UStruct from an invalid object JSON key"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Attempted to import UStruct from non-object JSON key"));
			return false;
		}
	}
	else
	{
		// Default to expect a string for everything else
		if (Property->ImportText(*JsonValue->AsString(), OutValue, 0, NULL) == NULL)
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToUProperty - Unable import property type %s from string value"), *Property->GetClass()->GetName());
			return false;
		}
	}

	return true;
}

bool FJsonObjectConverter::JsonObjectToUStruct(const TSharedRef<FJsonObject> JsonObject, const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags, int64 SkipFlags)
{
	// iterate over the struct properties
	for(TFieldIterator<UProperty> PropIt(StructDefinition); PropIt; ++PropIt)
	{
		UProperty* Property = *PropIt;
		FString PropertyName = Property->GetName();

		// Check to see if we should ignore this property
		if (CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
		{
			continue;
		}
		if (Property->HasAnyPropertyFlags(SkipFlags))
		{
			continue;
		}

		// find a json value matching this property name
		TSharedPtr<FJsonValue> JsonValue;
		for (auto It = JsonObject->Values.CreateConstIterator(); It; ++It)
		{
			// use case insensitive search sincd FName may change caseing strangely on us
			if (PropertyName.Equals(It.Key(), ESearchCase::IgnoreCase))
			{
				JsonValue = It.Value();
				break;
			}
		}
		if (!JsonValue.IsValid())
		{
			// we allow values to not be found since this mirrors the typical UObject mantra that all the fields are optional when deserializing
			continue;
		}

		void* Value = Property->ContainerPtrToValuePtr<uint8>(OutStruct);
		if (!JsonValueToUProperty(JsonValue, Property, Value, CheckFlags, SkipFlags))
		{
			UE_LOG(LogJson, Error, TEXT("JsonObjectToUStruct - Unable to parse %s.%s from JSON"), *StructDefinition->GetName(), *PropertyName);
			return false;
		}
	}
	
	return true;
}

bool FJsonObjectConverter::JsonObjectStringToUStruct(const FString& JsonString, const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags, int64 SkipFlags)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(JsonReader,JsonObject) ||
		!JsonObject.IsValid())
	{
		UE_LOG(LogJson, Warning, TEXT("JsonObjectStringToUStruct - Unable to parse json=[%s]"),
			*JsonString);
		return false;
	}
	else
	{
		return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructDefinition, OutStruct, CheckFlags, SkipFlags);
	}
}