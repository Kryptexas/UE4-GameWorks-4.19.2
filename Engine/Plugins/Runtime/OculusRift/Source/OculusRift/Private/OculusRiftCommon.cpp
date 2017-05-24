// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OculusRiftCommon.h"

#if OCULUS_RIFT_SUPPORTED_PLATFORMS

#include "OculusRiftHMD.h"

//////////////////////////////////////////////////////////////////////////
ovrResult FOvrSessionShared::Create(ovrGraphicsLuid& luid)
{
	Destroy();
	ovrResult result = FOculusRiftPlugin::Get().CreateSession(&Session, &luid);
	if (OVR_SUCCESS(result))
	{
		UE_LOG(LogHMD, Log, TEXT("New ovr session is created"));
		DelegatesLock.Lock();
		CreateDelegate.Broadcast(Session);
		DelegatesLock.Unlock();
	}
	else
	{
		UE_LOG(LogHMD, Log, TEXT("Failed to create new ovr session, err = %d"), int(result));
	}
	return result;
}

void FOvrSessionShared::Destroy()
{
	if (Session)
	{
		UE_LOG(LogHMD, Log, TEXT("Destroying ovr session"));
		FSuspendRenderingThread SuspendRenderingThread(false);
		if (LockCnt.GetValue() != 0)
		{
			UE_LOG(LogHMD, Warning, TEXT("OVR:  Lock count is not 0!  Check for multiple sessions having been started."));
		}

		DelegatesLock.Lock();
		DestoryDelegate.Broadcast(Session);
		DelegatesLock.Unlock();

		FOculusRiftPlugin::Get().DestroySession(Session);
		Session = nullptr;
	}
}

#endif // #if OCULUS_RIFT_SUPPORTED_PLATFORMS
