// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/BlueprintGeneratedClass.h"
#include "ScriptBlueprintGeneratedClass.generated.h"

/**
* Script-defined field (variable or function)
*/
struct FScriptField
{
	/** Field name */
	FName Name;
	/** Field type */
	UClass* Class;

	FScriptField()
		: Class(NULL)
	{
	}
	FScriptField(FName InName, UClass* InClass)
		: Name(InName)
		, Class(InClass)
	{
	}
};

/** Script context for this component */
class FScriptContextBase
{
public:
	/**
	 * Initializes script context given script code
	 * @param Code Script code
	 * @param Owner Owner of this context
	 */
	virtual bool Initialize(const FString& Code, UObject* Owner) = 0;

	/**
	* Sends BeginPlay event to the script
	*/
	virtual void BeginPlay() = 0;

	/**
	* Sends Tick event to the script
	*/
	virtual void Tick(float DeltaTime) = 0;

	/**
	* Sends Destroy event to the script and destroys the script.
	*/
	virtual void Destroy() = 0;

	/**
	* @return true if script defines Tick function
	*/
	virtual bool CanTick() = 0;

	/**
	* Calls arbitrary script function (no arguments) given its name.
	* @param FunctionName Name of the function to call
	*/
	virtual bool CallFunction(const FString&  FunctionName) = 0;

	virtual bool SetFloatVariable(const FString&  GlobalVariable, float NewValue) = 0;
	virtual float GetFloatVariable(const FString&  GlobalVariable) = 0;
	virtual bool SetStringVariable(const FString&  GlobalVariable, const FString& NewValue) = 0;
	virtual FString GetStringVariable(const FString&  GlobalVariable) = 0;

	/**
	* Invokes script function from Blueprint code
	*/
	virtual void InvokeScriptFunction(FFrame& Stack, RESULT_DECL) = 0;

#if WITH_EDITOR
	/**
	* Returns a list of exported fields from the script (member variables and functions).
	*/
	virtual void GetScriptDefinedFields(TArray<FScriptField>& OutFields) = 0;
#endif

	virtual ~FScriptContextBase() {}
};

/**
* Script generated class
*/
UCLASS(EarlyAccessPreview)
class SCRIPTPLUGIN_API UScriptBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

	/** Generated script bytecode */
	UPROPERTY()
	TArray<uint8> ByteCode;

	/** Script source code. @todo: this should be editor-only */
	UPROPERTY()
	FString SourceCode;

	virtual void PostInitProperties() override;
	virtual void Link(FArchive& Ar, bool bRelinkExistingProperties) override;

	/**
	 * Creates a script context object
	 */
	FScriptContextBase* CreateContext();

	/**
	* Adds a unique native function mapping
	* @param InName Name of the native function
	* @param InPointer Pointer to the native member function
	*/
	void AddUniqueNativeFunction(const FName& InName, Native InPointer);

	/**
	* Removes native function mapping
	* @param InName Name of the native function
	*/
	void RemoveNativeFunction(const FName& InName);
};