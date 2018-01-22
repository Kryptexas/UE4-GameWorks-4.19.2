// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AppleARKitLiveLinkSourceFactory.h"
#include "ILiveLinkClient.h"
#include "Tickable.h"
#include "ARTrackable.h"

#include "IPAddress.h"
#include "Sockets.h"
#include "NboSerializer.h"


class UARFaceGeometry;

/** Listens for LiveLink publication events and publishes them to the local LiveLinkClient */
class FAppleARKitLiveLinkRemoteListener :
	public FTickableGameObject
{
public:
	FAppleARKitLiveLinkRemoteListener();
	virtual ~FAppleARKitLiveLinkRemoteListener();

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return RecvSocket != nullptr; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAppleARKitLiveLinkRemoteListener, STATGROUP_Tickables);
	}
	virtual bool IsTickableInEditor() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	// End FTickableGameObject

	bool InitReceiveSocket();

private:
	void InitLiveLinkSource();

	/** Socket that is read from to publish remote data to this local LiveLinkClient */
	FSocket* RecvSocket;
	/** Buffer to reuse for receiving data */
	TArray<uint8> RecvBuffer;
	/** The reused blend shape map to avoid allocs/frees */
	FARBlendShapeMap BlendShapes;

	/** The source used to publish events from the remote socket */
	TSharedPtr<ILiveLinkSourceARKit> Source;
};

/** Forwards face blend shapes to another machine for LiveLink publication */
class FAppleARKitLiveLinkRemotePublisher :
	public IARKitBlendShapePublisher
{
public:
	FAppleARKitLiveLinkRemotePublisher();
	virtual ~FAppleARKitLiveLinkRemotePublisher();

	// IARKitBlendShapePublisher interface
	virtual void PublishBlendShapes(FName SubjectName, double Timestamp, uint32 FrameNumber, const FARBlendShapeMap& FaceBlendShapes) override;
	// End IARKitBlendShapePublisher

	bool InitSendSocket();

private:
	TSharedRef<FInternetAddr> GetSendAddress();

	/** Socket to send the data via to the remote listener */
	FSocket* SendSocket;
	/** The ram buffer to serialize the packet to */
	FNboSerializeToBuffer SendBuffer;
};

/** Publishes face blend shapes to LiveLink for use locally */
class FAppleARKitLiveLinkSource :
	public ILiveLinkSourceARKit
{
public:
	FAppleARKitLiveLinkSource(bool bCreateRemotePublisher);
	virtual ~FAppleARKitLiveLinkSource() {}

private:
	// ILiveLinkSource interface
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
	virtual bool IsSourceStillValid() override;
	virtual bool RequestSourceShutdown() override;
	virtual FText GetSourceMachineName() const override;
	virtual FText GetSourceStatus() const override;
	virtual FText GetSourceType() const override;
	// End ILiveLinkSource

	// IARKitBlendShapePublisher interface
	virtual void PublishBlendShapes(FName SubjectName, double Timestamp, uint32 FrameNumber, const FARBlendShapeMap& FaceBlendShapes) override;
	// End IARKitBlendShapePublisher

	/** The local client to push data updates to */
	ILiveLinkClient* Client;

	/** Our identifier in LiveLink */
	FGuid SourceGuid;

	/** Provider to push data to remote listeners */
	TSharedPtr<IARKitBlendShapePublisher> RemoteLiveLinkPublisher;

	/** The last time we sent the data. Used to not send redundant data */
	uint32 LastFramePublished;

	/** Used to track names changes */
	FName LastSubjectName;
};
