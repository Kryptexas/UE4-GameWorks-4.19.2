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
#include "Delegates/Delegate.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "GoogleVRControllerEventManager.generated.h"

UENUM(BlueprintType)
enum class EGoogleVRControllerState : uint8
{
	Disconnected = 0,
	Scanning = 1,
	Connecting = 2,
	Connected = 3,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGoogleVRControllerRecenterDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoogleVRControllerStateChangeDelegate, EGoogleVRControllerState, NewControllerState);

/**
 * GoogleVRController Extensions Function Library
 */
UCLASS()
class GOOGLEVRCONTROLLER_API UGoogleVRControllerEventManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FGoogleVRControllerRecenterDelegate OnControllerRecenteredDelegate;

	UPROPERTY(BlueprintAssignable)
	FGoogleVRControllerStateChangeDelegate OnControllerStateChangedDelegate;

public:
	static UGoogleVRControllerEventManager* GetInstance();
};