// Copyright 2017 Google Inc.

#include "TangoAndroidHelper.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#endif

#include "TangoLifecycle.h"

#if PLATFORM_ANDROID
extern "C"
{
	// Functions that are called on Android lifecycle events.

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationCreated(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationCreated();
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationDestroyed(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationDestroyed();
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationPause(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationPause();
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationResume(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationResume();
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onAreaDescriptionPermissionResult(JNIEnv*, jobject, jboolean bWasGranted)
	{
		FTangoAndroidHelper::OnAreaDescriptionPermissionResult(bWasGranted);
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onAreaDescriptionExportResult(JNIEnv*, jobject, jboolean bSucceeded)
	{
		FTangoAndroidHelper::OnAreaDescriptionExportResult(bSucceeded);
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onAreaDescriptionImportResult(JNIEnv*, jobject, jboolean bSucceeded)
	{
		FTangoAndroidHelper::OnAreaDescriptionImportResult(bSucceeded);
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationStop(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationStop();
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_onApplicationStart(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnApplicationStart();
	}

	// Functions for handling Tango service connect/disconnect events.

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_reportTangoServiceConnect(JNIEnv * jni, jobject, jobject iBinder)
	{
		FTangoAndroidHelper::OnTangoServiceConnect(jni, iBinder);
	}

	void Java_com_projecttango_unreal_TangoNativeEngineMethodWrapper_reportTangoServiceDisconnect(JNIEnv*, jobject)
	{
		FTangoAndroidHelper::OnTangoServiceDisconnect();
	}
}

// Redirect events to TangoLifecycle class:

void FTangoAndroidHelper::OnApplicationCreated()
{
	FTangoDevice::GetInstance()->OnApplicationCreated();
}

void FTangoAndroidHelper::OnApplicationDestroyed()
{
	FTangoDevice::GetInstance()->OnApplicationDestroyed();
}

void FTangoAndroidHelper::OnApplicationPause()
{
	FTangoDevice::GetInstance()->OnApplicationPause();
}

void FTangoAndroidHelper::OnApplicationStart()
{
	FTangoDevice::GetInstance()->OnApplicationStart();
}

void FTangoAndroidHelper::OnApplicationStop()
{
	FTangoDevice::GetInstance()->OnApplicationStop();
}

void FTangoAndroidHelper::OnApplicationResume()
{
	FTangoDevice::GetInstance()->OnApplicationResume();
}

void FTangoAndroidHelper::OnTangoServiceConnect(JNIEnv * jni, jobject iBinder)
{
	FTangoDevice::GetInstance()->OnTangoServiceBound(jni, iBinder);
}

void FTangoAndroidHelper::OnTangoServiceDisconnect()
{
	FTangoDevice::GetInstance()->OnTangoServiceUnbound();
}

void FTangoAndroidHelper::OnAreaDescriptionPermissionResult(bool bWasGranted)
{
	FTangoDevice::GetInstance()->OnAreaDescriptionPermissionResult(bWasGranted);
}

#endif
bool FTangoAndroidHelper::HasAreaDescriptionPermission()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_TangoHasAreaDescriptionPermission", "()Z", false);
		return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
	return false;
}

void FTangoAndroidHelper::RequestAreaDescriptionPermission()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_TangoRequestAreaDescriptionPermission", "()V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
}

bool FTangoAndroidHelper::InitiateTangoServiceBind()
{
	#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_InitiateTangoServiceBind", "()Z", false);
		return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
	#endif

	return false;
}

void FTangoAndroidHelper::UnbindTangoService()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnbindTangoService", "()V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
}

int32 FTangoAndroidHelper::GetDisplayRotation()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetDisplayRotation", "()I", false);
		return FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
	return 0;
}

int32 FTangoAndroidHelper::GetColorCameraRotation()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetColorCameraRotation", "()I", false);
		return FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
	return 0;
}

void FTangoAndroidHelper::ExportAreaDescription(const FString& UUID, const FString& Filename)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_TangoRequestAreaDescriptionExport", "(Ljava/lang/String;Ljava/lang/String;)V", false);
		jstring uuid = Env->NewStringUTF(TCHAR_TO_UTF8(*UUID));
		jstring filename = Env->NewStringUTF(TCHAR_TO_UTF8(*Filename));
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, uuid, filename);
		Env->DeleteLocalRef(uuid);
		Env->DeleteLocalRef(filename);
	}
#endif
}

void FTangoAndroidHelper::ImportAreaDescription(const FString& Filename)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_TangoRequestAreaDescriptionImport", "(Ljava/lang/String;)V", false);
		jstring filename = Env->NewStringUTF(TCHAR_TO_UTF8(*Filename));
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, filename);
		Env->DeleteLocalRef(filename);
	}
#endif
}

#if PLATFORM_ANDROID
void FTangoAndroidHelper::OnAreaDescriptionExportResult(bool bWasSuccessful)
{
	FTangoDevice::GetInstance()->RunOnGameThread([=]()->void {
		FTangoLocalAreaLearningEvent Event;
		Event.EventType = ETangoLocalAreaLearningEventType::EXPORT_RESULT;
		Event.EventValue = bWasSuccessful ? 1 : 0;
		FTangoDevice::GetInstance()->TangoLocalAreaLearningEventDelegate.Broadcast(Event);
	});
}

void FTangoAndroidHelper::OnAreaDescriptionImportResult(bool bWasSuccessful)
{
	FTangoDevice::GetInstance()->RunOnGameThread([=]()->void {
		FTangoLocalAreaLearningEvent Event;
		Event.EventType = ETangoLocalAreaLearningEventType::IMPORT_RESULT;
		Event.EventValue = bWasSuccessful ? 1 : 0;
		FTangoDevice::GetInstance()->TangoLocalAreaLearningEventDelegate.Broadcast(Event);
	});
}

#endif

bool FTangoAndroidHelper::IsTangoCorePresent()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsTangoCorePresent", "()Z", false);
		return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
	return false;
#else
	return true;
#endif
}

bool FTangoAndroidHelper::IsTangoCoreUpToDate()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsTangoCoreUpToDate", "()Z", false);
		return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
	return false;
#else
	return true;
#endif
}