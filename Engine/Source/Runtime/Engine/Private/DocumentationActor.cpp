// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/DocumentationActor.h"
#if WITH_EDITOR
#include "Components/MaterialBillboardComponent.h"
#include "IDocumentation.h"
#endif



ADocumentationActor::ADocumentationActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));
	RootComponent = SceneComponent;	

#if WITH_EDITORONLY_DATA
 	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(TEXT("/Engine/EditorMaterials/HelpActorMaterial"));
 	// Create a Material billboard to represent our actor
	Billboard = PCIP.CreateDefaultSubobject<UMaterialBillboardComponent>(this, TEXT("BillboardComponent"));
	Billboard->AddElement(MaterialAsset.Object, nullptr, false, 32.0f, 32.0f, nullptr);
	Billboard->AttachParent = RootComponent;
#endif //WITH_EDITORONLY_DATA
}

bool ADocumentationActor::OpenDocumentLink() const
{
	bool bOpened = false;

#if WITH_EDITOR
	bOpened = IDocumentation::Get()->Open(DocumentLink);
#endif //WITH_EDITOR

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

