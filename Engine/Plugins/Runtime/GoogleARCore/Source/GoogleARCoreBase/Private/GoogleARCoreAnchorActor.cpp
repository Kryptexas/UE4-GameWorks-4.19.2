// Copyright 2017 Google Inc.
#include "GoogleARCoreAnchorActor.h"
#include "GoogleARCoreFunctionLibrary.h"

void AGoogleARCoreAnchorActor::Tick(float DeltaSeconds)
{
	if (bTrackingEnabled && ARAnchorObject != nullptr && ARAnchorObject->GetTrackingState() == EARTrackingState::Tracking)
	{
		SetActorTransform(ARAnchorObject->GetLocalToWorldTransform());
	}

	if ((bHideWhenNotCurrentlyTracking || bDestroyWhenStoppedTracking) && ARAnchorObject != nullptr)
	{
		switch (ARAnchorObject->GetTrackingState())
		{
		case EARTrackingState::Tracking:
			SetActorHiddenInGame(false);
			break;
		case EARTrackingState::NotTracking:
			SetActorHiddenInGame(bHideWhenNotCurrentlyTracking);
			break;
		case EARTrackingState::StoppedTracking:
			if (bDestroyWhenStoppedTracking)
			{
				Destroy();
			}
			else
			{
				SetActorHiddenInGame(bHideWhenNotCurrentlyTracking);
			}
			break;
		default:
			// This case should never be reached, do nothing.
			break;
		}
	}

	Super::Tick(DeltaSeconds);
}

void AGoogleARCoreAnchorActor::BeginDestroy()
{
	if (bRemoveAnchorObjectWhenDestroyed && ARAnchorObject)
	{
		UGoogleARCoreSessionFunctionLibrary::RemoveGoogleARAnchorObject(ARAnchorObject);
	}

	Super::BeginDestroy();
}

void AGoogleARCoreAnchorActor::SetARAnchor(UARPin* InARAnchorObject)
{
	if (ARAnchorObject != nullptr && bRemoveAnchorObjectWhenAnchorChanged)
	{
		UGoogleARCoreSessionFunctionLibrary::RemoveGoogleARAnchorObject(ARAnchorObject);
	}

	ARAnchorObject = InARAnchorObject;
	SetActorTransform(ARAnchorObject->GetLocalToWorldTransform());
}

UARPin* AGoogleARCoreAnchorActor::GetARAnchor()
{
	return ARAnchorObject;
}