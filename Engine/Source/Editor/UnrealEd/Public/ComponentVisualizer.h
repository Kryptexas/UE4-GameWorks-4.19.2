// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct HComponentVisProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(UNREALED_API);

	HComponentVisProxy(const UActorComponent* InComponent, EHitProxyPriority InPriority = HPP_Wireframe) 
	: HHitProxy(InPriority)
	, Component(InComponent)
	{}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}

	TWeakObjectPtr<const UActorComponent> Component;
};

/** Base class for a component visualizer, that draw editor information for a particular component class */
class UNREALED_API FComponentVisualizer : public TSharedFromThis<FComponentVisualizer>
{
public:
	/** Draw visualization for the supplied component */
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) {}
	/** */
	virtual bool VisProxyHandleClick(HComponentVisProxy* VisProxy) { return false; }
	/** */
	virtual void EndEditing() {}
	/** */
	virtual bool GetWidgetLocation(FVector& OutLocation) { return false; }
	/** */
	virtual bool HandleInputDelta(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltalRotate, FVector& DeltaScale) { return false; }
	/** */
	virtual bool HandleInputKey(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,FKey Key,EInputEvent Event) { return false; }

	/** Find the name of the property that points to this component */
	static FName GetComponentPropertyName(const UActorComponent* Component);
	/** Get a component pointer from the property name */
	static UActorComponent* GetComponentFromPropertyName(const AActor* CompOwner, FName PropertyName);
};