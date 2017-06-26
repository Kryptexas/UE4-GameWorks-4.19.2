// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

// #include "UObject/Script.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "UObject/Package.h"

FAutoConsoleCommandWithWorldAndArgs GScriptFuzzingCommand(
	TEXT("Test.ScriptFuzzing"),
	TEXT("Fuzzes the script exposed API of engine classes"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
		[](const TArray<FString>& Params, UWorld* World)
{
	FEditorScriptExecutionGuard AllowScriptExec;

	// Run thru all native classes
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* TestClass = *ClassIt;
		if (TestClass->HasAnyClassFlags(CLASS_Native))
		{
			//@TODO: Need to fuzz these too, but they do lots of terrible things right now
			if (TestClass->GetOutermost()->GetName().Contains(TEXT("UnrealEd")))
			{
				continue;
			}

			// Determine if it has any script surface area
			bool bHasFunctions = false;
			bool bHasNonStaticFunctions = false;
			for (TFieldIterator<UFunction> FuncIt(TestClass); FuncIt; ++FuncIt)
			{
				UFunction* Function = *FuncIt;
				if (Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
				{
					bHasFunctions = true;
					if (!Function->HasAnyFunctionFlags(FUNC_Static))
					{
						bHasNonStaticFunctions = true;
					}
				}
			}

			if (bHasFunctions)
			{
				// Create an instance of the object if necessary (function libraries can use the CDO instead)
				UObject* CreatedInstance = nullptr;
				if (bHasNonStaticFunctions)
				{
					if (!TestClass->HasAnyClassFlags(CLASS_Abstract))
					{
						if (TestClass->IsChildOf(AActor::StaticClass()))
						{
							if (TestClass->HasAnyClassFlags(CLASS_NotPlaceable))
							{
								UE_LOG(LogTemp, Warning, TEXT("Cannot fuzz non-static methods in %s%s as it is marked NotPlaceable"), TestClass->GetPrefixCPP(), *TestClass->GetName());
							}
							else
							{
								FActorSpawnParameters SpawnParams;
								SpawnParams.bNoFail = true;
								SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
								CreatedInstance = World->SpawnActor<AActor>(TestClass, FTransform::Identity, SpawnParams);
							}
						}
						else
						{
							if (TestClass->ClassWithin != nullptr)
							{
								UE_LOG(LogTemp, Warning, TEXT("Cannot fuzz non-static methods in %s%s, no support for ClassWithin != nullptr yet"), TestClass->GetPrefixCPP(), *TestClass->GetName());
							}
							else
							{
								CreatedInstance = NewObject<UObject>(GetTransientPackage(), TestClass);
							}
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Cannot fuzz non-static methods in %s%s as it is marked Abstract; we might be able to in derived classes"), TestClass->GetPrefixCPP(), *TestClass->GetName());
					}
				}

				// Run thru all functions and fuzz them
				for (TFieldIterator<UFunction> FuncIt(TestClass); FuncIt; ++FuncIt)
				{
					UFunction* Function = *FuncIt;
					if (Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
					{
						UObject* TestInstance = Function->HasAnyFunctionFlags(FUNC_Static) ? TestClass->GetDefaultObject() : CreatedInstance;
						
						if (TestInstance != nullptr)
						{
							UE_LOG(LogTemp, Log, TEXT("Fuzzing %s%s::%s() on %s"), TestClass->GetPrefixCPP(), *TestClass->GetName(), *Function->GetName(), *TestInstance->GetName());

							// Build a permutation matrix
							//@TODO: Do that, right now we're just testing all empty values

							FString CallStr = FString::Printf(TEXT("%s"), *Function->GetName());

							TestInstance->CallFunctionByNameWithArguments(*CallStr, *GLog, nullptr, /*bForceCallWithNonExec=*/ true);
						}
						else
						{
							if (!TestClass->HasAnyClassFlags(CLASS_Abstract))
							{
								UE_LOG(LogTemp, Warning, TEXT("Failed to fuzz %s%s::%s() because we could not make an object to test it on"), TestClass->GetPrefixCPP(), *TestClass->GetName(), *Function->GetName());
							}
						}
					}
				}

				// Tear down the created instance
				if (CreatedInstance != nullptr)
				{
					if (TestClass->IsChildOf(AActor::StaticClass()))
					{
						World->DestroyActor(CastChecked<AActor>(CreatedInstance));
						CreatedInstance = nullptr;
					}
				}
			}
		}
	}
}));
