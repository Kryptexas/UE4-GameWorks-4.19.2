// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HUD.cpp: Heads up Display related functionality
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"

DEFINE_LOG_CATEGORY_STATIC(LogHUD, Log, All);

#define LOCTEXT_NAMESPACE "HUD"

AHUD::AHUD(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.TickGroup = TG_DuringPhysics;
	PrimaryActorTick.bCanEverTick = true;
	bHidden = true;
	bReplicates = false;
	
	WhiteColor = FColor(255, 255, 255, 255);	
	GreenColor = FColor(0, 255, 0, 255);
	RedColor = FColor(255, 0, 0, 255);

	bLostFocusPaused = false;

	bCanBeDamaged = false;
}

void AHUD::SetCanvas(class UCanvas* InCanvas, class UCanvas* InDebugCanvas)
{
	Canvas = InCanvas;
	DebugCanvas = InDebugCanvas;
}


void AHUD::Draw3DLine(FVector Start, FVector End, FColor LineColor)
{
	GetWorld()->LineBatcher->DrawLine(Start,End,LineColor,SDPG_World);
}

void AHUD::Draw2DLine(int32 X1,int32 Y1,int32 X2,int32 Y2,FColor LineColor)
{
	check(Canvas);

	FCanvasLineItem LineItem( FVector2D(X1, Y1), FVector2D(X2, Y2) );
	LineItem.SetColor( LineColor );
	LineItem.Draw( Canvas->Canvas );
}

void AHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	PlayerOwner = Cast<APlayerController>(GetOwner());

	// e.g. getting material pointers to control effects for gameplay
	NotifyBindPostProcessEffects();
}



void AHUD::NotifyBindPostProcessEffects()
{
	// overload with custom code e.g. getting material pointers to control effects for gameplay.
}

void AHUD::PostRender()
{
	// Set up delta time
	RenderDelta = GetWorld()->TimeSeconds - LastHUDRenderTime;

	if ( PlayerOwner != NULL )
	{
		// draw any debug text in real-time
		DrawDebugTextList();
	}

	if ( bShowDebugInfo )
	{
		UFont* Font = GEngine->GetTinyFont();
		float XL, YL;
		Canvas->StrLen(Font, TEXT("X"), XL, YL);

		float YPos = 50;
		ShowDebugInfo(YL, YPos);
	}
	else if ( bShowHUD )
	{
		if (!bSuppressNativeHUD)
		{
			DrawHUD();
		}

		// Kismet draw
		ReceiveDrawHUD(Canvas->SizeX, Canvas->SizeY);
		
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningPlayerController()->Player);

		if (LocalPlayer)
		{
			FVector2D MousePosition = LocalPlayer->ViewportClient->GetMousePosition();

			// Create a view family for the game viewport
			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues(
				LocalPlayer->ViewportClient->Viewport,
				GetWorld()->Scene,
				LocalPlayer->ViewportClient->EngineShowFlags )
				.SetRealtimeUpdate(true) );

			// Calculate a view where the player is to update the streaming from the players start location
			FVector ViewLocation;
			FRotator ViewRotation;
			FSceneView* SceneView = LocalPlayer->CalcSceneView( &ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, LocalPlayer->ViewportClient->Viewport );

			if (SceneView)
			{
				// This accounts for the borders when the aspect ratio is locked
				MousePosition.X -= (SceneView->ViewRect.Min.X - SceneView->UnconstrainedViewRect.Min.X);
				MousePosition.Y -= (SceneView->ViewRect.Min.Y - SceneView->UnconstrainedViewRect.Min.Y);
				// And this will deal with the viewport offset if its a split screen
				MousePosition.X -= SceneView->UnconstrainedViewRect.Min.X;
				MousePosition.Y -= SceneView->UnconstrainedViewRect.Min.Y;
			}
			UpdateHitBoxCandidates( MousePosition );
		}
	}
	
	if( bShowHitBoxDebugInfo )
	{
		RenderHitBoxes( Canvas->Canvas );
	}

	LastHUDRenderTime = GetWorld()->TimeSeconds;
}

void AHUD::DrawActorOverlays(FVector Viewpoint, FRotator ViewRotation)
{
	// determine rendered camera position
	FVector ViewDir = ViewRotation.Vector();
	int32 i = 0;
	while (i < PostRenderedActors.Num())
	{
		if ( PostRenderedActors[i] != NULL )
		{
			PostRenderedActors[i]->PostRenderFor(PlayerOwner,Canvas,Viewpoint,ViewDir);
			i++;
		}
		else
		{
			PostRenderedActors.RemoveAt(i,1);
		}
	}
}

void AHUD::RemovePostRenderedActor(AActor* A)
{
	for ( int32 i=0; i<PostRenderedActors.Num(); i++ )
	{
		if ( PostRenderedActors[i] == A )
		{
			PostRenderedActors[i] = NULL;
			return;
		}
	}
}

void AHUD::AddPostRenderedActor(AActor* A)
{
	// make sure that A is not already in list
	for ( int32 i=0; i<PostRenderedActors.Num(); i++ )
	{
		if ( PostRenderedActors[i] == A )
		{
			return;
		}
	}

	// add A at first empty slot
	for ( int32 i=0; i<PostRenderedActors.Num(); i++ )
	{
		if ( PostRenderedActors[i] == NULL )
		{
			PostRenderedActors[i] = A;
			return;
		}
	}

	// no empty slot found, so grow array
	PostRenderedActors.Add(A);
}

void AHUD::ShowHUD()
{
	bShowHUD = !bShowHUD;
}

void AHUD::ShowDebug(FName DebugType)
{
	if (DebugType == NAME_None)
	{
		bShowDebugInfo = !bShowDebugInfo;
	}
	else if ( DebugType == FName("HitBox") )
	{
		bShowHitBoxDebugInfo = !bShowHitBoxDebugInfo;
	}
	else
	{
		bool bRemoved = false;
		if (bShowDebugInfo)
		{
			// remove debugtype if already in array
			if (0 != DebugDisplay.Remove(DebugType))
			{
				bRemoved = true;
			}
		}
		if (!bRemoved)
		{
			DebugDisplay.Add(DebugType);
		}

		bShowDebugInfo = true;

		SaveConfig();
	}
}

bool AHUD::ShouldDisplayDebug(FName DebugType)
{
	return DebugDisplay.Contains(DebugType);
}

void AHUD::ShowDebugInfo(float& YL, float& YPos)
{
	PlayerOwner->PlayerCameraManager->ViewTarget.Target->DisplayDebug(DebugCanvas, DebugDisplay, YL, YPos);

	if (ShouldDisplayDebug(NAME_Game))
	{
		GetWorld()->GetAuthGameMode()->DisplayDebug(DebugCanvas, DebugDisplay, YL, YPos);
	}
}

void AHUD::DrawHUD()
{
	HitBoxMap.Empty();
	HitBoxHits.Empty();
	if ( bShowOverlays && (PlayerOwner != NULL) )
	{
		FVector ViewPoint;
		FRotator ViewRotation;
		PlayerOwner->GetPlayerViewPoint(ViewPoint, ViewRotation);
		DrawActorOverlays(ViewPoint, ViewRotation);
	}
}


void AHUD::DrawText(const FString& Text, FVector2D Position, UFont* TextFont, FVector2D FontScale, FColor TextColor)
{
	float XL, YL;
	Canvas->TextSize(TextFont, Text, XL, YL);
	const float X = Canvas->ClipX/2.0f - XL/2.0f + Position.X;
	const float Y = Canvas->ClipY/3.0f - YL/2.0f + Position.Y;
	FCanvasTextItem TextItem( FVector2D( X, Y ), FText::FromString( Text ), GEngine->GetTinyFont(), FLinearColor( TextColor ) );
	TextItem.Scale = FontScale;
	Canvas->DrawText(TextFont, Text, X, Y, FontScale.X, FontScale.Y);
}

UFont* AHUD::GetFontFromSizeIndex(int32 FontSizeIndex) const
{
	switch (FontSizeIndex)
	{
	case 0: return GEngine->GetTinyFont();
	case 1: return GEngine->GetSmallFont();
	case 2: return GEngine->GetMediumFont();
	case 3: return GEngine->GetLargeFont();
	}

	return GEngine->GetLargeFont();
}



void AHUD::OnLostFocusPause(bool bEnable)
{
	if ( bLostFocusPaused == bEnable )
		return;

	if ( GetNetMode() != NM_Client )
	{
		bLostFocusPaused = bEnable;
		PlayerOwner->SetPause(bEnable);
	}
}

void AHUD::DrawDebugTextList()
{
	if (DebugTextList.Num() > 0)
	{
		FRotator CameraRot;
		FVector CameraLoc;
		PlayerOwner->GetPlayerViewPoint(CameraLoc, CameraRot);

		FCanvasTextItem TextItem( FVector2D::ZeroVector, FText::GetEmpty(), GEngine->GetSmallFont(), FLinearColor::White );
		for (int32 Idx = 0; Idx < DebugTextList.Num(); Idx++)
		{
			if (DebugTextList[Idx].SrcActor == NULL)
			{
				DebugTextList.RemoveAt(Idx--,1);
				continue;
			}

			if( DebugTextList[Idx].Font != NULL )
			{
				TextItem.Font = DebugTextList[Idx].Font;
			}
			else
			{
				TextItem.Font = GEngine->GetSmallFont();
			}

			FVector WorldTextLoc;
			if (DebugTextList[Idx].bAbsoluteLocation)
			{
				WorldTextLoc = FMath::Lerp( DebugTextList[Idx].SrcActorOffset, DebugTextList[Idx].SrcActorDesiredOffset, 1.f - (DebugTextList[Idx].TimeRemaining/DebugTextList[Idx].Duration) );
			}
			else
			{
				FVector Offset = FMath::Lerp( DebugTextList[Idx].SrcActorOffset, DebugTextList[Idx].SrcActorDesiredOffset, 1.f - (DebugTextList[Idx].TimeRemaining/DebugTextList[Idx].Duration) );

				if( DebugTextList[Idx].bKeepAttachedToActor )
				{
					WorldTextLoc = DebugTextList[Idx].SrcActor->GetActorLocation() + Offset;
				}
				else
				{
					WorldTextLoc = DebugTextList[Idx].OrigActorLocation + Offset;
				}
			}

			// don't draw text behind the camera
			if ( ((WorldTextLoc - CameraLoc) | CameraRot.Vector()) > 0.f )
			{
				FVector ScreenLoc = Canvas->Project(WorldTextLoc);
				TextItem.SetColor( DebugTextList[Idx].TextColor );
				TextItem.Text = FText::FromString( DebugTextList[Idx].DebugText );
				TextItem.Scale = FVector2D( DebugTextList[Idx].FontScale, DebugTextList[Idx].FontScale);
				DebugCanvas->DrawItem( TextItem, FVector2D( FMath::Ceil(ScreenLoc.X), FMath::Ceil(ScreenLoc.Y) ) );
			}

			// do this at the end so even small durations get at least one frame
			if (DebugTextList[Idx].TimeRemaining != -1.f)
			{
				DebugTextList[Idx].TimeRemaining -= RenderDelta;
				if (DebugTextList[Idx].TimeRemaining <= 0.f)
				{
					DebugTextList.RemoveAt(Idx--,1);
					continue;
				}
			}
		}
	}
}

/**
 * Add debug text for a specific actor to be displayed via DrawDebugTextList().  If the debug text is invalid then it will
 * attempt to remove any previous entries via RemoveDebugText().
 */
void AHUD::AddDebugText_Implementation(const FString& DebugText,
										 AActor* SrcActor,
										 float Duration,
										 FVector Offset,
										 FVector DesiredOffset,
										 FColor TextColor,
										 bool bSkipOverwriteCheck,
										 bool bAbsoluteLocation,
										 bool bKeepAttachedToActor,
										 UFont* InFont,
										 float FontScale
										 )
{
	// set a default color
	if (TextColor == FColor::Black)
	{
		TextColor = FColor::White;
	}
	
	// and a default source actor of our pawn
	if (SrcActor != NULL)
	{
		if (DebugText.Len() == 0)
		{
			RemoveDebugText(SrcActor);
		}
		else
		{
			//`log("Adding debug text:"@DebugText@"for actor:"@SrcActor);
			// search for an existing entry
			int32 Idx = 0;
			if (!bSkipOverwriteCheck)
			{
				Idx = INDEX_NONE;
				for (int32 i = 0; i < DebugTextList.Num() && Idx == INDEX_NONE; ++i)
				{
					if (DebugTextList[i].SrcActor == SrcActor)
					{
						Idx = i;
					}
				}
				if (Idx == INDEX_NONE)
				{
					// manually grow the array one struct element
					Idx = DebugTextList.Add(FDebugTextInfo());
				}
			}
			else
			{
				Idx = DebugTextList.Add(FDebugTextInfo());
			}
			// assign the new text and actor
			DebugTextList[Idx].SrcActor = SrcActor;
			DebugTextList[Idx].SrcActorOffset = Offset;
			DebugTextList[Idx].SrcActorDesiredOffset = DesiredOffset;
			DebugTextList[Idx].DebugText = DebugText;
			DebugTextList[Idx].TimeRemaining = Duration;
			DebugTextList[Idx].Duration = Duration;
			DebugTextList[Idx].TextColor = TextColor;
			DebugTextList[Idx].bAbsoluteLocation = bAbsoluteLocation;
			DebugTextList[Idx].bKeepAttachedToActor = bKeepAttachedToActor;
			DebugTextList[Idx].OrigActorLocation = SrcActor->GetActorLocation();
			DebugTextList[Idx].Font = InFont;
			DebugTextList[Idx].FontScale = FontScale;
		}
	}
}

/** Remove debug text for the specific actor. */
void AHUD::RemoveDebugText_Implementation(AActor* SrcActor, bool bLeaveDurationText)
{
	int32 Idx = INDEX_NONE;
	for (int32 i = 0; i < DebugTextList.Num() && Idx == INDEX_NONE; ++i)
	{
		if (DebugTextList[i].SrcActor == SrcActor && (!bLeaveDurationText || DebugTextList[i].TimeRemaining == -1.f))
		{
			Idx = i;
		}
	}
	if (Idx != INDEX_NONE)
	{
		DebugTextList.RemoveAt(Idx,1);
	}
}

/** Remove all debug text */
void AHUD::RemoveAllDebugStrings_Implementation()
{
	DebugTextList.Empty();
}

void AHUD::GetTextSize(const FString& Text, float& OutWidth, float& OutHeight, class UFont* Font, float Scale) const
{
	if (IsCanvasValid_WarnIfNot())
	{
		Canvas->TextSize(Font, Text, OutWidth, OutHeight, Scale, Scale);
	}
}

void AHUD::DrawText(FString const& Text, FLinearColor Color, float ScreenX, float ScreenY, UFont* Font, float Scale, bool bScalePosition)
{
	if (IsCanvasValid_WarnIfNot())
	{
		if (bScalePosition)
		{
			ScreenX *= Scale;
			ScreenY *= Scale;
		}
		FCanvasTextItem TextItem( FVector2D( ScreenX, ScreenY ), FText::FromString(Text), Font ? Font : GEngine->GetMediumFont(), Color );
		TextItem.Scale = FVector2D( Scale, Scale );
		Canvas->DrawItem( TextItem );
	}
}

void AHUD::DrawMaterial(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight, float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
	if (IsCanvasValid_WarnIfNot() && Material)
	{
		FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Material->GetRenderProxy(0), FVector2D( ScreenW, ScreenH ) * Scale, FVector2D( MaterialU, MaterialV ), FVector2D( MaterialU+MaterialUWidth, MaterialV +MaterialVHeight) );
		TileItem.Rotation = FRotator(0, Rotation, 0);
		TileItem.PivotPoint = RotPivot;
		if (bScalePosition)
		{
			TileItem.Position *= Scale;
		}
		Canvas->DrawItem( TileItem );
	}
}

void AHUD::DrawMaterialSimple(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float Scale, bool bScalePosition)
{
	if (IsCanvasValid_WarnIfNot() && Material)
	{
		FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Material->GetRenderProxy(0), FVector2D( ScreenW, ScreenH ) * Scale );
		if (bScalePosition)
		{
			TileItem.Position *= Scale;
		}
		Canvas->DrawItem( TileItem );
	}
}

void AHUD::DrawTexture(UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float TextureU, float TextureV, float TextureUWidth, float TextureVHeight, FLinearColor Color, EBlendMode BlendMode, float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
	if (IsCanvasValid_WarnIfNot() && Texture)
	{
		FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Texture->Resource, FVector2D( ScreenW, ScreenH ) * Scale, FVector2D( TextureU, TextureV ), FVector2D( TextureU + TextureUWidth, TextureV + TextureVHeight ), Color );
		TileItem.Rotation = FRotator(0, Rotation, 0);
		TileItem.PivotPoint = RotPivot;
		if (bScalePosition)
		{
			TileItem.Position *= Scale;
		}
		TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend( BlendMode );
		Canvas->DrawItem( TileItem );
	}
}

void AHUD::DrawTextureSimple(UTexture* Texture, float ScreenX, float ScreenY, float Scale, bool bScalePosition)
{
	if (IsCanvasValid_WarnIfNot() && Texture)
	{
		FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Texture->Resource, FLinearColor::White );
		if (bScalePosition)
		{
			TileItem.Position *= Scale;
		}
		// Apply the scale to the size (which will have been setup from the texture in the constructor).
		TileItem.Size *= Scale;
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem( TileItem );
	}
}

FVector AHUD::Project(FVector Location)
{
	if (IsCanvasValid_WarnIfNot())
	{
		return Canvas->Project(Location);
	}
	return FVector(0,0,0);
}

void AHUD::DrawRect(FLinearColor Color, float ScreenX, float ScreenY, float Width, float Height)
{
	if (IsCanvasValid_WarnIfNot())	
	{
		FCanvasTileItem TileItem( FVector2D(ScreenX, ScreenY), GWhiteTexture, FVector2D( Width, Height ), Color );
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem( TileItem );		
	}
}

void AHUD::DrawLine(float StartScreenX, float StartScreenY, float EndScreenX, float EndScreenY, FLinearColor LineColor)
{
	if (IsCanvasValid_WarnIfNot())
	{
		FCanvasLineItem LineItem( FVector2D(StartScreenX, StartScreenY), FVector2D(EndScreenX, EndScreenY) );
		LineItem.SetColor( LineColor );
		Canvas->DrawItem( LineItem );
	}
}


APlayerController* AHUD::GetOwningPlayerController() const
{
	return PlayerOwner;
}

APawn* AHUD::GetOwningPawn() const
{
	return PlayerOwner ? PlayerOwner->GetPawn() : NULL;
}

void AHUD::RenderHitBoxes( FCanvas* InCanvas )
{
	for (int32 iBox = 0; iBox < HitBoxMap.Num() ; iBox++)
	{
		FLinearColor BoxColor = FLinearColor::White;
		if( HitBoxHits.Contains(&HitBoxMap[iBox]) )
		{
			BoxColor = FLinearColor::Red;
		}
		HitBoxMap[iBox].Draw( InCanvas, BoxColor );
	}
}

void AHUD::UpdateHitBoxCandidates( const FVector2D& InHitLocation )
{
	HitBoxHits.Empty();
	for (int32 iBox = 0; iBox < HitBoxMap.Num() ; iBox++)
	{
		if( HitBoxMap[iBox].Contains( InHitLocation ) )
		{
			HitBoxHits.Add(&HitBoxMap[iBox]);
		}
	}
	struct FHitBoxSort
	{
		FORCEINLINE bool operator()( const FHUDHitBox& A, const FHUDHitBox& B ) const { return B.GetPriority() < A.GetPriority(); }
	};

	// Now sort the hits
	HitBoxHits.Sort( FHitBoxSort() );

	TSet<FName> NotOverHitBoxes = HitBoxesOver;
	TArray<FName> NewlyOverHitBoxes;

	// Now figure out which boxes we are over and deal with begin/end cursor over messages 
	for (int32 HitIndex = 0; HitIndex < HitBoxHits.Num(); ++HitIndex )
	{
		FHUDHitBox* HitBox = HitBoxHits[HitIndex];
		const FName HitBoxName = HitBox->GetName();
		if (HitBoxesOver.Contains(HitBoxName))
		{
			NotOverHitBoxes.Remove(HitBoxName);
		}
		else
		{
			NewlyOverHitBoxes.AddUnique(HitBoxName);
		}
		if (HitBox->ConsumesInput())
		{
			break; //Early out if this box blocks further consideration
		}
	}

	// Dispatch the end cursor over messages
	for (auto It = NotOverHitBoxes.CreateConstIterator(); It; ++It)
	{
		const FName HitBoxName = *It;
		ReceiveHitBoxEndCursorOver(HitBoxName);
		HitBoxesOver.Remove(HitBoxName);
	}

	// Dispatch the newly over hitbox messages
	for (int32 OverIndex = 0; OverIndex < NewlyOverHitBoxes.Num(); ++OverIndex)
	{
		const FName HitBoxName = NewlyOverHitBoxes[OverIndex];
		ReceiveHitBoxBeginCursorOver(HitBoxName);
		HitBoxesOver.Add(HitBoxName);
	}
}

const FHUDHitBox* AHUD::GetHitBoxAtCoordinates( const FVector2D& InHitLocation ) const
{
	for (int32 iBox = 0; iBox < HitBoxMap.Num() ; iBox++)
	{
		if( HitBoxMap[iBox].Contains( InHitLocation ) )
		{
			return &HitBoxMap[iBox];
		}
	}
	return NULL;
}

const FHUDHitBox* AHUD::GetHitBoxWithName( const FName& InName ) const
{
	for (int32 iBox = 0; iBox < HitBoxMap.Num() ; iBox++)
	{
		if( HitBoxMap[iBox].GetName() == InName )
		{
			return &HitBoxMap[iBox];
		}
	}
	return NULL;
}

void AHUD::ClearUIBlurOverrideRects()
{
	UIBlurOverrideRectangles.Empty();
}

void AHUD::AddUIBlurOverrideRect( const FIntRect& UIBlurOverrideRect )
{
	UIBlurOverrideRectangles.Add(UIBlurOverrideRect);
}

bool AHUD::AnyCurrentHitBoxHits() const
{
	return HitBoxHits.Num() != 0;
}

const TArray<FIntRect>& AHUD::GetUIBlurRectangles() const
{
	return UIBlurOverrideRectangles;
}

bool AHUD::UpdateAndDispatchHitBoxClickEvents(EInputEvent InEventType)
{
	for (int32 iBox = 0; iBox < HitBoxHits.Num(); iBox++)
	{
		if(InEventType == IE_Pressed)
		{
			ReceiveHitBoxClick( HitBoxHits[ iBox ]->GetName() );
		}
		else if(InEventType == IE_Released)
		{
			ReceiveHitBoxRelease( HitBoxHits[ iBox ]->GetName() );
		}

		if( HitBoxHits[ iBox ]->ConsumesInput() == true )
		{
			break;	//Early out if this box consumed the click
		}
	}
	return AnyCurrentHitBoxHits();
}

void AHUD::AddHitBox(FVector2D Position, FVector2D Size, FName Name, bool bConsumesInput, int32 Priority)
{	
	if( GetHitBoxWithName(Name) == NULL )
	{
		HitBoxMap.Add( FHUDHitBox( Position, Size, Name, bConsumesInput, Priority ) );
	}
	else
	{
		UE_LOG(LogHUD, Warning, TEXT("Failed to add hitbox named %s as a hitbox with this name already exists"), *Name.ToString());
	}
}

bool AHUD::IsCanvasValid_WarnIfNot() const
{
	const bool bIsValid = Canvas != NULL;
	if (!bIsValid)
	{
		FMessageLog("PIE").Warning()
			->AddToken(FUObjectToken::Create(const_cast<AHUD*>(this)))
			->AddToken(FTextToken::Create(LOCTEXT( "PIE_Warning_Message_CanvasCallOutsideOfDrawCanvas", "Canvas Draw functions may only be called during the handling of the DrawHUD event" )));
	}

	return bIsValid;
}

/////////////////

FHUDHitBox::FHUDHitBox( FVector2D InCoords, FVector2D InSize, const FName& InName, bool bInConsumesInput, int32 InPriority )
{
	Coords = InCoords;
	Size = InSize;
	Name = InName;
	bConsumesInput = bInConsumesInput;
	Priority = InPriority;
}

bool FHUDHitBox::Contains( FVector2D InCoords ) const
{
	bool bResult = false;
	if( ( InCoords.X >= Coords.X ) && (InCoords.X <= ( Coords.X + Size.X ) ) )
	{
		if( ( InCoords.Y >= Coords.Y ) && (InCoords.Y <= ( Coords.Y + Size.Y ) ) )
		{
			bResult = true;
		}
	}
	return bResult;
}

void FHUDHitBox::Draw( FCanvas* InCanvas, const FLinearColor& InColor )
{
	FCanvasBoxItem	BoxItem( Coords, Size );
	BoxItem.SetColor( InColor );
	InCanvas->DrawItem( BoxItem );
	FCanvasTextItem	TextItem( Coords, FText::FromName( Name ), GEngine->GetSmallFont(), InColor );
	InCanvas->DrawItem( TextItem );
}

void FSimpleReticle::Draw( class FCanvas* InCanvas, FLinearColor InColor )
{
	FVector2D CanvasCenter( InCanvas->GetViewRect().Width() / 2.0f, InCanvas->GetViewRect().Height() / 2.0f );
	FCanvasLineItem LineItem( CanvasCenter, FVector2D(0.0f, 0.0f) );
	LineItem.SetColor( InColor );
	LineItem.Draw( InCanvas, CanvasCenter - HorizontalOffsetMin, CanvasCenter - HorizontalOffsetMax );
	LineItem.Draw( InCanvas, CanvasCenter + HorizontalOffsetMin, CanvasCenter + HorizontalOffsetMax );
	LineItem.Draw( InCanvas, CanvasCenter - VerticalOffsetMin, CanvasCenter - VerticalOffsetMax );
	LineItem.Draw( InCanvas, CanvasCenter + VerticalOffsetMin, CanvasCenter + VerticalOffsetMax );
}

#undef LOCTEXT_NAMESPACE
