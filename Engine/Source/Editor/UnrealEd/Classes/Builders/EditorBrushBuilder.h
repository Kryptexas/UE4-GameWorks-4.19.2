// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EditorBrushBuilder.generated.h"

UCLASS(abstract, MinimalAPI)
class UEditorBrushBuilder : public UBrushBuilder
{
	GENERATED_UCLASS_BODY()

public:

	/** UBrushBuilder interface */
	virtual void BeginBrush( bool InMergeCoplanars, FName InLayer ) OVERRIDE;
	virtual bool EndBrush( UWorld* InWorld, ABrush* InBrush ) OVERRIDE;
	virtual int32 GetVertexCount() OVERRIDE;
	virtual FVector GetVertex( int32 i ) OVERRIDE;
	virtual int32 GetPolyCount() OVERRIDE;
	virtual bool BadParameters( const FText& msg ) OVERRIDE;
	virtual int32 Vertexv( FVector v ) OVERRIDE;
	virtual int32 Vertex3f( float X, float Y, float Z ) OVERRIDE;
	virtual void Poly3i( int32 Direction, int32 i, int32 j, int32 k, FName ItemName = NAME_None, bool bIsTwoSidedNonSolid = false ) OVERRIDE;
	virtual void Poly4i( int32 Direction, int32 i, int32 j, int32 k, int32 l, FName ItemName = NAME_None, bool bIsTwoSidedNonSolid = false ) OVERRIDE;
	virtual void PolyBegin( int32 Direction, FName ItemName = NAME_None ) OVERRIDE;
	virtual void Polyi( int32 i ) OVERRIDE;
	virtual void PolyEnd() OVERRIDE;
	UNREALED_API virtual bool Build( UWorld* InWorld, ABrush* InBrush = NULL ) OVERRIDE;

	/** UObject interface */
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
};



