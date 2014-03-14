// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
* Copyright 2009 Autodesk, Inc.  All Rights Reserved.
*
* Permission to use, copy, modify, and distribute this software in object
* code form for any purpose and without fee is hereby granted, provided
* that the above copyright notice appears in all copies and that both
* that copyright notice and the limited warranty and restricted rights
* notice below appear in all supporting documentation.
*
* AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS.
* AUTODESK SPECIFICALLY DISCLAIMS ANY AND ALL WARRANTIES, WHETHER EXPRESS
* OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTY
* OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE OR NON-INFRINGEMENT
* OF THIRD PARTY RIGHTS.  AUTODESK DOES NOT WARRANT THAT THE OPERATION
* OF THE PROGRAM WILL BE UNINTERRUPTED OR ERROR FREE.
*
* In no event shall Autodesk, Inc. be liable for any direct, indirect,
* incidental, special, exemplary, or consequential damages (including,
* but not limited to, procurement of substitute goods or services;
* loss of use, data, or profits; or business interruption) however caused
* and on any theory of liability, whether in contract, strict liability,
* or tort (including negligence or otherwise) arising in any way out
* of such code.
*
* This software is provided to the U.S. Government with the same rights
* and restrictions as described herein.
*/

#include "UnrealEd.h"
#include "Factories.h"
#include "Engine.h"
#include "TextureLayout.h"
#include "EngineMaterialClasses.h"
#include "FbxImporter.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogFbxMaterialImport, Log, All);

using namespace UnFbx;

UTexture* UnFbx::FFbxImporter::ImportTexture( FbxFileTexture* FbxTexture, bool bSetupAsNormalMap )
{
	// create an unreal texture asset
	UTexture* UnrealTexture = NULL;
	FString Filename1 = ANSI_TO_TCHAR(FbxTexture->GetFileName());
	FString Extension = FPaths::GetExtension(Filename1).ToLower();
	// name the texture with file name
	FString TextureName = FPaths::GetBaseFilename(Filename1);

	TextureName = ObjectTools::SanitizeObjectName(TextureName);

	// set where to place the textures
	FString NewPackageName = FPackageName::GetLongPackagePath(Parent->GetOutermost()->GetName()) + TEXT("/") + TextureName;
	NewPackageName = PackageTools::SanitizePackageName(NewPackageName);
	UPackage* Package = CreatePackage(NULL, *NewPackageName);

	bool AlreadyExistTexture = (FindObject<UTexture>(Package,*TextureName/*textureName.GetName()*/) != NULL);

	// try opening from absolute path
	FString Filename = Filename1;
	TArray<uint8> DataBinary;
	if ( ! FFileHelper::LoadFileToArray( DataBinary, *Filename ))
	{
		// try fbx file base path + relative path
		FString Filename2 = FileBasePath / ANSI_TO_TCHAR(FbxTexture->GetRelativeFileName());
		Filename = Filename2;
		if ( ! FFileHelper::LoadFileToArray( DataBinary, *Filename ))
		{
			// try fbx file base path + texture file name (no path)
			FString Filename3 = ANSI_TO_TCHAR(FbxTexture->GetRelativeFileName());
			FString FileOnly = FPaths::GetCleanFilename(Filename3);
			Filename3 = FileBasePath / FileOnly;
			Filename = Filename3;
			if ( ! FFileHelper::LoadFileToArray( DataBinary, *Filename ))
			{
				UE_LOG(LogFbxMaterialImport, Warning,TEXT("Unable to find TEXTure file %s. Tried:\n - %s\n - %s\n - %s"),*FileOnly,*Filename1,*Filename2,*Filename3);
			}
		}
	}
	if (DataBinary.Num()>0)
	{
		UE_LOG(LogFbxMaterialImport, Warning, TEXT("Loading texture file %s"),*Filename);
		const uint8* PtrTexture = DataBinary.GetTypedData();
		UTextureFactory* TextureFact = new UTextureFactory(FPostConstructInitializeProperties());

		// save texture settings if texture exist
		TextureFact->SuppressImportOverwriteDialog();
		const TCHAR* TextureType = *Extension;

		// Unless the normal map setting is used during import, 
		//	the user has to manually hit "reimport" then "recompress now" button
		if ( bSetupAsNormalMap )
		{
			if (!AlreadyExistTexture)
			{
				TextureFact->LODGroup = TEXTUREGROUP_WorldNormalMap;
				TextureFact->CompressionSettings = TC_Normalmap;
			}
			else
			{
				UE_LOG(LogFbxMaterialImport, Warning, TEXT("Manual texture reimport and recompression may be needed for %s"), *TextureName);
			}
		}

		UnrealTexture = (UTexture*)TextureFact->FactoryCreateBinary(
			UTexture2D::StaticClass(), Package, *TextureName, 
			RF_Standalone|RF_Public, NULL, TextureType, 
			PtrTexture, PtrTexture+DataBinary.Num(), GWarn );

		if ( UnrealTexture != NULL )
		{
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(UnrealTexture);

			// Set the dirty flag so this package will get saved later
			Package->SetDirtyFlag(true);
		}
	}

	return UnrealTexture;
}

void UnFbx::FFbxImporter::ImportTexturesFromNode(FbxNode* Node)
{
	FbxProperty Property;
	int32 NbMat = Node->GetMaterialCount();

	// visit all materials
	int32 MaterialIndex;
	for (MaterialIndex = 0; MaterialIndex < NbMat; MaterialIndex++)
	{
		FbxSurfaceMaterial *Material = Node->GetMaterial(MaterialIndex);

		//go through all the possible textures
		if(Material)
		{
			int32 TextureIndex;
			FBXSDK_FOR_EACH_TEXTURE(TextureIndex)
			{
				Property = Material->FindProperty(FbxLayerElement::sTextureChannelNames[TextureIndex]);

				if( Property.IsValid() )
				{
					FbxTexture * lTexture= NULL;

					//Here we have to check if it's layered textures, or just textures:
					int32 LayeredTextureCount = Property.GetSrcObjectCount(FbxLayeredTexture::ClassId);
					FbxString PropertyName = Property.GetName();
					if(LayeredTextureCount > 0)
					{
						for(int32 LayerIndex=0; LayerIndex<LayeredTextureCount; ++LayerIndex)
						{
							FbxLayeredTexture *lLayeredTexture = FbxCast <FbxLayeredTexture>(Property.GetSrcObject(FbxLayeredTexture::ClassId, LayerIndex));
							int32 NbTextures = lLayeredTexture->GetSrcObjectCount(FbxTexture::ClassId);
							for(int32 TexIndex =0; TexIndex<NbTextures; ++TexIndex)
							{
								FbxFileTexture* Texture = FbxCast <FbxFileTexture> (lLayeredTexture->GetSrcObject(FbxFileTexture::ClassId,TexIndex));
								if(Texture)
								{
									ImportTexture(Texture, PropertyName == FbxSurfaceMaterial::sNormalMap || PropertyName == FbxSurfaceMaterial::sBump);
								}
							}
						}
					}
					else
					{
						//no layered texture simply get on the property
						int32 NbTextures = Property.GetSrcObjectCount(FbxTexture::ClassId);
						for(int32 TexIndex =0; TexIndex<NbTextures; ++TexIndex)
						{

							FbxFileTexture* Texture = FbxCast <FbxFileTexture> (Property.GetSrcObject(FbxFileTexture::ClassId,TexIndex));
							if(Texture)
							{
								ImportTexture(Texture, PropertyName == FbxSurfaceMaterial::sNormalMap || PropertyName == FbxSurfaceMaterial::sBump);
							}
						}
					}
				}
			}

		}//end if(Material)

	}// end for MaterialIndex
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
bool UnFbx::FFbxImporter::CreateAndLinkExpressionForMaterialProperty(
							FbxSurfaceMaterial& FbxMaterial,
							UMaterial* UnrealMaterial,
							const char* MaterialProperty ,
							FExpressionInput& MaterialInput, 
							bool bSetupAsNormalMap,
							TArray<FString>& UVSet )
{
	bool bCreated = false;
	FbxProperty FbxProperty = FbxMaterial.FindProperty( MaterialProperty );
	if( FbxProperty.IsValid() )
	{
		int32 LayeredTextureCount = FbxProperty.GetSrcObjectCount(FbxLayeredTexture::ClassId);
		if (LayeredTextureCount>0)
		{
			UE_LOG(LogFbxMaterialImport, Warning,TEXT("Layered TEXTures are not supported (material %s)"),ANSI_TO_TCHAR(FbxMaterial.GetName()));
		}
		else
		{
			int32 TextureCount = FbxProperty.GetSrcObjectCount(FbxTexture::ClassId);
			if (TextureCount>0)
			{
				for(int32 TextureIndex =0; TextureIndex<TextureCount; ++TextureIndex)
				{
					FbxFileTexture* FbxTexture = FbxProperty.GetSrcObject(FBX_TYPE(FbxFileTexture), TextureIndex);

					// create an unreal texture asset
					UTexture* UnrealTexture = ImportTexture(FbxTexture, bSetupAsNormalMap);
				
					if (UnrealTexture)
					{
						// and link it to the material 
						UMaterialExpressionTextureSample* UnrealTextureExpression = ConstructObject<UMaterialExpressionTextureSample>( UMaterialExpressionTextureSample::StaticClass(), UnrealMaterial );
						UnrealMaterial->Expressions.Add( UnrealTextureExpression );
						MaterialInput.Expression = UnrealTextureExpression;
						UnrealTextureExpression->Texture = UnrealTexture;
						UnrealTextureExpression->SamplerType = bSetupAsNormalMap ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
						
						// add/find UVSet and set it to the texture
						FbxString UVSetName = FbxTexture->UVSet.Get();
						FString LocalUVSetName = ANSI_TO_TCHAR(UVSetName.Buffer());
						int32 SetIndex = UVSet.Find(LocalUVSetName);
						UMaterialExpressionTextureCoordinate* MyCoordExpression = ConstructObject<UMaterialExpressionTextureCoordinate>( UMaterialExpressionTextureCoordinate::StaticClass(), UnrealMaterial );
						UnrealMaterial->Expressions.Add( MyCoordExpression );
						MyCoordExpression->CoordinateIndex = (SetIndex >= 0)? SetIndex: 0;
						UnrealTextureExpression->Coordinates.Expression = MyCoordExpression;

						if ( !bSetupAsNormalMap )
						{
							UnrealMaterial->BaseColor.Expression = UnrealTextureExpression;
						}
						else
						{
							UnrealMaterial->Normal.Expression = UnrealTextureExpression;
						}

						bCreated = true;
					}		
				}
			}

			if (MaterialInput.Expression)
			{
				TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
				FExpressionOutput* Output = Outputs.GetTypedData();
				MaterialInput.Mask = Output->Mask;
				MaterialInput.MaskR = Output->MaskR;
				MaterialInput.MaskG = Output->MaskG;
				MaterialInput.MaskB = Output->MaskB;
				MaterialInput.MaskA = Output->MaskA;
			}
		}
	}

	return bCreated;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void UnFbx::FFbxImporter::FixupMaterial( FbxSurfaceMaterial& FbxMaterial, UMaterial* UnrealMaterial )
{
	// add a basic diffuse color if no texture is linked to diffuse
	if (UnrealMaterial->BaseColor.Expression == NULL)
	{
		FbxDouble3 DiffuseColor;
		
		UMaterialExpressionVectorParameter* MyColorExpression = ConstructObject<UMaterialExpressionVectorParameter>( UMaterialExpressionVectorParameter::StaticClass(), UnrealMaterial );
		UnrealMaterial->Expressions.Add( MyColorExpression );
		UnrealMaterial->BaseColor.Expression = MyColorExpression;

		bool bFoundDiffuseColor = true;
		if( FbxMaterial.GetClassId().Is(FbxSurfacePhong::ClassId) )
		{
			DiffuseColor = ((FbxSurfacePhong&)(FbxMaterial)).Diffuse.Get();
		}
		else if( FbxMaterial.GetClassId().Is(FbxSurfaceLambert::ClassId) )
		{
			DiffuseColor = ((FbxSurfaceLambert&)(FbxMaterial)).Diffuse.Get();
		}
		else
		{
			bFoundDiffuseColor = false;
		}

		if( bFoundDiffuseColor )
		{
			MyColorExpression->DefaultValue.R = (float)(DiffuseColor[0]);
			MyColorExpression->DefaultValue.G = (float)(DiffuseColor[1]);
			MyColorExpression->DefaultValue.B = (float)(DiffuseColor[2]);
		}
		else
		{
			// use random color because there may be multiple materials, then they can be different 
			MyColorExpression->DefaultValue.R = 0.5f+(0.5f*FMath::Rand())/RAND_MAX;
			MyColorExpression->DefaultValue.G = 0.5f+(0.5f*FMath::Rand())/RAND_MAX;
			MyColorExpression->DefaultValue.B = 0.5f+(0.5f*FMath::Rand())/RAND_MAX;
		}

		TArray<FExpressionOutput> Outputs = UnrealMaterial->BaseColor.Expression->GetOutputs();
		FExpressionOutput* Output = Outputs.GetTypedData();
		UnrealMaterial->BaseColor.Mask = Output->Mask;
		UnrealMaterial->BaseColor.MaskR = Output->MaskR;
		UnrealMaterial->BaseColor.MaskG = Output->MaskG;
		UnrealMaterial->BaseColor.MaskB = Output->MaskB;
		UnrealMaterial->BaseColor.MaskA = Output->MaskA;
	}
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------

void UnFbx::FFbxImporter::CreateUnrealMaterial(FbxSurfaceMaterial* FbxMaterial, TArray<UMaterialInterface*>& OutMaterials, TArray<FString>& UVSets)
{
	FString MaterialFullName = ANSI_TO_TCHAR(MakeName(FbxMaterial->GetName()));

	// check for a 'skinXX' suffix in the material name
	int32 MaterialNameLen = FCString::Strlen(*MaterialFullName) + 1;
	char* MaterialNameANSI = new char[MaterialNameLen];
	FCStringAnsi::Strcpy(MaterialNameANSI, MaterialNameLen, TCHAR_TO_ANSI(*MaterialFullName));
	if (FCStringAnsi::Strlen(MaterialNameANSI) > 6)
	{
		const char* SkinXX = MaterialNameANSI + FCStringAnsi::Strlen(MaterialNameANSI) - 6;
		if (FCharAnsi::ToUpper(*SkinXX) == 'S' && FCharAnsi::ToUpper(*(SkinXX+1)) == 'K' && 
			FCharAnsi::ToUpper(*(SkinXX+2)) == 'I' && FCharAnsi::ToUpper(*(SkinXX+3)) == 'N')
		{
			if (FCharAnsi::IsDigit(*(SkinXX+4)) && FCharAnsi::IsDigit(*(SkinXX+5)))
			{
				// remove the 'skinXX' suffix from the material name
				MaterialFullName = MaterialFullName.Left(MaterialNameLen - 7);
			}
		}
	}

	MaterialFullName = ObjectTools::SanitizeObjectName(MaterialFullName);

	// Make sure we have a parent
	if ( !ensure(Parent) )
	{
		return;
	}
	
	// set where to place the materials
	FString NewPackageName = FPackageName::GetLongPackagePath(Parent->GetOutermost()->GetName()) + TEXT("/") + MaterialFullName;
	NewPackageName = PackageTools::SanitizePackageName(NewPackageName);
	UPackage* Package = CreatePackage(NULL, *NewPackageName);
	
	UMaterialInterface* UnrealMaterialInterface = FindObject<UMaterialInterface>(Package,*MaterialFullName);
	// does not override existing materials
	if (UnrealMaterialInterface != NULL)
	{
		OutMaterials.Add(UnrealMaterialInterface);
		return;
	}
	
	// create an unreal material asset
	UMaterialFactoryNew* MaterialFactory = new UMaterialFactoryNew(FPostConstructInitializeProperties());
	
	UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(), Package, *MaterialFullName, RF_Standalone|RF_Public, NULL, GWarn );
	// TODO :  need this ? UnrealMaterial->bUsedWithStaticLighting = true;

	if ( UnrealMaterial != NULL )
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(UnrealMaterial);

		// Set the dirty flag so this package will get saved later
		Package->SetDirtyFlag(true);
	}

	// textures and properties
	CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sDiffuse, UnrealMaterial->DiffuseColor, false, UVSets);
	CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sEmissive, UnrealMaterial->EmissiveColor, false, UVSets);
	CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sSpecular, UnrealMaterial->SpecularColor, false, UVSets);
	CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sSpecularFactor, UnrealMaterial->SpecularColor, false, UVSets); // SpecularFactor modulates the SpecularColor value if there's one
	//CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sShininess, UnrealMaterial->SpecularPower, false, UVSets);
	if (!CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sNormalMap, UnrealMaterial->Normal, true, UVSets))
	{
		CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, FbxSurfaceMaterial::sBump, UnrealMaterial->Normal, true, UVSets); // no bump in unreal, use as normal map
	}
	//CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, KFbxSurfaceMaterial::sTransparentColor, UnrealMaterial->Opacity, false, UVSets);
	//CreateAndLinkExpressionForMaterialProperty( *FbxMaterial, UnrealMaterial, KFbxSurfaceMaterial::sTransparencyFactor, UnrealMaterial->OpacityMask, false, UVSets);
	FixupMaterial( *FbxMaterial, UnrealMaterial); // add random diffuse if none exists

	// compile shaders for PC (from UPrecompileShadersCommandlet::ProcessMaterial
	// and FMaterialEditor::UpdateOriginalMaterial)

	// make sure that any static meshes, etc using this material will stop using the FMaterialResource of the original 
	// material, and will use the new FMaterialResource created when we make a new UMaterial in place
	FGlobalComponentReregisterContext RecreateComponents;
	
	// let the material update itself if necessary
	UnrealMaterial->PreEditChange(NULL);
		UnrealMaterial->PostEditChange();
	
	OutMaterials.Add(UnrealMaterial);
}

int32 UnFbx::FFbxImporter::CreateNodeMaterials(FbxNode* FbxNode, TArray<UMaterialInterface*>& OutMaterials, TArray<FString>& UVSets)
{
	int32 MaterialCount = FbxNode->GetMaterialCount();
	for(int32 MaterialIndex=0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		FbxSurfaceMaterial *FbxMaterial = FbxNode->GetMaterial(MaterialIndex);

		CreateUnrealMaterial(FbxMaterial, OutMaterials, UVSets);
	}
	return MaterialCount;
}
