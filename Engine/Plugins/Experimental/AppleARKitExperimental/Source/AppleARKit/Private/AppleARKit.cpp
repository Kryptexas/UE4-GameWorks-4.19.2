// AppleARKit
#include "AppleARKit.h"

// UE4
#if WITH_EDITOR
	#include "ISettingsModule.h"
	#include "ISettingsSection.h"
#endif

#define LOCTEXT_NAMESPACE "FAppleARKitModule"

void FAppleARKitExperimentalModule::StartupModule()
{
	// Create a shared session (but don't start it yet)
	Session = NewObject< UAppleARKitSession >();

	// Add it to the root set to avoid GC
	Session->AddToRoot();

	#if WITH_EDITOR

		// Register settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule != nullptr)
		{
			ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings( "Project", "Plugins", "AppleARKit",
				NSLOCTEXT("AppleARKit", "AppleARKitSettingsName", "AppleARKit"),
				NSLOCTEXT("AppleARKit", "AppleARKitSettingsDescription", "Configure the AppleARKit session configuration defaults."),
				GetMutableDefault<UAppleARKitWorldTrackingConfiguration>()
			);
		}

	#endif //WITH_EDITOR
}

void FAppleARKitExperimentalModule::ShutdownModule()
{
	#if WITH_EDITOR

		// Unregister settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>( "Settings" );
		if ( SettingsModule != nullptr )
		{
			SettingsModule->UnregisterSettings( "Project", "Plugins", "AppleARKit" );
		}

	#endif //WITH_EDITOR


	// note: when the Editor exits, Session has already been deleted, so there is no need to delete it.
	Session = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAppleARKitExperimentalModule, AppleARKitExperimental)

DEFINE_LOG_CATEGORY(LogAppleARKit);
