// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvQueryTest_TraceDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

protected:

	TSharedPtr<IPropertyHandle> ModeHandle;
	
	void OnModeChanged();
	EVisibility GetExtentXVisibility() const;
	EVisibility GetExtentYVisibility() const;
	EVisibility GetExtentZVisibility() const;

	uint32 bShowExtentX : 1;
	uint32 bShowExtentY : 1;
	uint32 bShowExtentZ : 1;
};
