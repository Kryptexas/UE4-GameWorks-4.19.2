// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"

//////////////////////////////////////////////////////////////////////////
// FSpritePolygonCollectionCustomization

class FSpritePolygonCollectionCustomization : public IPropertyTypeCustomization
{
public:
	// Makes a new instance of this customization
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	// End of IPropertyTypeCustomization interface

protected:
	EVisibility PolygonModeMatches(TSharedPtr<IPropertyHandle> Property, ESpritePolygonMode::Type DesiredMode) const;
};
