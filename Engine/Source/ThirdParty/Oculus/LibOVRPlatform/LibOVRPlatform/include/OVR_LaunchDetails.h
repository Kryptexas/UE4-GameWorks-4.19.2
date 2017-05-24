// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_LAUNCHDETAILS_H
#define OVR_LAUNCHDETAILS_H

#include "OVR_Platform_Defs.h"
#include "OVR_LaunchType.h"
#include "OVR_UserArray.h"
#include <stddef.h>

typedef struct ovrLaunchDetails *ovrLaunchDetailsHandle;

/// An arbitrary string provided by the developer to help them deeplink to
/// content on app startup.
OVRP_PUBLIC_FUNCTION(const char *) ovr_LaunchDetails_GetDeeplinkMessage(const ovrLaunchDetailsHandle obj);

OVRP_PUBLIC_FUNCTION(ovrLaunchType)      ovr_LaunchDetails_GetLaunchType(const ovrLaunchDetailsHandle obj);
OVRP_PUBLIC_FUNCTION(ovrID)              ovr_LaunchDetails_GetRoomID(const ovrLaunchDetailsHandle obj);
OVRP_PUBLIC_FUNCTION(ovrUserArrayHandle) ovr_LaunchDetails_GetUsers(const ovrLaunchDetailsHandle obj);

#endif
