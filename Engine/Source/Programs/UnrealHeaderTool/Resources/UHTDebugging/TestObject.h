#pragma once

#include "TestObject.generated.h"

UCLASS()
class UTestObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category="Random")
	void TestForNullPtrDefaults(UObject* Obj1 = NULL, UObject* Obj2 = nullptr, UObject* Obj3 = 0);

#if BLAH
	UFUNCTION() int x; // This should not compile if UHT parses it, which it shouldn't
#elif BLAH2
	UFUNCTION() int x;
#else
	UFUNCTION() int x;
#endif

#ifdef X
	UFUNCTION() int x;
	#ifndef X
		UFUNCTION() int x;
	#elif BLAH4
		UFUNCTION() int x;
	#else
		UFUNCTION() int x;
	#endif
	UFUNCTION() int x;
#elif BLAH3
	UFUNCTION() int x;
	#ifndef X
		UFUNCTION() int x;
	#elif BLAH4
		UFUNCTION() int x;
	#else
		UFUNCTION() int x;
	#endif
	UFUNCTION() int x;
#else
	UFUNCTION() int x;
	#ifndef X
		UFUNCTION() int x;
	#elif BLAH4
		UFUNCTION() int x;
	#else
		UFUNCTION() int x;
	#endif
	UFUNCTION() int x;
#endif

#ifndef X
	UFUNCTION() int x;
#elif BLAH4
	UFUNCTION() int x;
#else
	UFUNCTION() int x;
#endif
};
