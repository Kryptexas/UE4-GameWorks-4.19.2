// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_REQUESTS_ASSETFILE_H
#define OVR_REQUESTS_ASSETFILE_H

#include "OVR_Types.h"
#include "OVR_Platform_Defs.h"


/// \file
/// # Asset Files
///
/// An asset file is an extra dynamic contents which can be downloaded post-installation at runtime. This allows reducing initial installation size, and download the contents as users progress in your app. As an example might be extra game levels which are downloaded after finishing first set of levels.
///
/// Asset files are also used as a storage layer for DLCs (downloadable content), which can be proposed to a user separately from the main app.
///
/// If your apps supports asset files, users are able to download them from within the Oculus app on PDP page of the main app (including in Home), and you also can manage the asset files from your app itself via SDK calls.

/// Removes an previously installed asset file from the device by its ID.
/// Returns an object containing the asset file ID and a success flag.
/// \param assetFileID The asset file ID
///
/// A message with type ::ovrMessage_AssetFile_Delete will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrAssetFileDeleteResultHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetAssetFileDeleteResult().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_AssetFile_Delete(ovrID assetFileID);

/// Downloads an asset file by its ID on demand. Returns an object containing
/// filepath on the file system. Sends periodic
/// ovrNotification_AssetFile_DownloadUpdate to track the downloads.
/// \param assetFileID The asset file ID
///
/// A message with type ::ovrMessage_AssetFile_Download will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrAssetFileDownloadResultHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetAssetFileDownloadResult().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_AssetFile_Download(ovrID assetFileID);

/// Cancels a previously spawned download request for an asset file by its ID.
/// Returns an object containing asset file ID, and the success flag.
/// \param assetFileID The asset file ID
///
/// A message with type ::ovrMessage_AssetFile_DownloadCancel will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrAssetFileDownloadCancelResultHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetAssetFileDownloadCancelResult().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_AssetFile_DownloadCancel(ovrID assetFileID);

#endif
