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

#include "GoogleVRTransition2DBPLibrary.h"
#include "Classes/GoogleVRTransition2DCallbackProxy.h"
#include "GoogleVRTransition2D.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
static jclass _TransitionHelperClass;
static jmethodID _TransitionTo2DMethodId;
static jmethodID _TransitionToVRMethodId;
#endif

void UGoogleVRTransition2DBPLibrary::Initialize()
{
#if PLATFORM_ANDROID
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	_TransitionHelperClass = (jclass)env->NewGlobalRef(FAndroidApplication::FindJavaClass("com/google/vr/sdk/samples/transition/GVRTransitionHelper"));
	_TransitionTo2DMethodId = env->GetStaticMethodID(_TransitionHelperClass, "transitionTo2D", "(Landroid/app/Activity;)V");
	_TransitionToVRMethodId = env->GetStaticMethodID(_TransitionHelperClass, "transitionToVR", "()V");
#endif
}

UGoogleVRTransition2DBPLibrary::UGoogleVRTransition2DBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UGoogleVRTransition2DCallbackProxy* UGoogleVRTransition2DBPLibrary::TransitionTo2D()
{
#if PLATFORM_ANDROID
	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("UGoogleVRTransition2DBPLibrary::transitionTo2D"));
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	env->CallStaticVoidMethod(_TransitionHelperClass, _TransitionTo2DMethodId, FJavaWrapper::GameActivityThis);
	return UGoogleVRTransition2DCallbackProxy::GetInstance();
#else
	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("UGoogleVRTransition2DBPLibrary::transitionTo2D"));
	return UGoogleVRTransition2DCallbackProxy::GetInstance();
#endif
}

void UGoogleVRTransition2DBPLibrary::TransitionToVR()
{
#if PLATFORM_ANDROID
	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("UGoogleVRTransition2DBPLibrary::TransitionToVR"));
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	env->CallStaticVoidMethod(_TransitionHelperClass, _TransitionToVRMethodId);
#else
	UE_LOG(LogGoogleVRTransition2D, Log, TEXT("UGoogleVRTransition2DBPLibrary::TransitionToVR"));
#endif
}
