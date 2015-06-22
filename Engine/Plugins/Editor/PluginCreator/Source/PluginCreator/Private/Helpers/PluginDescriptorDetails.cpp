#include "PluginCreatorPrivatePCH.h"
#include "PluginDescriptorDetails.h"
#include "PluginDescriptorObject.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"

#define LOCTEXT_NAMESPACE "PluginDescriptorDetails"

TSharedRef<IDetailCustomization> FPluginDescriptorDetails::MakeInstance()
{
	return MakeShareable(new FPluginDescriptorDetails);
}

void FPluginDescriptorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TSharedPtr<IPropertyHandle> IsEnginePluginProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPluginDescriptorObject, bIsEnginePlugin));

	// Hide bIsEnginePlugin flag if this is a Launcher build
	// User should be able to create Game plugins only if thats the case
	if (FApp::IsEngineInstalled())
	{
		IsEnginePluginProperty->MarkHiddenByCustomization();
	}
}

#undef LOCTEXT_NAMESPACE