// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/DocumentationActor.h"

#if WITH_EDITORONLY_DATA
#include "IDocumentation.h"
#endif



ADocumentationActor::ADocumentationActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));
	RootComponent = SceneComponent;	

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DocuemtMesh(TEXT("/Engine/EditorMeshes/EditorHelp"));
	
	// Create a mesh to represent our actor
	StaticMeshComponent = PCIP.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("MeshComponent"));
	StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComponent->bGenerateOverlapEvents = false;
	StaticMeshComponent->AttachParent = RootComponent;		
	StaticMeshComponent->SetStaticMesh(DocuemtMesh.Object);
#endif //WITH_EDITORONLY_DATA
}

bool ADocumentationActor::OpenDocumentLink() const
{
	bool bOpened = false;

#if WITH_EDITORONLY_DATA
	bOpened = IDocumentation::Get()->Open(DocumentLink);
#endif //WITH_EDITORONLY_DATA

	return bOpened;
}

bool ADocumentationActor::HasValidDocumentLink() const
{
	bool bDocumentValid = false;

#if WITH_EDITORONLY_DATA
	bDocumentValid = DocumentLink.IsEmpty() == false; 
#endif // WITH_EDITORONLY_DATA

	return bDocumentValid;
	
}
