// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#define LOCTEXT_NAMESPACE "ALogVisualizerHUD"

//----------------------------------------------------------------------//
// ALogVisualizerHUD
//----------------------------------------------------------------------//
ALogVisualizerHUD::ALogVisualizerHUD(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bHidden = false;

	TextRenderInfo.bEnableShadow = true;
}

bool ALogVisualizerHUD::DisplayMaterials( float X, float& Y, float DY, UMeshComponent* MeshComp )
{
	return false;
}

void ALogVisualizerHUD::PostRender()
{
	static const FColor TextColor(200, 200, 128, 255);

	Super::Super::PostRender();

	if (bShowHUD)
	{
		ALogVisualizerCameraController* DebugCamController = Cast<ALogVisualizerCameraController>( PlayerOwner );
		if( DebugCamController != NULL )
		{
			FCanvasTextItem TextItem( FVector2D::ZeroVector, FText::GetEmpty(), GEngine->GetSmallFont(), TextColor);
			TextItem.FontRenderInfo = TextRenderInfo;
			float X = Canvas->SizeX * 0.025f+1;
			float Y = Canvas->SizeX * 0.025f+1;

			FVector const CamLoc = DebugCamController->PlayerCameraManager->GetCameraLocation();
			FRotator const CamRot = DebugCamController->PlayerCameraManager->GetCameraRotation();

			FCollisionQueryParams TraceParams(NAME_None, true, this);
			FHitResult Hit;
			bool bHit = GetWorld()->LineTraceSingle(Hit, CamLoc, CamRot.Vector() * 100000.f + CamLoc, ECC_Pawn, TraceParams);
			if( bHit )
			{
				TextItem.Text = FText::FromString(FString::Printf(TEXT("Under cursor: '%s'"), TEXT_NAME(Hit.GetActor())));
				Canvas->DrawItem( TextItem, X, Y );
				
				DrawDebugLine( GetWorld(), Hit.Location, Hit.Location+Hit.Normal*30.f, FColor::White );
			}
			else
			{
				TextItem.Text = LOCTEXT("NotActorUnderCursor", "Not actor under cursor" );
			}
			Canvas->DrawItem( TextItem, X, Y );
			Y += TextItem.DrawnSize.Y;

			if (DebugCamController->PickedActor != NULL)
			{
				TextItem.Text = FText::FromString(FString::Printf(TEXT("Selected: '%s'"), *DebugCamController->PickedActor->GetName()));
				Canvas->DrawItem( TextItem, X, Y );				
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE
