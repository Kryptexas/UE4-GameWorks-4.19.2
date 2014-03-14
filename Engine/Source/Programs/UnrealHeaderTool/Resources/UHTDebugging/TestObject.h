#pragma once

#include "TestObject.generated.h"

UCLASS()
class UTestObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category="Random")
	void TestForNullPtrDefaults(UObject* Obj1 = NULL, UObject* Obj2 = nullptr, UObject* Obj3 = 0);
};
