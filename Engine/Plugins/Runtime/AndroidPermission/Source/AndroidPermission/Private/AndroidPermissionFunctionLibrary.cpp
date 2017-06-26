/* Copyright 2016 Google Inc.
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

#include "AndroidPermissionFunctionLibrary.h"
#include "AndroidPermission.h"
#include "AndroidPermissionCallbackProxy.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
static jclass _PermissionHelperClass;
static jmethodID _CheckPermissionMethodId;
static jmethodID _AcquirePermissionMethodId;
#endif

DEFINE_LOG_CATEGORY(LogAndroidPermission);

void UAndroidPermissionFunctionLibrary::Initialize()
{
#if PLATFORM_ANDROID
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	_PermissionHelperClass = (jclass)env->NewGlobalRef(FAndroidApplication::FindJavaClass("com/google/vr/sdk/samples/permission/PermissionHelper"));
	_CheckPermissionMethodId = env->GetStaticMethodID(_PermissionHelperClass, "checkPermission", "(Ljava/lang/String;)Z");
	_AcquirePermissionMethodId = env->GetStaticMethodID(_PermissionHelperClass, "acquirePermissions", "([Ljava/lang/String;)V");
#endif
}

bool UAndroidPermissionFunctionLibrary::CheckPermission(const FString& permission)
{
#if PLATFORM_ANDROID
	UE_LOG(LogAndroidPermission, Log, TEXT("UAndroidPermissionFunctionLibrary::CheckPermission %s (Android)"), *permission);
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	jstring argument = env->NewStringUTF(TCHAR_TO_UTF8(*permission));
	bool bResult = env->CallStaticBooleanMethod(_PermissionHelperClass, _CheckPermissionMethodId, argument);
	env->DeleteLocalRef(argument);
	return bResult;
#else
	UE_LOG(LogAndroidPermission, Log, TEXT("UAndroidPermissionFunctionLibrary::CheckPermission (Else)"));
	return false;
#endif
}

UAndroidPermissionCallbackProxy* UAndroidPermissionFunctionLibrary::AcquirePermissions(const TArray<FString>& permissions)
{
#if PLATFORM_ANDROID
	UE_LOG(LogAndroidPermission, Log, TEXT("UAndroidPermissionFunctionLibrary::AcquirePermissions"));
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	jobjectArray permissionsArray = (jobjectArray)env->NewObjectArray(permissions.Num(), FJavaWrapper::JavaStringClass, NULL);
	for (int i = 0; i < permissions.Num(); i++) {
		jstring str = env->NewStringUTF(TCHAR_TO_UTF8(*(permissions[i])));
		env->SetObjectArrayElement(permissionsArray, i, str);
		env->DeleteLocalRef(str);
	}
	env->CallStaticVoidMethod(_PermissionHelperClass, _AcquirePermissionMethodId, permissionsArray);
	env->DeleteLocalRef(permissionsArray);
	return UAndroidPermissionCallbackProxy::GetInstance();
#else
	UE_LOG(LogAndroidPermission, Log, TEXT("UAndroidPermissionFunctionLibrary::AcquirePermissions(%s) (Android)"), *(FString::Join(permissions, TEXT(","))));
	return UAndroidPermissionCallbackProxy::GetInstance();
#endif
}