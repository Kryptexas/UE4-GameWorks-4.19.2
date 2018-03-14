// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IncludePython.h"
#include "PyUtil.h"
#include "PyPtr.h"
#include "Misc/CoreMisc.h"
#include "Modules/ModuleInterface.h"
#include "HAL/IConsoleManager.h"

class FPythonScriptPlugin;

#if WITH_PYTHON

/**
 * Executor for "Python" commands
 */
class FPythonCommandExecutor : public IConsoleCommandExecutor
{
public:
	FPythonCommandExecutor(FPythonScriptPlugin* InPythonScriptPlugin);

	static FName StaticName();
	virtual FName GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDescription() const override;
	virtual FText GetHintText() const override;
	virtual void GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out) override;
	virtual void GetExecHistory(TArray<FString>& Out) override;
	virtual bool Exec(const TCHAR* Input) override;
	virtual bool AllowHotKeyClose() const override;
	virtual bool AllowMultiLine() const override;

private:
	FPythonScriptPlugin* PythonScriptPlugin;
};

/**
 *
 */
struct IPythonCommandMenu
{
	virtual ~IPythonCommandMenu() {}

	virtual void OnStartupMenu() = 0;
	virtual void OnShutdownMenu() = 0;

	virtual void OnRunFile(const FString& InFile, bool bAdd) = 0;
};
#endif	// WITH_PYTHON

class FPythonScriptPlugin : public IModuleInterface, public FSelfRegisteringExec
{
public:
	FPythonScriptPlugin();

	/** Get this module */
	static FPythonScriptPlugin* Get();

	//~ IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//~ FSelfRegisteringExec interface
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

#if WITH_PYTHON
	/** 
	 * Import the given module into the "unreal" package.
	 * This function will take the given name and attempt to import either "unreal_{name}" or "_unreal_{name}" into the "unreal" package as "unreal.{name}".
	 */
	void ImportUnrealModule(const TCHAR* InModuleName);

	void HandlePythonExecCommand(const TCHAR* InPythonCommand);

	PyObject* EvalString(const TCHAR* InStr, const TCHAR* InContext, const int InMode);
	PyObject* EvalString(const TCHAR* InStr, const TCHAR* InContext, const int InMode, PyObject* InGlobalDict, PyObject* InLocalDict);

	void RunString(const TCHAR* InStr);

	void RunFile(const TCHAR* InFile);
#endif	// WITH_PYTHON

private:
#if WITH_PYTHON
	void InitializePython();

	void ShutdownPython();

	void OnModuleDirtied(FName InModuleName);

	void OnModulesChanged(FName InModuleName, EModuleChangeReason InModuleChangeReason);

	void OnContentPathMounted(const FString& InAssetPath, const FString& InFilesystemPath);

	void OnContentPathDismounted(const FString& InAssetPath, const FString& InFilesystemPath);

#if WITH_EDITOR
	void OnPrepareToCleanseEditorObject(UObject* InObject);
#endif	// WITH_EDITOR

	FPythonCommandExecutor CmdExec;
	IPythonCommandMenu* CmdMenu;
	FDelegateHandle ReinstanceTickerHandle;

	PyUtil::FPyApiBuffer PyProgramName;
	PyUtil::FPyApiBuffer PyHomePath;
	TArray<PyUtil::FPyApiBuffer> PyCommandLineArgs;
	TArray<PyUtil::FPyApiChar*> PyCommandLineArgPtrs;
	FPyObjectPtr PyGlobalDict;
	FPyObjectPtr PyLocalDict;
	FPyObjectPtr PyUnrealModule;
	bool bInitialized;
#endif	// WITH_PYTHON


};
