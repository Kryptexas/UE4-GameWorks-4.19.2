// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MotionDelayBuffer.h"
#include "UObject/WeakObjectPtr.h"


class UMixedRealityCaptureComponent;
class FMotionDelayBuffer;
class FSceneInterface;

class FMRLatencyViewExtension : public FMotionDelayClient
{
public:
	FMRLatencyViewExtension(const FAutoRegister& AutoRegister, UMixedRealityCaptureComponent* Owner);
	virtual ~FMRLatencyViewExtension() {}

	bool SetupPreCapture(FSceneInterface* Scene);
	void SetupPostCapture(FSceneInterface* Scene);

public:
	/** FMotionDelayClient interface */
	virtual uint32 GetDesiredDelay() const override;

public:
	/** ISceneViewExtension interface */
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override;

private:
	TWeakObjectPtr<UMixedRealityCaptureComponent> Owner;
	uint32 CachedRenderDelay = 0;
};
