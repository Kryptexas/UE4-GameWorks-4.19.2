// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebJSStructSerializerBackend.h"

#if WITH_CEF3

/* Private methods
 *****************************************************************************/

void FWebJSStructSerializerBackend::AddNull(UProperty* Property)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetNull(*Property->GetName());
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetNull(Current.ListValue->GetSize());
		break;
	}
}

void FWebJSStructSerializerBackend::Add(UProperty* Property, bool Value)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetBool(*Property->GetName(), Value);
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetBool(Current.ListValue->GetSize(), Value);
		break;
	}
}

void FWebJSStructSerializerBackend::Add(UProperty* Property, int32 Value)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetInt(*Property->GetName(), Value);
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetInt(Current.ListValue->GetSize(), Value);
		break;
	}
}

void FWebJSStructSerializerBackend::Add(UProperty* Property, double Value)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetDouble(*Property->GetName(), Value);
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetDouble(Current.ListValue->GetSize(), Value);
		break;
	}
}

void FWebJSStructSerializerBackend::Add(UProperty* Property, FString Value)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetString(*Property->GetName(), *Value);
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetString(Current.ListValue->GetSize(), *Value);
		break;
	}
}

void FWebJSStructSerializerBackend::Add(UProperty* Property, UObject* Value)
{
	StackItem& Current = Stack.Top();
	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetDictionary(*Property->GetName(), Scripting->ConvertObject(Value));
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetDictionary(Current.ListValue->GetSize(), Scripting->ConvertObject(Value));
		break;
	}
}

/* IStructSerializerBackend interface
 *****************************************************************************/

void FWebJSStructSerializerBackend::BeginArray( UProperty* Property )
{
	CefRefPtr<CefListValue> ListValue = CefListValue::Create();
	Stack.Push(StackItem(Property->GetName(), ListValue));
}

void FWebJSStructSerializerBackend::BeginStructure( UProperty* Property )
{
	CefRefPtr<CefDictionaryValue> DictionaryValue = CefDictionaryValue::Create();
	Stack.Push(StackItem(Property->GetName(), DictionaryValue));
}

void FWebJSStructSerializerBackend::BeginStructure( UStruct* TypeInfo )
{
	Result = CefDictionaryValue::Create();
	Stack.Push(StackItem(FString(), Result));
}


void FWebJSStructSerializerBackend::EndArray( UProperty* Property )
{
	StackItem Previous = Stack.Pop();
	check(Previous.Kind == StackItem::STYPE_LIST);
	check(Stack.Num() > 0); // The root level object is always a struct
	StackItem& Current = Stack.Top();

	switch (Current.Kind) {
		case StackItem::STYPE_DICTIONARY:
			Current.DictionaryValue->SetList(*Previous.Name, Previous.ListValue);
			break;
		case StackItem::STYPE_LIST:
			Current.ListValue->SetList(Current.ListValue->GetSize(), Previous.ListValue);
		break;
	}
}


void FWebJSStructSerializerBackend::EndStructure()
{
	StackItem Previous = Stack.Pop();
	check(Previous.Kind == StackItem::STYPE_DICTIONARY);

	if (Stack.Num() > 0)
	{
		StackItem& Current = Stack.Top();

		switch (Current.Kind) {
			case StackItem::STYPE_DICTIONARY:
				Current.DictionaryValue->SetDictionary(*Previous.Name, Previous.DictionaryValue);
				break;
			case StackItem::STYPE_LIST:
				Current.ListValue->SetDictionary(Current.ListValue->GetSize(), Previous.DictionaryValue);
			break;
		}
	}
	else
	{
		check(Result == Previous.DictionaryValue);
	}
}


void FWebJSStructSerializerBackend::WriteComment( const FString& Comment )
{
	// Cef values do not support comments
}


void FWebJSStructSerializerBackend::WriteProperty( UProperty* Property, const void* Data, UStruct* TypeInfo, int32 ArrayIndex )
{

	// booleans
	if (TypeInfo == UBoolProperty::StaticClass())
	{
		Add(Property, Cast<UBoolProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}

	// unsigned bytes & enumerations
	else if (TypeInfo == UByteProperty::StaticClass())
	{
		UByteProperty* ByteProperty = Cast<UByteProperty>(Property);

		if (ByteProperty->IsEnum())
		{
			Add(Property, ByteProperty->Enum->GetEnumName(ByteProperty->GetPropertyValue_InContainer(Data, ArrayIndex)));
		}
		else
		{
			Add(Property, (double)Cast<UByteProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
		}			
	}

	// floating point numbers
	else if (TypeInfo == UDoubleProperty::StaticClass())
	{
		Add(Property, Cast<UDoubleProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UFloatProperty::StaticClass())
	{
		Add(Property, Cast<UFloatProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}

	// signed integers
	else if (TypeInfo == UIntProperty::StaticClass())
	{
		Add(Property, (int32)Cast<UIntProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UInt8Property::StaticClass())
	{
		Add(Property, (int32)Cast<UInt8Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UInt16Property::StaticClass())
	{
		Add(Property, (int32)Cast<UInt16Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UInt64Property::StaticClass())
	{
		Add(Property, (double)Cast<UInt64Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}

	// unsigned integers
	else if (TypeInfo == UUInt16Property::StaticClass())
	{
		Add(Property, (int32)Cast<UUInt16Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UUInt32Property::StaticClass())
	{
		Add(Property, (double)Cast<UUInt32Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UUInt64Property::StaticClass())
	{
		Add(Property, (double)Cast<UUInt64Property>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}

	// names & strings
	else if (TypeInfo == UNameProperty::StaticClass())
	{
		Add(Property, Cast<UNameProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex).ToString());
	}
	else if (TypeInfo == UStrProperty::StaticClass())
	{
		Add(Property, Cast<UStrProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	else if (TypeInfo == UTextProperty::StaticClass())
	{
		Add(Property, Cast<UTextProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex).ToString());
	}

	// classes & objects
	else if (TypeInfo == UClassProperty::StaticClass())
	{
		Add(Property, Cast<UClassProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex)->GetPathName());
	}
	else if (TypeInfo == UObjectProperty::StaticClass())
	{
		Add(Property, Cast<UObjectProperty>(Property)->GetPropertyValue_InContainer(Data, ArrayIndex));
	}
	
	else
	{
		GLog->Logf(ELogVerbosity::Warning, TEXT("FWebJSStructSerializerBackend: Property %s cannot be serialized, because its type (%s) is not supported"), *Property->GetName(), *TypeInfo->GetName());
	}
}

#endif
