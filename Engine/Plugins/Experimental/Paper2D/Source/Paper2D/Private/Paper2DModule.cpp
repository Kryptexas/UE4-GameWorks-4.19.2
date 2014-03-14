// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "Paper2DModule.h"

#include "Paper2D.generated.inl"

#include "Physics/Box2DIntegration.h"

//////////////////////////////////////////////////////////////////////////
// FPaper2DModule

class FPaper2DModule : public IPaper2DModuleInterface
{
public:
	virtual void StartupModule() OVERRIDE
	{
		FPhysicsIntegration2D::InitializePhysics();

		check(GConfig);

		if (!GConfig->GetVector(TEXT("Paper2D"), TEXT("PaperAxisX"), PaperAxisX, GEngineIni))
		{
			PaperAxisX = FVector(1.0f, 0.0f, 0.0f);
		}
		if (!GConfig->GetVector(TEXT("Paper2D"), TEXT("PaperAxisY"), PaperAxisY, GEngineIni))
		{
			PaperAxisY = FVector(0.0f, 0.0f, 1.0f);
		}

		PaperAxisZ = FVector::CrossProduct(PaperAxisX, PaperAxisY);
	}

	virtual void ShutdownModule() OVERRIDE
	{
		FPhysicsIntegration2D::ShutdownPhysics();
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FPaper2DModule, Paper2D);
DEFINE_LOG_CATEGORY(LogPaper2D);

FVector PaperAxisX(1.0f, 0.0f, 0.0f);
FVector PaperAxisY(0.0f, 0.0f, 1.0f);
FVector PaperAxisZ(0.0f, 1.0f, 0.0f);
