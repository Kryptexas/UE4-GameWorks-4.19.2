// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AndroidJavaWebBrowser.h"
#include "AndroidApplication.h"

#if UE_BUILD_SHIPPING
// always clear any exceptions in SHipping
#define CHECK_JNI_RESULT(Id) if (Id == 0) { JEnv->ExceptionClear(); }
#else
#define CHECK_JNI_RESULT(Id) \
if (Id == 0) \
{ \
	if (bIsOptional) { JEnv->ExceptionClear(); } \
	else { JEnv->ExceptionDescribe(); checkf(Id != 0, TEXT("Failed to find " #Id)); } \
}
#endif

static jfieldID FindField(JNIEnv* JEnv, jclass Class, const ANSICHAR* FieldName, const ANSICHAR* FieldType, bool bIsOptional)
{
	jfieldID Field = Class == NULL ? NULL : JEnv->GetFieldID(Class, FieldName, FieldType);
	CHECK_JNI_RESULT(Field);
	return Field;
}

FJavaAndroidWebBrowser::FJavaAndroidWebBrowser(bool swizzlePixels, bool vulkanRenderer, int32 width, int32 height,
	jlong widgetPtr, bool bEnableRemoteDebugging, bool bUseTransparency)
	: FJavaClassObject(GetClassName(), "(JIIZZZZ)V", widgetPtr, width, height,  swizzlePixels, vulkanRenderer, bEnableRemoteDebugging, bUseTransparency)
	, ReleaseMethod(GetClassMethod("release", "()V"))
	, GetVideoLastFrameDataMethod(GetClassMethod("getVideoLastFrameData", "()Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, GetVideoLastFrameMethod(GetClassMethod("getVideoLastFrame", "(I)Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, DidResolutionChangeMethod(GetClassMethod("didResolutionChange", "()Z"))
	, UpdateVideoFrameMethod(GetClassMethod("updateVideoFrame", "(I)Lcom/epicgames/ue4/WebViewControl$FrameUpdateInfo;"))
	, UpdateMethod(GetClassMethod("Update", "(IIII)V"))
	, ExecuteJavascriptMethod(GetClassMethod("ExecuteJavascript", "(Ljava/lang/String;)V"))
	, LoadURLMethod(GetClassMethod("LoadURL", "(Ljava/lang/String;)V"))
	, LoadStringMethod(GetClassMethod("LoadString", "(Ljava/lang/String;Ljava/lang/String;)V"))
	, StopLoadMethod(GetClassMethod("StopLoad", "()V"))
	, ReloadMethod(GetClassMethod("Reload", "()V"))
	, CloseMethod(GetClassMethod("Close", "()V"))
	, GoBackOrForwardMethod(GetClassMethod("GoBackOrForward", "(I)V"))
	, SetAndroid3DBrowserMethod(GetClassMethod("SetAndroid3DBrowser", "(Z)V"))
{
	VideoTexture = nullptr;
	bVideoTextureValid = false;

	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();

	// get field IDs for FrameUpdateInfo class members
	jclass localFrameUpdateInfoClass = FAndroidApplication::FindJavaClass("com/epicgames/ue4/WebViewControl$FrameUpdateInfo");
	FrameUpdateInfoClass = (jclass)JEnv->NewGlobalRef(localFrameUpdateInfoClass);
	JEnv->DeleteLocalRef(localFrameUpdateInfoClass);
	FrameUpdateInfo_Buffer = FindField(JEnv, FrameUpdateInfoClass, "Buffer", "Ljava/nio/Buffer;", false);
	FrameUpdateInfo_FrameReady = FindField(JEnv, FrameUpdateInfoClass, "FrameReady", "Z", false);
	FrameUpdateInfo_RegionChanged = FindField(JEnv, FrameUpdateInfoClass, "RegionChanged", "Z", false);
}

void FJavaAndroidWebBrowser::Release()
{
	CallMethod<void>(ReleaseMethod);
}

bool FJavaAndroidWebBrowser::GetVideoLastFrameData(void* & outPixels, int64 & outCount, bool *bRegionChanged)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = JEnv->CallObjectMethod(Object, GetVideoLastFrameDataMethod.Method);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		if (nullptr != Result)
		{
			JEnv->DeleteLocalRef(Result);
		}
		*bRegionChanged = false;
		return false;
	}

	if (nullptr == Result)
	{
		return false;
	}

	jobject buffer = JEnv->GetObjectField(Result, FrameUpdateInfo_Buffer);
	if (nullptr != buffer)
	{
		bool bFrameReady = (bool)JEnv->GetBooleanField(Result, FrameUpdateInfo_FrameReady);
		*bRegionChanged = (bool)JEnv->GetBooleanField(Result, FrameUpdateInfo_RegionChanged);
		
		outPixels = JEnv->GetDirectBufferAddress(buffer);
		outCount = JEnv->GetDirectBufferCapacity(buffer);

		// the GetObjectField returns a local ref, but Java will still own the real buffer
		JEnv->DeleteLocalRef(buffer);

		JEnv->DeleteLocalRef(Result);

		return !(nullptr == outPixels || 0 == outCount);
	}

	JEnv->DeleteLocalRef(Result);
	return false;
}

bool FJavaAndroidWebBrowser::DidResolutionChange()
{
	return CallMethod<bool>(DidResolutionChangeMethod);
}

bool FJavaAndroidWebBrowser::UpdateVideoFrame(int32 ExternalTextureId, bool *bRegionChanged)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = JEnv->CallObjectMethod(Object, UpdateVideoFrameMethod.Method, ExternalTextureId);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		if (nullptr != Result)
		{
			JEnv->DeleteLocalRef(Result);
		}
		*bRegionChanged = false;
		return false;
	}

	if (nullptr == Result)
	{
		*bRegionChanged = false;
		return false;
	}

	bool bFrameReady = (bool)JEnv->GetBooleanField(Result, FrameUpdateInfo_FrameReady);
	*bRegionChanged = (bool)JEnv->GetBooleanField(Result, FrameUpdateInfo_RegionChanged);
	
	JEnv->DeleteLocalRef(Result);

	return bFrameReady;
}

bool FJavaAndroidWebBrowser::GetVideoLastFrame(int32 destTexture)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = JEnv->CallObjectMethod(Object, GetVideoLastFrameMethod.Method, destTexture);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		return false;
	}

	if (nullptr == Result)
	{
		return false;
	}

	bool bFrameReady = (bool)JEnv->GetBooleanField(Result, FrameUpdateInfo_FrameReady);
	
	JEnv->DeleteLocalRef(Result);

	return bFrameReady;
}

FName FJavaAndroidWebBrowser::GetClassName()
{
	return FName("com/epicgames/ue4/WebViewControl");
}

void FJavaAndroidWebBrowser::Update(const int posX, const int posY, const int sizeX, const int sizeY)
{
	CallMethod<void>(UpdateMethod, posX, posY, sizeX, sizeY);
}

void FJavaAndroidWebBrowser::ExecuteJavascript(const FString& Script)
{
	CallMethod<void>(ExecuteJavascriptMethod, FJavaClassObject::GetJString(Script));
}

void FJavaAndroidWebBrowser::LoadURL(const FString& NewURL)
{
	CallMethod<void>(LoadURLMethod, FJavaClassObject::GetJString(NewURL));
}

void FJavaAndroidWebBrowser::LoadString(const FString& Contents, const FString& BaseUrl)
{
	CallMethod<void>(LoadStringMethod, FJavaClassObject::GetJString(Contents), FJavaClassObject::GetJString(BaseUrl));
}

void FJavaAndroidWebBrowser::StopLoad()
{
	CallMethod<void>(StopLoadMethod);
}

void FJavaAndroidWebBrowser::Reload()
{
	CallMethod<void>(ReloadMethod);
}

void FJavaAndroidWebBrowser::Close()
{
	CallMethod<void>(CloseMethod);
}

void FJavaAndroidWebBrowser::GoBack()
{
	CallMethod<void>(GoBackOrForwardMethod, -1);
}

void FJavaAndroidWebBrowser::GoForward()
{
	CallMethod<void>(GoBackOrForwardMethod, 1);
}

void FJavaAndroidWebBrowser::SetAndroid3DBrowser(bool InIsAndroid3DBrowser)
{
	CallMethod<void>(SetAndroid3DBrowserMethod, InIsAndroid3DBrowser);
}


