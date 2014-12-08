// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EngineUtils.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "BehaviorTree/BlackboardComponent.h"

UBlackboardComponent::UBlackboardComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	bPausedNotifies = false;
}

void UBlackboardComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// cache blackboard component if owner has one
	// note that it's a valid scenario for this component to not have an owner at all (at least in terms of unittesting)
	AActor* Owner = GetOwner();
	if (Owner)
	{
		BrainComp = GetOwner()->FindComponentByClass<UBrainComponent>();
		if (BrainComp)
		{
			BrainComp->CacheBlackboardComponent(this);
		}
	}

	InitializeBlackboard(BlackboardAsset);
}

void UBlackboardComponent::CacheBrainComponent(UBrainComponent& BrainComponent)
{
	if (&BrainComponent != BrainComp)
	{
		BrainComp = &BrainComponent;
	}
}

struct FBlackboardInitializationData
{
	FBlackboard::FKey KeyID;
	uint16 DataSize;

	FBlackboardInitializationData() {}
	FBlackboardInitializationData(FBlackboard::FKey InKeyID, uint16 InDataSize) : KeyID(InKeyID)
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

void UBlackboardComponent::InitializeParentChain(UBlackboardData* NewAsset)
{
	if (NewAsset)
	{
		InitializeParentChain(NewAsset->Parent);
		NewAsset->UpdateKeyIDs();
	}
}

bool UBlackboardComponent::InitializeBlackboard(UBlackboardData* NewAsset)
{
	// if we re-initialize with the same asset then there's no point
	// in reseting, since we'd loose all the accumulated knowledge
	if (NewAsset == BlackboardAsset)
	{
		return false;
	}

	BlackboardAsset = NewAsset;
	ValueMemory.Reset();
	ValueOffsets.Reset();

	bool bSuccess = true;
	
	if (BlackboardAsset)
	{
		if (BlackboardAsset->IsValid())
		{
			InitializeParentChain(BlackboardAsset);

			TArray<FBlackboardInitializationData> InitList;
			const int32 NumKeys = BlackboardAsset->GetNumKeys();
			InitList.Reserve(NumKeys);
			ValueOffsets.AddZeroed(NumKeys);

			for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
			{
				for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); KeyIndex++)
				{
					if (It->Keys[KeyIndex].KeyType)
					{
						InitList.Add(FBlackboardInitializationData(KeyIndex + It->GetFirstKeyID(), It->Keys[KeyIndex].KeyType->GetValueSize()));
					}
				}
			}

			// sort key values by memory size, so they can be packed better
			// it still won't protect against structures, that are internally misaligned (-> uint8, uint32)
			// but since all Engine level keys are good... 
			InitList.Sort(FBlackboardInitializationData::FMemorySort());
			uint16 MemoryOffset = 0;
			for (int32 Index = 0; Index < InitList.Num(); Index++)
			{
				ValueOffsets[InitList[Index].KeyID] = MemoryOffset;
				MemoryOffset += InitList[Index].DataSize;
			}

			ValueMemory.AddZeroed(MemoryOffset);

			// initialize memory
			for (int32 Index = 0; Index < InitList.Num(); Index++)
			{
				const FBlackboardEntry* KeyData = BlackboardAsset->GetKey(InitList[Index].KeyID);
				uint8* RawData = GetKeyRawData(InitList[Index].KeyID);

				KeyData->KeyType->Initialize(RawData);
			}
		}
		else
		{
			bSuccess = false;
			UE_LOG(LogBehaviorTree, Error, TEXT("Blackboard asset (%s) has errors and can't be used!"), *GetNameSafe(BlackboardAsset));
		}
	}

	return bSuccess;
}

UBrainComponent* UBlackboardComponent::GetBrainComponent() const
{
	return BrainComp;
}

UBlackboardData* UBlackboardComponent::GetBlackboardAsset() const
{
	return BlackboardAsset;
}

FName UBlackboardComponent::GetKeyName(FBlackboard::FKey KeyID) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyName(KeyID) : NAME_None;
}

FBlackboard::FKey UBlackboardComponent::GetKeyID(const FName& KeyName) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyID(KeyName) : FBlackboard::InvalidKey;
}

TSubclassOf<UBlackboardKeyType> UBlackboardComponent::GetKeyType(FBlackboard::FKey KeyID) const
{
	return BlackboardAsset ? BlackboardAsset->GetKeyType(KeyID) : NULL;
}

int32 UBlackboardComponent::GetNumKeys() const
{
	return BlackboardAsset ? BlackboardAsset->GetNumKeys() : 0;
}

void UBlackboardComponent::RegisterObserver(FBlackboard::FKey KeyID, FOnBlackboardChange ObserverDelegate)
{
	Observers.AddUnique(KeyID, ObserverDelegate);
}

void UBlackboardComponent::UnregisterObserver(FBlackboard::FKey KeyID, FOnBlackboardChange ObserverDelegate)
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

	for (int32 UpdateIndex = 0; UpdateIndex < QueuedUpdates.Num(); UpdateIndex++)
	{
		NotifyObservers(QueuedUpdates[UpdateIndex]);
	}

	QueuedUpdates.Empty();
}

void UBlackboardComponent::NotifyObservers(FBlackboard::FKey KeyID) const
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
			ObserverDelegate.ExecuteIfBound(*this, KeyID);
		}
	}
}

bool UBlackboardComponent::IsCompatibleWith(UBlackboardData* TestAsset) const
{
	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		if (It == TestAsset)
		{
			return true;
		}

		if (It->Keys == TestAsset->Keys)
		{
			return true;
		}
	}

	return false;
}

EBlackboardCompare::Type UBlackboardComponent::CompareKeyValues(TSubclassOf<UBlackboardKeyType> KeyType, FBlackboard::FKey KeyA, FBlackboard::FKey KeyB) const
{	
	return GetDefault<UBlackboardKeyType>()->Compare(GetKeyRawData(KeyA), GetKeyRawData(KeyB));
}

FString UBlackboardComponent::GetDebugInfoString(EBlackboardDescription::Type Mode) const 
{
	FString DebugString = FString::Printf(TEXT("Blackboard (asset: %s)\n"), *GetNameSafe(BlackboardAsset));

	TArray<FString> KeyDesc;
	uint8 Offset = 0;
	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); KeyIndex++)
		{
			KeyDesc.Add(DescribeKeyValue(KeyIndex + Offset, Mode));
		}
		Offset += It->Keys.Num();
	}
	
	KeyDesc.Sort();
	for (int32 KeyDescIndex = 0; KeyDescIndex < KeyDesc.Num(); KeyDescIndex++)
	{
		DebugString += TEXT("  ");
		DebugString += KeyDesc[KeyDescIndex];
		DebugString += TEXT('\n');
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
				const FBlackboard::FKey KeyID = ObserversKeys[KeyIndex];
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

FString UBlackboardComponent::DescribeKeyValue(const FName& KeyName, EBlackboardDescription::Type Mode) const
{
	return DescribeKeyValue(GetKeyID(KeyName), Mode);
}

FString UBlackboardComponent::DescribeKeyValue(FBlackboard::FKey KeyID, EBlackboardDescription::Type Mode) const
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

void UBlackboardComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("Blackboard (asset: %s)"), *GetNameSafe(BlackboardAsset));

	for (UBlackboardData* It = BlackboardAsset; It; It = It->Parent)
	{
		for (int32 KeyIndex = 0; KeyIndex < It->Keys.Num(); KeyIndex++)
		{
			const FBlackboardEntry& Key = It->Keys[KeyIndex];

			const uint8* ValueData = GetKeyRawData(It->GetFirstKeyID() + KeyIndex);
			FString ValueDesc = Key.KeyType ? *(Key.KeyType->DescribeValue(ValueData)) : TEXT("empty");

			Category.Add(Key.EntryName.ToString(), ValueDesc);
		}
	}

	Category.Data.Sort();
	Snapshot->Status.Add(Category);
}

#endif

UObject* UBlackboardComponent::GetValueAsObject(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Object::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsObject(KeyID);
}

UObject* UBlackboardComponent::GetValueAsObject(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Object::GetValue(RawData) : NULL;
}

UClass* UBlackboardComponent::GetValueAsClass(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Class::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsClass(KeyID);
}

UClass* UBlackboardComponent::GetValueAsClass(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Class::GetValue(RawData) : NULL;
}

uint8 UBlackboardComponent::GetValueAsEnum(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Enum::StaticClass() &&
		GetKeyType(KeyID) != UBlackboardKeyType_NativeEnum::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsEnum(KeyID);
}

uint8 UBlackboardComponent::GetValueAsEnum(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Enum::GetValue(RawData) : 0;
}

int32 UBlackboardComponent::GetValueAsInt(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Int::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsInt(KeyID);
}

int32 UBlackboardComponent::GetValueAsInt(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Int::GetValue(RawData) : 0;
}

float UBlackboardComponent::GetValueAsFloat(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Float::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsFloat(KeyID);
}

float UBlackboardComponent::GetValueAsFloat(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Float::GetValue(RawData) : 0.0f;
}

bool UBlackboardComponent::GetValueAsBool(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Bool::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsBool(KeyID);
}

bool UBlackboardComponent::GetValueAsBool(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Bool::GetValue(RawData) : false;
}

FString UBlackboardComponent::GetValueAsString(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_String::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsString(KeyID);
}

FString UBlackboardComponent::GetValueAsString(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_String::GetValue(RawData) : FString();
}

FName UBlackboardComponent::GetValueAsName(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Name::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsName(KeyID);
}

FName UBlackboardComponent::GetValueAsName(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Name::GetValue(RawData) : NAME_None;
}

FVector UBlackboardComponent::GetValueAsVector(const FName& KeyName) const
{
	FBlackboard::FKey KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Vector::StaticClass())
	{
		KeyID = FBlackboard::InvalidKey;
	}

	return GetValueAsVector(KeyID);
}

FVector UBlackboardComponent::GetValueAsVector(FBlackboard::FKey KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Vector::GetValue(RawData) : FAISystem::InvalidLocation;
}


FRotator UBlackboardComponent::GetValueAsRotator(const FName& KeyName) const
{
	uint8 KeyID = GetKeyID(KeyName);
	if (GetKeyType(KeyID) != UBlackboardKeyType_Rotator::StaticClass())
	{
		KeyID = INDEX_NONE;
	}

	return GetValueAsRotator(KeyID);
}

FRotator UBlackboardComponent::GetValueAsRotator(uint8 KeyID) const
{
	const uint8* RawData = GetKeyRawData(KeyID);
	return RawData ? UBlackboardKeyType_Rotator::GetValue(RawData) : FRotator::ZeroRotator;
}

void UBlackboardComponent::SetValueAsObject(const FName& KeyName, UObject* ObjectValue)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsObject(KeyID, ObjectValue);
}

void UBlackboardComponent::SetValueAsObject(FBlackboard::FKey KeyID, UObject* ObjectValue)
{
	if ( GetKeyType(KeyID) != UBlackboardKeyType_Object::StaticClass() )
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsClass(KeyID, ClassValue);
}

void UBlackboardComponent::SetValueAsClass(FBlackboard::FKey KeyID, UClass* ClassValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Class::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsEnum(KeyID, EnumValue);
}

void UBlackboardComponent::SetValueAsEnum(FBlackboard::FKey KeyID, uint8 EnumValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Enum::StaticClass() &&
		GetKeyType(KeyID) != UBlackboardKeyType_NativeEnum::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsInt(KeyID, IntValue);
}

void UBlackboardComponent::SetValueAsInt(FBlackboard::FKey KeyID, int32 IntValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Int::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsFloat(KeyID, FloatValue);
}

void UBlackboardComponent::SetValueAsFloat(FBlackboard::FKey KeyID, float FloatValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Float::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsBool(KeyID, BoolValue);
}

void UBlackboardComponent::SetValueAsBool(FBlackboard::FKey KeyID, bool BoolValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Bool::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsString(KeyID, StringValue);
}

void UBlackboardComponent::SetValueAsString(FBlackboard::FKey KeyID, const FString& StringValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_String::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsName(KeyID, NameValue);
}

void UBlackboardComponent::SetValueAsName(FBlackboard::FKey KeyID, const FName& NameValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Name::StaticClass())
	{
		return;
	}

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
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsVector(KeyID, VectorValue);
}

void UBlackboardComponent::SetValueAsVector(FBlackboard::FKey KeyID, const FVector& VectorValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Vector::StaticClass())
	{
		return;
	}

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


void UBlackboardComponent::SetValueAsRotator(const FName& KeyName, FRotator RotatorValue)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	SetValueAsRotator(KeyID, RotatorValue);
}

void UBlackboardComponent::SetValueAsRotator(uint8 KeyID, const FRotator& RotatorValue)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Rotator::StaticClass())
	{
		return;
	}

	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Rotator::SetValue(RawData, RotatorValue);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::ClearValueAsVector(const FName& KeyName)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	ClearValueAsVector(KeyID);
}

void UBlackboardComponent::ClearValueAsVector(FBlackboard::FKey KeyID)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Vector::StaticClass())
	{
		return;
	}

	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Vector::SetValue(RawData, FAISystem::InvalidLocation);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

bool UBlackboardComponent::IsVectorValueSet(const FName& KeyName) const
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	return IsVectorValueSet(KeyID);
}

bool UBlackboardComponent::IsVectorValueSet(FBlackboard::FKey KeyID) const
{
	FVector VectorValue = GetValueAsVector(KeyID);
	if (VectorValue == FAISystem::InvalidLocation)
	{
		return false;
	}

	return true;
}

void UBlackboardComponent::ClearValueAsRotator(const FName& KeyName)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	ClearValueAsRotator(KeyID);
}

void UBlackboardComponent::ClearValueAsRotator(uint8 KeyID)
{
	if (GetKeyType(KeyID) != UBlackboardKeyType_Rotator::StaticClass())
	{
		return;
	}

	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		const bool bChanged = UBlackboardKeyType_Rotator::SetValue(RawData, FAISystem::InvalidRotation);
		if (bChanged)
		{
			NotifyObservers(KeyID);
		}
	}
}

void UBlackboardComponent::ClearValue(const FName& KeyName)
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	ClearValue(KeyID);
}

void UBlackboardComponent::ClearValue(FBlackboard::FKey KeyID)
{
	uint8* RawData = GetKeyRawData(KeyID);
	if (RawData)
	{
		TSubclassOf<UBlackboardKeyType> ValueType = GetKeyType(KeyID);
		if (ValueType)
		{
			// c-cast is ok here since ValueType is of at least UBlackboardKeyType and it's default object has to be UBlackboardKeyType instance
			const bool bChanged = GetDefault<UBlackboardKeyType>()->Clear(RawData);
			if (bChanged)
			{
				NotifyObservers(KeyID);
			}
		}
	}
}

bool UBlackboardComponent::GetLocationFromEntry(const FName& KeyName, FVector& ResultLocation) const
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	return GetLocationFromEntry(KeyID, ResultLocation);
}

bool UBlackboardComponent::GetLocationFromEntry(FBlackboard::FKey KeyID, FVector& ResultLocation) const
{
	if (BlackboardAsset && ValueOffsets.IsValidIndex(KeyID))
	{
		const FBlackboardEntry* EntryInfo = BlackboardAsset->GetKey(KeyID);
		if (EntryInfo && EntryInfo->KeyType)
		{
			const uint8* ValueData = ValueMemory.GetData() + ValueOffsets[KeyID];
			return EntryInfo->KeyType->GetLocation(ValueData, ResultLocation);
		}
	}

	return false;
}

bool UBlackboardComponent::GetRotationFromEntry(const FName& KeyName, FRotator& ResultRotation) const
{
	const FBlackboard::FKey KeyID = GetKeyID(KeyName);
	return GetRotationFromEntry(KeyID, ResultRotation);
}

bool UBlackboardComponent::GetRotationFromEntry(FBlackboard::FKey KeyID, FRotator& ResultRotation) const
{
	if (BlackboardAsset && ValueOffsets.IsValidIndex(KeyID))
	{
		const FBlackboardEntry* EntryInfo = BlackboardAsset->GetKey(KeyID);
		if (EntryInfo && EntryInfo->KeyType)
		{
			const uint8* ValueData = ValueMemory.GetData() + ValueOffsets[KeyID];
			return EntryInfo->KeyType->GetRotation(ValueData, ResultRotation);
		}
	}

	return false;
}
