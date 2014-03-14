// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "Box2DIntegration.h"

//////////////////////////////////////////////////////////////////////////
// FBodyInstance2D

FBodyInstance2D::FBodyInstance2D()
{
#if WITH_BOX2D
	BodyInstancePtr = NULL;
#endif
	bSimulatePhysics = true;
}

//@TODO: Everything about this block of code...
void FBodyInstance2D::InitBody(class UBodySetup2D* Setup, const UPaperSprite* SpriteSetupHack, const FTransform& Transform, class UPrimitiveComponent* PrimComp)
{
#if WITH_BOX2D && 0 //@TODO: DISABLED BOX2D FOR NOW!!!
	if (b2World* World = FPhysicsIntegration2D::FindAssociatedWorld(PrimComp->GetWorld()))
	{
		if ((SpriteSetupHack->CollisionData.Num() > 0) && ((SpriteSetupHack->CollisionData.Num() % 3) == 0))
		{
			OwnerComponentPtr = PrimComp;

			b2BodyDef BodyDefinition;
			BodyDefinition.type = bSimulatePhysics ? b2_dynamicBody : b2_kinematicBody;

			BodyInstancePtr = World->CreateBody(&BodyDefinition);
			BodyInstancePtr->SetUserData(this);

			const FVector2D PivotPos = FVector2D::ZeroVector;
			const FVector2D OffsetPos = FVector2D(SpriteSetupHack->SourceDimension.X*-0.5f, SpriteSetupHack->SourceDimension.Y*-0.5f) + PivotPos;

			for (int32 TriangleIndex = 0; TriangleIndex < SpriteSetupHack->CollisionData.Num() / 3; ++TriangleIndex)
			{
				b2Vec2 Verts[3];
				for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
				{
					const FVector2D SourceVert = SpriteSetupHack->CollisionData[(TriangleIndex * 3) + VertexIndex] + OffsetPos;
					Verts[VertexIndex] = FPhysicsIntegration2D::ConvertUnrealVectorToBox(FVector(SourceVert.X, 0.0f, SourceVert.Y));
				}
				
				b2PolygonShape Triangle;
				Triangle.Set(Verts, 3);

				b2FixtureDef FixtureDef;
				FixtureDef.shape = &Triangle;
				FixtureDef.density = 1.0f; //@TODO:
				FixtureDef.friction = 0.3f;//@TODO:

				BodyInstancePtr->CreateFixture(&FixtureDef);
			}

#if 0
			const b2Vec2 HalfBoxSize = FPhysicsIntegration2D::ConvertUnrealVectorToBox(FVector(SpriteSetupHack->BoundingBoxSize.X * 0.5f, 0.0f, SpriteSetupHack->BoundingBoxSize.Y * 0.5f));
			const b2Vec2 BoxCenter = FPhysicsIntegration2D::ConvertUnrealVectorToBox(FVector(
				SpriteSetupHack->BoundingBoxPosition.X,
				0.0f,
				SpriteSetupHack->BoundingBoxPosition.Y));

			b2PolygonShape DynamicBox;
			DynamicBox.SetAsBox(HalfBoxSize.x, HalfBoxSize.y, BoxCenter, 0.0f);

			b2FixtureDef FixtureDef;
			FixtureDef.shape = &DynamicBox;
			FixtureDef.density = 1.0f; //@TODO:
			FixtureDef.friction = 0.3f;//@TODO:

			BodyInstancePtr->CreateFixture(&FixtureDef);
#endif

			SetBodyTransform(Transform);
		}
	}
	else
	{
		//@TODO: Error out?
	}
#endif
}

void FBodyInstance2D::TermBody()
{
#if WITH_BOX2D
	UPrimitiveComponent* OwnerComponent = OwnerComponentPtr.Get();
	//@TODO: Handle this - check(OwnerComponent);
	if (OwnerComponent == NULL)
	{
		return;
	}

	if (BodyInstancePtr != NULL)
	{
		if (b2World* World = FPhysicsIntegration2D::FindAssociatedWorld(OwnerComponent->GetWorld()))
		{
			World->DestroyBody(BodyInstancePtr);
		}
		else
		{
			BodyInstancePtr->SetUserData(NULL);
		}

		BodyInstancePtr = NULL;
	}
#endif
}

bool FBodyInstance2D::IsValidBodyInstance() const
{
#if WITH_BOX2D
	return BodyInstancePtr != NULL;
#else
	return false;
#endif
}

FTransform FBodyInstance2D::GetUnrealWorldTransform() const
{
#if WITH_BOX2D
	if (BodyInstancePtr != NULL)
	{
		const b2Vec2 Pos2D = BodyInstancePtr->GetPosition();
		const float Rotation = BodyInstancePtr->GetAngle();
		//@TODO: Should I use get transform?
		//@TODO: What about scale?

		const FVector Translation3D(FPhysicsIntegration2D::ConvertBoxVectorToUnreal(Pos2D));
		const FRotator Rotation3D(FMath::RadiansToDegrees(Rotation), 0.0f, 0.0f);
		const FVector Scale3D(1.0f, 1.0f, 1.0f);
		
		return FTransform(Rotation3D, Translation3D, Scale3D);
	}
	else
#endif
	{
		return FTransform::Identity;
	}
}

void FBodyInstance2D::SetBodyTransform(const FTransform& NewTransform)
{
#if WITH_BOX2D
	if (BodyInstancePtr != NULL)
	{
		const FVector NewLocation = NewTransform.GetLocation();
		const b2Vec2 NewLocation2D(FPhysicsIntegration2D::ConvertUnrealVectorToBox(NewLocation));

		//@TODO: What about scale?
		const FRotator NewRotation3D(NewTransform.GetRotation());
		const float NewAngle = FMath::DegreesToRadians(NewRotation3D.Pitch);

		BodyInstancePtr->SetTransform(NewLocation2D, NewAngle);
	}
#endif
}

void FBodyInstance2D::SetInstanceSimulatePhysics(bool bSimulate)
{
#if WITH_BOX2D
	if (BodyInstancePtr != NULL)
	{
		bSimulatePhysics = bSimulate;
		BodyInstancePtr->SetType(bSimulatePhysics ? b2_dynamicBody : b2_kinematicBody);
	}
#endif
}