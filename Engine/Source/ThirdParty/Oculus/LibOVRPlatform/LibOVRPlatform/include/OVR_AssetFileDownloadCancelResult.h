// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_ASSETFILEDOWNLOADCANCELRESULT_H
#define OVR_ASSETFILEDOWNLOADCANCELRESULT_H

#include "OVR_Platform_Defs.h"
#include "OVR_Types.h"
#include <stdbool.h>

typedef struct ovrAssetFileDownloadCancelResult *ovrAssetFileDownloadCancelResultHandle;

/// ID of the asset file.
OVRP_PUBLIC_FUNCTION(ovrID) ovr_AssetFileDownloadCancelResult_GetAssetFileId(const ovrAssetFileDownloadCancelResultHandle obj);

/// Whether the cancel request is succeeded.
OVRP_PUBLIC_FUNCTION(bool) ovr_AssetFileDownloadCancelResult_GetSuccess(const ovrAssetFileDownloadCancelResultHandle obj);


#endif
