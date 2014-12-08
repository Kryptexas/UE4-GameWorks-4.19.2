// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
#include "Components/ActorComponent.h"
#include "BehaviorTreeTypes.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BlackboardData.h"
#include "BlackboardComponent.generated.h"

class UBlackboardData;
class UBrainComponent;
class UBlackboardKeyType;

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

UCLASS()
class AIMODULE_API UBlackboardComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** BEGIN UActorComponent overrides */
	virtual void InitializeComponent() override;
	/** END UActorComponent overrides */

	/** @return name of key */
	FName GetKeyName(FBlackboard::FKey KeyID) const;

	/** @return key ID from name */
	FBlackboard::FKey GetKeyID(const FName& KeyName) const;

	/** @return class of value for given key */
	TSubclassOf<UBlackboardKeyType> GetKeyType(FBlackboard::FKey KeyID) const;

	/** @return number of entries in data asset */
	int32 GetNumKeys() const;

	/** @return true if blackboard have valid data asset */
	bool HasValidAsset() const;

	/** register observer for blackboard key */
	void RegisterObserver(FBlackboard::FKey KeyID, FOnBlackboardChange ObserverDelegate);

	/** unregister observer from blackboard key */
	void UnregisterObserver(FBlackboard::FKey KeyID, FOnBlackboardChange ObserverDelegate);

	/** pause change notifies and add them to queue */
	void PauseUpdates();

	/** resume change notifies and process queued list */
	void ResumeUpdates();

	/** @return associated behavior tree component */
	UBrainComponent* GetBrainComponent() const;

	/** @return blackboard data asset */
	UBlackboardData* GetBlackboardAsset() const;

	/** caches UBrainComponent pointer to be used in communication */
	void CacheBrainComponent(UBrainComponent& BrainComponent);

	/** setup component for using given blackboard asset */
	bool InitializeBlackboard(UBlackboardData* NewAsset);
	
	/** @return true if component can be used with specified blackboard asset */
	bool IsCompatibleWith(UBlackboardData* TestAsset) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	UObject* GetValueAsObject(const FName& KeyName) const;
	UObject* GetValueAsObject(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	UClass* GetValueAsClass(const FName& KeyName) const;
	UClass* GetValueAsClass(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	uint8 GetValueAsEnum(const FName& KeyName) const;
	uint8 GetValueAsEnum(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	int32 GetValueAsInt(const FName& KeyName) const;
	int32 GetValueAsInt(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	float GetValueAsFloat(const FName& KeyName) const;
	float GetValueAsFloat(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	bool GetValueAsBool(const FName& KeyName) const;
	bool GetValueAsBool(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FString GetValueAsString(const FName& KeyName) const;
	FString GetValueAsString(FBlackboard::FKey KeyID) const;
	
	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FName GetValueAsName(const FName& KeyName) const;
	FName GetValueAsName(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FVector GetValueAsVector(const FName& KeyName) const;
	FVector GetValueAsVector(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	FRotator GetValueAsRotator(const FName& KeyName) const;
	FRotator GetValueAsRotator(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsObject(const FName& KeyName, UObject* ObjectValue);
	void SetValueAsObject(FBlackboard::FKey KeyID, UObject* ObjectValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsClass(const FName& KeyName, UClass* ClassValue);
	void SetValueAsClass(FBlackboard::FKey KeyID, UClass* ClassValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsEnum(const FName& KeyName, uint8 EnumValue);
	void SetValueAsEnum(FBlackboard::FKey KeyID, uint8 EnumValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsInt(const FName& KeyName, int32 IntValue);
	void SetValueAsInt(FBlackboard::FKey KeyID, int32 IntValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsFloat(const FName& KeyName, float FloatValue);
	void SetValueAsFloat(FBlackboard::FKey KeyID, float FloatValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsBool(const FName& KeyName, bool BoolValue);
	void SetValueAsBool(FBlackboard::FKey KeyID, bool BoolValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsString(const FName& KeyName, FString StringValue);
	void SetValueAsString(FBlackboard::FKey KeyID, const FString& StringValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsName(const FName& KeyName, FName NameValue);
	void SetValueAsName(FBlackboard::FKey KeyID, const FName& NameValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsVector(const FName& KeyName, FVector VectorValue);
	void SetValueAsVector(FBlackboard::FKey KeyID, const FVector& VectorValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void ClearValueAsVector(const FName& KeyName);
	void ClearValueAsVector(FBlackboard::FKey KeyID);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard", Meta=(
		Tooltip="If the vector value has been set (and not cleared), this function returns true (indicating that the value should be valid).  If it's not set, the vector value is invalid and this function will return false.  (Also returns false if the key specified does not hold a vector.)"))
	bool IsVectorValueSet(const FName& KeyName) const;
	bool IsVectorValueSet(FBlackboard::FKey KeyID) const;

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void SetValueAsRotator(const FName& KeyName, FRotator VectorValue);
	void SetValueAsRotator(FBlackboard::FKey KeyID, const FRotator& VectorValue);

	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	void ClearValueAsRotator(const FName& KeyName);
	void ClearValueAsRotator(FBlackboard::FKey KeyID);

	/** return false if call failed (most probably no such entry in BB) */
	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	bool GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const;
	bool GetLocationFromEntry(FBlackboard::FKey KeyID, FVector& ResultLocation) const;

	/** return false if call failed (most probably no such entry in BB) */
	UFUNCTION(BlueprintCallable, Category="AI|Components|Blackboard")
	bool GetRotationFromEntry(const FName& KeyName, FRotator& ResultRotation) const;
	bool GetRotationFromEntry(FBlackboard::FKey KeyID, FRotator& ResultRotation) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
	void ClearValue(const FName& KeyName);
	void ClearValue(FBlackboard::FKey KeyID);

	/** get pointer to raw data for given key */
	FORCEINLINE uint8* GetKeyRawData(const FName& KeyName) { return GetKeyRawData(GetKeyID(KeyName)); }
	FORCEINLINE uint8* GetKeyRawData(FBlackboard::FKey KeyID) { return ValueMemory.Num() && ValueOffsets.IsValidIndex(KeyID) ? (ValueMemory.GetData() + ValueOffsets[KeyID]) : NULL; }

	FORCEINLINE const uint8* GetKeyRawData(const FName& KeyName) const { return GetKeyRawData(GetKeyID(KeyName)); }
	FORCEINLINE const uint8* GetKeyRawData(FBlackboard::FKey KeyID) const { return ValueMemory.Num() && ValueOffsets.IsValidIndex(KeyID) ? (ValueMemory.GetData() + ValueOffsets[KeyID]) : NULL; }

	FORCEINLINE bool IsValidKey(FBlackboard::FKey KeyID) const { check(BlackboardAsset); return KeyID != FBlackboard::InvalidKey && BlackboardAsset->Keys.IsValidIndex(KeyID); }

	/** compares blackboard's values under specified keys */
	EBlackboardCompare::Type CompareKeyValues(TSubclassOf<UBlackboardKeyType> KeyType, FBlackboard::FKey KeyA, FBlackboard::FKey KeyB) const;

	FString GetDebugInfoString(EBlackboardDescription::Type Mode) const;

	/** get description of value under given key */
	FString DescribeKeyValue(const FName& KeyName, EBlackboardDescription::Type Mode) const;
	FString DescribeKeyValue(FBlackboard::FKey KeyID, EBlackboardDescription::Type Mode) const;

#if ENABLE_VISUAL_LOG
	/** prepare blackboard snapshot for logs */
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif
	
protected:

	/** cached behavior tree component */
	UPROPERTY(transient)
	UBrainComponent* BrainComp;

	/** data asset defining entries */
	UPROPERTY(transient)
	UBlackboardData* BlackboardAsset;

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
	void NotifyObservers(FBlackboard::FKey KeyID) const;

	/** initializes parent chain in asset */
	void InitializeParentChain(UBlackboardData* NewAsset);
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE bool UBlackboardComponent::HasValidAsset() const
{
	return BlackboardAsset && BlackboardAsset->IsValid();
}
