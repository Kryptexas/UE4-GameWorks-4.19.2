// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/StructOnScope.h"
#include "UserDefinedStruct.generated.h"

class UUserDefinedStruct;
class UUserDefinedStructEditorData;

UENUM()
enum EUserDefinedStructureStatus
{
	/** Struct is in an unknown state. */
	UDSS_UpToDate,
	/** Struct has been modified but not recompiled. */
	UDSS_Dirty,
	/** Struct tried but failed to be compiled. */
	UDSS_Error,
	/** Struct is a duplicate, the original one was changed. */
	UDSS_Duplicate,

	UDSS_MAX,
};

/** Wrapper for StructOnScope that tells it to ignore default values */
class ENGINE_API FUserStructOnScopeIgnoreDefaults : public FStructOnScope
{
public:
	/** Constructor with no script struct, call Recreate later */
	FUserStructOnScopeIgnoreDefaults() : FStructOnScope() {}

	/** Constructor that initializes for you */
	FUserStructOnScopeIgnoreDefaults(const UUserDefinedStruct* InUserStruct);

	/** Initialize from existing data, will free when scope closes */
	FUserStructOnScopeIgnoreDefaults(const UUserDefinedStruct* InUserStruct, uint8* InData);

	/** Destroys and creates new struct */
	void Recreate(const UUserDefinedStruct* InUserStruct);

	virtual void Initialize() override;
};

UCLASS()
class ENGINE_API UUserDefinedStruct : public UScriptStruct
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITORONLY_DATA
	/** The original struct, when current struct isn't a temporary duplicate, the field should be null */
	UPROPERTY(Transient)
	TWeakObjectPtr<UUserDefinedStruct> PrimaryStruct;

	UPROPERTY()
	FString ErrorMessage;

	UPROPERTY()
	UObject* EditorData;
#endif // WITH_EDITORONLY_DATA

	/** Status of this struct, outside of the editor it is assumed to always be UpToDate */
	UPROPERTY()
	TEnumAsByte<enum EUserDefinedStructureStatus> Status;

	/** Uniquely identifies this specific user struct */
	UPROPERTY()
	FGuid Guid;

protected:
	/** Default instance of this struct with default values filled in, used to initialize structure */
	FUserStructOnScopeIgnoreDefaults DefaultStructInstance;

	/** Bool to indicate we want to initialize a version of this struct without defaults, this is set while allocating the DefaultStructInstance itself */
	bool bIgnoreStructDefaults;

public:
#if WITH_EDITOR
	// UObject interface.
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void PostLoad() override;
	// End of UObject interface.

	// UScriptStruct interface.
	virtual UProperty* CustomFindProperty(const FName Name) const override;
	// End of UScriptStruct interface.

	/** Creates a new guid if needed */
	void ValidateGuid();

	friend UUserDefinedStructEditorData;
#endif	// WITH_EDITOR

	// UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void SerializeTaggedProperties(FArchive& Ar, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad=NULL) const override;
	virtual FString PropertyNameToDisplayName(FName InName) const override;
	// End of UObject interface.

	// UScriptStruct interface.
	virtual void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;
	virtual uint32 GetStructTypeHash(const void* Src) const override;
	virtual void RecursivelyPreload() override;
	virtual FGuid GetCustomGuid() const override;
	virtual FString GetStructCPPName() const override;
	// End of  UScriptStruct interface.

	/** Returns the raw memory of the default instance */
	const uint8* GetDefaultInstance() const;

	/** Specifically initialize this struct without using the default instance data */
	void InitializeStructIgnoreDefaults(void* Dest, int32 ArrayDim = 1) const;

	/** Computes hash */
	static uint32 GetUserDefinedStructTypeHash(const void* Src, const UScriptStruct* Type);

	/** returns references from default instance */
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

};
