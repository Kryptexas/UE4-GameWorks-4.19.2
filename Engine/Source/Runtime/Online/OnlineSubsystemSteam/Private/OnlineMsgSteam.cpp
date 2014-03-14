// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemSteamPrivatePCH.h"
#include "OnlineMsgSteam.h"

#if WITH_STEAMGC

#include "AllowWindowsPlatformTypes.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/stubs/common.h"
#include "HideWindowsPlatformTypes.h"

/**
 * Error handling output for protobuffer logging
 */
void ProtobuffErrorHandler(google::protobuf::LogLevel Level, const char* Filename, int32 Line, const std::string& Message)
{
	UE_LOG_ONLINE(Log, TEXT("%s %d %s"), ANSI_TO_TCHAR(Filename), Line, ANSI_TO_TCHAR(Message.c_str()));
}

/**
 * Pool of messages that are suitable for filling before transmission over the wire to a backend server.
 * Reuse of messages is encouraged, so it will allocate a message on demand based on the function signature.
 * Technically this pool is unbounded, but the number of messages allocated is directly related to how
 * many calls are in flight for a single RPC.
 *
 * The pool should last the lifetime of the application
 */
class FProtoMessagePool
{
private:

	/**
	 * Structure containing the beginning of a linked list for a single UFunction.
	 * Contains the message "prototype" which is used to allocate all future free elements
	 */
	struct PoolMapping
	{
		/** Prototype message for RPC */
		MessagePoolElement Prototype;
		/** List of free elements that can be used for new messages */
		MessagePoolElem* FreeList;

		PoolMapping() :
			FreeList(NULL)
		{
		}
	};

	/** Mapping of a function to the head of its message free pool */ 
	TMap<TWeakObjectPtr<UFunction>, PoolMapping> Pool;
	/** Descriptor pool containing the definitions of all messages described so far */
	google::protobuf::DescriptorPool DscrPool; 
	/** Message factory used to create dynamic messages at runtime */
	google::protobuf::DynamicMessageFactory ProtoFactory;

	/**
	 * Get the field type for a given property
	 * @param Property property that determines the type
	 *
	 * @return type of field as defined by message spec (-1 undefined)
	 */
	int32 GetTypeFromProperty(UProperty* Property)
	{
		UClass* PropClass = Property->GetClass();

		if (PropClass == UIntProperty::StaticClass())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_INT32;
		}
		else if (PropClass == UFloatProperty::StaticClass())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_FLOAT;
		}
		else if (PropClass == UBoolProperty::StaticClass() && CastChecked<UBoolProperty>(Property)->IsNativeBool())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_BOOL;
		}
		else if (PropClass == UByteProperty::StaticClass())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_BYTES;
		}
		else if (PropClass == UStrProperty::StaticClass())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_STRING;
		}
		else if (PropClass == UStructProperty::StaticClass())
		{
			return google::protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE;
		}

		return -1;
	}

	/**
	 * Creates a message definition for a group of properties
	 * @param MsgProtoDesc descriptor to fill with the definition
	 * @param FieldIt grouping of properties to define
	 * @param PropertyFlags flags properties must have to be considered for the message
	 *
	 * @return TRUE on successful creation, FALSE otherwise
	 */
	bool CreateProtoDeclaration(google::protobuf::DescriptorProto* MsgProtoDesc, UStruct* Object, uint64 PropertyFlags)
	{	
		bool bSuccess = true;
		int32 FieldIdx = 1;

		TFieldIterator<UProperty> FieldIt(Object);
		for(; FieldIt && (FieldIt->PropertyFlags & PropertyFlags); ++FieldIt)
		{
			UProperty* Property = *FieldIt;
			UClass* PropClass = Property->GetClass();

			if (PropClass != UInterfaceProperty::StaticClass() && PropClass != UObjectProperty::StaticClass())
			{
				bool bIsRepeated = false;
				UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
				if (ArrayProperty != NULL)
				{
					UClass* InnerPropClass = ArrayProperty->Inner->GetClass();
					if (InnerPropClass != UInterfaceProperty::StaticClass() && InnerPropClass != UObjectProperty::StaticClass())
					{
						Property = ArrayProperty->Inner;
						bIsRepeated = true;
					}
					else
					{
						UE_LOG_ONLINE(Error, TEXT("CreateProtoDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
						bSuccess = false;
						break;
					}
				}
				else if(Property->ArrayDim != 1)
				{
					bIsRepeated = true;
				}

				FString SubMsgName;
				UStructProperty* StructProperty = Cast<UStructProperty>(Property);
				if (StructProperty != NULL)
				{
					FString TypeText, ExtendedTypeText;
					TypeText = Property->GetCPPType(&ExtendedTypeText, CPPF_None);

					google::protobuf::DescriptorProto* StructProtDesc = MsgProtoDesc->add_nested_type();
					SubMsgName = FString::Printf(TEXT("%s%sMessage"), *TypeText, *ExtendedTypeText);
					StructProtDesc->set_name(TCHAR_TO_UTF8(*SubMsgName));

					if (!CreateProtoDeclaration(StructProtDesc, StructProperty->Struct, CPF_AllFlags))
					{
						StructProtDesc->Clear();
						bSuccess = false;
						break;
					}
				}

				int32 Type = GetTypeFromProperty(Property);
				if (google::protobuf::FieldDescriptorProto_Type_IsValid(Type))
				{
					google::protobuf::FieldDescriptorProto* MsgFieldProto = MsgProtoDesc->add_field();
					MsgFieldProto->set_name(TCHAR_TO_UTF8(*Property->GetNameCPP()));
					MsgFieldProto->set_number(FieldIdx);
					MsgFieldProto->set_type((google::protobuf::FieldDescriptorProto_Type)Type);
					if (SubMsgName.Len() > 0)
					{
						MsgFieldProto->set_type_name(TCHAR_TO_UTF8(*SubMsgName));
					}
					if (bIsRepeated)
					{
						MsgFieldProto->set_label(google::protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
					}
					else
					{
						MsgFieldProto->set_label(google::protobuf::FieldDescriptorProto_Label_LABEL_OPTIONAL);
					}

					FieldIdx++;
				}
				else
				{
					UE_LOG_ONLINE(Error, TEXT("CreateProtoDeclaration - Unhandled property mapping '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
					bSuccess = false;
					break;
				}
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("CreateProtoDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
				bSuccess = false;
				break;
			}
		}

		return bSuccess;
	}

	/**
	 * Create a message definition for a given function
	 * @param Function function to extract parameters from to create a message buffer
	 */
	const google::protobuf::Message* CreateRPCPrototype(UFunction* Function)
	{
		google::protobuf::FileDescriptorProto FunctionProtoDesc;

		FString FunctionName = Function->GetName();
		FString FunctionFileName = FString::Printf(TEXT("%s.proto"), *FunctionName);
		FunctionProtoDesc.set_package(TCHAR_TO_UTF8(*FunctionName));
		FunctionProtoDesc.set_name(TCHAR_TO_UTF8(*FunctionFileName));

		google::protobuf::DescriptorProto* MsgProtDesc = FunctionProtoDesc.add_message_type();
		MsgProtDesc->set_name(TCHAR_TO_UTF8(*FunctionName));

		if (CreateProtoDeclaration(MsgProtDesc, Function, CPF_Parm))
		{
			google::protobuf::string proto = FunctionProtoDesc.DebugString();
			const google::protobuf::FileDescriptor* FD = DscrPool.BuildFile(FunctionProtoDesc); 
			const google::protobuf::Descriptor* MsgDescriptor = FD->message_type(0);
			return ProtoFactory.GetPrototype(MsgDescriptor);
		}
		else
		{
			FunctionProtoDesc.Clear();
		}

		return NULL;
	}

public:

	FProtoMessagePool()
	{
		// Verify that the version of the library that we linked against is
		// compatible with the version of the headers we compiled against.
		GOOGLE_PROTOBUF_VERIFY_VERSION;

		// Turn on protobuffer logging
		SetLogHandler(ProtobuffErrorHandler);
	}

	~FProtoMessagePool()
	{
		for (TMap<TWeakObjectPtr<UFunction>, PoolMapping>::TIterator It(Pool); It; ++It)
		{
			PoolMapping& PM = It.Value();
			// Delete all the non-prototype versions of the message in the pool
			MessagePoolElem* Elem = PM.FreeList;
			while (Elem)
			{
				MessagePoolElem* NextElem = Elem->Next();
				delete (*Elem)->Msg;
				delete Elem;
				Elem = NextElem;
			}
		}

		Pool.Empty();

		// Optional:  Delete all global objects allocated by libprotobuf.
		google::protobuf::ShutdownProtobufLibrary();
	}

	/**
	 * Create a pool to hold a given function's message buffers
	 * Creates the message prototype and holds it in the never released head of the list
	 * 
	 * @return head of a new linked list for the given function
	 */
	PoolMapping* CreatePool(class UFunction* OwnerFunc)
	{
		TWeakObjectPtr<UFunction> FuncPtr(OwnerFunc);
		PoolMapping* FuncPool = Pool.Find(FuncPtr);
		check(!FuncPool);

		PoolMapping NewPool;
		NewPool.Prototype.OwnerFunc = OwnerFunc;
		NewPool.Prototype.Msg = const_cast<google::protobuf::Message*>(CreateRPCPrototype(OwnerFunc)); 
		Pool.Add(OwnerFunc, NewPool);
		return Pool.Find(OwnerFunc);
	}

	/**
	 * Return an allocated message buffer to the free pool
	 * @param Msg buffer no longer in use by the system
	 */
	void ReturnToPool(MessagePoolElem* Msg)
	{
		if (Msg)
		{
			if ((*Msg)->OwnerFunc.IsValid())
			{
				PoolMapping* FuncPool = Pool.Find((*Msg)->OwnerFunc);
				check(FuncPool);

				Msg->Link(FuncPool->FreeList);
			}
			else
			{
				// UFunction is gone, nothing to do but delete
                UE_LOG_ONLINE(Warning, TEXT("ReturnToPool: UFunction missing, deleting pool element")); 
				delete Msg;
			}
		}
	}
	
	/**
	 * Get an allocated message buffer from the free pool.  The ownership of the buffer
	 * is the responsibility of the caller until they are finished and call ReturnToPool.
	 * @param Function function signature that defines the kind of buffer to return
	 * 
	 * @return allocated buffer ready to be filled with data
	 */
	MessagePoolElem* GetNextFree(UFunction* Function)
	{
		// Create or allocate the appropriate pool to get a buffer from
		MessagePoolElem* NextMsg = NULL;
		TWeakObjectPtr<UFunction> FuncPtr(Function);
		PoolMapping* FuncPool = Pool.Find(FuncPtr);
		if (!FuncPool)
		{
			FuncPool = CreatePool(Function);
		}

		if (FuncPool)
		{
			if (FuncPool->FreeList)
			{
				// Retrieve and clear an existing allocation
				NextMsg = FuncPool->FreeList;
				FuncPool->FreeList = NextMsg->Next();

				NextMsg->Unlink();
				(**NextMsg).Msg->Clear();
			}
			else
			{
				// Create a new allocation from the head prototype
				NextMsg = new MessagePoolElem();
				(**NextMsg).OwnerFunc = FuncPtr;
				(**NextMsg).Msg = FuncPool->Prototype.Msg->New();
			}
		}

		return NextMsg;
	}
};

FProtoMessagePool GProtoPool;

/**
 * Serialize the parameter fields of a given UStruct into a message object
 * @param NewMessage message to fill
 * @param Object structure containing reflection information that matches message
 * @param Parms data to serialize into the new message
 * @param PropFlags any flags relevant to the field iteration
 *
 * @return TRUE if successful, FALSE otherwise
 */
bool SerializeRPCParams(google::protobuf::Message* NewMessage, UStruct* Object, void* Parms, int64 PropFlags)
{
	bool bSuccess = true;
	const google::protobuf::Descriptor* Desc = NewMessage->GetDescriptor();
	const google::protobuf::Reflection* Refl = NewMessage->GetReflection();

	for(TFieldIterator<UProperty> It(Object); It && (It->PropertyFlags & PropFlags); ++It)
	{
		UProperty* Property = *It;
		UClass* PropClass = Property->GetClass();
		const google::protobuf::FieldDescriptor* FieldDesc = Desc->FindFieldByName(TCHAR_TO_UTF8(*Property->GetNameCPP()));
		if (FieldDesc)
		{
			if (FieldDesc->is_repeated())
			{
				int32 NumElements = Property->ArrayDim;
				FScriptArrayHelper* ArrayHelper = NULL;
				if (PropClass == UArrayProperty::StaticClass())
				{
					void* Value = Property->ContainerPtrToValuePtr<uint8>(Parms);
					UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
					Property = ArrayProperty->Inner;
					PropClass = Property->GetClass();

					ArrayHelper = new FScriptArrayHelper(ArrayProperty, Value);
					NumElements = ArrayHelper->Num();
				}

				for (int32 i = 0; i < NumElements; i++)
				{
					void* Value = ArrayHelper ? ArrayHelper->GetRawPtr(i) : Property->ContainerPtrToValuePtr<void>(Parms, i);
					if (PropClass == UIntProperty::StaticClass())
					{
						Refl->AddInt32(NewMessage, FieldDesc, *(int32*)Value);
					}
					else if (PropClass == UFloatProperty::StaticClass())
					{
						Refl->AddFloat(NewMessage, FieldDesc, *(float*)Value);
					}
					else if (PropClass == UBoolProperty::StaticClass() && CastChecked<UBoolProperty>(Property)->IsNativeBool())
					{
						Refl->AddBool(NewMessage, FieldDesc, *(bool*)Value);
					}
					else if (PropClass == UByteProperty::StaticClass() && CastChecked<UByteProperty>(Property)->Enum == NULL)
					{
						// All bytes are stored in a single string
						google::protobuf::string Bytes((ANSICHAR*)Value, NumElements);
						Refl->AddString(NewMessage, FieldDesc, Bytes);
						break;
					}
					else if (PropClass == UStrProperty::StaticClass())
					{
						Refl->AddString(NewMessage, FieldDesc, TCHAR_TO_UTF8(**(FString*)Value));
					}
					else if (PropClass == UStructProperty::StaticClass())
					{
						UStructProperty* StructProperty = Cast<UStructProperty>(Property);
						google::protobuf::Message* SubMessage = Refl->AddMessage(NewMessage, FieldDesc, NULL);
						SerializeRPCParams(SubMessage, StructProperty->Struct, Value, CPF_AllFlags);
					}
					else
					{
						bSuccess = false;
						UE_LOG_ONLINE(Error, TEXT("SerializeRPCParams - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
					}
				}

				if (ArrayHelper)
				{
					delete ArrayHelper;
				}
			}
			else
			{
				if (Property->ArrayDim == 1)
				{
					void* Value = Property->ContainerPtrToValuePtr<uint8>(Parms);
					if (PropClass == UIntProperty::StaticClass())
					{
						Refl->SetInt32(NewMessage, FieldDesc, *(int32*)Value);
					}
					else if (PropClass == UFloatProperty::StaticClass())
					{
						Refl->SetFloat(NewMessage, FieldDesc, *(float*)Value);
					}
					else if (PropClass == UBoolProperty::StaticClass() && CastChecked<UBoolProperty>(Property)->IsNativeBool())
					{
						Refl->SetBool(NewMessage, FieldDesc, *(bool*)Value);
					}
					else if (PropClass == UByteProperty::StaticClass() && CastChecked<UByteProperty>(Property)->Enum == NULL)
					{
						google::protobuf::string Bytes((ANSICHAR*)Value, 1);
						Refl->SetString(NewMessage, FieldDesc, Bytes);
					}
					else if (PropClass == UStrProperty::StaticClass())
					{
						Refl->SetString(NewMessage, FieldDesc, TCHAR_TO_UTF8(**(FString*)Value));
					}
					else if (PropClass == UStructProperty::StaticClass())
					{
						UStructProperty* StructProperty = Cast<UStructProperty>(Property);
						google::protobuf::Message* SubMessage = Refl->MutableMessage(NewMessage, FieldDesc, NULL);
						SerializeRPCParams(SubMessage, StructProperty->Struct, Value, CPF_AllFlags);
					}
					else
					{
						bSuccess = false;
						UE_LOG_ONLINE(Error, TEXT("SerializeRPCParams - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
					}
				}
				else
				{
					bSuccess = false;
					UE_LOG_ONLINE(Error, TEXT("SerializeRPCParams - Property reflection mismatch type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
				}
			}
		}
		else
		{
			bSuccess = false;
			UE_LOG_ONLINE(Error, TEXT("SerializeRPCParams - Property reflection missing type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
		}
	}

	return bSuccess;
}

/**
 * Deserialize the data contained within a message using the parameter fields of a given UStruct 
 * @param Message message containing the data to retrieve
 * @param Object structure containing reflection information that matches message
 * @param Parms memory layout to store the message's data
 * @param PropFlags any flags relevant to the field iteration
 *
 * @return TRUE if successful, FALSE otherwise
 */
bool DeserializeRPCParams(const google::protobuf::Message* Message, UStruct* Object, void* Parms, int64 PropFlags)
{
	bool bSuccess = true;
	const google::protobuf::Descriptor* Desc = Message->GetDescriptor();
	const google::protobuf::Reflection* Refl = Message->GetReflection();

	for(TFieldIterator<UProperty> It(Object); It && (It->PropertyFlags & PropFlags); ++It)
	{
		UProperty* Property = *It;
		UClass* PropClass = Property->GetClass();
		const google::protobuf::FieldDescriptor* FieldDesc = Desc->FindFieldByName(TCHAR_TO_UTF8(*Property->GetNameCPP()));
		if (FieldDesc)
		{
			if (FieldDesc->is_repeated())
			{
				int32 NumElements = Refl->FieldSize(*Message, FieldDesc);
				FScriptArrayHelper* ArrayHelper = NULL;
				if (PropClass == UArrayProperty::StaticClass())
				{
					void* Value = Property->ContainerPtrToValuePtr<uint8>(Parms);
					UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
					Property = ArrayProperty->Inner;
					PropClass = Property->GetClass();

					ArrayHelper = new FScriptArrayHelper(ArrayProperty, Value);
					ArrayHelper->EmptyAndAddValues(NumElements);
				}
				else
				{
					if (Property->ArrayDim < NumElements)
					{
						UE_LOG_ONLINE(Error, TEXT("DeserializeRPCParams - Static array mismatch (%d != %d) property type '%s': %s"), NumElements, Property->ArrayDim, *PropClass->GetName(), *Property->GetPathName());
						NumElements = Property->ArrayDim;
					}
				}

				for (int32 i = 0; i < NumElements; i++)
				{
					void* Value = ArrayHelper ? ArrayHelper->GetRawPtr(i) : Property->ContainerPtrToValuePtr<uint8>(Parms, i);
					if (PropClass == UIntProperty::StaticClass())
					{
						*(int32*)Value = Refl->GetRepeatedInt32(*Message, FieldDesc, i);
					}
					else if (PropClass == UFloatProperty::StaticClass())
					{
						*(float*)Value = Refl->GetRepeatedFloat(*Message, FieldDesc, i);
					}
					else if (PropClass == UBoolProperty::StaticClass() && CastChecked<UBoolProperty>(Property)->IsNativeBool())
					{
						*(bool*)Value = Refl->GetRepeatedBool(*Message, FieldDesc, i);
					}
					else if (PropClass == UByteProperty::StaticClass() && CastChecked<UByteProperty>(Property)->Enum == NULL)
					{
						const google::protobuf::string& String = Refl->GetRepeatedStringReference(*Message, FieldDesc, i, NULL);
						if (ArrayHelper)
						{
							// Message only contains one string, but we need an array of bytes
							ArrayHelper->AddValues(String.size() - 1);
							Value = ArrayHelper->GetRawPtr(i);
						}

						FMemory::Memcpy(Value, String.c_str(), String.size());
						break;
					}
					else if (PropClass == UStrProperty::StaticClass())
					{
						const google::protobuf::string& String = Refl->GetRepeatedStringReference(*Message, FieldDesc, i, NULL);
						*(FString*)Value = UTF8_TO_TCHAR(String.c_str());
					}
					else if (PropClass == UStructProperty::StaticClass())
					{
						UStructProperty* StructProperty = Cast<UStructProperty>(Property);
						const google::protobuf::Message& SubMessage = Refl->GetRepeatedMessage(*Message, FieldDesc, i);
						DeserializeRPCParams(&SubMessage, StructProperty->Struct, Value, CPF_AllFlags);
					}
					else
					{
						bSuccess = false;
						UE_LOG_ONLINE(Error, TEXT("DeserializeRPCParams - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
					}
				}

				if (ArrayHelper)
				{
					delete ArrayHelper;
				}
			}
			else
			{
				if (Property->ArrayDim == 1)
				{
					void* Value = Property->ContainerPtrToValuePtr<uint8>(Parms);
					if (PropClass == UIntProperty::StaticClass())
					{
						*(int32*)Value = Refl->GetInt32(*Message, FieldDesc);
					}
					else if (PropClass == UFloatProperty::StaticClass())
					{
						*(float*)Value = Refl->GetFloat(*Message, FieldDesc);
					}
					else if (PropClass == UBoolProperty::StaticClass() && CastChecked<UBoolProperty>(Property)->IsNativeBool())
					{
						*(bool*)Value = Refl->GetBool(*Message, FieldDesc);
					}
					else if (PropClass == UByteProperty::StaticClass() && CastChecked<UByteProperty>(Property)->Enum == NULL)
					{
						int32 NumBytes = Refl->FieldSize(*Message, FieldDesc);
						google::protobuf::string Bytes = Refl->GetString(*Message, FieldDesc);
						FMemory::Memcpy(Value, Bytes.c_str(), NumBytes);
					}
					else if (PropClass == UStrProperty::StaticClass())
					{
						google::protobuf::string String = Refl->GetString(*Message, FieldDesc);
						*(FString*)Value = UTF8_TO_TCHAR(String.c_str());
					}
					else if (PropClass == UStructProperty::StaticClass())
					{
						UStructProperty* StructProperty = Cast<UStructProperty>(Property);
						const google::protobuf::Message& SubMessage = Refl->GetMessage(*Message, FieldDesc);
						DeserializeRPCParams(&SubMessage, StructProperty->Struct, Value, CPF_AllFlags);
					}
					else
					{
						bSuccess = false;
						UE_LOG_ONLINE(Error, TEXT("DeserializeRPCParams - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
					}
				}
				else
				{
					bSuccess = false;
					UE_LOG_ONLINE(Error, TEXT("DeserializeRPCParams - Property reflection mismatch type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
				}
			}
		}
		else
		{
			bSuccess = false;
			UE_LOG_ONLINE(Error, TEXT("DeserializeRPCParams - Property reflection missing type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
		}
	}

	return bSuccess;
}

TMap<int32, UFunction*> RPCFunctionMap;
UFunction* FindNetServiceFunctionById(int16 RPCId)
{
	UFunction** Function = RPCFunctionMap.Find(RPCId);
	if (!Function)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* Class = *ClassIt;
			if (Class->IsChildOf(AActor::StaticClass())
				&& !(Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)))
			{
				for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt) 
				{
					UFunction* CurFunc = *FuncIt;
					if (CurFunc->RPCId > 0)
					{
						RPCFunctionMap.Add(CurFunc->RPCId, CurFunc);
					}
				}
			}
		}
		
		Function = RPCFunctionMap.Find(RPCId);
	}

	return *Function;
}

bool DeserializeMessage(MessagePoolElem* Message, uint8** Params)
{
	bool bSuccess = false;
	if (Message)
	{
		google::protobuf::Message* NewMessage = (*Message)->Msg;
		UFunction* Function = (*Message)->OwnerFunc.Get();
		if (NewMessage && Function && Function->ParmsSize > 0)
		{
			*Params = new uint8[Function->ParmsSize];
			FMemory::Memzero(*Params, Function->ParmsSize);
			if (DeserializeRPCParams(NewMessage, Function, *Params, CPF_Parm))
			{
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

bool FOnlineAsyncMsgSteam::Deserialize()
{
	bWasSuccessful = DeserializeMessage(ResponseMessage, &ResponseBuffer);
	return bWasSuccessful;
}

FOnlineAsyncMsgSteam* FOnlineAsyncMsgSteam::Serialize(AActor* Actor, UFunction* Function, void* Params)
{
	// Allocate the message buffer
	MessagePoolElem* Msg = GProtoPool.GetNextFree(Function);

	// Allocate the response buffer if necessary
	MessagePoolElem* ResponseMsg = NULL;
	if (Function->RPCResponseId > 0)
	{
		UFunction* ResponseFunction = FindNetServiceFunctionById(Function->RPCResponseId);
		ResponseMsg = GProtoPool.GetNextFree(ResponseFunction);
	}

	google::protobuf::Message* NewMessage = (*Msg)->Msg;
	if (NewMessage && SerializeRPCParams(NewMessage, Function, Params, CPF_Parm))
	{
		TWeakObjectPtr<AActor> ActorPtr(Actor);
		return new FOnlineAsyncMsgSteam(ActorPtr, Function->RPCId, Function->RPCResponseId, Msg, ResponseMsg);
	}
	else
	{
		GProtoPool.ReturnToPool(Msg);
		GProtoPool.ReturnToPool(ResponseMsg);
	}

	return NULL;
}

void FOnlineAsyncMsgSteam::Destroy()
{
	if (ResponseBuffer)
	{
		UFunction* Function = (*ResponseMessage)->OwnerFunc.Get();
		if (Function)
		{
			// Destroy the parameters.
			// warning: highly dependent on UObject::ProcessEvent freeing of parms!
			for(UProperty* Destruct = Function->DestructorLink; Destruct; Destruct = Destruct->DestructorLinkNext)
			{
				if(Destruct->IsInContainer(Function->ParmsSize))
				{
					Destruct->DestroyValue_InContainer(ResponseBuffer);
				}
			}
		}
        else
        {
            UE_LOG_ONLINE(Warning, TEXT("MsgDestroy: UFunction missing, unable to free buffer.")); 
        }

		delete [] ResponseBuffer;
		ResponseBuffer = NULL;
	}

	GProtoPool.ReturnToPool(Message);
	GProtoPool.ReturnToPool(ResponseMessage);
	ActorPtr = NULL;
}


/**
 *	Get a human readable description of task
 */
FString FOnlineAsyncMsgSteam::ToString() const
{
	const AActor* Actor = ActorPtr.Get();
	return FString::Printf(TEXT("Steam message response received. Actor: %s MsgType: %d ResponseType: %d Success: %d"), 
		Actor ? *Actor->GetName() : TEXT("N/A"), MessageType, ResponseMessageType, bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

ProtoMsg* FOnlineAsyncMsgSteam::GetParams()
{
	return (*Message)->Msg;
}

ProtoMsg* FOnlineAsyncMsgSteam::GetResponse()
{
	return (*ResponseMessage)->Msg;
}

/**
 *	Async task is given a chance to trigger it's delegates
 */
void FOnlineAsyncMsgSteam::TriggerDelegates() 
{
	FOnlineAsyncItem::TriggerDelegates();

	AActor* Actor = ActorPtr.Get();
	if (Actor)
	{
		UFunction* Function = (*ResponseMessage)->OwnerFunc.Get();
		if (Function)
		{
			Actor->ProcessEvent(Function, ResponseBuffer);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("UFunction missing, unable to call RPC response.")); 
		}
	}

	Destroy();
}

#endif //WITH_STEAMGC