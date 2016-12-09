// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MathStructCustomizations.h"
#include "IDetailCustomNodeBuilder.h"
#include "ColorGradingVectorCustomization.h"

/**
 * Customizes FVector4 structs.
 */
class FVector4StructCustomization
	: public FMathStructCustomization
{
public:
	FVector4StructCustomization();
	~FVector4StructCustomization();
	/** @return A new instance of this class */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//////////////////////////////////////////////////////////////////////////
	/** FMathStructCustomization interface */

	virtual void MakeHeaderRow(TSharedRef<IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	// We specialize the detail display of color grading vector property. The color grading mode is specified inside the metadata of the UProperty
	TSharedPtr<FColorGradingVectorCustomization> ColorGradingVectorCustomization;
};

