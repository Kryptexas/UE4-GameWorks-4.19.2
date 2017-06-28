/* Copyright 2017 Google Inc.
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

#include "TangoAndroidPermissionHandler.h"
#include "AndroidPermissionFunctionLibrary.h"
#include "AndroidPermissionCallbackProxy.h"
#include "TangoLifecycle.h"

UTangoAndroidPermissionHandler::UTangoAndroidPermissionHandler(const FObjectInitializer& Init) : Super(Init)
{
}

bool UTangoAndroidPermissionHandler::CheckRuntimePermission(const FString& RuntimePermission)
{
	return UAndroidPermissionFunctionLibrary::CheckPermission(RuntimePermission);
}

void UTangoAndroidPermissionHandler::RequestRuntimePermissions(const TArray<FString>& RuntimePermissions)
{
	UAndroidPermissionCallbackProxy::GetInstance()->OnPermissionsGrantedDynamicDelegate.AddDynamic(this, &UTangoAndroidPermissionHandler::OnPermissionsGranted);
	UAndroidPermissionFunctionLibrary::AcquirePermissions(RuntimePermissions);
}

void UTangoAndroidPermissionHandler::OnPermissionsGranted(const TArray<FString> &Permissions, const TArray<bool>& Granted)
{
	UAndroidPermissionCallbackProxy::GetInstance()->OnPermissionsGrantedDynamicDelegate.RemoveDynamic(this, &UTangoAndroidPermissionHandler::OnPermissionsGranted);
	FTangoDevice::GetInstance()->HandleRuntimePermissionsGranted(Permissions, Granted);
}

