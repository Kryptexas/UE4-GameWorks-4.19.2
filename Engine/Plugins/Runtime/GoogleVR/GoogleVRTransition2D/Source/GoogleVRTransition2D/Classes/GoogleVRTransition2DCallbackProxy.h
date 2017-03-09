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
#include "Object.h"
#include "UObject/ObjectMacros.h" 
#include "UObject/ScriptMacros.h"
#include "Delegates/Delegate.h"
#include "GoogleVRTransition2DCallbackProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGoogleVRTransition2DDelegate);

UCLASS()
class UGoogleVRTransition2DCallbackProxy : public UObject
{
	GENERATED_BODY()
public:
	// delegate to handle the completion of the 2D transition
	UPROPERTY(BLueprintAssignable, Category="Google VR")
	FGoogleVRTransition2DDelegate OnTransitionTo2D;

public:
	static UGoogleVRTransition2DCallbackProxy *GetInstance();
};
