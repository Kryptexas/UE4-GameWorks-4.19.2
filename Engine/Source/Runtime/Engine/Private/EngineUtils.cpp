// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineUtils.h"

#include "DiagnosticTable.h"
#include "TargetPlatform.h"
#include "EngineUserInterfaceClasses.h"
#include "SoundDefinitions.h"

DEFINE_LOG_CATEGORY_STATIC(LogEngineUtils, Log, All);

IMPLEMENT_HIT_PROXY(HActor,HHitProxy)
IMPLEMENT_HIT_PROXY(HBSPBrushVert,HHitProxy);
IMPLEMENT_HIT_PROXY(HStaticMeshVert,HHitProxy);
IMPLEMENT_HIT_PROXY(HSplineProxy,HHitProxy);
IMPLEMENT_HIT_PROXY(HTranslucentActor,HActor)


#if !UE_BUILD_SHIPPING
FContentComparisonHelper::FContentComparisonHelper()
{
	FConfigSection* RefTypes = GConfig->GetSectionPrivate(TEXT("ContentComparisonReferenceTypes"), false, true, GEngineIni);
	if (RefTypes != NULL)
	{
		for( FConfigSectionMap::TIterator It(*RefTypes); It; ++It )
		{
			const FString RefType = It.Value();
			ReferenceClassesOfInterest.Add(RefType, true);
			UE_LOG(LogEngineUtils, Log, TEXT("Adding class of interest: %s"), *RefType);
		}
	}
}

FContentComparisonHelper::~FContentComparisonHelper()
{
}

bool FContentComparisonHelper::CompareClasses(const FString& InBaseClassName, int32 InRecursionDepth)
{
	TArray<FString> EmptyIgnoreList;
	return CompareClasses(InBaseClassName, EmptyIgnoreList, InRecursionDepth);
}

bool FContentComparisonHelper::CompareClasses(const FString& InBaseClassName, const TArray<FString>& InBaseClassesToIgnore, int32 InRecursionDepth)
{
	TMap<FString,TArray<FContentComparisonAssetInfo> > ClassToAssetsMap;

	UClass* TheClass = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *InBaseClassName, true);
	if (TheClass != NULL)
	{
		TArray<UClass*> IgnoreBaseClasses;
		for (int32 IgnoreIdx = 0; IgnoreIdx < InBaseClassesToIgnore.Num(); IgnoreIdx++)
		{
			UClass* IgnoreClass = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *(InBaseClassesToIgnore[IgnoreIdx]), true);
			if (IgnoreClass != NULL)
			{
				IgnoreBaseClasses.Add(IgnoreClass);
			}
		}

		for( TObjectIterator<UClass> It; It; ++It )
		{
			UClass* TheAssetClass = *It;
			if ((TheAssetClass->IsChildOf(TheClass) == true) && 
				(TheAssetClass->HasAnyClassFlags(CLASS_Abstract) == false))
			{
				bool bSkipIt = false;
				for (int32 CheckIdx = 0; CheckIdx < IgnoreBaseClasses.Num(); CheckIdx++)
				{
					UClass* CheckClass = IgnoreBaseClasses[CheckIdx];
					if (TheAssetClass->IsChildOf(CheckClass) == true)
					{
// 						UE_LOG(LogEngineUtils, Warning, TEXT("Skipping class derived from other content comparison class..."));
// 						UE_LOG(LogEngineUtils, Warning, TEXT("\t%s derived from %s"), *TheAssetClass->GetFullName(), *CheckClass->GetFullName());
						bSkipIt = true;
					}
				}
				if (bSkipIt == false)
				{
					TArray<FContentComparisonAssetInfo>* AssetList = ClassToAssetsMap.Find(TheAssetClass->GetFullName());
					if (AssetList == NULL)
					{
						TArray<FContentComparisonAssetInfo> TempAssetList;
						ClassToAssetsMap.Add(TheAssetClass->GetFullName(), TempAssetList);
						AssetList = ClassToAssetsMap.Find(TheAssetClass->GetFullName());
					}
					check(AssetList);

					// Serialize object with reference collector.
					const int32 MaxRecursionDepth = 6;
					InRecursionDepth = FMath::Clamp<int32>(InRecursionDepth, 1, MaxRecursionDepth);
					TMap<UObject*,bool> RecursivelyGatheredReferences;
					RecursiveObjectCollection(TheAssetClass, 0, InRecursionDepth, RecursivelyGatheredReferences);

					// Add them to the asset list
					for (TMap<UObject*,bool>::TIterator GatheredIt(RecursivelyGatheredReferences); GatheredIt; ++GatheredIt)
					{
						UObject* Object = GatheredIt.Key();
						if (Object)
						{
							bool bAddIt = true;
							if (ReferenceClassesOfInterest.Num() > 0)
							{
								FString CheckClassName = Object->GetClass()->GetName();
								if (ReferenceClassesOfInterest.Find(CheckClassName) == NULL)
								{
									bAddIt = false;
								}
							}
							if (bAddIt == true)
							{
								int32 NewIndex = AssetList->AddZeroed();
								FContentComparisonAssetInfo& Info = (*AssetList)[NewIndex];
								Info.AssetName = Object->GetFullName();
								Info.ResourceSize = Object->GetResourceSize(EResourceSizeMode::Inclusive);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogEngineUtils, Warning, TEXT("Failed to find class: %s"), *InBaseClassName);
		return false;
	}

#if 0
	// Log them all out
	UE_LOG(LogEngineUtils, Log, TEXT("CompareClasses on %s"), *InBaseClassName);
	for (TMap<FString,TArray<FContentComparisonAssetInfo>>::TIterator It(ClassToAssetsMap); It; ++It)
	{
		FString ClassName = It.Key();
		TArray<FContentComparisonAssetInfo>& AssetList = It.Value();

		UE_LOG(LogEngineUtils, Log, TEXT("\t%s"), *ClassName);
		for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); AssetIdx++)
		{
			FContentComparisonAssetInfo& Info = AssetList(AssetIdx);

			UE_LOG(LogEngineUtils, Log, TEXT("\t\t%s,%f"), *(Info.AssetName), Info.ResourceSize/1024.0f);
		}
	}
#endif

#if ALLOW_DEBUG_FILES
	// Write out a CSV file
	FString CurrentTime = FDateTime::Now().ToString();
	FString Platform(FPlatformProperties::PlatformName());

	FString BaseCSVName = (
		FString(TEXT("ContentComparison/")) + 
		FString::Printf(TEXT("ContentCompare-%s/"), *GEngineVersion.ToString()) +
		FString::Printf(TEXT("%s"), *InBaseClassName)
		);

	// Handle file name length on consoles... 
	FString EditedBaseClassName = InBaseClassName;
	FString TimeString = *FDateTime::Now().ToString();
	FString CheckLenName = FString::Printf(TEXT("%s-%s.csv"),*InBaseClassName,*TimeString);
	if (CheckLenName.Len() > PLATFORM_MAX_FILEPATH_LENGTH)
	{
		while (CheckLenName.Len() > PLATFORM_MAX_FILEPATH_LENGTH)
		{
			EditedBaseClassName = EditedBaseClassName.Right(EditedBaseClassName.Len() - 1);
			CheckLenName = FString::Printf(TEXT("%s-%s.csv"),*EditedBaseClassName,*TimeString);
		}
		BaseCSVName = (
			FString(TEXT("ContentComparison/")) + 
			FString::Printf(TEXT("ContentCompare-%s/"), *GEngineVersion.ToString()) +
			FString::Printf(TEXT("%s"), *EditedBaseClassName)
			);
	}

	FDiagnosticTableViewer* AssetTable = new FDiagnosticTableViewer(
			*FDiagnosticTableViewer::GetUniqueTemporaryFilePath(*BaseCSVName), true);
	if ((AssetTable != NULL) && (AssetTable->OutputStreamIsValid() == true))
	{
		// Fill in the header row
		AssetTable->AddColumn(TEXT("Class"));
		AssetTable->AddColumn(TEXT("Asset"));
		AssetTable->AddColumn(TEXT("ResourceSize(kB)"));
		AssetTable->CycleRow();

		// Fill it in
		for (TMap<FString,TArray<FContentComparisonAssetInfo> >::TIterator It(ClassToAssetsMap); It; ++It)
		{
			FString ClassName = It.Key();
			TArray<FContentComparisonAssetInfo>& AssetList = It.Value();

			AssetTable->AddColumn(*ClassName);
			AssetTable->CycleRow();
			for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); AssetIdx++)
			{
				FContentComparisonAssetInfo& Info = AssetList[AssetIdx];

				AssetTable->AddColumn(TEXT(""));
				AssetTable->AddColumn(*(Info.AssetName));
				AssetTable->AddColumn(TEXT("%f"), Info.ResourceSize/1024.0f);
				AssetTable->CycleRow();
			}
		}
	}
	else if (AssetTable != NULL)
	{
		// Created the class, but it failed to open the output stream.
		UE_LOG(LogEngineUtils, Warning, TEXT("Failed to open output stream in asset table!"));
	}

	if (AssetTable != NULL)
	{
		// Close it and kill it
		AssetTable->Close();
		delete AssetTable;
	}
#endif

	return true;
}

void FContentComparisonHelper::RecursiveObjectCollection(UObject* InStartObject, int32 InCurrDepth, int32 InMaxDepth, TMap<UObject*,bool>& OutCollectedReferences)
{
	// Serialize object with reference collector.
	TArray<UObject*> LocalCollectedReferences;
	FReferenceFinder ObjectReferenceCollector( LocalCollectedReferences, NULL, false, true, true, true );
	ObjectReferenceCollector.FindReferences( InStartObject );

	if (InCurrDepth < InMaxDepth)
	{
		InCurrDepth++;
		for (int32 ObjRefIdx = 0; ObjRefIdx < LocalCollectedReferences.Num(); ObjRefIdx++)
		{
			UObject* InnerObject = LocalCollectedReferences[ObjRefIdx];
			if ((InnerObject != NULL) &&
				(InnerObject->IsA(UFunction::StaticClass()) == false) &&
				(InnerObject->IsA(UPackage::StaticClass()) == false)
				)
			{
				OutCollectedReferences.Add(InnerObject, true);
				RecursiveObjectCollection(InnerObject, InCurrDepth, InMaxDepth, OutCollectedReferences);
			}
		}
		InCurrDepth--;
	}
}
#endif

bool EngineUtils::FindOrLoadAssetsByPath(const FString& Path, TArray<UObject*>& OutAssets)
{
	if ( !FPackageName::IsValidLongPackageName(Path, true) )
	{
		return false;
	}

	// Convert the package path to a filename with no extension (directory)
	const FString FilePath = FPackageName::LongPackageNameToFilename(Path);

	// Gather the package files in that directory and subdirectories
	TArray<FString> Filenames;
	FPackageName::FindPackagesInDirectory(Filenames, FilePath);

	// Cull out map files
	for (int32 FilenameIdx = Filenames.Num() - 1; FilenameIdx >= 0; --FilenameIdx)
	{
		const FString Extension = FPaths::GetExtension(Filenames[FilenameIdx], true);
		if ( Extension == FPackageName::GetMapPackageExtension() )
		{
			Filenames.RemoveAt(FilenameIdx);
		}
	}

	// Load packages or find existing ones and fully load them
	TSet<UPackage*> Packages;
	for (int32 FileIdx = 0; FileIdx < Filenames.Num(); ++FileIdx)
	{
		const FString& Filename = Filenames[FileIdx];

		UPackage* Package = FindPackage(NULL, *FPackageName::FilenameToLongPackageName(Filename));

		if (Package)
		{
			Package->FullyLoad();
		}
		else
		{
			Package = LoadPackage(NULL, *Filename, LOAD_None);
		}

		if (Package)
		{
			Packages.Add(Package);
		}
	}

	// If any packages were successfully loaded, find all assets that were in the packages and add them to OutAssets
	if ( Packages.Num() > 0 )
	{
		for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
		{
			if ( Packages.Contains(ObjIt->GetOutermost()) && ObjIt->IsAsset() )
			{
				OutAssets.Add(*ObjIt);
			}
		}
	}

	return true;
}


FString EngineUtils::SanitizeDisplayName( const FString& InDisplayName, const bool bIsBool )
{
	// Copy the characters out so that we can modify the string in place
	const TArray< TCHAR > Chars = InDisplayName.GetCharArray();

	// This is used to indicate that we are in a run of uppercase letter and/or digits.  The code attempts to keep
	// these characters together as breaking them up often looks silly (i.e. "Draw Scale 3 D" as opposed to "Draw Scale 3D"
	bool bInARun = false;
	bool bWasSpace = false;
	bool bWasOpenParen = false;
	FString OutDisplayName;
	for( int32 CharIndex = 0 ; CharIndex < Chars.Num() ; ++CharIndex )
	{
		TCHAR ch = Chars[CharIndex];

		bool bLowerCase = FChar::IsLower( ch );
		bool bUpperCase = FChar::IsUpper( ch );
		bool bIsDigit = FChar::IsDigit( ch );
		bool bIsUnderscore = FChar::IsUnderscore( ch );

		// Skip the first character if the property is a bool (they should all start with a lowercase 'b', which we don't want to keep)
		if( CharIndex == 0 && bIsBool && ch == 'b' )
		{
			// Check if next character is uppercase as it may be a user created string that doesn't follow the rules of Unreal variables
			if (Chars.Num() > 1 && FChar::IsUpper(Chars[1]))
			{
				continue;
			}
		}

		// If the current character is upper case or a digit, and the previous character wasn't, then we need to insert a space if there wasn't one previously
		if( (bUpperCase || bIsDigit) && !bInARun && !bWasOpenParen)
		{
			if( !bWasSpace && OutDisplayName.Len() > 0 )
			{
				OutDisplayName += TEXT( ' ' );
				bWasSpace = true;
			}
			bInARun = true;
		}

		// A lower case character will break a run of upper case letters and/or digits
		if( bLowerCase )
		{
			bInARun = false;
		}

		// An underscore denotes a space, so replace it and continue the run
		if( bIsUnderscore )
		{
			ch = TEXT( ' ' );
			bInARun = true;
		}

		// If this is the first character in the string, then it will always be upper-case
		if( OutDisplayName.Len() == 0 )
		{
			ch = FChar::ToUpper( ch );
		}
		else if( bWasSpace || bWasOpenParen)	// If this is first character after a space, then make sure it is case-correct
		{
			// Some words are always forced lowercase
			const TCHAR* Articles[] =
			{
				TEXT( "In" ),
				TEXT( "As" ),
				TEXT( "To" ),
				TEXT( "Or" ),
				TEXT( "At" ),
				TEXT( "On" ),
				TEXT( "If" ),
				TEXT( "Be" ),
				TEXT( "By" ),
				TEXT( "The" ),
				TEXT( "For" ),
				TEXT( "And" ),
				TEXT( "With" ),
				TEXT( "When" ),
				TEXT( "From" ),
			};

			// Search for a word that needs case repaired
			bool bIsArticle = false;
			for( int32 CurArticleIndex = 0; CurArticleIndex < ARRAY_COUNT( Articles ); ++CurArticleIndex )
			{
				// Make sure the character following the string we're testing is not lowercase (we don't want to match "in" with "instance")
				const int32 ArticleLength = FCString::Strlen( Articles[ CurArticleIndex ] );
				if( ( Chars.Num() - CharIndex ) > ArticleLength && !FChar::IsLower( Chars[ CharIndex + ArticleLength ] ) && Chars[ CharIndex + ArticleLength ] != '\0' )
				{
					// Does this match the current article?
					if( FCString::Strncmp( &Chars[ CharIndex ], Articles[ CurArticleIndex ], ArticleLength ) == 0 )
					{
						bIsArticle = true;
						break;
					}
				}
			}

			// Start of a keyword, force to lowercase
			if( bIsArticle )
			{
				ch = FChar::ToLower( ch );				
			}
			else	// First character after a space that's not a reserved keyword, make sure it's uppercase
			{
				ch = FChar::ToUpper( ch );
			}
		}

		bWasSpace = ( ch == TEXT( ' ' ) ? true : false );
		bWasOpenParen = ( ch == TEXT( '(' ) ? true : false );

		OutDisplayName += ch;
	}

	return OutDisplayName;
}


/** This will set the StreamingLevels TMap with the current Streaming Level Status and also set which level the player is in **/
void GetLevelStreamingStatus( UWorld* World, TMap<FName,int32>& StreamingLevels, FString& LevelPlayerIsInName )
{
	FWorldContext &Context = GEngine->WorldContextFromWorld(World);

	// Iterate over the world info's level streaming objects to find and see whether levels are loaded, visible or neither.
	for( int32 LevelIndex=0; LevelIndex<World->StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreaming* LevelStreaming = World->StreamingLevels[LevelIndex];

		if( LevelStreaming 
			&&  LevelStreaming->PackageName != NAME_None 
			&&	LevelStreaming->PackageName != World->GetOutermost()->GetFName() )
		{
			ULevel* Level = LevelStreaming->GetLoadedLevel();
			if( Level != NULL )
			{
				if( World->ContainsLevel( Level ) == true )
				{
					if( World->CurrentLevelPendingVisibility == Level )
					{
						StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_MakingVisible );
					}
					else
					{
						StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_Visible );
					}
				}
				else
				{
					StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_Loaded );
				}
			}
			else
			{
				// See whether the level's world object is still around.
				UPackage* LevelPackage	= Cast<UPackage>(StaticFindObjectFast( UPackage::StaticClass(), NULL, LevelStreaming->PackageName ));
				UWorld*	  LevelWorld	= NULL;
				if( LevelPackage )
				{
					LevelWorld = UWorld::FindWorldInPackage(LevelPackage);
				}

				if( LevelWorld )
				{
					StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_UnloadedButStillAround );
				}
				else if( GetAsyncLoadPercentage( *LevelStreaming->PackageName.ToString() ) >= 0 )
				{
					StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_Loading );
				}
				else
				{
					StreamingLevels.Add( LevelStreaming->PackageName, LEVEL_Unloaded );
				}
			}
		}
	}

	
	// toss in the levels being loaded by PrepareMapChange
	for( int32 LevelIndex=0; LevelIndex < Context.LevelsToLoadForPendingMapChange.Num(); LevelIndex++ )
	{
		const FName LevelName = Context.LevelsToLoadForPendingMapChange[LevelIndex];
		StreamingLevels.Add(LevelName, LEVEL_Preloading);
	}


	ULevel* LevelPlayerIsIn = NULL;

	for( FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;

		if( PlayerController->GetPawn() != NULL )
		{
			// need to do a trace down here
			//TraceActor = Trace( out_HitLocation, out_HitNormal, TraceDest, TraceStart, false, TraceExtent, HitInfo, true );
			FHitResult Hit(1.f);

			// this will not work for flying around :-(
			static FName NAME_FindLevel = FName(TEXT("FindLevel"), true);			
			PlayerController->GetWorld()->LineTraceSingle(Hit,PlayerController->GetPawn()->GetActorLocation(), (PlayerController->GetPawn()->GetActorLocation()-FVector(0.f, 0.f, 256.f)), FCollisionQueryParams(NAME_FindLevel, true, PlayerController->GetPawn()), FCollisionObjectQueryParams(ECC_WorldStatic));

			/** @todo UE4 FIXME
			if( Hit.Level != NULL )
			{
				LevelPlayerIsIn = Hit.Level;
			}
			else 
			*/
			if( Hit.GetActor() != NULL )
			{
				LevelPlayerIsIn = Hit.GetActor()->GetLevel();
			}
			else if( Hit.Component != NULL && Hit.Component->GetOwner() != NULL )
			{
				AActor* Owner = Hit.Component->GetOwner();
				if (Owner)
				{
					LevelPlayerIsIn = Owner->GetLevel();
				}
				else
				{
					// This happens for BSP where the ModelComponent's outer is the level
					LevelPlayerIsIn = Hit.Component->GetTypedOuter<ULevel>();
				}
			}
		}
	}

	// this no longer seems to be getting the correct level name :-(
	LevelPlayerIsInName = LevelPlayerIsIn != NULL ? LevelPlayerIsIn->GetOutermost()->GetName() : TEXT("None");
}

//////////////////////////////////////////////////////////////////////////
// FConsoleOutputDevice

void FConsoleOutputDevice::Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	FStringOutputDevice::Serialize(Text, Verbosity, Category);
	FStringOutputDevice::Serialize(TEXT("\n"), Verbosity, Category);
	GLog->Serialize(Text, Verbosity, Category);

	if( Console != NULL )
	{
		bool bLogToConsole = true;

		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("con.MinLogVerbosity"));

		if(CVar)
		{
			int MinVerbosity = CVar->GetValueOnGameThread();

			if((int)Verbosity <= MinVerbosity)
			{
				// in case the cvar is used we don't need this printout (avoid double print)
				bLogToConsole = false;
			}
		}

		if(bLogToConsole)
		{
			Console->OutputText(Text);
		}
	}
}

/*-----------------------------------------------------------------------------
	Serialized data stripping.
-----------------------------------------------------------------------------*/
FStripDataFlags::FStripDataFlags( class FArchive& Ar, uint8 InClassFlags /*= 0*/, int32 InVersion /*= VER_UE4_REMOVED_STRIP_DATA */ )
	: GlobalStripFlags( 0 )
	, ClassStripFlags( 0 )
{
	check(InVersion >= VER_UE4_REMOVED_STRIP_DATA);
	if (Ar.UE4Ver() >= InVersion)
	{
		if (Ar.IsCooking())
		{
			// When cooking GlobalStripFlags are automatically generated based on the current target
			// platform's properties.
			GlobalStripFlags |= Ar.CookingTarget()->HasEditorOnlyData() ? FStripDataFlags::None : FStripDataFlags::Editor;
			GlobalStripFlags |= Ar.CookingTarget()->IsServerOnly() ? FStripDataFlags::Server : FStripDataFlags::None;
			ClassStripFlags = InClassFlags;
		}
		Ar << GlobalStripFlags;
		Ar << ClassStripFlags;
	}
}

FStripDataFlags::FStripDataFlags( class FArchive& Ar, uint8 InGlobalFlags, uint8 InClassFlags, int32 InVersion /*= VER_UE4_REMOVED_STRIP_DATA */ )
	: GlobalStripFlags( 0 )
	, ClassStripFlags( 0 )
{
	check(InVersion >= VER_UE4_REMOVED_STRIP_DATA);
	if (Ar.UE4Ver() >= InVersion)
	{
		if (Ar.IsCooking())
		{
			// Don't generate global strip flags and use the ones passed in by the caller.
			GlobalStripFlags = InGlobalFlags;
			ClassStripFlags = InClassFlags;
		}
		Ar << GlobalStripFlags;
		Ar << ClassStripFlags;
	}
}