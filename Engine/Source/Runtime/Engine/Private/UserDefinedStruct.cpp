// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Engine/UserDefinedStruct.h"
#include "UObject/UObjectHash.h"
#include "Serialization/PropertyLocalizationDataGathering.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UObject/LinkerLoad.h"
#include "UObject/FrameworkObjectVersion.h"
#include "Misc/SecureHash.h"
#include "UObject/PropertyPortFlags.h"
#include "Misc/PackageName.h"
#include "Serialization/TextReferenceCollector.h"
#include "Blueprint/BlueprintSupport.h"

#if WITH_EDITOR
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "Kismet2/StructureEditorUtils.h"
#endif //WITH_EDITOR

FUserStructOnScopeIgnoreDefaults::FUserStructOnScopeIgnoreDefaults(const UUserDefinedStruct* InUserStruct)
{
	// Can't call super constructor because we need to call our overridden initialize
	ScriptStruct = InUserStruct;
	SampleStructMemory = nullptr;
	OwnsMemory = false;
	Initialize();
}

FUserStructOnScopeIgnoreDefaults::FUserStructOnScopeIgnoreDefaults(const UUserDefinedStruct* InUserStruct, uint8* InData) : FStructOnScope(InUserStruct, InData)
{
}

void FUserStructOnScopeIgnoreDefaults::Recreate(const UUserDefinedStruct* InUserStruct)
{
	Destroy();
	ScriptStruct = InUserStruct;
	Initialize();
}

void FUserStructOnScopeIgnoreDefaults::Initialize()
{
	if (ScriptStruct.IsValid())
	{
		SampleStructMemory = (uint8*)FMemory::Malloc(ScriptStruct->GetStructureSize() ? ScriptStruct->GetStructureSize() : 1);
		((UUserDefinedStruct*)ScriptStruct.Get())->InitializeStructIgnoreDefaults(SampleStructMemory);
		OwnsMemory = true;
	}
}

#if WITH_EDITORONLY_DATA
namespace
{
	void GatherUserDefinedStructForLocalization(const UObject* const Object, FPropertyLocalizationDataGatherer& PropertyLocalizationDataGatherer, const EPropertyLocalizationGathererTextFlags GatherTextFlags)
	{
		const UUserDefinedStruct* const UserDefinedStruct = CastChecked<UUserDefinedStruct>(Object);

		PropertyLocalizationDataGatherer.GatherLocalizationDataFromObject(UserDefinedStruct, GatherTextFlags);

		const FString PathToObject = UserDefinedStruct->GetPathName();

		PropertyLocalizationDataGatherer.GatherLocalizationDataFromStructFields(PathToObject, UserDefinedStruct, UserDefinedStruct->GetDefaultInstance(), nullptr, GatherTextFlags);
	}

	void CollectUserDefinedStructTextReferences(UObject* Object, FArchive& Ar)
	{
		UUserDefinedStruct* const UserDefinedStruct = CastChecked<UUserDefinedStruct>(Object);

		// User Defined Structs need some special handling as they store their default data in a way that serialize doesn't pick up
		UUserDefinedStructEditorData* UDSEditorData = Cast<UUserDefinedStructEditorData>(UserDefinedStruct->EditorData);
		if (UDSEditorData)
		{
			for (const FStructVariableDescription& StructVariableDesc : UDSEditorData->VariablesDescriptions)
			{
				static const FName TextCategory = TEXT("text"); // Must match UEdGraphSchema_K2::PC_Text
				if (StructVariableDesc.Category == TextCategory)
				{
					FText StructVariableValue;
					if (FTextStringHelper::ReadFromString(*StructVariableDesc.DefaultValue, StructVariableValue))
					{
						Ar << StructVariableValue;
					}
				}
			}
		}

		UserDefinedStruct->Serialize(Ar);
	}
}
#endif

UUserDefinedStruct::UUserDefinedStruct(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultStructInstance.SetPackage(GetOutermost());

#if WITH_EDITORONLY_DATA
	{ static const FAutoRegisterLocalizationDataGatheringCallback AutomaticRegistrationOfLocalizationGatherer(UUserDefinedStruct::StaticClass(), &GatherUserDefinedStructForLocalization); }
	{ static const FAutoRegisterTextReferenceCollectorCallback AutomaticRegistrationOfTextReferenceCollector(UUserDefinedStruct::StaticClass(), &CollectUserDefinedStructTextReferences); }
#endif
}

void UUserDefinedStruct::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);

	if (Ar.CustomVer(FFrameworkObjectVersion::GUID) >= FFrameworkObjectVersion::UserDefinedStructsStoreDefaultInstance)
	{
		if (EUserDefinedStructureStatus::UDSS_UpToDate == Status && !(Ar.GetPortFlags() & PPF_Duplicate))
		{
			// If we're saving or loading new data, serialize our defaults
			if (!DefaultStructInstance.IsValid() && Ar.IsLoading())
			{
				DefaultStructInstance.Recreate(this);
			}

			uint8* StructData = DefaultStructInstance.GetStructMemory();

			FScopedPlaceholderRawContainerTracker TrackStruct(StructData);
			SerializeItem(Ar, StructData, nullptr);
		}
	}

#if WITH_EDITOR
	if (Ar.IsLoading())
	{
		if (Ar.CustomVer(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::UserDefinedStructsBlueprintVisible)
		{
			for (TFieldIterator<UProperty> PropIt(this); PropIt; ++PropIt)
			{
				PropIt->PropertyFlags |= CPF_BlueprintVisible;
			}
		}

		if (EUserDefinedStructureStatus::UDSS_UpToDate == Status)
		{
			// We need to force the editor data to be preload in case anyone needs to extract variable
			// information at editor time about the user structure.
			if (EditorData != nullptr)
			{
				Ar.Preload(EditorData);
				if (!(Ar.GetPortFlags() & PPF_Duplicate))
				{
					FStructureEditorUtils::RecreateDefaultInstanceInEditorData(this);
				}
			}

			const FStructureEditorUtils::EStructureError Result = FStructureEditorUtils::IsStructureValid(this, NULL, &ErrorMessage);
			if (FStructureEditorUtils::EStructureError::Ok != Result)
			{
				Status = EUserDefinedStructureStatus::UDSS_Error;
				UE_LOG(LogClass, Log, TEXT("UUserDefinedStruct.Serialize '%s' validation: %s"), *GetName(), *ErrorMessage);
			}
		}
	}
#endif
}

#if WITH_EDITOR

void UUserDefinedStruct::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if (!bDuplicateForPIE)
	{
		Guid = FGuid::NewGuid();
	}
	if (!bDuplicateForPIE && (GetOuter() != GetTransientPackage()))
	{
		SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		FStructureEditorUtils::OnStructureChanged(this);
	}
}

void UUserDefinedStruct::PostLoad()
{
	Super::PostLoad();

	ValidateGuid();
}

void UUserDefinedStruct::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);

	OutTags.Add(FAssetRegistryTag(TEXT("Tooltip"), FStructureEditorUtils::GetTooltip(this), FAssetRegistryTag::TT_Hidden));
}

UProperty* UUserDefinedStruct::CustomFindProperty(const FName Name) const
{
	const FGuid PropertyGuid = FStructureEditorUtils::GetGuidFromPropertyName(Name);
	UProperty* Property = PropertyGuid.IsValid() ? FStructureEditorUtils::GetPropertyByGuid(this, PropertyGuid) : FStructureEditorUtils::GetPropertyByDisplayName(this, Name.ToString());
	ensure(!Property || !PropertyGuid.IsValid() || PropertyGuid == FStructureEditorUtils::GetGuidForProperty(Property));
	return Property;
}

void UUserDefinedStruct::ValidateGuid()
{
	// Backward compatibility:
	// The guid is created in an deterministic way using existing name.
	if (!Guid.IsValid() && (GetFName() != NAME_None))
	{
		const FString HashString = GetFName().ToString();
		ensure(HashString.Len());

		const uint32 BufferLength = HashString.Len() * sizeof(HashString[0]);
		uint32 HashBuffer[5];
		FSHA1::HashBuffer(*HashString, BufferLength, reinterpret_cast<uint8*>(HashBuffer));
		Guid = FGuid(HashBuffer[1], HashBuffer[2], HashBuffer[3], HashBuffer[4]);
	}
}

#endif	// WITH_EDITOR

FString UUserDefinedStruct::PropertyNameToDisplayName(FName Name) const
{
#if WITH_EDITOR
	FGuid PropertyGuid = FStructureEditorUtils::GetGuidFromPropertyName(Name);
	return FStructureEditorUtils::GetVariableDisplayName(this, PropertyGuid);
#endif	// WITH_EDITOR

	const int32 GuidStrLen = 32;
	const int32 MinimalPostfixlen = GuidStrLen + 3;
	const FString OriginalName = Name.ToString();
	if (OriginalName.Len() > MinimalPostfixlen)
	{
		FString DisplayName = OriginalName.LeftChop(GuidStrLen + 1);
		int FirstCharToRemove = -1;
		const bool bCharFound = DisplayName.FindLastChar(TCHAR('_'), FirstCharToRemove);
		if(bCharFound && (FirstCharToRemove > 0))
		{
			return DisplayName.Mid(0, FirstCharToRemove);
		}
	}
	return OriginalName;
}

void UUserDefinedStruct::InitializeStructIgnoreDefaults(void* Dest, int32 ArrayDim) const
{
	Super::InitializeStruct(Dest, ArrayDim);
}

void UUserDefinedStruct::InitializeStruct(void* Dest, int32 ArrayDim) const
{
	InitializeStructIgnoreDefaults(Dest, ArrayDim);

	if (Dest)
	{
		const uint8* DefaultInstance = GetDefaultInstance();
		if (DefaultInstance)
		{
			int32 Stride = GetStructureSize();

			for (int32 ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++)
			{
				void* DestStruct = (uint8*)Dest + (Stride * ArrayIndex);
				CopyScriptStruct(DestStruct, DefaultInstance);

				// When copying into another struct we need to register this raw struct pointer so any deferred dependencies will get fixed later
				FScopedPlaceholderRawContainerTracker TrackStruct(DestStruct);
				FBlueprintSupport::RegisterDeferredDependenciesInStruct(this, DestStruct);
			}	
		}
	}
}

void UUserDefinedStruct::SerializeTaggedProperties(FArchive& Ar, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad) const
{
	bool bTemporarilyEnableDelta = false;

#if WITH_EDITOR
	// In the editor the default structure may change while the editor is running, so we need to always delta serialize
	UUserDefinedStruct* UDDefaultsStruct = Cast<UUserDefinedStruct>(DefaultsStruct);

	const bool bDuplicate = (0 != (Ar.GetPortFlags() & PPF_Duplicate));

	// When saving delta, we want the difference between current data and true structure's default values.
	// So if we don't have defaults we need to use the struct defaults
	const bool bUseNewDefaults = !Defaults
		&& UDDefaultsStruct
		&& !bDuplicate
		&& (Ar.IsSaving() || Ar.IsLoading())
		&& !Ar.IsCooking();

	if (bUseNewDefaults)
	{
		Defaults = const_cast<uint8*>(UDDefaultsStruct->GetDefaultInstance());
	}

	// Temporarily enable delta serialization if this is a CPFUO 
	bTemporarilyEnableDelta = bUseNewDefaults && Ar.IsIgnoringArchetypeRef() && Ar.IsIgnoringClassRef() && !Ar.DoDelta();
	if (bTemporarilyEnableDelta)
	{
		Ar.ArNoDelta = false;
	}
#endif // WITH_EDITOR

	Super::SerializeTaggedProperties(Ar, Data, DefaultsStruct, Defaults);

	if (bTemporarilyEnableDelta)
	{
		Ar.ArNoDelta = true;
	}
}

uint32 UUserDefinedStruct::GetStructTypeHash(const void* Src) const
{
	return GetUserDefinedStructTypeHash(Src, this);
}

void UUserDefinedStruct::RecursivelyPreload()
{
	FLinkerLoad* Linker = GetLinker();
	if( Linker && (NULL == PropertyLink) )
	{
		TArray<UObject*> AllChildMembers;
		GetObjectsWithOuter(this, AllChildMembers);
		for (int32 Index = 0; Index < AllChildMembers.Num(); ++Index)
		{
			UObject* Member = AllChildMembers[Index];
			Linker->Preload(Member);
		}

		Linker->Preload(this);
		if (NULL == PropertyLink)
		{
			StaticLink(true);
		}
	}
}

FGuid UUserDefinedStruct::GetCustomGuid() const
{
	return Guid;
}

ENGINE_API FString GetPathPostfix(const UObject* ForObject)
{
	FString FullAssetName = ForObject->GetOutermost()->GetPathName();
	if (FullAssetName.StartsWith(TEXT("/Temp/__TEMP_BP__"), ESearchCase::CaseSensitive))
	{
		FullAssetName.RemoveFromStart(TEXT("/Temp/__TEMP_BP__"), ESearchCase::CaseSensitive);
	}
	FString AssetName = FPackageName::GetLongPackageAssetName(FullAssetName);
	// append a hash of the path, this uniquely identifies assets with the same name, but different folders:
	FullAssetName.RemoveFromEnd(AssetName);
	return FString::Printf(TEXT("%u"), GetTypeHash(FullAssetName));
}

FString UUserDefinedStruct::GetStructCPPName() const
{
	return ::UnicodeToCPPIdentifier(*GetName(), false, GetPrefixCPP()) + GetPathPostfix(this);
}

uint32 UUserDefinedStruct::GetUserDefinedStructTypeHash(const void* Src, const UScriptStruct* Type)
{
	// Ugliness to maximize entropy on first call to HashCombine... Alternatively we could
	// do special logic on the first iteration of the below loops (unwind loops or similar):
	const auto ConditionalCombineHash = [](uint32 AccumulatedHash, uint32 CurrentHash) -> uint32
	{
		if (AccumulatedHash != 0)
		{
			return HashCombine(AccumulatedHash, CurrentHash);
		}
		else
		{
			return CurrentHash;
		}
	};

	uint32 ValueHash = 0;
	// combining bool values and hashing them together, small range enums could get stuffed into here as well,
	// but UBoolProperty does not actually provide GetValueTypeHash (and probably shouldn't). For structs
	// with more than 64 boolean values we lose some information, but that is acceptable, just slightly 
	// increasing risk of hash collision:
	bool bHasBoolValues = false;
	uint64 BoolValues = 0;
	// for blueprint defined structs we can just loop and hash the individual properties:
	for (TFieldIterator<const UProperty> It(Type); It; ++It)
	{
		uint32 CurrentHash = 0;
		if (const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(*It))
		{
			for (int32 I = 0; I < It->ArrayDim; ++I)
			{
				BoolValues = (BoolValues << 1) + ( BoolProperty->GetPropertyValue_InContainer(Src, I) ? 1 : 0 );
			}
		}
		else if (ensure(It->HasAllPropertyFlags(CPF_HasGetValueTypeHash)))
		{
			for (int32 I = 0; I < It->ArrayDim; ++I)
			{
				uint32 Hash = It->GetValueTypeHash(It->ContainerPtrToValuePtr<void>(Src, I));
				CurrentHash = ConditionalCombineHash(CurrentHash, Hash);
			}
		}

		ValueHash = ConditionalCombineHash(ValueHash, CurrentHash);
	}

	if (bHasBoolValues)
	{
		ValueHash = ConditionalCombineHash(ValueHash, GetTypeHash(BoolValues));
	}

	return ValueHash;
}

const uint8* UUserDefinedStruct::GetDefaultInstance() const
{
	ensure(DefaultStructInstance.IsValid() && DefaultStructInstance.GetStruct() == this);
	return DefaultStructInstance.GetStructMemory();
}

void UUserDefinedStruct::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UUserDefinedStruct* This = CastChecked<UUserDefinedStruct>(InThis);

	ensure(!This->DefaultStructInstance.IsValid() || This->DefaultStructInstance.GetStruct() == This);
	uint8* StructData = This->DefaultStructInstance.GetStructMemory();
	if (StructData)
	{
		FVerySlowReferenceCollectorArchiveScope CollectorScope(Collector.GetVerySlowReferenceCollectorArchive(), This);
		This->SerializeBin(CollectorScope.GetArchive(), StructData);
	}

	Super::AddReferencedObjects(This, Collector);
}