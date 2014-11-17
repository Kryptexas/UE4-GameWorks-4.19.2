#pragma once

#include "TestInterfaceObject.generated.h"

UCLASS()
class UTestInterfaceObject : public UObject, public ITestInterface
{
	GENERATED_UCLASS_BODY()

//	UFUNCTION(BlueprintNativeEvent)
	FString SomeFunction(int32 Val) const;
};
