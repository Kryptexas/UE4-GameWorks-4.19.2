// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "VSAccessorPrivatePCH.h"
#include "ModuleManager.h"

#if !WITH_VSEXPRESS
#include "AllowWindowsPlatformTypes.h"
	#pragma warning(disable: 4278)
	#pragma warning(disable: 4146)
	#pragma warning(disable: 4191)

	// atltransactionmanager.h doesn't use the W equivalent functions, use this workaround
	#ifndef DeleteFile
		#define DeleteFile DeleteFileW
	#endif
	#ifndef MoveFile
		#define MoveFile MoveFileW
	#endif
	#include "atlbase.h"
	#undef DeleteFile
	#undef MoveFile

	// import EnvDTE
	#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") lcid("0") raw_interfaces_only named_guids

	#pragma warning(default: 4191)
	#pragma warning(default: 4146)
	#pragma warning(default: 4278)
#include "HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVSAccessor, Log, All);

IMPLEMENT_MODULE( FVSAccessorModule, VSAccessor );

/** The VS query in progress message */
TWeakPtr<class SNotificationItem> VSNotificationPtr = NULL;

/** save all open documents in visual studio, when recompiling */
void SaveVisualStudioDocuments()
{
	FVSAccessorModule& VSAccessorModule = FModuleManager::LoadModuleChecked<FVSAccessorModule>(TEXT("VSAccessor"));
	VSAccessorModule.SaveAllOpenVisualStudioDocuments();
}

void FVSAccessorModule::StartupModule()
{
	VSLaunchTime = 0.0;

	// Setup compilation for saving all VS documents upon compilation start
	FModuleManager::Get().OnModuleCompilerStarted().AddStatic( &SaveVisualStudioDocuments );

	// Cache this so we don't have to do it on a background thread
	SolutionPath = FPaths::ConvertRelativePathToFull(FModuleManager::Get().GetSolutionFilepath());

	// Preferential order of VS versions
	Locations.Add( VisualStudioLocation( TEXT("!VisualStudio.DTE.12.0"), TEXT("SOFTWARE\\Microsoft\\VisualStudio\\12.0"), TEXT("devenv.exe") ) );	// Visual Studio 2013
	if (!FRocketSupport::IsRocket())
	{
		// explicitly not supporting 2012 in rocket binary builds
		Locations.Add(VisualStudioLocation(TEXT("!VisualStudio.DTE.11.0"), TEXT("SOFTWARE\\Microsoft\\VisualStudio\\11.0"), TEXT("devenv.exe")));	// Visual Studio 2012
	}

	Locations.Add( VisualStudioLocation( TEXT("!WDExpress.DTE.12.0"), TEXT("SOFTWARE\\Microsoft\\WDExpress\\12.0"), TEXT("wdexpress.exe") ) );		// Visual Studio 2013 Express for Windows Desktop
	if (!FRocketSupport::IsRocket())
	{
		// explicitly not supporting 2012 in rocket binary builds
		Locations.Add( VisualStudioLocation( TEXT("!WDExpress.DTE.11.0"), TEXT("SOFTWARE\\Microsoft\\WDExpress\\11.0"), TEXT("wdexpress.exe") ) );		// Visual Studio 2012 Express for Windows Desktop
	}
}

void FVSAccessorModule::ShutdownModule()
{
	FModuleManager::Get().OnModuleCompilerStarted().RemoveStatic( &SaveVisualStudioDocuments );
}

#if !WITH_VSEXPRESS

bool FVSAccessorModule::CanRunVisualStudio( FString& OutPath ) const
{
	bool bSuccess = false;

	// Attempt to find the latest version of VS installed on the system by attempting each valid registry entry
	const FString RegistryVariableName = TEXT("InstallDir");
	for (int32 VSVersion = 0; VSVersion < Locations.Num() && !bSuccess; VSVersion++)
	{
		const FString& RegistryKeyName = Locations[VSVersion].RegistryKeyName;

		if ( FWindowsPlatformMisc::QueryRegKey( HKEY_LOCAL_MACHINE, *RegistryKeyName, *RegistryVariableName, OutPath ) && OutPath.Len() > 0 )
		{
			OutPath += Locations[VSVersion].ExecutableName;

			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FVSAccessorModule::RunVisualStudioAndOpenSolution() const
{
	bool bSuccess = false;

	FString Path;
	if ( CanRunVisualStudio( Path ) )
	{
		FString Params;
		if ( FModuleManager::Get().IsSolutionFilePresent() )
		{
			Params += TEXT("\"");
			Params += FModuleManager::Get().GetSolutionFilepath();
			Params += TEXT("\"");

			FProcHandle WorkerHandle = FPlatformProcess::CreateProc( *Path, *Params, true, false, false, NULL, 0, NULL, NULL );
			if ( WorkerHandle.IsValid() )
			{
				bSuccess = true;
			}
			WorkerHandle.Close();
		}
	}

	return bSuccess;
}

bool IsVisualStudioMoniker(const FString& InName, const TArray<FVSAccessorModule::VisualStudioLocation>& InLocations)
{
	for(int32 VSVersion = 0; VSVersion < InLocations.Num(); VSVersion++)
	{
		if(InName.StartsWith(InLocations[VSVersion].ROTMoniker))
		{
			return true;
		}
	}

	return false;
}

/** Accesses the correct visual studio instance if possible. */
bool AccessVisualStudio(CComPtr<EnvDTE::_DTE>& OutDTE, const FString& InSolutionPath, const TArray<FVSAccessorModule::VisualStudioLocation>& InLocations)
{
	bool bSuccess = false;

	// Open the Running Object Table (ROT)
	IRunningObjectTable* RunningObjectTable;
	if (SUCCEEDED(GetRunningObjectTable(0, &RunningObjectTable)) &&
		RunningObjectTable)
	{
		IEnumMoniker* MonikersTable;
		RunningObjectTable->EnumRunning(&MonikersTable);
		MonikersTable->Reset();

		// Look for all visual studio instances in the ROT
		IMoniker* CurrentMoniker;
		while (!bSuccess && MonikersTable->Next(1, &CurrentMoniker, NULL) == S_OK)
		{
			IBindCtx* BindContext;
			LPOLESTR OutName;
			CComPtr<IUnknown> ComObject;
			if (SUCCEEDED(CreateBindCtx(0, &BindContext)) &&
				SUCCEEDED(CurrentMoniker->GetDisplayName(BindContext, NULL, &OutName)) &&
				IsVisualStudioMoniker(FString(OutName), InLocations) &&
				SUCCEEDED(RunningObjectTable->GetObject(CurrentMoniker, &ComObject)))
			{
				CComPtr<EnvDTE::_DTE> TempDTE;
				TempDTE = ComObject;
				
				// Get the solution path for this instance
				// If it equals the solution we would have opened above in RunVisualStudio(), we'll take that
				CComPtr<EnvDTE::_Solution> Solution;
				LPOLESTR OutPath;
				if (SUCCEEDED(TempDTE->get_Solution(&Solution)) &&
					SUCCEEDED(Solution->get_FullName(&OutPath)))
				{
					FString Filename(OutPath);
					FPaths::NormalizeFilename(Filename);

					if( Filename == InSolutionPath )
					{
						OutDTE = TempDTE;
						bSuccess = true;
					}
				}
			}
			BindContext->Release();
			CurrentMoniker->Release();
		}
		MonikersTable->Release();
		RunningObjectTable->Release();
	}

	return bSuccess;
}

bool FVSAccessorModule::OpenVisualStudioSolution()
{
	// Automatically fail if there's already an attempt in progress
	if ( !IsVSLaunchInProgress() )
	{
		return RunVisualStudioAndOpenSolution();
	}
	return false;
}

bool FVSAccessorModule::OpenVisualStudioFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	// Automatically fail if there's already an attempt in progress
	if (!IsVSLaunchInProgress())
	{
		return OpenVisualStudioFileAtLineInternal( FullPath, LineNumber, ColumnNumber );
	}
	return false;
}

bool FVSAccessorModule::OpenVisualStudioFileAtLineInternal(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	// Check that the file actually exists first
	if (!FPaths::FileExists(FullPath))
	{
		OpenFileFailed.Broadcast(FullPath);
		return false;
	}

	// Initialize the com library, if not already by this thread
	if (!FWindowsPlatformMisc::CoInitialize())
	{
		UE_LOG(LogVSAccessor, Error, TEXT( "ERROR - Could not initialize COM library!" ));
		return false;
	}
	
	bool bDefer = false, bSuccess = false;
	CComPtr<EnvDTE::_DTE> DTE;
	if (AccessVisualStudio(DTE, SolutionPath, Locations))
	{
		// Set Focus on Visual Studio
		CComPtr<EnvDTE::Window> MainWindow;
		if (SUCCEEDED(DTE->get_MainWindow(&MainWindow)) &&
			SUCCEEDED(MainWindow->Activate()))
		{
			// Get ItemOperations
			CComPtr<EnvDTE::ItemOperations> ItemOperations;
			if (SUCCEEDED(DTE->get_ItemOperations(&ItemOperations)))
			{
				// Open File
				auto ANSIPath = StringCast<ANSICHAR>(*FullPath);
				CComBSTR COMStrFileName(ANSIPath.Get());
				CComBSTR COMStrKind(EnvDTE::vsViewKindTextView);
				CComPtr<EnvDTE::Window> Window;
				if (SUCCEEDED(ItemOperations->OpenFile(COMStrFileName, COMStrKind, &Window)))
				{
					// Scroll to Line Number
					CComPtr<EnvDTE::Document> Document;
					CComPtr<IDispatch> SelectionDispatch;
					CComPtr<EnvDTE::TextSelection> Selection;
					if (SUCCEEDED(DTE->get_ActiveDocument(&Document)) &&
						SUCCEEDED(Document->get_Selection(&SelectionDispatch)) &&
						SUCCEEDED(SelectionDispatch->QueryInterface(&Selection)) &&
						SUCCEEDED(Selection->GotoLine(LineNumber, true)))
					{
						if (ColumnNumber > 0 &&
							SUCCEEDED(Selection->MoveToLineAndOffset(LineNumber, ColumnNumber, false)))
						{
							bSuccess = true;
						}
						else
						{
							UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't goto column number '%i' of line '%i' in '%s'"), ColumnNumber, LineNumber, *FullPath);
						}
					}
					else
					{
						UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't goto line number '%i' in '%s'"), LineNumber, *FullPath);
					}
				}
				else
				{
					UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't open file '%s'."), *FullPath);
				}

				VSLaunchFinished( true );
			}
			else
			{
				UE_LOG(LogVSAccessor, Log, TEXT("Couldn't get item operations. Visual Studio may still be initializing."), *FullPath);
				bDefer = true;
			}
		}
		else
		{
			UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't set focus on Visual Studio."));
		}
	}
	else
	{
		bDefer = true;

		// We can't process until we're in the main thread, if we aren't initially defer until we are
		if ( IsInGameThread() )
		{
			// If we haven't already attempted to launch VS do so now
			if (!IsVSLaunchInProgress())
			{
				// If there's no valid instance of VS running, run one if we have it installed
				if ( !RunVisualStudioAndOpenSolution() )
				{
					bDefer = false;
				}
				else
				{
					VSLaunchStarted();
				}
			}
		}
	}

	// If we have attempted to launch VS, and it's taken too long, timeout so the user can try again
	if ( IsVSLaunchInProgress() && ( FPlatformTime::Seconds() - VSLaunchTime ) > 300 )
	{
		// We need todo this incase the process died or was kill prior to the code gaining focus of it
		bDefer = false;
		VSLaunchFinished( false );

		// We failed to open the solution and file, so lets just use the platforms default opener.
		FPlatformProcess::LaunchFileInDefaultExternalApplication( *FullPath );
	}

	// Defer the request until VS is available to take hold of
	if ( bDefer )
	{
		const FString DeferCommand = FString::Printf( TEXT( "OPEN_VS %s %d %d" ), *FullPath, LineNumber, ColumnNumber);
		LaunchVSDeferred.Broadcast( DeferCommand );
	}
	else
	{
		UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't access Visual Studio"));
	}

	// Uninitialize the com library, if we initialized it above (don't call if S_FALSE)
	FWindowsPlatformMisc::CoUninitialize();

	return bSuccess;
}

bool FVSAccessorModule::SaveAllOpenVisualStudioDocuments()
{
	bool bSuccess = false;

	// Initialize the com library, if not already by this thread
	if (!FWindowsPlatformMisc::CoInitialize())
	{
		UE_LOG(LogVSAccessor, Error, TEXT( "ERROR - Could not initialize COM library!" ));
		return bSuccess;
	}
	
	CComPtr<EnvDTE::_DTE> DTE;
	if (AccessVisualStudio(DTE, SolutionPath, Locations))
	{
		// Save all documents
		CComPtr<EnvDTE::Documents> Documents;
		if (SUCCEEDED(DTE->get_Documents(&Documents)) &&
			SUCCEEDED(Documents->SaveAll()))
		{
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't save all documents"));
		}
	}
	else
	{
		UE_LOG(LogVSAccessor, Warning, TEXT("Couldn't access Visual Studio"));
	}

	// Uninitialize the com library, if we initialized it above (don't call if S_FALSE)
	FWindowsPlatformMisc::CoUninitialize();

	return bSuccess;
}

bool FVSAccessorModule::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	const TCHAR *Str = Cmd;
	if( FParse::Command(&Str,TEXT("OPEN_VS")) )
	{
		// Valid column number?
		const FString Params( Str );
		const FString Space( TEXT(" ") );
		const int32 ColumnIndex = Params.Find( Space, ESearchCase::CaseSensitive, ESearchDir::FromEnd );
		if ( ColumnIndex != INDEX_NONE )
		{
			// Valid line number?
			const int32 ColumnNumber = FCString::Atoi( *Params.RightChop( ColumnIndex+Space.Len() ) );
			const int32 LineIndex = Params.Find( Space, ESearchCase::CaseSensitive, ESearchDir::FromEnd, ColumnIndex );
			if ( LineIndex != INDEX_NONE )
			{
				// Valid source path?
				const int32 LineNumber = FCString::Atoi( *Params.Mid( LineIndex+Space.Len(), ColumnIndex-LineIndex-Space.Len() ) );
				const FString FullPath = Params.LeftChop( Params.Len() - LineIndex );
				if ( FullPath.Len() > 0 )
				{
					OpenVisualStudioFileAtLineInternal( FullPath, LineNumber, ColumnNumber );
				}
			}
		}
		return true;
	}
	return false;
}

void FVSAccessorModule::VSLaunchStarted()
{
	// Broadcast the info and hope that MainFrame is around to receive it
	LaunchingVS.Broadcast();
	VSLaunchTime = FPlatformTime::Seconds();
}

void FVSAccessorModule::VSLaunchFinished( bool bSuccess )
{
	// Finished all requests! Notify the UI.
	DoneLaunchingVS.Broadcast( bSuccess );
	VSLaunchTime = 0.0;
}

#else // VS Express-only dummy versions

bool FVSAccessorModule::CanRunVisualStudio( FString& OutPath ) const { return false; }
bool FVSAccessorModule::RunVisualStudioAndOpenSolution() const { return false; }
bool FVSAccessorModule::OpenVisualStudioSolution() { return false; }
bool FVSAccessorModule::OpenVisualStudioFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber) { return false; }
bool FVSAccessorModule::OpenVisualStudioFileAtLineInternal(const FString& FullPath, int32 LineNumber, int32 ColumnNumber) { return false; }
bool FVSAccessorModule::SaveAllOpenVisualStudioDocuments() { return false; }
bool FVSAccessorModule::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) { return false; }
void FVSAccessorModule::VSLaunchStarted() {}
void FVSAccessorModule::VSLaunchFinished( bool bSuccess ) {}

#endif