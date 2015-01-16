// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InheritableComponentHandler.generated.h"

class USCS_Node;
class UActorComponent;

USTRUCT()
struct ENGINE_API FComponentKey
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UBlueprintGeneratedClass* OwnerClass;

	UPROPERTY()
	FName VariableName;

	// TODO: a GUID should be added

	FComponentKey()
		: OwnerClass(nullptr)
	{}

	FComponentKey(UBlueprintGeneratedClass* InOwnerClass, FName InVariableNam)
		: OwnerClass(nullptr)
		, VariableName(InVariableNam)
	{}

	FComponentKey(USCS_Node* ParentNode);

	bool Match(FComponentKey OtherKey) const;

	bool IsValid() const
	{
		return OwnerClass && (VariableName != NAME_None);
	}

	UActorComponent* GetOriginalTemplate() const;
};

USTRUCT()
struct FComponentOverrideRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UActorComponent* ComponentTemplate;

	UPROPERTY()
	FComponentKey ComponentKey;

	FComponentOverrideRecord()
		: ComponentTemplate(nullptr)
	{}
};

UCLASS()
class ENGINE_API UInheritableComponentHandler : public UObject
{
	GENERATED_BODY()

#if WITH_EDITOR
private:
	bool IsRecordValid(const FComponentOverrideRecord& Record) const;
	bool IsRecordNecessary(const FComponentOverrideRecord& Record) const;

public:
	UActorComponent* CreateOverridenComponentTemplate(FComponentKey Key);
	void UpdateOverridenComponentTemplate(FComponentKey Key);
	void UpdateOwnerClass(UBlueprintGeneratedClass* OwnerClass);
	void RemoveInvalidAndUnnecessaryTemplates();
	bool IsValid() const;
	UActorComponent* FindBestArchetype(FComponentKey Key) const;

	void GetAllTemplates(TArray<UActorComponent*>& OutArray) const
	{
		for (auto Record : Records)
		{
			OutArray.Add(Record.ComponentTemplate);
		}
	}

	bool IsEmpty() const
	{
		return 0 == Records.Num();
	}

	void PreloadAllTempates();

#endif

public:
	UActorComponent* GetOverridenComponentTemplate(FComponentKey Key) const;

private:
	const FComponentOverrideRecord* FindRecord(FComponentKey Key) const;
	
	UPROPERTY()
	TArray<FComponentOverrideRecord> Records;
};
