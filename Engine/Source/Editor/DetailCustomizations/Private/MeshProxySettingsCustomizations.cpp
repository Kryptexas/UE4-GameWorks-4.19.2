// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MeshProxySettingsCustomizations.h"
#include "Modules/ModuleManager.h"
#include "GameFramework/WorldSettings.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "MeshUtilities.h"
#include "IMeshReductionManagerModule.h"

#define LOCTEXT_NAMESPACE "MeshProxySettingsCustomizations"

void FMeshProxySettingsCustomizations::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}


TSharedRef<IPropertyTypeCustomization> FMeshProxySettingsCustomizations::MakeInstance()
{
	return MakeShareable(new FMeshProxySettingsCustomizations);
}


bool FMeshProxySettingsCustomizations::UseNativeProxyLODTool() const
{
	IMeshMerging* MergeModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface").GetMeshMergingInterface();
	return MergeModule && MergeModule->GetName().Equals("ProxyLODMeshMerging");
}

void FMeshProxySettingsCustomizations::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Retrieve structure's child properties
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);
	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// Determine if we are using our native module  If so, we will supress some of the options used by the current thirdparty tool (simplygon).

	IMeshReductionManagerModule& ModuleManager = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	IMeshMerging* MergeModule = ModuleManager.GetMeshMergingInterface();

	IDetailGroup& MaterialSettingsGroup = ChildBuilder.AddGroup(NAME_None, FText::FromString("Material Settings"));



	TSharedPtr< IPropertyHandle > HardAngleThresholdPropertyHandle        = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, HardAngleThreshold));
	TSharedPtr< IPropertyHandle > RecalculateNormalsPropertyHandle        = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, bRecalculateNormals));
	TSharedPtr< IPropertyHandle > UseLandscapeCullingPropertyHandle       = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, bUseLandscapeCulling));
	TSharedPtr< IPropertyHandle > LandscapeCullingPrecisionPropertyHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, LandscapeCullingPrecision));
	TSharedPtr< IPropertyHandle > MergeDistanceHandle                     = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, MergeDistance));
	TSharedPtr< IPropertyHandle > VoxelSizeHandle                         = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, VoxelSize));
	


	for (auto Iter(PropertyHandles.CreateConstIterator()); Iter; ++Iter)
	{
		// Handle special property cases (done inside the loop to maintain order according to the struct
		if (Iter.Value() == HardAngleThresholdPropertyHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsHardAngleThresholdVisible));
		}
		else if (Iter.Value() == RecalculateNormalsPropertyHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsRecalculateNormalsVisible));
		}
		else if (Iter.Value() == UseLandscapeCullingPropertyHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsUseLandscapeCullingVisible));
		}
		else if (Iter.Value() == LandscapeCullingPrecisionPropertyHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsUseLandscapeCullingPrecisionVisible));
		}
		else if (Iter.Value() == MergeDistanceHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsMergeDistanceVisible));
		}
		else if (Iter.Value() == VoxelSizeHandle)
		{
			IDetailPropertyRow& MeshProxySettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
			MeshProxySettingsRow.Visibility(TAttribute<EVisibility>(this, &FMeshProxySettingsCustomizations::IsVoxelSizeVisible));
		}
		else
		{
			IDetailPropertyRow& SettingsRow = MaterialSettingsGroup.AddPropertyRow(Iter.Value().ToSharedRef());
		}
	}
}

EVisibility FMeshProxySettingsCustomizations::IsThirdPartySpecificVisible() const
{
	// Static assignment.  The tool can only change during editor restart.
	static bool bUseNativeTool = UseNativeProxyLODTool();

	if (bUseNativeTool)
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}

EVisibility FMeshProxySettingsCustomizations::IsHardAngleThresholdVisible() const
{
	return IsThirdPartySpecificVisible();
}

EVisibility FMeshProxySettingsCustomizations::IsRecalculateNormalsVisible() const
{
	return IsThirdPartySpecificVisible();
}

EVisibility FMeshProxySettingsCustomizations::IsUseLandscapeCullingVisible() const
{
	return IsThirdPartySpecificVisible();
}

EVisibility FMeshProxySettingsCustomizations::IsUseLandscapeCullingPrecisionVisible() const
{
	return IsThirdPartySpecificVisible();
}

EVisibility FMeshProxySettingsCustomizations::IsMergeDistanceVisible() const
{
	return IsThirdPartySpecificVisible();
}

EVisibility FMeshProxySettingsCustomizations::IsVoxelSizeVisible() const
{
	if (EVisibility::Visible == IsThirdPartySpecificVisible())
	{
		return EVisibility::Hidden;
	}
	else
	{
		return EVisibility::Visible;
	}
}



#undef LOCTEXT_NAMESPACE