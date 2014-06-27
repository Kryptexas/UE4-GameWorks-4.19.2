// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "Json.h"
#include "JsonDocumentObjectModel.h"

//static 
const TArray< TSharedPtr<FJsonValue> > FJsonValue::EMPTY_ARRAY;
const TSharedPtr<FJsonObject> FJsonValue::EMPTY_OBJECT(new FJsonObject());

double FJsonValue::AsNumber() const
{
	double Number = 0.0;
	if(!TryGetNumber(Number))
	{
		ErrorMessage(TEXT("Number"));
	}
	return Number;
}

FString FJsonValue::AsString() const 
{
	FString String;
	if(!TryGetString(String))
	{
		ErrorMessage(TEXT("String"));
	}
	return String;
}

bool FJsonValue::AsBool() const 
{
	bool Bool = false;
	if(!TryGetBool(Bool))
	{
		ErrorMessage(TEXT("Boolean")); 
	}
	return Bool;
}

const TArray< TSharedPtr<FJsonValue> >& FJsonValue::AsArray() const 
{
	const TArray< TSharedPtr<FJsonValue> > *Array = &EMPTY_ARRAY;
	if(!TryGetArray(Array))
	{
		ErrorMessage(TEXT("Array")); 
	}
	return *Array;
}

const TSharedPtr<FJsonObject>& FJsonValue::AsObject() const 
{
	const TSharedPtr<FJsonObject> *Object = &EMPTY_OBJECT;
	if(!TryGetObject(Object))
	{
		ErrorMessage(TEXT("Object"));
	}
	return *Object;
}

bool FJsonValue::TryGetNumber(int32& OutNumber) const
{
	double Double;
	if(TryGetNumber(Double) && Double >= INT_MIN && Double <= INT_MAX)
	{
		OutNumber = (int32)(Double + 0.5);
		return true;
	}
	return false;
}

bool FJsonValue::TryGetNumber(uint32& OutNumber) const
{
	double Double;
	if(TryGetNumber(Double) && Double >= 0.0 && Double <= UINT_MAX)
	{
		OutNumber = (uint32)(Double + 0.5);
		return true;
	}
	return false;
}

//static 
bool FJsonValue::CompareEqual(const FJsonValue& Lhs, const FJsonValue& Rhs)
{
	if (Lhs.Type != Rhs.Type)
	{
		return false;
	}

	switch (Lhs.Type)
	{
	case EJson::None:
	case EJson::Null:
		return true;
	case EJson::String:
		return Lhs.AsString() == Rhs.AsString();
	case EJson::Number:
		return Lhs.AsNumber() == Rhs.AsNumber();
	case EJson::Boolean:
		return Lhs.AsBool() == Rhs.AsBool();
	case EJson::Array:
		{
			const TArray< TSharedPtr<FJsonValue> >& LhsArray = Lhs.AsArray();
			const TArray< TSharedPtr<FJsonValue> >& RhsArray = Rhs.AsArray();
			if (LhsArray.Num() != RhsArray.Num())
			{
				return false;
			}

			// compare each element
			for (int32 i = 0; i < LhsArray.Num(); ++i)
			{
				if (!CompareEqual(*LhsArray[i], *RhsArray[i]))
				{
					return false;
				}
			}
		}
		return true;
	case EJson::Object:
		{
			const TSharedPtr<FJsonObject>& LhsObject = Lhs.AsObject();
			const TSharedPtr<FJsonObject>& RhsObject = Rhs.AsObject();
			if (LhsObject.IsValid() != RhsObject.IsValid())
			{
				return false;
			}
			if (LhsObject.IsValid())
			{
				if (LhsObject->Values.Num() != RhsObject->Values.Num())
				{
					return false;
				}

				// compare each element
				for (const auto& It : LhsObject->Values)
				{
					const FString& Key = It.Key;
					const TSharedPtr<FJsonValue>* RhsValue = RhsObject->Values.Find(Key);
					if (RhsValue == NULL)
					{
						// not found in both objects
						return false;
					}
					const TSharedPtr<FJsonValue>& LhsValue = It.Value;
					if (LhsValue.IsValid() != RhsValue->IsValid())
					{
						return false;
					}
					if (LhsValue.IsValid())
					{
						if (!CompareEqual(*LhsValue.Get(), *RhsValue->Get()))
						{
							return false;
						}
					}
				}
			}
		}
		return true;
	default:
		return false;
	}
}

void FJsonObject::SetField( const FString& FieldName, const TSharedPtr<FJsonValue>& Value )
{
	this->Values.Add( FieldName, Value );
}

void FJsonObject::RemoveField(const FString& FieldName)
{
	this->Values.Remove(FieldName);
}

double FJsonObject::GetNumberField(const FString& FieldName) const
{
	return GetField<EJson::None>(FieldName)->AsNumber();
}

bool FJsonObject::TryGetNumberField(const FString& FieldName, double& OutNumber) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetNumber(OutNumber);
}

bool FJsonObject::TryGetNumberField(const FString& FieldName, int32& OutNumber) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetNumber(OutNumber);
}

bool FJsonObject::TryGetNumberField(const FString& FieldName, uint32& OutNumber) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetNumber(OutNumber);
}

void FJsonObject::SetNumberField( const FString& FieldName, double Number )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueNumber(Number)) );
}

FString FJsonObject::GetStringField(const FString& FieldName) const
{
	return GetField<EJson::None>(FieldName)->AsString();
}

bool FJsonObject::TryGetStringField(const FString& FieldName, FString& OutString) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetString(OutString);
}

bool FJsonObject::TryGetStringArrayField(const FString& FieldName, TArray<FString>& OutArray) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	if(!Field.IsValid())
	{
		return false;
	}

	const TArray< TSharedPtr<FJsonValue> > *Array;
	if(!Field->TryGetArray(Array))
	{
		return false;
	}

	for(int Idx = 0; Idx < Array->Num(); Idx++)
	{
		FString Element;
		if(!(*Array)[Idx]->TryGetString(Element))
		{
			return false;
		}
		OutArray.Add(Element);
	}
	return true;
}

void FJsonObject::SetStringField( const FString& FieldName, const FString& StringValue )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueString(StringValue)) );
}

bool FJsonObject::GetBoolField(const FString& FieldName) const
{
	return GetField<EJson::None>(FieldName)->AsBool();
}

bool FJsonObject::TryGetBoolField(const FString& FieldName, bool& OutBool) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetBool(OutBool);
}

void FJsonObject::SetBoolField( const FString& FieldName, bool InValue )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueBoolean(InValue) ) );
}

const TArray< TSharedPtr<FJsonValue> >& FJsonObject::GetArrayField(const FString& FieldName) const
{
	return GetField<EJson::Array>(FieldName)->AsArray();
}

bool FJsonObject::TryGetArrayField(const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >*& OutArray) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetArray(OutArray);
}

void FJsonObject::SetArrayField( const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >& Array )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueArray(Array) ) );
}

const TSharedPtr<FJsonObject>& FJsonObject::GetObjectField(const FString& FieldName) const
{
	return GetField<EJson::Object>(FieldName)->AsObject();
}

bool FJsonObject::TryGetObjectField(const FString& FieldName, const TSharedPtr<FJsonObject>*& OutObject) const
{
	TSharedPtr<FJsonValue> Field = TryGetField(FieldName);
	return Field.IsValid() && Field->TryGetObject(OutObject);
}

void FJsonObject::SetObjectField( const FString& FieldName, const TSharedPtr<FJsonObject>& JsonObject )
{
	if (JsonObject.IsValid())
	{
		this->Values.Add( FieldName, MakeShareable(new FJsonValueObject(JsonObject.ToSharedRef())) );
	}
	else
	{
		this->Values.Add( FieldName, MakeShareable( new FJsonValueNull() ) );
	}
}