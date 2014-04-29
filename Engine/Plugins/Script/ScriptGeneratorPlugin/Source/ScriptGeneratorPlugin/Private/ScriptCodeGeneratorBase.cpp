// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "ScriptGeneratorPluginPrivatePCH.h"
#include "ScriptCodeGeneratorBase.h"

FScriptCodeGeneratorBase::FScriptCodeGeneratorBase(const FString& InRootLocalPath, const FString& InRootBuildPath, const FString& InOutputDirectory)
{
	GeneratedCodePath = InOutputDirectory;
	RootLocalPath = InRootLocalPath;
	RootBuildPath = InRootBuildPath;
}

bool FScriptCodeGeneratorBase::SaveHeaderIfChanged(const FString& HeaderPath, const FString& NewHeaderContents)
{
	FString OriginalHeaderLocal;
	FFileHelper::LoadFileToString(OriginalHeaderLocal, *HeaderPath);

	const bool bHasChanged = OriginalHeaderLocal.Len() == 0 || FCString::Strcmp(*OriginalHeaderLocal, *NewHeaderContents);
	if (bHasChanged)
	{
		// save the updated version to a tmp file so that the user can see what will be changing
		const FString TmpHeaderFilename = HeaderPath + TEXT(".tmp");

		// delete any existing temp file
		IFileManager::Get().Delete(*TmpHeaderFilename, false, true);
		if (!FFileHelper::SaveStringToFile(NewHeaderContents, *TmpHeaderFilename))
		{
			UE_LOG(LogScriptGenerator, Warning, TEXT("Failed to save header export: '%s'"), *TmpHeaderFilename);
		}
		else
		{
			TempHeaders.Add(TmpHeaderFilename);
		}
	}

	return bHasChanged;
}

void FScriptCodeGeneratorBase::RenameTempFiles()
{
	// Rename temp headers
	for (auto& TempFilename : TempHeaders)
	{
		FString Filename = TempFilename.Replace(TEXT(".tmp"), TEXT(""));
		if (!IFileManager::Get().Move(*Filename, *TempFilename, true, true))
		{
			UE_LOG(LogScriptGenerator, Error, TEXT("%s"), *FString::Printf(TEXT("Couldn't write file '%s'"), *Filename));
		}
		else
		{
			UE_LOG(LogScriptGenerator, Log, TEXT("Exported updated script header: %s"), *Filename);
		}
	}
}

FString FScriptCodeGeneratorBase::RebaseToBuildPath(const FString& FileName) const
{
	FString NewFilename(FileName);
	if (RootLocalPath != RootBuildPath)
	{
		if (FPaths::MakePathRelativeTo(NewFilename, *RootLocalPath))
		{
			NewFilename = RootBuildPath / NewFilename;
		}
	}
	return NewFilename;
}

FString FScriptCodeGeneratorBase::GetClassNameCPP(UClass* Class)
{
	return FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
}

FString FScriptCodeGeneratorBase::GetScriptHeaderForClass(UClass* Class)
{
	return GeneratedCodePath / (Class->GetName() + TEXT(".script.h"));
}

bool FScriptCodeGeneratorBase::CanExportClass(UClass* Class)
{
	bool bCanExport = !(Class->GetClassFlags() & CLASS_Temporary) && // Don't export temporary classes
		(Class->ClassFlags & (CLASS_RequiredAPI | CLASS_MinimalAPI)) && // Don't export classes that don't export DLL symbols
		!ExportedClasses.Contains(Class->GetFName()); // Don't export classes that have already been exported

	if (bCanExport)
	{
		// Only export functions from selected modules
		static struct FSupportedModules
		{
			TArray<FString> SupportedScriptModules;
			FSupportedModules()
			{
				GConfig->GetArray(TEXT("Plugins"), TEXT("ScriptSupportedModules"), SupportedScriptModules, GEngineIni);
			}
		} SupportedModules;

		bCanExport = SupportedModules.SupportedScriptModules.Contains(Class->GetOutermost()->GetName());
	}
	return bCanExport;
}

bool FScriptCodeGeneratorBase::CanExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function)
{
	// We don't support delegates and non-public functions
	if ((Function->FunctionFlags & FUNC_Delegate) ||
		!(Function->FunctionFlags & FUNC_Public))
	{
		return false;
	}

	// Function must be DLL exported
	if ((Class->ClassFlags & CLASS_RequiredAPI) == 0 &&
		(Function->FunctionFlags & FUNC_RequiredAPI) == 0)
	{
		return false;
	}

	// Reject if any of the parameter types is unsupported yet
	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		UProperty* Param = *ParamIt;
		if (Param->IsA(UArrayProperty::StaticClass()) ||
			  Param->ArrayDim > 1 ||
			  Param->IsA(UDelegateProperty::StaticClass()) ||
				Param->IsA(UMulticastDelegateProperty::StaticClass()) ||
			  Param->IsA(UWeakObjectProperty::StaticClass()) ||
			  Param->IsA(UInterfaceProperty::StaticClass()))
		{
			return false;
		}
	}

	return true;
}

bool FScriptCodeGeneratorBase::CanExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property)
{
	// Property must be DLL exported
	if (!(Class->ClassFlags & CLASS_RequiredAPI))
	{
		return false;
	}

	// Only public, editable properties can be exported
	if (!Property->HasAnyFlags(RF_Public) || 
		  (Property->PropertyFlags & CPF_Protected) || 
			!(Property->PropertyFlags & CPF_Edit))
	{
		return false;
	}


	// Reject if it's one of the unsupported types (yet)
	if (Property->IsA(UArrayProperty::StaticClass()) ||
		Property->ArrayDim > 1 ||
		Property->IsA(UDelegateProperty::StaticClass()) ||
		Property->IsA(UMulticastDelegateProperty::StaticClass()) ||
		Property->IsA(UWeakObjectProperty::StaticClass()) ||
		Property->IsA(UInterfaceProperty::StaticClass()) ||
		Property->IsA(UStructProperty::StaticClass()))
	{
		return false;
	}

	return true;
}

int32 FScriptCodeGeneratorBase::GenerateExportedPropertyTable(const FString& ClassNameCPP, UClass* Class, FString& OutFunction, TArray<UProperty*>& OutExportedProperties)
{
	OutExportedProperties.Empty();
	OutFunction = FString::Printf(TEXT("UProperty* %s_GetExportedProperty(int32 PropertyIndex)\r\n{\r\n"), *Class->GetName());
	OutFunction += FString::Printf(TEXT("\tstatic UClass* Class = %s::StaticClass();\r\n"), *ClassNameCPP);
	OutFunction += TEXT("\tstatic UProperty* PropertyTable[] =\r\n\t{\r\n");
	
	for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		if (CanExportProperty(ClassNameCPP, Class, Property))
		{
			if (OutExportedProperties.Num() > 0)
			{
				OutFunction += TEXT(",\r\n");
			}
			OutFunction += FString::Printf(TEXT("\t\tFindScriptPropertyHelper(Class, TEXT(\"%s\"))"), *Property->GetName());
			OutExportedProperties.Add(Property);
		}
	}

	OutFunction += TEXT("\r\n\t};\r\n\tcheck(PropertyIndex >= 0 && PropertyIndex < ARRAY_COUNT(PropertyTable));\r\n");
	OutFunction += TEXT("\treturn PropertyTable[PropertyIndex];\r\n}\r\n\r\n");

	return OutExportedProperties.Num();
}