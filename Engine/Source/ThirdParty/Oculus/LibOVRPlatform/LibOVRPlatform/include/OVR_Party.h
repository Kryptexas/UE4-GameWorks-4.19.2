// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_PARTY_H
#define OVR_PARTY_H

#include "OVR_Platform_Defs.h"
#include "OVR_Room.h"
#include "OVR_Types.h"
#include "OVR_User.h"
#include "OVR_UserArray.h"
#include <stddef.h>

typedef struct ovrParty *ovrPartyHandle;

OVRP_PUBLIC_FUNCTION(ovrID)              ovr_Party_GetID(const ovrPartyHandle obj);
OVRP_PUBLIC_FUNCTION(ovrUserArrayHandle) ovr_Party_GetInvitedUsers(const ovrPartyHandle obj);
OVRP_PUBLIC_FUNCTION(ovrUserHandle)      ovr_Party_GetLeader(const ovrPartyHandle obj);
OVRP_PUBLIC_FUNCTION(ovrRoomHandle)      ovr_Party_GetRoom(const ovrPartyHandle obj);
OVRP_PUBLIC_FUNCTION(ovrUserArrayHandle) ovr_Party_GetUsers(const ovrPartyHandle obj);

#endif
