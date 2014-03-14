// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "Box2DIntegration.h"

#if WITH_BOX2D

//////////////////////////////////////////////////////////////////////////
// 

class FBox2DVisualizer : public b2Draw
{
public:
	// b2Draw interface
	virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) OVERRIDE;
	virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) OVERRIDE;
	virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) OVERRIDE;
	virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) OVERRIDE;
	virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) OVERRIDE;
	virtual void DrawTransform(const b2Transform& xf) OVERRIDE;
	// End of b2Draw interface

	FBox2DVisualizer()
		: PDI(NULL)
		, View(NULL)
	{
		//@TODO:
		SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_centerOfMassBit);
	}

	void SetParameters(FPrimitiveDrawInterface* InPDI, const FSceneView* InView)
	{
		PDI = InPDI;
		View = InView;
	}

public:
	FPrimitiveDrawInterface* PDI;
	const FSceneView* View;

	static FBox2DVisualizer GlobalVisualizer;
};

FBox2DVisualizer FBox2DVisualizer::GlobalVisualizer;

FLinearColor ConvertBoxColor(const b2Color& BoxColor)
{
	return FLinearColor(BoxColor.r, BoxColor.g, BoxColor.b, 1.0f);
}

void FBox2DVisualizer::DrawPolygon(const b2Vec2* Vertices, int32 VertexCount, const b2Color& BoxColor)
{
	// Draw a closed polygon provided in CCW order.
	const FLinearColor Color = ConvertBoxColor(BoxColor);
	
	for (int32 Index = 0; Index < VertexCount; ++Index)
	{
		const int32 NextIndex = (Index + 1) % VertexCount;

		const FVector Start(FPhysicsIntegration2D::ConvertBoxVectorToUnreal(Vertices[Index]));
		const FVector End(FPhysicsIntegration2D::ConvertBoxVectorToUnreal(Vertices[NextIndex]));

		PDI->DrawLine(Start, End, Color, 0);
	}
}

void FBox2DVisualizer::DrawSolidPolygon(const b2Vec2* Vertices, int32 VertexCount, const b2Color& BoxColor)
{
	//@TODO: Draw a solid closed polygon provided in CCW order.
	DrawPolygon(Vertices, VertexCount, BoxColor);
}

void FBox2DVisualizer::DrawCircle(const b2Vec2& Center, float32 Radius, const b2Color& BoxColor)
{
	// Draw a circle.
	const FLinearColor Color = ConvertBoxColor(BoxColor);
	PDI->DrawPoint(FPhysicsIntegration2D::ConvertBoxVectorToUnreal(Center), Color, Radius, 0);
}

void FBox2DVisualizer::DrawSolidCircle(const b2Vec2& Center, float32 Radius, const b2Vec2& Axis, const b2Color& BoxColor)
{
//	const FLinearColor Color = ConvertBoxColor(BoxColor);
	/// Draw a solid circle.
}

void FBox2DVisualizer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& BoxColor)
{
	//const FLinearColor Color = ConvertBoxColor(BoxColor);
	/// Draw a line segment.
}

void FBox2DVisualizer::DrawTransform(const b2Transform& xf)
{
	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
}

//////////////////////////////////////////////////////////////////////////
// 

FPhysicsScene2D::FPhysicsScene2D(UWorld* AssociatedWorld)
	: UnrealWorld(AssociatedWorld)
{
	const b2Vec2 DefaultGravity(0.0f, -10.0f);

	World = new b2World(DefaultGravity);
	World->SetDebugDraw(&FBox2DVisualizer::GlobalVisualizer);

	StartPhysicsTickFunction.Target = this;
	StartPhysicsTickFunction.TickGroup = TG_StartPhysics;
	StartPhysicsTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);

	EndPhysicsTickFunction.Target = this;
	EndPhysicsTickFunction.TickGroup = TG_EndPhysics;
	EndPhysicsTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);
	EndPhysicsTickFunction.AddPrerequisite(UnrealWorld, StartPhysicsTickFunction);
}

FPhysicsScene2D::~FPhysicsScene2D()
{
	StartPhysicsTickFunction.UnRegisterTickFunction();
	EndPhysicsTickFunction.UnRegisterTickFunction();
}

//////////////////////////////////////////////////////////////////////////
// FStartPhysics2DTickFunction

void FStartPhysics2DTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target->UnrealWorld->bShouldSimulatePhysics)
	{
		const int32 VelocityIterations = 8;
		const int32 PositionIterations = 3;
		
		// Advance the world
		b2World* PhysicsWorld = Target->World;
		PhysicsWorld->Step(DeltaTime, VelocityIterations, PositionIterations);
		PhysicsWorld->ClearForces();

		// Pull data back
		for (b2Body* Body = PhysicsWorld->GetBodyList(); Body; )
		{
			b2Body* NextBody = Body->GetNext();

			if (FBodyInstance2D* UnrealBodyInstance = (FBodyInstance2D*)Body->GetUserData())
			{
				// See if the transform is actually different, and if so, move the component to match physics
				const FTransform NewTransform = UnrealBodyInstance->GetUnrealWorldTransform();	
				const FTransform& OldTransform = UnrealBodyInstance->OwnerComponentPtr->ComponentToWorld;
				if (!NewTransform.Equals(OldTransform))
				{
					const FVector MoveBy = NewTransform.GetLocation() - OldTransform.GetLocation();
					const FRotator NewRotation = NewTransform.Rotator();

					UnrealBodyInstance->OwnerComponentPtr->MoveComponent(MoveBy, NewRotation, false, NULL, MOVECOMP_SkipPhysicsMove);
				}
			}

			Body = NextBody;
		}
	}
}

FString FStartPhysics2DTickFunction::DiagnosticMessage()
{
	return TEXT("FStartPhysics2DTickFunction");
}

//////////////////////////////////////////////////////////////////////////
// FEndPhysics2DTickFunction

void FEndPhysics2DTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
}

FString FEndPhysics2DTickFunction::DiagnosticMessage()
{
	return TEXT("FEndPhysics2DTickFunction");
}


//////////////////////////////////////////////////////////////////////////
// FPhysicsIntegration2D

TMap< UWorld*, TSharedPtr<FPhysicsScene2D> > FPhysicsIntegration2D::WorldMappings;
FWorldDelegates::FWorldInitializationEvent::FDelegate FPhysicsIntegration2D::OnWorldCreatedDelegate;
FWorldDelegates::FWorldEvent::FDelegate FPhysicsIntegration2D::OnWorldDestroyedDelegate;

void FPhysicsIntegration2D::OnWorldCreated(UWorld* UnrealWorld, const UWorld::InitializationValues IVS)
{
	if (IVS.bCreatePhysicsScene)
	{
		WorldMappings.Add(UnrealWorld, MakeShareable(new FPhysicsScene2D(UnrealWorld)));
	}
}

void FPhysicsIntegration2D::OnWorldDestroyed(UWorld* UnrealWorld)
{
	WorldMappings.Remove(UnrealWorld);
}

void FPhysicsIntegration2D::InitializePhysics()
{
	OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&FPhysicsIntegration2D::OnWorldCreated);
	OnWorldDestroyedDelegate = FWorldDelegates::FWorldEvent::FDelegate::CreateStatic(&FPhysicsIntegration2D::OnWorldDestroyed);

	FWorldDelegates::OnPreWorldInitialization.Add(OnWorldCreatedDelegate);
	FWorldDelegates::OnPreWorldFinishDestroy.Add(OnWorldDestroyedDelegate);
}

void FPhysicsIntegration2D::ShutdownPhysics()
{
	FWorldDelegates::OnPreWorldInitialization.Remove(OnWorldCreatedDelegate);
	FWorldDelegates::OnPreWorldFinishDestroy.Remove(OnWorldDestroyedDelegate);

	check(WorldMappings.Num() == 0);
}

TSharedPtr<FPhysicsScene2D> FPhysicsIntegration2D::FindAssociatedScene(UWorld* Source)
{
	return WorldMappings.FindRef(Source);
}

b2World* FPhysicsIntegration2D::FindAssociatedWorld(UWorld* Source)
{
	if (FPhysicsScene2D* Scene = FindAssociatedScene(Source).Get())
	{
		return Scene->World;
	}
	else
	{
		return NULL;
	}
}

void FPhysicsIntegration2D::DrawDebugPhysics(UWorld* World, FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	if (b2World* World2D = FindAssociatedWorld(World))
	{
		//@TODO: HORRIBLE CODE
		FBox2DVisualizer::GlobalVisualizer.SetParameters(PDI, View);
		World2D->DrawDebugData();
		FBox2DVisualizer::GlobalVisualizer.SetParameters(NULL, NULL);
	}
}

#else

void FPhysicsIntegration2D::InitializePhysics()
{
}

void FPhysicsIntegration2D::ShutdownPhysics()
{
}

void FPhysicsIntegration2D::DrawDebugPhysics(UWorld* World, FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
}
#endif