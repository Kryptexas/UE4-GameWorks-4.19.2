// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISCSEditorCustomization.h"

class FInstancedStaticMeshSCSEditorCustomization : public ISCSEditorCustomization
{
public:
	static TSharedRef<ISCSEditorCustomization> MakeInstance(TSharedRef< class IBlueprintEditor > InBlueprintEditor);

public:
	/** ISCSEditorCustomization interface */
	virtual bool HandleViewportClick(const TSharedRef<FEditorViewportClient>& InViewportClient, class FSceneView& InView, class HHitProxy* InHitProxy, FKey InKey, EInputEvent InEvent, uint32 InHitX, uint32 InHitY) OVERRIDE;
	virtual bool HandleViewportDrag(class USceneComponent* InComponentScene, class USceneComponent* InComponentTemplate, const FVector& InDeltaTranslation, const FRotator& InDeltaRotation, const FVector& InDeltaScale, const FVector& InPivot) OVERRIDE;
	virtual bool HandleGetWidgetLocation(class USceneComponent* InSceneComponent, FVector& OutWidgetLocation) OVERRIDE;
	virtual bool HandleGetWidgetTransform(class USceneComponent* InSceneComponent, FMatrix& OutWidgetTransform) OVERRIDE;

private:
	/** The blueprint editor we are bound to */
	TWeakPtr<class IBlueprintEditor> BlueprintEditorPtr;
};