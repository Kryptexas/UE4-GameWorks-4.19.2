// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusInput.h"
#include "IOculusInputPlugin.h"

#if USE_OVR_MOTION_SDK

class FOculusInputModule : public IOculusInputPlugin
{
	// IInputDeviceModule overrides
	virtual TSharedPtr< class IInputDevice > CreateInputDevice( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override
	{
		return TSharedPtr< class IInputDevice >( new FOculusInput( InMessageHandler ) );
	}
};

#else	//	USE_OVR_MOTION_SDK

class FOculusInputModule : public FDefaultModuleImpl
{
};

#endif	// USE_OVR_MOTION_SDK

IMPLEMENT_MODULE( FOculusInputModule, OculusInput )
