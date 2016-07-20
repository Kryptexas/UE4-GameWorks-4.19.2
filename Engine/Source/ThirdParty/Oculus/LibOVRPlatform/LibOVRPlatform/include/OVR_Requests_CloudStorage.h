// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#ifndef OVR_REQUESTS_CLOUDSTORAGE_H
#define OVR_REQUESTS_CLOUDSTORAGE_H

#include "OVR_Types.h"
#include "OVR_Platform_Defs.h"

#include "OVR_CloudStorage.h"
#include "OVR_CloudStorageConflictMetadata.h"
#include "OVR_CloudStorageMetadata.h"
#include "OVR_CloudStorageMetadataArray.h"
#include <stdbool.h>

/// Get the next page of entries
///
/// A message with type ::ovrMessage_CloudStorage_GetNextCloudStorageMetadataArrayPage will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageMetadataArrayHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageMetadataArray().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_GetNextCloudStorageMetadataArrayPage(ovrCloudStorageMetadataArrayHandle handle);

/// Loads the saved entry for the specified bucket and key. If a conflict
/// exists with the key then an error message is returned.
/// \param bucket the name of the storage bucket
/// \param key the name for this saved data
///
/// A message with type ::ovrMessage_CloudStorage_Load will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageDataHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageData().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_Load(const char *bucket, const char *key);

/// loads all the metadata for the saves in the specified bucket including
/// conflicts
/// \param bucket the name of the storage bucket
///
/// A message with type ::ovrMessage_CloudStorage_LoadBucketMetadata will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageMetadataArrayHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageMetadataArray().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_LoadBucketMetadata(const char *bucket);

/// Loads the metadata for this bucket-key combination that need to be manually
/// resolved.
/// \param bucket the name of the storage bucket
/// \param key the 'name' for this saved data
///
/// A message with type ::ovrMessage_CloudStorage_LoadConflictMetadata will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageConflictMetadataHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageConflictMetadata().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_LoadConflictMetadata(const char *bucket, const char *key);

/// Loads the data specified by the storage handle.
///
/// A message with type ::ovrMessage_CloudStorage_LoadHandle will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageDataHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageData().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_LoadHandle(ovrCloudStorageSaveHandle handle);

/// load the metadata for the specified key
/// \param bucket the name of the storage bucket
/// \param key the name for this saved data
///
/// A message with type ::ovrMessage_CloudStorage_LoadMetadata will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageMetadataHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageMetadata().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_LoadMetadata(const char *bucket, const char *key);

/// Selects the local save for manual conflict resolution.
/// \param bucket the name of the storage bucket
/// \param key the name for this saved data
/// \param remoteHandle the handle of the remote that the local file was resolved against
///
/// A message with type ::ovrMessage_CloudStorage_ResolveKeepLocal will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageUpdateResponseHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageUpdateResponse().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_ResolveKeepLocal(const char *bucket, const char *key, ovrCloudStorageSaveHandle remoteHandle);

/// Selects the remote save for manual conflict resolution.
/// \param bucket the name of the storage bucket
/// \param key the name for this saved data
/// \param remoteHandle the handle of the remote
///
/// A message with type ::ovrMessage_CloudStorage_ResolveKeepRemote will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageUpdateResponseHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageUpdateResponse().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_ResolveKeepRemote(const char *bucket, const char *key, ovrCloudStorageSaveHandle remoteHandle);

/// Send a save data buffer to the platform.
/// \param bucket the name of the storage bucket
/// \param key the name for this saved data
/// \param data start of the data block
/// \param dataSize size of the data block
/// \param counter optional counter used for user data or auto-deconfliction
/// \param extraData optional string that isn't used by platform
///
/// A message with type ::ovrMessage_CloudStorage_Save will be generated in response.
///
/// First call ::ovr_Message_IsError() to check if an error occurred.
///
/// If no error occurred, the message will contain a payload of type ::ovrCloudStorageUpdateResponseHandle.
/// Extract the payload from the message handle with ::ovr_Message_GetCloudStorageUpdateResponse().
OVRP_PUBLIC_FUNCTION(ovrRequest) ovr_CloudStorage_Save(const char *bucket, const char *key, const void *data, unsigned int dataSize, long long counter, const char *extraData);

#endif
