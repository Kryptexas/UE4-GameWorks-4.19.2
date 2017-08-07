#pragma once

// AppleARKit
#include "IAppleARKitModule.h"
#include "AppleARKitSession.h"

// UE4
#include "Templates/SharedPointer.h"

class FAppleARKitModule : public IAppleARKitModule
{
public:

	/** 
	 * Returns the module singelton session create and destroyed during StartupModule & 
	 * ShutdownModule respectively.
	 */
	UAppleARKitSession* GetSession() const
	{
		return Session;
	}

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FAppleARKitModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FAppleARKitModule >("AppleARKit");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "AppleARKit" );
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	// The index into GEngine->EngineStats
	int32 StatARKitIndex = -1;

	// Shared AppleARKit session. Non-ref-counted pointer as instance is in root object set.
	UAppleARKitSession* Session = nullptr;
};

DECLARE_LOG_CATEGORY_EXTERN(LogAppleARKit, Log, All);
