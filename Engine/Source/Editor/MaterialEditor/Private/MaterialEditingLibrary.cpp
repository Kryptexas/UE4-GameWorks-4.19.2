// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "MaterialEditingLibrary.h"
#include "MaterialEditor.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionStaticComponentMaskParameter.h"
#include "Materials/MaterialExpressionTransformPosition.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialParameterCollection.h"


DEFINE_LOG_CATEGORY_STATIC(LogMaterialEditingLibrary, Warning, All);

/** Util to find expression  */
static FExpressionInput* GetExpressionInputByName(UMaterialExpression* Expression, const FString& InputName)
{
	check(Expression);
	FExpressionInput* Result = nullptr;

	TArray<FExpressionInput*> Inputs = Expression->GetInputs();

	// Return first input if no name specified
	if (InputName.IsEmpty())
	{
		if (Inputs.Num() > 0)
		{
			return Inputs[0];
		}
	}
	else
	{
		// Get all inputs
		// Get name of each input, see if its the one we want
		for (int InputIdx = 0; InputIdx < Inputs.Num(); InputIdx++)
		{
			FString TestName = Expression->GetInputName(InputIdx);
			TestName = UMaterialGraphNode::GetShortenPinName(TestName);
			if (TestName == InputName)
			{
				Result = Inputs[InputIdx];
				break;
			}
		}
	}

	return Result;
}

static int32 GetExpressionOutputIndexByName(UMaterialExpression* Expression, const FString& OutputName)
{
	check(Expression);
	
	int32 Result = INDEX_NONE;

	if (Expression->Outputs.Num() == 0)
	{
		// leave as INDEX_NONE
	}
	// Return first output if no name specified
	else if (OutputName.IsEmpty())
	{
		Result = 0;
	}
	else
	{
		// Iterate over outputs and look for name match
		for (int OutIdx = 0; OutIdx < Expression->Outputs.Num(); OutIdx++)
		{
			bool bFoundMatch = false;

			FExpressionOutput& Output = Expression->Outputs[OutIdx];
			// If output name is no empty - see if it matches
			if(!Output.OutputName.IsEmpty())
			{
				if (OutputName == Output.OutputName)
				{
					bFoundMatch = true;
				}
			}
			// if it is empty we look for R/G/B/A
			else
			{
				if (OutputName == TEXT("R") && Output.MaskR && !Output.MaskG && !Output.MaskB && !Output.MaskA)
				{
					bFoundMatch = true;
				}
				else if (OutputName == TEXT("G") && !Output.MaskR && Output.MaskG && !Output.MaskB && !Output.MaskA)
				{
					bFoundMatch = true;
				}
				else if (OutputName == TEXT("B") && !Output.MaskR && !Output.MaskG && Output.MaskB && !Output.MaskA)
				{
					bFoundMatch = true;
				}
				else if (OutputName == TEXT("A") && !Output.MaskR && !Output.MaskG && !Output.MaskB && Output.MaskA)
				{
					bFoundMatch = true;
				}
			}

			// Got a match, remember the index, exit iteration
			if (bFoundMatch)
			{
				Result = OutIdx;
				break;
			}
		}
	}

	return Result;
}

int32 UMaterialEditingLibrary::GetNumExpressions(const UMaterial* Material)
{
	int32 Result = 0;
	if (Material)
	{
		Result = Material->Expressions.Num();
	}
	return Result;
}

UMaterialExpression* UMaterialEditingLibrary::CreateMaterialExpression(UMaterial* Material, TSubclassOf<UMaterialExpression> ExpressionClass, int32 NodePosX, int32 NodePosY)
{
	return CreateMaterialExpressionEx(Material, nullptr, ExpressionClass, nullptr, NodePosX, NodePosY);
}

UMaterialExpression* UMaterialEditingLibrary::CreateMaterialExpressionEx(UMaterial* Material, UMaterialFunction* MaterialFunction, TSubclassOf<UMaterialExpression> ExpressionClass, UObject* SelectedAsset, int32 NodePosX, int32 NodePosY)
{
	UMaterialExpression* NewExpression = nullptr;
	if (Material)
	{
		UObject* ExpressionOuter = Material;
		if (MaterialFunction)
		{
			ExpressionOuter = MaterialFunction;
		}

		NewExpression = NewObject<UMaterialExpression>(ExpressionOuter, ExpressionClass.Get(), NAME_None, RF_Transactional);
		Material->Expressions.Add(NewExpression);
		NewExpression->Material = Material;

		NewExpression->MaterialExpressionEditorX = NodePosX;
		NewExpression->MaterialExpressionEditorY = NodePosY;

		// Create a GUID for the node
		NewExpression->UpdateMaterialExpressionGuid(true, true);

		if (SelectedAsset)
		{
			// If the user is adding a texture, automatically assign the currently selected texture to it.
			UMaterialExpressionTextureBase* METextureBase = Cast<UMaterialExpressionTextureBase>(NewExpression);
			if (METextureBase)
			{
				if (UTexture* SelectedTexture = Cast<UTexture>(SelectedAsset))
				{
					METextureBase->Texture = SelectedTexture;
				}
				METextureBase->AutoSetSampleType();
			}

			UMaterialExpressionMaterialFunctionCall* MEMaterialFunction = Cast<UMaterialExpressionMaterialFunctionCall>(NewExpression);
			if (MEMaterialFunction)
			{
				MEMaterialFunction->SetMaterialFunction(MaterialFunction, NULL, Cast<UMaterialFunction>(SelectedAsset));
			}

			UMaterialExpressionCollectionParameter* MECollectionParameter = Cast<UMaterialExpressionCollectionParameter>(NewExpression);
			if (MECollectionParameter)
			{
				MECollectionParameter->Collection = Cast<UMaterialParameterCollection>(SelectedAsset);
			}
		}

		UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>(NewExpression);
		if (FunctionInput)
		{
			FunctionInput->ConditionallyGenerateId(true);
			FunctionInput->ValidateName();
		}

		UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(NewExpression);
		if (FunctionOutput)
		{
			FunctionOutput->ConditionallyGenerateId(true);
			FunctionOutput->ValidateName();
		}

		NewExpression->UpdateParameterGuid(true, true);

		if (NewExpression->HasAParameterName())
		{
			NewExpression->ValidateParameterName();
		}

		UMaterialExpressionComponentMask* ComponentMaskExpression = Cast<UMaterialExpressionComponentMask>(NewExpression);
		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		if (ComponentMaskExpression)
		{
			ComponentMaskExpression->R = true;
			ComponentMaskExpression->G = true;
		}

		UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskExpression = Cast<UMaterialExpressionStaticComponentMaskParameter>(NewExpression);
		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		if (StaticComponentMaskExpression)
		{
			StaticComponentMaskExpression->DefaultR = true;
		}

		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		UMaterialExpressionTransformPosition* PositionTransform = Cast<UMaterialExpressionTransformPosition>(NewExpression);
		if (PositionTransform)
		{
			PositionTransform->TransformSourceType = TRANSFORMPOSSOURCE_Local;
			PositionTransform->TransformType = TRANSFORMPOSSOURCE_World;
		}

		// Make sure the dynamic parameters are named based on existing ones
		UMaterialExpressionDynamicParameter* DynamicExpression = Cast<UMaterialExpressionDynamicParameter>(NewExpression);
		if (DynamicExpression)
		{
			DynamicExpression->UpdateDynamicParameterProperties();
		}

		Material->AddExpressionParameter(NewExpression, Material->EditorParameters);

		Material->MarkPackageDirty();
	}
	return NewExpression;
}

bool UMaterialEditingLibrary::SetMaterialUsage(UMaterial* Material, EMaterialUsage Usage, bool& bNeedsRecompile)
{
	bool bResult = false;
	bNeedsRecompile = false;
	if (Material)
	{
		bResult = Material->SetMaterialUsage(bNeedsRecompile, Usage);
	}
	return bResult;
}

bool UMaterialEditingLibrary::ConnectMaterialProperty(UMaterialExpression* FromExpression, FString FromOutputName, EMaterialProperty Property)
{
	bool bResult = false;
	if (FromExpression)
	{
		// Get material that owns this expression
		UMaterial* Material = Cast<UMaterial>(FromExpression->GetOuter());
		if (Material)
		{
			FExpressionInput* Input = Material->GetExpressionInputForProperty(Property);
			int32 FromIndex = GetExpressionOutputIndexByName(FromExpression, FromOutputName);
			if (Input && FromIndex != INDEX_NONE)
			{
				Input->Connect(FromIndex, FromExpression);
				bResult = true;
			}
		}
	}
	return bResult;
}

bool UMaterialEditingLibrary::ConnectMaterialExpressions(UMaterialExpression* FromExpression, FString FromOutputName, UMaterialExpression* ToExpression, FString ToInputName)
{
	bool bResult = false;
	if (FromExpression && ToExpression)
	{
		FExpressionInput* Input = GetExpressionInputByName(ToExpression, ToInputName);
		int32 FromIndex = GetExpressionOutputIndexByName(FromExpression, FromOutputName);
		if (Input && FromIndex != INDEX_NONE)
		{
			Input->Connect(FromIndex, FromExpression);
			bResult = true;
		}
	}
	return bResult;
}

void UMaterialEditingLibrary::RecompileMaterial(UMaterial* Material)
{
	FMaterialUpdateContext UpdateContext;

	UpdateContext.AddMaterial(Material);

	// Propagate the change to this material
	Material->PreEditChange(NULL);
	Material->PostEditChange();
}