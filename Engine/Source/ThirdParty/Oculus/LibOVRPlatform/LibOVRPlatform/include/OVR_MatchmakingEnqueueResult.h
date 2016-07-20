// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_MATCHMAKINGENQUEUERESULT_H
#define OVR_MATCHMAKINGENQUEUERESULT_H

#include "OVR_Platform_Defs.h"
#include <stddef.h>

typedef struct ovrMatchmakingEnqueueResult *ovrMatchmakingEnqueueResultHandle;

/// The mean amount of time that users in this user's queue have waited in the
/// last hour or more. Whether users cancel or find a match, the time they
/// waited is incorporated in this figure. Use this to give users an indication
/// of how long they can expect to wait.
OVRP_PUBLIC_FUNCTION(unsigned int) ovr_MatchmakingEnqueueResult_GetAverageWait(const ovrMatchmakingEnqueueResultHandle obj);

/// The number of matches made from the pool the user is participating in. You
/// can use this to give users an indication of whether they should bother to
/// wait.
OVRP_PUBLIC_FUNCTION(unsigned int) ovr_MatchmakingEnqueueResult_GetMatchesInLastHourCount(const ovrMatchmakingEnqueueResultHandle obj);

/// The 95% amount of time that users in this user's queue have waited in the
/// last hour or more. Whether users cancel or find a match, the time they
/// waited is incorporated in this figure. Use this to give users an indication
/// of how long they can expect to wait.
OVRP_PUBLIC_FUNCTION(unsigned int) ovr_MatchmakingEnqueueResult_GetMaxExpectedWait(const ovrMatchmakingEnqueueResultHandle obj);

/// 0-100 showing percentage of people in the same queue as the user who got
/// matched. Stats are taken from the last hour or more. You can use this to
/// give users an indication of whether they should wait.
OVRP_PUBLIC_FUNCTION(unsigned int) ovr_MatchmakingEnqueueResult_GetRecentMatchPercentage(const ovrMatchmakingEnqueueResultHandle obj);

OVRP_PUBLIC_FUNCTION(const char *) ovr_MatchmakingEnqueueResult_GetPool(const ovrMatchmakingEnqueueResultHandle obj);
OVRP_PUBLIC_FUNCTION(const char *) ovr_MatchmakingEnqueueResult_GetRequestHash(const ovrMatchmakingEnqueueResultHandle obj);

#endif
