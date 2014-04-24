// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"
#include "Manifest.h"
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

FManifest FManifest::LoadFromFile(const FString& Filename)
{
	FManifest Result;

	FString FilenamePath = FPaths::GetPath(Filename);

	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Filename))
		FError::Throwf(TEXT("Unable to load manifest: %s"), *Filename);

	auto RootObject = TSharedPtr<FJsonObject>();
	auto Reader     = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, RootObject))
		FError::Throwf(TEXT("Manifest is malformed: %s"), *Filename);

	TArray<TSharedPtr<FJsonValue>> ModulesArray;

	GetJsonFieldValue(Result.UseRelativePaths, RootObject, TEXT("UseRelativePaths"), TEXT("{manifest root}"));
	GetJsonFieldValue(Result.RootLocalPath,    RootObject, TEXT("RootLocalPath"),    TEXT("{manifest root}"));
	GetJsonFieldValue(Result.RootBuildPath,    RootObject, TEXT("RootBuildPath"),    TEXT("{manifest root}"));
	GetJsonFieldValue(ModulesArray,            RootObject, TEXT("Modules"),          TEXT("{manifest root}"));

	Result.RootLocalPath = FPaths::ConvertRelativePathToFull(FilenamePath, Result.RootLocalPath);
	Result.RootBuildPath = FPaths::ConvertRelativePathToFull(FilenamePath, Result.RootBuildPath);

	// Ensure directories end with a slash, because this aids their use with FPaths::MakePathRelativeTo.
	if (!Result.RootLocalPath.EndsWith(TEXT("/"))) { Result.RootLocalPath += TEXT("/"); }
	if (!Result.RootBuildPath.EndsWith(TEXT("/"))) { Result.RootBuildPath += TEXT("/"); }

	int32 ModuleIndex = 0;
	for (auto Module : ModulesArray)
	{
		auto ModuleObj = Module->AsObject();

		TArray<TSharedPtr<FJsonValue>> ClassesHeaders;
		TArray<TSharedPtr<FJsonValue>> PublicHeaders;
		TArray<TSharedPtr<FJsonValue>> PrivateHeaders;

		Result.Modules.AddZeroed();
		auto& KnownModule = Result.Modules.Last();

		FString Outer = FString::Printf(TEXT("Modules[%d]"), ModuleIndex);

		GetJsonFieldValue(KnownModule.Name,                      ModuleObj, TEXT("Name"),                     *Outer);
		GetJsonFieldValue(KnownModule.BaseDirectory,             ModuleObj, TEXT("BaseDirectory"),            *Outer);
		GetJsonFieldValue(KnownModule.GeneratedIncludeDirectory, ModuleObj, TEXT("OutputDirectory"),          *Outer);
		GetJsonFieldValue(KnownModule.SaveExportedHeaders,       ModuleObj, TEXT("SaveExportedHeaders"),      *Outer);
		GetJsonFieldValue(ClassesHeaders,                        ModuleObj, TEXT("ClassesHeaders"),           *Outer);
		GetJsonFieldValue(PublicHeaders,                         ModuleObj, TEXT("PublicHeaders"),            *Outer);
		GetJsonFieldValue(PrivateHeaders,                        ModuleObj, TEXT("PrivateHeaders"),           *Outer);
		GetJsonFieldValue(KnownModule.PCH,                       ModuleObj, TEXT("PCH"),                      *Outer);
		GetJsonFieldValue(KnownModule.GeneratedCPPFilenameBase,  ModuleObj, TEXT("GeneratedCPPFilenameBase"), *Outer);

		KnownModule.LongPackageName = FPackageName::ConvertToLongScriptPackageName(*KnownModule.Name);

		// Convert relative paths
		KnownModule.BaseDirectory             = FPaths::ConvertRelativePathToFull(FilenamePath, KnownModule.BaseDirectory);
		KnownModule.GeneratedIncludeDirectory = FPaths::ConvertRelativePathToFull(FilenamePath, KnownModule.GeneratedIncludeDirectory);
		KnownModule.GeneratedCPPFilenameBase  = FPaths::ConvertRelativePathToFull(FilenamePath, KnownModule.GeneratedCPPFilenameBase);

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

	return Result;
}
