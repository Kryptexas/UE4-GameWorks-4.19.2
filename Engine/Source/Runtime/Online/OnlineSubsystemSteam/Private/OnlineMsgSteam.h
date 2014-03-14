// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineAsyncTaskManagerSteam.h"
#include "OnlineSubsystemSteamPackage.h"

#if WITH_STEAMGC

namespace google
{
	namespace protobuf
	{
		class MessageLite;
		class Message;
	}
}

/**
 * Singular element of the free pool, contains the function 
 * that defines the message and the message itself
 */
class MessagePoolElement
{
public:

	/** Function that describes the dynamic message */
	TWeakObjectPtr<class UFunction> OwnerFunc;
	/** Allocated message for use */
	google::protobuf::Message* Msg;

	MessagePoolElement() :
		OwnerFunc(NULL),
		Msg(NULL)
	{
	}

	MessagePoolElement(UFunction* InOwnerFunc, google::protobuf::Message* InMsg) :
		OwnerFunc(InOwnerFunc),
		Msg(InMsg)
	{
	}
};

typedef TLinkedList<MessagePoolElement> MessagePoolElem;

// Hide google namespace
typedef ::google::protobuf::MessageLite ProtoMsg;
class FOnlineAsyncMsgSteam : public FOnlineAsyncItem
{
PACKAGE_SCOPE:

	/** Actor that initiated the message */
	TWeakObjectPtr<class AActor> ActorPtr;
	/** Unique Id defining the message type sent to the service */
	int32 MessageType;
	/** Unique Id defining the expected response message type sent from the service */
	int32 ResponseMessageType;
	/** Buffer containing the serialized outgoing message */
	MessagePoolElem* Message;
	/** Buffer containing the serialized response message */
	MessagePoolElem* ResponseMessage;
	/** Buffer containing the serialized response message in UObject layout */
	uint8* ResponseBuffer;

	FOnlineAsyncMsgSteam() :
		ActorPtr(NULL),
		MessageType(-1),
		ResponseMessageType(-1),
		Message(NULL),
		ResponseMessage(NULL),
		ResponseBuffer(NULL),
		bWasSuccessful(false)
	{
	}

public:

	/** Was the roundtrip request/response successful */
	bool bWasSuccessful;

	FOnlineAsyncMsgSteam(TWeakObjectPtr<class AActor> InActorPtr, int32 InMessageType, int32 InResponseMessageType, MessagePoolElem* InMessage, MessagePoolElem* InResponseMessage) :
		ActorPtr(InActorPtr),
		MessageType(InMessageType),
		ResponseMessageType(InResponseMessageType),
		Message(InMessage),
		ResponseMessage(InResponseMessage),
		ResponseBuffer(NULL),
		bWasSuccessful(false)
	{
	}

	~FOnlineAsyncMsgSteam()
	{

	}

	/**
	 * Serialize the parameter fields of a given UStruct into a message object
	 * @param Actor actor sending the message
	 * @param Function function containing parameter definition
	 * @param Parms data to serialize into the new message
	 *
	 * @return Message object ready to send if successful, NULL otherwise
	 */
	static FOnlineAsyncMsgSteam* Serialize(class AActor* Actor, UFunction* Function, void* Params);

	/**
	 * Deserialize the data contained within a response message
	 *
	 * @return TRUE if successful, FALSE otherwise
	 */
	bool Deserialize();

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const OVERRIDE;

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() OVERRIDE;

	/** Get Unique Id defining the message type sent to the service */
	int32 GetMsgType() const
	{
		return MessageType;
	}

	/** Get Unique Id defining the expected response message type sent from the service */
	int32 GetResponseMsgType() const
	{
		return ResponseMessageType;
	}
	
	/** 
	 * Get buffer containing the serialized outgoing message
	 * @return outgoing buffer
	 */
	ProtoMsg* GetParams();

    /** 
	 * Buffer containing the serialized response message
	 * @return incoming response buffer
	 */
	ProtoMsg* GetResponse();

	/**
	 * Deallocate and destroy all memory related to this message
	 */
	void Destroy();
};

#endif //WITH_STEAMGC