// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkProvider.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "LiveLinkMessages.h"

#include "HAL/PlatformProcess.h"

struct FLiveLinkProvider : public ILiveLinkProvider
{
private:
	const FString ProviderName;
	
	const FString MachineName;

	int32 SubjectID;

	FMessageEndpointPtr MessageEndpoint;

	FMessageAddress ConnectionAddress;

	void HandlePingMessage(const FLiveLinkPing& Message, const IMessageContextRef& Context);
	void HandleConnectMessage(const FLiveLinkConnect& Message, const IMessageContextRef& Context);

public:
	FLiveLinkProvider(const FString& InProviderName)
		: ProviderName(InProviderName)
		, MachineName(FPlatformProcess::ComputerName())
		, SubjectID(0)
	{
		MessageEndpoint = FMessageEndpoint::Builder(*InProviderName)
			.ReceivingOnAnyThread()
			.Handling<FLiveLinkPing>(this, &FLiveLinkProvider::HandlePingMessage)
			.Handling<FLiveLinkConnect>(this, &FLiveLinkProvider::HandleConnectMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FLiveLinkPing>();
		}
	}

	virtual void SendSubject(const FString& SubjectName, const TArray<FName>& BoneNames, const TArray<FTransform>& BoneTransforms)
	{
		static bool bSentNames = false;
		if (ConnectionAddress.IsValid())
		{
			++SubjectID;
			if(!bSentNames)
			{
				bSentNames = true;
				FLiveLinkSubjectData* SubjectData = new FLiveLinkSubjectData;
				SubjectData->RefSkeleton.SetBoneNames(BoneNames);
				SubjectData->SubjectName = SubjectName;
				SubjectData->BoneID = SubjectID;

				MessageEndpoint->Send(SubjectData, ConnectionAddress);
			}

			FLiveLinkSubjectFrame* SubjectFrame = new FLiveLinkSubjectFrame;
			SubjectFrame->Transforms = BoneTransforms;
			SubjectFrame->BoneID = SubjectID;

			MessageEndpoint->Send(SubjectFrame, ConnectionAddress);
		}
	}

	virtual ~FLiveLinkProvider()
	{
		FPlatformMisc::LowLevelOutputDebugString(TEXT("Destroyed"));
	}
};

void FLiveLinkProvider::HandlePingMessage(const FLiveLinkPing& Message, const IMessageContextRef& Context)
{
	//if (!ConnectionAddress.IsValid()) // Stop us getting locked out of Maya while address lifetime needs work
	{
		MessageEndpoint->Send(new FLiveLinkPong(ProviderName, MachineName, Message.PollRequest), Context->GetSender());
	}
}

void FLiveLinkProvider::HandleConnectMessage(const FLiveLinkConnect& Message, const IMessageContextRef& Context)
{
	//if (!ConnectionAddress.IsValid())
	{
		ConnectionAddress = Context->GetSender();
	}
}

TSharedPtr<ILiveLinkProvider> ILiveLinkProvider::CreateLiveLinkProvider(const FString& ProviderName)
{
	return MakeShareable(new FLiveLinkProvider(ProviderName));
}