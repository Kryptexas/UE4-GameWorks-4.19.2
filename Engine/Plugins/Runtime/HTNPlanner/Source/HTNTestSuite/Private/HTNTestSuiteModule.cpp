// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HTNTestSuiteModule.h"

#define LOCTEXT_NAMESPACE "HTNTestSuite"

class FHTNTestSuiteModule : public IHTNTestSuiteModule
{
};

IMPLEMENT_MODULE(FHTNTestSuiteModule, HTNTestSuiteModule)

#undef LOCTEXT_NAMESPACE
