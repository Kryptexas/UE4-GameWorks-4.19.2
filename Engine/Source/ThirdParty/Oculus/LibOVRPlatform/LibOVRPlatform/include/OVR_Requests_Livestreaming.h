// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_REQUESTS_LIVESTREAMING_H
#define OVR_REQUESTS_LIVESTREAMING_H

#include "OVR_Types.h"
#include "OVR_Platform_Defs.h"


/// Return the status of the current livestreaming session if there is one.
///
/// A message with type ::ovrMessage_Livestreaming_GetStatus will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrLivestreamingStatusHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetLivestreamingStatus().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_Livestreaming_GetStatus();

#endif
