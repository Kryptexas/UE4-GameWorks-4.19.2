// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkMessageBusSource.h"
#include "LiveLinkMessages.h"
#include "ILiveLinkClient.h"

#include "MessageEndpointBuilder.h"

void FLiveLinkMessageBusSource::ReceiveClient(ILiveLinkClient* InClient)
{
	Client = InClient;

	MessageEndpoint =	FMessageEndpoint::Builder(TEXT("LiveLinkMessageBusSource"))
						.Handling<FLiveLinkSubjectData>(this, &FLiveLinkMessageBusSource::HandleSubjectData)
						.Handling<FLiveLinkSubjectFrame>(this, &FLiveLinkMessageBusSource::HandleSubjectFrame);


	MessageEndpoint->Send(new FLiveLinkConnect(), ConnectionAddress);
}

bool FLiveLinkMessageBusSource::IsSourceStillValid()
{
	return true;
}

bool FLiveLinkMessageBusSource::RequestSourceShutdown()
{
	FMessageEndpoint::SafeRelease(MessageEndpoint);
	return true;
}

void FLiveLinkMessageBusSource::HandleSubjectData(const FLiveLinkSubjectData& Message, const IMessageContextRef& Context)
{
	//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("HandleSubjectData %s\n"), *Message.SubjectName);
	/*for (const FString& Name : Message.BoneNames)
	{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("\tName: %s\n"), *Name);
	}*/
	/*FScopeLock Lock(&GBoneDataCS);

	if (BoneID != (Message.BoneID - 1))
	{
	UE_LOG(LogTemp, Warning, TEXT("BONE ID SKIP Was On:%i Now:%i"), BoneID, Message.BoneID);
	}
	if (BoneNames.Num() == Message.BoneNames.Num() || BoneNames.Num() == 0)
	{
	BoneNames.Reset();
	for (const FString& Name : Message.BoneNames)
	{
	BoneNames.Add(FName(*Name));
	}
	//BoneTransforms.Reset();
	BoneID = Message.BoneID;
	}
	else
	{
	UE_LOG(LogTemp, Warning, TEXT("INVALID BONE NAMES RECIEVED %i != existing %i"), Message.BoneNames.Num(), BoneNames.Num());
	}*/

	static FName SubjectName(TEXT("Maya"));
	Client->PushSubjectSkeleton(SubjectName, Message.RefSkeleton);
}

void FLiveLinkMessageBusSource::HandleSubjectFrame(const FLiveLinkSubjectFrame& Message, const IMessageContextRef& Context)
{
	//FPlatformMisc::LowLevelOutputDebugString(TEXT("HandleSubjectFrame\n"));
	/*if (BoneID != Message.BoneID)
	{
	UE_LOG(LogTemp, Warning, TEXT("BONE ID MISMATCH Exp:%i Got:%i"), BoneID, Message.BoneID);
	}*/
	/*for (const FTransform& T : Message.Transforms)
	{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("\tTransform: %s\n"), *T.ToString());
	}*/

	static FName SubjectName(TEXT("Maya"));
	Client->PushSubjectData(SubjectName, Message.Transforms);
}