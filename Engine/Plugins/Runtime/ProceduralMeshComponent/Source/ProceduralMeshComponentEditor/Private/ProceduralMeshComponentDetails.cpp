// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ProceduralMeshComponentEditorPrivatePCH.h"
#include "ProceduralMeshComponentDetails.h"

#include "DlgPickAssetPath.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "ProceduralMeshComponentDetails"

TSharedRef<IDetailCustomization> FProceduralMeshComponentDetails::MakeInstance()
{
	return MakeShareable(new FProceduralMeshComponentDetails);
}

void FProceduralMeshComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& ProcMeshCategory = DetailBuilder.EditCategory("ProceduralMesh");

	const FText ConvertToStaticMeshText = LOCTEXT("ConvertToStaticMesh", "Create StaticMesh");

	// Cache set of selected things
	SelectedObjectsList = DetailBuilder.GetDetailsView().GetSelectedObjects();

	ProcMeshCategory.AddCustomRow(ConvertToStaticMeshText, false)
	.NameContent()
	[
		SNullWidget::NullWidget
	]
	.ValueContent()
	.VAlign(VAlign_Center)
	.MaxDesiredWidth(250)
	[
		SNew(SButton)
		.VAlign(VAlign_Center)
		.ToolTipText(LOCTEXT("ConvertToStaticMeshTooltip", "Create a new StaticMesh asset using current geometry from this ProceduralMeshComponent. Does not modify instance."))
		.OnClicked(this, &FProceduralMeshComponentDetails::ClickedOnConvertToStaticMesh)
		.Content()
		[
			SNew(STextBlock)
			.Text(ConvertToStaticMeshText)
		]
	];
}

FReply FProceduralMeshComponentDetails::ClickedOnConvertToStaticMesh()
{
	// Find first selected ProcMeshComp
	UProceduralMeshComponent* ProcMeshComp = nullptr;
	for (TWeakObjectPtr<UObject>& Object : SelectedObjectsList)
	{
		ProcMeshComp = Cast<UProceduralMeshComponent>(Object.Get());
		if (ProcMeshComp != nullptr)
		{
			break;
		}
	}

	if (ProcMeshComp != nullptr)
	{
		FString NewNameSuggestion = FString(TEXT("ProcMesh"));
		FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			// Get the full name of where we want to create the physics asset.
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MeshName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MeshName = *Name;
			}

			// Then find/create it.
			UPackage* Package = CreatePackage(NULL, *UserPackageName);
			check(Package);

			// Create StaticMesh object
			UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
			StaticMesh->InitResources();

			StaticMesh->LightingGuid = FGuid::NewGuid();

			// Raw mesh data we are filling in
			FRawMesh RawMesh;

			const int32 NumSections = ProcMeshComp->GetNumSections();
			int32 VertexBase = 0;
			for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
			{
				FProcMeshSection* ProcSection = ProcMeshComp->GetProcMeshSection(SectionIdx);

				// Copy verts
				for (FProcMeshVertex& Vert : ProcSection->ProcVertexBuffer)
				{
					RawMesh.VertexPositions.Add(Vert.Position);
				}

				// Copy 'wedge' info
				int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
				for (int32 IndexIdx=0; IndexIdx < NumIndices; IndexIdx++)
				{
					int32 Index = ProcSection->ProcIndexBuffer[IndexIdx];

					RawMesh.WedgeIndices.Add(Index + VertexBase);

					FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[Index];

					FVector TangentX = ProcVertex.Tangent.TangentX;
					FVector TangentZ = ProcVertex.Normal;
					FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal() * (ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f);

					RawMesh.WedgeTangentX.Add(TangentX);
					RawMesh.WedgeTangentY.Add(TangentY);
					RawMesh.WedgeTangentZ.Add(TangentZ);

					RawMesh.WedgeTexCoords[0].Add(ProcVertex.UV0);
					RawMesh.WedgeColors.Add(ProcVertex.Color);
				}

				// copy face info
				int32 NumTris = NumIndices / 3;
				for (int32 TriIdx=0; TriIdx < NumTris; TriIdx++)
				{
					RawMesh.FaceMaterialIndices.Add(SectionIdx);
					RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
				}

				// Copy material
				StaticMesh->Materials.Add(ProcMeshComp->GetMaterial(SectionIdx));

				// Update offset for creating one big index/vertex buffer
				VertexBase += ProcSection->ProcVertexBuffer.Num();
			}

			// Add source to new StaticMesh
			FStaticMeshSourceModel* SrcModel = new (StaticMesh->SourceModels) FStaticMeshSourceModel();
			SrcModel->BuildSettings.bRecomputeNormals = false;
			SrcModel->BuildSettings.bRecomputeTangents = false;
			SrcModel->BuildSettings.bRemoveDegenerates = false;
			SrcModel->BuildSettings.bUseHighPrecisionTangentBasis = false;
			SrcModel->BuildSettings.bUseFullPrecisionUVs = false;
			SrcModel->BuildSettings.bGenerateLightmapUVs = true;
			SrcModel->BuildSettings.SrcLightmapIndex = 0;
			SrcModel->BuildSettings.DstLightmapIndex = 1;
			SrcModel->RawMeshBulkData->SaveRawMesh(RawMesh);

			// Build mesh from source
			StaticMesh->Build(false);
			StaticMesh->PostEditChange();

			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
