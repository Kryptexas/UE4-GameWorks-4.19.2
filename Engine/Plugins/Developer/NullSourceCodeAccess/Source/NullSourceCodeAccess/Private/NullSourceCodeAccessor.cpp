// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NullSourceCodeAccessPrivatePCH.h"
#include "NullSourceCodeAccessor.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "NullSourceCodeAccessor"
bool FNullSourceCodeAccessor::CanAccessSourceCode() const
{
	return FPaths::FileExists(TEXT("/usr/bin/clang++"));
}

FName FNullSourceCodeAccessor::GetFName() const
{
	return FName("NullSourceCodeAccessor");
}

FText FNullSourceCodeAccessor::GetNameText() const 
{
	return LOCTEXT("NullDisplayName", "Null Source Code Access");
}

FText FNullSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("NullDisplayDesc", "Create a c++ project without an IDE installed.");
}

bool FNullSourceCodeAccessor::OpenSolution()
{
	return false;
}

bool FNullSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	return false;
}

bool FNullSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths) 
{
	return false;
}

bool FNullSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	return false;
}

bool FNullSourceCodeAccessor::SaveAllOpenDocuments() const
{
	return false;
}

void FNullSourceCodeAccessor::Tick(const float DeltaTime) 
{

}

#undef LOCTEXT_NAMESPACE
