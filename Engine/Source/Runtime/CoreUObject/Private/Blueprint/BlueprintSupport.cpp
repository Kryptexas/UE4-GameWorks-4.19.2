// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "CoreUObjectPrivate.h"
#include "UObject/LinkerPlaceholderClass.h"
#include "Linker.h"
#include "PropertyTag.h"


/** 
 * Defined in BlueprintSupport.cpp
 * Duplicates all fields of a class in depth-first order. It makes sure that everything contained
 * in a class is duplicated before the class itself, as well as all function parameters before the
 * function itself.
 *
 * @param	StructToDuplicate			Instance of the struct that is about to be duplicated
 * @param	Writer						duplicate writer instance to write the duplicated data to
 */
void FBlueprintSupport::DuplicateAllFields(UStruct* StructToDuplicate, FDuplicateDataWriter& Writer)
{
	// This is a very simple fake topological-sort to make sure everything contained in the class
	// is processed before the class itself is, and each function parameter is processed before the function
	if (StructToDuplicate)
	{
		// Make sure each field gets allocated into the array
		for (TFieldIterator<UField> FieldIt(StructToDuplicate, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
		{
			UField* Field = *FieldIt;

			// Make sure functions also do their parameters and children first
			if (UFunction* Function = dynamic_cast<UFunction*>(Field))
			{
				for (TFieldIterator<UField> FunctionFieldIt(Function, EFieldIteratorFlags::ExcludeSuper); FunctionFieldIt; ++FunctionFieldIt)
				{
					UField* InnerField = *FunctionFieldIt;
					Writer.GetDuplicatedObject(InnerField);
				}
			}

			Writer.GetDuplicatedObject(Field);
		}
	}
}

bool FBlueprintSupport::UseDeferredDependencyLoading()
{
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	static const FBoolConfigValueHelper DeferDependencyLoads(TEXT("Kismet"), TEXT("bDeferDependencyLoads"), GEngineIni);
	return DeferDependencyLoads;
#else
	return false;
#endif
}

bool FBlueprintSupport::UseDeferredCDOSerialization()
{
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	static const FBoolConfigValueHelper DeferCDOSerializations(TEXT("Kismet"), TEXT("bDeferCDOLoadSerialization"), GEngineIni);
	return UseDeferredDependencyLoading() && DeferCDOSerializations;
#else 
	return false;
#endif
}

bool FBlueprintSupport::UseDeferredLoadingForSubsequentLoads()
{
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	static const FBoolConfigValueHelper PropagateDeferredFlagForStructs(TEXT("Kismet"), TEXT("bPropagateDeferredLoadsForStructs"), GEngineIni);
	return UseDeferredDependencyLoading() && PropagateDeferredFlagForStructs;
#else 
	return false;
#endif
}

bool FBlueprintSupport::UseDeferredDependencyVerificationChecks()
{
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	static const FBoolConfigValueHelper TestCheckDeferredDependencies(TEXT("Kismet"), TEXT("bCheckDeferredDependencies"), GEngineIni);
	return UseDeferredDependencyLoading() && TestCheckDeferredDependencies;
#else 
	return false;
#endif
}

/*******************************************************************************
 * FScopedClassDependencyGather
 ******************************************************************************/

#if WITH_EDITOR
UClass* FScopedClassDependencyGather::BatchMasterClass = NULL;
TArray<UClass*> FScopedClassDependencyGather::BatchClassDependencies;

FScopedClassDependencyGather::FScopedClassDependencyGather(UClass* ClassToGather)
	: bMasterClass(false)
{
	// Do NOT track duplication dependencies, as these are intermediate products that we don't care about
	if( !GIsDuplicatingClassForReinstancing )
	{
		if( BatchMasterClass == NULL )
		{
			// If there is no current dependency master, register this class as the master, and reset the array
			BatchMasterClass = ClassToGather;
			BatchClassDependencies.Empty();
			bMasterClass = true;
		}
		else
		{
			// This class was instantiated while another class was gathering dependencies, so record it as a dependency
			BatchClassDependencies.AddUnique(ClassToGather);
		}
	}
}

FScopedClassDependencyGather::~FScopedClassDependencyGather()
{
	// If this gatherer was the initial gatherer for the current scope, process 
	// dependencies (unless compiling on load is explicitly disabled)
	if( bMasterClass && !GForceDisableBlueprintCompileOnLoad )
	{
		for( auto DependencyIter = BatchClassDependencies.CreateIterator(); DependencyIter; ++DependencyIter )
		{
			UClass* Dependency = *DependencyIter;
			if( Dependency->ClassGeneratedBy != BatchMasterClass->ClassGeneratedBy )
			{
				Dependency->ConditionalRecompileClass(&GObjLoaded);
			}
		}

		// Finally, recompile the master class to make sure it gets updated too
		BatchMasterClass->ConditionalRecompileClass(&GObjLoaded);
		
		BatchMasterClass = NULL;
	}
}

TArray<UClass*> const& FScopedClassDependencyGather::GetCachedDependencies()
{
	return BatchClassDependencies;
}
#endif //WITH_EDITOR



/*******************************************************************************
 * ULinkerLoad
 ******************************************************************************/
 
struct FPreloadMembersHelper
{
	static void PreloadMembers(UObject* InObject)
	{
		// Collect a list of all things this element owns
		TArray<UObject*> BPMemberReferences;
		FReferenceFinder ComponentCollector(BPMemberReferences, InObject, false, true, true, true);
		ComponentCollector.FindReferences(InObject);

		// Iterate over the list, and preload everything so it is valid for refreshing
		for (TArray<UObject*>::TIterator it(BPMemberReferences); it; ++it)
		{
			UObject* CurrentObject = *it;
			if (!CurrentObject->HasAnyFlags(RF_LoadCompleted))
			{
				CurrentObject->SetFlags(RF_NeedLoad);
				if (auto Linker = CurrentObject->GetLinker())
				{
					Linker->Preload(CurrentObject);
					PreloadMembers(CurrentObject);
				}
			}
		}
	}

	static void PreloadObject(UObject* InObject)
	{
		if (InObject && !InObject->HasAnyFlags(RF_LoadCompleted))
		{
			InObject->SetFlags(RF_NeedLoad);
			if (ULinkerLoad* Linker = InObject->GetLinker())
			{
				Linker->Preload(InObject);
			}
		}
	}
};

/**
 * Regenerates/Refreshes a blueprint class
 *
 * @param	LoadClass		Instance of the class currently being loaded and which is the parent for the blueprint
 * @param	ExportObject	Current object being exported
 * @return	Returns true if regeneration was successful, otherwise false
 */
bool ULinkerLoad::RegenerateBlueprintClass(UClass* LoadClass, UObject* ExportObject)
{
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	static ULinkerLoad* RegeneratingLinker = nullptr;
	checkSlow(!FBlueprintSupport::UseDeferredDependencyVerificationChecks() || (RegeneratingLinker == nullptr) || (RegeneratingLinker == this));
	TGuardValue<ULinkerLoad*> ReentrantGuard(RegeneratingLinker, this);
#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING

	// determine if somewhere further down the callstack, we're already in this
	// function for this class
	bool const bAlreadyRegenerating = LoadClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated);
	// Flag the class source object, so we know we're already in the process of compiling this class
	LoadClass->ClassGeneratedBy->SetFlags(RF_BeingRegenerated);

	// Cache off the current CDO, and specify the CDO for the load class 
	// manually... do this before we Preload() any children members so that if 
	// one of those preloads subsequently ends up back here for this class, 
	// then the ExportObject is carried along and used in the eventual RegenerateClass() call
	UObject* CurrentCDO = ExportObject;
	check(!bAlreadyRegenerating || (LoadClass->ClassDefaultObject == ExportObject));
	LoadClass->ClassDefaultObject = CurrentCDO;

	// Finish loading the class here, so we have all the appropriate data to copy over to the new CDO
	TArray<UObject*> AllChildMembers;
	GetObjectsWithOuter(LoadClass, /*out*/ AllChildMembers);
	for (int32 Index = 0; Index < AllChildMembers.Num(); ++Index)
	{
		UObject* Member = AllChildMembers[Index];
		Preload(Member);
	}

	// if this was subsequently regenerated from one of the above preloads, then 
	// we don't have to finish this off, it was already done
	bool const bWasSubsequentlyRegenerated = !LoadClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated);
	// @TODO: find some other condition to block this if we've already  
	//        regenerated the class (not just if we've regenerated the class 
	//        from an above Preload(Member))... UBlueprint::RegenerateClass() 
	//        has an internal conditional to block getting into it again, but we
	//        can't check UBlueprint members from this module
	if (!bWasSubsequentlyRegenerated)
	{
		Preload(LoadClass);

		LoadClass->StaticLink(true);
		Preload(CurrentCDO);

		// Make sure that we regenerate any parent classes first before attempting to build a child
		TArray<UClass*> ClassChainOrdered;
		{
			// Just ordering the class hierarchy from root to leafs:
			UClass* ClassChain = LoadClass->GetSuperClass();
			while (ClassChain && ClassChain->ClassGeneratedBy)
			{
				// O(n) insert, but n is tiny because this is a class hierarchy...
				ClassChainOrdered.Insert(ClassChain, 0);
				ClassChain = ClassChain->GetSuperClass();
			}
		}
		for (auto Class : ClassChainOrdered)
		{
			UObject* BlueprintObject = Class->ClassGeneratedBy;
			if (BlueprintObject && BlueprintObject->HasAnyFlags(RF_BeingRegenerated))
			{
				// Always load the parent blueprint here in case there is a circular dependency. This will
				// ensure that the blueprint is fully serialized before attempting to regenerate the class.
				FPreloadMembersHelper::PreloadObject(BlueprintObject);
			
				FPreloadMembersHelper::PreloadMembers(BlueprintObject);
				// recurse into this function for this parent class; 
				// 'ClassDefaultObject' should be the class's original ExportObject
				RegenerateBlueprintClass(Class, Class->ClassDefaultObject);
			}
		}

		{
			UObject* BlueprintObject = LoadClass->ClassGeneratedBy;
			// Preload the blueprint to make sure it has all the data the class needs for regeneration
			FPreloadMembersHelper::PreloadObject(BlueprintObject);

			UClass* RegeneratedClass = BlueprintObject->RegenerateClass(LoadClass, CurrentCDO, GObjLoaded);
			if (RegeneratedClass)
			{
				BlueprintObject->ClearFlags(RF_BeingRegenerated);
				// Fix up the linker so that the RegeneratedClass is used
				LoadClass->ClearFlags(RF_NeedLoad | RF_NeedPostLoad);
			}
		}
	}

	bool const bSuccessfulRegeneration = !LoadClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated);
	// if this wasn't already flagged as regenerating when we first entered this 
	// function, the clear it ourselves.
	if (!bAlreadyRegenerating)
	{
		LoadClass->ClassGeneratedBy->ClearFlags(RF_BeingRegenerated);
	}

	return bSuccessfulRegeneration;
}

bool ULinkerLoad::DeferPotentialCircularImport(const int32 Index)
{
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	if (!FBlueprintSupport::UseDeferredDependencyLoading())
	{
		return false;
	}

	//--------------------------------------
	// Phase 1: Stub in Dependencies
	//--------------------------------------

	FObjectImport& Import = ImportMap[Index];
	if (Import.XObject != nullptr)
	{
		// this import has already been created
		return Import.XObject->IsA<ULinkerPlaceholderClass>();
	}

	if ((LoadFlags & LOAD_DeferDependencyLoads) && !IsImportNative(Index))
	{
		if (UObject* ClassPackage = FindObject<UPackage>(/*Outer =*/nullptr, *Import.ClassPackage.ToString()))
		{
			if (const UClass* ImportClass = FindObject<UClass>(ClassPackage, *Import.ClassName.ToString()))
			{
				bool const bIsBlueprintClass = ImportClass->IsChildOf<UClass>();
				if (bIsBlueprintClass)
				{
					UPackage* PlaceholderOuter = LinkerRoot;
					UClass*   PlaceholderType  = ULinkerPlaceholderClass::StaticClass();

					FName PlaceholderName(*FString::Printf(TEXT("PLACEHOLDER-CLASS_%s"), *Import.ObjectName.ToString()));
					PlaceholderName = MakeUniqueObjectName(PlaceholderOuter, PlaceholderType, PlaceholderName);

					UClass* Placeholder = ConstructObject<ULinkerPlaceholderClass>(PlaceholderType, PlaceholderOuter, PlaceholderName, RF_Public | RF_Transient);
					// make sure the class is fully formed (has its 
					// ClassAddReferencedObjects/ClassConstructor members set)
					Placeholder->Bind();
					Placeholder->StaticLink(/*bRelinkExistingProperties =*/true);

					Import.XObject = Placeholder;
				}
			}
		}
	}
	return (Import.XObject != nullptr);
#else // !USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	return false;
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
}

void ULinkerLoad::ResolveDeferredDependencies(UClass* LoadClass)
{
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	if (!FBlueprintSupport::UseDeferredDependencyLoading())
	{
		return;
	}

	//--------------------------------------
	// Phase 2: Resolve Dependency Stubs
	//--------------------------------------
	TGuardValue<uint32> LoadFlagsGuard(LoadFlags, LoadFlags & ~LOAD_DeferDependencyLoads);

	for (int32 ImportIndex = 0; ImportIndex < ImportMap.Num(); ++ImportIndex)
	{
		FObjectImport& Import = ImportMap[ImportIndex];

		if (ULinkerPlaceholderClass* PlaceholderClass = Cast<ULinkerPlaceholderClass>(Import.XObject))
		{
			Import.XObject = nullptr;
			UClass* RealClassObj = CastChecked<UClass>(CreateImport(ImportIndex), ECastCheckedType::NullAllowed);

			for (UObjectPropertyBase* ObjProperty : PlaceholderClass->ReferencingProperties)
			{
				ObjProperty->PropertyClass = RealClassObj;
			}
			PlaceholderClass->ReferencingProperties.Empty();
			PlaceholderClass->MarkPendingKill(); // @TODO: ensure these are properly GC'd
		}
	}

	// @TODO: don't know if we need this, but could be good to have (as class 
	//        regeneration is about to force load a lot of this).
	LoadAllObjects();

	//--------------------------------------
	// Phase 3: Finalize (serialize CDO & regenerate class)
	//--------------------------------------

	// since this function is called after class serialization, then we can be
	// certain that the CDO export object exists (since class serialization 
	// would call CreateExport() for the CDO)

#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
	bool const bCircumventValidationChecks = !FBlueprintSupport::UseDeferredDependencyVerificationChecks();
	checkSlow(bCircumventValidationChecks || (LoadClass->ClassDefaultObject != nullptr));
#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING

	// this should assert if the index hasn't been set (as we expect)
	int32 CDOExportIndex = LoadClass->ClassDefaultObject->GetLinkerIndex();
	if (CDOExportIndex == INDEX_NONE)
	{
		CDOExportIndex = DeferredExportIndex;
	}
	else if (DeferredExportIndex != INDEX_NONE)
	{
		check(CDOExportIndex == DeferredExportIndex);
	}
	// else, the CDO hasn't been touched (which is expected, since the CDO needs the class first)
	// @TODO: this comment here ^ is in direct conflict with the assumption 
	//        stated above (that the CDO should be loaded)... figure out which is correct!

	{
		check(CDOExportIndex != INDEX_NONE);
		FObjectExport& CDOExport = ExportMap[CDOExportIndex];

		UObject* CDO = CDOExport.Object;
		if (FBlueprintSupport::UseDeferredCDOSerialization())
		{
			// have to prematurely set the CDO's linker so we can force a Preload()/
			// Serialization of the CDO before we regenerate the Blueprint class
			{
				const EObjectFlags OldFlags = CDO->GetFlags();
				CDO->ClearFlags(RF_NeedLoad | RF_NeedPostLoad);
				CDO->SetLinker(this, CDOExportIndex, /*bShouldDetatchExisting =*/false);
				CDO->SetFlags(OldFlags);
			}
			// should load the CDO (ensuring that it has been serialized in by the 
			// time we get to class regeneration)
			//
			// NOTE: this is point where circular dependencies could reveal 
			//       themselves, as the CDO could depend on a class not listed in 
			//       the package's imports
			//
			// @TODO: should we block/detect reentrant behavior? would this CDO ever
			//        need another CDO?
			Preload(CDO);
		}

		UClass* BlueprintClass = Cast<UClass>(IndexToObject(CDOExport.ClassIndex));
#if TEST_CHECK_DEPENDENCY_LOAD_DEFERRING
		checkSlow(bCircumventValidationChecks || CDO->HasAnyFlags(RF_LoadCompleted));
		checkSlow(bCircumventValidationChecks || BlueprintClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));

		// at this point there should not be any instances of the Blueprint (else,
		// we'd have to reinstance and that is too expensive an operation to 
		// have at load time)
		TArray<UObject*> ClassInstances;
		GetObjectsOfClass(BlueprintClass, ClassInstances, /*bIncludeDerivedClasses =*/true);
		checkSlow(bCircumventValidationChecks || ClassInstances.Num() == 0);

		for (TObjectIterator<ULinkerPlaceholderClass> PlaceholderIt; PlaceholderIt; ++PlaceholderIt)
		{
			ULinkerPlaceholderClass* PlaceholderClass = *PlaceholderIt;
			// there shouldn't be any deferred dependencies that need to be 
			// resolved by this point
			checkSlow(bCircumventValidationChecks || (PlaceholderClass->GetOuter() != LinkerRoot) || PlaceholderClass->IsPendingKill());
		}

		// @TODO: Test verify there are no instances of the class yet (except for the CDO)

#endif // TEST_CHECK_DEPENDENCY_LOAD_DEFERRING

		UObject* OldCDO = BlueprintClass->ClassDefaultObject;
		if (RegenerateBlueprintClass(BlueprintClass, CDO))
		{
			// emulate class CDO serialization (RegenerateBlueprintClass() could 
			// have a side-effect where it overwrites the class's CDO; so we 
			// want to make sure that we don't overwrite that new CDO with a 
			// stale one)
			if (OldCDO == BlueprintClass->ClassDefaultObject)
			{
				BlueprintClass->ClassDefaultObject = CDOExport.Object;
			}
		}
		
		// 
		DeferredExportIndex = INDEX_NONE;
	}
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
}

/*******************************************************************************
 * UObject
 ******************************************************************************/

/** 
 * Returns whether this object is contained in or part of a blueprint object
 */
bool UObject::IsInBlueprint() const
{
	// Exclude blueprint classes as they may be regenerated at any time
	// Need to exclude classes, CDOs, and their subobjects
	const UObject* TestObject = this;
 	while (TestObject)
 	{
 		const UClass *ClassObject = dynamic_cast<const UClass*>(TestObject);
		if (ClassObject 
			&& ClassObject->HasAnyClassFlags(CLASS_CompiledFromBlueprint) 
			&& ClassObject->ClassGeneratedBy)
 		{
 			return true;
 		}
		else if (TestObject->HasAnyFlags(RF_ClassDefaultObject) 
			&& TestObject->GetClass() 
			&& TestObject->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) 
			&& TestObject->GetClass()->ClassGeneratedBy)
 		{
 			return true;
 		}
 		TestObject = TestObject->GetOuter();
 	}

	return false;
}

/** 
 *  Destroy properties that won't be destroyed by the native destructor
 */
void UObject::DestroyNonNativeProperties()
{
	// Destroy properties that won't be destroyed by the native destructor
#if USE_UBER_GRAPH_PERSISTENT_FRAME
	GetClass()->DestroyPersistentUberGraphFrame(this);
#endif
	{
		for (UProperty* P = GetClass()->DestructorLink; P; P = P->DestructorLinkNext)
		{
			P->DestroyValue_InContainer(this);
		}
	}
}

/*******************************************************************************
 * FObjectInitializer
 ******************************************************************************/

/** 
 * Initializes a non-native property, according to the initialization rules. If the property is non-native
 * and does not have a zero constructor, it is initialized with the default value.
 * @param	Property			Property to be initialized
 * @param	Data				Default data
 * @return	Returns true if that property was a non-native one, otherwise false
 */
bool FObjectInitializer::InitNonNativeProperty(UProperty* Property, UObject* Data)
{
	if (!Property->GetOwnerClass()->HasAnyClassFlags(CLASS_Native | CLASS_Intrinsic)) // if this property belongs to a native class, it was already initialized by the class constructor
	{
		if (!Property->HasAnyPropertyFlags(CPF_ZeroConstructor)) // this stuff is already zero
		{
			Property->InitializeValue_InContainer(Data);
		}
		return true;
	}
	else
	{
		// we have reached a native base class, none of the rest of the properties will need initialization
		return false;
	}
}
