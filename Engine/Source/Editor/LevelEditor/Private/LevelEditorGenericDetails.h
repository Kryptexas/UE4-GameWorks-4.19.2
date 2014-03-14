// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Creates details for the level editor details view that are not specific to any selected actor type
 */
class FLevelEditorGenericDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

private:
	/**
	 * Adds a section for selected surface details
	 */
	void AddSurfaceDetails( class IDetailLayoutBuilder& DetailLayout );
};

