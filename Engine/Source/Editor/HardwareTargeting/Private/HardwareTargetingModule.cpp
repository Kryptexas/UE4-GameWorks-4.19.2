// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HardwareTargetingPrivatePCH.h"
#include "HardwareTargetingModule.h"
#include "HardwareTargetingSettings.h"
#include "Settings.h"
#include "Internationalization.h"
#include "SDecoratedEnumCombo.h"

#define LOCTEXT_NAMESPACE "HardwareTargeting"

class FHardwareTargetingModule : public IHardwareTargetingModule
{
	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End of IModuleInterface interface
	
	/** Apply the current hardware targeting settings if they have changed */
	virtual void ApplyHardwareTargetingSettings();

	/** Make a new combo box for choosing a hardware class target */
	virtual TSharedRef<SWidget> MakeHardwareClassTargetCombo(FOnHardwareClassChanged OnChanged, TAttribute<EHardwareClass::Type> SelectedEnum) override;

	/** Make a new combo box for choosing a graphics preference */
	virtual TSharedRef<SWidget> MakeGraphicsPresetTargetCombo(FOnGraphicsPresetChanged OnChanged, TAttribute<EGraphicsPreset::Type> SelectedEnum) override;
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
	
void FHardwareTargetingModule::ApplyHardwareTargetingSettings()
{
	UHardwareTargetingSettings* Settings = GetMutableDefault<UHardwareTargetingSettings>();

	int32 EnumValue = 0;
	bool bFoundKey = GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedTargetedHardwareClass"), EnumValue, GEditorIni);
	if (!bFoundKey || EHardwareClass::Type(EnumValue) != Settings->TargetedHardwareClass)
	{
		// @todo: Set up project to work with Settings->TargetedHardwareClass

		GConfig->SetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedTargetedHardwareClass"), Settings->TargetedHardwareClass, GEditorIni);
	}

	bFoundKey = GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedDefaultGraphicsPerformance"), EnumValue, GEditorIni);
	if (!bFoundKey || EGraphicsPreset::Type(EnumValue) != Settings->DefaultGraphicsPerformance)
	{
		// @todo: Set up project to work with Settings->DefaultGraphicsPerformance

		GConfig->SetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedDefaultGraphicsPerformance"), Settings->DefaultGraphicsPerformance, GEditorIni);
	}

	GConfig->Flush(false, GEditorIni);
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

IMPLEMENT_MODULE( FHardwareTargetingModule, HardwareTarget );
	
IHardwareTargetingModule& IHardwareTargetingModule::Get()
{
	static FHardwareTargetingModule Instance;
	return Instance;
}

UHardwareTargetingSettings::UHardwareTargetingSettings( const FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, TargetedHardwareClass(EHardwareClass::Desktop)
	, DefaultGraphicsPerformance(EGraphicsPreset::Maximum)
{ }
	
bool UHardwareTargetingSettings::HasPendingChanges() const
{
	UHardwareTargetingSettings* Settings = GetMutableDefault<UHardwareTargetingSettings>();

	int32 EnumValue = 0;
	if (!GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedTargetedHardwareClass"), EnumValue, GEditorIni))
	{
		return true;
	}

	if (EHardwareClass::Type(EnumValue) != Settings->TargetedHardwareClass)
	{
		return true;
	}

	if (!GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"),TEXT("AppliedDefaultGraphicsPerformance"), EnumValue, GEditorIni))
	{
		return true;
	}

	if (EGraphicsPreset::Type(EnumValue) != Settings->DefaultGraphicsPerformance)
	{
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE