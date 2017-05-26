// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AttenuationSettingsCustomizations.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "Engine/Attenuation.h"
#include "Audio.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "AudioDevice.h"
#include "Classes/Sound/AudioSettings.h"

TSharedRef<IPropertyTypeCustomization> FSoundAttenuationSettingsCustomization::MakeInstance() 
{
	return MakeShareable( new FSoundAttenuationSettingsCustomization );
}

TSharedRef<IPropertyTypeCustomization> FForceFeedbackAttenuationSettingsCustomization::MakeInstance() 
{
	return MakeShareable( new FForceFeedbackAttenuationSettingsCustomization );
}

void FBaseAttenuationSettingsCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	// We'll set up reset to default ourselves
	const bool bDisplayResetToDefault = false;
	const FText DisplayNameOverride = FText::GetEmpty();
	const FText DisplayToolTipOverride = FText::GetEmpty();

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget( DisplayNameOverride, DisplayToolTipOverride, bDisplayResetToDefault )
		];
}

void FBaseAttenuationSettingsCustomization::CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	AttenuationShapeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, AttenuationShape));
	DistanceAlgorithmHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, DistanceAlgorithm));

	TSharedRef<IPropertyHandle> AttenuationExtentsHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, AttenuationShapeExtents)).ToSharedRef();

	uint32 NumExtentChildren;
	AttenuationExtentsHandle->GetNumChildren( NumExtentChildren );

	TSharedPtr< IPropertyHandle > ExtentXHandle;
	TSharedPtr< IPropertyHandle > ExtentYHandle;
	TSharedPtr< IPropertyHandle > ExtentZHandle;

	for( uint32 ExtentChildIndex = 0; ExtentChildIndex < NumExtentChildren; ++ExtentChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = AttenuationExtentsHandle->GetChildHandle( ExtentChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, X))
		{
			ExtentXHandle = ChildHandle;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Y))
		{
			ExtentYHandle = ChildHandle;
		}
		else
		{
			check(PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Z));
			ExtentZHandle = ChildHandle;
		}
	}

	ChildBuilder.AddChildProperty(DistanceAlgorithmHandle.ToSharedRef() );

	IDetailPropertyRow& CustomCurveRow = ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, CustomAttenuationCurve)).ToSharedRef());
	CustomCurveRow.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsCustomCurveSelected));

	IDetailPropertyRow& dbAttenuationRow = ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, dBAttenuationAtMax)).ToSharedRef());
	dbAttenuationRow.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsNaturalSoundSelected));

	IDetailPropertyRow& AttenuationShapeRow = ChildBuilder.AddChildProperty( AttenuationShapeHandle.ToSharedRef() );
	
	ChildBuilder.AddChildProperty(AttenuationExtentsHandle)
		.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsBoxSelected))
		.DisplayName(NSLOCTEXT("AttenuationSettings", "BoxExtentsLabel", "Extents"))
		.ToolTip(NSLOCTEXT("AttenuationSettings", "BoxExtents", "The dimensions of the of the box."));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "RadiusLabel", "Radius"))
		.NameContent()
		[
			SNew(STextBlock)
				.Text(NSLOCTEXT("AttenuationSettings", "RadiusLabel", "Radius"))
				.ToolTipText(NSLOCTEXT("AttenuationSettings", "RadiusToolTip", "The distance from the location of the sound at which falloff begins."))
				.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsSphereSelected));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "CapsuleHalfHeightLabel", "Capsule Half Height"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("AttenuationSettings", "CapsuleHalfHeightLabel", "Capsule Half Height"))
				.ToolTipText(NSLOCTEXT("AttenuationSettings", "CapsuleHalfHeightToolTip", "The attenuation capsule's half height."))
				.Font(StructCustomizationUtils.GetRegularFont())
			]
		.ValueContent()
			[
				ExtentXHandle->CreatePropertyValueWidget()
			]
		.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsCapsuleSelected));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "CapsuleRadiusLabel", "Capsule Radius"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "CapsuleRadiusLabel", "Capsule Radius"))
			.ToolTipText(NSLOCTEXT("AttenuationSettings", "CapsuleRadiusToolTip", "The attenuation capsule's radius."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentYHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsCapsuleSelected));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "ConeRadiusLabel", "Cone Radius"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeRadiusLabel", "Cone Radius"))
			.ToolTipText(NSLOCTEXT("AttenuationSettings", "ConeRadiusToolTip", "The attenuation cone's radius."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "ConeAngleLabel", "Cone Angle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeAngleLabel", "Cone Angle"))
			.ToolTipText(NSLOCTEXT("AttenuationSettings", "ConeAngleToolTip", "The angle of the inner edge of the attenuation cone's falloff. Inside this angle sounds will be at full volume."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentYHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildContent(NSLOCTEXT("AttenuationSettings", "ConeFalloffAngleLabel", "Cone Falloff Angle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AttenuationSettings", "ConeFalloffAngleLabel", "Cone Falloff Angle"))
			.ToolTipText(NSLOCTEXT("AttenuationSettings", "ConeFalloffAngleToolTip", "The angle of the outer edge of the attenuation cone's falloff. Outside this angle sounds will be inaudible."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
	.ValueContent()
		[
			ExtentZHandle->CreatePropertyValueWidget()
		]
	.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsConeSelected));

	IDetailPropertyRow& ConeOffsetRow = ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, ConeOffset)).ToSharedRef());
	ConeOffsetRow.Visibility(TAttribute<EVisibility>(this, &FBaseAttenuationSettingsCustomization::IsConeSelected));

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FBaseAttenuationSettings, FalloffDistance)).ToSharedRef());
}

void FSoundAttenuationSettingsCustomization::CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	bIsFocusedHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bEnableListenerFocus)).ToSharedRef();
	bIsOcclusionEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bEnableOcclusion)).ToSharedRef();
	bIsSpatializedHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bSpatialize)).ToSharedRef();

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bAttenuate)).ToSharedRef());
	ChildBuilder.AddChildProperty(bIsSpatializedHandle.ToSharedRef());

	// Check to see if a spatialization plugin is enabled
	if (IsAudioPluginEnabled(EAudioPlugin::SPATIALIZATION))
	{
		TSharedPtr<IPropertyHandle> SpatializationAlgorithmHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, SpatializationAlgorithm));
		ChildBuilder.AddChildProperty(SpatializationAlgorithmHandle.ToSharedRef());
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, SpatializationPluginSettings)).ToSharedRef());
	}

	FBaseAttenuationSettingsCustomization::CustomizeChildren(StructPropertyHandle, ChildBuilder, StructCustomizationUtils);
	
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OmniRadius)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, StereoSpread)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bAttenuateWithLPF)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, LPFRadiusMin)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, LPFRadiusMax)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, LPFFrequencyAtMin)).ToSharedRef());
	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, LPFFrequencyAtMax)).ToSharedRef());

	if (GetDefault<UAudioSettings>()->IsAudioMixerEnabled())
	{
		if (IsAudioPluginEnabled(EAudioPlugin::REVERB))
		{
			ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, ReverbPluginSettings)).ToSharedRef());
		}

		// The reverb wet-level mapping is an audio mixer-only feature
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, ReverbWetLevelMin)).ToSharedRef());
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, ReverbWetLevelMax)).ToSharedRef());
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, ReverbDistanceMin)).ToSharedRef());
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, ReverbDistanceMax)).ToSharedRef());
	}

	ChildBuilder.AddChildProperty(bIsFocusedHandle.ToSharedRef());

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, FocusAzimuth)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, NonFocusAzimuth)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, FocusDistanceScale)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, NonFocusDistanceScale)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, FocusPriorityScale)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, NonFocusPriorityScale)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, FocusVolumeAttenuation)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, NonFocusVolumeAttenuation)).ToSharedRef())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(bIsOcclusionEnabledHandle.ToSharedRef());

	// Hide the occlusion plugin settings slot if there's no occlusion plugin loaded.
	// Don't show the built-in occlusion settings if we're using 
	if (GetDefault<UAudioSettings>()->IsAudioMixerEnabled() && IsAudioPluginEnabled(EAudioPlugin::OCCLUSION))
	{
		ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OcclusionPluginSettings)).ToSharedRef())
			.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);
	}

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OcclusionTraceChannel)).ToSharedRef())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OcclusionLowPassFilterFrequency)).ToSharedRef())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OcclusionVolumeAttenuation)).ToSharedRef())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, OcclusionInterpolationTime)).ToSharedRef())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	ChildBuilder.AddChildProperty(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FSoundAttenuationSettings, bUseComplexCollisionForOcclusion)).ToSharedRef())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	if (PropertyHandles.Num() != 39)
	{
		FString PropertyList;
		for (auto It(PropertyHandles.CreateConstIterator()); It; ++It)
		{
			PropertyList += It.Key().ToString() + TEXT(", ");
		}
		ensureMsgf(false, TEXT("Unexpected property handle(s) customizing FSoundAttenuationSettings: %s"), *PropertyList);
	}
}

void FForceFeedbackAttenuationSettingsCustomization::CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	FBaseAttenuationSettingsCustomization::CustomizeChildren(StructPropertyHandle, ChildBuilder, StructCustomizationUtils);

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	TSharedPtr<FPropertyRestriction> EnumRestriction = MakeShareable(new FPropertyRestriction(NSLOCTEXT("AttenuationSettings", "NoNaturalSound", "Natural Sound is only available for Sound Attenuation")));
	const UEnum* const AttenuationDistanceModelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAttenuationDistanceModel"));		
	EnumRestriction->AddHiddenValue(AttenuationDistanceModelEnum->GetNameStringByValue((uint8)EAttenuationDistanceModel::NaturalSound));
	DistanceAlgorithmHandle->AddRestriction(EnumRestriction.ToSharedRef());

	if (PropertyHandles.Num() != 7)
	{
		FString PropertyList;
		for (auto It(PropertyHandles.CreateConstIterator()); It; ++It)
		{
			PropertyList += It.Key().ToString() + TEXT(", ");
		}
		ensureMsgf(false, TEXT("Unexpected property handle(s) customizing FForceFeedbackAttenuationSettings: %s"), *PropertyList);
	}

}

bool FSoundAttenuationSettingsCustomization::IsFocusedEnabled() const
{
	bool bIsFocusEnabled;
	bIsFocusedHandle->GetValue(bIsFocusEnabled);
	if (!bIsFocusEnabled)
	{
		return false;
	}

	bool bIsSpatialized;
	bIsSpatializedHandle->GetValue(bIsSpatialized);

	return bIsSpatialized;
}

TAttribute<bool> FSoundAttenuationSettingsCustomization::GetIsFocusEnabledAttribute() const
{
	return TAttribute<bool>(this, &FSoundAttenuationSettingsCustomization::IsFocusedEnabled);
}

bool FSoundAttenuationSettingsCustomization::IsOcclusionEnabled() const
{
	bool bIsOcclusionEnabled;
	bIsOcclusionEnabledHandle->GetValue(bIsOcclusionEnabled);
	if (!bIsOcclusionEnabled)
	{
		return false;
	}

	bool bIsSpatialized;
	bIsSpatializedHandle->GetValue(bIsSpatialized);

	return bIsSpatialized;
}

TAttribute<bool> FSoundAttenuationSettingsCustomization::GetIsOcclusionEnabledAttribute() const
{
	return TAttribute<bool>(this, &FSoundAttenuationSettingsCustomization::IsOcclusionEnabled);
}

EVisibility FBaseAttenuationSettingsCustomization::IsConeSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Cone ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FBaseAttenuationSettingsCustomization::IsSphereSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Sphere ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FBaseAttenuationSettingsCustomization::IsBoxSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Box ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FBaseAttenuationSettingsCustomization::IsCapsuleSelected() const
{
	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAttenuationShape::Type AttenuationShape = (EAttenuationShape::Type)AttenuationShapeValue;

	return (AttenuationShape == EAttenuationShape::Capsule ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FBaseAttenuationSettingsCustomization::IsNaturalSoundSelected() const
{
	uint8 DistanceAlgorithmValue;
	DistanceAlgorithmHandle->GetValue(DistanceAlgorithmValue);

	const EAttenuationDistanceModel DistanceAlgorithm = (EAttenuationDistanceModel)DistanceAlgorithmValue;

	return (DistanceAlgorithm == EAttenuationDistanceModel::NaturalSound ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FBaseAttenuationSettingsCustomization::IsCustomCurveSelected() const
{
	uint8 DistanceAlgorithmValue;
	DistanceAlgorithmHandle->GetValue(DistanceAlgorithmValue);

	const EAttenuationDistanceModel DistanceAlgorithm = (EAttenuationDistanceModel)DistanceAlgorithmValue;

	return (DistanceAlgorithm == EAttenuationDistanceModel::Custom ? EVisibility::Visible : EVisibility::Hidden);
}
