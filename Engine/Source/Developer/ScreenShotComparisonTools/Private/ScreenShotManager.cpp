// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ScreenShotComparisonToolsPrivatePCH.h"

#include "JsonObjectConverter.h"
#include "ImageComparer.h"

class FScreenshotComparisons
{
public:
	FString ApprovedFolder;
	FString UnapprovedFolder;

	TArray<FString> Existing;
	TArray<FString> New;
	TArray<FString> Missing;
};

FScreenShotManager::FScreenShotManager( const IMessageBusRef& InMessageBus )
{
	MessageEndpoint = FMessageEndpoint::Builder("FScreenShotToolsModule", InMessageBus)
		.Handling<FAutomationWorkerScreenImage>(this, &FScreenShotManager::HandleScreenShotMessage);

	if (MessageEndpoint.IsValid())
	{
		MessageEndpoint->Subscribe<FAutomationWorkerScreenImage>();
	}

	ScreenshotApprovedFolder = FPaths::GameDir() / TEXT("Test/Screenshots/");
	ScreenshotIncomingFolder = FPaths::GameSavedDir() / TEXT("Automation/Incoming/");
	ScreenshotDeltaFolder = FPaths::GameSavedDir() / TEXT("Automation/Delta/");
	ScreenshotResultsFolder = FPaths::GameSavedDir() / TEXT("Automation/");
}

TSharedRef<FScreenshotComparisons> FScreenShotManager::GenerateFileList() const
{
	TSharedRef<FScreenshotComparisons> Comparisons = MakeShareable(new FScreenshotComparisons());

	Comparisons->ApprovedFolder = ScreenshotApprovedFolder;
	Comparisons->UnapprovedFolder = ScreenshotIncomingFolder;

	TArray<FString> UnapprovedShots;
	TArray<FString> ApprovedShots;

	{
		TArray<FString> UnapprovedShotList;
		IFileManager::Get().FindFilesRecursive(UnapprovedShotList, *ScreenshotIncomingFolder, TEXT("*.png"), true, false);

		TArray<FString> ApprovedShotList;
		IFileManager::Get().FindFilesRecursive(ApprovedShotList, *ScreenshotApprovedFolder, TEXT("*.png"), true, false);

		for ( FString UnapprovedPngFile : UnapprovedShotList )
		{
			FPaths::MakePathRelativeTo(UnapprovedPngFile, *ScreenshotIncomingFolder);
			UnapprovedShots.AddUnique(FPaths::GetPath(UnapprovedPngFile));
		}

		for ( FString ApprovedPngFile : ApprovedShotList )
		{
			FPaths::MakePathRelativeTo(ApprovedPngFile, *ScreenshotApprovedFolder);
			ApprovedShots.AddUnique(FPaths::GetPath(ApprovedPngFile));
		}
	}

	for ( int32 ShotIndex = 0; ShotIndex < UnapprovedShots.Num(); ShotIndex++ )
	{
		const FString& UnapprovedShot = UnapprovedShots[ShotIndex];

		// Look for an exact match first
		if ( ApprovedShots.Contains(UnapprovedShot) )
		{
			Comparisons->Existing.Add(UnapprovedShot);
			ApprovedShots.Remove(UnapprovedShot);
		}
		else
		{
			Comparisons->New.Add(UnapprovedShot);
		}

		UnapprovedShots.RemoveAt(ShotIndex);
		ShotIndex--;
	}

	for ( int32 ShotIndex = 0; ShotIndex < ApprovedShots.Num(); ShotIndex++ )
	{
		const FString& ApprovedShot = ApprovedShots[ShotIndex];

		Comparisons->Missing.Add(ApprovedShot);
	}

	return Comparisons;
}

void FScreenShotManager::CreateData()
{
	TSet<FString> TempPlatforms;

	// Clear existing data
	CachedPlatformList.Empty();
	ScreenShotDataArray.Empty();

	const FString RenderOutputValidation(TEXT("RenderOutputValidation"));

	const float AllowedPercentDifference = 0.02f;

	// <Stream or NetworkPath>/<CLNumber or Name>/<OS>_<RHI>/Project(<Project>) Map(<Map>) Time(<Time>s).png
	/*for ( const FImageComparisonResult& Result : Results )
	{
		FString PathString = Result.FileB;

		FPaths::MakePathRelativeTo(PathString, *Comparisons->UnapprovedFolder);

		FString PathRemainder = PathString;

		FString Map_Test;
		FString Platform_RHI;

		bool FoundMapAndTest = PathRemainder.Split(TEXT("/"), &Map_Test, &PathRemainder, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		bool FoundPlatformRHI = PathRemainder.Split(TEXT("/"), &Platform_RHI, &PathRemainder, ESearchCase::CaseSensitive, ESearchDir::FromStart);

		ScreenShotDataArray.Add(MakeShareable(new FScreenShotDataItem(Result.FileB, Map_Test, Platform_RHI, Result)));

		TempPlatforms.Add(Platform_RHI);
	}*/
	
	// Sort change list.
	//struct FSortDataItems
	//{
	//	FORCEINLINE bool operator()( const FScreenShotDataItem A, const FScreenShotDataItem B ) const 
	//	{ 
	//		if ( B.ViewName == A.ViewName )
	//		{
	//			return B.ChangeListNumber < A.ChangeListNumber;
	//		}
	//		else
	//		{
	//			return FCString::Stricmp( *B.ViewName, *A.ViewName ) > 0;
	//		}
	//	}
	//};
	//ScreenShotDataArray.Sort( FSortDataItems() );

	// Add platforms for searching
	CachedPlatformList.Add( MakeShareable(new FString("Any") ) );

	for ( FString Platform : TempPlatforms )
	{
		TSharedPtr<FString> PlatformName = MakeShareable(new FString(Platform));
		CachedPlatformList.Add(PlatformName);
	}

	SetFilter( nullptr );
}

TArray< TSharedPtr<FScreenShotDataItem> >& FScreenShotManager::GetScreenshotResults()
{
	return ScreenShotDataArray;
}


/* IScreenShotManager interface
 *****************************************************************************/

void FScreenShotManager::GenerateLists()
{
	// Create the screen shot tree root
	ScreenShotRoot = MakeShareable( new FScreenShotBaseNode( "ScreenShotRoot" ) ); 

	// Create the screen shot data
	CreateData();

	//// Populate the screen shot tree with the dummy screen shot data
	//for ( int32 Index = 0; Index < ScreenShotDataArray.Num(); Index++ )
	//{
	//	ScreenShotRoot->AddScreenShotData( ScreenShotDataArray[ Index ] );
	//}
}


TArray<TSharedPtr<FString> >& FScreenShotManager::GetCachedPlatfomList()
{
	return CachedPlatformList;
}


TArray<IScreenShotDataPtr>& FScreenShotManager::GetLists()
{
	return ScreenShotRoot->GetFilteredChildren();
}


void FScreenShotManager::RegisterScreenShotUpdate(const FOnScreenFilterChanged& InDelegate )
{
	// Set the callback delegate
	ScreenFilterChangedDelegate = InDelegate;
}


void FScreenShotManager::SetFilter( TSharedPtr< ScreenShotFilterCollection > InFilter )
{
	// Set the filter on the root screen shot
	ScreenShotRoot->SetFilter( InFilter );

	// Update the UI
	ScreenFilterChangedDelegate.ExecuteIfBound();
}

void FScreenShotManager::SetDisplayEveryNthScreenshot(int32 NewNth)
{
	ScreenShotRoot->SetDisplayEveryNthScreenshot(NewNth);

	// Update the UI
	ScreenFilterChangedDelegate.ExecuteIfBound();
}


/* IScreenShotManager event handlers
 *****************************************************************************/

void FScreenShotManager::HandleScreenShotMessage( const FAutomationWorkerScreenImage& Message, const IMessageContextRef& Context )
{
	bool bTree = true;
	FString FileName = ScreenshotIncomingFolder / Message.ScreenShotName;
	IFileManager::Get().MakeDirectory( *FPaths::GetPath(FileName), bTree );
	FFileHelper::SaveArrayToFile( Message.ScreenImage, *FileName );

	// TODO Automation There is identical code in, Engine\Source\Runtime\AutomationWorker\Private\AutomationWorkerModule.cpp,
	// need to move this code into common area.

	FString Json;
	if ( FJsonObjectConverter::UStructToJsonObjectString(Message.Metadata, Json) )
	{
		FString MetadataPath = FPaths::ChangeExtension(FileName, TEXT("json"));
		FFileHelper::SaveStringToFile(Json, *MetadataPath, FFileHelper::EEncodingOptions::ForceUTF8);
	}

	// Regenerate the UI
	//GenerateLists();

	//TODO Rescan?  Just add new one?

	ScreenShotRoot->SetFilter(MakeShareable( new ScreenShotFilterCollection()));
}

TFuture<void> FScreenShotManager::CompareScreensotsAsync()
{
	return Async<void>(EAsyncExecution::Thread, [&] () { CompareScreensots(); });
}

void FScreenShotManager::CompareScreensots()
{
	TSharedRef<FScreenshotComparisons> Comparisons = GenerateFileList();

	IFileManager::Get().DeleteDirectory(*ScreenshotDeltaFolder, /*RequireExists =*/false, /*Tree =*/true);

	FImageComparer Comparer;
	Comparer.ImageRootA = ScreenshotApprovedFolder;
	Comparer.ImageRootB = ScreenshotIncomingFolder;
	Comparer.DeltaDirectory = ScreenshotDeltaFolder;

	FImageTolerance Tolerance = FImageTolerance::DefaultIgnoreLess;
	Tolerance.IgnoreAntiAliasing = true;

	FComparisonResults Results;
	Results.ApprovedPath = TEXT("Approved");
	Results.IncomingPath = TEXT("Incoming");
	Results.DeltaPath = TEXT("Delta");

	for ( const FString& Existing : Comparisons->Existing )
	{
		FString TestApprovedFolder = FPaths::Combine(Comparisons->ApprovedFolder, Existing);
		FString TestUnapprovedFolder = FPaths::Combine(Comparisons->UnapprovedFolder, Existing);

		TArray<FString> ApprovedDeviceShots;
		IFileManager::Get().FindFilesRecursive(ApprovedDeviceShots, *TestApprovedFolder, TEXT("*.png"), true, false);

		TArray<FString> UnapprovedDeviceShots;
		IFileManager::Get().FindFilesRecursive(UnapprovedDeviceShots, *TestUnapprovedFolder, TEXT("*.png"), true, false);

		check(ApprovedDeviceShots.Num() > 0);
		check(UnapprovedDeviceShots.Num() > 0);

		// Look for an exact adapter match so we're comparing apples to apples.
		for ( const FString& UnapprovedShot : UnapprovedDeviceShots )
		{
			FString UnapprovedFileName = FPaths::GetCleanFilename(UnapprovedShot);

			FString* ExistingApprovedAdapter = ApprovedDeviceShots.FindByPredicate([&UnapprovedFileName] (const FString& ApprovedPath) {
				FString ApprovedFileName = FPaths::GetCleanFilename(ApprovedPath);
				return ApprovedFileName == UnapprovedFileName;
			});

			FString ExistingApproved;

			if ( ExistingApprovedAdapter )
			{
				ExistingApproved = FPaths::GetCleanFilename(*ExistingApprovedAdapter);
			}
			else
			{
				ExistingApproved = FPaths::GetCleanFilename(ApprovedDeviceShots[0]);
				// TODO No good, this should find closest resolution, maybe some other stuff as well?
			}

			FString ApprovedFullPath = FPaths::Combine(TestApprovedFolder, ExistingApproved);
			FString UnapprovedFullPath = FPaths::Combine(TestUnapprovedFolder, UnapprovedFileName);

			// TODO need to read in metadata file telling how to compare these files.

			FImageComparisonResult Result = Comparer.Compare(ApprovedFullPath, UnapprovedFullPath, Tolerance);
			Results.Comparisons.Add(Result);
		}
	}

	SaveComparisonResults(Results);
}

TFuture<void> FScreenShotManager::ExportScreensotsAsync(FString ExportPath)
{
	return Async<void>(EAsyncExecution::Thread, [&] () { ExportComparisonResults(ExportPath); });
}

bool FScreenShotManager::ExportComparisonResults(FString RootExportFolder)
{
	FPaths::NormalizeDirectoryName(RootExportFolder);

	if ( RootExportFolder.IsEmpty() )
	{
		RootExportFolder = GetDefaultExportDirectory();
	}

	IFileManager::Get().DeleteDirectory(*RootExportFolder, /*RequireExists =*/false, /*Tree =*/true);
	if ( !IFileManager::Get().MakeDirectory(*RootExportFolder, /*Tree =*/true) )
	{
		return false;
	}

	// Wait for file operations to complete.
	FPlatformProcess::Sleep(1.0f);

	FString ExportDirectory = RootExportFolder / FString::FromInt(FEngineVersion::Current().GetChangelist());
	FString ExportDirectory_ComparisonManifest = ExportDirectory / TEXT("ComparisonResults.json");
	FString ExportDirectory_Approved = ExportDirectory / TEXT("Approved");
	FString ExportDirectory_Incoming = ExportDirectory / TEXT("Incoming");
	FString ExportDirectory_Delta = ExportDirectory / TEXT("Delta");

	// Copy the comparison manifest
	IFileManager::Get().Copy(*ExportDirectory_ComparisonManifest, *GetComparisonResultsJsonFilename(), true, true);

	TArray<FString> FilesToCopy;

	// Copy the ground truth images
	CopyDirectory(ExportDirectory_Approved, ScreenshotApprovedFolder);

	// Copy the incoming images
	CopyDirectory(ExportDirectory_Incoming, ScreenshotIncomingFolder);

	// Copy the delta images
	CopyDirectory(ExportDirectory_Delta, ScreenshotDeltaFolder);

	return true;
}

TSharedPtr<FComparisonResults> FScreenShotManager::ImportScreensots(FString ImportPath)
{
	FString ManifestPath = ImportPath / TEXT("ComparisonResults.json");

	FString JsonString;

	if ( FFileHelper::LoadFileToString(JsonString, *ManifestPath) )
	{
		TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(JsonString);

		TSharedPtr<FJsonObject> JsonComparisons;
		if ( !FJsonSerializer::Deserialize(JsonReader, JsonComparisons) )
		{
			return nullptr;
		}

		FComparisonResults* ImportedResults = new FComparisonResults();

		if ( FJsonObjectConverter::JsonObjectToUStruct(JsonComparisons.ToSharedRef(), ImportedResults, 0, 0) )
		{
			return MakeShareable(ImportedResults);
		}

		delete ImportedResults;
	}

	return nullptr;
}

FString FScreenShotManager::GetDefaultExportDirectory() const
{
	return FPaths::GameSavedDir() / TEXT("Exported");
}

FString FScreenShotManager::GetComparisonResultsJsonFilename() const
{
	return ScreenshotResultsFolder / TEXT("ComparisonResults.json");
}

void FScreenShotManager::SaveComparisonResults(const FComparisonResults& Results) const
{
	FString Json;
	if ( FJsonObjectConverter::UStructToJsonObjectString(Results, Json) )
	{
		FFileHelper::SaveStringToFile(Json, *GetComparisonResultsJsonFilename(), FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

void FScreenShotManager::CopyDirectory(const FString& DestDir, const FString& SrcDir)
{
	TArray<FString> FilesToCopy;

	IFileManager::Get().FindFilesRecursive(FilesToCopy, *SrcDir, TEXT("*"), /*Files=*/true, /*Directories=*/true);
	for ( const FString& File : FilesToCopy )
	{
		const FString& SourceFilePath = File;
		FString DestFilePath = DestDir / SourceFilePath.RightChop(SrcDir.Len());

		IFileManager::Get().Copy(*DestFilePath, *File, true, true);
	}
}