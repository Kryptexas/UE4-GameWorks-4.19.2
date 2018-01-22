// Copyright 2017 Google Inc.

#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#endif

/** Wrappers for accessing Android Java stuff */
class FGoogleARCoreAndroidHelper
{
public:

	/**
	 * Update the Android display orientation as per the android.view.Display class' getRotation() method.
	 */
	static void UpdateDisplayRotation();

	/**
	 * Get Andriod display orientation.
	 */
	static int32 GetDisplayRotation();

	static void QueueStartSessionOnUiThread();

#if PLATFORM_ANDROID
	// Helpers for redirecting Android events.
	static void OnApplicationCreated();
	static void OnApplicationDestroyed();
	static void OnApplicationPause();
	static void OnApplicationResume();
	static void OnApplicationStop();
	static void OnApplicationStart();
	static void OnDisplayOrientationChanged();
#endif

private:
	static int32 CurrentDisplayRotation;
};
