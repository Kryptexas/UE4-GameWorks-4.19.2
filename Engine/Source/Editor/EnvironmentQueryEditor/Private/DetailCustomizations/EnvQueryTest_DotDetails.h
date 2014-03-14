// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvQueryTest_DotDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

protected:

	TSharedPtr<IPropertyHandle> LineAHandle;
	TSharedPtr<IPropertyHandle> LineBHandle;
	
	EVisibility GetLineASegmentVisibility() const;
	EVisibility GetLineADirectionVisibility() const;
	EVisibility GetLineBSegmentVisibility() const;
	EVisibility GetLineBDirectionVisibility() const;

	void OnModeAChanged();
	void OnModeBChanged();

	uint8 bModeASegment : 1;
	uint8 bModeBSegment : 1;
};
