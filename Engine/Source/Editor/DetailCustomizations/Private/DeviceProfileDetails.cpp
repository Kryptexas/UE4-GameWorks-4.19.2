// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldSettingsDetails.cpp: Implements the FWorldSettingsDetails class.
=============================================================================*/

#include "DetailCustomizationsPrivatePCH.h"
#include "DeviceProfileDetails.h"


/* IDetailCustomization overrides
 *****************************************************************************/

void FDeviceProfileDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("DeviceSettings");

	TSharedPtr<IPropertyHandle> DeviceTypeHandle = DetailBuilder.GetProperty("DeviceType");
	DetailBuilder.HideProperty(DeviceTypeHandle);
	
	TSharedPtr<IPropertyHandle> BaseProfileNameHandle = DetailBuilder.GetProperty("BaseProfileName");
	DetailBuilder.HideProperty(BaseProfileNameHandle);
	
	TSharedPtr<IPropertyHandle> MeshLODSettingsHandle = DetailBuilder.GetProperty("MeshLODSettings");
	DetailBuilder.HideProperty(MeshLODSettingsHandle);
	
	TSharedPtr<IPropertyHandle> TextureLODSettingsHandle = DetailBuilder.GetProperty("TextureLODSettings");
	DetailBuilder.HideProperty(TextureLODSettingsHandle);
}