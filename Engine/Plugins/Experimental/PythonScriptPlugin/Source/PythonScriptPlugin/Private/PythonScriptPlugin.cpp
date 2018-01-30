// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PythonScriptPlugin.h"
#include "PyCore.h"
#include "PyEditor.h"
#include "PyConstant.h"
#include "PyConversion.h"
#include "PyMethodWithClosure.h"
#include "PyReferenceCollector.h"
#include "PyWrapperTypeRegistry.h"
#include "EngineAnalytics.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "HAL/PlatformProcess.h"
#include "Containers/Ticker.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"
#include "ProfilingDebugging/ScopedTimers.h"

#if WITH_EDITOR
#include "EditorSupportDelegates.h"
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#endif	// WITH_EDITOR

#define LOCTEXT_NAMESPACE "PythonScriptPlugin"

#if WITH_PYTHON

FPythonCommandExecutor::FPythonCommandExecutor(FPythonScriptPlugin* InPythonScriptPlugin)
	: PythonScriptPlugin(InPythonScriptPlugin)
{
}

FName FPythonCommandExecutor::StaticName()
{
	static const FName CmdExecName = TEXT("Python");
	return CmdExecName;
}

FName FPythonCommandExecutor::GetName() const
{
	return StaticName();
}

FText FPythonCommandExecutor::GetDisplayName() const
{
	return LOCTEXT("PythonCommandExecutorDisplayName", "Python");
}

FText FPythonCommandExecutor::GetDescription() const
{
	return LOCTEXT("PythonCommandExecutorDescription", "Execute Python Scripts");
}

FText FPythonCommandExecutor::GetHintText() const
{
	return LOCTEXT("PythonCommandExecutorHintText", "Enter Python Script");
}

void FPythonCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
}

void FPythonCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(TEXT("Python"), Out);
}

bool FPythonCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(TEXT("Python"), Input);

	UE_LOG(LogPython, Log, TEXT("%s"), Input);
	PythonScriptPlugin->HandlePythonExecCommand(Input);

	return true;
}

bool FPythonCommandExecutor::AllowHotKeyClose() const
{
	return false;
}

bool FPythonCommandExecutor::AllowMultiLine() const
{
	return true;
}

#if WITH_EDITOR
class FPythonCommandMenuImpl : public IPythonCommandMenu
{
public:
	FPythonCommandMenuImpl()
		: bRecentsFilesDirty(false)
	{
		ConfigFilename = UObject::StaticClass()->GetDefaultObject()->GetGlobalUserConfigFilename();
	}

	virtual void OnStartupMenu() override
	{
		LoadConfig();

		// Create & Add menu extension
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuExtension("FileLoadAndSave", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FPythonCommandMenuImpl::CreateMenu));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	virtual void OnShutdownMenu() override
	{
		// Remove menu extension
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);
		MenuExtender = nullptr;

		// Write to file
		if (bRecentsFilesDirty)
		{
			SaveConfig();
			bRecentsFilesDirty = false;
		}
	}

	virtual void OnRunFile(const FString& InFile, bool bAdd) override
	{
		if (bAdd)
		{
			int32 Index = RecentsFiles.Find(InFile);
			if (Index != INDEX_NONE)
			{
				// If already in the list but not at the last position
				if (Index != RecentsFiles.Num() - 1)
				{
					RecentsFiles.RemoveAt(Index);
					RecentsFiles.Add(InFile);
					bRecentsFilesDirty = true;
				}
			}
			else
			{
				if (RecentsFiles.Num() >= MaxNumberOfFiles)
				{
					RecentsFiles.RemoveAt(0);
				}
				RecentsFiles.Add(InFile);
				bRecentsFilesDirty = true;
			}
		}
		else
		{
			if (RecentsFiles.RemoveSingle(InFile) > 0)
			{
				bRecentsFilesDirty = true;
			}
		}
	}

private:
	const TCHAR* STR_ConfigSection = TEXT("Python");
	const TCHAR* STR_ConfigDirectoryKey = TEXT("LastDirectory");
	const FName NAME_ConfigRecentsFilesyKey = TEXT("RecentsFiles");
	static const int32 MaxNumberOfFiles = 10;

	void LoadConfig()
	{
		RecentsFiles.Reset();

		GConfig->GetString(STR_ConfigSection, STR_ConfigDirectoryKey, LastDirectory, ConfigFilename);

		FConfigSection* Sec = GConfig->GetSectionPrivate(STR_ConfigSection, false, true, ConfigFilename);
		if (Sec)
		{
			TArray<FConfigValue> List;
			Sec->MultiFind(NAME_ConfigRecentsFilesyKey, List);

			int32 ListNum = FMath::Min(List.Num(), MaxNumberOfFiles);

			RecentsFiles.Reserve(ListNum);
			for (int32 Index = 0; Index < ListNum; ++Index)
			{
				RecentsFiles.Add(List[Index].GetValue());
			}
		}
	}

	void SaveConfig() const
	{
		GConfig->SetString(STR_ConfigSection, STR_ConfigDirectoryKey, *LastDirectory, ConfigFilename);

		FConfigSection* Sec = GConfig->GetSectionPrivate(STR_ConfigSection, true, false, ConfigFilename);
		if (Sec)
		{
			Sec->Remove(NAME_ConfigRecentsFilesyKey);
			for (int32 Index = RecentsFiles.Num() - 1; Index >= 0; --Index)
			{
				Sec->Add(NAME_ConfigRecentsFilesyKey, *RecentsFiles[Index]);
			}
		}

		GConfig->Flush(false);
	}

	void MakeRecentPythonScriptMenu(FMenuBuilder& InMenuBuilder)
	{
		for (int32 Index = RecentsFiles.Num() - 1; Index >= 0; --Index)
		{
			InMenuBuilder.AddMenuEntry(
				FText::FromString(RecentsFiles[Index]),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateRaw(this, &FPythonCommandMenuImpl::Menu_ExecutePythonRecent, Index))
			);
		}
		int32 Variable1 = 1;
	}

	void CreateMenu(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("Python", LOCTEXT("Python", "Python"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenPython", "Execute Python Script"),
			LOCTEXT("OpenPythonTooltip", "Open a Python Script file and Execute it."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FPythonCommandMenuImpl::Menu_ExecutePython))
		);
		MenuBuilder.AddSubMenu(
			LOCTEXT("RecentPythonsSubMenu", "Recent Python Scripts"),
			LOCTEXT("RecentPythonsSubMenu_ToolTip", "Select a level to load"),
			FNewMenuDelegate::CreateRaw(this, &FPythonCommandMenuImpl::MakeRecentPythonScriptMenu),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.RecentLevels")
		);
		MenuBuilder.EndSection();
	}

	void Menu_ExecutePythonRecent(int32 Index)
	{
		if (RecentsFiles.IsValidIndex(Index))
		{
			FString PyCopied = RecentsFiles[Index];
			GEngine->Exec(NULL, *FString::Printf(TEXT("py %s"), *PyCopied));
		}
	}

	void Menu_ExecutePython()
	{
		TArray<FString> OpenedFiles;
		FString DefaultDirectory = LastDirectory;

		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			bool bOpened = DesktopPlatform->OpenFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				LOCTEXT("OpenPython", "Execute Python Script").ToString(),
				DefaultDirectory,
				TEXT(""),
				TEXT("Python files|*.py|"),
				EFileDialogFlags::None,
				OpenedFiles
			);

			if (bOpened && OpenedFiles.Num() > 0)
			{
				if (DefaultDirectory != LastDirectory)
				{
					LastDirectory = DefaultDirectory;
					bRecentsFilesDirty = true;
				}

				GEngine->Exec(NULL, *FString::Printf(TEXT("py %s"), *OpenedFiles.Last()));
			}
		}
	}

private:
	TSharedPtr<FExtender> MenuExtender;

	TArray<FString> RecentsFiles;
	FString LastDirectory;

	FString ConfigFilename;
	bool bRecentsFilesDirty;
};
#endif // WITH_EDITOR

#endif	// WITH_PYTHON

FPythonScriptPlugin::FPythonScriptPlugin()
#if WITH_PYTHON
	: CmdExec(this)
	, CmdMenu(nullptr)
	, bInitialized(false)
#endif	// WITH_PYTHON
{
}

FPythonScriptPlugin* FPythonScriptPlugin::Get()
{
	static const FName ModuleName = "PythonScriptPlugin";
	return FModuleManager::GetModulePtr<FPythonScriptPlugin>(ModuleName);
}

void FPythonScriptPlugin::StartupModule()
{
#if WITH_PYTHON
	InitializePython();
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &CmdExec);

#if WITH_EDITOR
		check(CmdMenu == nullptr);
		CmdMenu = new FPythonCommandMenuImpl();
		CmdMenu->OnStartupMenu();
#endif // WITH_EDITOR

	FCoreDelegates::OnPreExit.AddRaw(this, &FPythonScriptPlugin::ShutdownPython);
#endif	// WITH_PYTHON
}

void FPythonScriptPlugin::ShutdownModule()
{
#if WITH_PYTHON
	FCoreDelegates::OnPreExit.RemoveAll(this);

#if WITH_EDITOR
	check(CmdMenu);
	CmdMenu->OnShutdownMenu();
	delete CmdMenu;
	CmdMenu = nullptr;
#endif // WITH_EDITOR

	IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), &CmdExec);
	ShutdownPython();
#endif	// WITH_PYTHON
}

bool FPythonScriptPlugin::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if WITH_PYTHON
	if (FParse::Command(&Cmd, TEXT("PY")))
	{
		HandlePythonExecCommand(Cmd);
		return true;
	}
#endif	// WITH_PYTHON
	return false;
}

#if WITH_PYTHON

void FPythonScriptPlugin::InitializePython()
{
	bInitialized = true;

	// Set-up the correct program name
	{
		FString ProgramName = FPlatformProcess::GetCurrentWorkingDirectory() / FPlatformProcess::ExecutableName(false);
		FPaths::NormalizeFilename(ProgramName);
		PyProgramName = PyUtil::TCHARToPyApiBuffer(*ProgramName);
	}

	// Set-up the correct home path
	{
		// Build the full Python directory (UE_PYTHON_DIR may be relative to UE4 engine directory for portability)
		FString PythonDir = UTF8_TO_TCHAR(UE_PYTHON_DIR);
		PythonDir.ReplaceInline(TEXT("{ENGINE_DIR}"), *FPaths::EngineDir(), ESearchCase::CaseSensitive);
		FPaths::NormalizeDirectoryName(PythonDir);
		FPaths::RemoveDuplicateSlashes(PythonDir);
		PyHomePath = PyUtil::TCHARToPyApiBuffer(*PythonDir);
	}

	// Set-up the correct command line
	{
		PyCommandLineArgs.Add(PyUtil::TCHARToPyApiBuffer(TEXT(""))); // Script name; always empty

		const TCHAR* CmdLine = FCommandLine::Get();
		FString NextToken;
		while (FParse::Token(CmdLine, NextToken, false))
		{
			PyCommandLineArgs.Add(PyUtil::TCHARToPyApiBuffer(*NextToken));
		}

		PyCommandLineArgPtrs.Reserve(PyCommandLineArgs.Num());
		for (PyUtil::FPyApiBuffer& PyCommandLineArg : PyCommandLineArgs)
		{
			PyCommandLineArgPtrs.Add(PyCommandLineArg.GetData());
		}
	}

	// Initialize the Python interpreter
	{
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4
		Py_SetStandardStreamEncoding("utf-8", nullptr);
#endif	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 4
		Py_SetProgramName(PyProgramName.GetData());
		Py_SetPythonHome(PyHomePath.GetData());
		Py_Initialize();

		PySys_SetArgv(PyCommandLineArgPtrs.Num(), PyCommandLineArgPtrs.GetData());

		// Initialize our custom method type as we'll need it when generating bindings
		InitializePyMethodWithClosure();

		// Initialize our custom constant type as we'll need it when generating bindings
		InitializePyConstant();

		PyObject* PyMainModule = PyImport_AddModule("__main__");
		PyGlobalDict = FPyObjectPtr::NewReference(PyModule_GetDict(PyMainModule));
		PyLocalDict = PyGlobalDict;

#if WITH_EDITOR
		FEditorSupportDelegates::PrepareToCleanseEditorObject.AddRaw(this, &FPythonScriptPlugin::OnPrepareToCleanseEditorObject);
#endif	// WITH_EDITOR
	}

	// Set-up the known Python script paths
	{
		PyUtil::AddSystemPath(FPaths::ConvertRelativePathToFull(FPlatformProcess::UserDir() / FApp::GetEpicProductIdentifier() / TEXT("Python")));

		TArray<FString> RootPaths;
		FPackageName::QueryRootContentPaths(RootPaths);
		for (const FString& RootPath : RootPaths)
		{
			const FString RootFilesystemPath = FPackageName::LongPackageNameToFilename(RootPath);
			PyUtil::AddSystemPath(FPaths::ConvertRelativePathToFull(RootFilesystemPath / TEXT("Python")));
		}

		FPackageName::OnContentPathMounted().AddRaw(this, &FPythonScriptPlugin::OnContentPathMounted);
		FPackageName::OnContentPathDismounted().AddRaw(this, &FPythonScriptPlugin::OnContentPathDismounted);
	}

	// Initialize the Unreal Python module
	{
		// Create the top-level "unreal" module
		PyUnrealModule = FPyObjectPtr::NewReference(PyImport_AddModule("unreal"));
			
		// Initialize the and import the "core" module
		PyCore::InitializeModule();
		ImportUnrealModule(TEXT("core"));

#if WITH_EDITOR
		// Initialize the and import the "editor" module
		PyEditor::InitializeModule();
		ImportUnrealModule(TEXT("editor"));
#endif	// WITH_EDITOR

		FPyWrapperTypeRegistry::Get().OnModuleDirtied().AddRaw(this, &FPythonScriptPlugin::OnModuleDirtied);
		FModuleManager::Get().OnModulesChanged().AddRaw(this, &FPythonScriptPlugin::OnModulesChanged);

		// Initialize the wrapped types
		FPyWrapperTypeRegistry::Get().GenerateWrappedTypes();

		// Initialize the re-instancer ticker
		ReinstanceTickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaTime)
		{
			FPyWrapperTypeReinstancer::Get().ProcessPending();
			return true;
		}));
	}
}

void FPythonScriptPlugin::ShutdownPython()
{
	if (!bInitialized)
	{
		return;
	}

	FTicker::GetCoreTicker().RemoveTicker(ReinstanceTickerHandle);

	FPyWrapperTypeRegistry::Get().OnModuleDirtied().RemoveAll(this);
	FModuleManager::Get().OnModulesChanged().RemoveAll(this);

	FPackageName::OnContentPathMounted().RemoveAll(this);
	FPackageName::OnContentPathDismounted().RemoveAll(this);

#if WITH_EDITOR
	FEditorSupportDelegates::PrepareToCleanseEditorObject.RemoveAll(this);
#endif	// WITH_EDITOR

	PyUnrealModule.Reset();
	PyGlobalDict.Reset();
	PyLocalDict.Reset();

	ShutdownPyMethodWithClosure();

	Py_Finalize();

	bInitialized = false;
}

void FPythonScriptPlugin::ImportUnrealModule(const TCHAR* InModuleName)
{
	const FString PythonModuleName = FString::Printf(TEXT("unreal_%s"), InModuleName);
	const FString NativeModuleName = FString::Printf(TEXT("_unreal_%s"), InModuleName);

	const TCHAR* ModuleNameToImport = nullptr;
	PyObject* ModuleToReload = nullptr;
	if (PyUtil::IsModuleAvailableForImport(*PythonModuleName))
	{
		// Python modules that are already loaded should be reloaded if we're requested to import them again
		if (!PyUtil::IsModuleImported(*PythonModuleName, &ModuleToReload))
		{
			ModuleNameToImport = *PythonModuleName;
		}
	}
	else if (PyUtil::IsModuleAvailableForImport(*NativeModuleName))
	{
		ModuleNameToImport = *NativeModuleName;
	}

	FPyObjectPtr PyModule;
	if (ModuleToReload)
	{
		PyModule = FPyObjectPtr::StealReference(PyImport_ReloadModule(ModuleToReload));
	}
	else if (ModuleNameToImport)
	{
		PyModule = FPyObjectPtr::StealReference(PyImport_ImportModule(TCHAR_TO_UTF8(ModuleNameToImport)));
	}

	if (PyModule)
	{
		check(PyUnrealModule);
		PyObject* PyUnrealModuleDict = PyModule_GetDict(PyUnrealModule);

		// Hoist every public symbol from this module into the top-level "unreal" module
		{
			PyObject* PyModuleDict = PyModule_GetDict(PyModule);

			PyObject* PyObjKey = nullptr;
			PyObject* PyObjValue = nullptr;
			Py_ssize_t ModuleDictIndex = 0;
			while (PyDict_Next(PyModuleDict, &ModuleDictIndex, &PyObjKey, &PyObjValue))
			{
				if (PyObjKey)
				{
					const FString Key = PyUtil::PyObjectToUEString(PyObjKey);
					if (Key.Len() > 0 && Key[0] != TEXT('_'))
					{
						PyDict_SetItem(PyUnrealModuleDict, PyObjKey, PyObjValue);
					}
				}
			}
		}
	}
	else
	{
		PyUtil::LogPythonError(/*bInteractive*/true);
	}
}

void FPythonScriptPlugin::HandlePythonExecCommand(const TCHAR* InPythonCommand)
{
	if (FPaths::GetExtension(InPythonCommand) == TEXT("py"))
	{
		RunFile(InPythonCommand);
	}
	else
	{
		RunString(InPythonCommand);
	}
}

PyObject* FPythonScriptPlugin::EvalString(const TCHAR* InStr, const TCHAR* InContext, const int InMode)
{
	return EvalString(InStr, InContext, InMode, PyGlobalDict, PyLocalDict);
}

PyObject* FPythonScriptPlugin::EvalString(const TCHAR* InStr, const TCHAR* InContext, const int InMode, PyObject* InGlobalDict, PyObject* InLocalDict)
{
	FScopedSlowTask SlowTask(0.f, LOCTEXT("PythonScript_EvalString", "Running Python"));
	SlowTask.MakeDialogDelayed(1.0f);

	PyCompilerFlags *PyCompFlags = nullptr;

	PyArena* PyArena = PyArena_New();
	if (!PyArena)
	{
		return nullptr;
	}

	_mod* PyModule = PyParser_ASTFromString(TCHAR_TO_UTF8(InStr), TCHAR_TO_UTF8(InContext), InMode, PyCompFlags, PyArena);
	if (!PyModule)
	{
		PyArena_Free(PyArena);
		return nullptr;
	}

	typedef TPyPtr<PyCodeObject> PyCodeObjectPtr;
	PyCodeObjectPtr PyCodeObj = PyCodeObjectPtr::StealReference(PyAST_Compile(PyModule, TCHAR_TO_UTF8(InContext), PyCompFlags, PyArena));
	if (!PyCodeObj)
	{
		return nullptr;
	}

	return PyEval_EvalCode((PyUtil::FPyCodeObjectType*)PyCodeObj.Get(), InGlobalDict, InLocalDict);
}

void FPythonScriptPlugin::RunString(const TCHAR* InStr)
{
	FPyObjectPtr PyResult = FPyObjectPtr::StealReference(EvalString(InStr, TEXT("<string>"), Py_file_input));
	if (!PyResult)
	{
		PyUtil::LogPythonError();
	}

	FPyWrapperTypeReinstancer::Get().ProcessPending();
}

void FPythonScriptPlugin::RunFile(const TCHAR* InFile)
{
	auto ResolveFilePath = [InFile]() -> FString
	{
		// Favor the CWD
		if (FPaths::FileExists(InFile))
		{
			return FPaths::ConvertRelativePathToFull(InFile);
		}

		// Then test against each system path in order (as Python would)
		FPyObjectPtr PySysModule = FPyObjectPtr::StealReference(PyImport_ImportModule("sys"));
		if (PySysModule)
		{
			PyObject* PySysDict = PyModule_GetDict(PySysModule);

			PyObject* PyPathList = PyDict_GetItemString(PySysDict, "path");
			if (PyPathList)
			{
				const Py_ssize_t PyPathLen = PyList_Size(PyPathList);
				for (Py_ssize_t PyPathIndex = 0; PyPathIndex < PyPathLen; ++PyPathIndex)
				{
					PyObject* PyPathObj = PyList_GetItem(PyPathList, PyPathIndex);

					FString PyPathStr;
					if (PyPathObj && PyConversion::Nativize(PyPathObj, PyPathStr, PyConversion::ESetErrorState::No))
					{
						const FString PotentialFilePath = PyPathStr / InFile;
						if (FPaths::FileExists(PotentialFilePath))
						{
							return PotentialFilePath;
						}
					}
				}
			}
		}

		// Didn't find a match... we know this file doesn't exist, but we'll use this path in the error reporting
		return FPaths::ConvertRelativePathToFull(InFile);
	};

	const FString ResolvedFilePath = ResolveFilePath();

	FString FileStr;
	bool bLoaded = FFileHelper::LoadFileToString(FileStr, *ResolvedFilePath);
#if WITH_EDITOR
	if (CmdMenu)
	{
		CmdMenu->OnRunFile(ResolvedFilePath, bLoaded);
	}
#endif // WITH_EDITOR

	if (!bLoaded)
	{
		UE_LOG(LogPython, Error, TEXT("Could not load Python file '%s' (resolved from '%s')"), *ResolvedFilePath, InFile);
		return;
	}

	FPyObjectPtr PyFileGlobalDict = FPyObjectPtr::StealReference(PyDict_Copy(PyGlobalDict));
	FPyObjectPtr PyFileLocalDict = PyFileGlobalDict;
	{
		FPyObjectPtr PyResolvedFilePath;
		if (PyConversion::Pythonize(ResolvedFilePath, PyResolvedFilePath.Get(), PyConversion::ESetErrorState::No))
		{
			PyDict_SetItemString(PyFileGlobalDict, "__file__", PyResolvedFilePath);
		}
	}

	double ElapsedSeconds = 0.0;
	FPyObjectPtr PyResult;
	{
		FScopedDurationTimer Timer(ElapsedSeconds);

		// We can't just use PyRun_File here as Python isn't always built against the same version of the CRT as UE4, so we get a crash at the CRT layer
		PyResult = FPyObjectPtr::StealReference(EvalString(*FileStr, *ResolvedFilePath, Py_file_input, PyFileGlobalDict, PyFileLocalDict));
	}

	if (!PyResult)
	{
		PyUtil::LogPythonError();
	}

	FPyWrapperTypeReinstancer::Get().ProcessPending();

	if (FEngineAnalytics::IsAvailable())
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Duration"), ElapsedSeconds));
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("PythonScriptPlugin"), EventAttributes);
	}
}

void FPythonScriptPlugin::OnModuleDirtied(FName InModuleName)
{
	ImportUnrealModule(*InModuleName.ToString());
}

void FPythonScriptPlugin::OnModulesChanged(FName InModuleName, EModuleChangeReason InModuleChangeReason)
{
	switch (InModuleChangeReason)
	{
	case EModuleChangeReason::ModuleLoaded:
		FPyWrapperTypeRegistry::Get().GenerateWrappedTypesForModule(InModuleName);
		break;

	case EModuleChangeReason::ModuleUnloaded:
		FPyWrapperTypeRegistry::Get().OrphanWrappedTypesForModule(InModuleName);
		break;

	default:
		break;
	}
}

void FPythonScriptPlugin::OnContentPathMounted(const FString& InAssetPath, const FString& InFilesystemPath)
{
	PyUtil::AddSystemPath(FPaths::ConvertRelativePathToFull(InFilesystemPath / TEXT("Python")));
}

void FPythonScriptPlugin::OnContentPathDismounted(const FString& InAssetPath, const FString& InFilesystemPath)
{
	PyUtil::RemoveSystemPath(FPaths::ConvertRelativePathToFull(InFilesystemPath / TEXT("Python")));
}

#if WITH_EDITOR

void FPythonScriptPlugin::OnPrepareToCleanseEditorObject(UObject* InObject)
{
	FPyReferenceCollector::Get().PurgeUnrealObjectReferences(InObject, true);
}

#endif	// WITH_EDITOR

#endif	// WITH_PYTHON

IMPLEMENT_MODULE(FPythonScriptPlugin, PythonScriptPlugin)

#undef LOCTEXT_NAMESPACE
