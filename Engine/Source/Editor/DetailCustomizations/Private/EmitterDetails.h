// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FEmitterDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) OVERRIDE;

private:
	/** Handles the Auto-Populate button's on click event */
	FReply OnAutoPopulateClicked();

	/** Handles the Emitter Reset button's on click event */
	FReply OnResetEmitter();
private:
	/** Cached off reference to the layout builder */
	IDetailLayoutBuilder* DetailLayout;
};
