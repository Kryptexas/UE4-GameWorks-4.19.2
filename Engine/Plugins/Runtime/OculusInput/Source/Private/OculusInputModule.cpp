// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusInput.h"
#include "IOculusInputPlugin.h"

#if OCULUS_TOUCH_SUPPORTED_PLATFORMS

class FOculusInputModule : public IOculusInputPlugin
{
	// IInputDeviceModule overrides
	virtual TSharedPtr< class IInputDevice > CreateInputDevice( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override
	{
		return TSharedPtr< class IInputDevice >( new FOculusInput( InMessageHandler ) );
	}
};

#else	//	OCULUS_TOUCH_SUPPORTED_PLATFORMS

class FOculusInputModule : public FDefaultModuleImpl
{
};

#endif	// OCULUS_TOUCH_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE( FOculusInputModule, OculusInput )
