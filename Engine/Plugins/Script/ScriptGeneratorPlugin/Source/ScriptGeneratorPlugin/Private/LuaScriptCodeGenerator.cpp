// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "ScriptGeneratorPluginPrivatePCH.h"
#include "LuaScriptCodeGenerator.h"

// Supported structs
static FName Name_Vector("Vector");
static FName Name_Vector4("Vector4");
static FName Name_Quat("Quat");
static FName Name_Transform("Transform");

FLuaScriptCodeGenerator::FLuaScriptCodeGenerator(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory)
: FScriptCodeGeneratorBase(RootLocalPath, RootBuildPath, OutputDirectory)
{

}

FString FLuaScriptCodeGenerator::GenerateWrapperFunctionDeclaration(const FString& ClassNameCPP, UClass* Class, UFunction* Function)
{
	return GenerateWrapperFunctionDeclaration(ClassNameCPP, Class, Function->GetName());
}

FString FLuaScriptCodeGenerator::GenerateWrapperFunctionDeclaration(const FString& ClassNameCPP, UClass* Class, const FString& FunctionName)
{
	return FString::Printf(TEXT("int32 %s_%s(lua_State* InScriptContext)"), *Class->GetName(), *FunctionName);
}

FString FLuaScriptCodeGenerator::GenerateFunctionParamDeclaration(const FString& ClassNameCPP, UClass* Class, UFunction* Function, UProperty* Param, int32 ParamIndex)
{
	// In Lua, the first param index on the stack is 1 and it's the object we're invoking the function on
	ParamIndex += 2;
	FString Initializer;
	if (Param->IsA(UIntProperty::StaticClass()))
	{
		Initializer = TEXT("(luaL_checkint");
	}
	else if (Param->IsA(UFloatProperty::StaticClass()))
	{
		Initializer = TEXT("(float)(luaL_checknumber");
	}
	else if (Param->IsA(UStrProperty::StaticClass()))
	{
		Initializer = TEXT("ANSI_TO_TCHAR(luaL_checkstring");
	}
	else if (Param->IsA(UNameProperty::StaticClass()))
	{
		Initializer = TEXT("FName(luaL_checkstring");
	}
	else if (Param->IsA(UBoolProperty::StaticClass()))
	{
		Initializer = TEXT("!!(lua_toboolean");
	}
	else if (Param->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty* StructProp = CastChecked<UStructProperty>(Param);
		if (StructProp->Struct->GetFName() == Name_Vector)
		{
			Initializer = TEXT("(FLuaVector::Get");
		}
		else if (StructProp->Struct->GetFName() == Name_Vector4)
		{
			Initializer = TEXT("(FLuaVector4::Get");
		}
		else if (StructProp->Struct->GetFName() == Name_Quat)
		{
			Initializer = TEXT("(FLuaQuat::Get");
		}
		else if (StructProp->Struct->GetFName() == Name_Transform)
		{
			Initializer = TEXT("(FLuaTransform::Get");
		}
		else
		{
			FError::Throwf(TEXT("Unsupported function param struct type: %s"), *StructProp->Struct->GetName());
		}
	}
	else if (Param->IsA(UClassProperty::StaticClass()))
	{
		Initializer = TEXT("(UClass*)(lua_touserdata");
	}
	else if (Param->IsA(UObjectPropertyBase::StaticClass()))
	{
		Initializer = FString::Printf(TEXT("(%s)(lua_touserdata"), *GetPropertyTypeCPP(Param, CPPF_ArgumentOrReturnValue), ParamIndex);
	}
	else
	{
		FError::Throwf(TEXT("Unsupported function param type: %s"), *Param->GetClass()->GetName());
	}

	return FString::Printf(TEXT("%s %s = %s(InScriptContext, %d));"), *GetPropertyTypeCPP(Param, CPPF_ArgumentOrReturnValue), *Param->GetName(), *Initializer, ParamIndex);
}

FString FLuaScriptCodeGenerator::GenerateObjectDeclarationFromContext(const FString& ClassNameCPP, UClass* Class)
{
	return FString::Printf(TEXT("%s* Obj = (%s*)lua_touserdata(InScriptContext, 1);"), *ClassNameCPP, *ClassNameCPP);
}

FString FLuaScriptCodeGenerator::GenerateReturnValueHandler(const FString& ClassNameCPP, UClass* Class, UFunction* Function, UProperty* ReturnValue)
{
	if (ReturnValue)
	{
		const FString ReturnValueName = ReturnValue->GetName();
		FString Initializer;		
		if (ReturnValue->IsA(UIntProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushinteger(InScriptContext, %s);"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UFloatProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushnumber(InScriptContext, %s);"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UStrProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushstring(InScriptContext, TCHAR_TO_ANSI(*%s));"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UNameProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushstring(InScriptContext, TCHAR_TO_ANSI(*%s.ToString()));"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UBoolProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushboolean(InScriptContext, %s);"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UStructProperty::StaticClass()))
		{
			UStructProperty* StructProp = CastChecked<UStructProperty>(ReturnValue);
			if (StructProp->Struct->GetFName() == Name_Vector)
			{
				Initializer = FString::Printf(TEXT("FLuaVector::Return(InScriptContext, %s);"), *ReturnValueName);
			}
			else if (StructProp->Struct->GetFName() == Name_Vector4)
			{
				Initializer = FString::Printf(TEXT("FLuaVector4::Return(InScriptContext, %s);"), *ReturnValueName);
			}
			else if (StructProp->Struct->GetFName() == Name_Quat)
			{
				Initializer = FString::Printf(TEXT("FLuaQuat::Return(InScriptContext, %s);"), *ReturnValueName);
			}
			else if (StructProp->Struct->GetFName() == Name_Transform)
			{
				Initializer = FString::Printf(TEXT("FLuaTransform::Return(InScriptContext, %s);"), *ReturnValueName);
			}
			else
			{
				FError::Throwf(TEXT("Unsupported function return value struct type: %s"), *StructProp->Struct->GetName());
			}
		}
		else if (ReturnValue->IsA(UObjectPropertyBase::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("lua_pushlightuserdata(InScriptContext, %s);"), *ReturnValueName);
		}
		else
		{
			FError::Throwf(TEXT("Unsupported function return type: %s"), *ReturnValue->GetClass()->GetName());
		}

		return FString::Printf(TEXT("%s\r\n\treturn 1;"), *Initializer);
	}
	else
	{
		return TEXT("return 0;");
	}	
}

bool FLuaScriptCodeGenerator::CanExportClass(UClass* Class)
{
	bool bCanExport = FScriptCodeGeneratorBase::CanExportClass(Class);
	if (bCanExport)
	{
		if (bCanExport)
		{
			const FString ClassNameCPP = GetClassNameCPP(Class);
			// No functions to export? Don't bother exporting the class.
			bool bHasMembersToExport = false;
			for (TFieldIterator<UFunction> FuncIt(Class); !bHasMembersToExport && FuncIt; ++FuncIt)
			{
				UFunction* Function = *FuncIt;
				if (CanExportFunction(ClassNameCPP, Class, Function))
				{
					bHasMembersToExport = true;
				}
			}
			bCanExport = bHasMembersToExport;
		}
	}
	return bCanExport;
}

bool FLuaScriptCodeGenerator::CanExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function)
{
	bool bExport = FScriptCodeGeneratorBase::CanExportFunction(ClassNameCPP, Class, Function);
	if (bExport)
	{
		for (TFieldIterator<UProperty> ParamIt(Function); bExport && ParamIt; ++ParamIt)
		{
			UProperty* Param = *ParamIt;
			if (Param->IsA(UStructProperty::StaticClass()))
			{
				UStructProperty* StructProp = CastChecked<UStructProperty>(Param);
				if (StructProp->Struct->GetFName() != Name_Vector &&
					StructProp->Struct->GetFName() != Name_Vector4 &&
					StructProp->Struct->GetFName() != Name_Quat &&
					StructProp->Struct->GetFName() != Name_Transform)
				{
					bExport = false;
				}
			}
			else if (!Param->IsA(UIntProperty::StaticClass()) &&
				  !Param->IsA(UFloatProperty::StaticClass()) &&
					!Param->IsA(UStrProperty::StaticClass()) &&
					!Param->IsA(UNameProperty::StaticClass()) &&
					!Param->IsA(UBoolProperty::StaticClass()) &&
					!Param->IsA(UObjectPropertyBase::StaticClass()) &&
					!Param->IsA(UClassProperty::StaticClass()))
			{
				bExport = false;
			}
		}
	}
	return bExport;
}

FString FLuaScriptCodeGenerator::ExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function)
{
	FString GeneratedGlue = GenerateWrapperFunctionDeclaration(ClassNameCPP, Class, Function);
	GeneratedGlue += TEXT("\r\n{\r\n");

	UProperty* ReturnValue = NULL;
	UClass* FuncSuper = NULL;
	
	if (Function->GetOwnerClass() != Class)
	{
		// Find the base definition of the function
		if (ExportedClasses.Contains(Function->GetOwnerClass()->GetFName()))
		{
			FuncSuper = Function->GetOwnerClass();
		}
	}

	FString FunctionBody;
	if (FuncSuper == NULL)
	{
		FunctionBody += FString::Printf(TEXT("\t%s\r\n"), *GenerateObjectDeclarationFromContext(ClassNameCPP, Class));

		FString FunctionCallArguments;
		FString ReturnValueDeclaration;
		int32 ParamIndex = 0;
		for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt, ++ParamIndex)
		{
			UProperty* Param = *ParamIt;
			if (!(Param->GetPropertyFlags() & CPF_ReturnParm))
			{
				FunctionBody += FString::Printf(TEXT("\t%s\r\n"), *GenerateFunctionParamDeclaration(ClassNameCPP, Class, Function, Param, ParamIndex));
				FunctionCallArguments += (FunctionCallArguments.Len() > 0) ? FString::Printf(TEXT(", %s"), *Param->GetName()) : Param->GetName();
			}
			else
			{
				ReturnValue = Param;
				ReturnValueDeclaration += FString::Printf(TEXT("%s %s = "), *GetPropertyTypeCPP(Param, CPPF_ArgumentOrReturnValue), *Param->GetName());
			}
		}		
		FunctionBody += FString::Printf(TEXT("\t%sObj->%s(%s);\r\n"), *ReturnValueDeclaration, *Function->GetName(), *FunctionCallArguments);
		FunctionBody += FString::Printf(TEXT("\t%s\r\n"), *GenerateReturnValueHandler(ClassNameCPP, Class, Function, ReturnValue));
	}
	else
	{
		FunctionBody = FString::Printf(TEXT("\treturn %s_%s(InScriptContext);\r\n"), *FuncSuper->GetName(), *Function->GetName());
	}

	GeneratedGlue += FunctionBody;
	GeneratedGlue += TEXT("}\r\n\r\n");

	auto& Exports = ClassExportedFunctions.FindOrAdd(Class);
	Exports.Add(Function->GetFName());

	return GeneratedGlue;
}

FString FLuaScriptCodeGenerator::ExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property)
{
	FString GeneratedGlue;
	return GeneratedGlue;
}

FString FLuaScriptCodeGenerator::ExportAdditionalClassGlue(const FString& ClassNameCPP, UClass* Class)
{
	FString GeneratedGlue;

	const FString ClassName = Class->GetName();

	// Constructor and destructor
	if (!(Class->GetClassFlags() & CLASS_Abstract))
	{
		GeneratedGlue += GenerateWrapperFunctionDeclaration(ClassNameCPP, Class, TEXT("New"));
		GeneratedGlue += TEXT("\r\n{\r\n");
		GeneratedGlue += FString::Printf(TEXT("\tUObject* Obj = NewObject<%s>();\r\n"), *ClassNameCPP);
		GeneratedGlue += TEXT("\tif (Obj)\r\n\t{\r\n");
		GeneratedGlue += TEXT("\t\tFScriptObjectReferencer::Get().AddObjectReference(Obj);\r\n");
		GeneratedGlue += TEXT("\t}\r\n");
		GeneratedGlue += TEXT("\tlua_pushlightuserdata(InScriptContext, Obj);\r\n");
		GeneratedGlue += TEXT("\treturn 1;\r\n");
		GeneratedGlue += TEXT("}\r\n\r\n");

		GeneratedGlue += GenerateWrapperFunctionDeclaration(ClassNameCPP, Class, TEXT("Destroy")); 
		GeneratedGlue += TEXT("\r\n{\r\n");
		GeneratedGlue += FString::Printf(TEXT("\t%s\r\n"), *GenerateObjectDeclarationFromContext(ClassNameCPP, Class));
		GeneratedGlue += TEXT("\tif (Obj)\r\n\t{\r\n");
		GeneratedGlue += TEXT("\t\tFScriptObjectReferencer::Get().RemoveObjectReference(Obj);\r\n");
		GeneratedGlue += TEXT("\t}\r\n\treturn 0;\r\n");
		GeneratedGlue += TEXT("}\r\n\r\n");
	}

	// Class: Equivalent of StaticClass()
	GeneratedGlue += GenerateWrapperFunctionDeclaration(ClassNameCPP, Class, TEXT("Class"));
	GeneratedGlue += TEXT("\r\n{\r\n");
	GeneratedGlue += FString::Printf(TEXT("\tUClass* Class = %s::StaticClass();\r\n"), *ClassNameCPP);
	GeneratedGlue += TEXT("\tlua_pushlightuserdata(InScriptContext, Class);\r\n");
	GeneratedGlue += TEXT("\treturn 1;\r\n");
	GeneratedGlue += TEXT("}\r\n\r\n");

	// Library
	GeneratedGlue += FString::Printf(TEXT("static const luaL_Reg %s_Lib[] =\r\n{\r\n"), *ClassName);
	if (!(Class->GetClassFlags() & CLASS_Abstract))
	{
		GeneratedGlue += FString::Printf(TEXT("\t{ \"New\", %s_New },\r\n"), *ClassName);
		GeneratedGlue += FString::Printf(TEXT("\t{ \"Destroy\", %s_Destroy },\r\n"), *ClassName);
		GeneratedGlue += FString::Printf(TEXT("\t{ \"Class\", %s_Class },\r\n"), *ClassName);
	}
	auto Exports = ClassExportedFunctions.Find(Class);
	if (Exports)
	{
		for (auto& FunctionName : *Exports)
		{
			GeneratedGlue += FString::Printf(TEXT("\t{ \"%s\", %s_%s },\r\n"), *FunctionName.ToString(), *ClassName, *FunctionName.ToString());
		}
	}
	GeneratedGlue += TEXT("\t{ NULL, NULL }\r\n};\r\n\r\n");

	return GeneratedGlue;
}

void FLuaScriptCodeGenerator::ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged)
{
	if (!CanExportClass(Class))
	{
		return;
	}
	
	UE_LOG(LogScriptGenerator, Log, TEXT("Exporting class %s"), *Class->GetName());
	
	ExportedClasses.Add(Class->GetFName());
	LuaExportedClasses.Add(Class);
	AllSourceClassHeaders.Add(SourceHeaderFilename);

	const FString ClassGlueFilename = GetScriptHeaderForClass(Class);
	AllScriptHeaders.Add(ClassGlueFilename);	

	const FString ClassNameCPP = GetClassNameCPP(Class);
	FString GeneratedGlue(TEXT("#pragma once\r\n\r\n"));		

	for (UClass* Super = Class->GetSuperClass(); Super != NULL; Super = Super->GetSuperClass())
	{
		FString SuperScriptHeader = GetScriptHeaderForClass(Super);
		if (FPaths::FileExists(SuperScriptHeader))
		{
			// Re-base to make sure we're including the right files on a remote machine
			FString NewFilename(RebaseToBuildPath(SuperScriptHeader));
			GeneratedGlue += FString::Printf(TEXT("#include \"%s\"\r\n\r\n"), *NewFilename);
			break;
		}
	}

	for (TFieldIterator<UFunction> FuncIt(Class /*, EFieldIteratorFlags::ExcludeSuper*/); FuncIt; ++FuncIt)
	{
		UFunction* Function = *FuncIt;
		if (CanExportFunction(ClassNameCPP, Class, Function))
		{
			GeneratedGlue += ExportFunction(ClassNameCPP, Class, Function);
		}
	}

	GeneratedGlue += ExportAdditionalClassGlue(ClassNameCPP, Class);

	SaveHeaderIfChanged(ClassGlueFilename, GeneratedGlue);
}

void FLuaScriptCodeGenerator::FinishExport()
{
	GlueAllGeneratedFiles();
	RenameTempFiles();
}

void FLuaScriptCodeGenerator::GlueAllGeneratedFiles()
{
	// Generate inl library file
	FString LibGlueFilename = GeneratedCodePath / TEXT("GeneratedScriptLibraries.inl");
	FString LibGlue;

	// Include all source header files
	for (auto& HeaderFilename : AllSourceClassHeaders)
	{
		// Re-base to make sure we're including the right files on a remote machine
		FString NewFilename(RebaseToBuildPath(HeaderFilename));
		LibGlue += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
	}

	// Include all script glue headers
	for (auto& HeaderFilename : AllScriptHeaders)
	{
		// Re-base to make sure we're including the right files on a remote machine
		FString NewFilename(RebaseToBuildPath(HeaderFilename));
		LibGlue += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
	}

	LibGlue += TEXT("\r\nvoid LuaRegisterExportedClasses(lua_State* InScriptContext)\r\n{\r\n");
	for (auto Class : LuaExportedClasses)
	{
		LibGlue += FString::Printf(TEXT("\tFLuaUtils::RegisterLibrary(InScriptContext, %s_Lib, \"%s\");\r\n"), *Class->GetName(), *Class->GetName());
	}
	LibGlue += TEXT("}\r\n\r\n");

	SaveHeaderIfChanged(LibGlueFilename, LibGlue);
}
