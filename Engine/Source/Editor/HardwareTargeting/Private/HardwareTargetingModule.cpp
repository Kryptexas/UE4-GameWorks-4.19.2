// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HardwareTargetingPrivatePCH.h"
#include "HardwareTargetingModule.h"
#include "HardwareTargetingSettings.h"
#include "Settings.h"
#include "Internationalization.h"
#include "SDecoratedEnumCombo.h"

#include "Runtime/Engine/Classes/Engine/RendererSettings.h"

#define LOCTEXT_NAMESPACE "HardwareTargeting"

//////////////////////////////////////////////////////////////////////////
// FMetaSettingGatherer

struct FMetaSettingGatherer
{
	TSet<UObject*> DirtyObjects;
	FTextBuilder DescriptionBuffer;

	// Are we just displaying what would change, or actually changing things?
	bool bReadOnly;

	FMetaSettingGatherer()
		: bReadOnly(false)
	{
	}

	void AddEntry(UObject* SettingsObject, UProperty* Property, FText NewValue, bool bModified)
	{
		if (bModified && !bReadOnly)
		{
			DirtyObjects.Add(SettingsObject);

			FPropertyChangedEvent ChangeEvent(Property, false, EPropertyChangeType::ValueSet);
			SettingsObject->PostEditChangeProperty(ChangeEvent);
		}

		if (bReadOnly)
		{
			FText SettingDisplayName = Property->GetDisplayNameText();

			FFormatNamedArguments Args;
			Args.Add(TEXT("SettingName"), SettingDisplayName);
			Args.Add(TEXT("SettingValue"), NewValue);

			FText FormatString = bModified ? LOCTEXT("MetaSettingDisplayStringUnmodified", "{SettingName} is {SettingValue} <HardwareTargets.Strong>(modified)</>") : LOCTEXT("MetaSettingDisplayStringUnmodified", "{SettingName} is {SettingValue}");

			DescriptionBuffer.AppendLine(FText::Format(FormatString, Args));
		}
	}

	template <typename ValueType>
	static FText ValueToString(ValueType Value);

	void Finalize()
	{
		check(!bReadOnly);
		for (UObject* SettingsObject : DirtyObjects)
		{
			SettingsObject->UpdateDefaultConfigFile();
		}
	}

	void StartSection(FText SectionHeading)
	{
		if (bReadOnly)
		{
			DescriptionBuffer.AppendLine(SectionHeading);
			DescriptionBuffer.Indent();
		}
	}

	void EndSection()
	{
		if (bReadOnly)
		{
			DescriptionBuffer.Unindent();
		}
	}
};


template <>
FText FMetaSettingGatherer::ValueToString(bool Value)
{
	return Value ? LOCTEXT("BoolEnabled", "enabled") : LOCTEXT("BoolDisabled", "disabled");
}

template <>
FText FMetaSettingGatherer::ValueToString(EAntiAliasingMethodUI::Type Value)
{
	switch (Value)
	{
	case EAntiAliasingMethodUI::AAM_None:
		return LOCTEXT("AA_None", "None");
	case EAntiAliasingMethodUI::AAM_FXAA:
		return LOCTEXT("AA_FXAA", "FXAA");
	case EAntiAliasingMethodUI::AAM_TemporalAA:
		return LOCTEXT("AA_TemporalAA", "Temporal AA");
	default:
		return FText::AsNumber((int32)Value);
	}
}


#define UE_META_SETTING_ENTRY(Builder, Class, PropertyName, TargetValue) \
{ \
	Class* SettingsObject = GetMutableDefault<Class>(); \
	bool bModified = SettingsObject->PropertyName != (TargetValue); \
	if (!Builder.bReadOnly) { SettingsObject->PropertyName = (TargetValue); } \
	Builder.AddEntry(SettingsObject, FindFieldChecked<UProperty>(Class::StaticClass(), GET_MEMBER_NAME_CHECKED(Class, PropertyName)), FMetaSettingGatherer::ValueToString(TargetValue), bModified); \
}

//////////////////////////////////////////////////////////////////////////
// FHardwareTargetingModule

class FHardwareTargetingModule : public IHardwareTargetingModule
{
public:
	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End of IModuleInterface interface
	
	// IHardwareTargetingModule interface
	virtual void ApplyHardwareTargetingSettings() override;
	virtual FText QueryReadableDescriptionOfHardwareTargetingSettings() override;
	virtual TSharedRef<SWidget> MakeHardwareClassTargetCombo(FOnHardwareClassChanged OnChanged, TAttribute<EHardwareClass::Type> SelectedEnum) override;
	virtual TSharedRef<SWidget> MakeGraphicsPresetTargetCombo(FOnGraphicsPresetChanged OnChanged, TAttribute<EGraphicsPreset::Type> SelectedEnum) override;
	// End of IHardwareTargetingModule interface

private:
	void GatherSettings(FMetaSettingGatherer& Builder);
};

void FHardwareTargetingModule::StartupModule()
{
	ISettingsModule& SettingsModule = *ISettingsModule::Get();
		
	SettingsModule.RegisterSettings("Project", "Project", "HardwareTargeting",
		LOCTEXT("HardwareTargetingSettingsName", "Hardware Targeting"),
		LOCTEXT("HardwareTargetingSettingsDescription", "Options for choosing which class of hardware to target"),
		GetMutableDefault<UHardwareTargetingSettings>()
	);

	// Apply any settings on startup if necessary
	ApplyHardwareTargetingSettings();
}

void FHardwareTargetingModule::ShutdownModule()
{
}

FText FHardwareTargetingModule::QueryReadableDescriptionOfHardwareTargetingSettings()
{
	// Gather and stringify the modified settings
	FMetaSettingGatherer Gatherer;
	Gatherer.bReadOnly = true;
	GatherSettings(Gatherer);

	return Gatherer.DescriptionBuffer.ToText();
}

	
void FHardwareTargetingModule::GatherSettings(FMetaSettingGatherer& Builder)
{
	UHardwareTargetingSettings* Settings = GetMutableDefault<UHardwareTargetingSettings>();

	const bool bLowEndMobile = (Settings->TargetedHardwareClass == EHardwareClass::Mobile) && (Settings->DefaultGraphicsPerformance == EGraphicsPreset::Scalable);
	const bool bAnyMobile = (Settings->TargetedHardwareClass == EHardwareClass::Mobile);
	const bool bHighEndMobile = (Settings->TargetedHardwareClass == EHardwareClass::Mobile) && (Settings->DefaultGraphicsPerformance == EGraphicsPreset::Maximum);
	const bool bAnyPC = (Settings->TargetedHardwareClass == EHardwareClass::Desktop);
	const bool bHighEndPC = (Settings->TargetedHardwareClass == EHardwareClass::Desktop) && (Settings->DefaultGraphicsPerformance == EGraphicsPreset::Maximum);

	{
		Builder.StartSection(LOCTEXT("RenderingHeading", "<HardwareTargets.H1>Default Rendering Settings</>"));

		// Based roughly on https://docs.unrealengine.com/latest/INT/Platforms/Mobile/PostProcessEffects/index.html
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bMobileHDR, !bLowEndMobile);

		// Bloom works and isn't terribly expensive on anything beyond low-end
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureBloom, !bLowEndMobile);

		// Separate translucency does nothing in the ES2 renderer
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bSeparateTranslucency, !bAnyMobile);

		// Motion blur, lens flare, auto-exposure, and ambient occlusion don't work in the ES2 renderer
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureMotionBlur, bHighEndPC);
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureLensFlare, bAnyPC);
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureAutoExposure, bHighEndPC);
		UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureAmbientOcclusion, bAnyPC);

		// DOF and AA work on mobile but are expensive, keeping them off by default
		//@TODO: DOF setting doesn't exist yet
		// UE_META_SETTING_ENTRY(Builder, URendererSettings, bDefaultFeatureDepthOfField, bHighEndPC);
		UE_META_SETTING_ENTRY(Builder, URendererSettings, DefaultFeatureAntiAliasing, bHighEndPC ? EAntiAliasingMethodUI::AAM_TemporalAA : EAntiAliasingMethodUI::AAM_None);

		Builder.EndSection();
	}

	{
		Builder.StartSection(LOCTEXT("InputHeading", "<HardwareTargets.H1>Default Input Settings</>"));

		// Mobile uses touch
		UE_META_SETTING_ENTRY(Builder, UInputSettings, bUseMouseForTouch, bAnyMobile);
		//@TODO: Use bAlwaysShowTouchInterface (sorta implied by bUseMouseForTouch)?

		Builder.EndSection();
	}

	{
		Builder.StartSection(LOCTEXT("GameHeading", "<HardwareTargets.H1>Default Game Settings</>"));
		
		// Tablets or phones are usually shared-screen multiplayer instead of split-screen
		UE_META_SETTING_ENTRY(Builder, UGameMapsSettings, bUseSplitscreen, bAnyPC);

		Builder.EndSection();
	}
}

void FHardwareTargetingModule::ApplyHardwareTargetingSettings()
{
	UHardwareTargetingSettings* Settings = GetMutableDefault<UHardwareTargetingSettings>();

	// Apply the settings if they've changed
	if (Settings->HasPendingChanges())
	{
		// Gather and apply the modified settings
		FMetaSettingGatherer Builder;
		Builder.bReadOnly = false;
		GatherSettings(Builder);
		Builder.Finalize();

		// Write out the 'did we apply' values
		GConfig->SetInt(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedDefaultGraphicsPerformance"), Settings->DefaultGraphicsPerformance, GEditorIni);
		GConfig->SetInt(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedTargetedHardwareClass"), Settings->TargetedHardwareClass, GEditorIni);
		GConfig->Flush(false, GEditorIni);
	}
}

TSharedRef<SWidget> FHardwareTargetingModule::MakeHardwareClassTargetCombo(FOnHardwareClassChanged OnChanged, TAttribute<EHardwareClass::Type> SelectedEnum)
{
	TArray<SDecoratedEnumCombo<EHardwareClass::Type>::FComboOption> HardwareClassInfo;
	HardwareClassInfo.Add(SDecoratedEnumCombo<EHardwareClass::Type>::FComboOption(
		EHardwareClass::Desktop, FSlateIcon(FEditorStyle::GetStyleSetName(), "HardwareTargeting.DesktopPlatform"), LOCTEXT("DesktopCaption", "Desktop / Console")));
	HardwareClassInfo.Add(SDecoratedEnumCombo<EHardwareClass::Type>::FComboOption(
		EHardwareClass::Mobile, FSlateIcon(FEditorStyle::GetStyleSetName(), "HardwareTargeting.MobilePlatform"), LOCTEXT("MobileCaption", "Mobile / Tablet")));

	return SNew(SDecoratedEnumCombo<EHardwareClass::Type>, MoveTemp(HardwareClassInfo))
		.SelectedEnum(SelectedEnum)
		.OnEnumChanged(OnChanged);
}

TSharedRef<SWidget> FHardwareTargetingModule::MakeGraphicsPresetTargetCombo(FOnGraphicsPresetChanged OnChanged, TAttribute<EGraphicsPreset::Type> SelectedEnum)
{
	TArray<SDecoratedEnumCombo<EGraphicsPreset::Type>::FComboOption> GraphicsPresetInfo;
	GraphicsPresetInfo.Add(SDecoratedEnumCombo<EGraphicsPreset::Type>::FComboOption(
		EGraphicsPreset::Maximum, FSlateIcon(FEditorStyle::GetStyleSetName(), "HardwareTargeting.MaximumQuality"), LOCTEXT("MaximumCaption", "Maximum Quality")));
	GraphicsPresetInfo.Add(SDecoratedEnumCombo<EGraphicsPreset::Type>::FComboOption(
		EGraphicsPreset::Scalable, FSlateIcon(FEditorStyle::GetStyleSetName(), "HardwareTargeting.ScalableQuality"), LOCTEXT("ScalableCaption", "Scalable 3D or 2D")));

	return SNew(SDecoratedEnumCombo<EGraphicsPreset::Type>, MoveTemp(GraphicsPresetInfo))
		.SelectedEnum(SelectedEnum)
		.OnEnumChanged(OnChanged);
}

IHardwareTargetingModule& IHardwareTargetingModule::Get()
{
	static FHardwareTargetingModule Instance;
	return Instance;
}

IMPLEMENT_MODULE(FHardwareTargetingModule, HardwareTarget);

#undef UE_META_SETTING_ENTRY
#undef LOCTEXT_NAMESPACE