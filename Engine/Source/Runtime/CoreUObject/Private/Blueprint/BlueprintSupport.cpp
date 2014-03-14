// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "CoreUObjectPrivate.h"

/**
 * Regenerates/Refreshes a blueprint class
 *
 * @param	LoadClass		Instance of the class currently being loaded and which is the parent for the blueprint
 * @param	ExportObject	Current object being exported
 * @return	Returns true if regeneration was successful, otherwise false
 */
bool ULinkerLoad::RegenerateBlueprintClass(UClass* LoadClass, UObject* ExportObject)
{
	UObject* PreviousCDO = NULL;
	if( !LoadClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated) )
	{
		// If this is the first time we're attempting to recompile this class on load, save off the original CDO here, so we can copy its properties after the class has been recompiled
		PreviousCDO = ExportObject;

		// Flag the class source object, so we know we're already in the process of compiling this class
		LoadClass->ClassGeneratedBy->SetFlags(RF_BeingRegenerated);
	}

	// Finish loading the class here, so we have all the appropriate data to copy over to the new CDO
	TArray<UObject*> AllChildMembers;
	GetObjectsWithOuter(LoadClass, /*out*/ AllChildMembers);
	for (int32 Index = 0; Index < AllChildMembers.Num(); ++Index)
	{
		UObject* Member = AllChildMembers[Index];
		Preload(Member);
	}
	Preload(LoadClass);

	LoadClass->StaticLink(true);
	Preload(ExportObject);


	// Cache off the current CDO, and specify the CDO for the load class manually
	UObject* CurrentCDO = ExportObject;
	LoadClass->ClassDefaultObject = CurrentCDO;

	// Make sure that we regenerate any parent classes first before attempting to build a child
 	UClass* ClassChain = LoadClass->GetSuperClass();
 	while(ClassChain && ClassChain->ClassGeneratedBy)
 	{
 		if(ClassChain->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated))
 		{
			// Always load the parent blueprint here in case there is a circular dependency. This will
			// ensure that the blueprint is fully serialized before attempting to regenerate the class.
			ClassChain->ClassGeneratedBy->SetFlags(RF_NeedLoad);
			ClassChain->ClassGeneratedBy->GetLinker()->Preload(ClassChain->ClassGeneratedBy);

			ClassChain->ClassGeneratedBy->RegenerateClass(ClassChain, NULL, GObjLoaded);
 		}
 
 		ClassChain = ClassChain->GetSuperClass();
 	}

	// Preload the blueprint to make sure it has all the data the class needs for regeneration
	if( LoadClass->ClassGeneratedBy->HasAnyFlags(RF_NeedLoad) )
	{
		LoadClass->ClassGeneratedBy->GetLinker()->Preload(LoadClass->ClassGeneratedBy);
	}

	UClass* RegeneratedClass = LoadClass->ClassGeneratedBy->RegenerateClass(LoadClass, PreviousCDO, GObjLoaded);
	if( RegeneratedClass )
	{
		// Fix up the linker so that the RegeneratedClass is used
 		LoadClass->ClearFlags(RF_NeedLoad|RF_NeedPostLoad);
		return true;
	}
	return false;
}

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
 		const UClass *ClassObject = Cast<const UClass>(TestObject);
 		if (ClassObject && ClassObject->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
 		{
 			return true;
 		}
 		else if (TestObject->HasAnyFlags(RF_ClassDefaultObject) && TestObject->GetClass() && TestObject->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
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
	{
		for (UProperty* P = GetClass()->DestructorLink; P; P = P->DestructorLinkNext)
		{
			P->DestroyValue_InContainer(this);
		}
	}
}

/** 
 * Initializes a non-native property, according to the initialization rules. If the property is non-native
 * and does not have a zero constructor, it is initialized with the default value.
 * @param	Property			Property to be initialized
 * @param	Data				Default data
 * @return	Returns true if that property was a non-native one, otherwise false
 */
bool FPostConstructInitializeProperties::InitNonNativeProperty(UProperty* Property, UObject* Data)
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
			if (UFunction* Function = Cast<UFunction>(Field))
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
				Dependency->ConditionalRecompileClass();
			}
		}

		// Finally, recompile the master class to make sure it gets updated too
		BatchMasterClass->ConditionalRecompileClass();
		
		BatchMasterClass = NULL;
	}
}
#endif //WITH_EDITOR
