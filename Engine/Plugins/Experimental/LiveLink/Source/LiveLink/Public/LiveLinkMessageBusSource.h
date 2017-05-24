// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ILiveLinkSource.h"
#include "MessageEndpoint.h"
#include "IMessageContext.h"

class ILiveLinkClient;
struct FLiveLinkPong;
struct FLiveLinkSubjectData;
struct FLiveLinkSubjectFrame;

class FLiveLinkMessageBusSource : public ILiveLinkSource
{
public:

	FLiveLinkMessageBusSource(const FText& InSourceType, const FText& InSourceMachineName, const FMessageAddress& InConnectionAddress)
		: ConnectionAddress(InConnectionAddress)
		, SourceType(InSourceType)
		, SourceMachineName(InSourceMachineName)
	{}

	virtual void ReceiveClient(ILiveLinkClient* InClient);

	void Connect(FMessageAddress& Address);

	virtual bool IsSourceStillValid();

	virtual bool RequestSourceShutdown();

	virtual FText GetSourceType() const { return SourceType; }
	virtual FText GetSourceMachineName() const { return SourceMachineName; }
	virtual FText GetSourceStatus() const { return SourceStatus; }

private:

	void HandleSubjectData(const FLiveLinkSubjectData& Message, const IMessageContextRef& Context);
	void HandleSubjectFrame(const FLiveLinkSubjectFrame& Message, const IMessageContextRef& Context);

	ILiveLinkClient* Client;

	FMessageEndpointPtr MessageEndpoint;

	FMessageAddress ConnectionAddress;

	FText SourceType;
	FText SourceMachineName;
	FText SourceStatus;
};