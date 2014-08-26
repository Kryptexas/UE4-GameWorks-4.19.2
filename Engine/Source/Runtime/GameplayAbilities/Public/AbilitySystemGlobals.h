// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemGlobals.generated.h"

class AActor;
class UAbilitySystemComponent;
class UCurveTable;
class UDataTable;

struct FGameplayAbilityActorInfo;
struct FGameplayTag;
struct FAttributeSetInitter;

/** Holds global data for the skill system. Can be configured per project via config file */
UCLASS(config=Game)
class GAMEPLAYABILITIES_API UAbilitySystemGlobals : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual void InitGlobalData();
	

	/** Holds all of the valid gameplay-related tags that can be applied to assets */
	UPROPERTY(config)
	FString GlobalCurveTableName;

	/** Holds information about the valid attributes' min and max values and stacking rules */
	UPROPERTY(config)
	FString GlobalAttributeMetaDataTableName;

	/** Holds default values for attribute sets, keyed off of Name/Levels. */
	UPROPERTY(config)
	FString GlobalAttributeSetDefaultsTableName;
	
	UCurveTable* GetGlobalCurveTable();

	UDataTable* GetGlobalAttributeMetaDataTable();

	void AutomationTestOnly_SetGlobalCurveTable(class UCurveTable *InTable)
	{
		GlobalCurveTable = InTable;
	}

	void AutomationTestOnly_SetGlobalAttributeDataTable(class UDataTable *InTable)
	{
		GlobalAttributeMetaDataTable = InTable;
	}

	static UAbilitySystemGlobals& Get();

	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(AActor* Actor);

	/** Should allocate a project specific AbilityActorInfo struct. Caller is responsible for dellocation */
	virtual FGameplayAbilityActorInfo * AllocAbilityActorInfo() const;

	UFunction* GetGameplayCueFunction(const FGameplayTag &Tag, UClass* Class, FName &MatchedTag);

	FAttributeSetInitter* GetAttributeSetInitter() const;

protected:

	virtual void InitAtributeDefaults();
	virtual void AllocAttributeSetInitter();

private:

	UPROPERTY()
	UCurveTable* GlobalCurveTable;

	UPROPERTY()
	UCurveTable* GlobalAttributeDefaultsTable;

	UPROPERTY()
	UDataTable* GlobalAttributeMetaDataTable;

	TSharedPtr<FAttributeSetInitter> GlobalAttributeSetInitter;

	template <class T>
	T* InternalGetLoadTable(T*& Table, FString TableName);

#if WITH_EDITOR
	void OnTableReimported(UObject* InObject);
#endif

#if WITH_EDITORONLY_DATA
	bool RegisteredReimportCallback;
#endif

};
