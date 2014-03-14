// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapDetailsCustomization

class FPaperTileMapDetailsCustomization : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) OVERRIDE;
	// End of IDetailCustomization interface

private:
	TWeakObjectPtr<class UPaperTileMapRenderComponent> TileMapPtr;
private:
	FReply AddLayerClicked();
	FReply AddCollisionLayerClicked();
	FReply EnterTileMapEditingMode();
	EVisibility GetNonEditModeVisibility() const;

	class UPaperTileLayer* AddLayer(bool bCollisionLayer);
};
