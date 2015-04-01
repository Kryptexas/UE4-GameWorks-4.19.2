// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "ILocalizationDashboardModule.h"
#include "LocalizationDashboard.h"
#include "LocalizationTargetTypes.h"
#include "SDockTab.h"
#include "Features/IModularFeatures.h"
#include "ILocalizationServiceProvider.h"
#include "LocalizationTargetSetDetailCustomization.h"
#include "LocalizationTargetDetailCustomization.h"

#define LOCTEXT_NAMESPACE "LocalizationDashboard"

class FLocalizationDashboardModule
	: public ILocalizationDashboardModule
{
public:
	// Begin IModuleInterface interface
	virtual void StartupModule() override
	{
		ServiceProviders = IModularFeatures::Get().GetModularFeatureImplementations<ILocalizationServiceProvider>("LocalizationService");
		ServiceProviders.Insert(nullptr, 0); // "None"

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout("LocalizationTargetSet", FOnGetDetailCustomizationInstance::CreateLambda(
			[]() -> TSharedRef<IDetailCustomization>
			{
				return MakeShareable( new FLocalizationTargetSetDetailCustomization());
			})
		);
		PropertyModule.RegisterCustomClassLayout("LocalizationTarget", FOnGetDetailCustomizationInstance::CreateLambda(
			[]() -> TSharedRef<IDetailCustomization>
			{
				return MakeShareable( new FLocalizationTargetDetailCustomization());
			})
		);

		FLocalizationDashboard::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FLocalizationDashboard::Terminate();

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("LocalizationTarget");
		PropertyModule.UnregisterCustomClassLayout("LocalizationTargetSet");
	}
	// End IModuleInterface interface

	virtual void Show() override
	{
		FLocalizationDashboard* const LocalizationDashboard = FLocalizationDashboard::Get();
		if (LocalizationDashboard)
		{
			LocalizationDashboard->Show();
		}
	}

	virtual const TArray<ILocalizationServiceProvider*>& GetLocalizationServiceProviders() const override
	{
		return ServiceProviders;
	}

	virtual ILocalizationServiceProvider* GetCurrentLocalizationServiceProvider() const override
	{
		if(CurrentServiceProviderName == NAME_None)
		{
			return nullptr;
		}

		auto ServiceProviderNameComparator = [this](ILocalizationServiceProvider* const LSP) -> bool
		{
			return LSP ? LSP->GetName() == CurrentServiceProviderName : false;
		};
		ILocalizationServiceProvider* const * LSP = ServiceProviders.FindByPredicate(ServiceProviderNameComparator);
		return LSP ? *LSP : nullptr;
	}

private:
	TArray<ILocalizationServiceProvider*> ServiceProviders;
	FName CurrentServiceProviderName;
};

IMPLEMENT_MODULE(FLocalizationDashboardModule, "LocalizationDashboard");

#undef LOCTEXT_NAMESPACE
