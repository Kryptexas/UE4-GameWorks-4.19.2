// Copyright 2017 Google Inc.

#include "tango_client_api_dynamic.h"
#include "TangoPluginPrivate.h"
#if PLATFORM_ANDROID
#include <dlfcn.h>
#include <jni.h>

static void Load(void* Handle, TangoService_API* API);

static void ReturnVoid()
{
}

static const char* ReturnString()
{
	return "";
}

static TangoConfig ReturnTangoConfig()
{
	return nullptr;
}


static TangoErrorType ReturnTangoError()
{
	return TANGO_ERROR;
}

template <class T> void* LookupSymbolOrElse(void* Handle, const char* SymbolName, T Else)
{
	if (Handle == nullptr)
	{
		return (void*)Else;
	}
	void* Result = dlsym(Handle, SymbolName);
	if (Result)
	{
		UE_LOG(LogTango, Log, TEXT("Loaded: %s"), *FString(SymbolName));
	}
	return Result ? Result : (void*)Else;
}

TangoService_API* TheTangoServiceAPI = nullptr;

static TangoService_API* GetOrInitTheTangoServiceAPI()
{
	if (TheTangoServiceAPI != nullptr)
	{
		return TheTangoServiceAPI;
	}
	TangoService_API* TheAPI = new TangoService_API;
	void* Handle = dlopen("libtango_client_api.so", RTLD_NOW | RTLD_LOCAL);
	if (Handle == nullptr)
	{
		UE_LOG(LogTango, Error, TEXT("Failed to load libtango_client_api.so"));
	}
	else
	{
		UE_LOG(LogTango, Log, TEXT("Loaded libtango_client_api.so"));
	}
	Load(Handle, TheAPI);
	TheAPI->TangoService_getConfig = (TangoConfig(*)(TangoConfigType))LookupSymbolOrElse(Handle, "TangoService_getConfig", &ReturnTangoConfig);
	TheAPI->TangoService_disconnect = (void(*)()) LookupSymbolOrElse(Handle, "TangoService_disconnect", &ReturnVoid);
	TheAPI->TangoConfig_free = (void(*)(TangoConfig))LookupSymbolOrElse(Handle, "TangoConfig_free", &ReturnVoid);
	TheAPI->TangoConfig_toString = (const char *(*)(TangoConfig))LookupSymbolOrElse(Handle, "TangoConfig_toString", &ReturnString);
	return TheTangoServiceAPI = TheAPI;
}

static void* LookupSymbol(void* Handle, const char* SymbolName)
{
	return LookupSymbolOrElse(Handle, SymbolName, &ReturnTangoError);
}

static void Load(void* Handle, TangoService_API* API)
{
	API->TangoService_initialize = (TangoErrorType(*)(void* jni_env, void* activity))LookupSymbol(Handle, "TangoService_initialize");
	API->TangoService_setBinder = (TangoErrorType(*)(void* jni_env, void* iBinder))LookupSymbol(Handle, "TangoService_setBinder");
	API->TangoService_connect = (TangoErrorType(*)(void* context, TangoConfig config))LookupSymbol(Handle, "TangoService_connect");
	API->TangoService_setRuntimeConfig = (TangoErrorType(*)(TangoConfig config))LookupSymbol(Handle, "TangoService_setRuntimeConfig");
	API->TangoService_connectOnPoseAvailable = (TangoErrorType(*)(
		uint32_t count, const TangoCoordinateFramePair* frames,
		void(*TangoService_onPoseAvailable)(void* context,
			const TangoPoseData* pose),
		...))LookupSymbol(Handle, "TangoService_connectOnPoseAvailable");
	API->TangoService_getPoseAtTime = (TangoErrorType(*)(double timestamp,
		TangoCoordinateFramePair frame,
		TangoPoseData* pose))LookupSymbol(Handle, "TangoService_getPoseAtTime");
	API->TangoService_connectOnXYZijAvailable = (TangoErrorType(*)(
		void(*TangoService_onXYZijAvailable)(void* context,
			const TangoXYZij* xyz_ij),
		...))LookupSymbol(Handle, "TangoService_connectOnXYZijAvailable");
	API->TangoService_connectOnPointCloudAvailable = (TangoErrorType(*)(
		void(*TangoService_onPointCloudAvailable)(void* context,
			const TangoPointCloud* cloud),
		...))LookupSymbol(Handle, "TangoService_connectOnPointCloudAvailable");
	API->TangoService_connectOnTangoEvent = (TangoErrorType(*)(
		void(*TangoService_onTangoEvent)(void* context, const TangoEvent* event),
		...))LookupSymbol(Handle, "TangoService_connectOnTangoEvent");
	API->TangoService_connectTextureId = (TangoErrorType(*)(
		TangoCameraId id, unsigned int tex, void* context,
		TangoService_OnTextureAvailable callback))LookupSymbol(Handle, "TangoService_connectTextureId");
	API->TangoService_updateTexture = (TangoErrorType(*)(TangoCameraId id, double* timestamp))LookupSymbol(Handle, "TangoService_updateTexture");
	API->TangoService_connectOnTextureAvailable = (TangoErrorType(*)(
		TangoCameraId id, void* context, TangoService_OnTextureAvailable callback))LookupSymbol(Handle, "TangoService_connectOnTextureAvailable");
	API->TangoService_updateTextureExternalOes = (TangoErrorType(*)(TangoCameraId id,
		unsigned int tex,
		double* timestamp))LookupSymbol(Handle, "TangoService_updateTextureExternalOes");
	API->TangoService_lockCameraBuffer = (TangoErrorType(*)(TangoCameraId id,
		double* timestamp,
		TangoBufferId* buffer))LookupSymbol(Handle, "TangoService_lockCameraBuffer");
	API->TangoService_unlockCameraBuffer = (TangoErrorType(*)(TangoCameraId id,
		double* timestamp,
		TangoBufferId* buffer))LookupSymbol(Handle, "TangoService_unlockCameraBuffer");
	API->TangoService_updateTextureExternalOesForBuffer = (TangoErrorType(*)(
		TangoCameraId id, unsigned int texture_id, TangoBufferId buffer))LookupSymbol(Handle, "TangoService_updateTextureExternalOesForBuffer");
	API->TangoService_connectOnFrameAvailable = (TangoErrorType(*)(
		TangoCameraId id, void* context,
		void(*onFrameAvailable)(void* context, TangoCameraId id,
			const TangoImageBuffer* buffer)))LookupSymbol(Handle, "TangoService_connectOnFrameAvailable");
	API->TangoService_connectOnImageAvailable = (TangoErrorType(*)(
		TangoCameraId id, void* context,
		void(*onImageAvailable)(void* context, TangoCameraId id,
			const TangoImage* image,
			const TangoCameraMetadata* metadata)))LookupSymbol(Handle, "TangoService_connectOnImageAvailable");
	API->TangoService_disconnectCamera = (TangoErrorType(*)(TangoCameraId id))LookupSymbol(Handle, "TangoService_disconnectCamera");
	API->TangoService_getCameraIntrinsics = (TangoErrorType(*)(TangoCameraId camera_id,
		TangoCameraIntrinsics* intrinsics))LookupSymbol(Handle, "TangoService_getCameraIntrinsics");
	API->TangoService_saveAreaDescription = (TangoErrorType(*)(TangoUUID* uuid))LookupSymbol(Handle, "TangoService_saveAreaDescription");
	API->TangoService_deleteAreaDescription = (TangoErrorType(*)(const TangoUUID uuid))LookupSymbol(Handle, "TangoService_deleteAreaDescription");
	API->TangoService_getAreaDescriptionUUIDList = (TangoErrorType(*)(char** uuid_list))LookupSymbol(Handle, "TangoService_getAreaDescriptionUUIDList");
	API->TangoService_getAreaDescriptionMetadata = (TangoErrorType(*)(
		const TangoUUID uuid, TangoAreaDescriptionMetadata* metadata))LookupSymbol(Handle, "TangoService_getAreaDescriptionMetadata");
	API->TangoService_saveAreaDescriptionMetadata = (TangoErrorType(*)(
		const TangoUUID uuid, TangoAreaDescriptionMetadata metadata))LookupSymbol(Handle, "TangoService_saveAreaDescriptionMetadata");
	API->TangoAreaDescriptionMetadata_free = (TangoErrorType(*)(
		TangoAreaDescriptionMetadata metadata))LookupSymbol(Handle, "TangoAreaDescriptionMetadata_free");
	API->TangoService_importAreaDescription = (TangoErrorType(*)(const char* src_file_path,
		TangoUUID* uuid))LookupSymbol(Handle, "TangoService_importAreaDescription");
	API->TangoService_exportAreaDescription = (TangoErrorType(*)(
		const TangoUUID uuid, const char* dst_file_dir))LookupSymbol(Handle, "TangoService_exportAreaDescription");
	API->TangoAreaDescriptionMetadata_get = (TangoErrorType(*)(
		TangoAreaDescriptionMetadata metadata, const char* key, size_t* value_size,
		char** value))LookupSymbol(Handle, "TangoAreaDescriptionMetadata_get");
	API->TangoAreaDescriptionMetadata_set = (TangoErrorType(*)(
		TangoAreaDescriptionMetadata metadata, const char* key, size_t value_size,
		const char* value))LookupSymbol(Handle, "TangoAreaDescriptionMetadata_set");
	API->TangoAreaDescriptionMetadata_listKeys = (TangoErrorType(*)(
		TangoAreaDescriptionMetadata metadata, char** key_list))LookupSymbol(Handle, "TangoAreaDescriptionMetadata_listKeys");
	API->TangoConfig_setBool = (TangoErrorType(*)(TangoConfig config, const char* key,
		bool value))LookupSymbol(Handle, "TangoConfig_setBool");
	API->TangoConfig_setInt32 = (TangoErrorType(*)(TangoConfig config, const char* key,
		int32_t value))LookupSymbol(Handle, "TangoConfig_setInt32");
	API->TangoConfig_setInt64 = (TangoErrorType(*)(TangoConfig config, const char* key,
		int64_t value))LookupSymbol(Handle, "TangoConfig_setInt64");
	API->TangoConfig_setDouble = (TangoErrorType(*)(TangoConfig config, const char* key,
		double value))LookupSymbol(Handle, "TangoConfig_setDouble");
	API->TangoConfig_setString = (TangoErrorType(*)(TangoConfig config, const char* key,
		const char* value))LookupSymbol(Handle, "TangoConfig_setString");
	API->TangoConfig_getBool = (TangoErrorType(*)(TangoConfig config, const char* key,
		bool* value))LookupSymbol(Handle, "TangoConfig_getBool");
	API->TangoConfig_getInt32 = (TangoErrorType(*)(TangoConfig config, const char* key,
		int32_t* value))LookupSymbol(Handle, "TangoConfig_getInt32");
	API->TangoConfig_getInt64 = (TangoErrorType(*)(TangoConfig config, const char* key,
		int64_t* value))LookupSymbol(Handle, "TangoConfig_getInt64");
	API->TangoConfig_getDouble = (TangoErrorType(*)(TangoConfig config, const char* key,
		double* value))LookupSymbol(Handle, "TangoConfig_getDouble");
	API->TangoConfig_getString = (TangoErrorType(*)(TangoConfig config, const char* key,
		char* value, size_t size))LookupSymbol(Handle, "TangoConfig_getString");
	API->TangoService_Experimental_connectTextureIdUnity = (TangoErrorType(*)(
		TangoCameraId id, unsigned int texture_y, unsigned int texture_Cb,
		unsigned int texture_Cr, void* context,
		void(*callback)(void*, TangoCameraId)))LookupSymbol(Handle, "TangoService_Experimental_connectTextureIdUnity");
	API->TangoService_Experimental_getDatasetUUIDs = (TangoErrorType(*)(
		TangoUUID** dataset_uuids, int* num_dataset_uuids))LookupSymbol(Handle, "TangoService_Experimental_getDatasetUUIDs");
	API->TangoService_Experimental_releaseDatasetUUIDs = (TangoErrorType(*)(
		TangoUUID** dataset_uuids))LookupSymbol(Handle, "TangoService_Experimental_releaseDatasetUUIDs");
	API->TangoService_Experimental_deleteDataset = (TangoErrorType(*)(
		const TangoUUID dataset_uuid))LookupSymbol(Handle, "TangoService_Experimental_deleteDataset");
	API->TangoService_Experimental_getCurrentDatasetUUID = (TangoErrorType(*)(
		TangoUUID* dataset_uuid))LookupSymbol(Handle, "TangoService_Experimental_getCurrentDatasetUUID");
}


TangoService_API* GetTheTangoServiceAPI()
{
	return GetOrInitTheTangoServiceAPI();
}
#endif
