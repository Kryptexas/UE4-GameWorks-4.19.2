// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HotReloadPrivatePCH.h"
#include "HotReloadClassReinstancer.h"

#if WITH_ENGINE

void FHotReloadClassReinstancer::SetupNewClassReinstancing(UClass* InNewClass, UClass* InOldClass)
{
	// Set base class members to valid values
	ClassToReinstance = InNewClass;
	DuplicatedClass = InOldClass;
	OriginalCDO = InOldClass->GetDefaultObject();
	bHasReinstanced = false;
	bSkipGarbageCollection = false;
	bNeedsReinstancing = true;

	SaveClassFieldMapping(InOldClass);

	TArray<UClass*> ChildrenOfClass;
	GetDerivedClasses(InOldClass, ChildrenOfClass);
	for (auto ClassIt = ChildrenOfClass.CreateConstIterator(); ClassIt; ++ClassIt)
	{
		UClass* ChildClass = *ClassIt;
		UBlueprint* ChildBP = Cast<UBlueprint>(ChildClass->ClassGeneratedBy);
		if (ChildBP && !ChildBP->HasAnyFlags(RF_BeingRegenerated))
		{
			// If this is a direct child, change the parent and relink so the property chain is valid for reinstancing
			if (!ChildBP->HasAnyFlags(RF_NeedLoad))
			{
				if (ChildClass->GetSuperClass() == InOldClass)
				{
					ReparentChild(ChildBP);
				}

				Children.AddUnique(ChildBP);
			}
			else
			{
				// If this is a child that caused the load of their parent, relink to the REINST class so that we can still serialize in the CDO, but do not add to later processing
				ReparentChild(ChildClass);
			}
		}
	}

	// Finally, remove the old class from Root so that it can get GC'd and mark it as CLASS_NewerVersionExists
	InOldClass->RemoveFromRoot();
	InOldClass->ClassFlags |= CLASS_NewerVersionExists;
}

void FHotReloadClassReinstancer::SerializeCDOProperties(UObject* InObject, TArray<uint8>& OutData)
{
	// Creates a mem-comparable CDO data
	class FCDOWriter : public FMemoryWriter
	{
		/** Objects already visited by this archive */
		TSet<UObject*>& VisitedObjects;

	public:
		/** Serializes all script properties of the provided DefaultObject */
		FCDOWriter(TArray<uint8>& InBytes, UObject* DefaultObject, TSet<UObject*>& InVisitedObjects)
			: FMemoryWriter(InBytes, /* bIsPersistent = */ false, /* bSetOffset = */ true)
			, VisitedObjects(InVisitedObjects)
		{
			DefaultObject->SerializeScriptProperties(*this);
		}
		/** Serializes an object. Only name and class for normal references, deep serialization for DSOs */
		virtual FArchive& operator<<(class UObject*& InObj) override
		{
			FArchive& Ar = *this;
			if (InObj)
			{
				FName ClassName = InObj->GetClass()->GetFName();
				FName ObjectName = InObj->GetFName();
				Ar << ClassName;
				Ar << ObjectName;
				if (!VisitedObjects.Contains(InObj))
				{
					VisitedObjects.Add(InObj);
					if (Ar.GetSerializedProperty() && Ar.GetSerializedProperty()->ContainsInstancedObjectProperty())
					{
						// Serialize all DSO properties too					
						FCDOWriter DefaultSubobjectWriter(Bytes, InObj, VisitedObjects);
					}
				}
			}
			else
			{
				FName UnusedName = NAME_None;
				Ar << UnusedName;
				Ar << UnusedName;
			}

			return *this;
		}
		/** Serializes an FName as its index and number */
		virtual FArchive& operator<<(FName& InName) override
		{
			FArchive& Ar = *this;
			NAME_INDEX ComparisonIndex = InName.GetComparisonIndex();
			NAME_INDEX DisplayIndex = InName.GetDisplayIndex();
			int32 Number = InName.GetNumber();
			Ar << ComparisonIndex;
			Ar << DisplayIndex;
			Ar << Number;
			return Ar;
		}
		virtual FArchive& operator<<(FLazyObjectPtr& LazyObjectPtr) override
		{
			FArchive& Ar = *this;
			auto UniqueID = LazyObjectPtr.GetUniqueID();
			Ar << UniqueID;
			return *this;
		}
		virtual FArchive& operator<<(FAssetPtr& AssetPtr) override
		{
			FArchive& Ar = *this;
			auto UniqueID = AssetPtr.GetUniqueID();
			Ar << UniqueID;
			return Ar;
		}
		virtual FArchive& operator<<(FStringAssetReference& Value) override
		{
			FArchive& Ar = *this;
			Ar << Value.AssetLongPathname;
			return Ar;
		}
		/** Archive name, for debugging */
		virtual FString GetArchiveName() const override { return TEXT("FCDOWriter"); }
	};
	TSet<UObject*> VisitedObjects;
	VisitedObjects.Add(InObject);
	FCDOWriter Ar(OutData, InObject, VisitedObjects);
}

void FHotReloadClassReinstancer::ReconstructClassDefaultObject(UClass* InOldClass)
{
	// Remember all the basic info about the object before we destroy it
	UObject* OldCDO = InOldClass->GetDefaultObject();
	EObjectFlags CDOFlags = OldCDO->GetFlags();
	UObject* CDOOuter = OldCDO->GetOuter();
	FName CDOName = OldCDO->GetFName();

	// Get the parent CDO
	UClass* ParentClass = InOldClass->GetSuperClass();
	UObject* ParentDefaultObject = NULL;
	if (ParentClass != NULL)
	{
		ParentDefaultObject = ParentClass->GetDefaultObject(); // Force the default object to be constructed if it isn't already
	}

	if (!OldCDO->HasAnyFlags(RF_FinishDestroyed))
	{
		// Begin the asynchronous object cleanup.
		OldCDO->ConditionalBeginDestroy();

		// Wait for the object's asynchronous cleanup to finish.
		while (!OldCDO->IsReadyForFinishDestroy())
		{
			FPlatformProcess::Sleep(0);
		}
		// Finish destroying the object.
		OldCDO->ConditionalFinishDestroy();
	}
	OldCDO->~UObject();

	// Re-create
	FMemory::Memzero((void*)OldCDO, InOldClass->GetPropertiesSize());
	new ((void *)OldCDO) UObjectBase(InOldClass, CDOFlags, CDOOuter, CDOName);
	const bool bShouldInitilizeProperties = false;
	const bool bCopyTransientsFromClassDefaults = false;
	(*InOldClass->ClassConstructor)(FObjectInitializer(OldCDO, ParentDefaultObject, bCopyTransientsFromClassDefaults, bShouldInitilizeProperties));
}

void FHotReloadClassReinstancer::RecreateCDOAndSetupOldClassReinstancing(UClass* InOldClass)
{
	// Set base class members to valid values
	ClassToReinstance = InOldClass;
	DuplicatedClass = InOldClass;
	OriginalCDO = InOldClass->GetDefaultObject();
	bHasReinstanced = false;
	bSkipGarbageCollection = false;
	bNeedsReinstancing = false;

	// Collect the original property values
	TArray<uint8> OriginalCDOProperties;
	SerializeCDOProperties(InOldClass->GetDefaultObject(), OriginalCDOProperties);
	
	// Destroy and re-create the CDO, re-running its constructor
	ReconstructClassDefaultObject(InOldClass);

	// Collect the property values after re-constructing the CDO
	TArray<uint8> ReconstructedCDOProperties;
	SerializeCDOProperties(InOldClass->GetDefaultObject(), ReconstructedCDOProperties);

	// We only want to re-instance the old class if its CDO's values have changed or any of its DSOs' property values have changed
	if (OriginalCDOProperties.Num() != ReconstructedCDOProperties.Num() ||
		FMemory::Memcmp(OriginalCDOProperties.GetData(), ReconstructedCDOProperties.GetData(), OriginalCDOProperties.Num()))
	{
		bNeedsReinstancing = true;
		SaveClassFieldMapping(InOldClass);

		TArray<UClass*> ChildrenOfClass;
		GetDerivedClasses(InOldClass, ChildrenOfClass);
		for (auto ClassIt = ChildrenOfClass.CreateConstIterator(); ClassIt; ++ClassIt)
		{
			UClass* ChildClass = *ClassIt;
			UBlueprint* ChildBP = Cast<UBlueprint>(ChildClass->ClassGeneratedBy);
			if (ChildBP && !ChildBP->HasAnyFlags(RF_BeingRegenerated))
			{
				if (!ChildBP->HasAnyFlags(RF_NeedLoad))
				{
					Children.AddUnique(ChildBP);
				}
			}
		}
	}
}

FHotReloadClassReinstancer::FHotReloadClassReinstancer(UClass* InNewClass, UClass* InOldClass)
{
	// If InNewClass is NULL, then the old class has not changed after hot-reload.
	// However, we still need to check for changes to its constructor code (CDO values).
	if (InNewClass)
	{
		SetupNewClassReinstancing(InNewClass, InOldClass);
	}
	else
	{
		RecreateCDOAndSetupOldClassReinstancing(InOldClass);
	}
}

FHotReloadClassReinstancer::~FHotReloadClassReinstancer()
{
	// Make sure the base class does not remove the DuplicatedClass from root, we not always want it.
	// For example when we're just reconstructing CDOs. Other cases are handled by HotReloadClassReinstancer.
	DuplicatedClass = NULL;
}

#endif