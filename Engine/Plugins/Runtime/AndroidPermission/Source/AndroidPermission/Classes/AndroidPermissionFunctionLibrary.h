/* Copyright 2016 Google Inc. All rights reserved.
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

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndroidPermissionFunctionLibrary.generated.h"

UCLASS()
class ANDROIDPERMISSION_API UAndroidPermissionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** check if the permission is already granted */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Check Android Permission"), Category="AndroidPermission")
	static bool CheckPermission(const FString& permission);

	/** try to acquire permissions and return a singleton callback proxy object containing OnPermissionsGranted delegate */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Request Android Permissions"), Category="AndroidPermission")
	static UAndroidPermissionCallbackProxy* AcquirePermissions(const TArray<FString>& permissions);

public:
	/** initialize java objects and cache them for further usage. called when the module is loaded */
	static void Initialize();
};
