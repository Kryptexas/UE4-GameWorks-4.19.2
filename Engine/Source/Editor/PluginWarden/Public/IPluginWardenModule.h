// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * The Plugin Warden is a simple module used to verify a user has purchased a plug-in.  This
 * module won't prevent a determined user from avoiding paying for a plug-in, it is merely to
 * prevent accidental violation of a per-seat license on a plug-in, and to direct those users
 * to the marketplace page where they may purchase the plug-in.
 */
class IPluginWardenModule : public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPluginWardenModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IPluginWardenModule >( "PluginWarden" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "PluginWarden" );
	}

	/**
	 * Ask the launcher if the user has authorization to use the given plug-in. The authorized 
	 * callback will only be called if the user is authorized to use the plug-in.
	 * 
	 * IPluginWardenModule::Get().CheckEntitlementForPlugin(LOCTEXT("AwesomePluginName", "My Awesome Plugin"), TEXT("PLUGIN_MARKETPLACE_GUID"), [&] () {
	 *		// Authorized Code Here
	 * });
	 * 
	 * @param PluginFriendlyName The localized friendly name of the plug-in.
	 * @param PluginGuid The unique identifier of the plug-in on the marketplace.
	 * @param AuthorizedCallback This function will be called after the user has been given entitlement.
	 */
	virtual void CheckEntitlementForPlugin(const FText& PluginFriendlyName, const FString& PluginGuid, TFunction<void()> AuthorizedCallback) = 0;
};