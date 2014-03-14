// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../../../../Source/Runtime/Engine/Classes/Engine/TextRenderActor.h"

ATextRenderActor::ATextRenderActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TextRenderTexture;
		FConstructorStatics()
		: TextRenderTexture(TEXT("/Engine/EditorResources/S_TextRenderActorIcon"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	TextRender = PCIP.CreateDefaultSubobject<UTextRenderComponent>(this, TEXT("NewTextRenderComponent"));
	RootComponent = TextRender;

	SpriteComponent = PCIP.CreateDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.TextRenderTexture.Get();
		SpriteComponent->AttachParent = TextRender;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->ScreenSize = 0.0025f;
		SpriteComponent->bAbsoluteScale = true;
		SpriteComponent->bReceivesDecals = false;
	}
}

// ---------------------------------------------------------------
// Text parsing class which understands line breaks using the '<br>' characters

struct FTextIterator
{
	const TCHAR* SourceString;
	const TCHAR* CurrentPosition;

	FTextIterator(const TCHAR* InSourceString)
		: SourceString(InSourceString)
		, CurrentPosition(InSourceString)
	{
		check(InSourceString);
	}

	bool NextLine()
	{
		check(CurrentPosition);
		return (CurrentPosition[0] != '\0');
	}

	bool NextCharacterInLine(int32& Ch)
	{	
		bool bRet = false;
		check(CurrentPosition);

		if (CurrentPosition[0] == '\0')
		{
			// Leave current position on the null terminator
		}
		else if (CurrentPosition[0] == '<' && CurrentPosition[1] == 'b' && CurrentPosition[2] == 'r' && CurrentPosition[3] == '>')
		{
			CurrentPosition += 4;
		}
		else
		{
			Ch = *CurrentPosition;
			CurrentPosition++;
			bRet = true;
		}

		return bRet;
	}

	bool Peek(int32& Ch)
	{
		check(CurrentPosition);
		if ( CurrentPosition[0] =='\0' || CurrentPosition[1] == '\0' || (CurrentPosition[1] == '<' && CurrentPosition[2] == 'b' && CurrentPosition[3] == 'r' && CurrentPosition[4] == '>'))
		{
			return false;
		}
		else
		{
			Ch = CurrentPosition[1];
			return true;
		}
	}
};

// ---------------------------------------------------------------

// @param It must be a valid initialized text iterator
// @param Font 0 is silently ignored
FVector2D ComputeTextSize(FTextIterator It, class UFont* Font,
	float XScale, float YScale, float HorizSpacingAdjust)
{
	FVector2D Ret(0, 0);

	if(!Font)
	{
		return Ret;
	}

	const float CharIncrement = ( (float)Font->Kerning + HorizSpacingAdjust ) * XScale;

	float LineX = 0;

	const int32 PageIndex = 0;

	int32 Ch;
	while (It.NextCharacterInLine(Ch))
	{
		Ch = (int32)Font->RemapChar(Ch);

		if(!Font->Characters.IsValidIndex(Ch + PageIndex))
		{
			continue;
		}

		FFontCharacter& Char = Font->Characters[Ch + PageIndex];

		if(!Font->Textures.IsValidIndex(Char.TextureIndex))
		{
			continue;
		}

		UTexture2D* Tex = Font->Textures[Char.TextureIndex];

		if(Tex && Tex->Resource)
		{
			FIntPoint ImportedTextureSize = Tex->GetImportedSize();
			FVector2D InvTextureSize(1.0f / (float)ImportedTextureSize.X, 1.0f / (float)ImportedTextureSize.Y);

			const float X		= LineX;
			const float Y		= Char.VerticalOffset * YScale;
			float SizeX			= Char.USize * XScale;
			const float SizeY	= Char.VSize * YScale;
			const float U		= Char.StartU * InvTextureSize.X;
			const float V		= Char.StartV * InvTextureSize.Y;
			const float SizeU	= Char.USize * InvTextureSize.X;
			const float SizeV	= Char.VSize * InvTextureSize.Y;

			float Right = X + SizeX;
			float Bottom = Y + SizeY;

			Ret = FMath::Max(Ret, FVector2D(Right, Bottom));

			// if we have another non-whitespace character to render, add the font's kerning.
			int32 NextCh;
			if( It.Peek(NextCh) && !FChar::IsWhitespace(NextCh) )
			{
				SizeX += CharIncrement;
			}

			LineX += SizeX;
		}
	}

	return Ret;
}

// compute the left top depending on the alignment
static float ComputeHorizontalAlignmentOffset(FVector2D Size, EHorizTextAligment HorizontalAlignment)
{
	float Ret = 0;

	switch (HorizontalAlignment)
	{
	case EHTA_Left:
		{
			// X is already 0
			break;
		}

	case EHTA_Center:
		{
			Ret = -Size.X * 0.5f;
			break;
		}

	case EHTA_Right:
		{
			Ret = -Size.X;
			break;
		}

	default:
		{
			// internal error
			check(0);
		}
	}

	return Ret;
}

float ComputeVerticalAlignmentOffset(float SizeY, EVerticalTextAligment VerticalAlignment, float LegacyVerticalOffset)
{
	switch (VerticalAlignment)
	{
	case EVRTA_QuadTop:
		{
			return LegacyVerticalOffset;
		}
	case EVRTA_TextBottom:
		{
			return -SizeY;
		}
	case EVRTA_TextTop:
		{
			return 0;
		}
	case EVRTA_TextCenter:
		{
			return -SizeY / 2.0f;
		}
	default:
		{
			check(0);
			return 0;
		}
	}
}

// @param Text must not be 0
// @param Font 0 is silently ignored
void DrawString3D(
	FPrimitiveDrawInterface* PDI,
	UMaterialInterface* TextMaterial,
	const TCHAR* Text,
	class UFont* Font,
	const FLinearColor& Color,
	FVector2D LeftTop,
	const FMatrix& Transform,
	float XScale, 
	float YScale, 
	float HorizSpacingAdjust,
	EHorizTextAligment HorizontalAlignment,
	EVerticalTextAligment VerticalAlignment,
	bool bReceivesDecals)
{
	check(Text);

	if(!Font)
	{
		return;
	}

	float FirstLineHeight = -1; // Only kept around for legacy positioning support
	float StartY = 0;
	
	FLinearColor ActualColor = Color;
		
	const float CharIncrement = ( (float)Font->Kerning + HorizSpacingAdjust ) * XScale;

	float LineX = 0;

	const int32 PageIndex = 0;

	FDynamicMeshBuilder MeshBuilder;

	FTextIterator It(Text);

	while (It.NextLine())
	{
		FVector2D LineSize = ComputeTextSize(It, Font, XScale, YScale, HorizSpacingAdjust);
		float StartX = ComputeHorizontalAlignmentOffset(LineSize, HorizontalAlignment);

		if (FirstLineHeight < 0)
		{
			FirstLineHeight = LineSize.Y;
		}

		LineX = 0;
		int32 Ch;

		while (It.NextCharacterInLine(Ch))
		{
			Ch = (int32)Font->RemapChar(Ch);

			if(!Font->Characters.IsValidIndex(Ch + PageIndex))
			{
				continue;
			}

			FFontCharacter& Char = Font->Characters[Ch + PageIndex];

			if(!Font->Textures.IsValidIndex(Char.TextureIndex))
			{
				continue;
			}

			UTexture2D* Tex = Font->Textures[Char.TextureIndex];

			if(Tex && Tex->Resource)
			{
				FIntPoint ImportedTextureSize = Tex->GetImportedSize();
				FVector2D InvTextureSize(1.0f / (float)ImportedTextureSize.X, 1.0f / (float)ImportedTextureSize.Y);

				const float X      = LineX + StartX;
				const float Y      = StartY + Char.VerticalOffset * YScale;
				float SizeX = Char.USize * XScale;
				const float SizeY = Char.VSize * YScale;
				const float U		= Char.StartU * InvTextureSize.X;
				const float V		= Char.StartV * InvTextureSize.Y;
				const float SizeU	= Char.USize * InvTextureSize.X;
				const float SizeV	= Char.VSize * InvTextureSize.Y;			

				float Left = X;
				float Top = Y;
				float Right = X + SizeX;
				float Bottom = Y + SizeY;

				// axis choice and sign to get good alignment when placed on surface
				FVector4 V0 = FVector4(0, -Left, -Top, 0);
				FVector4 V1 = FVector4(0, -Right, -Top, 0);
				FVector4 V2 = FVector4(0, -Left, -Bottom, 0);
				FVector4 V3 = FVector4(0, -Right, -Bottom, 0);

				FVector TangentX(0, -1, 0);
				FVector TangentY(0, 0, -1);
				FVector TangentZ(1, 0, 0);

				int32 V00 = MeshBuilder.AddVertex(V0, FVector2D(U,			V),			TangentX, TangentY, TangentZ, ActualColor);
				int32 V10 = MeshBuilder.AddVertex(V1, FVector2D(U + SizeU,	V),			TangentX, TangentY, TangentZ, ActualColor);
				int32 V01 = MeshBuilder.AddVertex(V2, FVector2D(U,			V + SizeV),	TangentX, TangentY, TangentZ, ActualColor);
				int32 V11 = MeshBuilder.AddVertex(V3, FVector2D(U + SizeU,	V + SizeV),	TangentX, TangentY, TangentZ, ActualColor);

				MeshBuilder.AddTriangle(V00, V11, V10);
				MeshBuilder.AddTriangle(V00, V01, V11);

				// if we have another non-whitespace character to render, add the font's kerning.
				int32 NextChar;
				if( It.Peek(NextChar) && !FChar::IsWhitespace(NextChar) )
				{
					SizeX += CharIncrement;
				}

				LineX += SizeX;
			}
		}

		// Move Y position down to next line. If the current line is empty, move by max char height in font
		StartY += LineSize.Y > 0 ? LineSize.Y : Font->GetMaxCharHeight();
	}

	// Calculate a vertical translation to create the correct vertical alignment
	FMatrix VerticalTransform = FMatrix::Identity;
	float VerticalAlignmentOffset = -ComputeVerticalAlignmentOffset(StartY, VerticalAlignment, FirstLineHeight);
	VerticalTransform = VerticalTransform.ConcatTranslation(FVector(0, 0, VerticalAlignmentOffset));

	// Draw mesh
	MeshBuilder.Draw(PDI, VerticalTransform * Transform, TextMaterial->GetRenderProxy(false), SDPG_World, 0.f, bReceivesDecals);
}

/**
 * For the given text info, calculate the vertical offset that needs to be applied to the component
 * in order to vertically align it to the requested alignment.
 **/
float CalculateVerticalAlignmentOffset(
	const TCHAR* Text,
	class UFont* Font,
	float XScale, 
	float YScale, 
	float HorizSpacingAdjust,
	EVerticalTextAligment VerticalAlignment)
{
	check(Text);

	if(!Font)
	{
		return 0;
	}

	float FirstLineHeight = -1; // Only kept around for legacy positioning support
	float StartY = 0;

	FTextIterator It(Text);

	while (It.NextLine())
	{
		FVector2D LineSize = ComputeTextSize(It, Font, XScale, YScale, HorizSpacingAdjust);

		if (FirstLineHeight < 0)
		{
			FirstLineHeight = LineSize.Y;
		}

		int32 Ch;

		// Iterate to end of line
		while (It.NextCharacterInLine(Ch))
		{
		}

		// Move Y position down to next line. If the current line is empty, move by max char height in font
		StartY += LineSize.Y > 0 ? LineSize.Y : Font->GetMaxCharHeight();
	}

	// Calculate a vertical translation to create the correct vertical alignment
	return -ComputeVerticalAlignmentOffset(StartY, VerticalAlignment, FirstLineHeight);
}
	
// ------------------------------------------------------

/** Represents a URenderTextComponent to the scene manager. */
class FTextRenderSceneProxy : public FPrimitiveSceneProxy
{
public:
	// constructor
	FTextRenderSceneProxy(UTextRenderComponent* Component);

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View);
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);
	virtual bool CanBeOccluded() const OVERRIDE;
	virtual uint32 GetMemoryFootprint() const;
	uint32 GetAllocatedSize() const;

private:
	FMaterialRelevance MaterialRelevance;
	const FColor TextRenderColor;
	UMaterialInterface* TextMaterial;
	UFont* Font;
	FString Text;
	float XScale;
	float YScale;
	float HorizSpacingAdjust;
	EHorizTextAligment HorizontalAlignment;
	EVerticalTextAligment VerticalAlignment;
};

FTextRenderSceneProxy::FTextRenderSceneProxy( UTextRenderComponent* Component) :
	FPrimitiveSceneProxy(Component),
	TextRenderColor(Component->TextRenderColor),
	Font(Component->Font),
	Text(Component->Text),
	XScale(Component->WorldSize * Component->XScale * Component->InvDefaultSize),
	YScale(Component->WorldSize * Component->YScale * Component->InvDefaultSize),
	HorizSpacingAdjust(Component->HorizSpacingAdjust),
	HorizontalAlignment(Component->HorizontalAlignment),
	VerticalAlignment(Component->VerticalAlignment)
{
	UMaterialInterface* EffectiveMaterial = 0;

	if(Component->TextMaterial)
	{
		UMaterial* BaseMaterial = Component->TextMaterial->GetMaterial();

		if(BaseMaterial->MaterialDomain == MD_Surface)
		{
			EffectiveMaterial = Component->TextMaterial;
		}
	}

	if(!EffectiveMaterial)
	{
		EffectiveMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	TextMaterial = EffectiveMaterial;
	MaterialRelevance |= TextMaterial->GetMaterial()->GetRelevance();
}

void FTextRenderSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_TextRenderSceneProxy_DrawDynamicElements );

	const FMatrix& Transform = GetLocalToWorld();

	FVector2D LeftTop(0, 0);

	// more importantly we optimize by not generating the vertex buffer each time and batch.
	DrawString3D(PDI, TextMaterial, *Text, Font, TextRenderColor, LeftTop, Transform, XScale, YScale, HorizSpacingAdjust, HorizontalAlignment, VerticalAlignment, ReceivesDecals());
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), IsSelected());
#endif
}

bool FTextRenderSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

FPrimitiveViewRelevance FTextRenderSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.TextRender;
	Result.bDynamicRelevance = true;
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();

	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}

uint32 FTextRenderSceneProxy::GetMemoryFootprint() const
{
	return( sizeof( *this ) + GetAllocatedSize() );
}

uint32 FTextRenderSceneProxy::GetAllocatedSize() const
{
	return( FPrimitiveSceneProxy::GetAllocatedSize() ); 
}

// ------------------------------------------------------

UTextRenderComponent::UTextRenderComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UFont> Font;
		ConstructorHelpers::FObjectFinderOptional<UMaterial> TextMaterial;
		FConstructorStatics()
			: Font(TEXT("/Engine/EngineFonts/RobotoDistanceField"))
			, TextMaterial(TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Text = "Text";
	Font = ConstructorStatics.Font.Get();
	TextMaterial = ConstructorStatics.TextMaterial.Get();

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TextRenderColor = FColor(255, 255, 255);
	XScale = 1;
	YScale = 1;
	HorizSpacingAdjust = 0;
	HorizontalAlignment = EHTA_Left;
	VerticalAlignment = EVRTA_TextBottom;

	bGenerateOverlapEvents = false;

	if( Font )
	{
		WorldSize = Font->GetMaxCharHeight();
		InvDefaultSize = 1.0f / WorldSize;
	}
	else
	{
		WorldSize = 30.0f;
		InvDefaultSize = 1.0f / 30.0f;
	}
}

FPrimitiveSceneProxy* UTextRenderComponent::CreateSceneProxy()
{
	return new FTextRenderSceneProxy(this);
}

void UTextRenderComponent::GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const
{
	OutMaterials.Add(TextMaterial);
}

int32 UTextRenderComponent::GetNumMaterials() const
{
	return 1;
}

void UTextRenderComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
{
	if (ElementIndex == 0)
	{
		SetTextMaterial(InMaterial);
	}
}

UMaterialInterface* UTextRenderComponent::GetMaterial(int32 ElementIndex) const
{
	return (ElementIndex == 0) ? TextMaterial : NULL;
}

bool UTextRenderComponent::ShouldRecreateProxyOnUpdateTransform() const
{
	return true;
}

FBoxSphereBounds UTextRenderComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if(!Text.IsEmpty())
	{
		FVector2D Size(FLT_MIN, 0);
		FVector2D LeftTop(FLT_MAX, FLT_MAX);
		float FirstLineHeight = -1;

		FTextIterator It(*Text);

		float AdjustedXScale = WorldSize * XScale * InvDefaultSize;
		float AdjustedYScale = WorldSize * YScale * InvDefaultSize;

		while (It.NextLine())
		{
			FVector2D LineSize = ComputeTextSize(It, Font, AdjustedXScale, YScale, HorizSpacingAdjust);
			float LineLeft = ComputeHorizontalAlignmentOffset(LineSize, HorizontalAlignment);

			Size.X = FMath::Max(LineSize.X, Size.X);
			Size.Y += LineSize.Y > 0 ? LineSize.Y : Font->GetMaxCharHeight();
			LeftTop.X = FMath::Min(LeftTop.X, LineLeft);

			if (FirstLineHeight < 0)
			{
				FirstLineHeight = LineSize.Y;
			}

			int32 Ch;
			while (It.NextCharacterInLine(Ch));
		}

		LeftTop.Y = ComputeVerticalAlignmentOffset(Size.Y, VerticalAlignment, FirstLineHeight);
		FBox LocalBox(FVector(0, -LeftTop.X, -LeftTop.Y), FVector(0, -(LeftTop.X + Size.X), -(LeftTop.Y + Size.Y)));

		return FBoxSphereBounds(LocalBox.TransformBy(LocalToWorld));
	}
	else
	{
		return FBoxSphereBounds(ForceInit);
	}
}

void UTextRenderComponent::SetText(const FString& Value)
{
	Text = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetTextMaterial(class UMaterialInterface* Value)
{
	TextMaterial = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetFont(UFont* Value)
{
	Font = Value;
	if( Font )
	{
		InvDefaultSize = 1.0f / Font->GetMaxCharHeight();
	}
	else
	{
		InvDefaultSize = 1.0f;
	}

	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetHorizontalAlignment(EHorizTextAligment Value)
{
	HorizontalAlignment = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetTextRenderColor(FColor Value)
{
	TextRenderColor = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetXScale(float Value)
{
	XScale = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetYScale(float Value)
{
	YScale = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetHorizSpacingAdjust(float Value)
{
	HorizSpacingAdjust = Value;
	MarkRenderStateDirty();	
}

void UTextRenderComponent::SetWorldSize(float Value)
{
	WorldSize = Value;
	MarkRenderStateDirty();	
}

FVector UTextRenderComponent::GetTextLocalSize() const
{
	FBoxSphereBounds Bounds = CalcBounds(FTransform::Identity);
	return Bounds.GetBox().GetSize();
}

FVector UTextRenderComponent::GetTextWorldSize() const
{
	FBoxSphereBounds Bounds = CalcBounds(ComponentToWorld);
	return Bounds.GetBox().GetSize();
}

void UTextRenderComponent::PostLoad()
{
	// Try and fix up assets created before the vertical alignment fix was implemented. Because we didn't flag that
	// fix with its own version, use the version number closest to that CL
	if (GetLinkerUE4Version() < VER_UE4_PACKAGE_REQUIRES_LOCALIZATION_GATHER_FLAGGING)
	{
		float Offset = CalculateVerticalAlignmentOffset(*Text, Font, XScale, YScale, HorizSpacingAdjust, VerticalAlignment);
		FTransform RelativeTransform = GetRelativeTransform();
		FTransform CorrectionLeft = FTransform::Identity;
		FTransform CorrectionRight = FTransform::Identity;
		CorrectionLeft.SetTranslation(FVector(0.0f, 0.0f, -Offset));
		CorrectionRight.SetTranslation(FVector(0.0f, 0.0f, Offset));
		SetRelativeTransform(CorrectionLeft * RelativeTransform * CorrectionRight);
	}

	if (GetLinkerUE4Version() < VER_UE4_ADD_TEXT_COMPONENT_VERTICAL_ALIGNMENT)
	{
		VerticalAlignment = EVRTA_QuadTop;
	}

	if( GetLinkerUE4Version() < VER_UE4_TEXT_RENDER_COMPONENTS_WORLD_SPACE_SIZING )
	{
		if( Font )
		{
			WorldSize = Font->GetMaxCharHeight();
			InvDefaultSize = 1.0f / WorldSize;
		}
		else
		{
			//Just guess I suppose? If there is no font then there's no text to break so it's ok.
			WorldSize = 30.0f;
			InvDefaultSize = 1.0f / 30.0f;
		}
	}

	Super::PostLoad();
}
