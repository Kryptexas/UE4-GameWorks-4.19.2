// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineUtils.h"

UBlackboardComponent::UBlackboardComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	bPausedNotifies = false;
}

void UBlackboardComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// cache blackboard component if owner has one
	BehaviorComp = GetOwner()->FindComponentByClass<UBehaviorTreeComponent>();
	InitializeBlackboard(BlackboardAsset);
}

struct FBlackboardInitializationData
{
	uint8 KeyID;
	uint16 DataSize;

	FBlackboardInitializationData() {}
	FBlackboardInitializationData(uint8 InKeyID, uint16 InDataSize) : KeyID(InKeyID)
	{
		DataSize = (InDataSize <= 2) ? InDataSize : ((InDataSize + 3) & ~3);
	}

	struct FMemorySort
	{
		FORCEINLINE bool operator()(const FBlackboardInitializationData& A, const FBlackboardInitializationData& B) const
		{
			return A.DataSize > B.DataSize;
		}
	};
};

void UBlackboardComponent::InitializeBlackboard(class UBlackboardData* NewAsset)
{
	// if we re-initialize with the same asset then there's no point
	// in reseting, since we'd loose all the accumulated knowledge
	if (NewAsset == BlackboardAsset)
	{
		return;
	}

	BlackboardAsset = NewAsset;
	ValueMemory.Reset();
	ValueOffsets.Reset();

	if (BlackboardAsset)
	{
		if (BlackboardAsset->IsValid())
		{
			TArray<FBlackboardInitializationData> InitList;
			const int32 NumKeys = BlackboardAsset->GetNumKeys();
			InitList.Reserve(NumKeys);
			ValueOffsets.AddZeroed(NumKeys);

			for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
			{
				for (int32 i = 0; i < It->Keys.Num(); i++)
				{
					if (It->Keys[i].KeyType)
					{
						InitList.Add(FBlackboardInitializationData(i + It->GetFirstKeyID(), It->Keys[i].KeyType->GetValueSize()));
					}
				}
			}

			// sort key values by memory size, so they can be packed better
			// it still won't protect against structures, that are internally misaligned (-> uint8, uint32)
			// but since all Engine level keys are good... 
			InitList.Sort(FBlackboardInitializationData::FMemorySort());
			uint16 MemoryOffset = 0;
			for (int32 i = 0; i < InitList.Num(); i++)
			{
				ValueOffsets[InitList[i].KeyID] = MemoryOffset;
				MemoryOffset += InitList[i].DataSize;
			}

			ValueMemory.AddZeroed(MemoryOffset);
		}
		else
		{
			UE_LOG(LogBehaviorTree, Error, TEXT("Blackboard asset (%s) has errors and can't be used!"), *GetNameSafe(BlackboardAsset));
		}
	}
}

class UBehaviorTreeComponent* UBlackboardComponent::GetBehaviorComponent() const
{
	return BehaviorComp;
}

class UBlackboardData* UBlackboardComponent::GetBlackboardAsset() const
{
	return BlackboardAsset;
}

FName UBlackboardComponent::GetKeyName(uint8 KeyID) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyName(KeyID) : NAME_None;
}

uint8 UBlackboardComponent::GetKeyID(const FName& KeyName) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyID(KeyName) : 0xFF;
}

TSubclassOf<UBlackboardKeyType> UBlackboardComponent::GetKeyType(uint8 KeyID) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyType(KeyID) : NULL;
}

int32 UBlackboardComponent::GetNumKeys() const
{
	return BlackboardAsset ? BlackboardAsset->GetNumKeys() : 0;
}

void UBlackboardComponent::RegisterObserver(uint8 KeyID, FOnBlackboardChange ObserverDelegate)
{
	Observers.AddUnique(KeyID, ObserverDelegate);
}

void UBlackboardComponent::UnregisterObserver(uint8 KeyID, FOnBlackboardChange ObserverDelegate)
{
	Observers.RemoveSingle(KeyID, ObserverDelegate);
}

void UBlackboardComponent::PauseUpdates()
{
	bPausedNotifies = true;
}

void UBlackboardComponent::ResumeUpdates()
{
	bPausedNotifies = false;

	for (int32 i = 0; i < QueuedUpdates.Num(); i++)
	{
		NotifyObservers(QueuedUpdates[i]);
	}

	QueuedUpdates.Empty();
}

void UBlackboardComponent::NotifyObservers(uint8 KeyID) const
{
	if (bPausedNotifies)
	{
		QueuedUpdates.AddUnique(KeyID);
	}
	else
	{
		for (TMultiMap<uint8, FOnBlackboardChange>::TConstKeyIterator KeyIt(Observers, KeyID); KeyIt; ++KeyIt)
		{
			const FOnBlackboardChange& ObserverDelegate = KeyIt.Value();
			ObserverDelegate.ExecuteIfBound(this, KeyID);
		}
	}
}

bool UBlackboardComponent::IsCompatibileWith(class UBlackboardData* TestAsset) const
{
	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		if (It == TestAsset)
		{
			return true;
		}
	}

	return false;
}

UBlackboardKeyType::CompareResult UBlackboardComponent::CompareKeyValues(TSubclassOf<class UBlackboardKeyType> KeyType, uint8 KeyA, uint8 KeyB) const
{
	return UBlackboardKeyType::CompareResult(Cast<UBlackboardKeyType>((*KeyType)->GetDefaultObject())->Compare(GetKeyRawData(KeyA), GetKeyRawData(KeyB)));
}

FString UBlackboardComponent::GetDebugInfoString(EBlackboardDescription::Type Mode) const 
{
	FString DebugString = FString::Printf(TEXT("Blackboard (asset: %s)\n"), *GetNameSafe(BlackboardAsset));

	uint8 Offset = 0;
	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		for (int32 i = 0; i < It->Keys.Num(); i++)
		{
			DebugString += FString::Printf(TEXT("  %s\n"), *DescribeKeyValue(i + Offset, Mode));
		}
		Offset += It->Keys.Num();
	}

	if (Mode == EBlackboardDescription::Full && BlackboardAsset)
	{
		DebugString += TEXT("Observed Keys:\n");

		TArray<uint8> ObserversKeys;
		if (Observers.GetKeys(ObserversKeys) > 0)
		{
			DebugString += TEXT("Observed Keys:\n");
		
			for (int32 KeyIndex = 0; KeyIndex < ObserversKeys.Num(); ++KeyIndex)
			{
				const uint8 KeyID = ObserversKeys[KeyIndex];
				//@todo shouldn't be using a localized value?; GetKeyName() [10/11/2013 justin.sargent]
				DebugString += FString::Printf(TEXT("  %s:\n"), *BlackboardAsset->GetKeyName(KeyID).ToString());
			}
		}
		else
		{
			DebugString += TEXT("  NONE\n");
		}
	}

	return DebugString;
}

FString UBlackboardComponent::DescribeKeyValue(uint8 KeyID, EBlackboardDescription::Type Mode) const
{
	FString Description;

	const FBlackboardEntry* Key = BlackboardAsset ? BlackboardAsset->GetKey(KeyID) : NULL;
	if (Key)
	{
		const uint8* ValueData = GetKeyRawData(KeyID);
		FString ValueDesc = Key->KeyType ? *(Key->KeyType->DescribeValue(ValueData)) : TEXT("empty");

		if (Mode == EBlackboardDescription::OnlyValue)
		{
			Description = ValueDesc;
		}
		else if (Mode == EBlackboardDescription::KeyWithValue)
		{
			Description = FString::Printf(TEXT("%s: %s"), *(Key->EntryName.ToString()), *ValueDesc);
		}
		else
		{
			const FString CommonTypePrefix = UBlackboardKeyType::StaticClass()->GetName().AppendChar(TEXT('_'));
			const FString FullKeyType = Key->KeyType ? GetNameSafe(Key->KeyType->GetClass()) : FString();
			const FString DescKeyType = FullKeyType.StartsWith(CommonTypePrefix) ? FullKeyType.RightChop(CommonTypePrefix.Len()) : FullKeyType;

			Description = FString::Printf(TEXT("%s [%s]: %s"), *(Key->EntryName.ToString()), *DescKeyType, *ValueDesc);
		}
	}

	return Description;
}

#if ENABLE_VISUAL_LOG

void UBlackboardComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{
	Snapshot->StatusString += GetDebugInfoString(EBlackboardDescription::Full);
}

#endif

UObject* UBlackboardComponent::GetValueAsObject(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Object::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsObject(KeyID);
}

UObject* UBlackboardComponent::GetValueAsObject(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Object::GetValue(RawData) : NULL;
}

UClass* UBlackboardComponent::GetValueAsClass(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Class::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsClass(KeyID);
}

UClass* UBlackboardComponent::GetValueAsClass(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Class::GetValue(RawData) : NULL;
}

uint8 UBlackboardComponent::GetValueAsEnum(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Enum::StaticClass() &&
		GetKeyType(KeyID) != UBlackboardKeyType_NativeEnum::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsEnum(KeyID);
}

uint8 UBlackboardComponent::GetValueAsEnum(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Enum::GetValue(RawData) : 0;
}

int32 UBlackboardComponent::GetValueAsInt(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Int::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsInt(KeyID);
}

int32 UBlackboardComponent::GetValueAsInt(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Int::GetValue(RawData) : 0;
}

float UBlackboardComponent::GetValueAsFloat(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Float::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsFloat(KeyID);
}

float UBlackboardComponent::GetValueAsFloat(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Float::GetValue(RawData) : 0.0f;
}

bool UBlackboardComponent::GetValueAsBool(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Bool::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsBool(KeyID);
}

bool UBlackboardComponent::GetValueAsBool(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Bool::GetValue(RawData) : false;
}

FString UBlackboardComponent::GetValueAsString(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_String::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsString(KeyID);
}

FString UBlackboardComponent::GetValueAsString(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_String::GetValue(RawData) : FString();
}

FName UBlackboardComponent::GetValueAsName(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Name::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsName(KeyID);
}

FName UBlackboardComponent::GetValueAsName(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Name::GetValue(RawData) : NAME_None;
}

FVector UBlackboardComponent::GetValueAsVector(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Vector::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsVector(KeyID);
}

FVector UBlackboardComponent::GetValueAsVector(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Vector::GetValue(RawData) : FVector::ZeroVector;
}

void UBlackboardComponent::SetValueAsObject(const FName& KeyName, UObject* ObjectValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Object::StaticClass())
	{
		SetValueAsObject(KeyID, ObjectValue);
	}
}

void UBlackboardComponent::SetValueAsObject(uint8 KeyID, UObject* ObjectValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Object::SetValue(RawData, ObjectValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsClass(const FName& KeyName, UClass* ClassValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Class::StaticClass())
	{
		SetValueAsClass(KeyID, ClassValue);
	}
}

void UBlackboardComponent::SetValueAsClass(uint8 KeyID, UClass* ClassValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Class::SetValue(RawData, ClassValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsEnum(const FName& KeyName, uint8 EnumValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Enum::StaticClass() ||
		GetKeyType(KeyID) == UBlackboardKeyType_NativeEnum::StaticClass())
	{
		SetValueAsEnum(KeyID, EnumValue);
	}
}

void UBlackboardComponent::SetValueAsEnum(uint8 KeyID, uint8 EnumValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Enum::SetValue(RawData, EnumValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsInt(const FName& KeyName, int32 IntValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Int::StaticClass())
	{
		SetValueAsInt(KeyID, IntValue);
	}
}

void UBlackboardComponent::SetValueAsInt(uint8 KeyID, int32 IntValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Int::SetValue(RawData, IntValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsFloat(const FName& KeyName, float FloatValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Float::StaticClass())
	{
		SetValueAsFloat(KeyID, FloatValue);
	}
}

void UBlackboardComponent::SetValueAsFloat(uint8 KeyID, float FloatValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Float::SetValue(RawData, FloatValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsBool(const FName& KeyName, bool BoolValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Bool::StaticClass())
	{
		SetValueAsBool(KeyID, BoolValue);
	}
}

void UBlackboardComponent::SetValueAsBool(uint8 KeyID, bool BoolValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Bool::SetValue(RawData, BoolValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsString(const FName& KeyName, FString StringValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_String::StaticClass())
	{
		SetValueAsString(KeyID, StringValue);
	}
}

void UBlackboardComponent::SetValueAsString(uint8 KeyID, const FString& StringValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_String::SetValue(RawData, StringValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsName(const FName& KeyName, FName NameValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Name::StaticClass())
	{
		SetValueAsName(KeyID, NameValue);
	}
}

void UBlackboardComponent::SetValueAsName(uint8 KeyID, const FName& NameValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Name::SetValue(RawData, NameValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::SetValueAsVector(const FName& KeyName, FVector VectorValue)
{
	const uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) == UBlackboardKeyType_Vector::StaticClass())
	{
		SetValueAsVector(KeyID, VectorValue);
	}
}

void UBlackboardComponent::SetValueAsVector(uint8 KeyID, const FVector& VectorValue)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Vector::SetValue(RawData, VectorValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

bool UBlackboardComponent::GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const
{
	const uint8 KeyID = GetKeyID(KeyName);
	return GetLocationFromEntry(KeyID, ResultLocation);
}

bool UBlackboardComponent::GetLocationFromEntry(uint8 KeyID, FVector& ResultLocation) const
{
	if (BlackboardAsset && ValueOffsets.IsValidIndex(KeyID))
	{
		const FBlackboardEntry* EntryInfo = BlackboardAsset->GetKey(KeyID);
		if (EntryInfo && EntryInfo->KeyType)
		{
			const uint8* ValueData = ValueMemory.GetTypedData() + ValueOffsets[KeyID];
			return EntryInfo->KeyType->GetLocation(ValueData, ResultLocation);
		}
	}

	return false;
}
