#pragma once

// UE4
#include "Factories/Factory.h"

// AppleARKit
#include "AppleARKitCameraTextureFactory.generated.h"

UCLASS()
class UAppleARKitCameraTextureFactory : public UFactory
{
	GENERATED_BODY()

public:

	UAppleARKitCameraTextureFactory();

	// UFactory interface
	virtual FText GetDisplayName() const override;
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) override;
};
