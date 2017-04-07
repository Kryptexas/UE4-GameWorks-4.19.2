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

#include "Classes/GoogleVRTransition2DCallbackProxy.h"
#include "CoreMinimal.h"
#include "LogMacros.h"
#include "GoogleVRTransition2D.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

static UGoogleVRTransition2DCallbackProxy *pProxy = NULL;

UGoogleVRTransition2DCallbackProxy *UGoogleVRTransition2DCallbackProxy::GetInstance()
{
	if (!pProxy) {
		pProxy = NewObject<UGoogleVRTransition2DCallbackProxy>();
		pProxy->AddToRoot();

	}
	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("UGoogleVRTransition2DCallbackProxy::GetInstance"));
	return pProxy;
}


#if PLATFORM_ANDROID
JNI_METHOD void Java_com_google_vr_sdk_samples_transition_GVRTransitionHelper_onTransitionTo2D(JNIEnv *env, jclass clazz, jobject thiz)
{
	if (!pProxy) return;

	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("GVRTransitionHelper_onTransitionTo2D, Broadcasting..."));
	pProxy->OnTransitionTo2D.Broadcast();
}
#endif
