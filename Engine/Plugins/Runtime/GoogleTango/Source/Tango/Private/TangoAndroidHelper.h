// Copyright 2017 Google Inc.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#endif

/** Wrappers for accessing Tango stuff that lives in Java. */
class FTangoAndroidHelper
{
public:
	/**
	 * Initiates binding to the Tango core service (which is an async operation).
	 * @return True if the binding was successfuly initiated, false otherwise.
	 */
	static bool InitiateTangoServiceBind();

	/**
	 * Explicitly unbind from the tango service.
	 */
	static void UnbindTangoService();

	/**
	 * Get Andriod display orientation as per the android.view.Display class' getRotation() method.
	 */
	static int32 GetDisplayRotation();

	/**
	 * Get Android camera orientation as per the android.hardware.Camera.CameraInfo class' orientation field.
	 */
	static int32 GetColorCameraRotation();

	static bool HasAreaDescriptionPermission();
	static void RequestAreaDescriptionPermission();

	static void ExportAreaDescription(const FString& UUID, const FString& Filename);
	static void ImportAreaDescription(const FString& Filename);

#if PLATFORM_ANDROID
	// Helpers for redirecting Android events.
	static void OnApplicationCreated();
	static void OnApplicationDestroyed();
	static void OnApplicationPause();
	static void OnApplicationResume();
	static void OnApplicationStop();
	static void OnApplicationStart();
	static void OnTangoServiceConnect(JNIEnv * jni, jobject iBinder);
	static void OnTangoServiceDisconnect();
	static void OnAreaDescriptionPermissionResult(bool bWasGranted);

	static void OnAreaDescriptionExportResult(bool bWasSuccessful);
	static void OnAreaDescriptionImportResult(bool bWasSuccessful);
#endif

	static bool IsTangoCorePresent();
	static bool IsTangoCoreUpToDate();
};
