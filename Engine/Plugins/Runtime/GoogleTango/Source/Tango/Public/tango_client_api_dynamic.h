/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID

#include "tango_client_api.h"

struct TangoService_API {
  TangoErrorType (*TangoService_initialize)(void* jni_env, void* activity);
  TangoErrorType (*TangoService_setBinder)(void* jni_env, void* iBinder);
  TangoErrorType (*TangoService_connect)(void* context, TangoConfig config);
  TangoErrorType (*TangoService_setRuntimeConfig)(TangoConfig tconfig);
  TangoErrorType (*TangoService_connectOnPoseAvailable)(
      uint32_t count, const TangoCoordinateFramePair* frames,
      void (*TangoService_onPoseAvailable)(void* context,
                                           const TangoPoseData* pose),
    ...);
  TangoErrorType (*TangoService_getPoseAtTime)(double timestamp,
                                               TangoCoordinateFramePair frame,
                                               TangoPoseData* pose);
  TangoErrorType (*TangoService_connectOnXYZijAvailable)(
      void (*TangoService_onXYZijAvailable)(void* context,
                                            const TangoXYZij* xyz_ij),
      ...);
  TangoErrorType (*TangoService_connectOnPointCloudAvailable)(
      void (*TangoService_onPointCloudAvailable)(void* context,
                                                 const TangoPointCloud* cloud),
      ...);
  TangoErrorType (*TangoService_connectOnTangoEvent)(
      void (*TangoService_onTangoEvent)(void* context, const TangoEvent* event),
      ...);
  TangoErrorType (*TangoService_connectTextureId)(
      TangoCameraId id, unsigned int tex, void* context,
      TangoService_OnTextureAvailable callback);
  TangoErrorType (*TangoService_updateTexture)(TangoCameraId id, double* timestamp);
  TangoErrorType (*TangoService_connectOnTextureAvailable)(
      TangoCameraId id, void* context, TangoService_OnTextureAvailable callback);
  TangoErrorType (*TangoService_updateTextureExternalOes)(TangoCameraId id,
                                                          unsigned int tex,
                                                          double* timestamp);
  TangoErrorType (*TangoService_lockCameraBuffer)(TangoCameraId id,
                                                  double* timestamp,
                                                  TangoBufferId* buffer);
  TangoErrorType (*TangoService_unlockCameraBuffer)(TangoCameraId id,
                                                    double* timestamp,
                                                    TangoBufferId* buffer);
TangoErrorType (*TangoService_updateTextureExternalOesForBuffer)(
    TangoCameraId id, unsigned int texture_id, TangoBufferId buffer);
TangoErrorType (*TangoService_connectOnFrameAvailable)(
    TangoCameraId id, void* context,
    void (*onFrameAvailable)(void* context, TangoCameraId id,
                             const TangoImageBuffer* buffer));
TangoErrorType (*TangoService_connectOnImageAvailable)(
    TangoCameraId id, void* context,
    void (*onImageAvailable)(void* context, TangoCameraId id,
                             const TangoImage* image,
                             const TangoCameraMetadata* metadata));
  TangoErrorType (*TangoService_disconnectCamera)(TangoCameraId id);
  TangoErrorType (*TangoService_getCameraIntrinsics)(TangoCameraId camera_id,
                                                   TangoCameraIntrinsics* intrinsics);
  TangoErrorType (*TangoService_saveAreaDescription)(TangoUUID* uuid);
  TangoErrorType (*TangoService_deleteAreaDescription)(const TangoUUID uuid);
  TangoErrorType (*TangoService_getAreaDescriptionUUIDList)(char** uuid_list);
  TangoErrorType (*TangoService_getAreaDescriptionMetadata)(
      const TangoUUID uuid, TangoAreaDescriptionMetadata* metadata);
  TangoErrorType (*TangoService_saveAreaDescriptionMetadata)(
      const TangoUUID uuid, TangoAreaDescriptionMetadata metadata);
  TangoErrorType (*TangoAreaDescriptionMetadata_free)(
      TangoAreaDescriptionMetadata metadata);
  TangoErrorType (*TangoService_importAreaDescription)(const char* src_file_path,
                                                       TangoUUID* uuid);
  TangoErrorType (*TangoService_exportAreaDescription)(
      const TangoUUID uuid, const char* dst_file_dir);
  TangoErrorType (*TangoAreaDescriptionMetadata_get)(
      TangoAreaDescriptionMetadata metadata, const char* key, size_t* value_size,
      char** value);
  TangoErrorType (*TangoAreaDescriptionMetadata_set)(
      TangoAreaDescriptionMetadata metadata, const char* key, size_t value_size,
      const char* value);
  TangoErrorType (*TangoAreaDescriptionMetadata_listKeys)(
      TangoAreaDescriptionMetadata metadata, char** key_list);
  TangoErrorType (*TangoConfig_setBool)(TangoConfig config, const char* key,
                                        bool value);
  TangoErrorType (*TangoConfig_setInt32)(TangoConfig config, const char* key,
                                         int32_t value);
  TangoErrorType (*TangoConfig_setInt64)(TangoConfig config, const char* key,
                                         int64_t value);
  TangoErrorType (*TangoConfig_setDouble)(TangoConfig config, const char* key,
                                          double value);
  TangoErrorType (*TangoConfig_setString)(TangoConfig config, const char* key,
                                          const char* value);
  TangoErrorType (*TangoConfig_getBool)(TangoConfig config, const char* key,
                                        bool* value);
  TangoErrorType (*TangoConfig_getInt32)(TangoConfig config, const char* key,
                                         int32_t* value);
  TangoErrorType (*TangoConfig_getInt64)(TangoConfig config, const char* key,
                                         int64_t* value);
  TangoErrorType (*TangoConfig_getDouble)(TangoConfig config, const char* key,
                                          double* value);
  TangoErrorType (*TangoConfig_getString)(TangoConfig config, const char* key,
                                          char* value, size_t size);
  TangoErrorType (*TangoService_Experimental_connectTextureIdUnity)(
      TangoCameraId id, unsigned int texture_y, unsigned int texture_Cb,
      unsigned int texture_Cr, void* context,
      void (*callback)(void*, TangoCameraId));
  TangoErrorType (*TangoService_Experimental_getDatasetUUIDs)(
      TangoUUID** dataset_uuids, int* num_dataset_uuids);
  TangoErrorType (*TangoService_Experimental_releaseDatasetUUIDs)(
      TangoUUID** dataset_uuids);
  TangoErrorType (*TangoService_Experimental_deleteDataset)(
      const TangoUUID dataset_uuid);
  TangoErrorType (*TangoService_Experimental_getCurrentDatasetUUID)(
      TangoUUID* dataset_uuid);

  // API functions that don't return TangoErrorType
  TangoConfig (*TangoService_getConfig)(TangoConfigType config_type);
  void (*TangoService_disconnect)();
  void(*TangoConfig_free)(TangoConfig);
  const char* (*TangoConfig_toString)(TangoConfig);
};

TangoService_API* GetTheTangoServiceAPI();

#define TangoService_initialize_dynamic GetTheTangoServiceAPI()->TangoService_initialize
#define TangoService_setBinder_dynamic GetTheTangoServiceAPI()->TangoService_setBinder
#define TangoService_connect_dynamic GetTheTangoServiceAPI()->TangoService_connect
#define TangoService_setRuntimeConfig_dynamic GetTheTangoServiceAPI()->TangoService_setRuntimeConfig
#define TangoService_connectOnPoseAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnPoseAvailable
#define TangoService_getPoseAtTime_dynamic GetTheTangoServiceAPI()->TangoService_getPoseAtTime
#define TangoService_connectOnXYZijAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnXYZijAvailable
#define TangoService_connectOnPointCloudAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnPointCloudAvailable
#define TangoService_connectOnTangoEvent_dynamic GetTheTangoServiceAPI()->TangoService_connectOnTangoEvent
#define TangoService_connectTextureId_dynamic GetTheTangoServiceAPI()->TangoService_connectTextureId
#define TangoService_updateTexture_dynamic GetTheTangoServiceAPI()->TangoService_updateTexture
#define TangoService_connectOnTextureAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnTextureAvailable
#define TangoService_updateTextureExternalOes_dynamic GetTheTangoServiceAPI()->TangoService_updateTextureExternalOes
#define TangoService_lockCameraBuffer_dynamic GetTheTangoServiceAPI()->TangoService_lockCameraBuffer
#define TangoService_updateTextureExternalOesForBuffer_dynamic GetTheTangoServiceAPI()->TangoService_updateTextureExternalOesForBuffer
#define TangoService_connectOnFrameAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnFrameAvailable
#define TangoService_connectOnImageAvailable_dynamic GetTheTangoServiceAPI()->TangoService_connectOnImageAvailable
#define TangoService_disconnectCamera_dynamic GetTheTangoServiceAPI()->TangoService_disconnectCamera
#define TangoService_getCameraIntrinsics_dynamic GetTheTangoServiceAPI()->TangoService_getCameraIntrinsics
#define TangoService_saveAreaDescription_dynamic GetTheTangoServiceAPI()->TangoService_saveAreaDescription
#define TangoService_deleteAreaDescription_dynamic GetTheTangoServiceAPI()->TangoService_deleteAreaDescription
#define TangoService_getAreaDescriptionUUIDList_dynamic GetTheTangoServiceAPI()->TangoService_getAreaDescriptionUUIDList
#define TangoService_getAreaDescriptionMetadata_dynamic GetTheTangoServiceAPI()->TangoService_getAreaDescriptionMetadata
#define TangoService_saveAreaDescriptionMetadata_dynamic GetTheTangoServiceAPI()->TangoService_saveAreaDescriptionMetadata
#define TangoAreaDescriptionMetadata_free_dynamic GetTheTangoServiceAPI()->TangoAreaDescriptionMetadata_free
#define TangoService_importAreaDescription_dynamic GetTheTangoServiceAPI()->TangoService_importAreaDescription
#define TangoService_exportAreaDescription_dynamic GetTheTangoServiceAPI()->TangoService_exportAreaDescription
#define TangoAreaDescriptionMetadata_get_dynamic GetTheTangoServiceAPI()->TangoAreaDescriptionMetadata_get
#define TangoAreaDescriptionMetadata_set_dynamic GetTheTangoServiceAPI()->TangoAreaDescriptionMetadata_set
#define TangoAreaDescriptionMetadata_listKeys_dynamic GetTheTangoServiceAPI()->TangoAreaDescriptionMetadata_listKeys
#define TangoConfig_setBool_dynamic GetTheTangoServiceAPI()->TangoConfig_setBool
#define TangoConfig_setInt32_dynamic GetTheTangoServiceAPI()->TangoConfig_setInt32
#define TangoConfig_setInt64_dynamic GetTheTangoServiceAPI()->TangoConfig_setInt64
#define TangoConfig_setDouble_dynamic GetTheTangoServiceAPI()->TangoConfig_setDouble
#define TangoConfig_setString_dynamic GetTheTangoServiceAPI()->TangoConfig_setString
#define TangoConfig_getBool_dynamic GetTheTangoServiceAPI()->TangoConfig_getBool
#define TangoConfig_getInt32_dynamic GetTheTangoServiceAPI()->TangoConfig_getInt32
#define TangoConfig_getInt64_dynamic GetTheTangoServiceAPI()->TangoConfig_getInt64
#define TangoConfig_getDouble_dynamic GetTheTangoServiceAPI()->TangoConfig_getDouble
#define TangoConfig_getString_dynamic GetTheTangoServiceAPI()->TangoConfig_getString
#define TangoService_Experimental_connectTextureIdUnity_dynamic GetTheTangoServiceAPI()->TangoService_Experimental_connectTextureIdUnity
#define TangoService_Experimental_getDatasetUUIDs_dynamic GetTheTangoServiceAPI()->TangoService_Experimental_getDatasetUUIDs
#define TangoService_Experimental_releaseDatasetUUIDs_dynamic GetTheTangoServiceAPI()->TangoService_Experimental_releaseDatasetUUIDs
#define TangoService_Experimental_deleteDataset_dynamic GetTheTangoServiceAPI()->TangoService_Experimental_deleteDataset
#define TangoService_Experimental_getCurrentDatasetUUID_dynamic GetTheTangoServiceAPI()->TangoService_Experimental_getCurrentDatasetUUID

#define TangoService_getConfig_dynamic GetTheTangoServiceAPI()->TangoService_getConfig
#define TangoService_disconnect_dynamic GetTheTangoServiceAPI()->TangoService_disconnect
#define TangoConfig_free_dynamic GetTheTangoServiceAPI()->TangoConfig_free
#define TangoConfig_toString_dynamic GetTheTangoServiceAPI()->TangoConfig_toString
#endif
