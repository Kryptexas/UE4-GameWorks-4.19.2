// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once 

/**
* The public interface of the CameraPhotographyModule
*/
class ENGINE_API ICameraPhotographyModule : public IModuleInterface, public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static FName FeatureName = FName(TEXT("CameraPhotography"));
		return FeatureName;
	}

	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	/**
	* Singleton-like access to ICameraPhotographyModule
	*
	* @return Returns ICameraPhotographyModule singleton instance, loading the module on demand if needed
	*/
	static inline ICameraPhotographyModule& Get()
	{
		return FModuleManager::LoadModuleChecked< ICameraPhotographyModule >("InputDevice");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("InputDevice");
	}

	/**
	* Attempts to create a new head tracking device interface
	*
	* @return	Interface to the new head tracking device, if we were able to successfully create one
	*/
	virtual TSharedPtr< class ICameraPhotography > CreateCameraPhotography() = 0;
};

class ENGINE_API ICameraPhotography
{
public:
	ICameraPhotography() {};
	virtual ~ICameraPhotography() {};
	virtual bool UpdateCamera(FMinimalViewInfo& InOutPOV, APlayerCameraManager* PCMgr) = 0;
	virtual bool IsSupported() = 0;
	virtual const TCHAR* const GetProviderName() = 0;	
};
