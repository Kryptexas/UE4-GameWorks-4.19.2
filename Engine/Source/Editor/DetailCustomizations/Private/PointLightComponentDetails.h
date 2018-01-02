// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

class FPointLightComponentDetails : public IDetailCustomization
{
public:
	FPointLightComponentDetails() {}

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;
	virtual void CustomizeDetails( const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder ) override;

protected:

	// The detail builder for this cusomtomisation
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;

	/** Called when the intensity units are changed */
	void OnIntensityUnitsChanged();
};
