// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MacGraphicsSwitchingModule.h"
#include "MacGraphicsSwitchingSettingsDetails.h"
#include "MacGraphicsSwitchingWidget.h"

#define LOCTEXT_NAMESPACE "MacGraphicsSwitchingSettingsDetails"

TSharedRef<IDetailCustomization> FMacGraphicsSwitchingSettingsDetails::MakeInstance()
{
	return MakeShareable(new FMacGraphicsSwitchingSettingsDetails());
}

void FMacGraphicsSwitchingSettingsDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	TSharedRef<IPropertyHandle> PreferredRendererPropertyHandle = DetailLayout.GetProperty("RendererID");
	DetailLayout.HideProperty("RendererID");

	bool bAllowMultiGPUs = IMacGraphicsSwitchingModule::Get().AllowMultipleGPUs();
	bool bAllowAutomaticGraphicsSwitching = IMacGraphicsSwitchingModule::Get().AllowAutomaticGraphicsSwitching();
	
	TSharedRef<IPropertyHandle> MultiGPUPropertyHandle = DetailLayout.GetProperty("bUseMultipleRenderers");
	if (!bAllowMultiGPUs)
	{
		MultiGPUPropertyHandle->SetValue(false);
		DetailLayout.HideProperty("bUseMultipleRenderers");
	}
	
	TSharedRef<IPropertyHandle> SwitchingPropertyHandle = DetailLayout.GetProperty("bAllowAutomaticGraphicsSwitching");
	if (!bAllowAutomaticGraphicsSwitching)
	{
		SwitchingPropertyHandle->SetValue(false);
		DetailLayout.HideProperty("bAllowAutomaticGraphicsSwitching");
	}
	
	IDetailCategoryBuilder& AccessorCategory = DetailLayout.EditCategory( "OpenGL" );
	AccessorCategory.AddCustomRow( LOCTEXT("PreferredRenderer", "Preferred Renderer").ToString() )
	.NameContent()
	[
		PreferredRendererPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(113)
	.MaxDesiredWidth(113)
	[
		SNew(SMacGraphicsSwitchingWidget)
		.bLiveSwitching(false)
		.PreferredRendererPropertyHandle(PreferredRendererPropertyHandle)
	];
}

#undef LOCTEXT_NAMESPACE