// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FLandscapeSplineDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

private:
	class FEdModeLandscape* GetEditorMode() const;
	FReply OnSelectConnectedControlPointsButtonClicked();
	FReply OnSelectConnectedSegmentsButtonClicked();
};
