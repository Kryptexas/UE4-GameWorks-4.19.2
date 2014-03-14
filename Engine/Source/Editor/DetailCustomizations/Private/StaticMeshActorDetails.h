// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FStaticMeshActorDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
private:
	/** Called when the set collision from builder brush command should be executed */
	FReply OnSetCollisionFromBuilder();
};