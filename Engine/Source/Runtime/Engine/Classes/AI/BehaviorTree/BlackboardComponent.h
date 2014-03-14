// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** 
 *  Blackboard - holds AI's world knowledge, easily accessible for behavior trees
 *
 *  Properties are stored in byte array, and should be accessed only though
 *  GetValue* / SetValue* functions. They will handle broadcasting change events
 *  for registered observers.
 *
 *  Keys are defined by BlackboardData data asset.
 */

#pragma once
#include "BehaviorTreeTypes.h"
#include "BlackboardComponent.generated.h"

namespace EBlackboardDescription
{
	enum Type
	{
		OnlyValue,
		KeyWithValue,
		DetailedKeyWithValue,
		Full,
	};
}

UCLASS(HeaderGroup=Component)
class ENGINE_API UBlackboardComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** BEGIN UActorComponent overrides */
	virtual void InitializeComponent() OVERRIDE;
	/** END UActorComponent overrides */

	/** @return name of key */
	FName GetKeyName(uint8 KeyID) const;

	/** @return key ID from name */
	uint8 GetKeyID(const FName& KeyName) const;

	/** @return class of value for given key */
	TSubclassOf<UBlackboardKeyType> GetKeyType(uint8 KeyID) const;

	/** @return number of entries in data asset */
	int32 GetNumKeys() const;

	/** @return true if blackboard have valid data asset */
	bool HasValidAsset() const;

	/** register observer for blackboard key */
	void RegisterObserver(uint8 KeyID, FOnBlackboardChange ObserverDelegate);

	/** unregister observer from blackboard key */
	void UnregisterObserver(uint8 KeyID, FOnBlackboardChange ObserverDelegate);

	/** pause change notifies and add them to queue */
	void PauseUpdates();

	/** resume change notifies and process queued list */
	void ResumeUpdates();

	/** @return associated behavior tree component */
	class UBehaviorTreeComponent* GetBehaviorComponent() const;

	/** @return blackboard data asset */
	class UBlackboardData* GetBlackboardAsset() const;

	/** setup component for using given blackboard asset */
	void InitializeBlackboard(class UBlackboardData* NewAsset);
	
	/** @return true if component can be used with specified blackboard asset */
	bool IsCompatibileWith(class UBlackboardData* TestAsset) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	UObject* GetValueAsObject(const FName& KeyName) const;
	UObject* GetValueAsObject(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	UClass* GetValueAsClass(const FName& KeyName) const;
	UClass* GetValueAsClass(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	uint8 GetValueAsEnum(const FName& KeyName) const;
	uint8 GetValueAsEnum(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	int32 GetValueAsInt(const FName& KeyName) const;
	int32 GetValueAsInt(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	float GetValueAsFloat(const FName& KeyName) const;
	float GetValueAsFloat(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	bool GetValueAsBool(const FName& KeyName) const;
	bool GetValueAsBool(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FString GetValueAsString(const FName& KeyName) const;
	FString GetValueAsString(uint8 KeyID) const;
	
	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FName GetValueAsName(const FName& KeyName) const;
	FName GetValueAsName(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FVector GetValueAsVector(const FName& KeyName) const;
	FVector GetValueAsVector(uint8 KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsObject(const FName& KeyName, UObject* ObjectValue);
	void SetValueAsObject(uint8 KeyID, UObject* ObjectValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsClass(const FName& KeyName, UClass* ClassValue);
	void SetValueAsClass(uint8 KeyID, UClass* ClassValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsEnum(const FName& KeyName, uint8 EnumValue);
	void SetValueAsEnum(uint8 KeyID, uint8 EnumValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsInt(const FName& KeyName, int32 IntValue);
	void SetValueAsInt(uint8 KeyID, int32 IntValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsFloat(const FName& KeyName, float FloatValue);
	void SetValueAsFloat(uint8 KeyID, float FloatValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsBool(const FName& KeyName, bool BoolValue);
	void SetValueAsBool(uint8 KeyID, bool BoolValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsString(const FName& KeyName, FString StringValue);
	void SetValueAsString(uint8 KeyID, const FString& StringValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsName(const FName& KeyName, FName NameValue);
	void SetValueAsName(uint8 KeyID, const FName& NameValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsVector(const FName& KeyName, FVector VectorValue);
	void SetValueAsVector(uint8 KeyID, const FVector& VectorValue);

	/** return false if call failed (most probably no such entry in BB) */
	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	bool GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const;
	bool GetLocationFromEntry(uint8 KeyID, FVector& ResultLocation) const;

	/** get pointer to raw data for given key */
	FORCEINLINE uint8* GetKeyRawData(const FName& KeyName) { return GetKeyRawData(GetKeyID(KeyName)); }
	FORCEINLINE uint8* GetKeyRawData(uint8 KeyID) { return ValueMemory.Num() && ValueOffsets.IsValidIndex(KeyID) ? (ValueMemory.GetTypedData() + ValueOffsets[KeyID]) : NULL; }

	FORCEINLINE const uint8* GetKeyRawData(const FName& KeyName) const { return GetKeyRawData(GetKeyID(KeyName)); }
	FORCEINLINE const uint8* GetKeyRawData(uint8 KeyID) const { return ValueMemory.Num() && ValueOffsets.IsValidIndex(KeyID) ? (ValueMemory.GetTypedData() + ValueOffsets[KeyID]) : NULL; }

	FORCEINLINE bool IsValidKey(uint8 KeyID) const { check(BlackboardAsset); return KeyID != UBlackboardData::InvalidKeyID && BlackboardAsset->Keys.IsValidIndex(KeyID); }

	/** compares blackboard's values under specified keys */
	UBlackboardKeyType::CompareResult CompareKeyValues(TSubclassOf<class UBlackboardKeyType> KeyType, uint8 KeyA, uint8 KeyB) const;

	FString GetDebugInfoString(EBlackboardDescription::Type Mode) const;

	/** get description of value under given key */
	FString DescribeKeyValue(uint8 KeyID, EBlackboardDescription::Type Mode) const;

#if ENABLE_VISUAL_LOG
	/** prepare blackboard snapshot for logs */
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif
	
protected:

	/** cached behavior tree component */
	UPROPERTY(transient)
	class UBehaviorTreeComponent* BehaviorComp;

	/** data asset defining entries */
	UPROPERTY(transient)
	class UBlackboardData* BlackboardAsset;

	/** memory block holding all values */
	TArray<uint8> ValueMemory;

	/** offsets in ValueMemory for each key */
	TArray<uint16> ValueOffsets;

	/** observers registered for blackboard keys */
	TMultiMap<uint8, FOnBlackboardChange> Observers;

	/** queued key change notification, will be processed on ResumeUpdates call */
	mutable TArray<uint8> QueuedUpdates;

	/** set when notifies are paused and shouldn't be passed to observers */
	uint32 bPausedNotifies : 1;

	/** notifies behavior tree decorators about change in blackboard */
	void NotifyObservers(uint8 KeyID) const;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE bool UBlackboardComponent::HasValidAsset() const
{
	return BlackboardAsset && BlackboardAsset->IsValid();
}
