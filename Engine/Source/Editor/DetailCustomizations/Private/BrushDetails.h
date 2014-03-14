// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBrushDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	~FBrushDetails();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

private:
	/** Callback for creating a static mesh from valid selected brushes. */
	FReply OnCreateStaticMesh();

private:
	/** Holds a list of BSP brushes, used for converting to static meshes */
	TArray< TWeakObjectPtr<ABrush> > SelectedBSPBrushes;

	/** Container widget for the geometry mode tools */
	TSharedPtr< SHorizontalBox > GeometryToolsContainer;
};