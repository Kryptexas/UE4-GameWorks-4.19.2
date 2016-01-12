// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HeadMountedDisplayPrivate.h"

void GatherSceneProxies(USceneComponent* Component, FPrimitiveSceneProxy* SceneProxies[], int32& SceneProxyCount, int32 MaxSceneProxyCount)
{
	// If a scene proxy is present, cache it
	UPrimitiveComponent* PrimitiveComponent = dynamic_cast<UPrimitiveComponent*>(Component);
	if (PrimitiveComponent && PrimitiveComponent->SceneProxy)
	{
		check(SceneProxyCount < MaxSceneProxyCount);
		SceneProxies[SceneProxyCount++] = PrimitiveComponent->SceneProxy;
	}

	// Gather children proxies
	const int32 ChildCount = Component->GetNumChildrenComponents();
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		USceneComponent* ChildComponent = Component->GetChildComponent(ChildIndex);
		if (!ChildComponent)
		{
			continue;
		}

		GatherSceneProxies(ChildComponent, SceneProxies, SceneProxyCount, MaxSceneProxyCount);
	}
}

/**
* HMD device console vars
*/
static TAutoConsoleVariable<int32> CVarHiddenAreaMask(
	TEXT("vr.HiddenAreaMask"),
	1,
	TEXT("0 to disable hidden area mask, 1 to enable."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

class FHeadMountedDisplayModule : public IHeadMountedDisplayModule
{
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay()
	{
		TSharedPtr<IHeadMountedDisplay, ESPMode::ThreadSafe> DummyVal = NULL;
		return DummyVal;
	}

	FString GetModulePriorityKeyName() const
	{
		return FString(TEXT("Default"));
	}
};

IMPLEMENT_MODULE( FHeadMountedDisplayModule, HeadMountedDisplay );

IHeadMountedDisplay::IHeadMountedDisplay()
	: LateUpdateSceneProxyCount(0)
{
	PreFullScreenRect = FSlateRect(-1.f, -1.f, -1.f, -1.f);
}

void IHeadMountedDisplay::PushPreFullScreenRect(const FSlateRect& InPreFullScreenRect)
{
	PreFullScreenRect = InPreFullScreenRect;
}

void IHeadMountedDisplay::PopPreFullScreenRect(FSlateRect& OutPreFullScreenRect)
{
	OutPreFullScreenRect = PreFullScreenRect;
	PreFullScreenRect = FSlateRect(-1.f, -1.f, -1.f, -1.f);
}

void IHeadMountedDisplay::SetupLateUpdate(const FTransform& ParentToWorld, USceneComponent* Component)
{
	LateUpdateParentToWorld = ParentToWorld;
	LateUpdateSceneProxyCount = 0;
	GatherSceneProxies(Component, LateUpdateSceneProxies, LateUpdateSceneProxyCount, ARRAY_COUNT(LateUpdateSceneProxies));
}

void IHeadMountedDisplay::ApplyLateUpdate(const FTransform& OldRelativeTransform, const FTransform& NewRelativeTransform)
{
	if (!LateUpdateSceneProxyCount)
	{
		return;
	}

	const FTransform OldCameraTransform = OldRelativeTransform * LateUpdateParentToWorld;
	const FTransform NewCameraTransform = NewRelativeTransform * LateUpdateParentToWorld;
	const FMatrix LateUpdateTransform = (OldCameraTransform.Inverse() * NewCameraTransform).ToMatrixWithScale();

	// Apply delta to the affected scene proxies
	for (int32 ProxyIndex = 0; ProxyIndex < LateUpdateSceneProxyCount; ++ProxyIndex)
	{
		LateUpdateSceneProxies[ProxyIndex]->ApplyLateUpdateTransform(LateUpdateTransform);
	}
	LateUpdateSceneProxyCount = 0;
}