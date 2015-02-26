// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetEditorSelectedItem.h"

//////////////////////////////////////////////////////////////////////////
// HSpriteSelectableObjectHitProxy

struct HSpriteSelectableObjectHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(PAPER2DEDITOR_API);

	TSharedPtr<FSelectedItem> Data;

	HSpriteSelectableObjectHitProxy(TSharedPtr<FSelectedItem> InData)
		: HHitProxy(HPP_UI)
		, Data(InData)
	{
	}
};

//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedSocket

class FSpriteSelectedSocket : public FSelectedItem
{
public:
	FName SocketName;
	TWeakObjectPtr<UPrimitiveComponent> PreviewComponentPtr;

	static const FName SocketTypeID;

public:
	FSpriteSelectedSocket();

	// FSelectedItem interface
	virtual bool Equals(const FSelectedItem& OtherItem) const override;
	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode) override;
	FVector GetWorldPos() const override;
	virtual void SplitEdge() override;
	// End of FSelectedItem interface
};

//////////////////////////////////////////////////////////////////////////
// FSocketEditingHelper

class FSocketEditingHelper
{
public:
	static void DrawSockets(UPrimitiveComponent* PreviewComponent, const FSceneView* View, FPrimitiveDrawInterface* PDI);
	static void DrawSocketNames(UPrimitiveComponent* PreviewComponent, FViewport& Viewport, FSceneView& View, FCanvas& Canvas);
};
