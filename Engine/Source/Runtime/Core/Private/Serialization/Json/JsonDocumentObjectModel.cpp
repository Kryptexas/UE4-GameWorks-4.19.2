// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "Json.h"
#include "JsonDocumentObjectModel.h"

//static 
const TArray< TSharedPtr<FJsonValue> > FJsonValue::EMPTY_ARRAY;
const TSharedPtr<FJsonObject> FJsonValue::EMPTY_OBJECT(new FJsonObject());

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

void FJsonObject::SetNumberField( const FString& FieldName, double Number )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueNumber(Number)) );
}

FString FJsonObject::GetStringField(const FString& FieldName) const
{
	return GetField<EJson::None>(FieldName)->AsString();
}

void FJsonObject::SetStringField( const FString& FieldName, const FString& StringValue )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueString(StringValue)) );
}

bool FJsonObject::GetBoolField(const FString& FieldName) const
{
	return GetField<EJson::None>(FieldName)->AsBool();
}

void FJsonObject::SetBoolField( const FString& FieldName, bool InValue )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueBoolean(InValue) ) );
}

const TArray< TSharedPtr<FJsonValue> >& FJsonObject::GetArrayField(const FString& FieldName) const
{
	return GetField<EJson::Array>(FieldName)->AsArray();
}

void FJsonObject::SetArrayField( const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >& Array )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueArray(Array) ) );
}

const TSharedPtr<FJsonObject>& FJsonObject::GetObjectField(const FString& FieldName) const
{
	return GetField<EJson::Object>(FieldName)->AsObject();
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