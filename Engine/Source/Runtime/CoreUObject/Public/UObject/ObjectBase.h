// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ObjectBase.h: Unreal object base class.
=============================================================================*/

#pragma once

/** Represents a serializable object pointer in blueprint bytecode. This is always 64-bits, even on 32-bit platforms. */
typedef	uint64 ScriptPointerType;


#if PLATFORM_VTABLE_AT_END_OF_CLASS
#error "not supported in UE4"
#endif

#if HACK_HEADER_GENERATOR 
#define USE_COMPILED_IN_NATIVES	0
#else
#define USE_COMPILED_IN_NATIVES	1
#endif



/*-----------------------------------------------------------------------------
	Core enumerations.
-----------------------------------------------------------------------------*/

//
// Flags for loading objects.
//
enum ELoadFlags
{
	LOAD_None						= 0x00000000,	// No flags.
	LOAD_SeekFree					= 0x00000001,	// Loads the package via the seek free loading path/ reader
	LOAD_NoWarn						= 0x00000002,	// Don't display warning if load fails.
	LOAD_Verify						= 0x00000010,	// Only verify existance; don't actually load.
	LOAD_AllowDll					= 0x00000020,	// Allow plain DLLs.
	LOAD_NoVerify					= 0x00000080,   // Don't verify imports yet.
	LOAD_Quiet						= 0x00002000,   // No log warnings.
	LOAD_FindIfFail					= 0x00004000,	// Tries FindObject if a linker cannot be obtained (e.g. package is currently being compiled)
	LOAD_MemoryReader				= 0x00008000,	// Loads the file into memory and serializes from there.
	LOAD_NoRedirects				= 0x00010000,	// Never follow redirects when loading objects; redirected loads will fail
	LOAD_ForDiff					= 0x00020000,	// Loading for diffing.
	LOAD_NoSeekFreeLinkerDetatch	= 0x00040000,	// Do not detach linkers for this package when seek-free loading
};

//
// Flags for saving packages
//
enum ESaveFlags
{
	SAVE_None			= 0x00000000,	// No flags
	SAVE_NoError		= 0x00000001,	// Don't generate errors on save
	SAVE_FromAutosave	= 0x00000002,   // Used to indicate this save was initiated automatically
	SAVE_KeepDirty		= 0x00000004,	// Do not clear the dirty flag when saving
	SAVE_KeepGUID		= 0x00000008,	// Keep the same guid, used to save cooked packages
	SAVE_Async			= 0x00000010,	// Save to a memory writer, then actually write to disk async
	SAVE_Unversioned	= 0x00000020,	// Save all versions as zero. Upon load this is changed to the current version. This is only reasonable to use with full cooked builds for distribution.
	SAVE_CutdownPackage	= 0x00000040,	// Saving cutdown packages in a temp location WITHOUT renaming the package.
};

//
// Package flags.
//
enum EPackageFlags
{
//	PKG_Unused						= 0x00000001,	// 
	PKG_ClientOptional				= 0x00000002,	// Purely optional for clients.
	PKG_ServerSideOnly				= 0x00000004,   // Only needed on the server side.
	PKG_CompiledIn					= 0x00000010,   // This package is from "compiled in" classes.
	PKG_ForDiffing					= 0x00000020,	// This package was loaded just for the purposes of diff'ing
	PKG_Need						= 0x00008000,	// Client needs to download this package.
	PKG_Compiling					= 0x00010000,	// package is currently being compiled
	PKG_ContainsMap					= 0x00020000,	// Set if the package contains a ULevel/ UWorld object
	PKG_RequiresLocalizationGather		= 0x00040000,	// Set if the package contains any data to be gathered by localization
	PKG_DisallowLazyLoading			= 0x00080000,	// Set if the archive serializing this package cannot use lazy loading
	PKG_PlayInEditor				= 0x00100000,	// Set if the package was created for the purpose of PIE
	PKG_ContainsScript				= 0x00200000,	// Package is allowed to contain UClass objects
//	PKG_Unused						= 0x00400000,
//	PKG_Unused						= 0x00800000,
//	PKG_Unused						= 0x01000000,	
	PKG_StoreCompressed				= 0x02000000,	// Package is being stored compressed, requires archive support for compression
	PKG_StoreFullyCompressed		= 0x04000000,	// Package is serialized normally, and then fully compressed after (must be decompressed before LoadPackage is called)
//	PKG_Unused						= 0x08000000,	
//	PKG_Unused						= 0x10000000,	
//	PKG_Unused						= 0x20000000,
	PKG_ReloadingForCooker			= 0x40000000,   // this package is reloading in the cooker, try to avoid getting data we will never need. We won't save this package.
	PKG_FilterEditorOnly			= 0x80000000,	// Package has editor-only data filtered
};

//
// Internal enums.
//
enum EStaticConstructor				{EC_StaticConstructor};
enum EInternal						{EC_Internal};
enum ECppProperty					{EC_CppProperty};

/** Empty API definition.  Used as a placeholder parameter when no DLL export/import API is needed for a UObject class */
#define NO_API


/**
 * Flags describing a class.
 */
enum EClassFlags
{
	/** @name Base flags */
	//@{
	CLASS_None				  = 0x00000000,
	/** Class is abstract and can't be instantiated directly. */
	CLASS_Abstract            = 0x00000001,
	/** Save object configuration only to Default INIs, never to local INIs. Must be combined with CLASS_Config */
	CLASS_DefaultConfig		  = 0x00000002,
	/** Load object configuration at construction time. */
	CLASS_Config			  = 0x00000004,
	/** This object type can't be saved; null it out at save time. */
	CLASS_Transient			  = 0x00000008,
	/** Successfully parsed. */
	CLASS_Parsed              = 0x00000010,
	/** Class contains localized text. */
	CLASS_Localized           = 0x00000020,
	/** All the properties on the class are shown in the advanced section (which is hidden by default) unless SimpleDisplay is specified on the property */
	CLASS_AdvancedDisplay	  = 0x00000040,
	/** Class is a native class - native interfaces will have CLASS_Native set, but not RF_Native */
	CLASS_Native			  = 0x00000080,
	/** Don't export to C++ header. */
	CLASS_NoExport            = 0x00000100,
	/** Do not allow users to create in the editor. */
	CLASS_NotPlaceable        = 0x00000200,
	/** Handle object configuration on a per-object basis, rather than per-class. */
	CLASS_PerObjectConfig     = 0x00000400,
	
	/** pointers to this class default to weak. */
	CLASS_PointersDefaultToWeak = 0x00000800,
	
	/** Class can be constructed from editinline New button. */
	CLASS_EditInlineNew		  = 0x00001000,
	/** Display properties in the editor without using categories. */
	CLASS_CollapseCategories  = 0x00002000,
	/** Class is an interface **/
	CLASS_Interface           = 0x00004000,
	/**  Do not export a constructor for this class, assuming it is in the cpptext **/
	CLASS_CustomConstructor   = 0x00008000,
	/** all properties and functions in this class are const and should be exported as const */
	CLASS_Const			      = 0x00010000,

	/** pointers to this class default to weak. */
	CLASS_PointersDefaultToAutoWeak = 0x00020000,
	
	/** Indicates that the class was created from blueprint source material */
	CLASS_CompiledFromBlueprint  = 0x00040000,

	/** Indicates that only the bare minimum bits of this class should be DLL exported/imported */
	CLASS_MinimalAPI	      = 0x00080000,
	
	/** Indicates this class must be DLL exported/imported (along with all of it's members) */
	CLASS_RequiredAPI	      = 0x00100000,

	/** Indicates that references to this class default to instanced. Used to be subclasses of UComponent, but now can be any UObject */
	CLASS_DefaultToInstanced  = 0x00200000,

	/** Indicates that the parent token stream has been merged with ours. */
	CLASS_TokenStreamAssembled  = 0x00400000,
	/** Class has component properties. */
	CLASS_HasInstancedReference= 0x00800000,
	/** Don't show this class in the editor class browser or edit inline new menus. */
	CLASS_Hidden			  = 0x01000000,
	/** Don't save objects of this class when serializing */
	CLASS_Deprecated		  = 0x02000000,
	/** Class not shown in editor drop down for class selection */
	CLASS_HideDropDown		  = 0x04000000,
	/** Class was declared directly in C++ and has no boilerplate generated by UnrealHeaderTool */
	CLASS_Intrinsic			  = 0x10000000,
	/** Internal, temporary UHT generated class. */
	CLASS_Temporary		  = 0x20000000,
	/** Indicates that object configuration will not check against ini base/defaults when serialized */
	CLASS_ConfigDoNotCheckDefaults = 0x40000000,
	/** Class has been consigned to oblivion as part of a blueprint recompile, and a newer version currently exists. */
	CLASS_NewerVersionExists  = 0x80000000,

	//@}


	/** @name Flags to inherit from base class */
	//@{
	CLASS_Inherit           = CLASS_Transient | CLASS_DefaultConfig | CLASS_Config | CLASS_Localized | CLASS_PerObjectConfig | CLASS_ConfigDoNotCheckDefaults | CLASS_NotPlaceable
							| CLASS_PointersDefaultToAutoWeak | CLASS_PointersDefaultToWeak | CLASS_Const
							| CLASS_HasInstancedReference | CLASS_Deprecated | CLASS_DefaultToInstanced,

	/** these flags will be cleared by the compiler when the class is parsed during script compilation */
	CLASS_RecompilerClear   = CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_Native | CLASS_Intrinsic | CLASS_TokenStreamAssembled,

	/** these flags will be cleared by the compiler when the class is parsed during script compilation */
	CLASS_ShouldNeverBeLoaded   = CLASS_Native | CLASS_Intrinsic | CLASS_TokenStreamAssembled,

	/** these flags will be inherited from the base class only for non-intrinsic classes */
	CLASS_ScriptInherit		= CLASS_Inherit | CLASS_EditInlineNew | CLASS_CollapseCategories,
	//@}

	/** This is used as a mask for the flags put into generated code for "compiled in" classes. */
	CLASS_SaveInCompiledInClasses = 
		CLASS_Abstract | 
		CLASS_DefaultConfig |
		CLASS_Config |
		CLASS_Transient |
		CLASS_Localized |
		CLASS_Native |
		CLASS_NotPlaceable |
		CLASS_PerObjectConfig |
		CLASS_ConfigDoNotCheckDefaults |
		CLASS_PointersDefaultToWeak |
		CLASS_EditInlineNew |
		CLASS_CollapseCategories |
		CLASS_Interface |
		CLASS_PointersDefaultToAutoWeak |
		CLASS_DefaultToInstanced |
		CLASS_HasInstancedReference |
		CLASS_Hidden |
		CLASS_Deprecated |
		CLASS_HideDropDown |
		CLASS_Intrinsic |
		CLASS_AdvancedDisplay |
		CLASS_Const |
		CLASS_MinimalAPI |
		CLASS_RequiredAPI,

	CLASS_AllFlags			= 0xFFFFFFFF,
};

/**
 * Flags used for quickly casting classes of certain types; all class cast flags are inherited
 */
typedef uint64 EClassCastFlags;

#define CASTCLASS_None							DECLARE_UINT64(0x0000000000000000)
#define CASTCLASS_UField						DECLARE_UINT64(0x0000000000000001)
#define CASTCLASS_UInt8Property					DECLARE_UINT64(0x0000000000000002)
#define CASTCLASS_UEnum							DECLARE_UINT64(0x0000000000000004)
#define CASTCLASS_UStruct						DECLARE_UINT64(0x0000000000000008)
#define CASTCLASS_UScriptStruct					DECLARE_UINT64(0x0000000000000010)
#define CASTCLASS_UClass						DECLARE_UINT64(0x0000000000000020)
#define CASTCLASS_UByteProperty					DECLARE_UINT64(0x0000000000000040)
#define CASTCLASS_UIntProperty					DECLARE_UINT64(0x0000000000000080)
#define CASTCLASS_UFloatProperty				DECLARE_UINT64(0x0000000000000100)
#define CASTCLASS_UUInt64Property				DECLARE_UINT64(0x0000000000000200)
#define CASTCLASS_UClassProperty				DECLARE_UINT64(0x0000000000000400)
#define CASTCLASS_UUInt32Property				DECLARE_UINT64(0x0000000000000800)
#define CASTCLASS_UInterfaceProperty			DECLARE_UINT64(0x0000000000001000)
#define CASTCLASS_UNameProperty					DECLARE_UINT64(0x0000000000002000)
#define CASTCLASS_UStrProperty					DECLARE_UINT64(0x0000000000004000)
#define CASTCLASS_UProperty						DECLARE_UINT64(0x0000000000008000)
#define CASTCLASS_UObjectProperty				DECLARE_UINT64(0x0000000000010000)
#define CASTCLASS_UBoolProperty					DECLARE_UINT64(0x0000000000020000)
#define CASTCLASS_UUInt16Property				DECLARE_UINT64(0x0000000000040000)
#define CASTCLASS_UFunction						DECLARE_UINT64(0x0000000000080000)
#define CASTCLASS_UStructProperty				DECLARE_UINT64(0x0000000000100000)
#define CASTCLASS_UArrayProperty				DECLARE_UINT64(0x0000000000200000)
#define CASTCLASS_UInt64Property				DECLARE_UINT64(0x0000000000400000)
#define CASTCLASS_UDelegateProperty				DECLARE_UINT64(0x0000000000800000)
#define CASTCLASS_UNumericProperty				DECLARE_UINT64(0x0000000001000000)
#define CASTCLASS_UMulticastDelegateProperty	DECLARE_UINT64(0x0000000002000000)
#define CASTCLASS_UObjectPropertyBase			DECLARE_UINT64(0x0000000004000000)
#define CASTCLASS_UWeakObjectProperty			DECLARE_UINT64(0x0000000008000000)
#define CASTCLASS_ULazyObjectProperty			DECLARE_UINT64(0x0000000010000000)
#define CASTCLASS_UAssetObjectProperty			DECLARE_UINT64(0x0000000020000000)
#define CASTCLASS_UTextProperty					DECLARE_UINT64(0x0000000040000000)
#define CASTCLASS_UInt16Property				DECLARE_UINT64(0x0000000080000000)
#define CASTCLASS_UDoubleProperty				DECLARE_UINT64(0x0000000100000000)
#define CASTCLASS_UAssetClassProperty			DECLARE_UINT64(0x0000000200000000)

#define CASTCLASS_AllFlags						DECLARE_UINT64(0xFFFFFFFFFFFFFFFF)

//
// Flags associated with each property in a class, overriding the
// property's default behavior.
// NOTE: When adding one here, please update ParsePropertyFlags
//

// For compilers that don't support 64 bit enums.
#define CPF_Edit							DECLARE_UINT64(0x0000000000000001)		// Property is user-settable in the editor.
#define CPF_ConstParm						DECLARE_UINT64(0x0000000000000002)		// This is a constant function parameter
#define CPF_BlueprintVisible				DECLARE_UINT64(0x0000000000000004)		// This property can be read by blueprint code
#define CPF_ExportObject					DECLARE_UINT64(0x0000000000000008)		// Object can be exported with actor.
#define CPF_BlueprintReadOnly				DECLARE_UINT64(0x0000000000000010)		// This property cannot be modified by blueprint code
#define CPF_Net								DECLARE_UINT64(0x0000000000000020)		// Property is relevant to network replication.
#define CPF_EditFixedSize					DECLARE_UINT64(0x0000000000000040)		// Indicates that elements of an array can be modified, but its size cannot be changed.
#define CPF_Parm							DECLARE_UINT64(0x0000000000000080)		// Function/When call parameter.
#define CPF_OutParm							DECLARE_UINT64(0x0000000000000100)		// Value is copied out after function call.
#define CPF_ZeroConstructor					DECLARE_UINT64(0x0000000000000200)		// memset is fine for construction
#define CPF_ReturnParm						DECLARE_UINT64(0x0000000000000400)		// Return value.
#define CPF_DisableEditOnTemplate			DECLARE_UINT64(0x0000000000000800)		// Disable editing of this property on an archetype/sub-blueprint
//#define CPF_      						DECLARE_UINT64(0x0000000000001000)		// 
#define CPF_Transient   					DECLARE_UINT64(0x0000000000002000)		// Property is transient: shouldn't be saved, zero-filled at load time.
#define CPF_Config      					DECLARE_UINT64(0x0000000000004000)		// Property should be loaded/saved as permanent profile.
#define CPF_Localized   					DECLARE_UINT64(0x0000000000008000)		// Property should be loaded as localizable text.
#define CPF_DisableEditOnInstance			DECLARE_UINT64(0x0000000000010000)		// Disable editing on an instance of this class
#define CPF_EditConst   					DECLARE_UINT64(0x0000000000020000)		// Property is uneditable in the editor.
#define CPF_GlobalConfig					DECLARE_UINT64(0x0000000000040000)		// Load config from base class, not subclass.
#define CPF_InstancedReference				DECLARE_UINT64(0x0000000000080000)		// Property is a component references.
#define CPF_AlwaysInit						DECLARE_UINT64(0x0000000000100000)		// Property should never be exported as NoInit	(@todo - this doesn't need to be a property flag...only used during make)
#define CPF_DuplicateTransient				DECLARE_UINT64(0x0000000000200000)		// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
#define CPF_SubobjectReference				DECLARE_UINT64(0x0000000000400000)		// Property contains subobject references (TSubobjectPtr)
//#define CPF_    							DECLARE_UINT64(0x0000000000800000)		// 
#define CPF_SaveGame						DECLARE_UINT64(0x0000000001000000)		// Property should be serialized for save games
#define CPF_NoClear							DECLARE_UINT64(0x0000000002000000)		// Hide clear (and browse) button.
#define CPF_EditInline						DECLARE_UINT64(0x0000000004000000)		// Edit this object reference inline.
#define CPF_ReferenceParm					DECLARE_UINT64(0x0000000008000000)		// Value is passed by reference; CPF_OutParam and CPF_Param should also be set.
#define CPF_BlueprintAssignable				DECLARE_UINT64(0x0000000010000000)		// MC Delegates only.  Property should be exposed for assigning in blueprint code
#define CPF_Deprecated  					DECLARE_UINT64(0x0000000020000000)		// Property is deprecated.  Read it from an archive, but don't save it.
#define CPF_IsPlainOldData					DECLARE_UINT64(0x0000000040000000)		// If this is set, then the property can be memcopied instead of CopyCompleteValue / CopySingleValue
#define CPF_RepSkip							DECLARE_UINT64(0x0000000080000000)		// Not replicated. For non replicated properties in replicated structs 
#define CPF_RepNotify						DECLARE_UINT64(0x0000000100000000)		// Notify actors when a property is replicated
#define CPF_Interp							DECLARE_UINT64(0x0000000200000000)		// interpolatable property for use with matinee
#define CPF_NonTransactional				DECLARE_UINT64(0x0000000400000000)		// Property isn't transacted
#define CPF_EditorOnly						DECLARE_UINT64(0x0000000800000000)		// Property should only be loaded in the editor
#define CPF_NoDestructor					DECLARE_UINT64(0x0000001000000000)		// No destructor
#define CPF_RepRetry						DECLARE_UINT64(0x0000002000000000)		// retry replication of this property if it fails to be fully sent (e.g. object references not yet available to serialize over the network)
#define CPF_AutoWeak						DECLARE_UINT64(0x0000004000000000)		// Only used for weak pointers, means the export type is autoweak
#define CPF_ContainsInstancedReference		DECLARE_UINT64(0x0000008000000000)		// Property contains component references.
#define CPF_AssetRegistrySearchable			DECLARE_UINT64(0x0000010000000000)		// asset instances will add properties with this flag to the asset registry automatically
#define CPF_SimpleDisplay					DECLARE_UINT64(0x0000020000000000)		// The property is visible by default in the editor details view
#define CPF_AdvancedDisplay					DECLARE_UINT64(0x0000040000000000)		// The property is advanced and not visible by default in the editor details view
#define CPF_Protected						DECLARE_UINT64(0x0000080000000000)		// property is protected from the perspective of script
#define CPF_BlueprintCallable				DECLARE_UINT64(0x0000100000000000)		// MC Delegates only.  Property should be exposed for calling in blueprint code
#define CPF_BlueprintAuthorityOnly			DECLARE_UINT64(0x0000200000000000)		// MC Delegates only.  This delegate accepts (only in blueprint) only events with BlueprintAuthorityOnly.
#define CPF_TextExportTransient				DECLARE_UINT64(0x0000400000000000)		// Property shouldn't be exported to text format (e.g. copy/paste)
#define CPF_NonPIETransient					DECLARE_UINT64(0x0000800000000000)		// Property should only be copied in PIE


/** @name Combinations flags */
//@{
#define CPF_ParmFlags				(CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_ReferenceParm | CPF_ConstParm)
#define CPF_PropagateToArrayInner	(CPF_ExportObject | CPF_EditInline | CPF_InstancedReference | CPF_ContainsInstancedReference | CPF_Localized | CPF_Config | CPF_AlwaysInit | CPF_EditConst | CPF_Deprecated | CPF_EditorOnly | CPF_AutoWeak )

/** the flags that should never be set on interface properties */
#define CPF_InterfaceClearMask		(CPF_ExportObject|CPF_InstancedReference|CPF_ContainsInstancedReference)

/** all the properties that can be stripped for final release console builds */
#define CPF_DevelopmentAssets		(CPF_EditorOnly)

/** all the properties that should never be loaded or saved */
#define CPF_ComputedFlags			(CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor)

#define CPF_AllFlags				DECLARE_UINT64(0xFFFFFFFFFFFFFFFF)

//@}

/** 
 * Flags describing an object instance
 */
enum EObjectFlags
{
	// Do not add new flags unless they truly belong here. There are alternatives.
	// if you change any the bit of any of the RF_Load flags, then you will need legacy serialization

	// This first group of flags mostly has to do with what kind of object it is. Other than transient, these are the persistent object flags.
	// The garbage collector also tends to look at these.
	RF_Public					=0x00000001,	///< Object is visible outside its package.
	RF_Standalone				=0x00000002,	///< Keep object around for editing even if unreferenced.
	RF_Native					=0x00000004,	///< Native (UClass only).
	RF_Transactional			=0x00000008,	///< Object is transactional.
	RF_ClassDefaultObject		=0x00000010,	///< This object is its class's default object
	RF_ArchetypeObject			=0x00000020,	///< This object is a template for another object - treat like a class default object
	RF_Transient				=0x00000040,	///< Don't save object.

	// This group of flags is primarily concerned with garbage collection.
	RF_RootSet					=0x00000080,	///< Object will not be garbage collected, even if unreferenced.
	RF_Unreachable				=0x00000100,	///< Object is not reachable on the object graph.
	RF_TagGarbageTemp			=0x00000200,	///< This is a temp user flag for various utilities that need to use the garbage collector. The garbage collector itself does not interpret it.

	// The group of flags tracks the stages of the lifetime of a uobject
	RF_NeedLoad					=0x00000400,	///< During load, indicates object needs loading.
	RF_AsyncLoading				=0x00000800,	///< Object is being asynchronously loaded.
	RF_NeedPostLoad				=0x00001000,	///< Object needs to be postloaded.
	RF_NeedPostLoadSubobjects	=0x00002000,	///< During load, indicates that the object still needs to instance subobjects and fixup serialized component references
	RF_PendingKill				=0x00004000,	///< Objects that are pending destruction (invalid for gameplay but valid objects)
	RF_BeginDestroyed			=0x00008000,	///< BeginDestroy has been called on the object.
	RF_FinishDestroyed			=0x00010000,	///< FinishDestroy has been called on the object.

	// Misc. Flags
	RF_BeingRegenerated			=0x00020000,	///< Flagged on UObjects that are used to create UClasses (e.g. Blueprints) while they are regenerating their UClass on load (See ULinkerLoad::CreateExport())
	RF_DefaultSubObject			=0x00040000,	///< Flagged on subobjects that are defaults
	RF_WasLoaded				=0x00080000,	///< Flagged on UObjects that were loaded
	RF_TextExportTransient		=0x00100000,	///< Do not export object to text form (e.g. copy/paste). Generally used for sub-objects that can be regenerated from data in their parent object.

	// Special all and none masks
	RF_AllFlags					=0x001fffff,	///< All flags, used mainly for error checking
	RF_NoFlags					=0x00000000,	///< No flags, used to avoid a cast

	// Predefined groups of the above
	RF_Load						= RF_Public | RF_Standalone | RF_Native | RF_Transactional | RF_ClassDefaultObject | RF_ArchetypeObject | RF_DefaultSubObject | RF_TextExportTransient, // Flags to load from Unrealfiles.
	RF_PropagateToSubObjects	= RF_Public | RF_ArchetypeObject | RF_Transactional | RF_Transient,		// Sub-objects will inherit these flags from their SuperObject.

	
};

FORCEINLINE EObjectFlags operator|(EObjectFlags Arg1,EObjectFlags Arg2)
{
	return EObjectFlags(uint32(Arg1) | uint32(Arg2));
}

FORCEINLINE EObjectFlags operator&(EObjectFlags Arg1,EObjectFlags Arg2)
{
	return EObjectFlags(uint32(Arg1) & uint32(Arg2));
}

FORCEINLINE EObjectFlags operator~(EObjectFlags Arg)
{
	return EObjectFlags(RF_AllFlags & ~uint32(Arg));
}

FORCEINLINE void operator&=(EObjectFlags& Dest,EObjectFlags Arg)
{
	Dest = EObjectFlags(Dest & Arg);
}
FORCEINLINE void operator|=(EObjectFlags& Dest,EObjectFlags Arg)
{
	Dest = EObjectFlags(Dest | Arg);
}


//@}


/*----------------------------------------------------------------------------
	Core types.
----------------------------------------------------------------------------*/

// forward declarations
class UObject;
class UProperty;

struct COREUOBJECT_API FReferencerInformation 
{
	/** the object that is referencing the target */
	UObject*				Referencer;

	/** the total number of references from Referencer to the target */
	int32						TotalReferences;

	/** the array of UProperties in Referencer which hold references to target */
	TArray<const UProperty*>		ReferencingProperties;

	FReferencerInformation( UObject* inReferencer );
	FReferencerInformation( UObject* inReferencer, int32 InReferences, const TArray<const UProperty*>& InProperties );
};

struct COREUOBJECT_API FReferencerInformationList
{
	TArray<FReferencerInformation>		InternalReferences;
	TArray<FReferencerInformation>		ExternalReferences;

	FReferencerInformationList();
	FReferencerInformationList( const TArray<FReferencerInformation>& InternalRefs, const TArray<FReferencerInformation>& ExternalRefs );
};

/*----------------------------------------------------------------------------
	Core macros.
----------------------------------------------------------------------------*/

// Special canonical package for FindObject, ParseObject.
#define ANY_PACKAGE ((UPackage*)-1)

// Special prefix for default objects (the UObject in a UClass containing the default values, etc)
#define DEFAULT_OBJECT_PREFIX TEXT("Default__")

///////////////////////////////
/// UObject definition macros
///////////////////////////////

// These macros wrap metadata parsed by the Unreal Header Tool, and are otherwise
// ignored when code containing them is compiled by the C++ compiler
#define UPROPERTY(...)
#define UFUNCTION(...)
#define USTRUCT(...)
#define UMETA(...)
#define UPARAM(...)
#define UENUM(...)
#define UDELEGATE(...)

// This pair of macros is used to help implement GENERATED_USTRUCT_BODY()
#define USTRUCT_COMBINE_INNER(A,B,C) A##B##C
#define USTRUCT_COMBINE(A,B,C) USTRUCT_COMBINE_INNER(A,B,C)

#define GENERATED_USTRUCT_BODY(...) USTRUCT_COMBINE(UCLASS_CURRENT_FILE_NAME,_USTRUCT_BODY_LINE_,__LINE__)

// This macro is used to declare a thunk function in autogenerated boilerplate code
#define DECLARE_FUNCTION(func) void func( FFrame& Stack, RESULT_DECL )

// These are used for syntax highlighting and to allow autocomplete hints

namespace UC
{
	// valid keywords for the UCLASS macro
	enum 
	{
		/// This keyword is used to set the actor group that the class is show in, in the editor.
		classGroup,

		/// This keyword specifies that this class cannot have its code generated until the given class has been processed.
		DependsOn,

		/// Declares that instances of this class should always have an outer of the specified class.  This is inherited by subclasses unless overridden.
		Within, /* =OuterClassName */

		/// Exposes this class as a type that can be used for variables in blueprints
		BlueprintType,

		/// Prevents this class from being used for variables in blueprints
		NotBlueprintType,

		/// Exposes this class as an acceptable base class for creating blueprints. The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses.
		Blueprintable,

		/// Specifies that this class is *NOT* an acceptable base class for creating blueprints. The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses.
		NotBlueprintable,

		/// This keyword indicates that the class should be accessible outside of it's module, but does not need all methods exported.
		/// It exports only the autogenerated methods required for Cast<>, etc... to work.
		MinimalAPI,

		/// Prevents automatic generation of the constructor declaration.
		customConstructor,

		/// Class was declared directly in C++ and has no boilerplate generated by UnrealHeaderTool.
		/// DO NOT USE THIS FLAG ON NEW CLASSES.
		Intrinsic,

		/// No autogenerated code will be created for this class; the header is only provided to parse metadata from.
		/// DO NOT USE THIS FLAG ON NEW CLASSES.
		noexport,

		/// Allow users to create and place this class in the editor.  This flag is inherited by subclasses.
		placeable,

		/// This class cannot be placed in the editor (it cancels out an inherited placeable flag).
		notplaceable,

		/// All instances of this class are considered "instanced". Instanced classes (components) are duplicated upon construction. This flag is inherited by subclasses. 
		DefaultToInstanced,

		/// All properties and functions in this class are const and should be exported as const.  This flag is inherited by subclasses.
		Const,

		/// Class is abstract and can't be instantiated directly.
		Abstract,

		/// This class is deprecated and objects of this class won't be saved when serializing.  This flag is inherited by subclasses.
		deprecated,

		/// This class can't be saved; null it out at save time.  This flag is inherited by subclasses.
		Transient,

		/// This class should be saved normally (it cancels out an inherited transient flag).
		nonTransient,

		/// Load object configuration at construction time.  These flags are inherited by subclasses.
		/// Class containing config properties. Usage config=ConfigName or config=inherit (inherits config name from base class).
		config,
		/// Handle object configuration on a per-object basis, rather than per-class. 
		perObjectConfig,
		/// Determine whether on serialize to configs a check should be done on the base/defaults ini's
		configdonotcheckdefaults,

		/// Save object config only to Default INIs, never to local INIs.
		defaultconfig,

		/// These affect the behavior of the property editor.
		/// Class can be constructed from editinline New button.
		editinlinenew,
		/// Class can't be constructed from editinline New button.
		noteditinlinenew,
		/// Class not shown in editor drop down for class selection.
		hidedropdown,

		/// Shows the specified categories in a property viewer. Usage: showCategories=CategoryName or showCategories=(category0, category1, ...)
		showCategories,
		/// Hides the specified categories in a property viewer. Usage: hideCategories=CategoryName or hideCategories=(category0, category1, ...)
		hideCategories,
		/// Shows the specified function in a property viewer. Usage: showFunctions=FunctionName or showFunctions=(category0, category1, ...)
		showFunctions,
		/// Hides the specified function in a property viewer. Usage: hideFunctions=FunctionName or hideFunctions=(category0, category1, ...)
		hideFunctions,
		/// Specifies which categories should be automatically expanded in a property viewer.
		autoExpandCategories,
		/// Specifies which categories should be automatically collapsed in a property viewer.
		autoCollapseCategories,
		/// Clears the list of auto collapse categories.
		dontAutoCollapseCategories,
		/// Display properties in the editor without using categories.
		collapseCategories,
		/// Display properties in the editor using categories (default behaviour).
		dontCollapseCategories,

		/// All the properties of the class are hidden in the main display by default, and are only shown in the advanced details section.
		AdvancedClassDisplay,

		/// A root convert limits a sub-class to only be able to convert to child classes of the first root class going up the hierarchy.
		ConversionRoot,
	};
}

namespace UI
{
	// valid keywords for the UINTERFACE macro, see the UCLASS versions, above
	enum 
	{
		/// This keyword specifies that this class cannot have its code generated until the given class has been processed.
		DependsOn,

		/// This keyword indicates that the interface should be accessible outside of it's module, but does not need all methods exported.
		/// It exports only the autogenerated methods required for Cast<>, etc... to work.
		MinimalAPI,

		/// Exposes this interface as an acceptable base class for creating blueprints.  The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses.
		Blueprintable,

		/// Specifies that this interface is *NOT* an acceptable base class for creating blueprints.  The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses.
		NotBlueprintable,

		/// Sets IsConversionRoot metadata flag for this interface.
		ConversionRoot,
	};
}

namespace UF
{
	// valid keywords for the UFUNCTION and UDELEGATE macros
	enum 
	{
		/// This function is designed to be overridden by a blueprint.  Do not provide a body for this function;
		/// the autogenerated code will include a thunk that calls ProcessEvent to execute the overridden body.
		BlueprintImplementableEvent,

		/// This function is designed to be overridden by a blueprint, but also has a native implementation.
		/// Provide a body named [FunctionName]_Implementation instead of [FunctionName]; the autogenerated
		/// code will include a thunk that calls the implementation method when necessary.
		BlueprintNativeEvent,

		/// This function is sealed and cannot be overridden in subclasses.
		/// It is only a valid keyword for events; declare other methods as static or FINAL to indicate that they are sealed.
		SealedEvent,

		/// This function is executable from the command line.
		Exec,

		/// This function is replicated, and executed on servers.  Provide a body named [FunctionName]_Implementation instead of [FunctionName];
		/// the autogenerated code will include a thunk that calls the implementation method when necessary.
		Server,

		/// This function is replicated, and executed on clients.  Provide a body named [FunctionName]_Implementation instead of [FunctionName];
		/// the autogenerated code will include a thunk that calls the implementation method when necessary.
		Client,

		/// This function is both executed locally on the server and replicated to all clients, regardless of the Actor's NetOwner
		NetMulticast,

		/// Replication of calls to this function should be done on a reliable channel.
		/// Only valid when used in conjunction with Client or Server
		Reliable,

		/// Replication of calls to this function can be done on an unreliable channel.
		/// Only valid when used in conjunction with Client or Server
		Unreliable,

		/// This function fulfills a contract of producing no side effects, and additionally implies BlueprintCallable.
		BlueprintPure,

		/// This function can be called from blueprint code and should be exposed to the user of blueprint editing tools.
		BlueprintCallable,

		/// This function will not execute from blueprint code if running on something without network authority
		BlueprintAuthorityOnly,

		/// This function is cosmetic and will not run on dedicated servers
		BlueprintCosmetic,

		/// The UnrealHeaderTool code generator will not produce a execFoo thunk for this function; it is up to the user to provide one.
		CustomThunk,

		/// Specifies the category of the function when displayed in blueprint editing tools.
		/// Usage: Category=CategoryName or Category="MajorCategory,SubCategory"
		Category,

		/// This function must supply a _Validate implementation
		WithValidation,

		/// This function is RPC service request
		ServiceRequest,

		/// This function is RPC service response
		ServiceResponse
	};
}

namespace UP
{
	// valid keywords for the UPROPERTY macro
	enum 
	{
		/// This property is const and should be exported as const.
		Const,

		/// Property should be loaded/saved to ini file as permanent profile.
		Config,

		/// Same as above but load config from base class, not subclass.
		GlobalConfig,

		/// Property should be loaded as localizable text. Implies ReadOnly.
		Localized,

		/// Property is transient: shouldn't be saved, zero-filled at load time.
		Transient,

		/// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
		DuplicateTransient,

		/// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
		NonPIETransient,

		/// Value is copied out after function call. Only valid on function param declaration.
		Ref,

		/// Object property can be exported with it's owner.
		Export,

		/// Edit this object reference inline in the editor.
		EditInline,

		/// Hide clear (and browse) button in the editor.
		NoClear,

		/// Indicates that elements of an array can be modified, but its size cannot be changed.
		EditFixedSize,

		/// Property is relevant to network replication.
		Replicated,

		/// Property is relevant to network replication. Notify actors when a property is replicated (usage: ReplicatedUsing=FunctionName).
		ReplicatedUsing,

		/// Retry replication of this property if it fails to be fully sent (e.g. object references not yet available to serialize over the network)
		RepRetry,

		/// Skip replication (only for struct members and parameters in service request functions).
		NotReplicated,

		/// Interpolatable property for use with matinee. Always user-settable in the editor.
		Interp,

		/// Property isn't transacted.
		NonTransactional,

		/// Property is a component reference. Implies EditInline and Export.
		Instanced,

		/// MC Delegates only.  Property should be exposed for assigning in blueprints.
		BlueprintAssignable,

		/// Specifies the category of the property. Usage: Category=CategoryName.
		Category,

		/// Properties appear visible by default in a details panel
		SimpleDisplay,

		/// Properties are in the advanced dropdown in a details panel
		AdvancedDisplay,

		/// Indicates that this property can be edited by property windows in the editor
		EditAnywhere,

		/// Indicates that this property can be edited by property windows, but only on instances, not on archetypes
		EditInstanceOnly,

		/// Indicates that this property can be edited by property windows, but only on archetypes
		EditDefaultsOnly,

		/// Indicates that this property is visible in property windows, but cannot be edited at all
		VisibleAnywhere,
		
		/// Indicates that this property is only visible in property windows for instances, not for archetypes, and cannot be edited
		VisibleInstanceOnly,

		/// Indicates that this property is only visible in property windows for archetypes, and cannot be edited
		VisibleDefaultsOnly,

		/// This property can be read by blueprints, but not modified.
		BlueprintReadOnly,

		/// This property can be read or written from a blueprint.
		BlueprintReadWrite,

		/// The AssetRegistrySearchable keyword indicates that this property and it's value will be automatically added
		/// to the asset registry for any asset class instances containing this as a member variable.  It is not legal
		/// to use on struct properties or parameters.
		AssetRegistrySearchable,

		/// Property should be serialized for save game.
		SaveGame,

		/// MC Delegates only.  Property should be exposed for calling in blueprint code
		BlueprintCallable,

		/// MC Delegates only. This delegate accepts (only in blueprint) only events with BlueprintAuthorityOnly.
		BlueprintAuthorityOnly,

		/// Property shouldn't be exported to text format (e.g. copy/paste)
		TextExportTransient,
	};
}

namespace US
{
	// valid keywords for the USTRUCT macro
	enum 
	{
		/// No autogenerated code will be created for this class; the header is only provided to parse metadata from.
		NoExport,

		/// This class can't be serialized.
		Transient,

		/// Indicates that this struct should always be serialized as a single unit
		Atomic,

		/// Immutable is only legal in Object.h and is being phased out, do not use on new structs!
		Immutable,

		/// Exposes this struct as a type that can be used for variables in blueprints
		BlueprintType,
	};
}

// Metadata specifiers
namespace UM
{
	enum
	{
		/// [FunctionMetadata] This function can only be called on 'this' in a blueprint. It cannot be called on another instance.
		BlueprintProtected,

		/// [FunctionMetadata] This function is not safe to call during Actor construction.
		UnsafeDuringActorConstruction,

		/// [FunctionMetadata] This function is an internal implementation detail, used to implement another function or node.  It is never directly exposed in a graph.
		BlueprintInternalUseOnly,

		/// [FunctionMetadata] This function is deprecated, any blueprint references to it cause a compilation warning.
		DeprecatedFunction,

		/// [FunctionMetadata] Supplies the custom message to use for deprecation
		DeprecationMessage,

		/// [ClassMetadata] If present, the component class can be spawned by a blueprint
		BlueprintSpawnableComponent,

		/// [InterfaceMetadata] This interface cannot be implemented by a blueprint (e.g., it has only non-exposed C++ member methods)
		CannotImplementInterfaceInBlueprint,

		/// [ClassMetadata] if the native class cannot tick, the blueprint generated class (based on exactly this native class) can have bCanEverTick flag overridden.
		ChildCanTick,
	};
}

#define RELAY_CONSTRUCTOR(TClass, TSuperClass) TClass(const class FPostConstructInitializeProperties& PCIP) : TSuperClass(PCIP) {}

#if !USE_COMPILED_IN_NATIVES
	#define COMPILED_IN_FLAGS(TStaticFlags) (TStaticFlags& ~(CLASS_Intrinsic))
	#define COMPILED_IN_INTRINSIC 0
#else
	#define COMPILED_IN_FLAGS(TStaticFlags) (TStaticFlags | CLASS_Intrinsic)
	#define COMPILED_IN_INTRINSIC 1
#endif

#define DECLARE_SERIALIZER( TClass ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
	{ \
		return Ar << (UObject*&)Res; \
	} 

/*-----------------------------------------------------------------------------
Class declaration macros.
-----------------------------------------------------------------------------*/

#define DECLARE_CLASS( TClass, TSuperClass, TStaticFlags, TStaticCastFlags, TPackage, TRequiredAPI  ) \
private: \
    TClass & operator=(TClass const &);   \
	TRequiredAPI static UClass* GetPrivateStaticClass(const TCHAR* Package); \
public: \
	/** Bitwise union of #EClassFlags pertaining to this class.*/ \
	enum {StaticClassFlags=TStaticFlags}; \
	/** Typedef for the base class ({{ typedef-type }}) */ \
	typedef TSuperClass Super;\
	/** Typedef for {{ typedef-type }}. */ \
	typedef TClass ThisClass;\
	/** Returns a UClass object representing this class at runtime */ \
	inline static UClass* StaticClass() \
	{ \
		return GetPrivateStaticClass(TEXT("/Script/") TEXT(#TPackage)); \
	} \
	/** Returns the StaticClassFlags for this class */ \
	inline static EClassCastFlags StaticClassCastFlags() \
	{ \
		return TStaticCastFlags; \
	} \
	/** For internal use only; use StaticConstructObject() to create new objects. */ \
	inline void* operator new( const size_t InSize, UObject* InOuter=(UObject*)GetTransientPackage(), FName InName=NAME_None, EObjectFlags InSetFlags=RF_NoFlags ) \
	{ \
		return StaticAllocateObject( StaticClass(), InOuter, InName, InSetFlags ); \
	} \
	/** For internal use only; use StaticConstructObject() to create new objects. */ \
	inline void* operator new( const size_t InSize, EInternal* InMem ) \
	{ \
		return (void*)InMem; \
	}

#define DECLARE_CLASS_INTRINSIC(TClass,TSuperClass,TStaticFlags,TPackage) \
	DECLARE_CLASS(TClass,TSuperClass,TStaticFlags|CLASS_Intrinsic,CASTCLASS_None,TPackage,NO_API ) \
	RELAY_CONSTRUCTOR(TClass, TSuperClass) \
	enum {IsIntrinsic=1}; \
	static void StaticRegisterNatives##TClass() {} \
	DECLARE_SERIALIZER(TClass)

#define DECLARE_CASTED_CLASS_INTRINSIC_WITH_API( TClass, TSuperClass, TStaticFlags, TPackage, TStaticCastFlags, TRequiredAPI ) \
	DECLARE_CLASS(TClass,TSuperClass,TStaticFlags|CLASS_Intrinsic,TStaticCastFlags,TPackage,TRequiredAPI ) \
	RELAY_CONSTRUCTOR(TClass, TSuperClass) \
	enum {IsIntrinsic=1}; \
	static void StaticRegisterNatives##TClass() {} \
	DECLARE_SERIALIZER(TClass)

#define DECLARE_CASTED_CLASS_INTRINSIC( TClass, TSuperClass, TStaticFlags, TPackage, TStaticCastFlags ) \
	DECLARE_CASTED_CLASS_INTRINSIC_WITH_API( TClass, TSuperClass, TStaticFlags, TPackage, TStaticCastFlags, NO_API)

#define DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR( TClass, TSuperClass, TStaticFlags, TPackage, TStaticCastFlags, TRequiredAPI ) \
	DECLARE_CLASS(TClass,TSuperClass,TStaticFlags|CLASS_Intrinsic,TStaticCastFlags,TPackage, TRequiredAPI ) \
	enum {IsIntrinsic=1}; \
	static void StaticRegisterNatives##TClass() {} \
	DECLARE_SERIALIZER(TClass)



// Declare that objects of class being defined reside within objects of the specified class.
#define DECLARE_WITHIN( TWithinClass ) \
	/** The required type of this object's outer ({{ typedef-type }}) */ \
	typedef class TWithinClass WithinClass;  \
	TWithinClass* GetOuter##TWithinClass() const { return (TWithinClass*)GetOuter(); }

// Register a class at startup time.
#define IMPLEMENT_CLASS(TClass) \
	static UClass* AutoInitialize##TClass = TClass::StaticClass(); \
	UClass* TClass::GetPrivateStaticClass(const TCHAR* Package) \
	{ \
		static UClass* PrivateStaticClass = NULL; \
		if (!PrivateStaticClass) \
		{ \
			/* this could be handled with templates, but we want it external to avoid code bloat */ \
			GetPrivateStaticClassBody<TClass>( \
				Package, \
				(TCHAR*)TEXT(#TClass) + 1 + ((StaticClassFlags & CLASS_Deprecated) ? 11 : 0), \
				PrivateStaticClass, \
				StaticRegisterNatives##TClass \
			); \
		} \
		return PrivateStaticClass; \
	}

// Used for intrinsics, this sets up the boiler plate, plus an initialization singleton, which can create properties and GC tokens
#define IMPLEMENT_INTRINSIC_CLASS(TClass, TRequiredAPI, TSuperClass, TSuperRequiredAPI, InitCode) \
	IMPLEMENT_CLASS(TClass) \
	TRequiredAPI UClass* Z_Construct_UClass_##TClass(); \
	UClass* Z_Construct_UClass_##TClass() \
	{ \
		static UClass* Class = NULL; \
		if (!Class) \
		{ \
			extern TSuperRequiredAPI UClass* Z_Construct_UClass_##TSuperClass(); \
			UClass* SuperClass = Z_Construct_UClass_##TSuperClass(); \
			Class = TClass::StaticClass(); \
			UObjectForceRegistration(Class); \
			check(Class->GetSuperClass() == SuperClass); \
			InitCode \
			Class->StaticLink(); \
		} \
		check(Class->GetClass()); \
		return Class; \
	} \
	static FCompiledInDefer Z_CompiledInDefer_UClass_##TClass(Z_Construct_UClass_##TClass);

#define IMPLEMENT_CORE_INTRINSIC_CLASS(TClass, TSuperClass, InitCode) \
	IMPLEMENT_INTRINSIC_CLASS(TClass, COREUOBJECT_API, TSuperClass, COREUOBJECT_API, InitCode)


/*-----------------------------------------------------------------------------
	ERenameFlags.

	Options to the UObject::Rename function, bit flag
-----------------------------------------------------------------------------*/

typedef uint32 ERenameFlags;

#define REN_None				(0x0000)
#define REN_ForceNoResetLoaders	(0x0001) // Rename won't call ResetLoaders - most likely you should never specify this option (unless you are renaming a UPackage possibly)
#define REN_Test				(0x0002) // Just test to make sure that the rename is guaranteed to succeed if an non test rename immediately follows
#define REN_DoNotDirty			(0x0004) // Indicates that the object (and new outer) should not be dirtied.
#define REN_DontCreateRedirectors (0x0010) // Don't create an object redirector, even if the class is marked RF_Public
#define REN_NonTransactional	(0x0020) // Don't call Modify() on the objects, so they won't be stored in the transaction buffer
#define REN_ForceGlobalUnique	(0x0040) // Force unique names across all packages not just while the scope of the new outer

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

typedef void (*FAsyncCompletionCallback)( UObject* LinkerRoot, void* CallbackUserData );

/*-----------------------------------------------------------------------------
	UObject.
-----------------------------------------------------------------------------*/

namespace UE4
{
	/**
	 * Controls how calls to LoadConfig() should be propagated
	 */
	enum ELoadConfigPropagationFlags
	{
		LCPF_None					=	0x0,
		/**
		 * Indicates that the object should read ini values from each section up its class's hierarchy chain;
		 * Useful when calling LoadConfig on an object after it has already been initialized against its archetype
		 */
		LCPF_ReadParentSections		=	0x1,

		/**
		 * Indicates that LoadConfig() should be also be called on the class default objects for all children of the original class.
		 */
		LCPF_PropagateToChildDefaultObjects		=	0x2,

		/**
		 * Indicates that LoadConfig() should be called on all instances of the original class.
		 */
		LCPF_PropagateToInstances	=	0x4,

		/**
		 * Indicates that this object is reloading its config data
		 */
		LCPF_ReloadingConfigData	=	0x8,

		// Combination flags
		LCPF_PersistentFlags		=	LCPF_ReloadingConfigData,
	};
}


/**
 * Helper class used to save and restore information across a StaticAllocateObject over the top of an existing object.
 * Currently only used by UClass
 */
class FRestoreForUObjectOverwrite
{
public:
	/** virtual destructor **/
	virtual ~FRestoreForUObjectOverwrite() {}
	/** Called once the new object has been reinitialized 
	**/
	virtual void Restore() const=0;
};


#include "UObjectAllocator.h"
#include "UObjectHash.h"
#include "UObjectGlobals.h"
#include "UObjectMarks.h"
#include "UObjectBase.h"
#include "UObjectBaseUtility.h"
#include "UObjectArray.h"
#include "WeakObjectPtr.h"
#include "UObject.h"
#include "UObjectIterator.h"

/**
 * Test validity of object 
 *
 * @param	Test			The object to test
 * @return	Return true if the object is usable: non-null and not pending kill
 */
FORCEINLINE bool IsValid(const UObject *Test)
{
	return Test && !Test->IsPendingKill();
}

/**
 * Template to allow UClass's to be passed around with type safety
**/
template<class TClass>
class TSubclassOf
{
public:
	/**
	 * Default Constructor, defaults to NULL
	**/
	FORCEINLINE TSubclassOf() : 
		Class(NULL)
	{
	}
	/**
	 * Constructor that takes a UClass and does a runtime check to make sure this is a compatible class
	 * @param From UClass to assign to this TSubclassOf
	**/
	FORCEINLINE TSubclassOf(UClass* From) : 
		Class(From)
	{
	}
	/**
	 * Copy Constructor 
	 * @param From TSubclassOf to copy
	**/
	template<class TClassA>
	FORCEINLINE TSubclassOf(const TSubclassOf<TClassA>& From) : 
		Class(*From)
	{
		TClass* Test = (TClassA*)0; // verify that TClassA derives from TClass
	}

	/**
	 * Assignment operator
	 * @param From TSubclassOf to copy
	**/
	template<class TClassA>
	FORCEINLINE TSubclassOf& operator=(const TSubclassOf<TClassA>& From)
	{
		Class = *From;
		TClass* Test = (TClassA*)0; // verify that TClassA derives from TClass
		return *this;
	}
	/**
	 * Assignment operator from UClass, runtime check for compatibility
	 * @param From UClass to assign to this TSubclassOf
	**/
	FORCEINLINE TSubclassOf& operator=(UClass* From)
	{
		Class = From;
		return *this;
	}
	/**
 	 * Dereference back into a UClass
	 * @return	the embedded UClass
	 * The body for this method is in Class.h
	 */
	FORCEINLINE UClass* operator*() const;

	/**
 	 * Dereference back into a UClass
	 * @return	the embedded UClass
	 */
	FORCEINLINE UClass* operator->() const
	{
		return **this;
	}

	/**
	 * Implicit conversion to UClass
	 * @return	the embedded UClass
	 */
	FORCEINLINE operator UClass* () const
	{
		return **this;
	}

	/**
	 * Get the CDO if we are referencing a valid class
	 * @return the CDO, or NULL
	 */
	FORCEINLINE TClass* GetDefaultObject() const;
	
	friend FArchive& operator<<( FArchive& Ar, TSubclassOf& SubclassOf )
	{
		Ar << SubclassOf.Class;
		return Ar;
	}
private:
	UClass* Class;
};

