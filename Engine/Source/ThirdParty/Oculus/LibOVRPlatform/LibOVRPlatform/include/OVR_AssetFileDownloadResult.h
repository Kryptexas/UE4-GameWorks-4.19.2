// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_ASSETFILEDOWNLOADRESULT_H
#define OVR_ASSETFILEDOWNLOADRESULT_H

#include "OVR_Platform_Defs.h"

typedef struct ovrAssetFileDownloadResult *ovrAssetFileDownloadResultHandle;

/// Filepath of the downloaded asset file.
OVRP_PUBLIC_FUNCTION(const char *) ovr_AssetFileDownloadResult_GetFilepath(const ovrAssetFileDownloadResultHandle obj);


#endif
