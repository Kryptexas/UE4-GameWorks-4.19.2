// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UObjectBase.cpp: Unreal UObject base class
=============================================================================*/

#include "CoreUObjectPrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogUObjectBase, Log, All);
DEFINE_STAT(STAT_UObjectsStatGroupTester);

/** Whether uobject system is initialized.												*/
namespace Internal
{
	bool GObjInitialized = false;
};


/** Objects to automatically register once the object system is ready.					*/
struct FPendingRegistrantInfo
{
	const TCHAR*	Name;
	const TCHAR*	PackageName;
	FPendingRegistrantInfo(const TCHAR* InName,const TCHAR* InPackageName)
		:	Name(InName)
		,	PackageName(InPackageName)
	{}
	static TMap<UObjectBase*, FPendingRegistrantInfo>& GetMap()
	{
		static TMap<UObjectBase*, FPendingRegistrantInfo> PendingRegistrantInfo;
		return PendingRegistrantInfo;
	}
};



/** Objects to automatically register once the object system is ready.					*/
struct FPendingRegistrant
{
	UObjectBase*	Object;
	FPendingRegistrant*	NextAutoRegister;
	FPendingRegistrant(UObjectBase* InObject)
	:	Object(InObject)
	,	NextAutoRegister(NULL)
	{}
};
static FPendingRegistrant* GFirstPendingRegistrant = NULL;
static FPendingRegistrant* GLastPendingRegistrant = NULL;

#if EXTERNAL_OBJECT_NAMES

/**
 * Annotation for FNames
 */
struct FNameAnnotation
{
	/**
	 * default constructor
	 * Default constructor must be the default item
	 */
	FNameAnnotation() :
		Name(NAME_None)
	{
	}
	/**
	 * Determine if this Name is the default...which is NAME_None
	 * @return true is this is NAME_None
	 */
	FORCEINLINE bool IsDefault()
	{
		return Name == NAME_None;
	}

	/**
	 * Constructor 
	 * @param InName name to assign
	 */
	FNameAnnotation(FName InName) :
		Name(InName)
	{
	}

	/**
	 * Name for this object
	 */
	FName				Name; 

};

template <> struct TIsPODType<FNameAnnotation> { enum { Value = true }; };


/**
 * Annotation to relate names to uobjects
 *
 */
static FUObjectAnnotationDense<FNameAnnotation,false> NameAnnotation;

#endif

/**
 * Constructor used for bootstrapping
 * @param	InClass			possibly NULL, this gives the class of the new object, if known at this time
 * @param	InFlags			RF_Flags to assign
 */
UObjectBase::UObjectBase( EObjectFlags InFlags )
:	ObjectFlags			(InFlags)
,	InternalIndex		(INDEX_NONE)
,	Class				(NULL)
,	Outer				(NULL)
{}

/**
 * Constructor used by StaticAllocateObject
 * @param	InClass				non NULL, this gives the class of the new object, if known at this time
 * @param	InFlags				RF_Flags to assign
 * @param	InOuter				outer for this object
 * @param	InName				name of the new object
 * @param	InObjectArchetype	archetype to assign
 */
UObjectBase::UObjectBase( UClass* InClass, EObjectFlags InFlags, UObject *InOuter, FName InName )
:	ObjectFlags			(InFlags)
,	InternalIndex		(INDEX_NONE)
,	Class				(InClass)
,	Outer				(InOuter)
{
	check(Class);
	// Add to global table.
	AddObject(InName);
}


/**
 * Final destructor, removes the object from the object array, and indirectly, from any annotations
 **/
UObjectBase::~UObjectBase()
{
	// If not initialized, skip out.
	if( UObjectInitialized() && Class && !GIsCriticalError )
	{
		// Validate it.
		check(IsValidLowLevel());
		LowLevelRename(NAME_None);
		GUObjectArray.FreeUObjectIndex(this);
	}
}


const FName UObjectBase::GetFName() const
{
#if EXTERNAL_OBJECT_NAMES
	return NameAnnotation.GetAnnotation(InternalIndex).Name;
#else
	return Name;
#endif
}


#if STATS

void UObjectBase::CreateStatID() const
{
	FString LongName;
	UObjectBase const* Target = this;
	do 
	{
		LongName = Target->GetFName().GetPlainNameString() + (LongName.IsEmpty() ? TEXT("") : TEXT(".")) + LongName;
		Target = Target->GetOuter();
	} while(Target);
	if (GetClass())
	{
		LongName = GetClass()->GetFName().GetPlainNameString() / LongName;
	}
	StatID = IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(FName(*LongName), STAT_GROUP_TO_FStatGroup(STATGROUP_UObjects)::GetGroupName(), STAT_GROUP_TO_FStatGroup(STATGROUP_UObjects)::DefaultEnable, true, EStatDataType::ST_int64, *LongName, true);
}
#endif


/**
 * Convert a boot-strap registered class into a real one, add to uobject array, etc
 *
 * @param UClassStaticClass Now that it is known, fill in UClass::StaticClass() as the class
 */
void UObjectBase::DeferredRegister(UClass *UClassStaticClass,const TCHAR* PackageName,const TCHAR* InName)
{
	check(Internal::GObjInitialized);
	// Set object properties.
	Outer        = CreatePackage(NULL,PackageName);
	check(Outer);

	check(UClassStaticClass);
	check(!Class);
	Class = UClassStaticClass;

	// Add to the global object table.
	AddObject(FName(InName));

	// Make sure that objects disregarded for GC are part of root set.
	check(!GUObjectArray.IsDisregardForGC(this) || (GetFlags() & RF_RootSet) );
}

/**
 * Add a newly created object to the name hash tables and the object array
 *
 * @param Name name to assign to this uobject
 */
void UObjectBase::AddObject(FName InName)
{
	if ( !FPackageName::IsShortPackageName(InName) )
	{
		UE_LOG(LogLongPackageNames, Verbose, TEXT("Package created with long name %s"), *InName.ToString());
	}
#if EXTERNAL_OBJECT_NAMES
	NameAnnotation.AddAnnotation(InternalIndex,InName);
#else
	Name = InName;
#endif
	GUObjectArray.AllocateUObjectIndex(this);
	check(InName != NAME_None && InternalIndex >= 0);
	HashObject(this);
	check(IsValidLowLevel());
}

/**
 * Just change the FName and Outer and rehash into name hash tables. For use by higher level rename functions.
 *
 * @param NewName	new name for this object
 * @param NewOuter	new outer for this object, if NULL, outer will be unchanged
 */
void UObjectBase::LowLevelRename(FName NewName,UObject *NewOuter)
{
	STAT(StatID = TStatId();) // reset the stat id since this thing now has a different name
	UnhashObject(this);
	check(InternalIndex >= 0);
#if EXTERNAL_OBJECT_NAMES
	NameAnnotation.AddAnnotation(InternalIndex,NewName);
#else
	Name = NewName;
#endif
	if (NewOuter)
	{
		Outer = NewOuter;
	}
	HashObject(this);
}

void UObjectBase::SetClass(UClass* NewClass)
{
	STAT(StatID = TStatId();) // reset the stat id since this thing now has a different name

	UnhashObject(this);
	Class = NewClass;
	HashObject(this);
}


/**
 * Checks to see if the object appears to be valid
 * @return true if this appears to be a valid object
 */
bool UObjectBase::IsValidLowLevel() const
{
	if( !this )
	{
		UE_LOG(LogUObjectBase, Warning, TEXT("NULL object") );
		return false;
	}
	if( !Class )
	{
		UE_LOG(LogUObjectBase, Warning, TEXT("Object is not registered") );
		return false;
	}
	return GUObjectArray.IsValid(this);
}

bool UObjectBase::IsValidLowLevelFast(bool bRecursive /*= true*/) const
{
	// As DEFULT_ALIGNMENT is defined to 0 now, I changed that to the original numerical value here
	const int32 AlignmentCheck = MIN_ALIGNMENT - 1;

	// Check 'this' pointer before trying to access any of the Object's members
	if (!this || (UPTRINT)this < 0x100)
	{
		UE_LOG(LogUObjectBase, Error, TEXT("\'this\' pointer is invalid."));
		return false;
	}	
	if ((UPTRINT)this & AlignmentCheck)
	{
		UE_LOG(LogUObjectBase, Error, TEXT("\'this\' pointer is misaligned."));
		return false;
	}
	if (*(void**)this == NULL)
	{
		UE_LOG(LogUObjectBase, Error, TEXT("Virtual functions table is invalid."));
		return false;
	}

	// These should all be 0.
	const UPTRINT CheckZero = (ObjectFlags & ~RF_AllFlags) | ((UPTRINT)Class & AlignmentCheck) | ((UPTRINT)Outer & AlignmentCheck);
	if (!!CheckZero)
	{
		UE_LOG(LogUObjectBase, Error, TEXT("Object flags are invalid or either Class or Outer is misaligned"));
		return false;
	}
	// Avoid infinite recursion so call IsValidLowLevelFast on the class object with bRecirsive = false.
	if (bRecursive && !Class->IsValidLowLevelFast(false))
	{
		UE_LOG(LogUObjectBase, Error, TEXT("Class object failed IsValidLowLevelFast test."));
		return false;
	}
	// These should all be non-NULL (except CDO-alignment check which should be 0)
	if (Class == NULL || Class->ClassDefaultObject == NULL || ((UPTRINT)Class->ClassDefaultObject & AlignmentCheck) != 0)
	{
		UE_LOG(LogUObjectBase, Error, TEXT("Class pointer is invalid or CDO is invalid."));
		return false;
	}
	// Lightweight versions of index checks.
	if (!GUObjectArray.IsValidIndex(this) || !Name.IsValidIndexFast())
	{
		UE_LOG(LogUObjectBase, Error, TEXT("Object array index or name index is invalid."));
		return false;
	}
	return true;
}

void UObjectBase::EmitBaseReferences(UClass *RootClass)
{
	RootClass->EmitObjectReference( STRUCT_OFFSET( UObjectBase, Class ) );
	RootClass->EmitObjectReference( STRUCT_OFFSET( UObjectBase, Outer ), GCRT_PersistentObject);
}

/** Enqueue the registration for this object. */
void UObjectBase::Register(const TCHAR* PackageName,const TCHAR* InName)
{
	FPendingRegistrant* PendingRegistration = new FPendingRegistrant(this);
	FPendingRegistrantInfo::GetMap().Add(this, FPendingRegistrantInfo(InName, PackageName));
	if(GLastPendingRegistrant)
	{
		GLastPendingRegistrant->NextAutoRegister = PendingRegistration;
	}
	else
	{
		check(!GFirstPendingRegistrant);
		GFirstPendingRegistrant = PendingRegistration;
	}
	GLastPendingRegistrant = PendingRegistration;
}


/**
 * Dequeues registrants from the list of pending registrations into an array.
 * The contents of the array is preserved, and the new elements are appended.
 */
static void DequeuePendingAutoRegistrants(TArray<FPendingRegistrant>& OutPendingRegistrants)
{
	// We process registrations in the order they were enqueued, since each registrant ensures
	// its dependencies are enqueued before it enqueues itself.
	FPendingRegistrant* NextPendingRegistrant = GFirstPendingRegistrant;
	GFirstPendingRegistrant = NULL;
	GLastPendingRegistrant = NULL;
	while(NextPendingRegistrant)
	{
		FPendingRegistrant* PendingRegistrant = NextPendingRegistrant;
		OutPendingRegistrants.Add(*PendingRegistrant);
		NextPendingRegistrant = PendingRegistrant->NextAutoRegister;
		delete PendingRegistrant;
	};
}

/**
 * Process the auto register objects adding them to the UObject array
 */
static void UObjectProcessRegistrants()
{
	check(UObjectInitialized());
	// Make list of all objects to be registered.
	TArray<FPendingRegistrant> PendingRegistrants;
	DequeuePendingAutoRegistrants(PendingRegistrants);

	for(int32 RegistrantIndex = 0;RegistrantIndex < PendingRegistrants.Num();++RegistrantIndex)
	{
		const FPendingRegistrant& PendingRegistrant = PendingRegistrants[RegistrantIndex];

		UObjectForceRegistration(PendingRegistrant.Object);

		check(PendingRegistrant.Object->GetClass()); // should have been set by DeferredRegister

		// Register may have resulted in new pending registrants being enqueued, so dequeue those.
		DequeuePendingAutoRegistrants(PendingRegistrants);
	}
}

void UObjectForceRegistration(UObjectBase* Object)
{
	FPendingRegistrantInfo* Info = FPendingRegistrantInfo::GetMap().Find(Object);
	if (Info)
	{
		const TCHAR* PackageName = Info->PackageName;
		const TCHAR* Name = Info->Name;
		FPendingRegistrantInfo::GetMap().Remove(Object);  // delete this first so that it doesn't try to do it twice
		Object->DeferredRegister(UClass::StaticClass(),PackageName,Name);
	}
}

static TArray<class UScriptStruct *(*)()>& GetDeferredCompiledInStructRegistration()
{
	static TArray<class UScriptStruct *(*)()> DeferredCompiledInRegistration;
	return DeferredCompiledInRegistration;
}

void UObjectCompiledInDeferStruct(class UScriptStruct *(*InRegister)())
{
	// we do reregister StaticStruct in hot reload
	checkSlow(!GetDeferredCompiledInStructRegistration().Contains(InRegister));
	GetDeferredCompiledInStructRegistration().Add(InRegister);
}

class UScriptStruct *GetStaticStruct(class UScriptStruct *(*InRegister)(), UObject* StructOuter, const TCHAR* StructName)
{
#if !IS_MONOLITHIC
	if (GIsHotReload)
	{
		UScriptStruct* ReturnStruct = FindObject<UScriptStruct>(StructOuter, StructName);
		if (ReturnStruct)
		{
			UE_LOG(LogClass, Log, TEXT( "%s HotReload."), StructName);
			return ReturnStruct;
		}
		UE_LOG(LogClass, Log, TEXT("Could not find existing script struct %s for HotReload. Assuming new"), StructName);
	}
#endif
	return (*InRegister)();
}

// Same thing as GetDeferredCompiledInStructRegistration but for UEnums declared in header files without UClasses.
static TArray<class UEnum *(*)()>& GetDeferredCompiledInEnumRegistration()
{
	static TArray<class UEnum *(*)()> DeferredCompiledInRegistration;
	return DeferredCompiledInRegistration;
}

void UObjectCompiledInDeferEnum(class UEnum *(*InRegister)())
{
	// we do reregister StaticStruct in hot reload
	checkSlow(!GetDeferredCompiledInEnumRegistration().Contains(InRegister));
	GetDeferredCompiledInEnumRegistration().Add(InRegister);
}

class UEnum *GetStaticEnum(class UEnum *(*InRegister)(), UObject* EnumOuter, const TCHAR* EnumName)
{
#if !IS_MONOLITHIC
	if (GIsHotReload)
	{
		UEnum* ReturnEnum = FindObjectChecked<UEnum>(EnumOuter, EnumName);
		if (ReturnEnum)
		{
			UE_LOG(LogClass, Log, TEXT( "%s HotReload."), EnumName);
			return ReturnEnum;
		}
		UE_LOG(LogClass, Log, TEXT("Could not find existing enum %s for HotReload. Assuming new"), EnumName);
	}
#endif
	return (*InRegister)();
}

static TArray<class UClass *(*)()>& GetDeferredCompiledInRegistration()
{
	static TArray<class UClass *(*)()> DeferredCompiledInRegistration;
	return DeferredCompiledInRegistration;
}

void UObjectCompiledInDefer(class UClass *(*InRegister)())
{
#if !IS_MONOLITHIC
	if (!GIsHotReload) // in hot reload, we don't reregister anything
#endif
	{
		checkSlow(!GetDeferredCompiledInRegistration().Contains(InRegister));
		GetDeferredCompiledInRegistration().Add(InRegister);
	}
}

/**
 * Load any outstanding compiled in default properties
 */
static void UObjectLoadAllCompiledInDefaultProperties()
{
	const bool bHaveRegistrants = GetDeferredCompiledInRegistration().Num() != 0;
	if( bHaveRegistrants )
	{
		TArray<class UClass *(*)()> PendingRegistrants;
		TArray<UClass*> NewClasses;
		PendingRegistrants = GetDeferredCompiledInRegistration();
		GetDeferredCompiledInRegistration().Empty();
		for(int32 RegistrantIndex = 0;RegistrantIndex < PendingRegistrants.Num();++RegistrantIndex)
		{
			NewClasses.Add(PendingRegistrants[RegistrantIndex]());
		}
		for (int32 Index = 0; Index < NewClasses.Num(); Index++)
		{
			UClass* Class = NewClasses[Index];
			Class->GetDefaultObject();
		}
		FFeedbackContext& ErrorsFC = UClass::GetDefaultPropertiesFeedbackContext();
		if (ErrorsFC.Errors.Num() || ErrorsFC.Warnings.Num())
		{
			TArray<FString> All;
			All = ErrorsFC.Errors;
			All += ErrorsFC.Warnings;

			ErrorsFC.Errors.Empty();
			ErrorsFC.Warnings.Empty();

			FString AllInOne;
			UE_LOG(LogUObjectBase, Warning, TEXT("-------------- Default Property warnings and errors:"));
			for (int32 Index = 0; Index < All.Num(); Index++)
			{
				UE_LOG(LogUObjectBase, Warning, TEXT("%s"), *All[Index]);
				AllInOne += All[Index];
				AllInOne += TEXT("\n");
			}
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format( NSLOCTEXT("Core", "DefaultPropertyWarningAndErrors", "Default Property warnings and errors:\n{0}"), FText::FromString( AllInOne ) ) );
		}
	}
}

/**
 * Call StaticStruct for each struct...this sets up the internal singleton, and important works correctly with hot reload
 */
static void UObjectLoadAllCompiledInStructs()
{
	// Load Enums first
	const bool bHaveEnumRegistrants = GetDeferredCompiledInEnumRegistration().Num() != 0;
	if( bHaveEnumRegistrants )
	{
		TArray<class UEnum *(*)()> PendingRegistrants;
		PendingRegistrants = GetDeferredCompiledInEnumRegistration();
		GetDeferredCompiledInEnumRegistration().Empty();
		for(int32 RegistrantIndex = 0;RegistrantIndex < PendingRegistrants.Num();++RegistrantIndex)
		{
			PendingRegistrants[RegistrantIndex]();
		}
	}

	// Load Structs
	const bool bHaveRegistrants = GetDeferredCompiledInStructRegistration().Num() != 0;
	if( bHaveRegistrants )
	{
		TArray<class UScriptStruct *(*)()> PendingRegistrants;
		PendingRegistrants = GetDeferredCompiledInStructRegistration();
		GetDeferredCompiledInStructRegistration().Empty();
		for(int32 RegistrantIndex = 0;RegistrantIndex < PendingRegistrants.Num();++RegistrantIndex)
		{
			PendingRegistrants[RegistrantIndex]();
		}
	}
}

bool AnyNewlyLoadedUObjects()
{
	return GFirstPendingRegistrant != NULL || GetDeferredCompiledInRegistration().Num() || GetDeferredCompiledInStructRegistration().Num();
}


void ProcessNewlyLoadedUObjects()
{
	while( AnyNewlyLoadedUObjects() )
	{
		UObjectProcessRegistrants();
		UObjectLoadAllCompiledInStructs();
		UObjectLoadAllCompiledInDefaultProperties();		
	}
}


/**
 * Final phase of UObject initialization. all auto register objects are added to the main data structures.
 */
void UObjectBaseInit()
{
	// Zero initialize and later on get value from .ini so it is overridable per game/ platform...
	int32 MaxObjectsNotConsideredByGC	= 0;  
	int32 SizeOfPermanentObjectPool	= 0;

	// To properly set MaxObjectsNotConsideredByGC look for "Log: XXX objects as part of root set at end of initial load."
	// in your log file. This is being logged from LaunchEnglineLoop after objects have been added to the root set. 

	// Disregard for GC relies on seekfree loading for interaction with linkers. We also don't want to use it in the Editor, for which
	// FPlatformProperties::RequiresCookedData() will be false. Please note that GIsEditor and FApp::IsGame() are not valid at this point.
	if( FPlatformProperties::RequiresCookedData() )
	{
		GConfig->GetInt( TEXT("Core.System"), TEXT("MaxObjectsNotConsideredByGC"), MaxObjectsNotConsideredByGC, GEngineIni );

		// Not used on PC as in-place creation inside bigger pool interacts with the exit purge and deleting UObject directly.
		GConfig->GetInt( TEXT("Core.System"), TEXT("SizeOfPermanentObjectPool"), SizeOfPermanentObjectPool, GEngineIni );
	}

	// Log what we're doing to track down what really happens as log in LaunchEngineLoop doesn't report those settings in pristine form.
	UE_LOG(LogInit, Log, TEXT("Presizing for %i objects not considered by GC, pre-allocating %i bytes."), MaxObjectsNotConsideredByGC, SizeOfPermanentObjectPool );

	GUObjectAllocator.AllocatePermanentObjectPool(SizeOfPermanentObjectPool);
	GUObjectArray.AllocatePermanentObjectPool(MaxObjectsNotConsideredByGC);

	// Note initialized.
	Internal::GObjInitialized = true;

	UObjectProcessRegistrants();
}

/**
 * Final phase of UObject shutdown
 */
void UObjectBaseShutdown()
{
	GUObjectArray.ShutdownUObjectArray();
	Internal::GObjInitialized = false;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Associated name
 */
const TCHAR* DebugFName(UObject* Object)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	FName Name = Object->GetFName();
	FCString::Strcpy(TempName,Object ? *FName::SafeString((EName)Name.GetIndex(), Name.GetNumber()) : TEXT("NULL"));
	return TempName;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Object)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Fully qualified path name
 */
const TCHAR* DebugPathName(UObject* Object)
{
	if( Object )
	{
		// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
		static TCHAR PathName[1024];
		PathName[0] = 0;

		// Keep track of how many outers we have as we need to print them in inverse order.
		UObject*	TempObject = Object;
		int32			OuterCount = 0;
		while( TempObject )
		{
			TempObject = TempObject->GetOuter();
			OuterCount++;
		}

		// Iterate over each outer + self in reverse oder and append name.
		for( int32 OuterIndex=OuterCount-1; OuterIndex>=0; OuterIndex-- )
		{
			// Move to outer name.
			TempObject = Object;
			for( int32 i=0; i<OuterIndex; i++ )
			{
				TempObject = TempObject->GetOuter();
			}

			// Dot separate entries.
			if( OuterIndex != OuterCount -1 )
			{
				FCString::Strcat( PathName, TEXT(".") );
			}
			// And app end the name.
			FCString::Strcat( PathName, DebugFName( TempObject ) );
		}

		return PathName;
	}
	else
	{
		return TEXT("None");
	}
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Object)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Fully qualified path name prepended by class name
 */
const TCHAR* DebugFullName(UObject* Object)
{
	if( Object )
	{
		// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
		static TCHAR FullName[1024];
		FullName[0]=0;

		// Class Full.Path.Name
		FCString::Strcat( FullName, DebugFName(Object->GetClass()) );
		FCString::Strcat( FullName, TEXT(" "));
		FCString::Strcat( FullName, DebugPathName(Object) );

		return FullName;
	}
	else
	{
		return TEXT("None");
	}
}