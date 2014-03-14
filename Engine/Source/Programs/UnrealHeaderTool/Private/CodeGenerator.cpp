// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealHeaderTool.h"

#include "UniquePtr.h"
#include "ParserHelper.h"
#include "NativeClassExporter.h"
#include "HeaderParser.h"
#include "ClassMaps.h"
#include "Json.h"

namespace
{
	template <typename T> struct TJsonFieldType;
	template <>           struct TJsonFieldType<double>                         { static const EJson::Type Value = EJson::Number;  };
	template <>           struct TJsonFieldType<FString>                        { static const EJson::Type Value = EJson::String;  };
	template <>           struct TJsonFieldType<bool>                           { static const EJson::Type Value = EJson::Boolean; };
	template <>           struct TJsonFieldType<TArray<TSharedPtr<FJsonValue>>> { static const EJson::Type Value = EJson::Array;   };
	template <>           struct TJsonFieldType<TSharedPtr<FJsonObject>>        { static const EJson::Type Value = EJson::Object;  };

	template <typename T>
	void GetJsonValue(T& OutVal, const TSharedPtr<FJsonValue>& JsonValue, const TCHAR* Outer)
	{
		if (JsonValue->Type != TJsonFieldType<T>::Value)
			FError::Throwf(TEXT("'%s' is the wrong type"), Outer);

		JsonValue->AsArgumentType(OutVal);
	}

	template <typename T>
	void GetJsonFieldValue(T& OutVal, const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, const TCHAR* Outer)
	{
		auto JsonValue = JsonObject->Values.Find(FieldName);
		if (!JsonValue)
			FError::Throwf(TEXT("Unable to find field '%s' in '%s'"), FieldName, Outer);

		if ((*JsonValue)->Type != TJsonFieldType<T>::Value)
			FError::Throwf(TEXT("Field '%s' in '%s' is the wrong type"), Outer);

		(*JsonValue)->AsArgumentType(OutVal);
	}

	void ProcessHeaderArray(FString* OutStringArray, const TArray<TSharedPtr<FJsonValue>>& InJsonArray, const TCHAR* Outer)
	{
		for (int32 Index = 0, Count = InJsonArray.Num(); Index != Count; ++Index)
		{
			GetJsonValue(*OutStringArray++, InJsonArray[Index], *FString::Printf(TEXT("%s[%d]"), Outer, Index));
		}
	}
}

/////////////////////////////////////////////////////
// FCodeGeneratorConfigState

struct FUObjectModuleInfo
{
	/** The name of the module */
	FString Name;

	/** Long package name for this module's UObject class */
	FString LongPackageName;

	/** Base directory of this module on disk */
	FString BaseDirectory;

	/** Directory where generated include files should go */
	FString GeneratedIncludeDirectory;

	/** List of C++ public 'Classes' header files with UObjects in them (legacy) */
	TArray<FString> PublicUObjectClassesHeaders;

	/** List of C++ public header files with UObjects in them */
	TArray<FString> PublicUObjectHeaders;

	/** List of C++ private header files with UObjects in them */
	TArray<FString> PrivateUObjectHeaders;

	/** Whether or not to write out headers that have changed */
	bool SaveExportedHeaders;
};


class FCodeGeneratorConfigState
{
public:
	bool    UseRelativePaths;
	FString RootLocalPath;
	FString RootBuildPath;

	/** Ordered list of modules that define UObjects or UStructs, which we may need to generate
	    code for.  The list is in module dependency order, such that most dependent modules appear first. */
	TArray<FUObjectModuleInfo> UObjectModuleList;

public:
	/**
	 * Loads module info data
	 */
	void LoadManifest(const FString& ModuleInfoPath, const FString& ManifestFilename)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *ManifestFilename))
		{
			UE_LOG(LogCompile, Error, TEXT("Unable to load manifest: %s"), *ManifestFilename );
			return;
		}

		auto RootObject = TSharedPtr<FJsonObject>();
		auto Reader     = TJsonReaderFactory<TCHAR>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, RootObject))
		{
			UE_LOG(LogCompile, Error, TEXT("Manifest is malformed: %s"), *ManifestFilename );
			return;
		}

		TArray<TSharedPtr<FJsonValue>> ModulesArray;

		GetJsonFieldValue(UseRelativePaths, RootObject, TEXT("UseRelativePaths"), TEXT("{manifest root}"));
		GetJsonFieldValue(RootLocalPath,    RootObject, TEXT("RootLocalPath"),    TEXT("{manifest root}"));
		GetJsonFieldValue(RootBuildPath,    RootObject, TEXT("RootBuildPath"),    TEXT("{manifest root}"));
		GetJsonFieldValue(ModulesArray,     RootObject, TEXT("Modules"),          TEXT("{manifest root}"));

		RootLocalPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(ManifestFilename), RootLocalPath);
		RootBuildPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(ManifestFilename), RootBuildPath);

		// Ensure directories end with a slash, because this aids their use with FPaths::MakePathRelativeTo.
		if (!RootLocalPath.EndsWith(TEXT("/"))) { RootLocalPath += TEXT("/"); }
		if (!RootBuildPath.EndsWith(TEXT("/"))) { RootBuildPath += TEXT("/"); }

		int32 ModuleIndex = 0;
		for (auto Module : ModulesArray)
		{
			auto ModuleObj = Module->AsObject();

			TArray<TSharedPtr<FJsonValue>> ClassesHeaders;
			TArray<TSharedPtr<FJsonValue>> PublicHeaders;
			TArray<TSharedPtr<FJsonValue>> PrivateHeaders;

			UObjectModuleList.AddZeroed();
			FUObjectModuleInfo& KnownModule = UObjectModuleList.Last();

			FString Outer = FString::Printf(TEXT("Modules[%d]"), ModuleIndex);

			GetJsonFieldValue(KnownModule.Name,                      ModuleObj, TEXT("Name"),                *Outer);
			GetJsonFieldValue(KnownModule.BaseDirectory,             ModuleObj, TEXT("BaseDirectory"),       *Outer);
			GetJsonFieldValue(KnownModule.GeneratedIncludeDirectory, ModuleObj, TEXT("OutputDirectory"),     *Outer);
			GetJsonFieldValue(KnownModule.SaveExportedHeaders,       ModuleObj, TEXT("SaveExportedHeaders"), *Outer);
			GetJsonFieldValue(ClassesHeaders,                        ModuleObj, TEXT("ClassesHeaders"),      *Outer);
			GetJsonFieldValue(PublicHeaders,                         ModuleObj, TEXT("PublicHeaders"),       *Outer);
			GetJsonFieldValue(PrivateHeaders,                        ModuleObj, TEXT("PrivateHeaders"),      *Outer);

			KnownModule.LongPackageName = FPackageName::ConvertToLongScriptPackageName(*KnownModule.Name);

			// Convert relative paths
			KnownModule.BaseDirectory             = FPaths::ConvertRelativePathToFull(ModuleInfoPath, KnownModule.BaseDirectory);
			KnownModule.GeneratedIncludeDirectory = FPaths::ConvertRelativePathToFull(ModuleInfoPath, KnownModule.GeneratedIncludeDirectory);

			// Ensure directories end with a slash, because this aids their use with FPaths::MakePathRelativeTo.
			if (!KnownModule.BaseDirectory            .EndsWith(TEXT("/"))) { KnownModule.BaseDirectory            .AppendChar(TEXT('/')); }
			if (!KnownModule.GeneratedIncludeDirectory.EndsWith(TEXT("/"))) { KnownModule.GeneratedIncludeDirectory.AppendChar(TEXT('/')); }

			KnownModule.PublicUObjectClassesHeaders.AddZeroed(ClassesHeaders.Num());
			KnownModule.PublicUObjectHeaders       .AddZeroed(PublicHeaders .Num());
			KnownModule.PrivateUObjectHeaders      .AddZeroed(PrivateHeaders.Num());

			ProcessHeaderArray(KnownModule.PublicUObjectClassesHeaders.GetTypedData(), ClassesHeaders, *(Outer + TEXT(".ClassHeaders"  )));
			ProcessHeaderArray(KnownModule.PublicUObjectHeaders       .GetTypedData(), PublicHeaders , *(Outer + TEXT(".PublicHeaders" )));
			ProcessHeaderArray(KnownModule.PrivateUObjectHeaders      .GetTypedData(), PrivateHeaders, *(Outer + TEXT(".PrivateHeaders")));

			// Sort the headers alphabetically.  This is just to add determinism to the compilation dependency order, since we currently
			// don't rely on explicit includes (but we do support 'dependson')
			// @todo uht: Ideally, we should sort these by sensical order before passing them in -- or better yet, follow include statements ourselves in here.
			KnownModule.PublicUObjectClassesHeaders.Sort();
			KnownModule.PublicUObjectHeaders       .Sort();
			KnownModule.PrivateUObjectHeaders      .Sort();

			++ModuleIndex;
		}
	}
};

FCodeGeneratorConfigState CodeGeneneratorConfig;

static TArray<FString> ChangeMessages;
static bool bWriteContents = false;
static bool bVerifyContents = false;

static const bool bMultiLineUFUNCTION = true;
static const bool bMultiLineUPROPERTY = true;

static UClass* GenerateCodeForHeader(UObject* InParent, const TCHAR* Name, EObjectFlags Flags, const TCHAR* Buffer, int32& OutClassDeclLine);

FCompilerMetadataManager* GScriptHelper = NULL;

/////////////////////////////////////////////////////
// FClassPackageComparator

class FClassPackageComparator
{
	UPackage* Package;

public:

	FClassPackageComparator( UPackage* InPackage )
	: Package(InPackage)
	{
	}

	bool IsValidClass( const class FClassTree* Node ) const
	{
		UClass* Cls = Node->GetClass();
		return Cls == UObject::StaticClass() || Cls->GetOuter() == Package;
	}
};


/** C++ name lookup helper */
FNameLookupCPP* NameLookupCPP;

/////////////////////////////////////////////////////
// FFlagAudit

//@todo: UCREMOVAL this is all audit stuff

static struct FFlagAudit
{
	struct Pair
	{
		FString Name;
		uint64 Flags;
		Pair(UObject* Source, const TCHAR* FlagType, uint64 InFlags)
		{
			Name = Source->GetFullName() + TEXT("[") + FlagType + TEXT("]");
			Flags = InFlags;
		}
	};

	TArray<Pair> Items;

	void Add(UObject* Source, const TCHAR* FlagType, uint64 Flags)
	{
		new (Items) Pair(Source, FlagType, Flags);
	}

	void WriteResults()
	{
		bool bDoDiff = false;
		FString Filename;
		FString RefFilename = FString(FPaths::GameSavedDir()) / TEXT("ReferenceFlags.txt");
		if( !FParse::Param( FCommandLine::Get(), TEXT("WRITEFLAGS") ) )
		{
			return;
		}
		if( FParse::Param( FCommandLine::Get(), TEXT("WRITEREF") ) )
		{
			Filename = RefFilename;
		}
		else if( FParse::Param( FCommandLine::Get(), TEXT("VERIFYREF") ) )
		{
			Filename = FString(FPaths::GameSavedDir()) / TEXT("VerifyFlags.txt");
			bDoDiff = true;
		}

		struct FComparePairByName
		{
			FORCEINLINE bool operator()( const FFlagAudit::Pair& A, const FFlagAudit::Pair& B ) const { return A.Name < B.Name; }
		};
		Items.Sort( FComparePairByName() );

		int32 MaxLen = 0;
		for (int32 Index = 0; Index < Items.Num(); Index++)
		{
			MaxLen = FMath::Max<int32>(Items[Index].Name.Len(), MaxLen);
		}
		MaxLen += 4;
		FStringOutputDevice File;
		for (int32 Index = 0; Index < Items.Num(); Index++)
		{
			File.Logf(TEXT("%s%s0x%016llx\r\n"), *Items[Index].Name, FCString::Spc(MaxLen - Items[Index].Name.Len()), Items[Index].Flags);
		}
		FFileHelper::SaveStringToFile(File, *Filename);
		if (bDoDiff)
		{
			FString Verify = File;
			FString Ref;
			if (FFileHelper::LoadFileToString(Ref, *RefFilename))
			{
				FStringOutputDevice MisMatches;
				TArray<FString> VerifyLines;
				Verify.ParseIntoArray(&VerifyLines, TEXT("\n"), true);
				TArray<FString> RefLines;
				Ref.ParseIntoArray(&RefLines, TEXT("\n"), true);
				check(VerifyLines.Num() == RefLines.Num()); // we aren't doing a sophisticated diff
				for (int32 Index = 0; Index < RefLines.Num(); Index++)
				{
					if (RefLines[Index] != VerifyLines[Index])
					{
						MisMatches.Logf(TEXT("REF   : %s"), *RefLines[Index]);
						MisMatches.Logf(TEXT("VERIFY: %s"), *VerifyLines[Index]);
					}
				}
				FString DiffFilename = FString(FPaths::GameSavedDir()) / TEXT("FlagsDiff.txt");
				FFileHelper::SaveStringToFile(MisMatches, *DiffFilename);
			}
		}
	}
} TheFlagAudit;

static FString Tabify(const TCHAR *Input)
{
	FString Result(Input); 

	const int32 SpacesPerTab = 4;
	Result.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

	TArray<FString> Lines;
	Result.ParseIntoArray( &Lines, TEXT( "\n" ), false );

	Result = TEXT("");
	for (int32 Index = 0; Index < Lines.Num(); Index++)
	{
		if (Index)
		{
			Result += TEXT("\r\n");
		}
		FString& Line = Lines[Index];
		int32 FirstNonWS = 0;
		while (1)
		{
			TCHAR c = *(*Line + FirstNonWS);
			if (c != TEXT('\t') && c != TEXT(' '))
			{
				break;
			}
			FirstNonWS++;
		}
		FString Start = Line.Left(FirstNonWS);
		FString Remainder = *Line + FirstNonWS;
		Start.ReplaceInline(TEXT("\t"), FCString::Spc(SpacesPerTab));
		int32 NumTabs = Start.Len() / SpacesPerTab;
		for (int32 Tab = 0; Tab < NumTabs; Tab++)
		{
			Result += TEXT("\t");
		}
		Result += FCString::Spc(Start.Len() - NumTabs * SpacesPerTab);
		Result += Remainder;
	}

	return Result;
}

/**
 *	Helper function for finding the location of a package
 *	This is required as source now lives in several possible directories
 *
 *	@param	InPackage		The name of the package of interest
 *	@param	OutLocation		The location of the given package, if found
 *  @param	OutHeaderLocation	The directory where generated headers should be placed
 *
 *	@return	bool			true if found, false if not
 */
bool MakeCommandlet_FindPackageLocation(const TCHAR* InPackage, FString& OutLocation, FString& OutHeaderLocation)
{
	// Mapping of processed packages to their locations
	// An empty location string means it was processed but not found
	static TMap<FString,FUObjectModuleInfo*> CheckedPackageList;

	FString CheckPackage(InPackage);

	FUObjectModuleInfo* ModuleInfoPtr = CheckedPackageList.FindRef(CheckPackage);

	if (!ModuleInfoPtr)
	{
		FUObjectModuleInfo* ModuleInfoPtr2 = CodeGeneneratorConfig.UObjectModuleList.FindByPredicate([&](FUObjectModuleInfo& Module) { return Module.Name == CheckPackage; });
		if (ModuleInfoPtr2 && IFileManager::Get().DirectoryExists(*ModuleInfoPtr2->BaseDirectory))
		{
			ModuleInfoPtr = ModuleInfoPtr2;
			CheckedPackageList.Add(CheckPackage, ModuleInfoPtr);
		}
	}

	if (!ModuleInfoPtr)
		return false;

	OutLocation       = ModuleInfoPtr->BaseDirectory;
	OutHeaderLocation = ModuleInfoPtr->GeneratedIncludeDirectory;
	return true;
}


FString Macroize(const TCHAR *MacroName, const TCHAR *StringToMacroize)
{
	FString Result = StringToMacroize;
	if (Result.Len())
	{
		Result.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		Result.ReplaceInline(TEXT("\n"), TEXT(" \\\n"));
		check(Result.EndsWith(TEXT(" \\\n")));
		Result = Result.LeftChop(3);
		Result += TEXT("\n\n\n");
		Result.ReplaceInline(TEXT("\n"), TEXT("\r\n"));		
	}
	return FString::Printf(TEXT("#define %s%s\r\n"), MacroName, Result.Len() ? TEXT(" \\") : TEXT("")) + Result;
}

struct FParmsAndReturnProperties
{
	FParmsAndReturnProperties()
		: Return(NULL)
	{
	}

	#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS

		FParmsAndReturnProperties(FParmsAndReturnProperties&&) = default;
		FParmsAndReturnProperties(const FParmsAndReturnProperties&) = default;
		FParmsAndReturnProperties& operator=(FParmsAndReturnProperties&&) = default;
		FParmsAndReturnProperties& operator=(const FParmsAndReturnProperties&) = default;

	#else

		FParmsAndReturnProperties(      FParmsAndReturnProperties&& Other) : Parms(MoveTemp(Other.Parms)), Return(MoveTemp(Other.Return)) {}
		FParmsAndReturnProperties(const FParmsAndReturnProperties&  Other) : Parms(         Other.Parms ), Return(         Other.Return ) {}
		FParmsAndReturnProperties& operator=(      FParmsAndReturnProperties&& Other) { Parms = MoveTemp(Other.Parms); Return = MoveTemp(Other.Return); return *this; }
		FParmsAndReturnProperties& operator=(const FParmsAndReturnProperties&  Other) { Parms =          Other.Parms ; Return =          Other.Return ; return *this; }

	#endif

	bool HasParms() const
	{
		return Parms.Num() || Return;
	}

	TArray<UProperty*> Parms;
	UProperty*         Return;
};

/**
 * Get parameters and return type for a given function.
 *
 * @param  Function The function to get the parameters for.
 * @return An aggregate containing the parameters and return type of that function.
 */
FParmsAndReturnProperties GetFunctionParmsAndReturn(UFunction* Function)
{
	FParmsAndReturnProperties Result;
	for ( TFieldIterator<UProperty> It(Function); It; ++It)
	{
		UProperty* Field = *It;

		if ((It->PropertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm)
		{
			Result.Parms.Add(Field);
		}
		else if (It->PropertyFlags & CPF_ReturnParm)
		{
			Result.Return = Field;
		}
	}
	return Result;
}

/**
 * Determines whether the glue version of the specified native function
 * should be exported
 *
 * @param	Function	the function to check
 * @return	true if the glue version of the function should be exported.
 */
bool FNativeClassHeaderGenerator::ShouldExportFunction( UFunction* Function )
{
	// export any script stubs for native functions declared in interface classes
	bool bIsBlueprintNativeEvent = (Function->FunctionFlags & FUNC_BlueprintEvent) && (Function->FunctionFlags & FUNC_Native);
	if (Function->GetOwnerClass()->HasAnyClassFlags(CLASS_Interface) && !bIsBlueprintNativeEvent)
		return true;

	// always export if the function is static
	if (Function->FunctionFlags & FUNC_Static)
		return true;

	// don't export the function if this is not the original declaration and there is
	// at least one parent version of the function that is declared native
	for (UFunction* ParentFunction = Function->GetSuperFunction(); ParentFunction; ParentFunction = ParentFunction->GetSuperFunction())
	{
		if (ParentFunction->FunctionFlags & FUNC_Native)
			return false;
	}

	return true;
}

FString CreateLiteralString(const FString& Str)
{
	FString Result = TEXT("TEXT(\"");

	// Have a reasonable guess at reserving the right size
	Result.Reserve(Str.Len() + Result.Len());

	bool bPreviousCharacterWasHex = false;

	const TCHAR* Ptr = *Str;
	while (TCHAR Ch = *Ptr++)
	{
		switch (Ch)
		{
			case TEXT('\r'): continue;
			case TEXT('\n'): Result += TEXT("\\n");  bPreviousCharacterWasHex = false; break;
			case TEXT('\\'): Result += TEXT("\\\\"); bPreviousCharacterWasHex = false; break;
			case TEXT('\"'): Result += TEXT("\\\""); bPreviousCharacterWasHex = false; break;
			default:
				if (Ch < 31 || Ch >= 128)
				{
					Result += FString::Printf(TEXT("\\x%04x"), Ch);
					bPreviousCharacterWasHex = true;
				}
				else
				{
					// We close and open the literal (with TEXT) here in order to ensure that successive hex characters aren't appended to the hex sequence, causing a different number
					if (bPreviousCharacterWasHex && FCharWide::IsHexDigit(Ch))
					{
						Result += "\")TEXT(\"";
					}
					bPreviousCharacterWasHex = false;
					Result += Ch;
				}
				break;
		}
	}

	Result += TEXT("\")");
	return Result;
}

static FString GetMetaDataCodeForObject(UObject* Object, const TCHAR* SymbolName, const TCHAR* Spaces)
{
	TMap<FName, FString>* MetaData = UMetaData::GetMapForObject(Object);

	FString Result;
	if (MetaData && MetaData->Num())
	{
		typedef TKeyValuePair<FName, FString> KVPType;
		TArray<KVPType> KVPs;
		for (auto& KVP : *MetaData)
		{
			KVPs.Add(KVPType(KVP.Key, KVP.Value));
		}

		// We sort the metadata here so that we can get consistent output across multiple runs
		// even when metadata is added in a different order
		KVPs.Sort([](const KVPType& Lhs, const KVPType& Rhs) { return Lhs.Key < Rhs.Key; });

		for (const auto& KVP : KVPs)
		{
			Result += FString::Printf(TEXT("%sMetaData->SetValue(%s, TEXT(\"%s\"), %s);\r\n"), Spaces, SymbolName, *KVP.Key.ToString(), *CreateLiteralString(KVP.Value));
		}
	}
	return Result;
}

/**
 * Returns a C++ code string for declaring a class or variable as an export, if the class is configured for that
 *
 * @return	The API declaration string
 */
FString FNativeClassHeaderGenerator::MakeAPIDecl() const
{
	FString APIDecl;
	if( CurrentClass->HasAnyClassFlags( CLASS_RequiredAPI ) )
	{
		APIDecl = FString::Printf( TEXT( "%s_API " ), *API );
	}

	return APIDecl;
}

/**
 * Exports the struct's C++ properties to the specified output device and adds special
 * compiler directives for GCC to pack as we expect.
 *
 * @param	Struct				UStruct to export properties
 * @param	TextIndent			Current text indentation
 * @param	ImportsDefaults		whether this struct will be serialized with a default value
 */
void FNativeClassHeaderGenerator::ExportProperties( UStruct* Struct, int32 TextIndent, bool bAccessSpecifiers, class FStringOutputDevice* Output )
{
	UProperty*	Previous			= NULL;
	UProperty*	PreviousNonEditorOnly = NULL;
	UProperty*	LastInSuper			= NULL;
	UStruct*	InheritanceSuper	= Struct->GetInheritanceSuper();
	bool		bEmittedHasEditorOnlyMacro = false;
	bool		bEmittedHasScriptAlign = false;

	check(Output != NULL)
	FStringOutputDevice& HeaderOutput = *Output;


	// Find last property in the lowest base class that has any properties
	UStruct* CurrentSuper = InheritanceSuper;
	while (LastInSuper == NULL && CurrentSuper)
	{
		for( TFieldIterator<UProperty> It(CurrentSuper,EFieldIteratorFlags::ExcludeSuper); It; ++It )
		{
			UProperty* Current = *It;

			// Disregard properties with 0 size like functions.
			if( It.GetStruct() == CurrentSuper && Current->ElementSize )
			{
				LastInSuper = Current;
			}
		}
		// go up a layer in the hierarchy
		CurrentSuper = CurrentSuper->GetSuperStruct();
	}

	EPropertyHeaderExportFlags CurrentExportType = PROPEXPORT_Public;

	// find structs that are nothing but bytes, account for editor only properties being 
	// removed on consoles
	int32 NumProperties = 0;
	int32 NumByteProperties = 0;
	int32 NumNonEditorOnlyProperties = 0;
	int32 NumNonEditorOnlyByteProperties = 0;
	for( TFieldIterator<UProperty> It(Struct, EFieldIteratorFlags::ExcludeSuper); It; ++It )
	{
		// treat bitfield and bytes the same
		bool bIsByteProperty = It->IsA(UByteProperty::StaticClass());// || It->IsA(UBoolProperty::StaticClass());
		bool bIsEditorOnlyProperty = It->IsEditorOnlyProperty();
		
		// count our propertie
		NumProperties++;
		if (bIsByteProperty)
		{
			NumByteProperties;
		}
		if (!bIsEditorOnlyProperty)
		{
			NumNonEditorOnlyProperties++;
		}
		if (!bIsEditorOnlyProperty && bIsByteProperty)
		{
			NumNonEditorOnlyByteProperties++;
		}
	}

	bool bCurrentlyInNotCPPBlock = false;
	// Iterate over all properties in this struct.
	for( TFieldIterator<UProperty> It(Struct, EFieldIteratorFlags::ExcludeSuper); It; ++It )
	{
		UProperty* Current = *It;

		FStringOutputDevice PropertyText;

		// Disregard properties with 0 size like functions.
		if (It.GetStruct() == Struct)
		{
			FString AccessSpecifier = TEXT("public");
			if (bAccessSpecifiers)
			{
				// find the class info for this class
				FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

				// find the compiler token for this property
				FTokenData* PropData = ClassData->FindTokenData(Current);
				if ( PropData != NULL )
				{
					// if this property has a different access specifier, then export that now
					if ( (PropData->Token.PropertyExportFlags & CurrentExportType) == 0 )
					{
						if ( (PropData->Token.PropertyExportFlags & PROPEXPORT_Private) != 0 )
						{
							CurrentExportType = PROPEXPORT_Private;
							AccessSpecifier = TEXT("private");
						}
						else if ( (PropData->Token.PropertyExportFlags & PROPEXPORT_Protected) != 0 )
						{
							CurrentExportType = PROPEXPORT_Protected;
							AccessSpecifier = TEXT("protected");
						}
						else
						{
							CurrentExportType = PROPEXPORT_Public;
							AccessSpecifier = TEXT("public");
						}

						if ( AccessSpecifier.Len() )
						{
							// If we are changing the access specifier we need to emit the #endif for the WITH_EDITORONLY_DATA macro first otherwise the access specifier may
							// only be conditionally compiled in.
							if( bEmittedHasEditorOnlyMacro )
							{
								PropertyText.Logf( TEXT("#endif // WITH_EDITORONLY_DATA\r\n") );
								bEmittedHasEditorOnlyMacro = false;
							}

							PropertyText.Logf(TEXT("%s:%s"), *AccessSpecifier, LINE_TERMINATOR);
						}
					}
				}
			}
			// If we are switching from editor to non-editor or vice versa and the state of the WITH_EDITORONLY_DATA macro emission doesn't match, generate the 
			// #if or #endif appropriately.
			bool RequiresHasEditorOnlyMacro = Current->IsEditorOnlyProperty();
			if( !bEmittedHasEditorOnlyMacro && RequiresHasEditorOnlyMacro )
			{
				// Indent code and export CPP text.
				PropertyText.Logf( TEXT("#if WITH_EDITORONLY_DATA\r\n") );
				bEmittedHasEditorOnlyMacro = true;
			}
			else if( bEmittedHasEditorOnlyMacro && !RequiresHasEditorOnlyMacro )
			{
				PropertyText.Logf( TEXT("#endif // WITH_EDITORONLY_DATA\r\n") );
				bEmittedHasEditorOnlyMacro = false;
			}

			// Export property specifiers
			// Indent code and export CPP text.
			{
				FStringOutputDevice JustPropertyDecl;
				JustPropertyDecl.Log( FCString::Spc(TextIndent+4) );

				const FString* Dim = GArrayDimensions.Find(Current);
				Current->ExportCppDeclaration( JustPropertyDecl, EExportedDeclaration::Member, Dim ? **Dim : NULL);
				ApplyAlternatePropertyExportText(*It, JustPropertyDecl);

				// Finish up line.
				JustPropertyDecl.Log(TEXT(";"));

				// Finish up line.
				PropertyText.Logf(TEXT("%s\r\n"), *JustPropertyDecl);
			}

			LastInSuper	= NULL;
			Previous	= Current;
			if (!Current->IsEditorOnlyProperty())
			{
				PreviousNonEditorOnly = Current;
			}

			HeaderOutput.Log(PropertyText);
		}
	}
	if (bCurrentlyInNotCPPBlock)
	{
		HeaderOutput.Log(TEXT("#endif\r\n"));
		bCurrentlyInNotCPPBlock = false;
	}

	// End of property list.  If we haven't generated the WITH_EDITORONLY_DATA #endif, do so now.
	if (bEmittedHasEditorOnlyMacro)
	{
		HeaderOutput.Logf( TEXT("#endif // WITH_EDITORONLY_DATA\r\n") );
	}

	// if the last property that was exported wasn't public, emit a line to reset the access to "public" so that we don't interfere with cpptext
	if ( CurrentExportType != PROPEXPORT_Public )
	{
		HeaderOutput.Logf(TEXT("public:%s"), LINE_TERMINATOR);
	}
}

/** This table maps a singleton name to an extern declaration for it. For cross-module access, we add these to GeneratedFunctionDeclarations **/
static TMap<FString, FString> SingletonNameToExternDecl;

FString FNativeClassHeaderGenerator::GetSingletonName(UField* Item, bool bRequiresValidObject)
{
	UClass* ItemClass = Cast<UClass>(Item);
	if (ItemClass && ItemClass->HasAllClassFlags(CLASS_Intrinsic))
	{
		return FString::Printf(TEXT("%s::StaticClass()"), NameLookupCPP->GetNameCPP(ItemClass));
	}

	FString Suffix;
	if (ItemClass && !bRequiresValidObject && ItemClass->HasAllClassFlags(CLASS_Native))
	{
		Suffix = TEXT("_NoRegister");
	}

	FString Result;
	UObject* Outer = Item;
	while (Outer)
	{
		if (Cast<UClass>(Outer) || Cast<UScriptStruct>(Outer))
		{
			FString OuterName = NameLookupCPP->GetNameCPP(Cast<UStruct>(Outer));
			if (Result.Len())
			{
				Result = OuterName + TEXT("_") + Result;
			}
			else
			{
				Result = OuterName;
			}
			// Structs can also have UPackage outer.
			if (Cast<UClass>(Outer) || Cast<UPackage>(Outer->GetOuter()))
			{
				break;
			}
		}
		else if (Result.Len())
		{
			// Handle UEnums with UPackage outer.
			Result = Outer->GetName() + TEXT("_") + Result;
		}
		else
		{
			Result = Outer->GetName();
		}
		Outer = Outer->GetOuter();
	}
	FString ClassString = NameLookupCPP->GetNameCPP(Item->GetClass());
	// Can't use long package names in function names.
	if (Result.StartsWith(TEXT("/Script/")))
	{
		Result = FPackageName::GetShortName(Result);
	}
	Result = FString(TEXT("Z_Construct_")) + ClassString + TEXT("_") + Result + Suffix + TEXT("()");
	if (CastChecked<UPackage>(Item->GetOutermost()) != Package)
	{
		// this is a cross module reference, we need to include the right extern decl
		FString* Extern = SingletonNameToExternDecl.Find(Result);
		if (!Extern)
		{
			UE_LOG(LogCompile, Warning, TEXT("Could not find extern declaration for cross module reference %s\n"), *Result);
		}
		else
		{
			bool bAlreadyInSet = false;
			UniqueCrossModuleReferences.Add(*Extern, &bAlreadyInSet);

			if (!bAlreadyInSet)
			{
				CrossModuleGeneratedFunctionDeclarations.Log(*Extern);
			}
		}
	}
	return Result;
}

FString FNativeClassHeaderGenerator::PropertyNew(FString& Meta, UProperty* Prop, const FString& OuterString, const FString& PropMacro, const TCHAR* NameSuffix, const TCHAR* Spaces, const TCHAR* SourceStruct)
{
	FString ExtraArgs;

	FString PropNameDep = Prop->GetName();
	if (Prop->HasAllPropertyFlags(CPF_Deprecated))
	{
		PropNameDep += TEXT("_DEPRECATED");
	}

	if (UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(Prop))
	{
		UClass *TargetClass = ObjectProperty->PropertyClass;
		if (UClassProperty* ClassProperty = Cast<UClassProperty>(Prop))
		{
			TargetClass = ClassProperty->MetaClass;
		}
		if (UAssetClassProperty* SubclassOfProperty = Cast<UAssetClassProperty>(Prop))
		{
			TargetClass = SubclassOfProperty->MetaClass;
		}
		ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(TargetClass, false)); 
	}
	else if (UInterfaceProperty* InterfaceProperty = Cast<UInterfaceProperty>(Prop))
	{
		UClass *TargetClass = InterfaceProperty->InterfaceClass;
		ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(TargetClass, false)); 
	}
	else if (UStructProperty* StructProperty = Cast<UStructProperty>(Prop))
	{
		UScriptStruct* Struct = StructProperty->Struct;
		check(Struct);
		ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(Struct));
	}
	else if (UByteProperty* ByteProperty = Cast<UByteProperty>(Prop))
	{
		if (ByteProperty->Enum)
		{
			ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(ByteProperty->Enum));
		}
	}
	else if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(Prop))
	{
		if (Cast<UArrayProperty>(BoolProperty->GetOuter()))
		{
			ExtraArgs = FString(TEXT(", 0"));  // this is an array of C++ bools so the mask is irrelevant.
		}
		else
		{
			check(SourceStruct);
			ExtraArgs = FString::Printf(TEXT(", CPP_BOOL_PROPERTY_BITMASK(%s, %s)"), *PropNameDep, SourceStruct);
		}
		ExtraArgs += FString::Printf(TEXT(", sizeof(%s), %s"), *BoolProperty->GetCPPType(NULL, 0), BoolProperty->IsNativeBool() ? TEXT("true") : TEXT("false"));
	}
	else if (UDelegateProperty* DelegateProperty = Cast<UDelegateProperty>(Prop))
	{
		UFunction *TargetFunction = DelegateProperty->SignatureFunction;
		ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(TargetFunction));
	}
	else if (UMulticastDelegateProperty* MulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Prop))
	{
		UFunction *TargetFunction = MulticastDelegateProperty->SignatureFunction;
		ExtraArgs = FString::Printf(TEXT(", %s"), *GetSingletonName(TargetFunction));
	}

	FString Constructor = FString::Printf(TEXT("new(%s, TEXT(\"%s\"), RF_Public|RF_Transient|RF_Native) U%s(%s, 0x%016llx%s);"),
		*OuterString,
		*Prop->GetName(), 
		*Prop->GetClass()->GetName(), 
		*PropMacro,
		Prop->PropertyFlags & ~CPF_ComputedFlags, 
		*ExtraArgs);
	TheFlagAudit.Add(Prop, TEXT("PropertyFlags"), Prop->PropertyFlags);

	FString Symbol = FString::Printf(TEXT("NewProp_%s%s"), *Prop->GetName(), NameSuffix);
	FString Lines  = FString::Printf(TEXT("%sUProperty* %s = %s\r\n"), 
		Spaces, 
		*Symbol, 
		*Constructor);

	if (Prop->ArrayDim != 1)
	{
		Lines += FString::Printf(TEXT("%s%s->ArrayDim = CPP_ARRAY_DIM(%s, %s);\r\n"), Spaces, *Symbol, *PropNameDep, SourceStruct);
	}

	if (Prop->RepNotifyFunc != NAME_None)
	{
		Lines += FString::Printf(TEXT("%s%s->RepNotifyFunc = FName(TEXT(\"%s\"));\r\n"), Spaces, *Symbol, *Prop->RepNotifyFunc.ToString());
	}
	Meta += GetMetaDataCodeForObject(Prop, *Symbol, Spaces);
	return Lines;
}

void FNativeClassHeaderGenerator::OutputProperties(FString& Meta, FOutputDevice& OutputDevice, const FString& OuterString, TArray<UProperty*>& Properties, const TCHAR* Spaces)
{
	bool bEmittedHasEditorOnlyMacro = false;
	for (int32 Index = Properties.Num() - 1; Index >= 0; Index--)
	{
		bool RequiresHasEditorOnlyMacro = Properties[Index]->IsEditorOnlyProperty();
		if  (!bEmittedHasEditorOnlyMacro && RequiresHasEditorOnlyMacro)
		{
			// Indent code and export CPP text.
			OutputDevice.Logf( TEXT("#if WITH_EDITORONLY_DATA\r\n") );
			bEmittedHasEditorOnlyMacro = true;
		}
		else if (bEmittedHasEditorOnlyMacro && !RequiresHasEditorOnlyMacro)
		{
			OutputDevice.Logf( TEXT("#endif // WITH_EDITORONLY_DATA\r\n") );
			bEmittedHasEditorOnlyMacro = false;
		}
		OutputProperty(Meta, OutputDevice, OuterString, Properties[Index], TEXT("            "));
	}
	if (bEmittedHasEditorOnlyMacro)
	{
		OutputDevice.Logf( TEXT("#endif // WITH_EDITORONLY_DATA\r\n") );
	}
}

inline FString GetEventStructParamsName(const TCHAR* ClassName, const TCHAR* FunctionName)
{
	FString Result = FString::Printf(TEXT("%s_event%s_Parms"), ClassName, FunctionName);
	return Result;
}

void FNativeClassHeaderGenerator::OutputProperty(FString& Meta, FOutputDevice& OutputDevice, const FString& OuterString, UProperty* Prop, const TCHAR* Spaces)
{
	{
		FString SourceStruct;
		UFunction* Function = Cast<UFunction>(Prop->GetOuter());
		if (Function)
		{
			while (Function->GetSuperFunction())
			{
				Function = Function->GetSuperFunction();
			}
			FString FunctionName = Function->GetName();
			if( Function->HasAnyFunctionFlags( FUNC_Delegate ) )
			{
				FunctionName = FunctionName.LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
			}

			SourceStruct = GetEventStructParamsName(*CastChecked<UClass>(Function->GetOuter())->GetName(), *FunctionName);
		}
		else
		{			
			SourceStruct = NameLookupCPP->GetNameCPP(CastChecked<UStruct>(Prop->GetOuter()));
		}
		FString PropMacroOuterClass;
		FString PropNameDep = Prop->GetName();
		if (Prop->HasAllPropertyFlags(CPF_Deprecated))
		{
			 PropNameDep += TEXT("_DEPRECATED");
		}
		UBoolProperty* BoolProperty = Cast<UBoolProperty>(Prop);
		if (BoolProperty)
		{
			OutputDevice.Logf(TEXT("%sCPP_BOOL_PROPERTY_BITMASK_STRUCT(%s, %s, %s);\r\n"), 
				Spaces, 
				*PropNameDep, 
				*SourceStruct,
				*BoolProperty->GetCPPType(NULL, 0));
			PropMacroOuterClass = FString::Printf(TEXT("FPostConstructInitializeProperties(), EC_CppProperty, CPP_BOOL_PROPERTY_OFFSET(%s, %s)"),
				*PropNameDep, *SourceStruct);
		}
		else
		{
			PropMacroOuterClass = FString::Printf(TEXT("CPP_PROPERTY_BASE(%s, %s)"), *PropNameDep, *SourceStruct);
		}
		OutputDevice.Log(*PropertyNew(Meta, Prop, OuterString, PropMacroOuterClass, TEXT(""), Spaces, *SourceStruct));
	}
	UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Prop);
	if (ArrayProperty)
	{
		FString InnerOuterString = FString::Printf(TEXT("NewProp_%s"), *Prop->GetName());
		FString PropMacroOuterArray = TEXT("FPostConstructInitializeProperties(), EC_CppProperty, 0");
		OutputDevice.Log(*PropertyNew(Meta, ArrayProperty->Inner, InnerOuterString, PropMacroOuterArray, TEXT("_Inner"), Spaces));
	}
}

static FString BackSlashAndIndent(const FString& Text)
{
	FString Trailer(TEXT("    \\\r\n    "));
	FString Result = FString(TEXT("    ")) + Text.Replace(TEXT("\r\n"), *Trailer);
	return Result.LeftChop(Trailer.Len()) + TEXT("\r\n");
}

static bool IsAlwaysAccessible(UScriptStruct* Script)
{
	FName ToTest = Script->GetFName();
	if (ToTest == NAME_Matrix)
	{
		return false; // special case, the C++ FMatrix does not have the same members.
	}
	bool Result = Script->HasDefaults(); // if we have cpp struct ops in it for UHT, then we can assume it is always accessible
	if( ToTest == NAME_Plane
		||	ToTest == NAME_Vector
		||	ToTest == NAME_Vector4 
		||	ToTest == NAME_Quat
		||	ToTest == NAME_Color
		)
	{
		check(Result);
	}
	return Result;
}

static void FindNoExportStructs(TArray<UScriptStruct*>& Structs, UStruct* Start)
{
	while (Start)
	{
		if (UScriptStruct* StartScript = Cast<UScriptStruct>(Start))
		{
			if (StartScript->StructFlags & STRUCT_Native)
			{
				break;
			}

			if (!IsAlwaysAccessible(StartScript)) // these are a special cases that already exists and if wrong if exported nievely
			{
				// this will topologically sort them in reverse order
				Structs.Remove(StartScript);
				Structs.Add(StartScript);
			}
		}

		for ( TFieldIterator<UProperty> ItInner(Start, EFieldIteratorFlags::ExcludeSuper); ItInner; ++ItInner )
		{
			UStructProperty* InnerStruct = Cast<UStructProperty>(*ItInner);
			UArrayProperty* InnerArray = Cast<UArrayProperty>(*ItInner);
			if (InnerArray)
			{
				InnerStruct = Cast<UStructProperty>(InnerArray->Inner);
			}
			if (InnerStruct)
			{
				FindNoExportStructs(Structs, InnerStruct->Struct);
			}
		}
		Start = Start->GetSuperStruct();
	}
}

FString GetPackageSingletonName(UPackage* Package)
{
	static FString ClassString = NameLookupCPP->GetNameCPP(UPackage::StaticClass());
	return FString(TEXT("Z_Construct_")) + ClassString + TEXT("_") + FPackageName::GetShortName(Package) + TEXT("()");
}

void FNativeClassHeaderGenerator::ExportGeneratedPackageInitCode(UPackage* Package)
{
	FString ApiString = FString::Printf(TEXT("%s_API "), *API);
	FString SingletonName(GetPackageSingletonName(Package));

	if( GeneratedFunctionBodyTextSplit.Num() == 0 )
	{
		new(GeneratedFunctionBodyTextSplit) FStringOutputDeviceCountLines;
	}
	FStringOutputDevice& GeneratedFunctionText = GeneratedFunctionBodyTextSplit[GeneratedFunctionBodyTextSplit.Num()-1];

	GeneratedFunctionDeclarations.Logf(TEXT("    %sclass UPackage* %s;\r\n"), *ApiString, *SingletonName);

	GeneratedFunctionText.Logf(TEXT("    UPackage* %s\r\n"), *SingletonName);
	GeneratedFunctionText.Logf(TEXT("    {\r\n"));
	GeneratedFunctionText.Logf(TEXT("        static UPackage* ReturnPackage = NULL;\r\n"));
	GeneratedFunctionText.Logf(TEXT("        if (!ReturnPackage)\r\n"));
	GeneratedFunctionText.Logf(TEXT("        {\r\n"));
	GeneratedFunctionText.Logf(TEXT("            ReturnPackage = CastChecked<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), NULL, FName(TEXT(\"%s\")), false, false));\r\n"), *Package->GetName());

	FString Meta = GetMetaDataCodeForObject(Package, TEXT("ReturnPackage"), TEXT("            "));
	if (Meta.Len())
	{
		GeneratedFunctionText.Logf(TEXT("#if WITH_METADATA\r\n"));
		GeneratedFunctionText.Logf(TEXT("            UMetaData* MetaData = ReturnPackage->GetMetaData();\r\n"));
		GeneratedFunctionText.Log(*Meta);
		GeneratedFunctionText.Logf(TEXT("#endif\r\n"));
	}

	GeneratedFunctionText.Logf(TEXT("            ReturnPackage->PackageFlags |= PKG_CompiledIn | 0x%08X;\r\n"), Package->PackageFlags & (PKG_ClientOptional|PKG_ServerSideOnly));
	TheFlagAudit.Add(Package, TEXT("PackageFlags"), Package->PackageFlags);
	{
		FGuid Guid;
		Guid.A = FCrc::StrCrc_DEPRECATED(*GeneratedFunctionText        .ToUpper());
		Guid.B = FCrc::StrCrc_DEPRECATED(*GeneratedFunctionDeclarations.ToUpper());
		GeneratedFunctionText.Logf(TEXT("            FGuid Guid;\r\n"));
		GeneratedFunctionText.Logf(TEXT("            Guid.A = 0x%08X;\r\n"), Guid.A);
		GeneratedFunctionText.Logf(TEXT("            Guid.B = 0x%08X;\r\n"), Guid.B);
		GeneratedFunctionText.Logf(TEXT("            Guid.C = 0x%08X;\r\n"), Guid.C);
		GeneratedFunctionText.Logf(TEXT("            Guid.D = 0x%08X;\r\n"), Guid.D);
		GeneratedFunctionText.Logf(TEXT("            ReturnPackage->SetGuid(Guid);\r\n"), Guid.D);
	}

	GeneratedFunctionText.Logf(TEXT("        }\r\n"));
	GeneratedFunctionText.Logf(TEXT("        return ReturnPackage;\r\n"));
	GeneratedFunctionText.Logf(TEXT("    }\r\n"));

}

void FNativeClassHeaderGenerator::ExportNativeGeneratedInitCode(UClass* Class)
{
	// Emit the IMPLEMENT_CLASS macro to go in the generated cpp file (unless it's the temporary UHT class).
	if (!Class->HasAnyClassFlags(CLASS_Temporary))
	{
		GeneratedPackageCPP.Logf(TEXT("    IMPLEMENT_CLASS(%s);\r\n"),NameLookupCPP->GetNameCPP(Class));
	}

	int32 MaxLinesPerCpp = 30000;

	if ((GeneratedFunctionBodyTextSplit.Num() == 0) || (GeneratedFunctionBodyTextSplit[GeneratedFunctionBodyTextSplit.Num()-1].GetLineCount() > MaxLinesPerCpp))
	{
		new(GeneratedFunctionBodyTextSplit) FStringOutputDeviceCountLines;
	}
	FStringOutputDevice& GeneratedFunctionText = GeneratedFunctionBodyTextSplit[GeneratedFunctionBodyTextSplit.Num()-1];

	check(!FriendText.Len());
	// Emit code to build the UObjects that used to be in .u files
	bool bIsNoExport = Class->HasAnyClassFlags(CLASS_NoExport|CLASS_Temporary);
	FStringOutputDevice BodyText;
	FStringOutputDevice CallSingletons;
	FString ApiString = FString::Printf(TEXT("%s_API "), *API);

	// enums
	for ( TFieldIterator<UEnum> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It )
	{
		UEnum* Enum = *It;

		FString SingletonName(GetSingletonName(Enum));

		CallSingletons.Logf(TEXT("            OuterClass->LinkChild(%s);\r\n"), *SingletonName);

		FString Extern = FString::Printf(TEXT("    %sclass UEnum* %s;\r\n"), *ApiString, *SingletonName);
		SingletonNameToExternDecl.Add(SingletonName, Extern);
		GeneratedFunctionDeclarations.Log(*Extern);
			
		GeneratedFunctionText.Logf(TEXT("    UEnum* %s\r\n"), *SingletonName);
		GeneratedFunctionText.Logf(TEXT("    {\r\n"));
		// Enums can either have a UClass or UPackage as outer (if declared in non-UClass header).
		if (Enum->GetOuter()->IsA(UStruct::StaticClass()))
		{
			GeneratedFunctionText.Logf(TEXT("        UClass* Outer=%s;\r\n"), *GetSingletonName(CastChecked<UStruct>(Enum->GetOuter())));
		}
		else
		{
			GeneratedFunctionText.Logf(TEXT("        UPackage* Outer=%s;\r\n"), *GetPackageSingletonName(CastChecked<UPackage>(Enum->GetOuter())));
		}
		GeneratedFunctionText.Logf(TEXT("        static UEnum* ReturnEnum = NULL;\r\n"));
		GeneratedFunctionText.Logf(TEXT("        if (!ReturnEnum)\r\n"));
		GeneratedFunctionText.Logf(TEXT("        {\r\n"));

		GeneratedFunctionText.Logf(TEXT("            ReturnEnum = new(Outer, TEXT(\"%s\"), RF_Public|RF_Transient|RF_Native) UEnum(FPostConstructInitializeProperties());\r\n"), *Enum->GetName());
		GeneratedFunctionText.Logf(TEXT("            TArray<FName> EnumNames;\r\n"));
		for (int32 Index = 0; Index < Enum->NumEnums(); Index++)
		{
			GeneratedFunctionText.Logf(TEXT("            EnumNames.Add(FName(TEXT(\"%s\")));\r\n"), *Enum->GetEnum(Index).ToString());
		}
		GeneratedFunctionText.Logf(TEXT("            ReturnEnum->SetEnums(EnumNames, %s);\r\n"), Enum->IsNamespaceEnum() ? TEXT("true") : TEXT("false"));

		FString Meta = GetMetaDataCodeForObject(Enum, TEXT("ReturnEnum"), TEXT("            "));
		if (Meta.Len())
		{
			GeneratedFunctionText.Logf(TEXT("#if WITH_METADATA\r\n"));
			GeneratedFunctionText.Logf(TEXT("            UMetaData* MetaData = ReturnEnum->GetOutermost()->GetMetaData();\r\n"));
			GeneratedFunctionText.Log(*Meta);
			GeneratedFunctionText.Logf(TEXT("#endif\r\n"));
		}

		GeneratedFunctionText.Logf(TEXT("        }\r\n"));
		GeneratedFunctionText.Logf(TEXT("        return ReturnEnum;\r\n"));
		GeneratedFunctionText.Logf(TEXT("    }\r\n"));
	}

	// structs
	for ( TFieldIterator<UScriptStruct> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It )
	{
		UScriptStruct* ScriptStruct = *It;
		UStruct* BaseStruct = ScriptStruct->GetSuperStruct();

		FString SingletonName(GetSingletonName(ScriptStruct));

		CallSingletons.Logf(TEXT("            OuterClass->LinkChild(%s);\r\n"), *SingletonName);
		FString Extern = FString::Printf(TEXT("    %sclass UScriptStruct* %s;\r\n"), *ApiString, *SingletonName);
		SingletonNameToExternDecl.Add(SingletonName, Extern);
		GeneratedFunctionDeclarations.Log(*Extern);

		GeneratedFunctionText.Logf(TEXT("    UScriptStruct* %s\r\n"), *SingletonName);
		GeneratedFunctionText.Logf(TEXT("    {\r\n"));

		// if this is a no export struct, we will put a local struct here for offset determination
		TArray<UScriptStruct*> Structs;
		FindNoExportStructs(Structs, ScriptStruct);
		if (Structs.Num())
		{
			TGuardValue<bool> Guard(bIsExportingForOffsetDeterminationOnly,true);
			ExportMirrorsForNoexportStructs(Structs, 8, GeneratedFunctionText);
		}

		// Structs can either have a UClass or UPackage as outer (if delcared in non-UClass header).
		if (ScriptStruct->GetOuter()->IsA(UStruct::StaticClass()))
		{
			GeneratedFunctionText.Logf(TEXT("        UStruct* Outer=%s;\r\n"), *GetSingletonName(CastChecked<UStruct>(ScriptStruct->GetOuter())));
		}
		else
		{
			GeneratedFunctionText.Logf(TEXT("        UPackage* Outer=%s;\r\n"), *GetPackageSingletonName(CastChecked<UPackage>(ScriptStruct->GetOuter())));
		}
		GeneratedFunctionText.Logf(TEXT("        static UScriptStruct* ReturnStruct = NULL;\r\n"));
		GeneratedFunctionText.Logf(TEXT("        if (!ReturnStruct)\r\n"));
		GeneratedFunctionText.Logf(TEXT("        {\r\n"));
		FString BaseStructString(TEXT("NULL"));
		if (BaseStruct)
		{
			CastChecked<UScriptStruct>(BaseStruct); // this better actually be a script struct
			BaseStructString = GetSingletonName(BaseStruct);
		}
		FString CppStructOpsString(TEXT("NULL"));
		FString ExplicitSizeString;
		FString ExplicitAlignmentString;
		if ( (ScriptStruct->StructFlags&STRUCT_Native) != 0 )
		{
			//@todo .u we don't need the auto register versions of these (except for hotreload, which should be fixed)
			CppStructOpsString = FString::Printf( TEXT("new UScriptStruct::TCppStructOps<%s>"), NameLookupCPP->GetNameCPP(ScriptStruct) );
		}
		else
		{
			ExplicitSizeString = FString::Printf( TEXT(", sizeof(%s), ALIGNOF(%s)"), NameLookupCPP->GetNameCPP(ScriptStruct), NameLookupCPP->GetNameCPP(ScriptStruct) );
		}

		GeneratedFunctionText.Logf(TEXT("            ReturnStruct = new(Outer, TEXT(\"%s\"), RF_Public|RF_Transient|RF_Native) UScriptStruct(FPostConstructInitializeProperties(), %s, %s, EStructFlags(0x%08X)%s);\r\n"), 
			*ScriptStruct->GetName(),
			*BaseStructString,
			*CppStructOpsString,
			(uint32)(ScriptStruct->StructFlags & ~STRUCT_ComputedFlags),
			*ExplicitSizeString
			);
		TheFlagAudit.Add(ScriptStruct, TEXT("StructFlags"), (uint64)(ScriptStruct->StructFlags & ~STRUCT_ComputedFlags));

		TArray<UProperty*> Props;
		for ( TFieldIterator<UProperty> ItInner(ScriptStruct, EFieldIteratorFlags::ExcludeSuper); ItInner; ++ItInner )
		{
			Props.Add(*ItInner);
		}
		FString OuterString = FString(TEXT("ReturnStruct"));
		FString Meta = GetMetaDataCodeForObject(ScriptStruct, *OuterString, TEXT("            "));
		OutputProperties(Meta, GeneratedFunctionText, OuterString, Props, TEXT("            "));
		GeneratedFunctionText.Logf(TEXT("            ReturnStruct->StaticLink();\r\n"));

		if (Meta.Len())
		{
			GeneratedFunctionText.Logf(TEXT("#if WITH_METADATA\r\n"));
			GeneratedFunctionText.Logf(TEXT("            UMetaData* MetaData = ReturnStruct->GetOutermost()->GetMetaData();\r\n"));
			GeneratedFunctionText.Log(*Meta);
			GeneratedFunctionText.Logf(TEXT("#endif\r\n"));
		}

		GeneratedFunctionText.Logf(TEXT("        }\r\n"));
		GeneratedFunctionText.Logf(TEXT("        return ReturnStruct;\r\n"));
		GeneratedFunctionText.Logf(TEXT("    }\r\n"));

	}

	TArray<UFunction*> FunctionsToExport;
	for( TFieldIterator<UFunction> Function(Class,EFieldIteratorFlags::ExcludeSuper); Function; ++Function )
	{
		FunctionsToExport.Add(*Function);
	}

	// Sort the list of functions
	FunctionsToExport.Sort();

	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(Class);

	// Export the init code for each function
	for (int32 FuncIndex = 0; FuncIndex < FunctionsToExport.Num(); FuncIndex++)
	{
		UFunction* Function = FunctionsToExport[FuncIndex];
		UFunction* SuperFunction = Function->GetSuperFunction();

		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);

		const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();

		FString SingletonName(GetSingletonName(Function));

		CallSingletons.Logf(TEXT("            OuterClass->LinkChild(%s);\r\n"), *SingletonName);
		FString Extern = FString::Printf(TEXT("    %sclass UFunction* %s;\r\n"), *ApiString, *SingletonName);
		SingletonNameToExternDecl.Add(SingletonName, Extern);
		GeneratedFunctionDeclarations.Log(*Extern);

		GeneratedFunctionText.Logf(TEXT("    UFunction* %s\r\n"), *SingletonName);
		GeneratedFunctionText.Logf(TEXT("    {\r\n"));

		if (bIsNoExport || !(Function->FunctionFlags&FUNC_Event))  // non-events do not export a params struct, so lets do that locally for offset determination
		{
			TGuardValue<bool> Guard(bIsExportingForOffsetDeterminationOnly,true);
			TArray<UScriptStruct*> Structs;
			FindNoExportStructs(Structs, Function);
			if (Structs.Num())
			{
				ExportMirrorsForNoexportStructs(Structs, 8, GeneratedFunctionText);
			}
			TArray<UFunction*> CallbackFunctions;
			CallbackFunctions.Add(Function);
			ExportEventParms(CallbackFunctions, NULL, 8, false, &GeneratedFunctionText);
		}

		GeneratedFunctionText.Logf(TEXT("        UClass* OuterClass=%s;\r\n"), *GetSingletonName(Class));
		GeneratedFunctionText.Logf(TEXT("        static UFunction* ReturnFunction = NULL;\r\n"));
		GeneratedFunctionText.Logf(TEXT("        if (!ReturnFunction)\r\n"));
		GeneratedFunctionText.Logf(TEXT("        {\r\n"));
		FString SuperFunctionString(TEXT("NULL"));
		if (SuperFunction)
		{
			SuperFunctionString = GetSingletonName(SuperFunction);
		}
		TArray<UProperty*> Props;
		for ( TFieldIterator<UProperty> ItInner(Function,EFieldIteratorFlags::ExcludeSuper); ItInner; ++ItInner )
		{
			Props.Add(*ItInner);
		}
		FString StructureSize;
		if (Props.Num())
		{
			UFunction* TempFunction = Function;
			while (TempFunction->GetSuperFunction())
			{
				TempFunction = TempFunction->GetSuperFunction();
			}
			FString FunctionName = TempFunction->GetName();
			if( TempFunction->HasAnyFunctionFlags( FUNC_Delegate ) )
			{
				FunctionName = FunctionName.LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
			}

			StructureSize = FString::Printf(TEXT(", sizeof(%s)"), *GetEventStructParamsName(*CastChecked<UClass>(TempFunction->GetOuter())->GetName(), *FunctionName));
		}

		GeneratedFunctionText.Logf(TEXT("            ReturnFunction = new(OuterClass, TEXT(\"%s\"), RF_Public|RF_Transient|RF_Native) UFunction(FPostConstructInitializeProperties(), %s, 0x%08X, %d%s);\r\n"), 
			*Function->GetName(),
			*SuperFunctionString,
			Function->FunctionFlags,
			(uint32)Function->RepOffset,
			*StructureSize
			);
		TheFlagAudit.Add(Function, TEXT("FunctionFlags"), Function->FunctionFlags);
		

		FString OuterString = FString(TEXT("ReturnFunction"));
		FString Meta        = GetMetaDataCodeForObject(Function, *OuterString, TEXT("            "));

		for (int32 Index = Props.Num() - 1; Index >= 0; Index--)
		{
			OutputProperty(Meta, GeneratedFunctionText, OuterString, Props[Index], TEXT("            "));
		}

		if (FunctionData.FunctionFlags & (FUNC_NetRequest | FUNC_NetResponse))
		{
			GeneratedFunctionText.Logf(TEXT("            ReturnFunction->RPCId=%d;\r\n"), FunctionData.RPCId);
			GeneratedFunctionText.Logf(TEXT("            ReturnFunction->RPCResponseId=%d;\r\n"), FunctionData.RPCResponseId);
		}

		GeneratedFunctionText.Logf(TEXT("            ReturnFunction->Bind();\r\n"));
		GeneratedFunctionText.Logf(TEXT("            ReturnFunction->StaticLink();\r\n"));

		if (Meta.Len())
		{
			GeneratedFunctionText.Logf(TEXT("#if WITH_METADATA\r\n"));
			GeneratedFunctionText.Logf(TEXT("            UMetaData* MetaData = ReturnFunction->GetOutermost()->GetMetaData();\r\n"));
			GeneratedFunctionText.Log(*Meta);
			GeneratedFunctionText.Logf(TEXT("#endif\r\n"));
		}

		GeneratedFunctionText.Logf(TEXT("        }\r\n"));
		GeneratedFunctionText.Logf(TEXT("        return ReturnFunction;\r\n"));
		GeneratedFunctionText.Logf(TEXT("    }\r\n"));
	}

	// The class itself (unless it's a temporary UHT class).
	if (!Class->HasAnyClassFlags(CLASS_Temporary))
	{
		// simple ::StaticClass wrapper to avoid header, link and DLL hell
		{
			FString SingletonNameNoRegister(GetSingletonName(Class, false));

			FString Extern = FString::Printf(TEXT("    %sclass UClass* %s;\r\n"), *ApiString, *SingletonNameNoRegister);
			SingletonNameToExternDecl.Add(SingletonNameNoRegister, Extern);
			GeneratedFunctionDeclarations.Log(*Extern);

			GeneratedFunctionText.Logf(TEXT("    UClass* %s\r\n"), *SingletonNameNoRegister);
			GeneratedFunctionText.Logf(TEXT("    {\r\n"));
			GeneratedFunctionText.Logf(TEXT("        return %s::StaticClass();\r\n"), NameLookupCPP->GetNameCPP(Class));
			GeneratedFunctionText.Logf(TEXT("    }\r\n"));


		}
		FString SingletonName(GetSingletonName(Class));

		FriendText.Logf(TEXT("    friend %sclass UClass* %s;\r\n"), *ApiString, *SingletonName);
		FString Extern = FString::Printf(TEXT("    %sclass UClass* %s;\r\n"), *ApiString, *SingletonName);
		SingletonNameToExternDecl.Add(SingletonName, Extern);
		GeneratedFunctionDeclarations.Log(*Extern);

		GeneratedFunctionText.Logf(TEXT("    UClass* %s\r\n"), *SingletonName);
		GeneratedFunctionText.Logf(TEXT("    {\r\n"));
		GeneratedFunctionText.Logf(TEXT("        static UClass* OuterClass = NULL;\r\n"));
		GeneratedFunctionText.Logf(TEXT("        if (!OuterClass)\r\n"));
		GeneratedFunctionText.Logf(TEXT("        {\r\n"));
		if (Class->GetSuperClass() && Class->GetSuperClass() != Class)
		{
			GeneratedFunctionText.Logf(TEXT("            %s;\r\n"), *GetSingletonName(Class->GetSuperClass()));
		}
		GeneratedFunctionText.Logf(TEXT("            %s;\r\n"), *GetPackageSingletonName(CastChecked<UPackage>(Class->GetOutermost())));
		GeneratedFunctionText.Logf(TEXT("            OuterClass = %s::StaticClass();\r\n"), NameLookupCPP->GetNameCPP(Class));
		GeneratedFunctionText.Logf(TEXT("            UObjectForceRegistration(OuterClass);\r\n"));
		uint32 Flags = Class->ClassFlags & CLASS_SaveInCompiledInClasses;
		GeneratedFunctionText.Logf(TEXT("            OuterClass->ClassFlags |= 0x%08X;\r\n"), Flags);
		//if (Class->IsChildOf(UObject::StaticClass()))
		//{
		//	GeneratedFunctionText.Logf(TEXT("            OuterClass->ClassAddReferencedObjects = &%s::AddReferencedObjects;\r\n"), NameLookupCPP->GetNameCPP(Class));
		//}
		TheFlagAudit.Add(Class, TEXT("ClassFlags"), Flags);
		GeneratedFunctionText.Logf(TEXT("\r\n"));
		GeneratedFunctionText.Log(CallSingletons);
		GeneratedFunctionText.Logf(TEXT("\r\n"));

		FString OuterString = FString(TEXT("OuterClass"));
		FString Meta = GetMetaDataCodeForObject(Class, *OuterString, TEXT("            "));
		// properties
		{
			TArray<UProperty*> Props;
			for ( TFieldIterator<UProperty> ItInner(Class,EFieldIteratorFlags::ExcludeSuper); ItInner; ++ItInner )
			{
				Props.Add(*ItInner);
			}
			OutputProperties(Meta, GeneratedFunctionText, OuterString, Props, TEXT("            "));
		}
		// function table
		{

			// Grab and sort the list of functions in the function map
			TArray<UFunction*> FunctionsInMap;
			for (auto Function : TFieldRange<UFunction>(Class, EFieldIteratorFlags::ExcludeSuper))
			{
				FunctionsInMap.Add(Function);
			}
			FunctionsInMap.Sort();

			// Emit code to construct each UFunction and rebuild the function map at runtime
			for (UFunction* Function : FunctionsInMap)
			{
				GeneratedFunctionText.Logf(TEXT("            OuterClass->AddFunctionToFunctionMap(%s);\r\n"), *GetSingletonName(Function));
			}
		}

		// class flags are handled by the intrinsic bootstrap code
		//GeneratedFunctionText.Logf(TEXT("            OuterClass->ClassFlags = 0x%08X;\r\n"), Class->ClassFlags);
		if (Class->ClassConfigName != NAME_None)
		{
			GeneratedFunctionText.Logf(TEXT("            OuterClass->ClassConfigName = FName(TEXT(\"%s\"));\r\n"), *Class->ClassConfigName.ToString());
		}

		for (auto& Inter : Class->Interfaces)
		{
			check(Inter.Class);
			FString OffsetString(TEXT("0"));
			if (Inter.PointerOffset)
			{
				OffsetString = FString::Printf(TEXT("VTABLE_OFFSET(%s, %s)"), NameLookupCPP->GetNameCPP(Class), NameLookupCPP->GetNameCPP(Inter.Class, true));
			}
			GeneratedFunctionText.Logf(TEXT("            OuterClass->Interfaces.Add(FImplementedInterface(%s, %s, %s ));\r\n"), 
				*GetSingletonName(Inter.Class, false),
				*OffsetString,
				Inter.bImplementedByK2 ? TEXT("true") : TEXT("false")
				);
		}
		if (Class->ClassGeneratedBy)
		{
			UE_LOG(LogCompile, Fatal, TEXT("For intrinsic and compiled-in classes, ClassGeneratedBy should always be NULL"));
			GeneratedFunctionText.Logf(TEXT("            OuterClass->ClassGeneratedBy = %s;\r\n"), *GetSingletonName(CastChecked<UClass>(Class->ClassGeneratedBy), false));
		}

		GeneratedFunctionText.Logf(TEXT("            OuterClass->StaticLink();\r\n"));

		if (Meta.Len())
		{
			GeneratedFunctionText.Logf(TEXT("#if WITH_METADATA\r\n"));
			GeneratedFunctionText.Logf(TEXT("            UMetaData* MetaData = OuterClass->GetOutermost()->GetMetaData();\r\n"));
			GeneratedFunctionText.Log(*Meta);
			GeneratedFunctionText.Logf(TEXT("#endif\r\n"));
		}

		GeneratedFunctionText.Logf(TEXT("        }\r\n"));
		GeneratedFunctionText.Logf(TEXT("        check(OuterClass->GetClass());\r\n"));
		GeneratedFunctionText.Logf(TEXT("        return OuterClass;\r\n"));
		GeneratedFunctionText.Logf(TEXT("    }\r\n"));
	}
	if (FriendText.Len())
	{
		if (bIsNoExport)
		{
			GeneratedFunctionText.Logf(TEXT("    /* friend declarations for pasting into noexport class %s\r\n"), NameLookupCPP->GetNameCPP(Class));
			GeneratedFunctionText.Log(FriendText);
			GeneratedFunctionText.Logf(TEXT("    */\r\n"));
			FriendText.Empty();
		}
	}
	FString SingletonName(GetSingletonName(Class));
	SingletonName.ReplaceInline(TEXT("()"), TEXT("")); // function address

	// Skip temporary UHT classes.
	if (!Class->HasAnyClassFlags(CLASS_Temporary))
	{
		GeneratedFunctionText.Logf(TEXT("    static FCompiledInDefer Z_CompiledInDefer_UClass_%s(%s);\r\n"), NameLookupCPP->GetNameCPP(Class), *SingletonName);
	}
}

void FNativeClassHeaderGenerator::ExportNatives(UClass* Class)
{
	// Skip temporary UHT classes.
	if (Class->HasAnyClassFlags(CLASS_NoExport|CLASS_Temporary))
		return;

	GeneratedPackageCPP.Logf(TEXT("    void %s::StaticRegisterNatives%s()\r\n"),NameLookupCPP->GetNameCPP(Class),NameLookupCPP->GetNameCPP(Class));
	GeneratedPackageCPP.Logf(TEXT("    {\r\n"));

	{
		TArray<UFunction*> FunctionsToExport;
		for (auto* Function : TFieldRange<UFunction>(Class, EFieldIteratorFlags::ExcludeSuper))
		{
			if( (Function->FunctionFlags & (FUNC_Native | FUNC_NetRequest)) == FUNC_Native )
			{
				FunctionsToExport.Add(Function);
			}
		}

		FunctionsToExport.Sort();

		for (auto Func : FunctionsToExport)
		{
			GeneratedPackageCPP.Logf(TEXT("        FNativeFunctionRegistrar::RegisterFunction(%s::StaticClass(),\"%s\",(Native)&%s::exec%s);\r\n"),
				NameLookupCPP->GetNameCPP(Class),
				*Func->GetName(),
				Class->HasAnyClassFlags(CLASS_Interface) ? *FString::Printf(TEXT("I%s"), *Class->GetName()) : NameLookupCPP->GetNameCPP(Class),
				*Func->GetName()
			);
		}
	}

	for (auto* Struct : TFieldRange<UScriptStruct>(Class, EFieldIteratorFlags::ExcludeSuper))
	{
		if (Struct->StructFlags & STRUCT_Native)
		{
			GeneratedPackageCPP.Logf( TEXT("        UScriptStruct::DeferCppStructOps(FName(TEXT(\"%s\")),new UScriptStruct::TCppStructOps<%s%s>);\r\n"), *Struct->GetName(), Struct->GetPrefixCPP(), *Struct->GetName() );
		}
	}

	GeneratedPackageCPP.Logf(TEXT("    }\r\n"));
}

void FNativeClassHeaderGenerator::ExportInterfaceCallFunctions( const TArray<UFunction*>& InCallbackFunctions, FStringOutputDevice& HeaderOutput )
{
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	TArray<UFunction*> CallbackFunctions = InCallbackFunctions;
	CallbackFunctions.Sort();

	for (auto FuncIt = CallbackFunctions.CreateConstIterator(); FuncIt; ++FuncIt)
	{
		UFunction* Function     = *FuncIt;
		FString    FunctionName = Function->GetName();
		FString    ClassName    = CurrentClass->GetName();

		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);

		const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();
		const TCHAR* ConstQualifier = FunctionData.FunctionReference->HasAllFunctionFlags(FUNC_Const) ? TEXT("const ") : TEXT("");
		FString ExtraParam = FString::Printf(TEXT("%sUObject* O"), ConstQualifier);

		ExportNativeFunctionHeader(FunctionData, HeaderOutput, EExportFunctionType::Interface, EExportFunctionHeaderStyle::Declaration, *ExtraParam);
		HeaderOutput.Logf( TEXT(";%s"), LINE_TERMINATOR );


		ExportNativeFunctionHeader(FunctionData, GeneratedPackageCPP, EExportFunctionType::Interface, EExportFunctionHeaderStyle::Definition, *ExtraParam);
		GeneratedPackageCPP.Logf( TEXT("%s%s{%s"), LINE_TERMINATOR, FCString::Spc(4), LINE_TERMINATOR );

		GeneratedPackageCPP.Logf(TEXT("%scheck(O != NULL);%s"), FCString::Spc(8), LINE_TERMINATOR);
		GeneratedPackageCPP.Logf(TEXT("%scheck(O->GetClass()->ImplementsInterface(U%s::StaticClass()));%s"), FCString::Spc(8), *ClassName, LINE_TERMINATOR);

		auto Parameters = GetFunctionParmsAndReturn(FunctionData.FunctionReference);

		// See if we need to create Parms struct
		bool bHasParms = Parameters.HasParms();
		if (bHasParms)
		{
			FString EventParmStructName = GetEventStructParamsName(*Function->GetOwnerClass()->GetName(), *FunctionName);
			GeneratedPackageCPP.Logf(TEXT("%s%s Parms;%s"), FCString::Spc(8), *EventParmStructName, LINE_TERMINATOR);
		}

		GeneratedPackageCPP.Logf(TEXT("%sUFunction* const Func = O->FindFunction(%s_%s);%s"), FCString::Spc(8), *API, *FunctionName, LINE_TERMINATOR);
		GeneratedPackageCPP.Logf(TEXT("%sif (Func)%s"), FCString::Spc(8), LINE_TERMINATOR);
		GeneratedPackageCPP.Logf(TEXT("%s{%s"), FCString::Spc(8), LINE_TERMINATOR);

		// code to populate Parms struct
		for (auto It = Parameters.Parms.CreateConstIterator(); It; ++It)
		{
			UProperty* Param = *It;

			GeneratedPackageCPP.Logf(TEXT("%sParms.%s=%s;%s"), FCString::Spc(12), *Param->GetName(), *Param->GetName(), LINE_TERMINATOR);
		}

		GeneratedPackageCPP.Logf(TEXT("%sO->ProcessEvent(Func, %s);%s"), FCString::Spc(12), bHasParms ? TEXT("&Parms") : TEXT("NULL"), LINE_TERMINATOR);

		for (auto It = Parameters.Parms.CreateConstIterator(); It; ++It)
		{
			UProperty* Param = *It;

			if( Param->HasAllPropertyFlags(CPF_OutParm) && !Param->HasAnyPropertyFlags(CPF_ConstParm|CPF_ReturnParm))
			{
				GeneratedPackageCPP.Logf(TEXT("%s%s=Parms.%s;%s"), FCString::Spc(12), *Param->GetName(), *Param->GetName(), LINE_TERMINATOR);
			}
		}

		GeneratedPackageCPP.Logf(TEXT("%s}%s"), FCString::Spc(8), LINE_TERMINATOR);
		

		// else clause to call back into native if it's a BlueprintNativeEvent
		if (Function->FunctionFlags & FUNC_Native)
		{
			GeneratedPackageCPP.Logf(TEXT("%selse if (auto I = (%sI%s*)(O->GetNativeInterfaceAddress(U%s::StaticClass())))%s"), FCString::Spc(8), ConstQualifier, *ClassName, *ClassName, LINE_TERMINATOR);
			GeneratedPackageCPP.Logf(TEXT("%s{%s"), FCString::Spc(8), LINE_TERMINATOR);

			GeneratedPackageCPP.Logf(FCString::Spc(12));
			if (Parameters.Return)
			{
				GeneratedPackageCPP.Logf(TEXT("Parms.ReturnValue = "));
			}

			GeneratedPackageCPP.Logf(TEXT("I->%s_Implementation("), *FunctionName);

			bool First = true;
			for (auto It = Parameters.Parms.CreateConstIterator(); It; ++It)
			{
				UProperty* Param = *It;

				if (!First)
				{
					GeneratedPackageCPP.Logf(TEXT(","));
				}
				First = false;

				GeneratedPackageCPP.Logf(TEXT("%s"), *Param->GetName());
			}

			GeneratedPackageCPP.Logf(TEXT(");%s"), LINE_TERMINATOR);

			GeneratedPackageCPP.Logf(TEXT("%s}%s"), FCString::Spc(8), LINE_TERMINATOR);
		}

		if (Parameters.Return)
		{
			GeneratedPackageCPP.Logf(TEXT("%sreturn Parms.ReturnValue;%s"), FCString::Spc(8), LINE_TERMINATOR);
		}

		GeneratedPackageCPP.Logf(TEXT("%s}%s"), FCString::Spc(4), LINE_TERMINATOR);

		//
		// also generate an empty implementation for the base event function, since we did the Execute_ version above
		//
		ExportNativeFunctionHeader(FunctionData, GeneratedPackageCPP, EExportFunctionType::Event, EExportFunctionHeaderStyle::Definition, NULL);
		GeneratedPackageCPP.Logf(TEXT("%s%s{%s"), LINE_TERMINATOR, FCString::Spc(4), LINE_TERMINATOR);

		// assert if this is ever called directly
		GeneratedPackageCPP.Logf(TEXT("%scheck(0 && \"Do not directly call Event functions in Interfaces. Call Execute_%s instead.\");%s"), FCString::Spc(8), *FunctionName, LINE_TERMINATOR);

		// satisfy compiler if it's expecting a return value
		if (Parameters.Return)
		{
			FString EventParmStructName = GetEventStructParamsName(*Function->GetOwnerClass()->GetName(), *FunctionName);
			GeneratedPackageCPP.Logf(TEXT("%s%s Parms;%s"), FCString::Spc(8), *EventParmStructName, LINE_TERMINATOR);
			GeneratedPackageCPP.Logf(TEXT("%sreturn Parms.ReturnValue;%s"), FCString::Spc(8), LINE_TERMINATOR);
		}
		GeneratedPackageCPP.Logf(TEXT("%s}%s"), FCString::Spc(4), LINE_TERMINATOR);
	}
}

void FNativeClassHeaderGenerator::ExportNames()
{
	// Skip temporary UHT classes.
	if (CurrentClass->HasAnyClassFlags(CLASS_NoExport|CLASS_Temporary))
	{
		return;
	}
	GeneratedHeaderText.Logf(
		TEXT("#ifdef %s_%s_generated_h")													LINE_TERMINATOR
		TEXT("#error \"%s.generated.h already included, missing '#pragma once' in %s.h\"")	LINE_TERMINATOR
		TEXT("#endif")																		LINE_TERMINATOR
		TEXT("#define %s_%s_generated_h")													LINE_TERMINATOR
		LINE_TERMINATOR,
		*API, *CurrentClass->GetName(), *CurrentClass->GetName(), *CurrentClass->GetName(), *API, *CurrentClass->GetName()
		);

	TSet<FName> ClassReferencedNames;
	for( TFieldIterator<UFunction> Function(CurrentClass,EFieldIteratorFlags::ExcludeSuper); Function; ++Function )
	{
		if ( Function->HasAnyFunctionFlags(FUNC_Event) && !Function->GetSuperFunction() )
		{
			FName FunctionName = Function->GetFName();
			ClassReferencedNames.Add(FunctionName);
			ReferencedNames.Add(FunctionName);
		}
	}
	if (!ClassReferencedNames.Num())
	{
		return;
	}

	TArray<FString> Names;
	for (const auto& Name : ClassReferencedNames)
	{
		Names.Add(Name.ToString());
	}	
	Names.Sort();

	for( int32 NameIndex=0; NameIndex<Names.Num(); NameIndex++ )
	{
		GeneratedHeaderText.Logf( TEXT("extern %s_API FName %s_%s;") LINE_TERMINATOR, *API, *API, *Names[NameIndex] );
	}

}

/**
 * Exports the C++ class declarations for a native interface class.
 */
void FNativeClassHeaderGenerator::ExportInterfaceClassDeclaration( UClass* Class )
{
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	TArray<UEnum*>			Enums;
	TArray<UScriptStruct*>	Structs;
	TArray<UFunction*>		CallbackFunctions;
	TArray<UFunction*>		NativeFunctions;
	TArray<UFunction*>		DelegateFunctions;

	// get the lists of fields that belong to this class that should be exported
	RetrieveRelevantFields(Class, Enums, Structs, CallbackFunctions, NativeFunctions, DelegateFunctions);

	ExportNames();

	// export enum declarations
	ExportEnums(Enums);

	// export delegate definitions
	ExportDelegateDefinitions(DelegateFunctions, false);

	// Export enums declared in non-UClass headers.
	ExportGeneratedEnumsInitCode(Enums);

	// export boilerplate macros for struct
	ExportGeneratedStructBodyMacros(Structs);

	// export parameters structs for all events and delegates
	ExportEventParms(CallbackFunctions, TEXT(""));

	// export delegate wrapper function implementations
	ExportDelegateDefinitions(DelegateFunctions, true);

	UClass* SuperClass = Class->GetSuperClass();


	// the name for the C++ version of the UClass
	const TCHAR* ClassCPPName		= NameLookupCPP->GetNameCPP( Class );
	const TCHAR* SuperClassCPPName	= NameLookupCPP->GetNameCPP( SuperClass );

	// =============================================
	// Export the UClass declaration
	// Class definition.
	ExportNatives(Class);
	ExportNativeGeneratedInitCode(Class);

	{
		FStringOutputDevice UInterfaceBoilerplate;

		// Export the class's native function registration.
		UInterfaceBoilerplate.Logf(TEXT("    private:\r\n"));
		UInterfaceBoilerplate.Logf(TEXT("    static void StaticRegisterNatives%s();\r\n"),NameLookupCPP->GetNameCPP(Class));
		UInterfaceBoilerplate.Log(*FriendText);
		FriendText.Empty();
		UInterfaceBoilerplate.Logf( TEXT("public:\r\n") );

		// Build the DECLARE_CLASS line
		FString APIArg(API);
		if( !Class->HasAnyClassFlags( CLASS_MinimalAPI ) )
		{
			APIArg = TEXT("NO");
		}

		const bool bCastedClass = Class->HasAnyCastFlag(CASTCLASS_AllFlags) && SuperClass && Class->ClassCastFlags != SuperClass->ClassCastFlags;
		UInterfaceBoilerplate.Logf( TEXT("    DECLARE_CLASS(%s, %s, COMPILED_IN_FLAGS(CLASS_Abstract%s), %s, %s, %s_API)\r\n"), 
			ClassCPPName, 
			SuperClassCPPName, 
			*GetClassFlagExportText(Class), 
			bCastedClass ? *FString::Printf(TEXT("CASTCLASS_%s"), ClassCPPName) : TEXT("0"),
			*FPackageName::GetShortName(Class->GetOuter()->GetName()), 
			*APIArg );
		if (!Class->HasAnyClassFlags(CLASS_CustomConstructor))
		{
			UInterfaceBoilerplate.Logf(TEXT("    %s_API %s(const class FPostConstructInitializeProperties& PCIP);\r\n"), *APIArg, ClassCPPName);
		}
		UInterfaceBoilerplate.Logf(TEXT("    DECLARE_SERIALIZER(%s)\r\n"), ClassCPPName);
		UInterfaceBoilerplate.Log(TEXT("    enum {IsIntrinsic=COMPILED_IN_INTRINSIC};\r\n"));

		if (Class->ClassWithin != Class->GetSuperClass()->ClassWithin)
		{
			UInterfaceBoilerplate.Logf(TEXT("    DECLARE_WITHIN(%s)\r\n"), NameLookupCPP->GetNameCPP( Class->ClassWithin ) );
		}

		FString MacroName = TEXT("GENERATED_UINTERFACE_BODY()");
		FString Macroized = Macroize(*MacroName, *UInterfaceBoilerplate);
		GeneratedHeaderText.Log(TEXT("#undef GENERATED_UINTERFACE_BODY\r\n"));
		GeneratedHeaderText.Log(*Macroized);
	}

	// =============================================
	// Export the pure interface version of the class

	// the name of the pure interface class
	FString InterfaceCPPName		= FString::Printf(TEXT("I%s"), *Class->GetName());
	FString SuperInterfaceCPPName;
	if ( SuperClass != NULL )
	{
		SuperInterfaceCPPName = FString::Printf(TEXT("I%s"), *SuperClass->GetName());
	}

	// C++ -> VM stubs (native function execs)
	ExportNativeFunctions(NativeFunctions);

	// VM -> C++ proxies (events).
	ExportCallbackFunctions(CallbackFunctions);

	// Thunk functions
	{
		FStringOutputDevice InterfaceBoilerplate;

		InterfaceBoilerplate.Logf( TEXT("protected:\r\n\tvirtual ~%s() {}\r\npublic:\r\n"), *InterfaceCPPName );
		InterfaceBoilerplate.Logf( TEXT("\ttypedef %s UClassType;\r\n"), ClassCPPName );

		ExportInterfaceCallFunctions(CallbackFunctions, InterfaceBoilerplate);

		// we'll need a way to get to the UObject portion of a native interface, so that we can safely pass native interfaces
		// to script VM functions
		if (SuperClass->IsChildOf(UInterface::StaticClass()))
		{
			InterfaceBoilerplate.Logf(TEXT("\tvirtual UObject* GetUObjectInterface%s()=0;\r\n"), *Class->GetName());
			InterfaceBoilerplate.Logf(TEXT("\tvirtual const UObject* GetUObjectInterface%s() const=0;\r\n"), *Class->GetName());
		}

		// Replication, add in the declaration for GetLifetimeReplicatedProps() automatically if there are any net flagged properties
		bool bNeedsRep = false;
		for( TFieldIterator<UProperty> It(Class,EFieldIteratorFlags::ExcludeSuper); It; ++It )
		{
			if( (It->PropertyFlags & CPF_Net) != 0 )
			{
				bNeedsRep = true;
				break;
			}
		}
		if (bNeedsRep)
		{
			InterfaceBoilerplate.Logf(TEXT("\tvirtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const OVERRIDE;\r\n"));
		}
		const TCHAR* Suffix = TEXT("_INCLASS_IINTERFACE");
		FString MacroName = FString(NameLookupCPP->GetNameCPP(Class)) + Suffix;
		FString Macroized = Macroize(*MacroName, *InterfaceBoilerplate);
		GeneratedHeaderText.Log(*Macroized);
		InClassMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(4),  *MacroName);
	}
}

/**
 * Helper function to find an element in a TArray of pointers by GetFName().
 * @param Array		TArray of pointers.
 * @param Name		Name to search for.
 * @return			Index of the found element, or INDEX_NONE if not found.
 */
template <typename T>
int32 FindByIndirectName( const TArray<T>& Array, const FName& Name )
{
	for ( int32 Index=0; Index < Array.Num(); ++Index )
	{
		if ( Array[Index]->GetFName() == Name )
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

/**
 * Appends the header definition for an inheritance hierarchy of classes to the header.
 * Wrapper for ExportClassHeaderRecursive().
 *
 * @param	Class				The class to be exported.
 */
void FNativeClassHeaderGenerator::ExportClassHeader( UClass* Class )
{
	TArray<UClass*> DependencyChain;
	VisitedMap.Empty( Classes.Num() );
	ExportClassHeaderRecursive( Class, DependencyChain, false );
}


void FNativeClassHeaderGenerator::ExportClassHeaderInner(UClass* Class, bool bValidNonTemporaryClass)
{
	if ( !Class->HasAnyClassFlags(CLASS_Interface) )
	{
		TArray<UEnum*>			Enums;
		TArray<UScriptStruct*>	Structs;
		TArray<UFunction*>		CallbackFunctions;
		TArray<UFunction*>		NativeFunctions;
		TArray<UFunction*>		DelegateFunctions;

		// get the lists of fields that belong to this class that should be exported
		RetrieveRelevantFields(Class, Enums, Structs, CallbackFunctions, NativeFunctions, DelegateFunctions);

		ExportNames();

		// export enum declarations
		ExportEnums(Enums);

		// export delegate definitions
		ExportDelegateDefinitions(DelegateFunctions, false);

		// Export enums declared in non-UClass headers.
		ExportGeneratedEnumsInitCode(Enums);

		// export boilerplate macros for structs
		ExportGeneratedStructBodyMacros(Structs);

		if (bValidNonTemporaryClass)
		{
			// export parameters structs for all events and delegates
			ExportEventParms(CallbackFunctions, TEXT(""));

			// export .proto files for any net service functions
			ExportProtoMessage(CallbackFunctions);

			// export .java files for any net service functions
			ExportMCPMessage(CallbackFunctions);

			// export delegate wrapper function implementations
			ExportDelegateDefinitions(DelegateFunctions, true);

			// Class definition.
			ExportNatives(Class);
		}

		ExportNativeGeneratedInitCode(Class);

		if (bValidNonTemporaryClass)
		{
			// C++ -> VM stubs (native function execs)
			ExportNativeFunctions(NativeFunctions);

			// VM -> C++ proxies (events and delegates).
			ExportCallbackFunctions(CallbackFunctions);

			UClass* SuperClass = Class->GetSuperClass();

			const TCHAR* ClassCPPName = NameLookupCPP->GetNameCPP( Class );
			const TCHAR* SuperClassCPPName = (SuperClass != NULL) ? NameLookupCPP->GetNameCPP( SuperClass ) : NULL;

			{
				FStringOutputDevice ClassBoilerplate;

				// Export the class's native function registration.
				ClassBoilerplate.Logf(TEXT("    private:\r\n"));
				ClassBoilerplate.Logf(TEXT("    static void StaticRegisterNatives%s();\r\n"),NameLookupCPP->GetNameCPP(Class));
				ClassBoilerplate.Log(FriendText);
				FriendText.Empty();
				ClassBoilerplate.Logf(TEXT("    public:\r\n"));

				// Build the DECLARE_CLASS line
				FString APIArg(API);
				if( !Class->HasAnyClassFlags( CLASS_MinimalAPI ) )
				{
					APIArg = TEXT("NO");
				}
				const bool bCastedClass = Class->HasAnyCastFlag(CASTCLASS_AllFlags) && SuperClass && Class->ClassCastFlags != SuperClass->ClassCastFlags;
				ClassBoilerplate.Logf( TEXT("    DECLARE_CLASS(%s, %s, COMPILED_IN_FLAGS(%s%s), %s, %s, %s_API)\r\n"), 
					ClassCPPName, 
					SuperClassCPPName ? SuperClassCPPName : TEXT("None"), 
					Class->HasAnyClassFlags(CLASS_Abstract) ? TEXT("CLASS_Abstract") : TEXT("0"),
					*GetClassFlagExportText(Class), 
					bCastedClass ? *FString::Printf(TEXT("CASTCLASS_%s"), ClassCPPName) : TEXT("0"),
					*FPackageName::GetShortName(*Class->GetOuter()->GetName()), 
					*APIArg );
				if (!Class->HasAnyClassFlags(CLASS_CustomConstructor))
				{
					ClassBoilerplate.Logf(TEXT("    /** Standard constructor, called after all reflected properties have been initialized */"));
					ClassBoilerplate.Logf(TEXT("    %s_API %s(const class FPostConstructInitializeProperties& PCIP);\r\n"), *APIArg, ClassCPPName);
				}
				ClassBoilerplate.Logf(TEXT("    DECLARE_SERIALIZER(%s)\r\n"), ClassCPPName);
				ClassBoilerplate.Log(TEXT("    /** Indicates whether the class is compiled into the engine */"));
				ClassBoilerplate.Log(TEXT("    enum {IsIntrinsic=COMPILED_IN_INTRINSIC};\r\n"));

				if(SuperClass && Class->ClassWithin != SuperClass->ClassWithin)
				{
					ClassBoilerplate.Logf(TEXT("    DECLARE_WITHIN(%s)\r\n"), NameLookupCPP->GetNameCPP( Class->ClassWithin ) );
				}

				// export the class's config name
				if ( SuperClass && Class->ClassConfigName != NAME_None && Class->ClassConfigName != SuperClass->ClassConfigName )
				{
					ClassBoilerplate.Logf(TEXT("    static const TCHAR* StaticConfigName() {return TEXT(\"%s\");}\r\n\r\n"), *Class->ClassConfigName.ToString());
				}

				{
					// arrays for preventing multiple export of accessor function in situations where the native class
					// implements multiple children of the same interface base class
					TArray<UClass*> UObjectExportedInterfaces;
					TArray<UClass*> ExportedInterfaces;

					for (TArray<FImplementedInterface>::TIterator It(Class->Interfaces); It; ++It)
					{
						UClass* InterfaceClass = It->Class;
						if ( InterfaceClass->HasAnyClassFlags(CLASS_Native) )
						{
							for ( UClass* IClass = InterfaceClass; IClass && IClass->HasAnyClassFlags(CLASS_Interface); IClass = IClass->GetSuperClass() )
							{
								if ( IClass != UInterface::StaticClass() && !UObjectExportedInterfaces.Contains(IClass) )
								{
									UObjectExportedInterfaces.Add(IClass);
									ClassBoilerplate.Logf(TEXT("%svirtual UObject* GetUObjectInterface%s(){return this;}\r\n"), FCString::Spc(4), *IClass->GetName());
									ClassBoilerplate.Logf(TEXT("%svirtual const UObject* GetUObjectInterface%s() const{return this;}\r\n"), FCString::Spc(4), *IClass->GetName());
								}
							}
						}
					}
				}


				// Replication, add in the declaration for GetLifetimeReplicatedProps() automatically if there are any net flagged properties
				for( TFieldIterator<UProperty> It(Class,EFieldIteratorFlags::ExcludeSuper); It; ++It )
				{
					if( (It->PropertyFlags & CPF_Net) != 0 )
					{
						ClassBoilerplate.Logf(TEXT("\tvirtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const OVERRIDE;\r\n"));
						break;
					}
				}


				const TCHAR* Suffix = TEXT("_INCLASS");
				FString MacroName = FString(NameLookupCPP->GetNameCPP(Class)) + Suffix;
				FString Macroized = Macroize(*MacroName, *ClassBoilerplate);
				GeneratedHeaderText.Log(*Macroized);
				InClassMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(4),  *MacroName);
			}
		}
	}
	else
	{
		// this is an interface class
		ExportInterfaceClassDeclaration(Class);
	}
}

void FNativeClassHeaderGenerator::ExportClassHeaderWrapper( UClass* Class, bool bIsExportClass )
{
	const bool bTemporaryJunkClass = Class->HasAnyClassFlags(CLASS_Temporary);
	const bool bValidClass = !bTemporaryJunkClass;

	check(!GeneratedHeaderText.Len());
	ExportClassHeaderInner(Class, bValidClass);

	if (!GeneratedHeaderText.Len() && !EnumForeachText.Len())
	{
		// Nothing to export
		return;
	}

	GeneratedHeaderText.Logf(TEXT("#undef UCLASS_CURRENT_FILE_NAME\r\n"));
	GeneratedHeaderText.Logf(TEXT("#define UCLASS_CURRENT_FILE_NAME %s\r\n\r\n\r\n"), NameLookupCPP->GetNameCPP(CurrentClass));

	if (bValidClass)
	{
		{
			GeneratedHeaderText.Logf(TEXT("#undef UCLASS\r\n"));
			GeneratedHeaderText.Logf(TEXT("#undef UINTERFACE\r\n"));
			FString MacroName = FString::Printf(TEXT("%s(...)"), Class->HasAnyClassFlags(CLASS_Interface) ? TEXT("UINTERFACE") : TEXT("UCLASS"));
			FString Macroized = Macroize(*MacroName, *PrologMacroCalls);
			GeneratedHeaderText.Log(*Macroized);
			PrologMacroCalls.Empty();
		}

		{
			GeneratedHeaderText.Logf(TEXT("#undef GENERATED_UCLASS_BODY\r\n"));
			GeneratedHeaderText.Logf(TEXT("#undef GENERATED_IINTERFACE_BODY\r\n"));
			FString MacroName = FString::Printf(TEXT("GENERATED_%s_BODY()"), Class->HasAnyClassFlags(CLASS_Interface) ? TEXT("IINTERFACE") : TEXT("UCLASS"));
			FString Macroized = Macroize(*MacroName, *(FString(TEXT("public:\r\n")) + InClassMacroCalls + TEXT("public:\r\n")));
			GeneratedHeaderText.Log(*Macroized);
			InClassMacroCalls.Empty();
		}
	}

	const FString SourceFilename = GClassSourceFileMap[Class];
	const FString PkgName        = FPackageName::GetShortName(Package);

	FString NewFileName = SourceFilename.EndsWith(TEXT(".h")) ? SourceFilename : (SourceFilename + TEXT(".h"));

	FString PkgDir;
	FString GeneratedIncludeDirectory;
	if (MakeCommandlet_FindPackageLocation(*PkgName, PkgDir, GeneratedIncludeDirectory) == false)
	{
		UE_LOG(LogCompile, Error, TEXT("Failed to find path for package %s"), *PkgName);
	}

	if (bIsExportClass)
	{
		const FString ClassHeaderPath(GeneratedIncludeDirectory / GClassHeaderNameWithNoPathMap[Class] + TEXT(".generated.h") );

		GeneratedHeaderText += EnumForeachText;

		FStringOutputDevice GeneratedHeaderTextWithCopyright;

		GeneratedHeaderTextWithCopyright.Log(
			TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.\r\n")
			TEXT("/*===========================================================================\r\n")
			TEXT("    C++ class header boilerplate exported from UnrealHeaderTool.\r\n")
			TEXT("    This is automatically generated by the tools.\r\n")
			TEXT("    DO NOT modify this manually! Edit the corresponding .h files instead!\r\n")
			TEXT("===========================================================================*/\r\n"));
		GeneratedHeaderTextWithCopyright.Log(
			LINE_TERMINATOR
			TEXT("#include \"ObjectBase.h\"") LINE_TERMINATOR
			LINE_TERMINATOR);
		GeneratedHeaderTextWithCopyright.Log(*GeneratedHeaderText);

		SaveHeaderIfChanged(*ClassHeaderPath, *GeneratedHeaderTextWithCopyright);

		check(SourceFilename.EndsWith(TEXT(".h")));

		if(bUseRelativePaths)
		{
			// When building Rocket, we need relative paths in the generated headers so that they can be installed to any directory.
			FPaths::MakePathRelativeTo(NewFileName, *ClassHeaderPath);
			NewFileName.ReplaceInline(TEXT("\\"), TEXT("/"));
		}
		// ...we cannot rebase paths relative to the install directory. It may be on a different drive.
		else if (FPaths::MakePathRelativeTo(NewFileName, *CodeGeneneratorConfig.RootLocalPath))
		{
			NewFileName = CodeGeneneratorConfig.RootBuildPath / NewFileName;
		}

		// Keep track of all of the UObject headers for this module, in the same order that we digest them in
		// @todo uht: We're wrapping these includes in checks for header guards, ONLY because most existing UObject headers are missing '#pragma once'
		ListOfAllUObjectHeaderIncludes.Logf(
			TEXT("#ifndef %s_%s_generated_h")													LINE_TERMINATOR
			TEXT("    #include \"%s\"")															LINE_TERMINATOR
			TEXT("#endif")																		LINE_TERMINATOR,
			*API, *CurrentClass->GetName(), *NewFileName );

		// For the 'Classes' mega-header, only include legacy UObjects classes from the 'Classes' folder.  Private and Public classes are
		// not included here -- you should include those yourself if you need them!
		const bool bIsPublicClassesHeader = GPublicClassSet.Contains( Class );
		if( bIsPublicClassesHeader )
		{
			ListOfPublicClassesUObjectHeaderIncludes.Logf(TEXT("#include \"%s\"\r\n"), *NewFileName);
		}
	}

	GeneratedHeaderText.Empty();
	EnumForeachText.Empty();
}

/**
 * Appends the header definition for an inheritance hierarchy of classes to the header.
 *
 * @param	Class					The class to be exported.
 * @param	DependencyChain			Used for finding errors. Must be empty before the first call.
 * @param	bCheckDependenciesOnly	Whether we should just keep checking for dependency errors, without exporting anything.
 */
void FNativeClassHeaderGenerator::ExportClassHeaderRecursive( UClass* Class, TArray<UClass*>& DependencyChain, bool bCheckDependenciesOnly )
{
	bool bIsExportClass   = GClassStrippedHeaderTextMap.Contains(Class) && Class->HasAnyClassFlags(CLASS_Native|CLASS_Temporary) && !Class->HasAnyClassFlags(CLASS_NoExport);
	bool bIsCorrectHeader = GClassHeaderFilenameMap.FindRef(Class) == ClassHeaderFilename && Class->GetOuter() == Package;

	// Check for circular header dependencies between export classes.
	if ( bIsExportClass )
	{
		if ( bIsCorrectHeader == false )
		{
			if ( DependencyChain.Num() == 0 )
			{
				// The first export class we found doesn't belong in this header: No need to keep exporting along this dependency path.
				return;
			}
			else
			{
				// From now on, we're not going to export anything. Instead, we're going to check that no deeper dependency tries to export to this header file.
				bCheckDependenciesOnly = true;
			}
		}
		else if ( bCheckDependenciesOnly && !ClassHeaderFilename.IsEmpty() )
		{
			FString DependencyChainString;
			for ( int32 DependencyIndex=0; DependencyIndex < DependencyChain.Num(); DependencyIndex++ )
			{
				UClass* DependencyClass = DependencyChain[DependencyIndex];
				DependencyChainString += DependencyClass->GetName() + TEXT(" -> ");
			}
			DependencyChainString += Class->GetName();
			UE_LOG(LogCompile, Warning, TEXT("Circular header dependency detected (%s), while exporting %s!"), *DependencyChainString, *(FPackageName::GetShortName(Package) + ClassHeaderFilename + TEXT("Classes.h")) );
		}
	}

	// Check if the Class has already been exported, after we've checked for circular header dependencies.
	if (GExportedClasses.Contains(Class))
	{
		return;
	}

	// Check for circular dependencies.
	bool* bVisited = VisitedMap.Find( Class );
	if ( bVisited && *bVisited == true )
	{
		UE_LOG(LogCompile, Error, TEXT("Circular dependency detected for class %s!"), *Class->GetName() );
		return;
	}
	// Temporarily mark the Class as VISITED. Make sure to clear this flag before returning!
	VisitedMap.Add( Class, true );

	if ( bIsExportClass )
	{
		DependencyChain.Add( Class );
	}

	// Export the super class first.
	if (UClass* SuperClass = Class->GetSuperClass())
	{
		ExportClassHeaderRecursive(SuperClass, DependencyChain, bCheckDependenciesOnly);
	}

	// Export all classes we depend on.
	for( auto It = GClassDependentOnMap.FindOrAdd(Class)->CreateConstIterator(); It; ++It )
	{
		const FName& DependencyClassName = *It;

		// By this point, all classes should have been validated so we can strip the given class name when searching.		
		FString DependencyClassNameStripped(DependencyClassName.ToString());

		// Handle #include directives as 'dependson' (allowing them to fail).
		const bool bClassNameFromHeader = FHeaderParser::DependentClassNameFromHeader(*DependencyClassNameStripped, DependencyClassNameStripped);
		DependencyClassNameStripped = FHeaderParser::GetClassNameWithPrefixRemoved( DependencyClassNameStripped );

		int32 ClassIndex = FindByIndirectName( Classes, *DependencyClassNameStripped );
		UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *DependencyClassNameStripped);
		// Only export the class if it's in Classes array (it may be in another package).
		UClass* DependsOnClass = (ClassIndex != INDEX_NONE) ? FoundClass : NULL;
		if (FoundClass == NULL)
		{
			// Try again with temporary class name. This may be struct only header.
			DependencyClassNameStripped = FHeaderParser::GetClassNameWithoutPrefix(DependencyClassName.ToString());
			DependencyClassNameStripped = FHeaderParser::GenerateTemporaryClassName(*DependencyClassNameStripped);
			ClassIndex = FindByIndirectName( Classes, *DependencyClassNameStripped );
			FoundClass = FindObject<UClass>(ANY_PACKAGE, *DependencyClassNameStripped);
			DependsOnClass = (ClassIndex != INDEX_NONE) ? FoundClass : NULL;
		}

		if (DependsOnClass)
		{
			ExportClassHeaderRecursive(DependsOnClass, DependencyChain, bCheckDependenciesOnly);
		}
		else if ( !FoundClass && !bClassNameFromHeader )
		{
			UE_LOG(LogCompile, Error, TEXT("Unknown class %s used in dependson declaration in class %s!"), *DependencyClassName.ToString(), *Class->GetName() );
		}
	}

	// Export class header.
	if ( bIsCorrectHeader && !bCheckDependenciesOnly && !Class->HasAnyClassFlags(CLASS_Intrinsic) && Class->HasAnyClassFlags(CLASS_Native|CLASS_Temporary) )
	{
		CurrentClass = Class;

		// Mark class as exported.
		GExportedClasses.Add(Class);

		ExportClassHeaderWrapper(Class, bIsExportClass);
	}

	// We're done visiting this Class.
	VisitedMap.Add( Class, false );
	if ( bIsExportClass )
	{
		DependencyChain.RemoveAtSwap( DependencyChain.Num() - 1 );
	}
}

/**
 * Returns a string in the format CLASS_Something|CLASS_Something which represents all class flags that are set for the specified
 * class which need to be exported as part of the DECLARE_CLASS macro
 */
FString FNativeClassHeaderGenerator::GetClassFlagExportText( UClass* Class )
{
	FString StaticClassFlagText;

	check(Class);
	if ( Class->HasAnyClassFlags(CLASS_Transient) )
	{
		StaticClassFlagText += TEXT(" | CLASS_Transient");
	}				
	if( Class->HasAnyClassFlags(CLASS_DefaultConfig) )
	{
		StaticClassFlagText += TEXT(" | CLASS_DefaultConfig");
	}
	if( Class->HasAnyClassFlags(CLASS_Config) )
	{
		StaticClassFlagText += TEXT(" | CLASS_Config");
	}
	if ( Class->HasAnyClassFlags(CLASS_Interface) )
	{
		StaticClassFlagText += TEXT(" | CLASS_Interface");
	}
	if ( Class->HasAnyClassFlags(CLASS_Deprecated) )
	{
		StaticClassFlagText += TEXT(" | CLASS_Deprecated");
	}

	return StaticClassFlagText;
}

void FNativeClassHeaderGenerator::RetrieveRelevantFields(UClass* Class, TArray<UEnum*>& Enums, TArray<UScriptStruct*>& Structs, TArray<UFunction*>& CallbackFunctions, TArray<UFunction*>& NativeFunctions, TArray<UFunction*>& DelegateFunctions)
{
	for ( TFieldIterator<UField> It(Class,EFieldIteratorFlags::ExcludeSuper); It; ++It )
	{
		UField* CurrentField = *It;
		UClass* FieldClass = CurrentField->GetClass();
		if ( FieldClass == UEnum::StaticClass() )
		{
			UEnum* Enum = (UEnum*)CurrentField;
			Enums.Add(Enum);
		}

		else if ( FieldClass == UScriptStruct::StaticClass() )
		{
			UScriptStruct* Struct = (UScriptStruct*)CurrentField;
			Structs.Add(Struct);
		}

		else if ( FieldClass == UFunction::StaticClass() )
		{
			bool bAdded = false;
			UFunction* Function = (UFunction*)CurrentField;

			if ( (Function->FunctionFlags&(FUNC_Event)) != 0 &&
				Function->GetSuperFunction() == NULL )
			{
				CallbackFunctions.Add(Function);
				bAdded = true;
			}

			else if ( (Function->FunctionFlags&(FUNC_Delegate)) != 0 &&
				Function->GetSuperFunction() == NULL )
			{
				DelegateFunctions.Add(Function);
				bAdded = true;
			}

			if ( (Function->FunctionFlags&FUNC_Native) != 0 )
			{
				NativeFunctions.Add(Function);
				bAdded = true;
			}

			check(bAdded)
		}
	}
}

/**
* Exports the header text for the list of enums specified
*
* @param	Enums	the enums to export
*/
void FNativeClassHeaderGenerator::ExportEnums( const TArray<UEnum*>& Enums )
{
	// Enum definitions.
	for( int32 EnumIdx = Enums.Num() - 1; EnumIdx >= 0; EnumIdx-- )
	{
		UEnum* Enum = Enums[EnumIdx];

		FString QualifierPrefix; 
		if (!Enum->ActualEnumNameInsideNamespace.IsEmpty())
		{
			QualifierPrefix = Enum->GetName() + TEXT("::");
		}

		// Export FOREACH macro
		EnumForeachText.Logf( TEXT("#define FOREACH_ENUM_%s(op) "), *Enum->GetName().ToUpper() );
		for (int32 i = 0; i < Enum->NumEnums() - 1; i++)
		{
			const FString QualifiedEnumValue = Enum->GetEnum(i).ToString();
			EnumForeachText.Logf( TEXT("\\\r\n    op(%s) "), *QualifiedEnumValue );
		}
		EnumForeachText.Logf( TEXT("\r\n") );
	}
}

// Exports the header text for the list of structs specified (GENERATED_USTRUCT_BODY impls)
void FNativeClassHeaderGenerator::ExportGeneratedStructBodyMacros(const TArray<UScriptStruct*>& NativeStructs)
{
	// reverse the order.
	for (int32 i = NativeStructs.Num() - 1; i >= 0; --i)
	{
		UScriptStruct* Struct = NativeStructs[i];

		// Export struct.
		if (Struct->StructFlags & STRUCT_Native)
		{
			check(Struct->StructMacroDeclaredLineNumber != INDEX_NONE);

			const FString FriendApiString = FString::Printf(TEXT("%s_API "), *API);
			const FString StaticConstructionString = GetSingletonName(Struct);

			FString RequiredAPI;
			if (!(Struct->StructFlags & STRUCT_RequiredAPI))
			{
				RequiredAPI = FriendApiString;
			}

			const FString FriendLine = FString::Printf(TEXT("    friend %sclass UScriptStruct* %s;\r\n"), *FriendApiString, *StaticConstructionString);
			const FString StaticClassLine = FString::Printf(TEXT("    %sstatic class UScriptStruct* StaticStruct();\r\n"), *RequiredAPI);

			const FString CombinedLine = FriendLine + StaticClassLine;
			const FString MacroName = 
				FString::Printf(TEXT("%s_USTRUCT_BODY_LINE_%d"), NameLookupCPP->GetNameCPP(CurrentClass), Struct->StructMacroDeclaredLineNumber);

			const FString Macroized = Macroize(*MacroName, *CombinedLine);
			GeneratedHeaderText.Log(*Macroized);

			FString SingletonName = StaticConstructionString.Replace(TEXT("()"), TEXT("")); // function address

			GeneratedPackageCPP.Logf(TEXT("class UScriptStruct* %s::StaticStruct()\r\n"), NameLookupCPP->GetNameCPP(Struct));
			GeneratedPackageCPP.Logf(TEXT("{\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    static class UScriptStruct* Singleton = NULL;\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    if (!Singleton)\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    {\r\n"));
			GeneratedPackageCPP.Logf(TEXT("        extern %sclass UScriptStruct* %s;\r\n"), *FriendApiString, *StaticConstructionString);

			// UStructs can have UClass or UPackage outer (if declared in non-UClass headers).
			if (Struct->GetOuter()->IsA(UStruct::StaticClass()))
			{
				GeneratedPackageCPP.Logf(TEXT("        Singleton = GetStaticStruct(%s, %s::StaticClass(), TEXT(\"%s\"));\r\n"), *SingletonName, NameLookupCPP->GetNameCPP(CastChecked<UStruct>(Struct->GetOuter())), *Struct->GetName());
			}
			else
			{
				FString PackageSingletonName = GetPackageSingletonName(CastChecked<UPackage>(Struct->GetOuter()));
				GeneratedPackageCPP.Logf(TEXT("        extern %sclass UPackage* %s;\r\n"), *FriendApiString, *PackageSingletonName);
				GeneratedPackageCPP.Logf(TEXT("        Singleton = GetStaticStruct(%s, %s, TEXT(\"%s\"));\r\n"), *SingletonName, *PackageSingletonName, *Struct->GetName());
			}
			
			GeneratedPackageCPP.Logf(TEXT("    }\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    return Singleton;\r\n"));
			GeneratedPackageCPP.Logf(TEXT("}\r\n"));
			GeneratedPackageCPP.Logf(TEXT("static FCompiledInDeferStruct Z_CompiledInDeferStruct_UScriptStruct_%s(%s::StaticStruct);\r\n"), NameLookupCPP->GetNameCPP(Struct), NameLookupCPP->GetNameCPP(Struct));

			// Generate StaticRegisterNatives equivalent for structs without classes.
			if (!Struct->GetOuter()->IsA(UStruct::StaticClass()))
			{
				const FString ShortPackageName = FPackageName::GetShortName(Struct->GetOuter()->GetName());
				GeneratedPackageCPP.Logf(TEXT("static struct FScriptStruct_%s_StaticRegisterNatives%s\r\n"), *ShortPackageName, NameLookupCPP->GetNameCPP(Struct));
				GeneratedPackageCPP.Logf(TEXT("{\r\n"));
				GeneratedPackageCPP.Logf(TEXT("\tFScriptStruct_%s_StaticRegisterNatives%s()\r\n"), *ShortPackageName, NameLookupCPP->GetNameCPP(Struct));
				GeneratedPackageCPP.Logf(TEXT("\t{\r\n"));

				GeneratedPackageCPP.Logf( TEXT("\t\tUScriptStruct::DeferCppStructOps(FName(TEXT(\"%s\")),new UScriptStruct::TCppStructOps<%s%s>);\r\n"), *Struct->GetName(), Struct->GetPrefixCPP(), *Struct->GetName() );

				GeneratedPackageCPP.Logf(TEXT("\t}\r\n"));
				GeneratedPackageCPP.Logf(TEXT("} ScriptStruct_%s_StaticRegisterNatives%s;\r\n"), *ShortPackageName, NameLookupCPP->GetNameCPP(Struct));
			}
		}
	}
}

void FNativeClassHeaderGenerator::ExportGeneratedEnumsInitCode(const TArray<UEnum*>& Enums)
{
	// reverse the order.
	for (int32 i = Enums.Num() - 1; i >= 0; --i)
	{
		UEnum* Enum = Enums[i];

		// Export Enum.
		if (Enum->GetOuter()->IsA(UPackage::StaticClass()))
		{
			const FString FriendApiString = FString::Printf(TEXT("%s_API "), *API);
			const FString StaticConstructionString = GetSingletonName(Enum);

			FString SingletonName = StaticConstructionString.Replace(TEXT("()"), TEXT("")); // function address

			GeneratedPackageCPP.Logf(TEXT("static class UEnum* %s_StaticEnum()\r\n"), *Enum->GetName());
			GeneratedPackageCPP.Logf(TEXT("{\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    static class UEnum* Singleton = NULL;\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    if (!Singleton)\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    {\r\n"));
			GeneratedPackageCPP.Logf(TEXT("        extern %sclass UEnum* %s;\r\n"), *FriendApiString, *StaticConstructionString);

			FString PackageSingletonName = GetPackageSingletonName(CastChecked<UPackage>(Enum->GetOuter()));
			GeneratedPackageCPP.Logf(TEXT("        extern %sclass UPackage* %s;\r\n"), *FriendApiString, *PackageSingletonName);
			GeneratedPackageCPP.Logf(TEXT("        Singleton = GetStaticEnum(%s, %s, TEXT(\"%s\"));\r\n"), *SingletonName, *PackageSingletonName, *Enum->GetName());
			
			GeneratedPackageCPP.Logf(TEXT("    }\r\n"));
			GeneratedPackageCPP.Logf(TEXT("    return Singleton;\r\n"));
			GeneratedPackageCPP.Logf(TEXT("}\r\n"));
			GeneratedPackageCPP.Logf(TEXT("static FCompiledInDeferEnum Z_CompiledInDeferEnum_UEnum_%s(%s_StaticEnum);\r\n"), *Enum->GetName(), *Enum->GetName());
		}
	}
}

void FNativeClassHeaderGenerator::ExportMirrorsForNoexportStructs(const TArray<UScriptStruct*>& NativeStructs, int32 TextIndent, FStringOutputDevice& HeaderOutput)
{
	// reverse the order.
	for (int32 i = NativeStructs.Num() - 1; i >= 0; --i)
	{
		UScriptStruct* Struct = NativeStructs[i];

		// Export struct.
		HeaderOutput.Logf( TEXT("%sstruct %s"), FCString::Spc(TextIndent), NameLookupCPP->GetNameCPP( Struct ) );
		if (Struct->GetSuperStruct() != NULL)
		{
			HeaderOutput.Logf(TEXT(" : public %s"), NameLookupCPP->GetNameCPP(Struct->GetSuperStruct()));
		}
		HeaderOutput.Logf( TEXT("\r\n%s{\r\n"), FCString::Spc(TextIndent) );

		// Export the struct's CPP properties.
		ExportProperties(Struct, TextIndent, /*bAccessSpecifiers=*/ false, &HeaderOutput);

		HeaderOutput.Logf( TEXT("%s};\r\n"), FCString::Spc(TextIndent) );

		HeaderOutput.Log( TEXT("\r\n") );
	}
}


/**
	* Exports the parameter struct declarations for the list of functions specified
	* 
	* @param	Function	the function that (may) have parameters which need to be exported
	* @return	true		if the structure generated is not completely empty
	*/
bool FNativeClassHeaderGenerator::WillExportEventParms( UFunction* Function )
{
	for( TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags&CPF_Parm); ++It )
	{
		return true;
	}
	return false;
}

void WriteEventFunctionPrologue(FStringOutputDevice& Output, int32 Indent, const FParmsAndReturnProperties& Parameters, const TCHAR* ClassName, const TCHAR* FunctionName)
{
	// now the body - first we need to declare a struct which will hold the parameters for the event/delegate call
	Output.Logf( TEXT("\r\n%s{\r\n"), FCString::Spc(Indent) );

	// declare and zero-initialize the parameters and return value, if applicable
	if (!Parameters.HasParms())
		return;

	FString EventStructName = GetEventStructParamsName(ClassName, FunctionName);

	Output.Logf( TEXT("%s%s Parms;\r\n"), FCString::Spc(Indent + 4), *EventStructName );

	// Declare a parameter struct for this event/delegate and assign the struct members using the values passed into the event/delegate call.
	for (auto It = Parameters.Parms.CreateConstIterator(); It; ++It)
	{
		UProperty* Prop = *It;

		const FString PropertyName = Prop->GetName();
		if ( Prop->ArrayDim > 1 )
		{
			Output.Logf( TEXT("%sFMemory::Memcpy(Parms.%s,%s,sizeof(Parms.%s));\r\n"), FCString::Spc(Indent + 4), *PropertyName, *PropertyName, *PropertyName );
		}
		else
		{
			FString ValueAssignmentText = PropertyName;
			if (Prop->IsA<UBoolProperty>())
			{
				ValueAssignmentText += TEXT(" ? true : false");
			}

			Output.Logf( TEXT("%sParms.%s=%s;\r\n"), FCString::Spc(Indent + 4), *PropertyName, *ValueAssignmentText );
		}
	}
}

void WriteEventFunctionEpilogue(FStringOutputDevice& Output, int32 Indent, const FParmsAndReturnProperties& Parameters, const TCHAR* ClassName, const TCHAR* FunctionName)
{
	// Out parm copying.
	for (auto It = Parameters.Parms.CreateConstIterator(); It; ++It)
	{
		UProperty* Prop = *It;

		if (Prop->HasAnyPropertyFlags(CPF_OutParm) && (!Prop->HasAnyPropertyFlags(CPF_ConstParm) || Prop->IsA<UObjectPropertyBase>()))
		{
			const FString PropertyName = Prop->GetName();
			if ( Prop->ArrayDim > 1 )
			{
				Output.Logf( TEXT("%sFMemory::Memcpy(&%s,&Parms.%s,sizeof(%s));\r\n"), FCString::Spc(Indent + 4), *PropertyName, *PropertyName, *PropertyName );
			}
			else
			{
				Output.Logf(TEXT("%s%s=Parms.%s;\r\n"), FCString::Spc(Indent + 4), *PropertyName, *PropertyName);
			}
		}
	}

	// Return value.
	if (Parameters.Return)
	{
		// Make sure uint32 -> bool is supported
		bool bBoolProperty = Parameters.Return->IsA(UBoolProperty::StaticClass());
		Output.Logf( TEXT("%sreturn %sParms.%s;\r\n"), FCString::Spc(Indent + 4), bBoolProperty ? TEXT("!!") : TEXT(""), *Parameters.Return->GetName() );
	}
	Output.Logf( TEXT("%s}\r\n"), FCString::Spc(Indent) );
}

/**
 * Exports C++ type definitions for delegates
 *
 * @param	DelegateFunctions	the functions that have parameters which need to be exported
 * @param	bWrapperImplementationsOnly	 True if only function implementations for delegate wrappers should be exported
 */
void FNativeClassHeaderGenerator::ExportDelegateDefinitions( const TArray<UFunction*>& DelegateFunctions, const bool bWrapperImplementationsOnly )
{
	FStringOutputDevice HeaderOutput;
	if( bWrapperImplementationsOnly )
	{
		// Export parameters structs for all delegates.  We'll need these to declare our delegate execution function.
		ExportEventParms( DelegateFunctions, TEXT(""), 0, true, &HeaderOutput);
	}

	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);
	UClass*         Class     = CurrentClass;
	const FString   ClassName = Class->GetName();

	for ( int32 i = DelegateFunctions.Num() - 1; i >= 0 ; i-- )
	{
		UFunction* Function = DelegateFunctions[i];
		check( Function->HasAnyFunctionFlags( FUNC_Delegate ) );

		const bool bIsMulticastDelegate = Function->HasAnyFunctionFlags( FUNC_MulticastDelegate );

		// Unmangle the function name
		const FString DelegateName = Function->GetName().LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );

		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);

		FFuncInfo FunctionData = CompilerInfo->GetFunctionData();

		if( bWrapperImplementationsOnly )
		{
			// Always export delegate wrapper functions as inline
			FunctionData.FunctionExportFlags |= FUNCEXPORT_Inline;
		}

		// Add class name to beginning of function, to avoid collisions with other classes with the same delegate name in this scope
		FString Delegate(TEXT("delegate"));
		check(FunctionData.MarshallAndCallName.StartsWith(Delegate));
		FString ShortName = *FunctionData.MarshallAndCallName + Delegate.Len();
		FunctionData.MarshallAndCallName = FString::Printf( TEXT( "F%s_DelegateWrapper" ), *ShortName );

		// Setup delegate parameter
		const FString ExtraParam( FString::Printf( TEXT( "const %s& %s" ),
			bIsMulticastDelegate ? TEXT( "FMulticastScriptDelegate" ) : TEXT( "FScriptDelegate" ),
			*DelegateName ) );

		// export the line that looks like: int32 Main(const FString& Parms)
		ExportNativeFunctionHeader(FunctionData,HeaderOutput,EExportFunctionType::Event,EExportFunctionHeaderStyle::Declaration,*ExtraParam);

		if( !bWrapperImplementationsOnly )
		{
			// Only exporting function prototype
			HeaderOutput.Logf( TEXT( ";\r\n" ) );
		}
		else
		{
			auto Parameters = GetFunctionParmsAndReturn(FunctionData.FunctionReference);

			WriteEventFunctionPrologue(HeaderOutput, 0, Parameters, *ClassName, *DelegateName);
			{
				const TCHAR* DelegateType = bIsMulticastDelegate ? TEXT( "ProcessMulticastDelegate" ) : TEXT( "ProcessDelegate" );
				const TCHAR* DelegateArg  = Parameters.HasParms() ? TEXT("&Parms") : TEXT("NULL");
				HeaderOutput.Logf( TEXT("%s%s.%s<UObject>(%s);\r\n"), FCString::Spc(4), *DelegateName, DelegateType, DelegateArg );
			}
			WriteEventFunctionEpilogue(HeaderOutput, 0, Parameters, *ClassName, *DelegateName);
		}
	}

	if (HeaderOutput.Len())
	{
		if( !bWrapperImplementationsOnly )
		{
			// these are at the top, so we can output directly
			GeneratedHeaderText.Log(*HeaderOutput);
			GeneratedHeaderText.Log(TEXT("\r\n\r\n"));
		}
		else
		{
			const TCHAR* Suffix = TEXT("_DELEGATES");
			FString MacroName = FString(NameLookupCPP->GetNameCPP(CurrentClass)) + Suffix;
			FString Macroized = Macroize(*MacroName, *HeaderOutput);
			GeneratedHeaderText.Log(*Macroized);
			PrologMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(0),  *MacroName);
		}
	}
}

/**
 * Export a single .proto declaration, recursing as necessary for sub declarations
 *
 * @param Out output device
 * @param MessageName name of the message in the declaration
 * @param Properties array of parameters in the function definition
 * @param PropertyFlags flags to filter property array against
 * @param Ident starting indentation level
 */
void ExportProtoDeclaration(FOutputDevice& Out, const FString& MessageName, TFieldIterator<UProperty>& Properties, uint64 PropertyFlags, int32 Indent)
{
	Out.Logf(TEXT("%smessage CMsg%sMessage\r\n"), FCString::Spc(Indent), *MessageName);
	Out.Logf(TEXT("%s{\r\n"), FCString::Spc(Indent));

	static TMap<FString, FString> ToProtoTypeMappings;
	static bool bInitMapping = false;
	if (!bInitMapping)
	{
		// Explicit type mappings
		ToProtoTypeMappings.Add(TEXT("FString"), TEXT("string"));
		ToProtoTypeMappings.Add(TEXT("int32"), TEXT("int32"));
		ToProtoTypeMappings.Add(TEXT("int64"), TEXT("int64"));
		ToProtoTypeMappings.Add(TEXT("uint8"), TEXT("bytes"));
		ToProtoTypeMappings.Add(TEXT("bool"), TEXT("bool"));
		ToProtoTypeMappings.Add(TEXT("double"), TEXT("double"));
		ToProtoTypeMappings.Add(TEXT("float"), TEXT("float"));
		bInitMapping = true;
	}

	int32 FieldIdx = 1;
	for(; Properties && (Properties->PropertyFlags & PropertyFlags); ++Properties)
	{
		UProperty* Property = *Properties;
		UClass* PropClass = Property->GetClass();

		// Skip out and return paramaters
		if ((Property->PropertyFlags & CPF_RepSkip) || (Property->PropertyFlags & CPF_ReturnParm))
		{
			continue;
		}

		// export the property type text (e.g. FString; int32; TArray, etc.)
		FString TypeText, ExtendedTypeText;
		TypeText = Property->GetCPPType(&ExtendedTypeText, CPPF_None);
		if (PropClass != UInterfaceProperty::StaticClass() && PropClass != UObjectProperty::StaticClass())
		{
			bool bIsRepeated = false;
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			if (ArrayProperty != NULL)
			{
				UClass* InnerPropClass = ArrayProperty->Inner->GetClass();
				if (InnerPropClass != UInterfaceProperty::StaticClass() && InnerPropClass != UObjectProperty::StaticClass())
				{
					FString InnerExtendedTypeText;
					FString InnerTypeText = ArrayProperty->Inner->GetCPPType(&InnerExtendedTypeText, CPPF_None);
					TypeText = InnerTypeText;
					ExtendedTypeText = InnerExtendedTypeText;
					Property = ArrayProperty->Inner;
					bIsRepeated = true;
				}
				else
				{
					FError::Throwf(TEXT("ExportProtoDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
				}
			}
			else if(Property->ArrayDim != 1)
			{
				bIsRepeated = true;
			}

			FString VariableTypeName = FString::Printf(TEXT("%s%s"), *TypeText, *ExtendedTypeText);
			FString* ProtoTypeName = NULL;

			UStructProperty* StructProperty = Cast<UStructProperty>(Property);
			if (StructProperty != NULL)
			{
				TFieldIterator<UProperty> StructIt(StructProperty->Struct);
				ExportProtoDeclaration(Out, VariableTypeName, StructIt, CPF_AllFlags, Indent + 4);
				VariableTypeName = FString::Printf(TEXT("CMsg%sMessage"), *VariableTypeName);
				ProtoTypeName = &VariableTypeName;
			}
			else
			{
				ProtoTypeName = ToProtoTypeMappings.Find(VariableTypeName);
			}

			Out.Log(FCString::Spc(Indent + 4));
			if (bIsRepeated)
			{
				Out.Log( TEXT("repeated "));
			}
			else
			{
				Out.Log( TEXT("optional "));
			}

			if (ProtoTypeName != NULL)
			{
				Out.Logf( TEXT("%s %s = %d;\r\n"), **ProtoTypeName, *Property->GetNameCPP(), FieldIdx);
				FieldIdx++;
			}
			else
			{
				FError::Throwf(TEXT("ExportProtoDeclaration - Unhandled property mapping '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
			}
		}
		else
		{
			FError::Throwf(TEXT("ExportProtoDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
		}
	}

	Out.Logf( TEXT("%s}\r\n"), FCString::Spc(Indent) );
}

/**
 * Generate a .proto message declaration for any functions marked as requiring one
 * 
 * @param InCallbackFunctions array of functions for consideration to generate .proto definitions
 * @param Indent starting indentation level
 * @param Output optional output redirect
 */
void FNativeClassHeaderGenerator::ExportProtoMessage(const TArray<UFunction*>& InCallbackFunctions, int32 Indent, class FStringOutputDevice* Output)
{
	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	// Parms struct definitions.
	FStringOutputDevice HeaderOutput;

	TArray<UFunction*> CallbackFunctions = InCallbackFunctions;
	CallbackFunctions.Sort();

	for (int32 Index = 0; Index < CallbackFunctions.Num(); Index++)
	{
		UFunction* Function = CallbackFunctions[Index];
		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);
		const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();
		if (FunctionData.FunctionExportFlags & FUNCEXPORT_NeedsProto)
		{
			if (WillExportEventParms(Function) && !Function->HasAnyFunctionFlags(FUNC_Delegate))
			{
				FString FunctionName = Function->GetName();
				TFieldIterator<UProperty> CommentIt(Function);
				FString ParameterList;
				for(; CommentIt && (CommentIt->PropertyFlags & CPF_Parm); ++CommentIt)
				{
					UProperty* Param = *CommentIt;
					FString TypeText, ExtendedTypeText;
					TypeText = Param->GetCPPType(&ExtendedTypeText, CPPF_None);
					FString ParamName = FString::Printf(TEXT("%s%s %s"), *TypeText, *ExtendedTypeText, *Param->GetName());

					// add this property to the parameter list string
					if (ParameterList.Len())
					{
						ParameterList += TCHAR(',');
					}

					ParameterList += ParamName;
				}

				HeaderOutput.Logf(TEXT("// %s%s(%s)\r\n"), FCString::Spc(Indent), *FunctionName, *ParameterList);

				TFieldIterator<UProperty> ParamIt(Function);
				ExportProtoDeclaration(HeaderOutput, FunctionName, ParamIt, CPF_Parm, Indent);
			}
		}
	}

	if (!Output)
	{
		GeneratedProtoText.Log(*HeaderOutput);
	}
	else
	{
		Output->Log(HeaderOutput);
	}
}

// Java uses different coding standards for capitalization
static FString FixJavaName(const FString &StringIn)
{
	FString FixedString = StringIn;

	FixedString[0] = FChar::ToLower(FixedString[0]); // java classes/variable start lower case
	FixedString.ReplaceInline(TEXT("ID"), TEXT("Id"), ESearchCase::CaseSensitive); // Id is standard instead of ID, some of our fnames use ID

	return FixedString;
}

/**
 * Export a single .java declaration, recursing as necessary for sub declarations
 *
 * @param Out output device
 * @param MessageName name of the message in the declaration
 * @param Properties array of parameters in the function definition
 * @param PropertyFlags flags to filter property array against
 * @param Ident starting indentation level
 */
void ExportMCPDeclaration(FOutputDevice& Out, const FString& MessageName, TFieldIterator<UProperty>& Properties, uint64 PropertyFlags, int32 Indent)
{
	Out.Logf(TEXT("%spublic class %sCommand extends ProfileCommand\r\n"), FCString::Spc(Indent), *MessageName);
	Out.Logf(TEXT("%s{\r\n"), FCString::Spc(Indent));

	static TMap<FString, FString> ToMCPTypeMappings;
	static TMap<FString, FString> ToAnnotationMappings;
	static bool bInitMapping = false;
	if (!bInitMapping)
	{
		// Explicit type mappings
		ToMCPTypeMappings.Add(TEXT("FString"), TEXT("String"));
		ToMCPTypeMappings.Add(TEXT("int32"), TEXT("int"));
		ToMCPTypeMappings.Add(TEXT("int64"), TEXT("int"));
		ToMCPTypeMappings.Add(TEXT("uint8"), TEXT("byte"));
		ToMCPTypeMappings.Add(TEXT("bool"), TEXT("boolean"));
		ToMCPTypeMappings.Add(TEXT("double"), TEXT("double"));
		ToMCPTypeMappings.Add(TEXT("float"), TEXT("float"));

		ToAnnotationMappings.Add(TEXT("FString"), TEXT("@NotBlankOrNull"));

		bInitMapping = true;
	}

	FString ConstructorParams;
	FString ConstructorText;

	for(; Properties && (Properties->PropertyFlags & PropertyFlags); ++Properties)
	{
		UProperty* Property = *Properties;
		UClass* PropClass = Property->GetClass();

		// Skip out and return paramaters
		if ((Property->PropertyFlags & CPF_RepSkip) || (Property->PropertyFlags & CPF_ReturnParm))
		{
			continue;
		}

		// export the property type text (e.g. FString; int32; TArray, etc.)
		FString TypeText, ExtendedTypeText;
		TypeText = Property->GetCPPType(&ExtendedTypeText, CPPF_None);
		if (PropClass != UInterfaceProperty::StaticClass() && PropClass != UObjectProperty::StaticClass())
		{
			// TODO Implement arrays
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			if (ArrayProperty != NULL)
			{
				// skip array generation for Java, this should result in a List<TYPE> declaration, but we can do this by hand for now.
				continue;
			}
			/*bool bIsRepeated = false;
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			if (ArrayProperty != NULL)
			{
				UClass* InnerPropClass = ArrayProperty->Inner->GetClass();
				if (InnerPropClass != UInterfaceProperty::StaticClass() && InnerPropClass != UObjectProperty::StaticClass())
				{
					FString InnerExtendedTypeText;
					FString InnerTypeText = ArrayProperty->Inner->GetCPPType(&InnerExtendedTypeText, CPPF_None);
					TypeText = InnerTypeText;
					ExtendedTypeText = InnerExtendedTypeText;
					Property = ArrayProperty->Inner;
					bIsRepeated = true;
				}
				else
				{
					FError::Throwf(TEXT("ExportMCPDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
				}
			}
			else if(Property->ArrayDim != 1)
			{
				bIsRepeated = true;
			}*/

			FString VariableTypeName = FString::Printf(TEXT("%s%s"), *TypeText, *ExtendedTypeText);
			FString PropertyName = FixJavaName(Property->GetNameCPP());
			FString* MCPTypeName = NULL;
			FString* AnnotationName = NULL;


			UStructProperty* StructProperty = Cast<UStructProperty>(Property);
			if (StructProperty != NULL)
			{
				// TODO Implement structs
/*				TFieldIterator<UProperty> StructIt(StructProperty->Struct);
				ExportMCPDeclaration(Out, VariableTypeName, StructIt, CPF_AllFlags, Indent + 4);
				VariableTypeName = FString::Printf(TEXT("CMsg%sMessage"), *VariableTypeName);
				MCPTypeName = &VariableTypeName;*/
			}
			else
			{
				MCPTypeName = ToMCPTypeMappings.Find(VariableTypeName);
				AnnotationName = ToAnnotationMappings.Find(VariableTypeName);
			}

			if (AnnotationName != NULL && !AnnotationName->IsEmpty())
			{
				Out.Log(FCString::Spc(Indent + 4));
				Out.Logf(TEXT("%s\r\n"), **AnnotationName);
			}

			if (MCPTypeName != NULL)
			{
				Out.Log(FCString::Spc(Indent + 4));
				Out.Logf(TEXT("private %s %s;\r\n"), **MCPTypeName, *PropertyName);

				ConstructorParams += FString::Printf(TEXT(", %s %s"), **MCPTypeName, *PropertyName);
				ConstructorText += FString::Printf(TEXT("%sthis.%s = %s;\r\n"), FCString::Spc(Indent + 8), *PropertyName, *PropertyName);
			}
			else
			{
				FError::Throwf(TEXT("ExportMCPDeclaration - Unhandled property mapping '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
			}
		}
		else
		{
			FError::Throwf(TEXT("ExportMCPDeclaration - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
		}
	}

	Out.Logf(TEXT("\r\n%spublic %sCommand(String epicId, String profileId%s)\r\n"), FCString::Spc(Indent + 4), *MessageName, *ConstructorParams);
	Out.Logf(TEXT("%s{\r\n"), FCString::Spc(Indent + 4));
	Out.Logf(TEXT("%ssuper(epicId, profileId);\r\n"), FCString::Spc(Indent + 8));
	Out.Logf(TEXT("%s"), *ConstructorText);
	Out.Logf(TEXT("%s}\r\n"), FCString::Spc(Indent + 4));

	Out.Logf(TEXT("\r\n%s@Override\r\n"), FCString::Spc(Indent + 4));
	Out.Logf(TEXT("%sprotected void execute(@Name(\"profile\") @NotNull ProfileEx profile)\r\n"), FCString::Spc(Indent + 4));
	Out.Logf(TEXT("%s{\r\n"), FCString::Spc(Indent + 4));
	Out.Logf(TEXT("%s}\r\n"), FCString::Spc(Indent + 4));

	Out.Logf( TEXT("%s}\r\n"), FCString::Spc(Indent) );
}

/**
 * Generate a .MCP message declaration for any functions marked as requiring one
 * 
 * @param InCallbackFunctions array of functions for consideration to generate .proto definitions
 * @param Indent starting indentation level
 * @param Output optional output redirect
 */
void FNativeClassHeaderGenerator::ExportMCPMessage(const TArray<UFunction*>& InCallbackFunctions, int32 Indent, class FStringOutputDevice* Output)
{
	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	// Parms struct definitions.
	FStringOutputDevice HeaderOutput;

	TArray<UFunction*> CallbackFunctions = InCallbackFunctions;
	CallbackFunctions.Sort();

	for (int32 Index = 0; Index < CallbackFunctions.Num(); Index++)
	{
		UFunction* Function = CallbackFunctions[Index];
		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);
		const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();
		if (FunctionData.FunctionExportFlags & FUNCEXPORT_NeedsMCP)
		{
			if (WillExportEventParms(Function) && !Function->HasAnyFunctionFlags(FUNC_Delegate))
			{
				FString FunctionName = Function->GetName();
				TFieldIterator<UProperty> CommentIt(Function);
				FString ParameterList;
				for(; CommentIt && (CommentIt->PropertyFlags & CPF_Parm); ++CommentIt)
				{
					UProperty* Param = *CommentIt;
					FString TypeText, ExtendedTypeText;
					TypeText = Param->GetCPPType(&ExtendedTypeText, CPPF_None);
					FString ParamName = FString::Printf(TEXT("%s%s %s"), *TypeText, *ExtendedTypeText, *Param->GetName());

					// add this property to the parameter list string
					if (ParameterList.Len())
					{
						ParameterList += TCHAR(',');
					}

					ParameterList += ParamName;
				}

				HeaderOutput.Logf(TEXT("// %s%s(%s)\r\n"), FCString::Spc(Indent), *FunctionName, *ParameterList);

				TFieldIterator<UProperty> ParamIt(Function);
				ExportMCPDeclaration(HeaderOutput, FunctionName, ParamIt, CPF_Parm, Indent);
			}
		}
	}

	if (!Output)
	{
		GeneratedMCPText.Log(*HeaderOutput);
	}
	else
	{
		Output->Log(HeaderOutput);
	}
}

/**
* Exports the parameter struct declarations for the list of functions specified
*
* @param	CallbackFunctions	the functions that have parameters which need to be exported
*/
void FNativeClassHeaderGenerator::ExportEventParms( const TArray<UFunction*>& InCallbackFunctions, const TCHAR* MacroSuffix, int32 Indent, bool bOutputConstructor, class FStringOutputDevice* Output )
{
	// Parms struct definitions.
	FStringOutputDevice HeaderOutput;

	TArray<UFunction*> CallbackFunctions = InCallbackFunctions;
	CallbackFunctions.Sort();

	for ( int32 Index = 0; Index < CallbackFunctions.Num(); Index++ )
	{
		UFunction* Function = CallbackFunctions[Index];
		if (!WillExportEventParms(Function))
		{
			continue;
		}

		FString FunctionName = Function->GetName();
		if( Function->HasAnyFunctionFlags( FUNC_Delegate ) )
		{
			FunctionName = FunctionName.LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
		}

		FString EventParmStructName = GetEventStructParamsName(*Function->GetOwnerClass()->GetName(), *FunctionName);
		HeaderOutput.Logf( TEXT("%sstruct %s\r\n"), FCString::Spc(Indent), *EventParmStructName);
		HeaderOutput.Logf( TEXT("%s{\r\n"), FCString::Spc(Indent) );

		for( TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags&CPF_Parm); ++It )
		{
			UProperty* Prop = *It;

			FStringOutputDevice PropertyText;
			PropertyText.Log( FCString::Spc(4 + Indent) );

			bool bEmitConst = Prop->HasAnyPropertyFlags(CPF_ConstParm) && Prop->IsA<UObjectProperty>();

			//@TODO: UCREMOVAL: This is awful code duplication to avoid a double-const
			{
				// export 'const' for parameters
				const bool bIsConstParam = (Prop->IsA(UInterfaceProperty::StaticClass()) && !Prop->HasAllPropertyFlags(CPF_OutParm)); //@TODO: This should be const once that flag exists
				const bool bIsOnConstClass = (Prop->IsA(UObjectProperty::StaticClass()) && ((UObjectProperty*)Prop)->PropertyClass != NULL && ((UObjectProperty*)Prop)->PropertyClass->HasAnyClassFlags(CLASS_Const));

				if (bIsConstParam || bIsOnConstClass)
				{
					bEmitConst = false; // ExportCppDeclaration will do it for us
				}
			}

			if (bEmitConst)
			{
				PropertyText.Logf(TEXT("const "));
			}

			const FString* Dim = GArrayDimensions.Find(Prop);
			Prop->ExportCppDeclaration(PropertyText, EExportedDeclaration::Local, Dim ? **Dim : NULL);
			ApplyAlternatePropertyExportText(Prop, PropertyText);
			
			PropertyText.Log( TEXT(";\r\n") );
			HeaderOutput += *PropertyText;

		}
		// constructor must initialize the return property if it needs it
		UProperty* Prop = Function->GetReturnProperty();
		if (Prop && bOutputConstructor)
		{
			FStringOutputDevice InitializationAr;

			UStructProperty* InnerStruct = Cast<UStructProperty>(Prop);
			bool bNeedsOutput = true;
			if (InnerStruct)
			{
				bNeedsOutput = InnerStruct->HasNoOpConstructor();
			}
			else if (
				Cast<UNameProperty>(Prop) ||
				Cast<UDelegateProperty>(Prop) ||
				Cast<UMulticastDelegateProperty>(Prop) ||
				Cast<UStrProperty>(Prop) ||
				Cast<UTextProperty>(Prop) ||
				Cast<UArrayProperty>(Prop) ||
				Cast<UInterfaceProperty>(Prop)
				)
			{
				bNeedsOutput = false;
			}
			if (bNeedsOutput)
			{
				check(Prop->ArrayDim == 1); // can't return arrays
				HeaderOutput.Logf(TEXT("\r\n%s/** Constructor, intializes return property only **/\r\n"), FCString::Spc(4 + Indent));
				HeaderOutput.Logf(TEXT("%s%s()\r\n"), FCString::Spc(4 + Indent), *EventParmStructName);
				HeaderOutput.Logf(TEXT("%s%s %s(%s)\r\n"), FCString::Spc(8 + Indent),TEXT(":") , *Prop->GetName(), *GetNullParameterValue(Prop,false,true));
				HeaderOutput.Logf(TEXT("%s{\r\n"), FCString::Spc(4 + Indent));
				HeaderOutput.Logf(TEXT("%s}\r\n"), FCString::Spc(4 + Indent));
			}
		}
		HeaderOutput.Logf( TEXT("%s};\r\n"), FCString::Spc(Indent) );
	}

	if (!Output)
	{
		const TCHAR* Suffix = TEXT("_EVENTPARMS");
		FString MacroName = FString(NameLookupCPP->GetNameCPP(CurrentClass)) + Suffix + MacroSuffix;
		FString Macroized = Macroize(*MacroName, *HeaderOutput);
		GeneratedHeaderText.Log(*Macroized);
		PrologMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(Indent),  *MacroName);
	}
	else
	{
		Output->Log(HeaderOutput);
	}
}

/**
 * Get the intrinsic null value for this property
 * 
 * @param	Prop				the property to get the null value for
 * @param	bMacroContext		true when exporting the P_GET* macro, false when exporting the friendly C++ function header
 *
 * @return	the intrinsic null value for the property (0 for ints, TEXT("") for strings, etc.)
 */
FString FNativeClassHeaderGenerator::GetNullParameterValue( UProperty* Prop, bool bMacroContext, bool bInitializer/*=false*/ )
{
	UClass* PropClass = Prop->GetClass();
	UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(Prop);
	if (PropClass == UByteProperty::StaticClass()
	||	PropClass == UIntProperty::StaticClass()
	||	PropClass == UBoolProperty::StaticClass()
	||	PropClass == UFloatProperty::StaticClass()
	||	PropClass == UDoubleProperty::StaticClass())
	{
		// if we have a BoolProperty then set it to be false instead of 0
 		if( PropClass == UBoolProperty::StaticClass() )
 		{
 			return TEXT("false");
 		}

		return TEXT("0");
	}
	else if ( PropClass == UNameProperty::StaticClass() )
	{
		return TEXT("NAME_None");
	}
	else if ( PropClass == UStrProperty::StaticClass() )
	{
		return TEXT("TEXT(\"\")");
	}
	else if ( PropClass == UTextProperty::StaticClass() )
	{
		return TEXT("FText::GetEmpty()");
	}
	else if ( PropClass == UArrayProperty::StaticClass()
		||    PropClass == UDelegateProperty::StaticClass()
		||    PropClass == UMulticastDelegateProperty::StaticClass() )
	{
		FString Type, ExtendedType;
		Type = Prop->GetCPPType(&ExtendedType,CPPF_OptionalValue);
		return Type + ExtendedType + TEXT("()");
	}
	else if ( PropClass == UStructProperty::StaticClass() )
	{
		bool bHasNoOpConstuctor = CastChecked<UStructProperty>(Prop)->HasNoOpConstructor();
		if (bInitializer && bHasNoOpConstuctor)
		{
			return TEXT("ForceInit");
		}

		FString Type, ExtendedType;
		Type = Prop->GetCPPType(&ExtendedType,CPPF_OptionalValue);
		return Type + ExtendedType + (bHasNoOpConstuctor ? TEXT("(ForceInit)") : TEXT("()"));
	}
	else if (ObjectProperty)
	{
		return TEXT("NULL");
	}
	else if ( PropClass == UInterfaceProperty::StaticClass() )
	{
		return TEXT("NULL");
	}

	UE_LOG(LogCompile, Fatal,TEXT("GetNullParameterValue - Unhandled property type '%s': %s"), *PropClass->GetName(), *Prop->GetPathName());
	return TEXT("");
}

void FNativeClassHeaderGenerator::ExportNativeFunctionHeader( const FFuncInfo& FunctionData, FStringOutputDevice& HeaderOutput, EExportFunctionType::Type FunctionType, EExportFunctionHeaderStyle::Type FunctionHeaderStyle, const TCHAR* ExtraParam )
{
	UFunction* Function = FunctionData.FunctionReference;

	const bool bIsDelegate   = Function->HasAnyFunctionFlags( FUNC_Delegate );
	const bool bIsInterface  = !bIsDelegate && Function->GetOwnerClass()->HasAnyClassFlags(CLASS_Interface);
	const bool bIsK2Override = Function->HasAnyFunctionFlags( FUNC_BlueprintEvent );
	
	HeaderOutput.Log( FCString::Spc( bIsDelegate ? 0 : 4) );

	if (FunctionHeaderStyle == EExportFunctionHeaderStyle::Declaration)
	{
		// cpp implementation of functions never have these appendages

		// If the function was marked as 'RequiredAPI', then add the *_API macro prefix.  Note that if the class itself
		// was marked 'RequiredAPI', this is not needed as C++ will exports all methods automatically.
		if( !Function->GetOwnerClass()->HasAnyClassFlags( CLASS_RequiredAPI ) &&
			( FunctionData.FunctionExportFlags & FUNCEXPORT_RequiredAPI ) )
		{
			HeaderOutput.Log( FString::Printf( TEXT( "%s_API " ), *API ) );
		}

		if(FunctionType == EExportFunctionType::Interface)
		{
			HeaderOutput.Log(TEXT("static "));
		}
		else if (bIsK2Override)
		{
			HeaderOutput.Log(TEXT("virtual "));
		}
		// if the owning class is an interface class
		else if ( bIsInterface )
		{
			HeaderOutput.Log(TEXT("virtual "));
		}
		// this is not an event, the function is not a static function and the function is not marked final
		else if ( FunctionType != EExportFunctionType::Event && !Function->HasAnyFunctionFlags(FUNC_Static) && !(FunctionData.FunctionExportFlags & FUNCEXPORT_Final) )
		{
			HeaderOutput.Log(TEXT("virtual "));
		}
		else if( FunctionData.FunctionExportFlags & FUNCEXPORT_Inline )
		{
			HeaderOutput.Log(TEXT("inline "));
		}
	}

	if (auto Return = Function->GetReturnProperty())
	{
		FString ExtendedReturnType;
		FString ReturnType = Return->GetCPPType(&ExtendedReturnType, (FunctionHeaderStyle == EExportFunctionHeaderStyle::Definition && (FunctionType != EExportFunctionType::Interface) ? CPPF_Implementation : 0) | CPPF_ArgumentOrReturnValue);
		FStringOutputDevice ReplacementText(*ReturnType);
		ApplyAlternatePropertyExportText(Return, ReplacementText);
		HeaderOutput.Logf(TEXT("%s%s"), *ReplacementText, *ExtendedReturnType);
	}
	else
	{
		HeaderOutput.Log( TEXT("void") );
	}

	FString FunctionName;
	if (FunctionHeaderStyle == EExportFunctionHeaderStyle::Definition)
	{
		FunctionName = FString(NameLookupCPP->GetNameCPP(CastChecked<UClass>(Function->GetOuter()), bIsInterface || FunctionType == EExportFunctionType::Interface)) + TEXT("::");
	}

	if (FunctionType == EExportFunctionType::Interface)
	{
		FunctionName += FString::Printf(TEXT("Execute_%s"), *Function->GetName());
	}
	else if (FunctionType == EExportFunctionType::Event)
	{
		FunctionName += FunctionData.MarshallAndCallName;
	}
	else
	{
		FunctionName += FunctionData.CppImplName;
	}

	HeaderOutput.Logf( TEXT(" %s("), *FunctionName);

	int32 ParmCount=0;

	// Emit extra parameter if we have one
	if( ExtraParam )
	{
		HeaderOutput += ExtraParam;
		++ParmCount;
	}

	for( TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
	{
		UProperty* Property = *It;

		if( ParmCount++ )
		{
			HeaderOutput.Log(TEXT(", "));
		}

		FStringOutputDevice PropertyText;

		const FString* Dim = GArrayDimensions.Find(Property);
		Property->ExportCppDeclaration( PropertyText, EExportedDeclaration::Parameter, Dim ? **Dim : NULL );
		ApplyAlternatePropertyExportText(Property, PropertyText);

		HeaderOutput += PropertyText;
	}

	HeaderOutput.Log( TEXT(")") );
	if (FunctionType != EExportFunctionType::Interface)
	{
		if (!bIsDelegate && Function->HasAllFunctionFlags(FUNC_Const))
		{
			HeaderOutput.Log( TEXT(" const") );
		}

		if (bIsInterface && FunctionHeaderStyle == EExportFunctionHeaderStyle::Declaration)
		{
			// all methods in interface classes are pure virtuals
			HeaderOutput.Log(TEXT("=0"));
		}
	}
}

/**
 * Export the actual internals to a standard thunk function
 *
 * @param RPCWrappers output device for writing
 * @param FunctionData function data for the current function
 * @param Parameters list of parameters in the function
 * @param Return return parameter for the function
 */
void FNativeClassHeaderGenerator::ExportFunctionThunk(FStringOutputDevice& RPCWrappers, const FFuncInfo& FunctionData, const TArray<UProperty*>& Parameters, UProperty* Return)
{
	// export the GET macro for this parameter
	FString ParameterList;
	for ( int32 ParameterIndex = 0; ParameterIndex < Parameters.Num(); ParameterIndex++ )
	{
		UProperty* Param = Parameters[ParameterIndex];

		FString EvalBaseText = TEXT("P_GET_");	// e.g. P_GET_STR
		FString EvalModifierText;				// e.g. _REF
		FString EvalParameterText;				// e.g. (UObject*,NULL)

		FString TypeText;
		if ( Param->ArrayDim > 1 )
		{
			EvalBaseText += TEXT("ARRAY");
			TypeText = Param->GetCPPType();
		}
		else
		{
			EvalBaseText += Param->GetCPPMacroType(TypeText);
		}
		FStringOutputDevice ReplacementText(*TypeText);
		ApplyAlternatePropertyExportText(Param, ReplacementText);
		TypeText = ReplacementText;

		FString DefaultValueText;
		FString ParamPrefix;

		// if this property is an out parm, add the REF tag
		if (Param->PropertyFlags & CPF_OutParm)
		{
			EvalModifierText += TEXT("_REF");
			ParamPrefix = TEXT("Out_");
		}

		// if this property requires a specialization, add a comma to the type name so we can print it out easily
		if ( TypeText != TEXT("") )
		{
			TypeText += TCHAR(',');
		}

		FString ParamName = ParamPrefix + Param->GetName();

		EvalParameterText = FString::Printf(TEXT("(%s%s%s)"), *TypeText, *ParamName, *DefaultValueText);

		RPCWrappers.Logf(TEXT("%s%s%s%s;%s"), FCString::Spc(8), *EvalBaseText, *EvalModifierText, *EvalParameterText, LINE_TERMINATOR);

		// add this property to the parameter list string
		if ( ParameterList.Len() )
		{
			ParameterList += TCHAR(',');
		}

		{
			UDelegateProperty* DelegateProp = Cast< UDelegateProperty >( Param );
			if( DelegateProp != NULL )
			{
				// For delegates, add an explicit conversion to the specific type of delegate before passing it along
				const FString FunctionName = DelegateProp->SignatureFunction->GetName().LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
				const FString CPPDelegateName = FString( TEXT("F" ) ) + FunctionName;
				ParamName = FString::Printf( TEXT( "%s(%s)" ), *CPPDelegateName, *ParamName );
			}
		}

		{
			UMulticastDelegateProperty* MulticastDelegateProp = Cast< UMulticastDelegateProperty >( Param );
			if( MulticastDelegateProp != NULL )
			{
				// For delegates, add an explicit conversion to the specific type of delegate before passing it along
				const FString FunctionName = MulticastDelegateProp->SignatureFunction->GetName().LeftChop( FString( HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX ).Len() );
				const FString CPPDelegateName = FString( TEXT("F" ) ) + FunctionName;
				ParamName = FString::Printf( TEXT( "%s(%s)" ), *CPPDelegateName, *ParamName );
			}
		}

		UByteProperty* ByteProp = Cast< UByteProperty >( Param );
		if ((ByteProp != NULL) && (ByteProp->Enum != NULL))
		{
			UEnum* Enum = ByteProp->Enum;

			FString FullyQualifiedEnumName = Enum->GetName();
			if (!Enum->ActualEnumNameInsideNamespace.IsEmpty())
			{
				FullyQualifiedEnumName += TEXT("::");
				FullyQualifiedEnumName += Enum->ActualEnumNameInsideNamespace;
			}

			// For enums, add an explicit conversion 
			if (!(ByteProp->PropertyFlags & CPF_OutParm))
			{
				ParamName = FString::Printf( TEXT( "%s(%s)" ), *FullyQualifiedEnumName, *ParamName );
			}
			else
			{
				ParamName = FString::Printf( TEXT( "(TEnumAsByte<%s>&)(%s)" ), *FullyQualifiedEnumName, *ParamName );
			}
		}

		ParameterList += ParamName;
	}

	RPCWrappers.Logf(TEXT("%sP_FINISH;%s"), FCString::Spc(8), LINE_TERMINATOR);

	// Call the validate function if there is one
	if ( !( FunctionData.FunctionExportFlags & FUNCEXPORT_CppStatic ) && ( FunctionData.FunctionFlags & FUNC_NetValidate ) )
	{
		// Call the validate function, and if it fails, return (which will NOT execute the _Implementation function below)
		// NOTE - We don't need to set "Result", since RPC calls don't return any values...
		RPCWrappers.Logf(TEXT("%sif (!this->%s(%s))%s"), FCString::Spc(8), *FunctionData.CppValidationImplName, *ParameterList, LINE_TERMINATOR);
		RPCWrappers.Logf(TEXT("%s{%s"), FCString::Spc(8), LINE_TERMINATOR);
		RPCWrappers.Logf(TEXT("%sRPC_ValidateFailed(TEXT(\"%s\"));%s"), FCString::Spc(8+4), *FunctionData.CppValidationImplName, LINE_TERMINATOR);
		RPCWrappers.Logf(TEXT("%sreturn;%s"), FCString::Spc(8+4), LINE_TERMINATOR);	// If we got here, the validation function check failed
		RPCWrappers.Logf(TEXT("%s}%s"), FCString::Spc(8), LINE_TERMINATOR);
	}

	// write out the return value
	RPCWrappers.Log(FCString::Spc(8));
	if ( Return != NULL )
	{
		FString ReturnType, ReturnExtendedType;
		ReturnType = Return->GetCPPType(&ReturnExtendedType);
		FStringOutputDevice ReplacementText(*ReturnType);
		ApplyAlternatePropertyExportText(Return, ReplacementText);
		ReturnType = ReplacementText;
		RPCWrappers.Logf(TEXT("*(%s%s*)Result="), *ReturnType, *ReturnExtendedType);
	}

	// export the call to the C++ version
	if (FunctionData.FunctionExportFlags & FUNCEXPORT_CppStatic)
	{
		RPCWrappers.Logf(TEXT("%s::%s(%s);%s"), NameLookupCPP->GetNameCPP(CurrentClass), *FunctionData.CppImplName, *ParameterList, LINE_TERMINATOR);
	}
	else
	{
		RPCWrappers.Logf(TEXT("this->%s(%s);%s"), *FunctionData.CppImplName, *ParameterList, LINE_TERMINATOR);
	}
}

void FNativeClassHeaderGenerator::ExportNativeFunctions(const TArray<UFunction*>& NativeFunctions)
{
	FStringOutputDevice RPCWrappers;

	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	//@todo: UCREMOVAL this multipass stuff is only used for diffs, remove it
	TArray<UFunction*> SortedNativeFunctions = NativeFunctions;
	SortedNativeFunctions.Sort();

	// export the C++ stubs
	for (int32 Pass = 0; Pass < 2; Pass++)
	{
		for ( int32 i = NativeFunctions.Num() - 1; i >= 0; i-- )
		{
			UFunction*     Function     = Pass ? SortedNativeFunctions[i] : NativeFunctions[i];
			FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
			check(CompilerInfo);

			const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();

			// Custom thunks don't get any C++ stub function generated
			if (FunctionData.FunctionExportFlags & FUNCEXPORT_CustomThunk)
				continue;

			// All functions that are declared in class headers are by definition native
			check(Function->FunctionFlags & FUNC_Native);

			// Should we emit these to RPC wrappers or just ignore them?
			const bool bWillBeProgrammerTyped = FunctionData.CppImplName == Function->GetName();

			FStringOutputDevice WrongPass;
			FStringOutputDevice& DestinationForDecl = (!bWillBeProgrammerTyped && Pass) ? RPCWrappers : WrongPass;

			// Declare validation function if needed
			if ( FunctionData.FunctionFlags & FUNC_NetValidate ) 
			{
				FString ParameterList;

				for ( TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
				{
					UProperty* Property = *It;

					if ( ParameterList.Len() )
					{
						ParameterList += TEXT(", ");
					}

					FStringOutputDevice PropertyText;

					const FString* Dim = GArrayDimensions.Find(Property);
					Property->ExportCppDeclaration(PropertyText, EExportedDeclaration::Parameter, Dim ? **Dim : NULL);
					ApplyAlternatePropertyExportText(Property, PropertyText);

					ParameterList += PropertyText;
				}
				DestinationForDecl.Logf( TEXT("%sbool %s(%s);\r\n"), FCString::Spc( 4 ), *FunctionData.CppValidationImplName, *ParameterList );
			}

			ExportNativeFunctionHeader(FunctionData, DestinationForDecl, EExportFunctionType::Function, EExportFunctionHeaderStyle::Declaration);

			DestinationForDecl.Log(TEXT(";"));
			if (bMultiLineUFUNCTION)
			{
				DestinationForDecl.Log(TEXT("\r\n"));
			}
			DestinationForDecl.Log(TEXT("\r\n"));

			// if this function was originally declared in a base class, and it isn't a static function,
			// only the C++ function header will be exported
			if (!Pass || !ShouldExportFunction(Function))
			{
				continue;
			}

			// export the script wrappers
			RPCWrappers.Logf(TEXT("%sDECLARE_FUNCTION(%s)"), FCString::Spc(4), *FunctionData.UnMarshallAndCallName);
			RPCWrappers.Logf(TEXT("%s%s{%s"), LINE_TERMINATOR, FCString::Spc(4), LINE_TERMINATOR);

			auto Parameters = GetFunctionParmsAndReturn(FunctionData.FunctionReference);
			ExportFunctionThunk(RPCWrappers, FunctionData, Parameters.Parms, Parameters.Return);

			RPCWrappers.Logf(TEXT("%s}%s"), FCString::Spc(4), LINE_TERMINATOR);
		}
	}

	const TCHAR* Suffix = TEXT("_RPC_WRAPPERS");
	FString MacroName = FString(NameLookupCPP->GetNameCPP(CurrentClass)) + Suffix;
	FString Macroized = Macroize(*MacroName, *RPCWrappers);
	GeneratedHeaderText.Log(*Macroized);
	InClassMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(4),  *MacroName);
}

/**
 * Exports the methods which trigger UnrealScript events and delegates.
 *
 * @param	CallbackFunctions	the functions to export
 */
void FNativeClassHeaderGenerator::ExportCallbackFunctions( const TArray<UFunction*>& InCallbackFunctions )
{
	FStringOutputDevice RPCWrappers;
	FStringOutputDevice RPCWrappersCPP;
	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);

	// Skip interfaces; they don't have any callbacks
	if (CurrentClass->HasAnyClassFlags(CLASS_Interface))
	{
		return;
	}

	TArray<UFunction*> CallbackFunctions = InCallbackFunctions;
	CallbackFunctions.Sort();

	for ( int32 CallbackIndex = 0; CallbackIndex < CallbackFunctions.Num(); CallbackIndex++ )
	{
		UFunction* Function = CallbackFunctions[ CallbackIndex ];

		// Never expecting to export delegate functions this way
		check( !Function->HasAnyFunctionFlags( FUNC_Delegate ) );

		UClass* Class = CurrentClass;

		FFunctionData* CompilerInfo = ClassData->FindFunctionData(Function);
		check(CompilerInfo);

		// cache the TCHAR* for a few strings we'll use a lot here
		FString FunctionName = Function->GetName();

		const FFuncInfo& FunctionData = CompilerInfo->GetFunctionData();

		if (FunctionData.FunctionFlags & FUNC_NetResponse)
		{
			// Net response functions don't go into the VM
			continue;
		}

		FString ClassName = Class->GetName();

		const bool bWillBeProgrammerTyped = Function->GetName() == FunctionData.MarshallAndCallName;

		// Emit the declaration if the programmer isn't responsible for declaring this wrapper
		if (!bWillBeProgrammerTyped)
		{
			// export the line that looks like: int32 Main(const FString& Parms)
			ExportNativeFunctionHeader(FunctionData, RPCWrappers, EExportFunctionType::Event, EExportFunctionHeaderStyle::Declaration);

			RPCWrappers.Log(TEXT(";\r\n"));

			if (bMultiLineUFUNCTION)
			{
				RPCWrappers.Log(TEXT("\r\n"));
			}
		}

		// Emit the thunk implementation
		ExportNativeFunctionHeader(FunctionData, RPCWrappersCPP, EExportFunctionType::Event, EExportFunctionHeaderStyle::Definition);

		auto Parameters = GetFunctionParmsAndReturn(FunctionData.FunctionReference);

		WriteEventFunctionPrologue(RPCWrappersCPP, 4, Parameters, *ClassName, *FunctionName);
		{
			// Cast away const just in case, because ProcessEvent isn't const
			RPCWrappersCPP.Logf(
				TEXT("%s%sProcessEvent(FindFunctionChecked(%s_%s),%s);\r\n"),
				FCString::Spc(8),
				(Function->HasAllFunctionFlags(FUNC_Const)) ? *FString::Printf(TEXT("const_cast<%s*>(this)->"), NameLookupCPP->GetNameCPP(Class)) : TEXT(""),
				*API,
				*FunctionName,
				Parameters.HasParms() ? TEXT("&Parms") : TEXT("NULL")
			);
		}
		WriteEventFunctionEpilogue(RPCWrappersCPP, 4, Parameters, *ClassName, *FunctionName);
	}

	if (!CurrentClass->HasAnyClassFlags(CLASS_NoExport|CLASS_Temporary))
	{
		GeneratedPackageCPP.Log(*RPCWrappersCPP);
	}
	// else drop the implementation on the floor

	const TCHAR* Suffix = TEXT("_CALLBACK_WRAPPERS");
	FString MacroName = FString(NameLookupCPP->GetNameCPP(CurrentClass)) + Suffix;
	FString Macroized = Macroize(*MacroName, *RPCWrappers);
	GeneratedHeaderText.Log(*Macroized);
	InClassMacroCalls.Logf( TEXT("%s%s\r\n"), FCString::Spc(4),  *MacroName);
}


/**
 * Determines if the property has alternate export text associated with it and if so replaces the text in PropertyText with the
 * alternate version. (for example, structs or properties that specify a native type using export-text).  Should be called immediately
 * after ExportCppDeclaration()
 *
 * @param	Prop			the property that is being exported
 * @param	PropertyText	the string containing the text exported from ExportCppDeclaration
 */
void FNativeClassHeaderGenerator::ApplyAlternatePropertyExportText( UProperty* Prop, FStringOutputDevice& PropertyText )
{
	if (bIsExportingForOffsetDeterminationOnly)
	{
		UDelegateProperty* DelegateProperty = Cast<UDelegateProperty>(Prop);
		UMulticastDelegateProperty* MulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Prop);
		if (DelegateProperty || MulticastDelegateProperty)
		{
			FString Original = Prop->GetCPPType();
			FString PlaceholderOfSameSizeAndAlignemnt;
			if (DelegateProperty)
			{
				PlaceholderOfSameSizeAndAlignemnt = TEXT("FScriptDelegate");
			}
			else
			{
				PlaceholderOfSameSizeAndAlignemnt = TEXT("FMulticastScriptDelegate");
			}
			PropertyText.ReplaceInline(*Original, *PlaceholderOfSameSizeAndAlignemnt, ESearchCase::CaseSensitive);
		}
	}
	// find the class info for this class
	FClassMetaData* ClassData = GScriptHelper->FindClassData(CurrentClass);
	// find the compiler token for this property
	FTokenData* PropData = ClassData->FindTokenData(Prop);

	UObjectProperty* ObjectProp = Cast<UObjectProperty>(Prop);
	if ( ObjectProp == NULL )
	{
		UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
		if ( ArrayProp != NULL )
		{
			ObjectProp = Cast<UObjectProperty>(ArrayProp->Inner);
		}
	}
	if ( ObjectProp != NULL && PropData && PropData->Token.ExportInfo.Len() > 0)
	{
		PropertyText.ReplaceInline(*(FString("class ") + NameLookupCPP->GetNameCPP(ObjectProp->PropertyClass) + TEXT("*")), *PropData->Token.ExportInfo);
		return;
	}

	UStructProperty* StructProp = Cast<UStructProperty>(Prop);
	if ( StructProp == NULL )
	{
		UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop);
		if ( ArrayProp != NULL )
		{
			StructProp = Cast<UStructProperty>(ArrayProp->Inner);
		}
	}

	if ( StructProp )
	{
		if ( PropData != NULL )
		{
			FString ExportText;
			if ( PropData->Token.ExportInfo.Len() > 0 )
			{
				ExportText = PropData->Token.ExportInfo;
			}
			else
			{
				// we didn't have any export-text associated with the variable, so check if we have
				// anything for the struct itself
				FStructData* StructData = ClassData->FindStructData(StructProp->Struct);
				if ( StructData != NULL && StructData->StructData.ExportInfo.Len() )
				{
					ExportText = StructData->StructData.ExportInfo;
				}
			}

			if ( ExportText.Len() > 0 )
			{
				(FString&)PropertyText = PropertyText.Replace(NameLookupCPP->GetNameCPP(StructProp->Struct), *ExportText);
			}
		}
	}
}

/**
 * Sorts the list of header files being exported from a package according to their dependency on each other.
 *
 * @param	HeaderDependencyMap		a mapping of header filenames to a list of header filenames that must be processed before that one.
 * @param	SortedHeaderFilenames	[out] receives the sorted list of header filenames.
 */
bool FNativeClassHeaderGenerator::SortHeaderDependencyMap(const TMap<const FString*, HeaderDependents>& HeaderDependencyMap, TArray<const FString*>& SortedHeaderFilenames) const
{
	SortedHeaderFilenames.Empty(HeaderDependencyMap.Num());

	while (SortedHeaderFilenames.Num() < HeaderDependencyMap.Num())
	{
		bool bAddedSomething = false;

		// Find headers with no dependencies and add those to the list.
		for (auto It = HeaderDependencyMap.CreateConstIterator(); It; ++It)
		{
			auto Header = It->Key;
			if (SortedHeaderFilenames.Contains(Header))
				continue;

			bool bHasRemainingDependencies = false;
			for (auto It2 = It->Value.CreateConstIterator(); It2; ++It2)
			{
				if (!SortedHeaderFilenames.Contains(*It2))
				{
					bHasRemainingDependencies = true;
					break;
				}
			}

			if (!bHasRemainingDependencies)
			{
				// Add it to the list.
				SortedHeaderFilenames.AddUnique(Header);
				bAddedSomething           = true;
			}
		}

		// Circular dependency error?
		if (!bAddedSomething)
			return false;
	}

	return true;
}

bool FNativeClassHeaderGenerator::FindInterDependency( TMap<const FString*, HeaderDependents>& HeaderDependencyMap, const FString* Header, const FString*& OutHeader1, const FString*& OutHeader2 )
{
	TSet<const FString*> VisitedHeaders;
	return FindInterDependencyRecursive( HeaderDependencyMap, Header, VisitedHeaders, OutHeader1, OutHeader2 );
}

/**
 * Finds to headers that are dependent on each other.
 *
 * @param	HeaderDependencyMap	A map of headers and their dependencies. Each header is represented as an index into a TArray of the actual filename strings.
 * @param	HeaderIndex			A header to scan for any inter-dependency.
 * @param	VisitedHeaders		Must be filled with false values before the first call (must be large enough to be indexed by all headers).
 * @param	OutHeader1			[out] Receives the first inter-dependent header index.
 * @param	OutHeader2			[out] Receives the second inter-dependent header index.
 * @return	true if an inter-dependency was found.
 */
bool FNativeClassHeaderGenerator::FindInterDependencyRecursive( TMap<const FString*, HeaderDependents>& HeaderDependencyMap, const FString* Header, TSet<const FString*>& VisitedHeaders, const FString*& OutHeader1, const FString*& OutHeader2 )
{
	VisitedHeaders.Add(Header);
	for (auto It = HeaderDependencyMap[Header].CreateConstIterator(); It; ++It)
	{
		auto DependentHeader = *It;
		if (VisitedHeaders.Contains(DependentHeader))
		{
			OutHeader1 = Header;
			OutHeader2 = DependentHeader;
			return true;
		}

		if ( FindInterDependencyRecursive( HeaderDependencyMap, DependentHeader, VisitedHeaders, OutHeader1, OutHeader2 ) )
		{
			return true;
		}
	}
	return false;
}

/**
 * Finds a dependency chain between two class header files.
 * Wrapper around FindDependencyChainRecursive().
 *
 * @param	Class				A class to scan for a dependency chain between the two headers.
 * @param	Header1				First class header filename.
 * @param	Header2				Second class header filename.
 * @param	DependencyChain		[out] Receives dependency chain, if found.
 * @return	true if a dependency chain was found and filled in.
 */
bool FNativeClassHeaderGenerator::FindDependencyChain( const UClass* Class, const FString& Header1, const FString& Header2, TArray<const UClass*>& DependencyChain )
{
	DependencyChain.Empty();
	return FindDependencyChainRecursive( Class, Header1, Header2, false, DependencyChain );
}

/**
 * Finds a dependency chain between two class header files.
 *
 * @param	Class				A class to scan for a dependency chain between the two headers.
 * @param	Header1				First class header filename.
 * @param	Header2				Second class header filename.
 * @param	bChainStarted		Whether Header1 has been found and we've started to fill in DependencyChain. Must be false to begin with.
 * @param	DependencyChain		[out] Receives dependency chain, if found. Must be empty before the call.
 * @return	true if a dependency chain was found and filled in.
 */
bool FNativeClassHeaderGenerator::FindDependencyChainRecursive( const UClass* Class, const FString& Header1, const FString& Header2, bool bChainStarted, TArray<const UClass*>& DependencyChain )
{
	bool bIsExportClass = Class->HasAnyClassFlags(CLASS_Native) && !Class->HasAnyClassFlags(CLASS_NoExport|CLASS_Intrinsic|CLASS_Temporary);
	if ( bIsExportClass )
	{
		auto HeaderFilename = GClassHeaderFilenameMap.FindRef(Class);

		bChainStarted = bChainStarted || HeaderFilename == Header1;

		if ( bChainStarted )
		{
			DependencyChain.Add( Class );
			if ( HeaderFilename == Header2 )
			{
				return true;
			}
		}
	}

	UClass* SuperClass = Class->GetSuperClass();
	if ( SuperClass && SuperClass->GetOuter() == Class->GetOuter() && FindDependencyChainRecursive( SuperClass, Header1, Header2, bChainStarted, DependencyChain ) )
	{
		return true;
	}

	for (TArray<FImplementedInterface>::TConstIterator It(Class->Interfaces); It; ++It)
	{
		UClass* InterfaceClass = It->Class;
		if ( InterfaceClass->GetOuter() == Class->GetOuter() && FindDependencyChainRecursive( InterfaceClass, Header1, Header2, bChainStarted, DependencyChain ) )
		{
			return true;
		}
	}

	if ( bIsExportClass && bChainStarted )
	{
		DependencyChain.RemoveAtSwap( DependencyChain.Num() - 1 );
	}
	return false;
}

// Constructor.
FNativeClassHeaderGenerator::FNativeClassHeaderGenerator( UPackage* InPackage, FClassTree& inClassTree, bool InAllowSaveExportedHeaders, bool bInUseRelativePaths )
	: CurrentClass                          (NULL)
	, API                                   (FPackageName::GetShortName(InPackage).ToUpper())
	, Package                               (InPackage)
	, ClassTree                             (inClassTree)
	, bIsExportingForOffsetDeterminationOnly(false)
	, bAllowSaveExportedHeaders             (InAllowSaveExportedHeaders)
	, bFailIfGeneratedCodeChanges           (false)
	, bUseRelativePaths                     (bInUseRelativePaths)
{
	const FString PackageName = FPackageName::GetShortName(Package);

	GeneratedCPPFilenameBase =  PackageName + TEXT(".generated");
	GeneratedProtoFilenameBase = PackageName + TEXT(".generated");
	GeneratedMCPFilenameBase = PackageName + TEXT(".generated");
	bFailIfGeneratedCodeChanges = FParse::Param(FCommandLine::Get(), TEXT("FailIfGeneratedCodeChanges"));

	// Tag native classes in this package for export.

	Classes.Add(UObject::StaticClass());
	ClassTree.GetChildClasses(Classes, FDefaultComparator(), true);

	TMap<FName,UClass*> ClassNameMap;
	for ( int32 ClassIndex = 0; ClassIndex < Classes.Num(); ClassIndex++ )
	{
		ClassNameMap.Add(Classes[ClassIndex]->GetFName(), Classes[ClassIndex]);
	}

	class UnsortedHeaderFilenamesType
	{
	public:
		FString* AddUnique(const FString& Str)
		{
			for (auto It = Data.CreateConstIterator(); It; ++It)
			{
				auto& Ptr = *It;

				if (*Ptr == Str)
					return &*Ptr;
			}

			return Data[Data.Add(MakeUnique<FString>(Str))].Get();
		}

		TArray<TUniquePtr<FString>> Data;
	};

	UnsortedHeaderFilenamesType UnsortedHeaderFilenames;
	TArray<const FString*>      HeaderFilenames; // These reference the strings stored in UnsortedHeaderFilenames

	bool bCircularDependencyDetected = false;
 	{
	 	TMap<const FString*, HeaderDependents> HeaderDependencyMap;
		TArray<UClass*> ClassesInPackage;

		ClassesInPackage.Add(UObject::StaticClass());
		ClassTree.GetChildClasses(ClassesInPackage, FClassPackageComparator(Package), true);
		for (auto ClassIt = ClassesInPackage.CreateConstIterator(); ClassIt; ++ClassIt)
		{
			UClass* Cls = *ClassIt;

			auto ClsHeaderFilename = GClassHeaderFilenameMap.FindRef(Cls);

			bool bIsExportClass = Cls->HasAnyClassFlags(CLASS_Native) && !Cls->HasAnyClassFlags(CLASS_NoExport|CLASS_Intrinsic|CLASS_Temporary);
			if ( bIsExportClass )
			{
				auto* HeaderFilename = UnsortedHeaderFilenames.AddUnique(ClsHeaderFilename);
				auto& Dependencies   = HeaderDependencyMap.FindOrAdd(HeaderFilename);

				// Add the super class' header as a dependency if it's different.
				UClass* SuperClass = Cls->GetSuperClass();
				bool bIsDependentExportClass = SuperClass && SuperClass->HasAnyClassFlags(CLASS_Native) && !SuperClass->HasAnyClassFlags(CLASS_NoExport|CLASS_Intrinsic|CLASS_Temporary);
				if ( bIsDependentExportClass && SuperClass->GetOuter() == Cls->GetOuter() )
				{
					auto SuperClassHeaderFilename = GClassHeaderFilenameMap.FindRef(SuperClass);
					if ( SuperClassHeaderFilename != ClsHeaderFilename )
					{
						auto* DependentHeaderFilename = UnsortedHeaderFilenames.AddUnique(SuperClassHeaderFilename);
						Dependencies.AddUnique(DependentHeaderFilename);
					}
				}

				// Add base interface headers as dependencies, if they're different.
				for (TArray<FImplementedInterface>::TIterator It(Cls->Interfaces); It; ++It)
				{
					UClass* InterfaceClass = It->Class;
					bool bIsDependentExportClass = InterfaceClass->HasAnyClassFlags(CLASS_Native) && !InterfaceClass->HasAnyClassFlags(CLASS_NoExport|CLASS_Intrinsic|CLASS_Temporary);
					if ( bIsDependentExportClass && InterfaceClass->GetOuter() == Cls->GetOuter() )
					{
						auto InterfaceClassHeaderFilename = GClassHeaderFilenameMap.FindRef(InterfaceClass);
						if ( InterfaceClassHeaderFilename != ClsHeaderFilename )
						{
							auto* DependentHeaderFilename = UnsortedHeaderFilenames.AddUnique(InterfaceClassHeaderFilename);
							Dependencies.AddUnique(DependentHeaderFilename);
						}
					}
				}
			}
			else if (Cls->HasAnyClassFlags(CLASS_Temporary))
			{
				// Struct only headers can generate header groups too so make sure their added too.
				auto* HeaderFilename = UnsortedHeaderFilenames.AddUnique(ClsHeaderFilename);
				HeaderDependencyMap.FindOrAdd(HeaderFilename);
			}
		}
		bCircularDependencyDetected = !SortHeaderDependencyMap(HeaderDependencyMap, HeaderFilenames);
		if (bCircularDependencyDetected)
		{
			// Find one circular path (though there may be multiple).
			for (auto HeaderIt = HeaderDependencyMap.CreateConstIterator(); HeaderIt; ++HeaderIt)
			{
				const FString* Header = HeaderIt->Key;

				const FString* ClassHeaderFilename1;
				const FString* ClassHeaderFilename2;
				if (!FindInterDependency(HeaderDependencyMap, Header, ClassHeaderFilename1, ClassHeaderFilename2))
					continue;

				TArray<const UClass*> DependencyChain1, DependencyChain2;
				for (auto ClassIt = ClassesInPackage.CreateConstIterator(); ClassIt; ++ClassIt)
				{
					UClass* Class = *ClassIt;

					FindDependencyChain( Class, *ClassHeaderFilename1, *ClassHeaderFilename2, DependencyChain1 );

					if (DependencyChain1.Num())
						break;
				}

				for (auto ClassIt = ClassesInPackage.CreateConstIterator(); ClassIt; ++ClassIt)
				{
					UClass* Class = *ClassIt;

					FindDependencyChain( Class, *ClassHeaderFilename2, *ClassHeaderFilename1, DependencyChain2 );

					if (DependencyChain2.Num())
						break;
				}

				if (!DependencyChain1.Num() || !DependencyChain2.Num())
					continue;

				FString DependencyChainString1;
				for (auto DependencyIt = DependencyChain1.CreateConstIterator(); DependencyIt; ++DependencyIt)
				{
					auto Dependency = *DependencyIt;

					if (!DependencyChainString1.IsEmpty())
					{
						DependencyChainString1 += TEXT(" -> ");
					}

					DependencyChainString1 += Dependency->GetName();
				}

				FString DependencyChainString2;
				for (auto DependencyIt = DependencyChain2.CreateConstIterator(); DependencyIt; ++DependencyIt)
				{
					auto Dependency = *DependencyIt;

					if (!DependencyChainString1.IsEmpty())
					{
						DependencyChainString2 += TEXT(" -> ");
					}

					DependencyChainString2 += Dependency->GetName();
				}

				UE_LOG(LogCompile, Error, TEXT("Header interdependency: %s <-> %s (%s and %s)."),
					*(PackageName + *ClassHeaderFilename1 + TEXT("Classes.h")),
					*(PackageName + *ClassHeaderFilename2 + TEXT("Classes.h")),
					*DependencyChainString1,
					*DependencyChainString2 );
				break;
			}
			UE_LOG(LogCompile, Error, TEXT("Interdependent headers detected - aborting!") );
		}
	}

	bool bHasNamesForExport = false;
	TempHeaderPaths.Empty();
	PackageHeaderPaths.Empty();
	// Reset header generation output strings
	GeneratedPackageCPP.Empty();
	GeneratedProtoText.Empty();
	GeneratedMCPText.Empty();
	CrossModuleGeneratedFunctionDeclarations.Empty();
	UniqueCrossModuleReferences.Empty();
	GeneratedFunctionDeclarations.Empty();
	GeneratedFunctionBodyTextSplit.Empty();
	
	for (auto It = HeaderFilenames.CreateConstIterator(); !bCircularDependencyDetected && It; ++It)
	{
		ClassHeaderFilename = **It;

		FString PkgDir;
		FString GeneratedIncludeDirectory;
		if (MakeCommandlet_FindPackageLocation(*PackageName, PkgDir, GeneratedIncludeDirectory) == false)
		{
			UE_LOG(LogCompile, Error, TEXT("Failed to find path for package %s"), *PackageName);
		}
		ClassesHeaderPath = GeneratedIncludeDirectory / PackageName + ClassHeaderFilename + TEXT("Classes.h");

		int32 ClassCount = 0;
		for( int32 ClassIndex = 0; ClassIndex < Classes.Num(); ClassIndex++ )
		{
			UClass* Class = Classes[ClassIndex];

			if( Class->GetOuter()==Package && Class->HasAnyClassFlags(CLASS_Native|CLASS_Temporary) && GClassHeaderFilenameMap.FindRef(Class) == ClassHeaderFilename )
			{
				if (GClassStrippedHeaderTextMap.Contains(Class) && !Class->HasAnyClassFlags(CLASS_NoExport))
				{
					ClassCount++;
					// Skip temporary classes autogenerated in struct-only headers.
					if (!Class->HasAnyClassFlags(CLASS_Temporary))
					{
						Class->UnMark(OBJECTMARK_TagImp);
						Class->Mark(OBJECTMARK_TagExp);
					}
				}
			}
			else
			{
				Class->UnMark(EObjectMark(OBJECTMARK_TagImp | OBJECTMARK_TagExp));
			}
		}

		if( ClassCount == 0 )
		{
			continue;
		}

		CurrentClass = NULL;
		ListOfPublicClassesUObjectHeaderIncludes.Empty();
		ListOfAllUObjectHeaderIncludes.Empty();
		OriginalHeader.Empty();
		PreHeaderText.Empty();

		// Load the original header file into memory
		FFileHelper::LoadFileToString(OriginalHeader,*ClassesHeaderPath);
		
		UE_LOG(LogCompile, Log,  TEXT("Autogenerating C++ header: %s"), *ClassHeaderFilename );

		// Write the classes and enums header prefixes.

		PreHeaderText.Logf(
			TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.\r\n")
			TEXT("/*===========================================================================\r\n")
			TEXT("    C++ class boilerplate exported from UnrealHeaderTool.\r\n")
			TEXT("    This is automatically generated by the tools.\r\n")
			TEXT("    DO NOT modify this manually! Edit the corresponding .h files instead!\r\n")
			TEXT("===========================================================================*/\r\n")
			TEXT("#pragma once\r\n")
			TEXT("\r\n")
			);

		// if a global auto-include file exists, generate a line to have that file included
		FString GlobalAutoIncludeFilename = PackageName + ClassHeaderFilename + TEXT("GlobalIncludes.h");
		const FString StandardHeaderFileLocation = PkgDir / TEXT("Public");
		if ( IFileManager::Get().FileSize(*(StandardHeaderFileLocation / GlobalAutoIncludeFilename)) > 0 )
		{
			PreHeaderText.Logf(TEXT("#include \"%s\"\r\n\r\n"), *GlobalAutoIncludeFilename);
		}

		FString HeaderAPI = API;
		if( ClassHeaderFilename.Len() > 0 )
		{
			HeaderAPI = HeaderAPI + TEXT("_") + ClassHeaderFilename.ToUpper();
		}

		// Export an include line for each header
		for ( int32 ClassIndex = 0; ClassIndex < Classes.Num(); ClassIndex++ )
		{
			UClass* Class = Classes[ClassIndex];
			if ( GClassHeaderFilenameMap.FindRef(Class) == ClassHeaderFilename && Class->GetOuter() == Package )
			{
				ExportClassHeader(Class);
			}
		}
		
		ListOfPublicClassesUObjectHeaderIncludes.Logf( TEXT("\r\n") );
		
		// build the full header file out of its pieces
		const FString FullClassesHeader = FString::Printf(
			TEXT("%s\r\n%s"),
			*PreHeaderText, 
			*ListOfPublicClassesUObjectHeaderIncludes
		);

		// Save the classes header if it has changed.
		SaveHeaderIfChanged(*ClassesHeaderPath,*FullClassesHeader);
	}

	if (HeaderFilenames.Num() == 0)
	{
		// If no headers are generated, check if there's any no export classes that need to have the inl file generated.
		for ( int32 ClassIndex = 0; ClassIndex < Classes.Num(); ClassIndex++ )
		{
			UClass* Class = Classes[ClassIndex];
			if ( Class->GetOuter() == Package && !Class->HasAnyClassFlags(CLASS_Intrinsic|CLASS_Temporary) )
			{
				ExportClassHeader(Class);
			}
		}
	}

	// now export the names for the functions in this package
	// notice we always export this file (as opposed to only exporting if we have any marked names)
	// because there would be no way to know when the file was created otherwise
	if (!bCircularDependencyDetected)
	{
		// Export ..generated.cpp
		ExportGeneratedCPP();
	}

	ExportGeneratedProto();

	ExportGeneratedMCP();

	// Export all changed headers from their temp files to the .h files
	ExportUpdatedHeaders(PackageName);

	// Delete stale *.generated.h files
	DeleteUnusedGeneratedHeaders();
}

void FNativeClassHeaderGenerator::DeleteUnusedGeneratedHeaders()
{
	TArray<FString> AllIntermediateFolders;

	for (auto It = PackageHeaderPaths.CreateConstIterator(); It; ++It)
	{
		const FString IntermediatePath = FPaths::GetPath(*It);

		if (AllIntermediateFolders.Contains(IntermediatePath))
		{
			continue;
		}

		AllIntermediateFolders.Add( IntermediatePath );

		TArray<FString> AllHeaders;
		IFileManager::Get().FindFiles( AllHeaders, *(IntermediatePath / TEXT("*.generated.h")), true, false );

		for (auto It = AllHeaders.CreateConstIterator(); It; ++It)
		{
			const FString HeaderPath = IntermediatePath / *It;

			if (PackageHeaderPaths.Contains(HeaderPath))
			{
				continue;
			}

			// Check intrinsic classes. Get the class name from file name by removing .generated.h.
			const FString HeaderFilename = FPaths::GetBaseFilename(HeaderPath);
			const int32   GeneratedIndex = HeaderFilename.Find(TEXT(".generated"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			const FString ClassName      = HeaderFilename.Mid(0, GeneratedIndex);
			UClass* IntrinsicClass       = FindObject<UClass>(ANY_PACKAGE, *ClassName);
			if (!IntrinsicClass || !IntrinsicClass->HasAnyClassFlags(CLASS_Intrinsic))
			{
				IFileManager::Get().Delete(*HeaderPath);
			}
		}
	}
}

bool FNativeClassHeaderGenerator::SaveHeaderIfChanged(const TCHAR* HeaderPath, const TCHAR* InNewHeaderContents)
{
	if ( !bAllowSaveExportedHeaders )
	{
		// Return false indicating that the header did not need updating
		return false;
	}

	FString Tabified = Tabify(InNewHeaderContents);
	const TCHAR* NewHeaderContents = *Tabified;
	static bool bTestedCmdLine = false;
	if (!bTestedCmdLine)
	{
		bTestedCmdLine = true;
		if( FParse::Param( FCommandLine::Get(), TEXT("WRITEREF") ) )
		{
			bWriteContents = true;
			UE_LOG(LogCompile, Log, TEXT("********************************* Writing reference generated code to ReferenceGeneratedCode."));
			UE_LOG(LogCompile, Log, TEXT("********************************* Deleting all files in ReferenceGeneratedCode."));
			IFileManager::Get().DeleteDirectory(*(FString(FPaths::GameSavedDir()) / TEXT("ReferenceGeneratedCode/")), false, true);
			IFileManager::Get().MakeDirectory(*(FString(FPaths::GameSavedDir()) / TEXT("ReferenceGeneratedCode/")));
		}
		else if( FParse::Param( FCommandLine::Get(), TEXT("VERIFYREF") ) )
		{
			bVerifyContents = true;
			UE_LOG(LogCompile, Log, TEXT("********************************* Writing generated code to VerifyGeneratedCode and comparing to ReferenceGeneratedCode"));
			UE_LOG(LogCompile, Log, TEXT("********************************* Deleting all files in VerifyGeneratedCode."));
			IFileManager::Get().DeleteDirectory(*(FString(FPaths::GameSavedDir()) / TEXT("VerifyGeneratedCode/")), false, true);
			IFileManager::Get().MakeDirectory(*(FString(FPaths::GameSavedDir()) / TEXT("VerifyGeneratedCode/")));
		}
	}

	if (bWriteContents || bVerifyContents)
	{
		FString Ref = FString(FPaths::GameSavedDir()) / TEXT("ReferenceGeneratedCode") / FPaths::GetCleanFilename(HeaderPath);
		FString Verify = FString(FPaths::GameSavedDir()) / TEXT("VerifyGeneratedCode") / FPaths::GetCleanFilename(HeaderPath);

		if (bWriteContents)
		{
			int32 i;
			for (i = 0 ;i < 10; i++)
			{
				if (FFileHelper::SaveStringToFile(NewHeaderContents, *Ref))
				{
					break;
				}
				FPlatformProcess::Sleep(1.0f); // I don't know why this fails after we delete the directory
			}
			check(i<10);
		}
		else
		{
			int32 i;
			for (i = 0 ;i < 10; i++)
			{
				if (FFileHelper::SaveStringToFile(NewHeaderContents, *Verify))
				{
					break;
				}
				FPlatformProcess::Sleep(1.0f); // I don't know why this fails after we delete the directory
			}
			check(i<10);
			FString RefHeader;
			FString Message;
			if (!FFileHelper::LoadFileToString(RefHeader, *Ref))
			{
				Message = FString::Printf(TEXT("********************************* %s appears to be a new generated file."), *FPaths::GetCleanFilename(HeaderPath));
			}
			else
			{
				if (FCString::Strcmp(NewHeaderContents, *RefHeader) != 0)
				{
					Message = FString::Printf(TEXT("********************************* %s has changed."), *FPaths::GetCleanFilename(HeaderPath));
				}
			}
			if (Message.Len())
			{
				UE_LOG(LogCompile, Log, TEXT("%s"), *Message);
				ChangeMessages.AddUnique(Message);
			}
		}
	}


	FString OriginalHeaderLocal;
	FFileHelper::LoadFileToString(OriginalHeaderLocal, HeaderPath);

	const bool bHasChanged = OriginalHeaderLocal.Len() == 0 || FCString::Strcmp(*OriginalHeaderLocal, NewHeaderContents);
	if (bHasChanged)
	{
		if (bFailIfGeneratedCodeChanges)
		{
			FString ConflictPath = FString(HeaderPath) + TEXT(".conflict");
			FFileHelper::SaveStringToFile(NewHeaderContents, *ConflictPath);
			FError::Throwf(TEXT("ERROR: '%s': Changes to generated code are not allowed - conflicts written to '%s'"), HeaderPath, *ConflictPath);
		}

		// save the updated version to a tmp file so that the user can see what will be changing
		const FString TmpHeaderFilename = GenerateTempHeaderName( HeaderPath, false );

		// delete any existing temp file
		IFileManager::Get().Delete( *TmpHeaderFilename, false, true );
		if ( !FFileHelper::SaveStringToFile(NewHeaderContents, *TmpHeaderFilename) )
		{
			UE_LOG(LogCompile, Warning, TEXT("Failed to save header export preview: '%s'"), *TmpHeaderFilename);
		}

		TempHeaderPaths.Add(TmpHeaderFilename);
	}

	// Remember this header filename to be able to check for any old (unused) headers later.
	PackageHeaderPaths.Add( FString(HeaderPath).Replace( TEXT( "\\" ), TEXT( "/" ) ) );

	return bHasChanged;
}

/**
* Create a temp header file name from the header name
*
* @param	CurrentFilename		The filename off of which the current filename will be generated
* @param	bReverseOperation	Get the header from the temp file name instead
*
* @return	The generated string
*/
FString FNativeClassHeaderGenerator::GenerateTempHeaderName( FString CurrentFilename, bool bReverseOperation )
{
	return bReverseOperation
		? CurrentFilename.Replace(TEXT(".tmp"), TEXT(""))
		: CurrentFilename + TEXT(".tmp");
}

/** 
* Exports the temp header files into the .h files, then deletes the temp files.
* 
* @param	PackageName	Name of the package being saved
*/
void FNativeClassHeaderGenerator::ExportUpdatedHeaders(FString PackageName)
{
	for (auto It = TempHeaderPaths.CreateConstIterator(); It; ++It)
	{
		const FString& TmpFilename = *It;

		FString Filename = GenerateTempHeaderName( TmpFilename, true );
		if (!IFileManager::Get().Move(*Filename, *TmpFilename, true, true))
		{
			UE_LOG(LogCompile, Error, TEXT("%s"), *FString::Printf(TEXT("Error exporting %s: couldn't write file '%s'"),*PackageName,*Filename));
		}
		else
		{
			UE_LOG(LogCompile, Log, TEXT("Exported updated C++ header: %s"), *Filename);
		}
	}
}

/**
 * Exports protobuffer definitions from boilerplate that was generated for a package.
 * They are exported to a file using the name <PackageName>.generated.proto
 */
void FNativeClassHeaderGenerator::ExportGeneratedProto()
{
	if (GeneratedProtoText.Len())
	{
		UE_LOG(LogCompile, Log,  TEXT("Autogenerating boilerplate proto: %s.proto"), *GeneratedProtoFilenameBase );

		FStringOutputDevice ProtoPreamble;

		ProtoPreamble.Logf(
			TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.")					LINE_TERMINATOR
			TEXT("/*===========================================================================")	LINE_TERMINATOR
			TEXT("  Purpose: The file defines our Google Protocol Buffers which are used in over ") LINE_TERMINATOR
			TEXT("  the wire messages between servers as well as between clients and servers.")		LINE_TERMINATOR
			TEXT("  This is automatically generated by UnrealHeaderTool.")							LINE_TERMINATOR
			TEXT("  DO NOT modify this manually! Edit the corresponding .h files instead!")			LINE_TERMINATOR
			TEXT("===========================================================================*/")	LINE_TERMINATOR
																									LINE_TERMINATOR
			TEXT("// We care more about speed than code size")										LINE_TERMINATOR
			TEXT("option optimize_for = SPEED;")													LINE_TERMINATOR
			TEXT("// We don't use the service generation functionality")							LINE_TERMINATOR
			TEXT("option cc_generic_services = false;")												LINE_TERMINATOR
																									LINE_TERMINATOR
			);

		FString PkgName = FPackageName::GetShortName(Package);
		FString PkgDir;
		FString GeneratedIncludeDirectory;
		if (MakeCommandlet_FindPackageLocation(*PkgName, PkgDir, GeneratedIncludeDirectory) == false)
		{
			UE_LOG(LogCompile, Error, TEXT("Failed to find path for package %s"), *PkgName);
		}

		FString HeaderPath = GeneratedIncludeDirectory / GeneratedProtoFilenameBase + TEXT(".proto");
		SaveHeaderIfChanged(*HeaderPath, *(ProtoPreamble + GeneratedProtoText));
	}
}

/**
 * Exports MCPbuffer definitions from boilerplate that was generated for a package.
 * They are exported to a file using the name <PackageName>.generated.MCP
 */
void FNativeClassHeaderGenerator::ExportGeneratedMCP()
{
	if (GeneratedMCPText.Len())
	{
		UE_LOG(LogCompile, Log,  TEXT("Autogenerating boilerplate MCP: %s.java"), *GeneratedMCPFilenameBase );

		FStringOutputDevice MCPPreamble;

		MCPPreamble.Logf(
			TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.")					LINE_TERMINATOR
			TEXT("/*===========================================================================")	LINE_TERMINATOR
			TEXT("  Purpose: The file defines java heaers for MCP rpc messages. ")					LINE_TERMINATOR
			TEXT("  DO NOT modify this manually! Edit the corresponding .h files instead!")			LINE_TERMINATOR
			TEXT("===========================================================================*/")	LINE_TERMINATOR
			);

		FString PkgName = FPackageName::GetShortName(Package);
		FString PkgDir;
		FString GeneratedIncludeDirectory;
		if (MakeCommandlet_FindPackageLocation(*PkgName, PkgDir, GeneratedIncludeDirectory) == false)
		{
			UE_LOG(LogCompile, Error, TEXT("Failed to find path for package %s"), *PkgName);
		}

		FString HeaderPath = GeneratedIncludeDirectory / GeneratedMCPFilenameBase + TEXT(".java");
		SaveHeaderIfChanged(*HeaderPath, *(MCPPreamble + GeneratedMCPText));
	}
}

/**
 * Exports C++ definitions for boilerplate that was generated for a package.
 * They are exported to a file using the name <PackageName>.generated.inl
 * @param ReferencedNames list of function names to export.
 */
void FNativeClassHeaderGenerator::ExportGeneratedCPP()
{
	FStringOutputDevice GeneratedCPPPreamble;
	FStringOutputDevice GeneratedCPPEpilogue;
	FStringOutputDevice GeneratedCPPText;

	TArray<FStringOutputDevice> GeneratedCPPFiles;

	UE_LOG(LogCompile, Log,  TEXT("Autogenerating boilerplate cpp: %s.cpp"), *GeneratedCPPFilenameBase );

	GeneratedCPPPreamble.Logf(
		TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.")					LINE_TERMINATOR
		TEXT("/*===========================================================================")	LINE_TERMINATOR
		TEXT("    Boilerplate C++ definitions for a single module.")							LINE_TERMINATOR
		TEXT("    This is automatically generated by UnrealHeaderTool.")						LINE_TERMINATOR
		TEXT("    DO NOT modify this manually! Edit the corresponding .h files instead!")		LINE_TERMINATOR
		TEXT("===========================================================================*/")	LINE_TERMINATOR
																								LINE_TERMINATOR
		);

	// Include all the UObject headers for this module first, in the same order that we digested them
	GeneratedCPPText.Logf( *ListOfAllUObjectHeaderIncludes );

	GeneratedCPPText.Log(*GeneratedPackageCPP);
	// Add all names marked for export to a list (for sorting)
	TArray<FString> Names;
	for (TSet<FName>::TIterator It(ReferencedNames); It; ++It)
	{
		Names.Add(It->ToString());
	}	

	// Autogenerate names (alphabetically sorted).
	Names.Sort();
	for( int32 NameIndex=0; NameIndex<Names.Num(); NameIndex++ )
	{
		GeneratedCPPText.Logf( TEXT("FName %s_%s = FName(TEXT(\"%s\"));") LINE_TERMINATOR, *API, *Names[NameIndex], *Names[NameIndex] );
	}

	GeneratedCPPEpilogue.Logf(
		LINE_TERMINATOR
		);
	
	FString PkgName = FPackageName::GetShortName(Package);
	FString PkgDir;
	FString GeneratedIncludeDirectory;
	if (MakeCommandlet_FindPackageLocation(*PkgName, PkgDir, GeneratedIncludeDirectory) == false)
	{
		UE_LOG(LogCompile, Error, TEXT("Failed to find path for package %s"), *PkgName);
	}
	if (GeneratedFunctionDeclarations.Len() || CrossModuleGeneratedFunctionDeclarations.Len())
	{
		ExportGeneratedPackageInitCode(Package);
	}

	TArray<FString> NumberedHeaderNames;

	// Generate each of the .generated.inl files
	for( int32 FileIdx=0;FileIdx<GeneratedFunctionBodyTextSplit.Num();FileIdx++ )
	{
		FStringOutputDevice FileText;
		// The first file has all of the GeneratedCPPText, only the functions are split.
		if( FileIdx == 0)
		{
			FileText = GeneratedCPPText;
		}

		if (GeneratedFunctionDeclarations.Len() || CrossModuleGeneratedFunctionDeclarations.Len())
		{
			FileText.Logf(TEXT("#if USE_COMPILED_IN_NATIVES\r\n"));
			if (CrossModuleGeneratedFunctionDeclarations.Len())
			{
				FileText.Logf(TEXT("// Cross Module References\r\n"));
				FileText.Log(CrossModuleGeneratedFunctionDeclarations);
				FileText.Logf(TEXT("\r\n"));
			}
			FileText.Log(GeneratedFunctionDeclarations);
			FileText.Log(GeneratedFunctionBodyTextSplit[FileIdx]);
			FileText.Logf(TEXT("#endif\r\n"));
		}

		FString HeaderName = GeneratedCPPFilenameBase + (GeneratedFunctionBodyTextSplit.Num() > 1 ? *FString::Printf(TEXT(".%d.inl"),FileIdx+1) : TEXT(".inl"));
		FString HeaderPath = GeneratedIncludeDirectory / HeaderName;
		
		SaveHeaderIfChanged(*HeaderPath, *(GeneratedCPPPreamble + FileText + GeneratedCPPEpilogue));

		if (GeneratedFunctionBodyTextSplit.Num() > 1)
		{
			NumberedHeaderNames.Add(HeaderName);
		}
	}

	if (GeneratedFunctionBodyTextSplit.Num() > 1)
	{
		FStringOutputDevice FileText;

		for (int32 i=0; i < NumberedHeaderNames.Num(); ++i)
		{
			FileText.Logf(TEXT("#include \"%s\"") LINE_TERMINATOR, *NumberedHeaderNames[i]);
		}

		FString HeaderPath = GeneratedIncludeDirectory / GeneratedCPPFilenameBase + TEXT(".inl");
		SaveHeaderIfChanged(*HeaderPath, *(GeneratedCPPPreamble + FileText + GeneratedCPPEpilogue));
	}

	if( FParse::Param( FCommandLine::Get(), TEXT("GenerateConstructors") ) && AllConstructors.Len())
	{
		const FString PrivateLoc = PkgDir / PkgName / TEXT("Private");
		FString ConstructorPath = PrivateLoc / PkgName + TEXT("Constructors.cpp");
		FString ConstructorsWithIfdef(TEXT("// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.\r\n"));
		ConstructorsWithIfdef += *Tabify(*AllConstructors);
		SaveHeaderIfChanged(*ConstructorPath,*ConstructorsWithIfdef);
	}
}


int32 UnrealHeaderTool_Main(const FString& ModuleInfoFilename)
{
	check(GIsUCCMakeStandaloneHeaderGenerator);
	
	if ( !FParse::Param( FCommandLine::Get(), TEXT("IgnoreWarnings")) )
	{
		GWarn->TreatWarningsAsErrors = true;
	}

	FString ModuleInfoPath = FPaths::GetPath(ModuleInfoFilename);

	// Load the manifest file, giving a list of all modules to be processed, pre-sorted by dependency ordering
	try
	{
		CodeGeneneratorConfig.LoadManifest( ModuleInfoPath, ModuleInfoFilename );
	}
	catch (const TCHAR* Ex)
	{
		UE_LOG(LogCompile, Error, TEXT("Failed to load manifest file '%s': %s"), *ModuleInfoFilename, Ex);
		return 1;
	}

	NameLookupCPP = new FNameLookupCPP();

	FCompilerMetadataManager* ScriptHelper = new FCompilerMetadataManager();
	GScriptHelper = ScriptHelper;

	// Load classes for editing.
	bool Success = true;
	int32 NumFailures = 0;

	// Three passes.  1) Public 'Classes' headers (legacy)  2) Public headers   3) Private headers
	for( int32 PassIndex = 0; PassIndex < 3; ++PassIndex )
	{
		enum
		{
			PublicClassesHeaders,
			PublicHeaders,
			PrivateHeaders,
		} CurrentlyProcessing;
		{
			switch( PassIndex )
			{
				case 0:
				default:
					CurrentlyProcessing = PublicClassesHeaders;
					break;

				case 1:
					CurrentlyProcessing = PublicHeaders;
					break;

				case 2:
					CurrentlyProcessing = PrivateHeaders;
					break;
			};
		}

		for (const FUObjectModuleInfo& Module : CodeGeneneratorConfig.UObjectModuleList)
		{
			if (!Success)
				break;

			// We'll make an ordered list of all UObject headers we care about.
			TArray<FString> UObjectHeaders;
			if( CurrentlyProcessing == PublicClassesHeaders )
			{
				UObjectHeaders = Module.PublicUObjectClassesHeaders;
			}
			// @todo uht: Ideally 'dependson' would not be allowed from public -> private, or NOT at all for new style headers
			else if( CurrentlyProcessing == PublicHeaders )
			{
				// Append the public headers
				UObjectHeaders = Module.PublicUObjectHeaders;
			}
			else
			{
				// Append the private headers
				UObjectHeaders = Module.PrivateUObjectHeaders;
			}
			if( UObjectHeaders.Num() > 0 )
			{
				UPackage* Package = Cast<UPackage>( StaticFindObjectFast( UPackage::StaticClass(), NULL, FName(*Module.LongPackageName), false, false ) );
				if( Package == NULL )
				{
					Package = CreatePackage( NULL, *Module.LongPackageName );
				}

				// Set some package flags for indicating that this package contains script
				// NOTE: We do this even if we didn't have to create the package, because CoreUObject is compiled into UnrealHeaderTool and we still
				//       want to make sure our flags get set
				Package->PackageFlags |= PKG_ContainsScript;
				Package->PackageFlags &= ~(PKG_ClientOptional|PKG_ServerSideOnly);
				Package->PackageFlags |= PKG_Compiling;

				for (const FString& Filename : UObjectHeaders)
				{
					// Best faith effort at a useful line number for errors occurring during this initial pre-parsing
					int32 ClassDeclLine = -1;

				#if !PLATFORM_EXCEPTIONS_DISABLED
					try
				#endif
					{
						// Import class.
						const FString ClassName      = FPaths::GetBaseFilename(Filename);
						const FString FullModulePath = FPaths::ConvertRelativePathToFull(ModuleInfoPath, Filename);

						FString HeaderFile;
						if (!FFileHelper::LoadFileToString(HeaderFile, *FullModulePath))
							FError::Throwf(TEXT( "UnrealHeaderTool was unable to load source file '%s'"), *FullModulePath);

						UClass* ResultClass = GenerateCodeForHeader(Package, *ClassName, RF_Public|RF_Standalone, *HeaderFile, ClassDeclLine);
						GClassSourceFileMap.Add(ResultClass, Filename);

						if( CurrentlyProcessing == PublicClassesHeaders )
						{
							GPublicClassSet.Add(ResultClass);
						}

						// Save metadata for the class path, both for it's include path and relative to the module base directory
						if(FullModulePath.StartsWith(Module.BaseDirectory))
						{
							// Get the path relative to the module directory
							const TCHAR *ModuleRelativePath = *FullModulePath + Module.BaseDirectory.Len();
							GClassModuleRelativePathMap.Add(ResultClass, ModuleRelativePath);

							// Add the include path
							const TCHAR *IncludePath = ModuleRelativePath;
							if(*IncludePath == '/')
							{
								IncludePath++;
							}
							while(*IncludePath != 0 && *IncludePath != '/')
							{
								IncludePath++;
							}
							if(*IncludePath == '/' && *(IncludePath + 1) != 0)
							{
								GClassIncludePathMap.Add(ResultClass, IncludePath + 1);
							}
						}
					}
				#if !PLATFORM_EXCEPTIONS_DISABLED
					catch( TCHAR* ErrorMsg )
					{
						TGuardValue<ELogTimes::Type> DisableLogTimes(GPrintLogTimes, ELogTimes::None);

						FString Prefix;
						if (ClassDeclLine != -1)
						{
							const FString AbsFilename = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Filename);
							Prefix = FString::Printf(TEXT("%s(%i): "), *AbsFilename, ClassDeclLine);
						}

						FString FormattedErrorMessage = FString::Printf(TEXT("%sError: %s\r\n"), *Prefix, ErrorMsg);

						UE_LOG(LogCompile, Log, TEXT("%s"), *FormattedErrorMessage);
						GWarn->Log(ELogVerbosity::Error, FormattedErrorMessage);

						++NumFailures;
					}
				#endif
				}

				Success = NumFailures == 0;
			}
		}
	}


	// Save generated headers
	if ( Success )
	{
		// Verify that all script declared superclasses exist.
		for( TObjectIterator<UClass> ItC; ItC; ++ItC )
		{
			const UClass* ScriptClass      = *ItC;
			const UClass* ScriptSuperClass = ScriptClass->GetSuperClass();

			if (ScriptSuperClass && !ScriptSuperClass->HasAnyClassFlags(CLASS_Intrinsic) && GClassStrippedHeaderTextMap.Contains(ScriptClass) && !GClassStrippedHeaderTextMap.Contains(ScriptSuperClass))
			{
				UE_LOG(LogCompile, Error, TEXT("Superclass %s of class %s not found"), *ScriptSuperClass->GetName(), *ScriptClass->GetName());
				Success = false;
				++NumFailures;
			}
		}

		if (Success)
		{
			for (const FUObjectModuleInfo& Module : CodeGeneneratorConfig.UObjectModuleList)
			{
				if (UPackage* Package = Cast<UPackage>( StaticFindObjectFast( UPackage::StaticClass(), NULL, FName(*Module.LongPackageName), false, false ) ))
				{
					Success = FHeaderParser::ParseAllHeadersInside(GWarn, Package, Module.SaveExportedHeaders, CodeGeneneratorConfig.UseRelativePaths);
					if (!Success)
					{
						++NumFailures;
						break;
					}
				}
			}
		}
	}

	// Avoid TArray slack for meta data.
	GScriptHelper->Shrink();

	if( bWriteContents )
	{
		UE_LOG(LogCompile, Log, TEXT("********************************* Wrote reference generated code to ReferenceGeneratedCode."));
	}
	else if( bVerifyContents )
	{
		UE_LOG(LogCompile, Log, TEXT("********************************* Wrote generated code to VerifyGeneratedCode and compared to ReferenceGeneratedCode"));
		for (FString& Msg : ChangeMessages)
		{
			UE_LOG(LogCompile, Error, TEXT("%s"), *Msg);
		}
		TArray<FString> RefFileNames;
		IFileManager::Get().FindFiles( RefFileNames, *(FString(FPaths::GameSavedDir()) / TEXT("ReferenceGeneratedCode/*.*")), true, false );
		TArray<FString> VerFileNames;
		IFileManager::Get().FindFiles( VerFileNames, *(FString(FPaths::GameSavedDir()) / TEXT("VerifyGeneratedCode/*.*")), true, false );
		if (RefFileNames.Num() != VerFileNames.Num())
		{
			UE_LOG(LogCompile, Error, TEXT("Number of generated files mismatch ref=%d, ver=%d"), RefFileNames.Num(), VerFileNames.Num());
		}
	}
	TheFlagAudit.WriteResults();

	delete NameLookupCPP;
	NameLookupCPP = NULL;

	delete ScriptHelper;
	ScriptHelper = NULL;

	GIsRequestingExit = true;
	
	return NumFailures > 0 ? 1 : 0;
}

UClass* GenerateCodeForHeader
(
	UObject*     InParent,
	const TCHAR* Name,
	EObjectFlags Flags,
	const TCHAR* Buffer,
	int32&       OutClassDeclLine
)
{
	// Support for headers without UClasses.
	bool bNonClassHeader = false;
	const TCHAR* InBuffer = Buffer;

	// Import the script text.
	TArray<FName> DependentOn;

	// is the parsed class name an interface?
	bool bClassIsAnInterface = false;

	// Parse the header to extract the information needed
	FStringOutputDevice ClassHeaderTextStrippedOfCppText;
	FString ClassName;
	FString BaseClassName;
	FHeaderParser::SimplifiedClassParse(Buffer, /*out*/ bClassIsAnInterface, /*out*/ DependentOn, /*out*/ ClassName, /*out*/ BaseClassName, /*out*/ OutClassDeclLine, ClassHeaderTextStrippedOfCppText);

	// In case no UClass is defined, generate the default temporary UClass.
	if (ClassName.IsEmpty())
	{
		ClassName = FString::Printf(TEXT("U%s"), Name);
		BaseClassName = TEXT("UObject");
		bNonClassHeader = true;
	}

	FString ClassNameStripped = FHeaderParser::GetClassNameWithPrefixRemoved( *ClassName );

	// Ensure the base class has any valid prefix and exists as a valid class. Checking for the 'correct' prefix will occur during compilation
	FString BaseClassNameStripped;
	if (!BaseClassName.IsEmpty())
	{
		BaseClassNameStripped = FHeaderParser::GetClassNameWithPrefixRemoved(BaseClassName);
		if( !FHeaderParser::ClassNameHasValidPrefix(BaseClassName, BaseClassNameStripped) )
			FError::Throwf(TEXT("No prefix or invalid identifier for base class %s.\nClass names must match Unreal prefix specifications (e.g., \"UObject\" or \"AActor\")"), *BaseClassName );

		if (DependentOn.ContainsByPredicate([&](const FName& Dependency){ FString DependencyStr = Dependency.ToString(); return !DependencyStr.Contains(TEXT(".generated.h")) && FPaths::GetBaseFilename(DependencyStr) == ClassNameStripped; }))
			FError::Throwf(TEXT("Class '%s' contains a dependency (#include or DependsOn) to itself"), *ClassName);
	}

	//UE_LOG(LogCompile, Log, TEXT("Class: %s extends %s"),*ClassName,*BaseClassName);
	// Handle failure and non-class headers.
	if (BaseClassName.IsEmpty() && (ClassName != TEXT("UObject")))
	{
		FError::Throwf(TEXT("Class '%s' must inherit UObject or a UObject-derived class"), *ClassName );
	}

	if (ClassName == BaseClassName)
	{
		FError::Throwf(TEXT("Class '%s' cannot inherit from itself"), *ClassName );
	}

	// In case the file system and the class disagree on the case of the
	// class name replace the fname with the one from the script class file
	// This is needed because not all source control systems respect the
	// original filename's case
	FName ClassNameReplace(*ClassName,FNAME_Replace_Not_Safe_For_Threading);

	// All classes must start with a valid unreal prefix
	if (!FHeaderParser::ClassNameHasValidPrefix(ClassName, Name) || !FHeaderParser::ClassNameHasValidPrefix(ClassNameReplace.ToString(), Name))
	{
		FError::Throwf(TEXT("Invalid class name '%s'. The class name must match the name of the class header file it is contained in ('%s'), with an appropriate prefix added (A for Actors, U for other classes)"), *ClassNameReplace.ToString(), Name);
	}

	// Use stripped class name for processing and replace as we did above
	FName ClassNameStrippedReplace(*ClassNameStripped, FNAME_Replace_Not_Safe_For_Threading);

	UClass* ResultClass = FindObject<UClass>(InParent, *ClassNameStripped);

	// if we aren't generating headers, then we shouldn't set misaligned object, since it won't get cleared

	const static bool bVerboseOutput = FParse::Param(FCommandLine::Get(), TEXT("VERBOSE"));

	// Gracefully update an existing hardcoded class.
	// Note: Most 'native' classes will not have RF_Native set at this point, only classes in Core will go down this path
	if ((ResultClass != NULL) && ResultClass->HasAnyFlags(RF_Native) )
	{
		UClass* SuperClass = ResultClass->GetSuperClass();
		if ((SuperClass != NULL) && (SuperClass->GetName() != BaseClassNameStripped))
		{
			// the code that handles the DependsOn list in the script compiler doesn't work correctly if we manually add the Object class to a class's DependsOn list
			// if Object is also the class's parent.  The only way this can happen (since specifying a parent class in a DependsOn statement is a compiler error) is
			// in this block of code, so just handle that here rather than trying to make the script compiler handle this gracefully
			if (BaseClassNameStripped != TEXT("Object"))
			{
				// we're changing the parent of a native class, which may result in the
				// child class being parsed before the new parent class, so add the new
				// parent class to this class's DependsOn() array to guarantee that it
				// will be parsed before this class
				DependentOn.AddUnique(*BaseClassNameStripped);
			}

			// if the new parent class is an existing native class, attempt to change the parent for this class to the new class 
			UClass* NewSuperClass = FindObject<UClass>(ANY_PACKAGE, *BaseClassNameStripped);
			if (NewSuperClass != NULL)
			{
				check(0); // we don't support changing base clases anymore
			}
		}
	}
	else
	{
		// detect if the same class name is used in multiple packages
		if (ResultClass == NULL)
		{
			UClass* ConflictingClass = FindObject<UClass>(ANY_PACKAGE, *ClassNameStripped, true);
			if (ConflictingClass != NULL)
			{
				UE_LOG(LogCompile, Warning, TEXT("Duplicate class name: %s also exists in file %s"), *ClassName, *ConflictingClass->GetOutermost()->GetName());
			}
		}

		// Create new class.
		ResultClass = new( InParent, *ClassNameStripped, Flags ) UClass( FPostConstructInitializeProperties(),NULL );
		GClassHeaderNameWithNoPathMap.Add(ResultClass, ClassNameStripped);

		// add CLASS_Interface flag if the class is an interface
		// NOTE: at this pre-parsing/importing stage, we cannot know if our super class is an interface or not,
		// we leave the validation to the main header parser
		if (bClassIsAnInterface)
		{
			ResultClass->ClassFlags |= CLASS_Interface;
		}

		// In case there actually was no class, mark it as UHT temporary.
		if (bNonClassHeader)
		{
			const FString DummyClassName = FHeaderParser::GenerateTemporaryClassName(*ResultClass->GetName());

			ResultClass->ClassFlags |= CLASS_Temporary | CLASS_Native;
			ResultClass->Rename(*DummyClassName, NULL, REN_DontCreateRedirectors);
		}

		// Find or forward-declare base class.
		ResultClass->SetSuperStruct( FindObject<UClass>( InParent, *BaseClassNameStripped ) );
		if (ResultClass->GetSuperStruct() == NULL)
		{
			ResultClass->SetSuperStruct( FindObject<UClass>( ANY_PACKAGE, *BaseClassNameStripped ) );
		}

		if (ResultClass->GetSuperStruct() == NULL)
		{
			// don't know its parent class yet
			ResultClass->SetSuperStruct( new(InParent, *BaseClassNameStripped) UClass(FPostConstructInitializeProperties(),NULL) );
		}

		if (ResultClass->GetSuperStruct() != NULL)
		{
			ResultClass->ClassCastFlags |= ResultClass->GetSuperClass()->ClassCastFlags;
		}

		if ( bVerboseOutput )
		{
			UE_LOG(LogCompile, Log, TEXT("Imported: %s"), *ResultClass->GetFullName() );
		}
	}

	if (bVerboseOutput)
	{
		for (const auto& Dependency : DependentOn)
		{
			UE_LOG(LogCompile, Log, TEXT("\tAdding %s as a dependency"), *Dependency.ToString());
		}
	}

	// Set class info.
	GClassStrippedHeaderTextMap.Emplace(ResultClass, MoveTemp(ClassHeaderTextStrippedOfCppText));
	GClassDependentOnMap       .Emplace(ResultClass, MoveTemp(DependentOn));

	return ResultClass;
}
