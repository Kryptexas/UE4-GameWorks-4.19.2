// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Landscape.h"
#include "EngineUtils.h"
#include "MaterialCompiler.h"
#include "Materials/MaterialExpressionLandscapeGrassOutput.h"
#include "EdGraph/EdGraphNode.h"

#define LOCTEXT_NAMESPACE "Landscape"

//
// UMaterialExpressionLandscapeGrassOutput
//
UMaterialExpressionLandscapeGrassOutput::UMaterialExpressionLandscapeGrassOutput(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FString STRING_Landscape;
		FName NAME_Grass;
		FConstructorStatics()
			: STRING_Landscape(LOCTEXT("Landscape", "Landscape").ToString())
			, NAME_Grass("Grass")
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	MenuCategories.Add(ConstructorStatics.STRING_Landscape);

	// No outputs
	Outputs.Reset();

	// Default input
	new(GrassTypes)FGrassInput(ConstructorStatics.NAME_Grass);
}

int32 UMaterialExpressionLandscapeGrassOutput::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex)
{
	if (GrassTypes.IsValidIndex(OutputIndex))
	{
		if (GrassTypes[OutputIndex].Input.Expression)
		{
			return Compiler->CustomOutput(this, OutputIndex, GrassTypes[OutputIndex].Input.Compile(Compiler, MultiplexIndex));
		}
		else
		{
			return CompilerError(Compiler, TEXT("Input missing"));
		}
	}

	return INDEX_NONE;
}

void UMaterialExpressionLandscapeGrassOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Grass"));
}

const TArray<FExpressionInput*> UMaterialExpressionLandscapeGrassOutput::GetInputs()
{
	TArray<FExpressionInput*> OutInputs;
	for (auto& GrassType : GrassTypes)
	{
		OutInputs.Add(&GrassType.Input);
	}
	return OutInputs;
}

FExpressionInput* UMaterialExpressionLandscapeGrassOutput::GetInput(int32 InputIndex)
{
	return &GrassTypes[InputIndex].Input;
}

FString UMaterialExpressionLandscapeGrassOutput::GetInputName(int32 InputIndex) const
{
	return GrassTypes[InputIndex].Name.ToString();
}

#if WITH_EDITOR
void UMaterialExpressionLandscapeGrassOutput::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMaterialExpressionLandscapeGrassOutput, GrassTypes))
		{
			if (GraphNode)
			{
				GraphNode->ReconstructNode();
			}
		}
	}
}
#endif

//
// ULandscapeGrassType
//
ULandscapeGrassType::ULandscapeGrassType(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GrassDensity = 400;
	StartCullDistance = 10000.0f;
	EndCullDistance = 10000.0f;
	PlacementJitter = 1.0f;
	RandomRotation = true;
	AlignToSurface = true;
}

#if WITH_EDITOR
void ULandscapeGrassType::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (GIsEditor)
	{
		// Only care current world object
		for (TActorIterator<ALandscapeProxy> It(GWorld); It; ++It)
		{
			ALandscapeProxy* Proxy = *It;
			const UMaterialInterface* MaterialInterface = Proxy->LandscapeMaterial;
			if (MaterialInterface)
			{
				TArray<const UMaterialExpressionLandscapeGrassOutput*> GrassExpressions;
				MaterialInterface->GetMaterial()->GetAllExpressionsOfType<UMaterialExpressionLandscapeGrassOutput>(GrassExpressions);

				// Should only be one grass type node
				if (GrassExpressions.Num() > 0)
				{
					for (auto& Output : GrassExpressions[0]->GrassTypes)
					{
						if (Output.GrassType == this)
						{
							Proxy->FlushFoliageComponents();
							break;
						}
					}
				}
			}
		}
	}
}
#endif


#undef LOCTEXT_NAMESPACE
