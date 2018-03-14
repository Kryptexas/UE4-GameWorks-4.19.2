// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARSystem.h"
#include "ARTrackable.h"
#include "ILiveLinkSource.h"


/** Interface that publishes face ar blend shape information */
class APPLEARKIT_API IARKitBlendShapePublisher
{
public:
	virtual void PublishBlendShapes(FName SubjectName, double Timestamp, uint32 FrameNumber, const FARBlendShapeMap& BlendShapes) = 0;
};

/** Interface that publishes face ar blend shape information via LiveLink */
class APPLEARKIT_API ILiveLinkSourceARKit :
	public IARKitBlendShapePublisher,
	public ILiveLinkSource
{
};

/** Factory that creates and registers the sources with the LiveLink client */
class APPLEARKIT_API FAppleARKitLiveLinkSourceFactory
{
public:
	/** Creates a face mesh source that will autobind to the tracked face mesh */
	static TSharedPtr<ILiveLinkSourceARKit> CreateLiveLinkSource(bool bCreateRemotePublisher);

	/** Creates the listener that will receive remote events and publish them locally */
	static void CreateLiveLinkRemoteListener();
};
