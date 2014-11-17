#pragma once

#include "TestInterface.generated.h"

UINTERFACE()
class UTestInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ITestInterface
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent)
	FString SomeFunction(int32 Val) const;
};
