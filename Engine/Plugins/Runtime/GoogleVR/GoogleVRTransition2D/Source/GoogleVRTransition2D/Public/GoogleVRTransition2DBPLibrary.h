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

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
//#include "Engine/EngineTypes.h"
#include "GoogleVRTransition2DBPLibrary.generated.h"

UCLASS()
class UGoogleVRTransition2DBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//transition to 2D with the visual guidance. a black 2D screen will be displayed after transition
	//this function returns a singleton callback proxy object to handle OnTransition2D delegate
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Transition to 2D"), Category = "Google VR")
	static UGoogleVRTransition2DCallbackProxy* TransitionTo2D();

	//transition back from 2D to VR. will see Back to VR button which make you resume to game by clicking it
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Transition back to VR"), Category = "Google VR")
	static void TransitionToVR();

public:
	//intitialize and caches java classes and methods when the module is loaded
	static void Initialize();
};
