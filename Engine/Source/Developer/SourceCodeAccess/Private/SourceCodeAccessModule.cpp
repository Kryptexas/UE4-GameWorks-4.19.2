// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SourceCodeAccessPrivatePCH.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "SourceCodeAccessModule.h"
#include "SourceCodeAccessSettings.h"
#include "Settings.h"

IMPLEMENT_MODULE( FSourceCodeAccessModule, SourceCodeAccess );

#define LOCTEXT_NAMESPACE "SourceCodeAccessModule"

static FName SourceCodeAccessorFeatureName(TEXT("SourceCodeAccessor"));

FSourceCodeAccessModule::FSourceCodeAccessModule()
	: CurrentSourceCodeAccessor(nullptr)
{
}

void FSourceCodeAccessModule::StartupModule()
{
	GetMutableDefault<USourceCodeAccessSettings>()->LoadConfig();

	// Register to check for source control features
	IModularFeatures::Get().OnModularFeatureRegistered().AddRaw(this, &FSourceCodeAccessModule::HandleModularFeatureRegistered);

	// bind default accessor to editor
	IModularFeatures::Get().RegisterModularFeature(SourceCodeAccessorFeatureName, &DefaultSourceCodeAccessor);

	// Register to display our settings
	ISettingsModule* SettingsModule = ISettingsModule::Get();
	if (SettingsModule != nullptr)
	{
		SettingsModule->RegisterSettings("Editor", "General", "Source Code",
			LOCTEXT("TargetSettingsName", "Source Code"),
			LOCTEXT("TargetSettingsDescription", "Control how the editor accesses source code."),
			GetMutableDefault<USourceCodeAccessSettings>()
		);
	}
}

void FSourceCodeAccessModule::ShutdownModule()
{
	// Unregister our settings
	ISettingsModule* SettingsModule = ISettingsModule::Get();
	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Editor", "General", "Source Code");
	}

	// unbind default provider from editor
	IModularFeatures::Get().UnregisterModularFeature(SourceCodeAccessorFeatureName, &DefaultSourceCodeAccessor);

	// we don't care about modular features any more
	IModularFeatures::Get().OnModularFeatureRegistered().RemoveAll(this);
}

bool FSourceCodeAccessModule::CanAccessSourceCode() const
{
	return CurrentSourceCodeAccessor->CanAccessSourceCode();
}

ISourceCodeAccessor& FSourceCodeAccessModule::GetAccessor() const
{
	return *CurrentSourceCodeAccessor;
}

void FSourceCodeAccessModule::SetAccessor(const FName& InName)
{
	const int32 FeatureCount = IModularFeatures::Get().GetModularFeatureImplementationCount(SourceCodeAccessorFeatureName);
	for(int32 FeatureIndex = 0; FeatureIndex < FeatureCount; FeatureIndex++)
	{
		IModularFeature* Feature = IModularFeatures::Get().GetModularFeatureImplementation(SourceCodeAccessorFeatureName, FeatureIndex);
		check(Feature);

		ISourceCodeAccessor& Accessor = *static_cast<ISourceCodeAccessor*>(Feature);
		if(InName == Accessor.GetFName())
		{
			CurrentSourceCodeAccessor = static_cast<ISourceCodeAccessor*>(Feature);
			if(FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().SetWidgetReflectorSourceAccessDelegate( FAccessSourceCode::CreateRaw( CurrentSourceCodeAccessor, &ISourceCodeAccessor::OpenFileAtLine ) );
				FSlateApplication::Get().SetWidgetReflectorQuerySourceAccessDelegate( FQueryAccessSourceCode::CreateRaw( CurrentSourceCodeAccessor, &ISourceCodeAccessor::CanAccessSourceCode ) );
			}
			break;
		}
	}
}

FLaunchingCodeAccessor& FSourceCodeAccessModule::OnLaunchingCodeAccessor()
{
	return LaunchingCodeAccessorDelegate;
}

FDoneLaunchingCodeAccessor& FSourceCodeAccessModule::OnDoneLaunchingCodeAccessor()
{
	return DoneLaunchingCodeAccessorDelegate;
}

FLaunchCodeAccessorDeferred& FSourceCodeAccessModule::OnLaunchCodeAccessorDeferred()
{
	return LaunchCodeAccessorDeferredDelegate;
}

FOpenFileFailed& FSourceCodeAccessModule::OnOpenFileFailed()
{
	return OpenFileFailedDelegate;
}

void FSourceCodeAccessModule::HandleModularFeatureRegistered(const FName& Type)
{
	if(Type == SourceCodeAccessorFeatureName)
	{
		CurrentSourceCodeAccessor = &DefaultSourceCodeAccessor;

		const FString PreferredAccessor = GetDefault<USourceCodeAccessSettings>()->PreferredAccessor;

		const int32 FeatureCount = IModularFeatures::Get().GetModularFeatureImplementationCount(Type);
		for(int32 FeatureIndex = 0; FeatureIndex < FeatureCount; FeatureIndex++)
		{
			IModularFeature* Feature = IModularFeatures::Get().GetModularFeatureImplementation(Type, FeatureIndex);
			check(Feature);

			ISourceCodeAccessor& Accessor = *static_cast<ISourceCodeAccessor*>(Feature);
			if(PreferredAccessor == Accessor.GetFName().ToString())
			{
				CurrentSourceCodeAccessor = static_cast<ISourceCodeAccessor*>(Feature);
				if(FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().SetWidgetReflectorSourceAccessDelegate( FAccessSourceCode::CreateRaw( CurrentSourceCodeAccessor, &ISourceCodeAccessor::OpenFileAtLine ) );
					FSlateApplication::Get().SetWidgetReflectorQuerySourceAccessDelegate( FQueryAccessSourceCode::CreateRaw( CurrentSourceCodeAccessor, &ISourceCodeAccessor::CanAccessSourceCode ) );
				}
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE 