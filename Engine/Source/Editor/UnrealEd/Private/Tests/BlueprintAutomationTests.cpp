// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "ObjectTools.h"
#include "Json.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Editor/GraphEditor/Public/GraphDiffControl.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintAutomationTests, Log, All);

IMPLEMENT_COMPLEX_AUTOMATION_TEST( FBlueprintCompileTest, "Blueprints.Compile-On-Load", EAutomationTestFlags::ATF_Editor )
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FBlueprintInstancesTest, "Blueprints.Instance Test", EAutomationTestFlags::ATF_Editor )
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FBlueprintReparentTest, "Blueprints.Reparent", EAutomationTestFlags::ATF_Editor )
//IMPLEMENT_COMPLEX_AUTOMATION_TEST( FBlueprintRenameAndCloneTest, "Blueprints.Rename And Clone", EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser )

class FBlueprintAutomationTestUtilities
{
	/** An incrementing number that can be used to tack on to save files, etc. (for avoiding naming conflicts)*/
	static uint32 QueuedTempId;

	/** List of packages touched by automation tests that can no longer be saved */
	static TArray<FName> DontSavePackagesList;

	/** Callback to check if package is ok to save. */
	static bool IsPackageOKToSave(UPackage* InPackage, const FString& InFilename, FOutputDevice* Error)
	{
		return !DontSavePackagesList.Contains(InPackage->GetFName());
	}

	/**
	 * Gets a unique int (this run) for automation purposes (to avoid temp save 
	 * file collisions, etc.)
	 * 
	 * @return A id for unique identification that can be used to tack on to save files, etc. (for avoiding naming conflicts)
	 */
	static uint32 GenTempUid()
	{
		return QueuedTempId++;
	}

public:

	typedef TMap< FString, FString >	FPropertiesMap;

	/** 
	 * Helper struct to ensure that a package is not inadvertently left in
	 * a dirty state by automation tests
	 */
	struct FPackageCleaner
	{
		FPackageCleaner(UPackage* InPackage) : Package(InPackage)
		{
			bIsDirty = Package ? Package->IsDirty() : false;
		}

		~FPackageCleaner()
		{
			// reset the dirty flag
			if (Package) 
			{
				Package->SetDirtyFlag(bIsDirty);
			}
		}

	private:
		bool bIsDirty;
		UPackage* Package;
	};

	/**
	 * Loads the map specified by an automation test
	 * 
	 * @param MapName - Map to load
	 */
	static void LoadMap(const FString& MapName)
	{
		const bool bLoadAsTemplate = false;
		const bool bShowProgress = false;

		FEditorFileUtils::LoadMap(MapName, bLoadAsTemplate, bShowProgress);
	}

	/** 
	 * Filter used to test to see if a UProperty is candidate for comparison.
	 * @param Property	The property to test
	 *
	 * @return True if UProperty should be compared, false otherwise
	 */
	static bool ShouldCompareProperty (const UProperty* Property)
	{
		// Ignore components & transient properties
		const bool bIsTransient = !!( Property->PropertyFlags & CPF_Transient );
		const bool bIsComponent = !!( Property->PropertyFlags & (CPF_InstancedReference | CPF_ContainsInstancedReference) );
		const bool bShouldCompare = !(bIsTransient || bIsComponent);

		return (bShouldCompare && Property->HasAnyPropertyFlags(CPF_BlueprintVisible));
	}
	
	/** 
	 * Get a given UObject's properties in simple key/value string map
	 *
	 * @param Obj			The UObject to extract properties for 
	 * @param ObjProperties	[OUT] Property map to be filled
	 */
	static void GetObjProperties (UObject* Obj, FPropertiesMap& ObjProperties)
	{
		for (TFieldIterator<UProperty> PropIt(Obj->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
		{
			UProperty* Prop = *PropIt;

			if ( ShouldCompareProperty(Prop) )
			{
				for (int32 Index = 0; Index < Prop->ArrayDim; Index++)
				{
					FString PropName = (Prop->ArrayDim > 1) ? FString::Printf(TEXT("%s[%d]"), *Prop->GetName(), Index) : *Prop->GetName();
					FString PropText;
					Prop->ExportText_InContainer(Index, PropText, Obj, Obj, Obj, PPF_SimpleObjectText);
					ObjProperties.Add(PropName, PropText);
				}
			}
		}
	}

	/** 
	 * Compare two object property maps
	 * @param OrigName		The name of the original object being compared against
	 * @param OrigMap		The property map for the object
	 * @param CmpName		The name of the object to compare
	 * @param CmpMap		The property map for the object to compare
	 */
	static bool ComparePropertyMaps(FName OrigName, TMap<FString, FString>& OrigMap, FName CmpName, FPropertiesMap& CmpMap, FCompilerResultsLog& Results)
	{
		if (OrigMap.Num() != CmpMap.Num())
		{
			Results.Error( *FString::Printf(TEXT("Objects have a different number of properties (%d vs %d)"), OrigMap.Num(), CmpMap.Num()) );
			return false;
		}

		bool bMatch = true;
		for (auto PropIt = OrigMap.CreateIterator(); PropIt; ++PropIt)
		{
			FString Key = PropIt.Key();
			FString Val = PropIt.Value();

			const FString* CmpValue = CmpMap.Find(Key);

			// Value is missing
			if (CmpValue == NULL)
			{
				bMatch = false;
				Results.Error( *FString::Printf(TEXT("Property is missing in object being compared: (%s %s)"), *Key, *Val) );
				break;
			}
			else if (Val != *CmpValue)
			{
				// string out object names and retest
				FString TmpCmp(*CmpValue);
				TmpCmp.ReplaceInline(*CmpName.ToString(), TEXT(""));
				FString TmpVal(Val);
				TmpVal.ReplaceInline(*OrigName.ToString(), TEXT(""));

				if (TmpCmp != TmpVal)
				{
					bMatch = false;
					Results.Error( *FString::Printf(TEXT("Object properties do not match: %s (%s vs %s)"), *Key, *Val, *(*CmpValue)) );
					break;
				}
			}
		}
		return bMatch;
	}

	/** 
	 * Compares the properties of two UObject instances

	 * @param OriginalObj		The first UObject to compare
	 * @param CompareObj		The second UObject to compare
	 * @param Results			The results log to record errors or warnings
	 *
	 * @return True of the blueprints are the same, false otherwise (see the Results log for more details)
	 */
	static bool CompareObjects(UObject* OriginalObj, UObject* CompareObj, FCompilerResultsLog& Results)
	{
		bool bMatch = false;

		// ensure we have something sensible to compare
		if (OriginalObj == NULL)
		{
			Results.Error( TEXT("Original object is null") );
			return false;
		}
		else if (CompareObj == NULL)
		{	
			Results.Error( TEXT("Compare object is null") );
			return false;
		}
		else if (OriginalObj == CompareObj)
		{
			Results.Error( TEXT("Objects to compare are the same") );
			return false;
		}

		TMap<FString, FString> ObjProperties;
		GetObjProperties(OriginalObj, ObjProperties);

		TMap<FString, FString> CmpProperties;
		GetObjProperties(CompareObj, CmpProperties);

		return ComparePropertyMaps(OriginalObj->GetFName(), ObjProperties, CompareObj->GetFName(), CmpProperties, Results);
	}

	/** 
	 * Runs over all the assets looking for ones that can be used by this test
	 *
	 * @param Class					The UClass of assets to load for the test
	 * @param OutBeautifiedNames	[Filled by method] The beautified filenames of the assets to use in the test
	 * @oaram OutTestCommands		[Filled by method] The asset name in a form suitable for passing as a param to the test
	 * @param bIgnoreLoaded			If true, ignore any already loaded assets
	 */
	static void CollectTestsByClass(UClass * Class, TArray< FString >& OutBeautifiedNames, TArray< FString >& OutTestCommands, bool bIgnoreLoaded) 
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), ObjectList);

		for (auto ObjIter=ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			FString Filename = Asset.ObjectPath.ToString();
			//convert to full paths
			Filename = FPackageName::LongPackageNameToFilename(Filename);
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				// optionally discount already loaded assets
				if (!bIgnoreLoaded || !Asset.IsAssetLoaded())
				{
					FString BeautifiedFilename = Asset.AssetName.ToString();
					OutBeautifiedNames.Add(BeautifiedFilename);
					OutTestCommands.Add(Asset.ObjectPath.ToString());
				}
			}
		}
	}

	/**
	 * Adds a package to a list of packages that can no longer be saved.
	 *
	 * @param Package Package to prevent from being saved to disk.
	 */
	static void DontSavePackage(UPackage* Package)
	{
		if (DontSavePackagesList.Num() == 0)
		{
			GIsPackageOKToSaveDelegate.BindStatic(&IsPackageOKToSave);
		}
		DontSavePackagesList.AddUnique(Package->GetFName());
	}

	/**
	 * A helper method that will reset a package for reload, and flag it as 
	 * unsavable (meant to be used after you've messed with a package for testing 
	 * purposes, leaving it in a questionable state).
	 * 
	 * @param  Package	The package you wish to invalidate.
	 */
	static void InvalidatePackage(UPackage* const Package)
	{
		// reset the blueprint's original package/linker so that we can get by
		// any early returns (in the load code), and reload its exports as if new 
		ResetLoaders(Package);
		Package->ClearFlags(RF_WasLoaded);	
		Package->bHasBeenFullyLoaded = false;

		Package->GetMetaData()->RemoveMetaDataOutsidePackage();
		// we've mucked around with the package manually, we should probably prevent 
		// people from saving it in this state (this means you won't be able to save 
		// the blueprints that these tests were run on until you relaunch the editor)
		DontSavePackage(Package);
	}

	/** 
	 * Helper method to unload loaded blueprints. Use with caution.
	 *
	 * @param BlueprintObj	The blueprint to unload
	 * @param bForceFlush   If true, will force garbage collection on everything but native objects (defaults to false)
	 */
	static void UnloadBlueprint(UBlueprint* const BlueprintObj, bool bForceFlush = false)
	{
		// have to grab the blueprint's package before we move it to the transient package
		UPackage* const OldPackage = Cast<UPackage>(BlueprintObj->GetOutermost());

		UPackage* NewPackage = GetTransientPackage();
		// move the blueprint to the transient package (to be picked up by garbage collection later)
		FName UnloadedName = MakeUniqueObjectName(NewPackage, UBlueprint::StaticClass(), BlueprintObj->GetFName());
		BlueprintObj->Rename(*UnloadedName.ToString(), NewPackage, REN_DontCreateRedirectors|REN_DoNotDirty);
		
		// make sure the blueprint is properly trashed so we can rerun tests on it
		BlueprintObj->SetFlags(RF_Transient);
		BlueprintObj->ClearFlags(RF_Standalone | RF_RootSet | RF_Transactional);
		BlueprintObj->RemoveFromRoot();
		BlueprintObj->MarkPendingKill();

		InvalidatePackage(OldPackage);

		// because we just emptied out an existing package, we may want to clean 
		// up garbage so an attempted load doesn't stick us with an invalid asset
		if (bForceFlush)
		{
			CollectGarbage(RF_Native);
		}
	}

	/**
	 * Helper method for checking to see if a blueprint is currently loaded.
	 * 
	 * @param  AssetPath	A path detailing the asset in question (in the form of <PackageName>.<AssetName>)
	 * @return True if a blueprint can be found with a matching package/name, false if no corresponding blueprint was found.
	 */
	static bool IsBlueprintLoaded(FString const& AssetPath)
	{
		bool bIsLoaded = false;

		FString PackageName;
		FString AssetName;
		AssetPath.Split(TEXT("."), &PackageName, &AssetName);

		if (FindPackage(NULL, *PackageName) != NULL)
		{
			UPackage* ExistingPackage = LoadPackage(NULL, *PackageName, LOAD_None);
			if (UBlueprint* ExistingBP = Cast<UBlueprint>(StaticFindObject(UBlueprint::StaticClass(), ExistingPackage, *AssetName)))
			{
				bIsLoaded = true;
			}
		}

		return bIsLoaded;
	}

	/**
	 * Takes two blueprints and compares them (as if we were running the in-editor 
	 * diff tool). Any discrepancies between the two graphs will be listed in the DiffsOut array.
	 * 
	 * @param  LhsBlueprint	The baseline blueprint you wish to compare against.
	 * @param  RhsBlueprint	The blueprint you wish to look for changes in.
	 * @param  DiffsOut		An output list that will contain any graph internal differences that were found.
	 * @return True if the two blueprints differ, false if they are identical.
	 */
	static bool DiffBlueprints(UBlueprint* const LhsBlueprint, UBlueprint* const RhsBlueprint, TArray<FDiffSingleResult>& DiffsOut)
	{
		TArray<UEdGraph*> LhsGraphs;
		LhsBlueprint->GetAllGraphs(LhsGraphs);
		TArray<UEdGraph*> RhsGraphs;
		RhsBlueprint->GetAllGraphs(RhsGraphs);

		bool bDiffsFound = false;
		// walk the graphs in the rhs blueprint (because, conceptually, it is the more up to date one)
		for (auto RhsGraphIt(RhsGraphs.CreateIterator()); RhsGraphIt; ++RhsGraphIt)
		{
			UEdGraph* RhsGraph = *RhsGraphIt;
			UEdGraph* LhsGraph = NULL;

			// search for the corresponding graph in the lhs blueprint
			for (auto LhsGraphIt(LhsGraphs.CreateIterator()); LhsGraphIt; ++LhsGraphIt)
			{
				// can't trust the guid until we've done a resave on every asset
				//if ((*LhsGraphIt)->GraphGuid == RhsGraph->GraphGuid)
				
				// name compares is probably sufficient, but just so we don't always do a string compare
				if (((*LhsGraphIt)->GetClass() == RhsGraph->GetClass()) &&
					((*LhsGraphIt)->GetName() == RhsGraph->GetName()))
				{
					LhsGraph = *LhsGraphIt;
					break;
				}
			}

			// if a matching graph wasn't found in the lhs blueprint, then that is a BIG inconsistency
			if (LhsGraph == NULL)
			{
				bDiffsFound = true;
				continue;
			}

			bDiffsFound |= FGraphDiffControl::DiffGraphs(LhsGraph, RhsGraph, DiffsOut);
		}

		return bDiffsFound;
	}

	/**
	 * Gathers a list of asset files corresponding to a config array (an array 
	 * of package paths).
	 * 
	 * @param  ConfigKey	A key to the config array you want to look up.
	 * @param  AssetsOut	An output array that will be filled with the desired asset data.
	 * @param  ClassType	If specified, will further filter the asset look up by class.
	 */
	static void GetAssetListingFromConfig(FString const& ConfigKey, TArray<FAssetData>& AssetsOut, UClass const* const ClassType = NULL)
	{
		check(GConfig != NULL);

		FARFilter AssetFilter;
		AssetFilter.bRecursivePaths = true;
		if (ClassType != NULL)
		{
			AssetFilter.ClassNames.Add(ClassType->GetFName());
		}
		
		TArray<FString> AssetPaths;
		GConfig->GetArray(TEXT("AutomationTesting.Blueprint"), *ConfigKey, AssetPaths, GEngineIni);
		for (FString const& AssetPath : AssetPaths)
		{
			AssetFilter.PackagePaths.Add(*AssetPath);
		}
		
		if (AssetFilter.PackagePaths.Num() > 0)
		{
			IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
			AssetRegistry.GetAssets(AssetFilter, AssetsOut);
		}
	}

	/**
	 * A utility function for spawning an empty temporary package, meant for test purposes.
	 * 
	 * @param  Name		A suggested string name to get the package (NOTE: it will further be decorated with things like "Temp", etc.) 
	 * @return A newly created package for throwaway use.
	 */
	static UPackage* CreateTempPackage(FString Name)
	{
		FString TempPackageName = FString::Printf(TEXT("/Temp/BpAutomation-%u-%s"), GenTempUid(), *Name);
		return CreatePackage(NULL, *TempPackageName);
	}

	/**
	 * A helper that will take a blueprint and copy it into a new, temporary 
	 * package (intended for throwaway purposes).
	 * 
	 * @param  BlueprintToClone		The blueprint you wish to duplicate.
	 * @return A new temporary blueprint copy of what was passed in.
	 */
	static UBlueprint* DuplicateBlueprint(UBlueprint const* const BlueprintToClone)
	{
		UPackage* TempPackage = CreateTempPackage(BlueprintToClone->GetName());

		FString TempBlueprintName = MakeUniqueObjectName(TempPackage, UBlueprint::StaticClass(), BlueprintToClone->GetFName()).ToString();
		return Cast<UBlueprint>(StaticDuplicateObject(BlueprintToClone, TempPackage, *TempBlueprintName));
	}

	/**
	 * A getter function for coordinating between multiple tests, a place for 
	 * temporary test files to be saved.
	 * 
	 * @return A relative path to a directory meant for temp automation files.
	 */
	static FString GetTempDir()
	{
		return FPaths::GameSavedDir() + TEXT("Automation/");
	}

	/**
	 * Will save a blueprint package under a temp file and report on weather it succeeded or not.
	 * 
	 * @param  BlueprintObj		The blueprint you wish to save.
	 * @return True if the save was successful, false if it failed.
	 */
	static bool TestSaveBlueprint(UBlueprint* const BlueprintObj)
	{
		FString TempDir = GetTempDir();
		IFileManager::Get().MakeDirectory(*TempDir);

		FString SavePath = FString::Printf(TEXT("%sTemp-%u-%s"), *TempDir, GenTempUid(), *FPaths::GetCleanFilename(BlueprintObj->GetName()));

		UPackage* const AssetPackage = Cast<UPackage>(BlueprintObj->GetOutermost());
		return UPackage::SavePackage(AssetPackage, NULL, RF_Standalone, *SavePath, GWarn);
	}
};

uint32 FBlueprintAutomationTestUtilities::QueuedTempId = 0u;
TArray<FName> FBlueprintAutomationTestUtilities::DontSavePackagesList;

/************************************************************************/
/* FBlueprintCompileTest                                                */
/************************************************************************/

/** 
 * Gather the tests to run
 */
void FBlueprintCompileTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	bool bTestLoadedBlueprints = false;
	GConfig->GetBool( TEXT("AutomationTesting.Blueprint"), TEXT("TestAllBlueprints"), /*out*/ bTestLoadedBlueprints, GEngineIni );
	FBlueprintAutomationTestUtilities::CollectTestsByClass(UBlueprint::StaticClass(), OutBeautifiedNames, OutTestCommands, !bTestLoadedBlueprints);
}

/** 
 * Runs compile-on-load test against all unloaded, and optionally loaded, blueprints
 * See the TestAllBlueprints config key in the [Automation.Blueprint] config sections
 */
bool FBlueprintCompileTest::RunTest(const FString& BlueprintAssetPath)
{
	FCompilerResultsLog Results;

	// if this blueprint was already loaded, then these tests are invalidated 
	// (because dependencies have already been loaded)
	if (FBlueprintAutomationTestUtilities::IsBlueprintLoaded(BlueprintAssetPath))
	{
		AddWarning(FString::Printf(TEXT("Invalid test, blueprint has previously been loaded: '%s'"), *BlueprintAssetPath));
	}

	// We load the blueprint twice and compare the two for discrepancies. This is 
	// to bring dependency load issues to light (among other things). If a blueprint's
	// dependencies are loaded too late, then this first object is the degenerate one.
	UBlueprint* InitialBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintAssetPath));

	// if we failed to load it the first time, then there is no need to make a 
	// second attempt, leave them to fix up this issue first
	if (InitialBlueprint == NULL)
	{
		AddError(*FString::Printf(TEXT("Unable to load blueprint for: '%s'"), *BlueprintAssetPath));
		return false;
	}


	// store off data for the initial blueprint so we can unload it (and reconstruct 
	// later to compare it with a second one)
	TArray<uint8> InitialLoadData;
	FObjectWriter(InitialBlueprint, InitialLoadData);

	// grab the name before we unload the blueprint
	FName const BlueprintName = InitialBlueprint->GetFName();
	// unload the blueprint so we can reload it (to catch any differences, now  
	// that all its dependencies should be loaded as well)
	FBlueprintAutomationTestUtilities::UnloadBlueprint(InitialBlueprint);
	// this blueprint is now invalid
	InitialBlueprint = NULL;

	// load the blueprint a second time; if the two separately loaded blueprints 
	// are different, then this one is most likely the choice one (it has all its 
	// dependencies loaded)
	UBlueprint* ReloadedBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintAssetPath));

	UPackage* TransientPackage = GetTransientPackage();
	FName ReconstructedName = MakeUniqueObjectName(TransientPackage, UBlueprint::StaticClass(), BlueprintName);
	// reconstruct the initial blueprint (using the serialized data from its initial load)
	EObjectFlags const StandardBlueprintFlags = RF_Public | RF_Standalone | RF_Transactional;
	InitialBlueprint = ConstructObject<UBlueprint>(UBlueprint::StaticClass(), TransientPackage, ReconstructedName, StandardBlueprintFlags | RF_Transient);
	FObjectReader(InitialBlueprint, InitialLoadData);

	// look for diffs between subsequent loads and log them as errors
	TArray<FDiffSingleResult> BlueprintDiffs;
	bool bDiffsFound = FBlueprintAutomationTestUtilities::DiffBlueprints(InitialBlueprint, ReloadedBlueprint, BlueprintDiffs);
	if (bDiffsFound)
	{
		AddError(FString::Printf(TEXT("Inconsistencies between subsequent blueprint loads for: '%s' (was a dependency not preloaded?)"), *BlueprintAssetPath));
		// list all the differences (so as to help identify what dependency was missing)
		for (auto DiffIt(BlueprintDiffs.CreateIterator()); DiffIt; ++DiffIt)
		{
			// will be presented in the context of "what changed between the initial load and the second?"
			FString DiffDescription = DiffIt->ToolTip;
			if (DiffDescription != DiffIt->DisplayString)
			{
				DiffDescription = FString::Printf(TEXT("%s (%s)"), *DiffDescription, *DiffIt->DisplayString);
			}

			FString const GraphName = DiffIt->Node1->GetGraph()->GetName();
			AddError(FString::Printf(TEXT("%s.%s differs between subsequent loads: %s"), *BlueprintName.ToString(), *GraphName, *DiffDescription));
		}
	}

	FBlueprintAutomationTestUtilities::UnloadBlueprint(ReloadedBlueprint);
	FBlueprintAutomationTestUtilities::UnloadBlueprint(InitialBlueprint);
	// run garbage collection, in hopes that the imports for this blueprint get destroyed
	// so that they don't invalidate other tests that share the same dependencies
	CollectGarbage(RF_Native);

	return !bDiffsFound;
}


/************************************************************************/
/* FBlueprintInstancesTest                                              */
/************************************************************************/

void FBlueprintInstancesTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	// Load the test maps
	check( GConfig );

	// Load from config file
	TArray<FString> MapsToLoad;
	GConfig->GetArray( TEXT("AutomationTesting.Blueprint"), TEXT("InstanceTestMaps"), MapsToLoad, GEngineIni);
	for (auto It = MapsToLoad.CreateConstIterator(); It; ++It)
	{
		const FString MapFileName = *It;
		if( IFileManager::Get().FileSize( *MapFileName ) > 0 )
		{
			OutBeautifiedNames.Add(FPaths::GetBaseFilename(MapFileName));
			OutTestCommands.Add(MapFileName);
		}
	}
}

/**
  * Wait for the given amount of time
  */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDelayLatentCommand, float, Duration);

bool FDelayLatentCommand::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}

/************************************************************************/
/*
 * Uses test maps in Engine and/or game content folder which are populated with a few blueprint instances
 * See InstanceTestMaps entries in the [Automation.Blueprint] config sections
 * For all blueprint instances in the map:
 *		Duplicates the instance 
 *		Compares the duplicated instance properties to the original instance properties
 */
bool FBlueprintInstancesTest::RunTest(const FString& InParameters)
{
	FBlueprintAutomationTestUtilities::LoadMap(InParameters);

	// Pause before running test
	ADD_LATENT_AUTOMATION_COMMAND(FDelayLatentCommand(2.f));

	// Grab BP instances from map
	TSet<AActor*> BlueprintInstances;
	for ( FActorIterator It(GWorld); It; ++It )
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			UClass* ActorClass = Actor->GetClass();

			if (ActorClass->ClassGeneratedBy && ActorClass->ClassGeneratedBy->IsA( UBlueprint::StaticClass() ) )
			{
				BlueprintInstances.Add(Actor);
			}
		}
	}

	bool bPropertiesMatch = true;
	FCompilerResultsLog ResultLog;
	TSet<UPackage*> PackagesUserRefusedToFullyLoad;
	ObjectTools::FPackageGroupName PGN;

	for (auto BpIter = BlueprintInstances.CreateIterator(); BpIter; ++BpIter )
	{
		AActor* BPInstance = *BpIter;
		UObject* BPInstanceOuter = BPInstance ? BPInstance->GetOuter() : NULL;

		TMap<FString,FString> BPNativePropertyValues;
		BPInstance->GetNativePropertyValues(BPNativePropertyValues);

		// Grab the package and save out its dirty state
		UPackage* ActorPackage = BPInstance->GetOutermost();
		FBlueprintAutomationTestUtilities::FPackageCleaner Cleaner(ActorPackage);

		// Use this when duplicating the object to keep a list of everything that was duplicated
		//TMap<UObject*, UObject*> DuplicatedObjectList;

		FObjectDuplicationParameters Parameters(BPInstance, BPInstanceOuter);
		//Parameters.CreatedObjects = &DuplicatedObjectList;
		Parameters.DestName = MakeUniqueObjectName( BPInstanceOuter, AActor::StaticClass(), BPInstance->GetFName() );

		// Duplicate the object
		AActor* ClonedInstance = Cast<AActor>(StaticDuplicateObjectEx(Parameters));

		if (!FBlueprintAutomationTestUtilities::CompareObjects(BPInstance, ClonedInstance, ResultLog))
		{
			bPropertiesMatch = false;
			break;
		}

		// Ensure we can't save package in editor
		FBlueprintAutomationTestUtilities::DontSavePackage(ActorPackage);
	}

	// Start a new map for now
	// @todo find a way return to previous map thats a 100% reliably
	GEditor->CreateNewMapForEditing();

	ADD_LATENT_AUTOMATION_COMMAND(FDelayLatentCommand(2.f));

	return bPropertiesMatch;
}

/*******************************************************************************
* FBlueprintReparentTest
*******************************************************************************/

void FBlueprintReparentTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	TArray<FAssetData> Assets;
	FBlueprintAutomationTestUtilities::GetAssetListingFromConfig(TEXT("ReparentTest.ChildrenPackagePaths"), Assets, UBlueprint::StaticClass());

	for (FAssetData const& AssetData : Assets)
	{
		OutBeautifiedNames.Add(AssetData.AssetName.ToString());
		OutTestCommands.Add(AssetData.ObjectPath.ToString());
	}
}

bool FBlueprintReparentTest::RunTest(const FString& BlueprintAssetPath)
{
	bool bTestFailed = false;

	UBlueprint const* const BlueprintTemplate = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintAssetPath));
	if (BlueprintTemplate != NULL)
	{
		// want to explicitly test switching from actors->objects, and vise versa (objects->actors), 
		// also could cover the case of changing non-native parents to native ones
		TArray<UClass*> TestParentClasses;
		if (!BlueprintTemplate->ParentClass->IsChildOf(AActor::StaticClass()))
		{
			TestParentClasses.Add(AActor::StaticClass());
		}
		else
		{
			// not many engine level Blueprintable classes that aren't Actors
			TestParentClasses.Add(USaveGame::StaticClass());
		}

		TArray<FAssetData> Assets;
		FBlueprintAutomationTestUtilities::GetAssetListingFromConfig(TEXT("ReparentTest.ParentsPackagePaths"), Assets, UBlueprint::StaticClass());
		// additionally gather up any blueprints that we explicitly specify though the config
		for (FAssetData const& AssetData : Assets)
		{
			UClass* AssetClass = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());
			TestParentClasses.Add(AssetClass);
		}

		for (UClass* Class : TestParentClasses)
		{		 
			UBlueprint* BlueprintObj = FBlueprintAutomationTestUtilities::DuplicateBlueprint(BlueprintTemplate);
			check(BlueprintObj != NULL);
			BlueprintObj->ParentClass = Class;

			if (!FBlueprintAutomationTestUtilities::TestSaveBlueprint(BlueprintObj))
			{
				AddError(FString::Printf(TEXT("Failed to save blueprint after reparenting with %s: '%s'"), *Class->GetName(), *BlueprintAssetPath));
				bTestFailed = true;
			}

			FBlueprintAutomationTestUtilities::UnloadBlueprint(BlueprintObj, /* bForceFlush =*/true);
		}
	}

	return !bTestFailed;
}

/*******************************************************************************
* FBlueprintRenameTest
*******************************************************************************/

// void FBlueprintRenameAndCloneTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
// {
// 	FBlueprintAutomationTestUtilities::CollectTestsByClass(UBlueprint::StaticClass(), OutBeautifiedNames, OutTestCommands, /* bIgnoreLoaded =*/false);
// }
// 
// bool FBlueprintRenameAndCloneTest::RunTest(const FString& BlueprintAssetPath)
// {
// 	bool bTestFailed = false;
// 
// 	UBlueprint* const OriginalBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintAssetPath));
// 	if (OriginalBlueprint != NULL)
// 	{
// 		// duplicate
// 		{
// 			UBlueprint* DuplicateBluprint = FBlueprintAutomationTestUtilities::DuplicateBlueprint(OriginalBlueprint);
// 			if (!FBlueprintAutomationTestUtilities::TestSaveBlueprint(DuplicateBluprint))
// 			{
// 				AddError(FString::Printf(TEXT("Failed to save blueprint after duplication: '%s'"), *BlueprintAssetPath));
// 				bTestFailed = true;
// 			}
// 			FBlueprintAutomationTestUtilities::UnloadBlueprint(DuplicateBluprint);
// 		}
// 
// 		// rename
// 		{
// 			// store the original package so we can manually invalidate it after the move
// 			UPackage* const OriginalPackage = Cast<UPackage>(OriginalBlueprint->GetOutermost());
// 
// 			FString const BlueprintName = OriginalBlueprint->GetName();
// 			UPackage* TempPackage = FBlueprintAutomationTestUtilities::CreateTempPackage(BlueprintName);
// 
// 			FString NewName = FString::Printf(TEXT("%s-Rename"), *BlueprintName);
// 			NewName = MakeUniqueObjectName(TempPackage, OriginalBlueprint->GetClass(), *NewName).ToString();
// 
// 			OriginalBlueprint->Rename(*NewName, TempPackage, REN_None);
// 
// 			if (!FBlueprintAutomationTestUtilities::TestSaveBlueprint(OriginalBlueprint))
// 			{
// 				AddError(FString::Printf(TEXT("Failed to save blueprint after rename: '%s'"), *BlueprintAssetPath));
// 				bTestFailed = true;
// 			}
// 
// 			// the blueprint has been moved out of this package, invalidate it so 
// 			// we don't save it in this state and so we can reload the blueprint later
// 			FBlueprintAutomationTestUtilities::InvalidatePackage(OriginalPackage);
// 		}
// 
// 		FBlueprintAutomationTestUtilities::UnloadBlueprint(OriginalBlueprint);
// 		// make sure the unloaded blueprints are properly flushed (for future tests)
// 		CollectGarbage(RF_Native);
// 	}
// 
// 	return !bTestFailed;
// }
