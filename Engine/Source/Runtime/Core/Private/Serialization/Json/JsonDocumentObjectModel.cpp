// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "Json.h"
#include "JsonDocumentObjectModel.h"

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
	return GetField<EJson::Number>(FieldName)->AsNumber();
}

void FJsonObject::SetNumberField( const FString& FieldName, double Number )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueNumber(Number)) );
}

FString FJsonObject::GetStringField(const FString& FieldName) const
{
	return GetField<EJson::String>(FieldName)->AsString();
}

void FJsonObject::SetStringField( const FString& FieldName, const FString& StringValue )
{
	this->Values.Add( FieldName, MakeShareable(new FJsonValueString(StringValue)) );
}

bool FJsonObject::GetBoolField(const FString& FieldName) const
{
	return GetField<EJson::Boolean>(FieldName)->AsBool();
}

void FJsonObject::SetBoolField( const FString& FieldName, bool InValue )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueBoolean(InValue) ) );
}

TArray< TSharedPtr<FJsonValue> > FJsonObject::GetArrayField(const FString& FieldName) const
{
	return GetField<EJson::Array>(FieldName)->AsArray();
}

void FJsonObject::SetArrayField( const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >& Array )
{
	this->Values.Add( FieldName, MakeShareable( new FJsonValueArray(Array) ) );
}

TSharedPtr<FJsonObject> FJsonObject::GetObjectField(const FString& FieldName) const
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