// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_REQUESTS_APPLICATIONLIFECYCLE_H
#define OVR_REQUESTS_APPLICATIONLIFECYCLE_H

#include "OVR_Types.h"
#include "OVR_Platform_Defs.h"


/// Return a list of all the pids that we know are registered to your
/// application.
///
/// A message with type ::ovrMessage_ApplicationLifecycle_GetRegisteredPIDs will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrPidArrayHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetPidArray().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_ApplicationLifecycle_GetRegisteredPIDs();

/// The launcher, which we know and trust can use this to create a key, pass it
/// to the UWP app, which will be able to register using it as the original
/// application.
///
/// A message with type ::ovrMessage_ApplicationLifecycle_GetSessionKey will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type const char *.
/// Extract the payload from the message handle with ::ovr_Message_GetString().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_ApplicationLifecycle_GetSessionKey();

/// The actual UWP app will be able to register as the original application
/// using the guid from getSessionKey() .
///
/// A message with type ::ovrMessage_ApplicationLifecycle_RegisterSessionKey will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// This response has no payload. If no error occured, the request was successful. Yay!
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_ApplicationLifecycle_RegisterSessionKey(const char *sessionKey);

#endif
