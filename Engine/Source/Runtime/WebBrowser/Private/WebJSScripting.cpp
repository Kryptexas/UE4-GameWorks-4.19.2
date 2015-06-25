
 // Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebJSScripting.h"
#include "WebJSFunction.h"
#include "WebBrowserWindow.h"
#include "WebJSStructSerializerBackend.h"
#include "WebJSStructDeserializerBackend.h"
#include "StructSerializer.h"
#include "StructDeserializer.h"

#if WITH_CEF3

// Internal utility function(s)
namespace
{

	template<typename DestContainerType, typename SrcContainerType, typename DestKeyType, typename SrcKeyType>
	bool CopyContainerValue(DestContainerType DestContainer, SrcContainerType SrcContainer, DestKeyType DestKey, SrcKeyType SrcKey )
	{
		switch (SrcContainer->GetType(SrcKey))
		{
			case VTYPE_NULL:
				return DestContainer->SetNull(DestKey);
			case VTYPE_BOOL:
				return DestContainer->SetBool(DestKey, SrcContainer->GetBool(SrcKey));
			case VTYPE_INT:
				return DestContainer->SetInt(DestKey, SrcContainer->GetInt(SrcKey));
			case VTYPE_DOUBLE:
				return DestContainer->SetDouble(DestKey, SrcContainer->GetDouble(SrcKey));
			case VTYPE_STRING:
				return DestContainer->SetString(DestKey, SrcContainer->GetString(SrcKey));
			case VTYPE_BINARY:
				return DestContainer->SetBinary(DestKey, SrcContainer->GetBinary(SrcKey));
			case VTYPE_DICTIONARY:
				return DestContainer->SetDictionary(DestKey, SrcContainer->GetDictionary(SrcKey));
			case VTYPE_LIST:
				return DestContainer->SetList(DestKey, SrcContainer->GetList(SrcKey));
			case VTYPE_INVALID:
			default:
				return false;
		}

	}
}

FWebJSParam FWebJSScripting::ConvertResult(UProperty* Property, uint8* Data)
{
	// booleans
	if (Property->IsA<UBoolProperty>())
	{
		return FWebJSParam(Cast<UBoolProperty>(Property)->GetPropertyValue_InContainer(Data));
	}

	// unsigned bytes & enumerations
	else if (Property->IsA<UByteProperty>())
	{
		UByteProperty* ByteProperty = Cast<UByteProperty>(Property);

		if (ByteProperty->IsEnum())
		{
			return FWebJSParam(ByteProperty->Enum->GetEnumName(ByteProperty->GetPropertyValue_InContainer(Data)));
		}
		else
		{
			return FWebJSParam((int32)ByteProperty->GetPropertyValue_InContainer(Data));
		}
	}

	// floating point numbers
	else if (Property->IsA<UDoubleProperty>())
	{
		return FWebJSParam(Cast<UDoubleProperty>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UFloatProperty>())
	{
		return FWebJSParam(Cast<UFloatProperty>(Property)->GetPropertyValue_InContainer(Data));
	}

	// signed integers
	else if (Property->IsA<UIntProperty>())
	{
		return FWebJSParam(Cast<UIntProperty>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UInt8Property>())
	{
		return FWebJSParam((int32)Cast<UInt8Property>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UInt16Property>())
	{
		return FWebJSParam((int32)Cast<UInt16Property>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UInt64Property>())
	{
		return FWebJSParam((double)Cast<UInt64Property>(Property)->GetPropertyValue_InContainer(Data));
	}

	// unsigned integers
	else if (Property->IsA<UUInt16Property>())
	{
		return FWebJSParam((int32)Cast<UUInt16Property>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UUInt32Property>())
	{
		return FWebJSParam((double)Cast<UUInt32Property>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UUInt64Property>())
	{
		return FWebJSParam((double)Cast<UUInt64Property>(Property)->GetPropertyValue_InContainer(Data));
	}

	// names & strings
	else if (Property->IsA<UNameProperty>())
	{
		return FWebJSParam(Cast<UNameProperty>(Property)->GetPropertyValue_InContainer(Data).ToString());
	}
	else if (Property->IsA<UStrProperty>())
	{
		return FWebJSParam(Cast<UStrProperty>(Property)->GetPropertyValue_InContainer(Data));
	}

	// classes & objects
	else if (Property->IsA<UClassProperty>())
	{
		return FWebJSParam(Cast<UClassProperty>(Property)->GetPropertyValue_InContainer(Data)->GetPathName());
	}
	else if (Property->IsA<UObjectProperty>())
	{
		return FWebJSParam(Cast<UObjectProperty>(Property)->GetPropertyValue_InContainer(Data));
	}
	else if (Property->IsA<UStructProperty>())
	{
		UStructProperty* StructProperty = Cast<UStructProperty>(Property);
		return FWebJSParam(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(Data));
	}
	else
	{
		GLog->Logf(ELogVerbosity::Warning, TEXT("FWebJSScripting: %s cannot be serialized, because its type (%s) is not supported"), *Property->GetName(), *Property->GetClass()->GetName());
	}
	return FWebJSParam();
}


CefRefPtr<CefDictionaryValue> FWebJSScripting::ConvertStruct(UStruct* TypeInfo, void* StructPtr)
{
	FWebJSStructSerializerBackend Backend (SharedThis(this));
	FStructSerializer::Serialize(StructPtr, *TypeInfo, Backend);

	CefRefPtr<CefDictionaryValue> Result = CefDictionaryValue::Create();
	Result->SetString("$type", "struct");
	Result->SetString("$ue4Type", *TypeInfo->GetName());
	Result->SetDictionary("$value", Backend.GetResult());
	return Result;
}

CefRefPtr<CefDictionaryValue> FWebJSScripting::ConvertObject(UObject* Object)
{
	CefRefPtr<CefDictionaryValue> Result = CefDictionaryValue::Create();
	RetainBinding(Object);

	UClass* Class = Object->GetClass();
	CefRefPtr<CefListValue> MethodNames = CefListValue::Create();
	int32 MethodIndex = 0;
	for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
	{
		UFunction* Function = *FunctionIt;
		MethodNames->SetString(MethodIndex++, *Function->GetName());
	}

	Result->SetString("$type", "uobject");
	Result->SetString("$id", *PtrToGuid(Object).ToString(EGuidFormats::Digits));
	Result->SetList("$methods", MethodNames);
	return Result;
}


bool FWebJSScripting::OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser, CefProcessId SourceProcess, CefRefPtr<CefProcessMessage> Message)
{
	bool Result = false;
	FString MessageName = Message->GetName().ToWString().c_str();
	if (MessageName == TEXT("UE::ExecuteUObjectMethod"))
	{
		Result = HandleExecuteUObjectMethodMessage(Message->GetArgumentList());
	}
	else if (MessageName == TEXT("UE::ReleaseUObject"))
	{
		Result = HandleReleaseUObjectMessage(Message->GetArgumentList());
	}
	return Result;
}

void FWebJSScripting::SendProcessMessage(CefRefPtr<CefProcessMessage> Message)
{
	if (IsValid() )
	{
		InternalCefBrowser->SendProcessMessage(PID_RENDERER, Message);
	}
}

CefRefPtr<CefDictionaryValue> FWebJSScripting::GetPermanentBindings()
{
	CefRefPtr<CefDictionaryValue> Result = CefDictionaryValue::Create();
	for(auto& Entry : PermanentUObjectsByName)
	{
		Result->SetDictionary(*Entry.Key, ConvertObject(Entry.Value));
	}
	return Result;
}


void FWebJSScripting::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	CefRefPtr<CefDictionaryValue> Converted = ConvertObject(Object);
	if (bIsPermanent)
	{
		// Each object can only have one permanent binding
		if (BoundObjects[Object].bIsPermanent)
		{
			return;
		}
		// Existing permanent objects must be removed first
		if (PermanentUObjectsByName.Contains(Name))
		{
			return;
		}
		BoundObjects[Object]={true, -1};
		PermanentUObjectsByName.Add(Name, Object);
	}

	CefRefPtr<CefProcessMessage> SetValueMessage = CefProcessMessage::Create(TEXT("UE::SetValue"));
	CefRefPtr<CefListValue>MessageArguments = SetValueMessage->GetArgumentList();
	CefRefPtr<CefDictionaryValue> Value = CefDictionaryValue::Create();
	Value->SetString("name", *Name);
	Value->SetDictionary("value", Converted);
	Value->SetBool("permanent", bIsPermanent);

	MessageArguments->SetDictionary(0, Value);
	SendProcessMessage(SetValueMessage);
}

void FWebJSScripting::UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	if (bIsPermanent)
	{
		// If overriding an existing permanent object, make it non-permanent
		if (PermanentUObjectsByName.Contains(Name) && (Object == nullptr || PermanentUObjectsByName[Name] == Object))
		{
			Object = PermanentUObjectsByName.FindAndRemoveChecked(Name);
			BoundObjects.Remove(Object);
			return;
		}
		else
		{
			return;
		}
	}

	CefRefPtr<CefProcessMessage> DeleteValueMessage = CefProcessMessage::Create(TEXT("UE::DeleteValue"));
	CefRefPtr<CefListValue>MessageArguments = DeleteValueMessage->GetArgumentList();
	CefRefPtr<CefDictionaryValue> Info = CefDictionaryValue::Create();
	Info->SetString("name", *Name);
	Info->SetString("id", *PtrToGuid(Object).ToString(EGuidFormats::Digits));
	Info->SetBool("permanent", bIsPermanent);

	MessageArguments->SetDictionary(0, Info);
	SendProcessMessage(DeleteValueMessage);
}

bool FWebJSScripting::HandleReleaseUObjectMessage(CefRefPtr<CefListValue> MessageArguments)
{
	FGuid ObjectKey;
	// Message arguments are Name, Value, bGlobal
	if (MessageArguments->GetSize() != 1 || MessageArguments->GetType(0) != VTYPE_STRING)
	{
		// Wrong message argument types or count
		return false;
	}

	if (!FGuid::Parse(FString(MessageArguments->GetString(0).ToWString().c_str()), ObjectKey))
	{
		// Invalid GUID
		return false;
	}

	UObject* Object = GuidToPtr(ObjectKey);
	if ( Object == nullptr )
	{
		// Invalid object
		return false;
	}
	ReleaseBinding(Object);
	return true;
}

bool FWebJSScripting::HandleExecuteUObjectMethodMessage(CefRefPtr<CefListValue> MessageArguments)
{
	FGuid ObjectKey;
	// Message arguments are Name, Value, bGlobal
	if (MessageArguments->GetSize() != 4
		|| MessageArguments->GetType(0) != VTYPE_STRING
		|| MessageArguments->GetType(1) != VTYPE_STRING
		|| MessageArguments->GetType(2) != VTYPE_STRING
		|| MessageArguments->GetType(3) != VTYPE_LIST
		)
	{
		// Wrong message argument types or count
		return false;
	}

	if (!FGuid::Parse(FString(MessageArguments->GetString(0).ToWString().c_str()), ObjectKey))
	{
		// Invalid GUID
		return false;
	}

	// Get the promise callback and use that to report any results from executing this function.
	FGuid ResultCallbackId;
	if (!FGuid::Parse(FString(MessageArguments->GetString(2).ToWString().c_str()), ResultCallbackId))
	{
		// Invalid GUID
		return false;
	}

	UObject* Object = GuidToPtr(ObjectKey);
	if (Object == nullptr)
	{
		// Unknown uobject id
		InvokeJSErrorResult(ResultCallbackId, TEXT("Unknown UObject ID"));
		return true;
	}

	FName MethodName = MessageArguments->GetString(1).ToWString().c_str();
	UFunction* Function = Object->FindFunction(MethodName);
	if (!Function)
	{
		InvokeJSErrorResult(ResultCallbackId, TEXT("Unknown UObject Function"));
		return true;
	}
	// Coerce arguments to function arguments.
	uint16 ParamsSize = Function->ParmsSize;
	TArray<uint8> Params;
	UProperty* ReturnParam = nullptr;

	// Convert cef argument list to a dictionary, so we can use FStructDeserializer to convert it for us
	CefRefPtr<CefDictionaryValue> NamedArgs = CefDictionaryValue::Create();
	{
		int32 CurrentArg = 0;
		CefRefPtr<CefListValue> CefArgs = MessageArguments->GetList(3);
		for ( TFieldIterator<UProperty> It(Function); It; ++It )
		{
			UProperty* Param = *It;
			if (Param->PropertyFlags & CPF_Parm)
			{
				if (Param->PropertyFlags & CPF_ReturnParm)
				{
					ReturnParam = Param;
				}
				else
				{
					CopyContainerValue(NamedArgs, CefArgs, CefString(*Param->GetName()), CurrentArg);
					CurrentArg++;
				}
			}
		}

		if (ParamsSize > 0)
		{
			Params.AddZeroed(ParamsSize);
			// UFunction is a subclass of UStruct, so we can treat the arguments as a struct for deserialization
			FWebJSStructDeserializerBackend Backend = FWebJSStructDeserializerBackend(SharedThis(this), NamedArgs);
			FStructDeserializer::Deserialize(Params.GetData(), *Cast<UStruct>(Function), Backend);
		}
	}

	Object->ProcessEvent(Function, Params.GetData());
	if ( ReturnParam )
	{
		FWebJSParam Results[1] = {ConvertResult(ReturnParam, Params.GetData())};
		InvokeJSFunction(ResultCallbackId, 1, Results, false);
	}
	else
	{
		InvokeJSFunction(ResultCallbackId, 0, nullptr, false);
	}
	return true;
}

void FWebJSScripting::BindCefBrowser(CefRefPtr<CefBrowser> Browser)
{
	check(Browser.get() == nullptr || InternalCefBrowser.get() == nullptr || InternalCefBrowser->IsSame(Browser));
	InternalCefBrowser = Browser;
}

void FWebJSScripting::UnbindCefBrowser()
{
	InternalCefBrowser = nullptr;
}


void FWebJSScripting::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Ensure bound UObjects are not garbage collected as long as this object is valid.
	for (auto& Binding : BoundObjects)
	{
		Collector.AddReferencedObject(Binding.Key);
	}
}

void FWebJSScripting::InvokeJSErrorResult(FGuid FunctionId, const FString& Error)
{
	FWebJSParam Arguments[1] = {FWebJSParam(Error)};
	InvokeJSFunction(FunctionId, 1, Arguments, true);
}

void FWebJSScripting::InvokeJSFunction(FGuid FunctionId, int32 ArgCount, FWebJSParam Arguments[], bool bIsError)
{
	CefRefPtr<CefListValue> FunctionArguments = CefListValue::Create();
	for ( int32 i=0; i<ArgCount; i++)
	{
		SetConverted(FunctionArguments, i, Arguments[i]);
	}

	CefRefPtr<CefProcessMessage> Message = CefProcessMessage::Create(TEXT("UE::ExecuteJSFunction"));
	CefRefPtr<CefListValue> MessageArguments = Message->GetArgumentList();
	MessageArguments->SetString(0, *FunctionId.ToString(EGuidFormats::Digits));
	MessageArguments->SetList(1, FunctionArguments);
	MessageArguments->SetBool(2, bIsError);
	SendProcessMessage(Message);
}

#endif