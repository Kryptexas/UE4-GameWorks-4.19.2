// Copyright 2017 Google Inc.

#include "GoogleARCoreAndroidHelper.h"
#include "GoogleARCoreBaseLogCategory.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#endif

#include "GoogleARCoreDevice.h"

#if PLATFORM_ANDROID
extern "C"
{
	// Functions that are called on Android lifecycle events.

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationCreated(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationCreated();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationDestroyed(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationDestroyed();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationPause(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationPause();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationResume(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationResume();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationStop(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationStop();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onApplicationStart(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnApplicationStart();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_onDisplayOrientationChanged(JNIEnv*, jobject)
	{
		FGoogleARCoreAndroidHelper::OnDisplayOrientationChanged();
	}

	void Java_com_google_arcore_unreal_GoogleARCoreJavaHelper_ARCoreSessionStart(JNIEnv*, jobject)
	{
		FGoogleARCoreDevice::GetInstance()->StartSessionWithRequestedConfig();
	}
}

// Redirect events to FGoogleARCoreDevice class:

void FGoogleARCoreAndroidHelper::OnApplicationCreated()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationCreated();
}

void FGoogleARCoreAndroidHelper::OnApplicationDestroyed()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationDestroyed();
}

void FGoogleARCoreAndroidHelper::OnApplicationPause()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationPause();
}

void FGoogleARCoreAndroidHelper::OnApplicationStart()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationStart();
}

void FGoogleARCoreAndroidHelper::OnApplicationStop()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationStop();
}

void FGoogleARCoreAndroidHelper::OnApplicationResume()
{
	FGoogleARCoreDevice::GetInstance()->OnApplicationResume();
}

void FGoogleARCoreAndroidHelper::OnDisplayOrientationChanged()
{
	FGoogleARCoreDevice::GetInstance()->OnDisplayOrientationChanged();
}

#endif

int32 FGoogleARCoreAndroidHelper::CurrentDisplayRotation = 0;
void FGoogleARCoreAndroidHelper::UpdateDisplayRotation()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetDisplayRotation", "()I", false);
		CurrentDisplayRotation = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
}

int32 FGoogleARCoreAndroidHelper::GetDisplayRotation()
{
	return CurrentDisplayRotation;
}

void FGoogleARCoreAndroidHelper::QueueStartSessionOnUiThread()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_QueueStartSessionOnUiThread", "()V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
}
