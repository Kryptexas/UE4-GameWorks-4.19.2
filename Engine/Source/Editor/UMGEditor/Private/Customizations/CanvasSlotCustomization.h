// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCanvasSlotCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<class IDetailCustomization> MakeInstance(UBlueprint* Blueprint)
	{
		return MakeShareable(new FCanvasSlotCustomization(Blueprint));
	}

	FCanvasSlotCustomization(UBlueprint* InBlueprint)
		: Blueprint(CastChecked<UWidgetBlueprint>(InBlueprint))
	{
	}
	
	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;
	
private:
	
	FReply OnAnchorClicked(TSharedRef<IPropertyHandle> AnchorsHandle, FAnchors Anchors);

private:
	UWidgetBlueprint* Blueprint;
	
};
