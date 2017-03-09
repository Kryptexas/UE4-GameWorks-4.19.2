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

#include "AndroidPermissionCallbackProxy.h"
#include "AndroidPermission.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

static UAndroidPermissionCallbackProxy *pProxy = NULL;

UAndroidPermissionCallbackProxy *UAndroidPermissionCallbackProxy::GetInstance()
{
	if (!pProxy) {
		pProxy = NewObject<UAndroidPermissionCallbackProxy>();
		pProxy->AddToRoot();

	}
	UE_LOG(LogAndroidPermission, Log, TEXT("UAndroidPermissionCallbackProxy::GetInstance"));
	return pProxy;
}

#if PLATFORM_ANDROID
extern "C" void Java_com_google_vr_sdk_samples_permission_PermissionHelper_onAcquirePermissions(JNIEnv *env, jclass clazz, jobjectArray permissions, jintArray grantResults) 
{
	if (!pProxy) return;

	TArray<FString> arrPermissions;
	TArray<bool> arrGranted;
	int num = env->GetArrayLength(permissions);
	jint* jarrGranted = env->GetIntArrayElements(grantResults, 0);
	for (int i = 0; i < num; i++) {
		jstring str = (jstring)env->GetObjectArrayElement(permissions, i);
		const char* charStr = env->GetStringUTFChars(str, 0);
		arrPermissions.Add(FString(UTF8_TO_TCHAR(charStr)));
		env->ReleaseStringUTFChars(str, charStr);

		arrGranted.Add(jarrGranted[i] == 0 ? true : false); // 0: permission granted, -1: permission denied
	}
	env->ReleaseIntArrayElements(grantResults, jarrGranted, 0);

	UE_LOG(LogAndroidPermission, Log, TEXT("PermissionHelper_onAcquirePermissions %s %d (%d), Broadcasting..."),
		*(arrPermissions[0]), arrGranted[0], num);
	pProxy->OnPermissionsGranted.Broadcast(arrPermissions, arrGranted);
}
#endif
