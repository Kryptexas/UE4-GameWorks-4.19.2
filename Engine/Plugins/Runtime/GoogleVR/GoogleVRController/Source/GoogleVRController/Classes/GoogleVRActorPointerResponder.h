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
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "GoogleVRActorPointerResponder.generated.h"

class UGoogleVRPointerInputComponent;

/**
 * IGoogleVRActorPointerResponder is an interface for an Actor
 * to respond to pointer input events from UGoogleVRPointerInputComponent.
 */
UINTERFACE(BlueprintType)
class UGoogleVRActorPointerResponder : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class GOOGLEVRCONTROLLER_API IGoogleVRActorPointerResponder
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerEnter(const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerExit(AActor* PreviousActor, const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerHover(const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerComponentChanged(UPrimitiveComponent* PreviousComponent, const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerClick(const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerPressed(const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="PointerResponder")
	void OnPointerReleased(const FHitResult& HitResult, UGoogleVRPointerInputComponent* Source);
};
