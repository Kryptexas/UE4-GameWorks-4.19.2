// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PyTest.generated.h"

/**
 * Delegate to allow testing of the various script delegate features that are exposed to Python wrapped types.
 */
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(int32, FPyTestDelegate, int32, InValue);

/**
 * Multicast delegate to allow testing of the various script delegate features that are exposed to Python wrapped types.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPyTestMulticastDelegate, FString, InStr);

/**
 * Enum to allow testing of the various UEnum features that are exposed to Python wrapped types.
 */
UENUM(BlueprintType)
enum class EPyTestEnum : uint8
{
	One,
	Two,
};

/**
 * Struct to allow testing of the various UStruct features that are exposed to Python wrapped types.
 */
USTRUCT(BlueprintType)
struct FPyTestStruct
{
	GENERATED_BODY()

public:
	FPyTestStruct();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	int32 Int;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	float Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	EPyTestEnum Enum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TArray<FString> StringArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TSet<FString> StringSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TMap<FString, int32> StringIntMap;
};

/**
 * Struct to allow testing of inheritance on Python wrapped types.
 */
USTRUCT(BlueprintType)
struct FPyTestChildStruct : public FPyTestStruct
{
	GENERATED_BODY()
};

/**
 * Object to allow testing of the various UObject features that are exposed to Python wrapped types.
 */
UCLASS(Blueprintable)
class UPyTestObject : public UObject
{
	GENERATED_BODY()

public:
	UPyTestObject();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	int32 Int;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	float Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	EPyTestEnum Enum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TArray<FString> StringArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TSet<FString> StringSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TMap<FString, int32> StringIntMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FPyTestDelegate Delegate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FPyTestMulticastDelegate MulticastDelegate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FPyTestStruct Struct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	TArray<FPyTestStruct> StructArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Python)
	FPyTestChildStruct ChildStruct;

	UFUNCTION(BlueprintImplementableEvent, Category = Python)
	int32 FuncBlueprintImplementable(const int32 InValue) const;

	UFUNCTION(BlueprintNativeEvent, Category = Python)
	int32 FuncBlueprintNative(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	int32 CallFuncBlueprintImplementable(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	int32 CallFuncBlueprintNative(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	void FuncTakingPyTestStruct(const FPyTestStruct& InStruct) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	void FuncTakingPyTestChildStruct(const FPyTestChildStruct& InStruct) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	int32 FuncTakingPyTestDelegate(const FPyTestDelegate& InDelegate, const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	int32 DelegatePropertyCallback(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = Python)
	void MulticastDelegatePropertyCallback(FString InStr) const;
};

/**
 * Object to allow testing of inheritance on Python wrapped types.
 */
UCLASS(Blueprintable)
class UPyTestChildObject : public UPyTestObject
{
	GENERATED_BODY()
};
