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
void FBodyInstance2D::InitBody(class UBodySetup2D* Setup, const FTransform& Transform, class UPrimitiveComponent* PrimComp)
{
#if WITH_BOX2D
	if (b2World* World = FPhysicsIntegration2D::FindAssociatedWorld(PrimComp->GetWorld()))
	{
		const FVector Scale3D = Transform.GetScale3D();
		const b2Vec2 Scale2D = FPhysicsIntegration2D::ConvertUnrealVectorToBox(Scale3D);

		if (Setup != nullptr)
		{
			OwnerComponentPtr = PrimComp;

			b2BodyDef BodyDefinition;
			BodyDefinition.type = bSimulatePhysics ? b2_dynamicBody : b2_kinematicBody;

			BodyInstancePtr = World->CreateBody(&BodyDefinition);
			BodyInstancePtr->SetUserData(this);

			// Circles
			for (const FCircleElement2D& Circle : Setup->AggGeom2D.CircleElements)
			{
				b2CircleShape CircleShape;
				CircleShape.m_radius = Circle.Radius * Scale3D.Size() / UnrealUnitsPerMeter;
				CircleShape.m_p.x = Circle.Center.X;
				CircleShape.m_p.y = Circle.Center.Y;

				b2FixtureDef FixtureDef;
				FixtureDef.shape = &CircleShape;
				FixtureDef.density = 1.0f; //@TODO:
				FixtureDef.friction = 0.3f;//@TODO:

				BodyInstancePtr->CreateFixture(&FixtureDef);
			}

			// Boxes
			for (const FBoxElement2D& Box : Setup->AggGeom2D.BoxElements)
			{
				const b2Vec2 HalfBoxSize(Box.Width * 0.5f * Scale2D.x, Box.Height * 0.5f * Scale2D.y);
				const b2Vec2 BoxCenter(Box.Center.X * Scale2D.x, Box.Center.Y * Scale2D.y);

				b2PolygonShape DynamicBox;
				DynamicBox.SetAsBox(HalfBoxSize.x, HalfBoxSize.y, BoxCenter, FMath::DegreesToRadians(Box.Angle));

				b2FixtureDef FixtureDef;
				FixtureDef.shape = &DynamicBox;
				FixtureDef.density = 1.0f; //@TODO:
				FixtureDef.friction = 0.3f;//@TODO:

				BodyInstancePtr->CreateFixture(&FixtureDef);
			}

			// Convex hulls
			for (const FConvexElement2D& Convex : Setup->AggGeom2D.ConvexElements)
			{
				const int32 NumVerts = Convex.VertexData.Num();

				if (NumVerts <= b2_maxPolygonVertices)
				{
					TArray<b2Vec2, TInlineAllocator<b2_maxPolygonVertices>> Verts;

					for (int32 VertexIndex = 0; VertexIndex < Convex.VertexData.Num(); ++VertexIndex)
					{
						const FVector2D SourceVert = Convex.VertexData[VertexIndex];
						new (Verts) b2Vec2(SourceVert.X * Scale2D.x, SourceVert.Y * Scale2D.y);
					}

					b2PolygonShape ConvexPoly;
					ConvexPoly.Set(Verts.GetTypedData(), Verts.Num());

					b2FixtureDef FixtureDef;
					FixtureDef.shape = &ConvexPoly;
					FixtureDef.density = 1.0f; //@TODO:
					FixtureDef.friction = 0.3f;//@TODO:

					BodyInstancePtr->CreateFixture(&FixtureDef);
				}
				else
				{
					UE_LOG(LogPaper2D, Warning, TEXT("Too many vertices in a 2D convex body")); //@TODO: Create a better error message that indicates the asset
				}
			}

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