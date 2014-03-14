// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "MaterialEditorModule.h"
#include "MaterialEditorUtilities.h"
#include "Toolkits/ToolkitManager.h"
#include "MaterialExpressionClasses.h"

#define LOCTEXT_NAMESPACE "MaterialEditorUtilities"

DEFINE_LOG_CATEGORY_STATIC(LogMaterialEditorUtilities, Log, All);

UMaterialExpression* FMaterialEditorUtilities::CreateNewMaterialExpression(const class UEdGraph* Graph, UClass* NewExpressionClass, const FVector2D& NodePos, bool bAutoSelect, bool bAutoAssignResource)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		return MaterialEditor->CreateNewMaterialExpression(NewExpressionClass, NodePos, bAutoSelect, bAutoAssignResource);
	}
	return NULL;
}

UMaterialExpressionComment* FMaterialEditorUtilities::CreateNewMaterialExpressionComment(const class UEdGraph* Graph, const FVector2D& NodePos)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		return MaterialEditor->CreateNewMaterialExpressionComment(NodePos);
	}
	return NULL;
}

void FMaterialEditorUtilities::ForceRefreshExpressionPreviews(const class UEdGraph* Graph)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		MaterialEditor->ForceRefreshExpressionPreviews();
	}
}

void FMaterialEditorUtilities::AddToSelection(const class UEdGraph* Graph, UMaterialExpression* Expression)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		MaterialEditor->AddToSelection(Expression);
	}
}

void FMaterialEditorUtilities::DeleteSelectedNodes(const class UEdGraph* Graph)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		MaterialEditor->DeleteSelectedNodes();
	}
}

FString FMaterialEditorUtilities::GetOriginalObjectName(const class UEdGraph* Graph)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		return MaterialEditor->GetOriginalObjectName();
	}
	return TEXT("");
}

void FMaterialEditorUtilities::UpdateMaterialAfterGraphChange(const class UEdGraph* Graph)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if (MaterialEditor.IsValid())
	{
		MaterialEditor->UpdateMaterialAfterGraphChange();
	}
}

bool FMaterialEditorUtilities::CanPasteNodes(const class UEdGraph* Graph)
{
	bool bCanPaste = false;
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if(MaterialEditor.IsValid())
	{
		bCanPaste = MaterialEditor->CanPasteNodes();
	}
	return bCanPaste;
}

void FMaterialEditorUtilities::PasteNodesHere(class UEdGraph* Graph, const FVector2D& Location)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if(MaterialEditor.IsValid())
	{
		MaterialEditor->PasteNodesHere(Location);
	}
}

int32 FMaterialEditorUtilities::GetNumberOfSelectedNodes(const class UEdGraph* Graph)
{
	int32 SelectedNodes = 0;
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if(MaterialEditor.IsValid())
	{
		SelectedNodes = MaterialEditor->GetNumberOfSelectedNodes();
	}
	return SelectedNodes;
}

void FMaterialEditorUtilities::GetMaterialExpressionActions(FGraphActionMenuBuilder& ActionMenuBuilder, bool bMaterialFunction)
{
	// TODO: Not sure if this is necessary/usable anymore
	// Get all menu extenders for this context menu from the material editor module
	/*IMaterialEditorModule& MaterialEditor = FModuleManager::GetModuleChecked<IMaterialEditorModule>( TEXT("MaterialEditor") );
	TArray<IMaterialEditorModule::FMaterialMenuExtender> MenuExtenderDelegates = MaterialEditor.GetAllMaterialCanvasMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(MaterialEditorPtr.Pin()->GetToolkitCommands()));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);*/

	bool bUseUnsortedMenus = false;
	MaterialExpressionClasses* ExpressionClasses = MaterialExpressionClasses::Get();

	if (bUseUnsortedMenus)
	{
		AddMaterialExpressionCategory(ActionMenuBuilder, TEXT(""), &ExpressionClasses->AllExpressionClasses, bMaterialFunction);
	}
	else
	{
		// Add Favourite expressions as a category
		const FText FavouritesCategory = LOCTEXT("FavoritesMenu", "Favorites");
		AddMaterialExpressionCategory(ActionMenuBuilder, FavouritesCategory.ToString(), &ExpressionClasses->FavoriteExpressionClasses, bMaterialFunction);

		// Add each category to the menu
		for (int32 CategoryIndex = 0; CategoryIndex < ExpressionClasses->CategorizedExpressionClasses.Num(); ++CategoryIndex)
		{
			FCategorizedMaterialExpressionNode* CategoryNode = &(ExpressionClasses->CategorizedExpressionClasses[CategoryIndex]);
			AddMaterialExpressionCategory(ActionMenuBuilder, CategoryNode->CategoryName, &CategoryNode->MaterialExpressions, bMaterialFunction);
		}

		if (ExpressionClasses->UnassignedExpressionClasses.Num() > 0)
		{
			AddMaterialExpressionCategory(ActionMenuBuilder, TEXT(""), &ExpressionClasses->UnassignedExpressionClasses, bMaterialFunction);
		}
	}
}

bool FMaterialEditorUtilities::IsMaterialExpressionInFavorites(UMaterialExpression* InExpression)
{
	return MaterialExpressionClasses::Get()->IsMaterialExpressionInFavorites(InExpression);
}

FMaterialRenderProxy* FMaterialEditorUtilities::GetExpressionPreview(const class UEdGraph* Graph, UMaterialExpression* InExpression)
{
	FMaterialRenderProxy* ExpressionPreview = NULL;
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if(MaterialEditor.IsValid())
	{
		ExpressionPreview = MaterialEditor->GetExpressionPreview(InExpression);
	}
	return ExpressionPreview;
}

void FMaterialEditorUtilities::UpdateSearchResults(const class UEdGraph* Graph)
{
	TSharedPtr<class IMaterialEditor> MaterialEditor = GetIMaterialEditorForObject(Graph);
	if(MaterialEditor.IsValid())
	{
		MaterialEditor->UpdateSearch(false);
	}
}

/////////////////////////////////////////////////////
// Static functions moved from SMaterialEditorCanvas

void FMaterialEditorUtilities::GetVisibleMaterialParameters(const UMaterial *Material, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions)
{
	VisibleExpressions.Empty();

	TArray<UMaterialExpression*> ProcessedExpressions;

	for(uint32 i = 0; i < MP_MAX; ++i)
	{
		FExpressionInput* ExpressionInput = GetMaterialInput((UMaterial *)Material, i);

		GetVisibleMaterialParametersFromExpression(ExpressionInput->Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
	}
}

bool FMaterialEditorUtilities::GetStaticSwitchExpressionValue(UMaterialInstance* MaterialInstance, UMaterialExpression *SwitchValueExpression, bool& OutValue, FGuid& OutExpressionID, const TArray<FFunctionExpressionInput>* FunctionInputs)
{
	// If switch value is a function input expression then we must recursively find the associated input expressions from the parent function/material to evaluate the value.
	UMaterialExpressionFunctionInput* FunctionInputExpression =  Cast<UMaterialExpressionFunctionInput>(SwitchValueExpression);
	if(FunctionInputExpression && FunctionInputExpression->InputType == FunctionInput_StaticBool)
	{
		// Get the FFunctionExpressionInput which stores information about the input node from the parent that this is linked to.
		const FFunctionExpressionInput* MatchingInput = FindInputById(FunctionInputExpression, *FunctionInputs);
		if (MatchingInput && (MatchingInput->Input.Expression || !FunctionInputExpression->bUsePreviewValueAsDefault))
		{
			GetStaticSwitchExpressionValue(MaterialInstance, MatchingInput->Input.Expression, OutValue, OutExpressionID, FunctionInputs);
		}
		else
		{
			GetStaticSwitchExpressionValue(MaterialInstance, FunctionInputExpression->Preview.Expression, OutValue, OutExpressionID, FunctionInputs);
		}
	}

	// The expression can only be a static bool parameter when the current scope is not within a function
	if(SwitchValueExpression && SwitchValueExpression->Material != NULL)
	{
		UMaterialExpressionStaticBoolParameter* SwitchParamValue = Cast<UMaterialExpressionStaticBoolParameter>(SwitchValueExpression);

		if(SwitchParamValue)
		{
			MaterialInstance->GetStaticSwitchParameterValue(SwitchParamValue->ParameterName, OutValue, OutExpressionID);
			return true;
		}
	}

	UMaterialExpressionStaticBool* StaticSwitchValue = Cast<UMaterialExpressionStaticBool>(SwitchValueExpression);
	if(StaticSwitchValue)
	{
		OutValue = StaticSwitchValue->Value;
		return true;
	}

	return false;
}

bool FMaterialEditorUtilities::IsFunctionContainingSwitchExpressions(UMaterialFunction* MaterialFunction)
{
	if (MaterialFunction)
	{
		TArray<UMaterialFunction*> DependentFunctions;
		MaterialFunction->GetDependentFunctions(DependentFunctions);
		DependentFunctions.AddUnique(MaterialFunction);
		for (int32 FunctionIndex = 0; FunctionIndex < DependentFunctions.Num(); ++FunctionIndex)
		{
			UMaterialFunction* CurrentFunction = DependentFunctions[FunctionIndex];
			for(int32 ExpressionIndex = 0; ExpressionIndex < CurrentFunction->FunctionExpressions.Num(); ++ExpressionIndex )
			{
				UMaterialExpressionStaticSwitch* StaticSwitchExpression = Cast<UMaterialExpressionStaticSwitch>(CurrentFunction->FunctionExpressions[ExpressionIndex]);
				if (StaticSwitchExpression)
				{
					return true;
				}
			}
		}
	}

	return false;
}

const FFunctionExpressionInput* FMaterialEditorUtilities::FindInputById(const UMaterialExpressionFunctionInput* InputExpression, const TArray<FFunctionExpressionInput>& Inputs)
{
	for (int32 InputIndex = 0; InputIndex < Inputs.Num(); InputIndex++)
	{
		const FFunctionExpressionInput& CurrentInput = Inputs[InputIndex];
		if (CurrentInput.ExpressionInputId == InputExpression->Id && CurrentInput.ExpressionInput->GetOuter() == InputExpression->GetOuter())
		{
			return &CurrentInput;
		}
	}
	return NULL;
}

FExpressionInput* FMaterialEditorUtilities::GetMaterialInput(UMaterial* Material, int32 Index)
{
	FExpressionInput* ExpressionInput = NULL;
	switch( Index )
	{
	case 0: ExpressionInput = &Material->DiffuseColor ; break;
	case 1: ExpressionInput = &Material->SpecularColor ; break;
	case 2: ExpressionInput = &Material->BaseColor ; break;
	case 3: ExpressionInput = &Material->Metallic ; break;
	case 4: ExpressionInput = &Material->Specular ; break;
	case 5: ExpressionInput = &Material->Roughness ; break;
	case 6: ExpressionInput = &Material->EmissiveColor ; break;
	case 7: ExpressionInput = &Material->Opacity ; break;
	case 8: ExpressionInput = &Material->OpacityMask ; break;
	case 9: ExpressionInput = &Material->Normal ; break;
	case 10: ExpressionInput = &Material->WorldPositionOffset ; break;
	case 11: ExpressionInput = &Material->WorldDisplacement ; break;
	case 12: ExpressionInput = &Material->TessellationMultiplier ; break;
	case 13: ExpressionInput = &Material->SubsurfaceColor ; break;		
	case 14: ExpressionInput = &Material->AmbientOcclusion ; break;		
	case 15: ExpressionInput = &Material->Refraction ; break;		
	case 24: ExpressionInput = &Material->MaterialAttributes ; break;		
	default: 
		if (Index >= 16 && Index <= 23)
		{
			ExpressionInput = &Material->CustomizedUVs[Index - 16]; break;	
		}
		else
		{
			UE_LOG(LogMaterialEditorUtilities, Fatal, TEXT("%i: Invalid material input index"), Index );
		}
	}
	return ExpressionInput;
}

bool FMaterialEditorUtilities::IsInputVisible(UMaterial* Material, int32 Index)
{
	static const auto UseDiffuseSpecularMaterialInputs = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UseDiffuseSpecularMaterialInputs"));

	bool bVisibleUV = true;
	if (Index >= 16 && Index <= 23)
	{
		bVisibleUV = Index - 16 < Material->NumCustomizedUVs;
	}

	switch( Index )
	{
	case 0:
	case 1:
		return (UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() != 0) && !Material->bUseMaterialAttributes;
	case 2:
	case 3:
	case 4:
		return (UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() == 0) && !Material->bUseMaterialAttributes;
	case 24:
		return Material->bUseMaterialAttributes;
	default:
		return !Material->bUseMaterialAttributes && bVisibleUV;
	}
}

void FMaterialEditorUtilities::InitExpressions(UMaterial* Material)
{
	FString ParmName;

	Material->EditorComments.Empty();
	Material->Expressions.Empty();

	TArray<UObject*> ChildObjects;
	GetObjectsWithOuter(Material, ChildObjects, /*bIncludeNestedObjects=*/false);

	for ( int32 ChildIdx = 0; ChildIdx < ChildObjects.Num(); ++ChildIdx )
	{
		UMaterialExpression* MaterialExpression = Cast<UMaterialExpression>(ChildObjects[ChildIdx]);
		if( MaterialExpression != NULL && !MaterialExpression->IsPendingKill() )
		{
			// Comment expressions are stored in a separate list.
			if ( MaterialExpression->IsA( UMaterialExpressionComment::StaticClass() ) )
			{
				Material->EditorComments.Add( static_cast<UMaterialExpressionComment*>(MaterialExpression) );
			}
			else
			{
				Material->Expressions.Add( MaterialExpression );
			}
		}
	}

	Material->BuildEditorParameterList();

	// Propagate RF_Transactional to all referenced material expressions.
	Material->SetFlags( RF_Transactional );
	for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->Expressions.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions[ MaterialExpressionIndex ];

		if(MaterialExpression)
		{
			MaterialExpression->SetFlags( RF_Transactional );
		}
	}
	for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Material->EditorComments.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionComment* Comment = Material->EditorComments[ MaterialExpressionIndex ];
		Comment->SetFlags( RF_Transactional );
	}
}

///////////
// private

void FMaterialEditorUtilities::GetVisibleMaterialParametersFromExpression(UMaterialExpression *MaterialExpression, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions, TArray<UMaterialExpression*> &ProcessedExpressions, const TArray<FFunctionExpressionInput>* FunctionInputs, TArray<FGuid>* VisibleFunctionInputExpressions)
{
	if(!MaterialExpression)
	{
		return;
	}

	check(MaterialInstance);

	//don't allow re-entrant expressions to continue
	if (ProcessedExpressions.Contains(MaterialExpression))
	{
		return;
	}
	ProcessedExpressions.Push(MaterialExpression);

	if(MaterialExpression->Material != NULL)
	{
		// if it's a material parameter it must be visible so add it to the map
		UMaterialExpressionParameter *Param = Cast<UMaterialExpressionParameter>( MaterialExpression );
		UMaterialExpressionTextureSampleParameter *TexParam = Cast<UMaterialExpressionTextureSampleParameter>( MaterialExpression );
		UMaterialExpressionFontSampleParameter *FontParam = Cast<UMaterialExpressionFontSampleParameter>( MaterialExpression );
		if( Param )
		{
			VisibleExpressions.AddUnique(Param->ExpressionGUID);

			UMaterialExpressionScalarParameter *ScalarParam = Cast<UMaterialExpressionScalarParameter>( MaterialExpression );
			UMaterialExpressionVectorParameter *VectorParam = Cast<UMaterialExpressionVectorParameter>( MaterialExpression );
			TArray<FName> Names;
			TArray<FGuid> Ids;
			if( ScalarParam )
			{
				MaterialInstance->GetMaterial()->GetAllScalarParameterNames( Names, Ids );
				for( int32 i = 0; i < Names.Num(); i++ )
				{
					if( Names[i] == ScalarParam->ParameterName )
					{
						VisibleExpressions.AddUnique( Ids[ i ] );
					}
				}
			}
			else if ( VectorParam )
			{
				MaterialInstance->GetMaterial()->GetAllVectorParameterNames( Names, Ids );
				for( int32 i = 0; i < Names.Num(); i++ )
				{
					if( Names[i] == VectorParam->ParameterName )
					{
						VisibleExpressions.AddUnique( Ids[ i ] );
					}
				}
			}
		}
		else if(TexParam)
		{
			VisibleExpressions.AddUnique( TexParam->ExpressionGUID );
			TArray<FName> Names;
			TArray<FGuid> Ids;
			MaterialInstance->GetMaterial()->GetAllTextureParameterNames( Names, Ids );
			for( int32 i = 0; i < Names.Num(); i++ )
			{
				if( Names[i] == TexParam->ParameterName )
				{
					VisibleExpressions.AddUnique( Ids[ i ] );
				}
			}
		}
		else if(FontParam)
		{
			VisibleExpressions.AddUnique( FontParam->ExpressionGUID );
			TArray<FName> Names;
			TArray<FGuid> Ids;
			MaterialInstance->GetMaterial()->GetAllFontParameterNames( Names, Ids );
			for( int32 i = 0; i < Names.Num(); i++ )
			{
				if( Names[i] == FontParam->ParameterName )
				{
					VisibleExpressions.AddUnique( Ids[ i ] );
				}
			}
		}
	}

	UMaterialExpressionFunctionInput* InputExpression = Cast<UMaterialExpressionFunctionInput>(MaterialExpression);
	if(InputExpression)
	{
		VisibleFunctionInputExpressions->AddUnique(InputExpression->Id);
	}

	// check if it's a switch expression and branch according to its value
	UMaterialExpressionStaticSwitchParameter* StaticSwitchParamExpression = Cast<UMaterialExpressionStaticSwitchParameter>(MaterialExpression);
	UMaterialExpressionStaticSwitch* StaticSwitchExpression = Cast<UMaterialExpressionStaticSwitch>(MaterialExpression);
	UMaterialExpressionMaterialFunctionCall* FunctionCallExpression = Cast<UMaterialExpressionMaterialFunctionCall>(MaterialExpression);
	if(StaticSwitchParamExpression)
	{
		bool Value = false;
		FGuid ExpressionID;
		MaterialInstance->GetStaticSwitchParameterValue(StaticSwitchParamExpression->ParameterName, Value, ExpressionID);
		VisibleExpressions.AddUnique(ExpressionID);

		if(Value)
		{
			GetVisibleMaterialParametersFromExpression(StaticSwitchParamExpression->A.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
		}
		else
		{
			GetVisibleMaterialParametersFromExpression(StaticSwitchParamExpression->B.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions);
		}
	}
	else if(StaticSwitchExpression)
	{
		bool bValue = StaticSwitchExpression->DefaultValue;
		FGuid ExpressionID;
		if(StaticSwitchExpression->Value.Expression)
		{

			GetStaticSwitchExpressionValue(MaterialInstance, StaticSwitchExpression->Value.Expression, bValue, ExpressionID, FunctionInputs );

			// Flag the switch value as a visible expression
			UMaterialExpressionFunctionInput* FunctionInputExpression =  Cast<UMaterialExpressionFunctionInput>(StaticSwitchExpression->Value.Expression);
			if(FunctionInputExpression)
			{
				VisibleFunctionInputExpressions->AddUnique(FunctionInputExpression->Id);
			}
			else if (StaticSwitchExpression && StaticSwitchExpression->Material != NULL)
			{
				if(ExpressionID.IsValid())
				{
					VisibleExpressions.AddUnique(ExpressionID);
				}
			}
		}

		if(bValue)
		{
			GetVisibleMaterialParametersFromExpression(StaticSwitchExpression->A.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, FunctionInputs, VisibleFunctionInputExpressions);
		}
		else
		{
			GetVisibleMaterialParametersFromExpression(StaticSwitchExpression->B.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, FunctionInputs, VisibleFunctionInputExpressions);
		}
	}
	else if(FunctionCallExpression)
	{
		// Only process functions if any of the dependent functions contain static switches that could potentially cull the material parameters.
		if(IsFunctionContainingSwitchExpressions(FunctionCallExpression->MaterialFunction))
		{

			// Pass an accumalated array of FFunctionExpressionInput so that input expressions can be matched to the input from the parent.			
			TArray<FGuid> VisibleFunctionInputs;
			TArray<FFunctionExpressionInput> FunctionCallInputs = FunctionCallExpression->FunctionInputs;
			if(FunctionInputs)
			{
				FunctionCallInputs.Append(*FunctionInputs);
			}

			for(int32 FunctionOutputIndex = 0; FunctionOutputIndex < FunctionCallExpression->FunctionOutputs.Num(); FunctionOutputIndex++)
			{
				// Recurse material functions. Returns an array of input ids that are visible and have not been culled by static switches.
				GetVisibleMaterialParametersFromExpression( FunctionCallExpression->FunctionOutputs[FunctionOutputIndex].ExpressionOutput->A.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, &FunctionCallInputs, &VisibleFunctionInputs);
			}

			// Parse children of function call inputs that have not been culled 
			for(int32 ExpressionInputIndex = 0; ExpressionInputIndex < FunctionCallExpression->FunctionInputs.Num(); ExpressionInputIndex++)
			{
				const FFunctionExpressionInput& Input = FunctionCallExpression->FunctionInputs[ExpressionInputIndex];
				if(VisibleFunctionInputs.Contains(Input.ExpressionInputId)) 
				{
					// Retrieve the expression input and then start parsing its children
					GetVisibleMaterialParametersFromExpression(Input.Input.Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, FunctionInputs, VisibleFunctionInputExpressions );
				}
			}
		}
		else
		{
			const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
			for(int32 ExpressionInputIndex = 0; ExpressionInputIndex < ExpressionInputs.Num(); ExpressionInputIndex++)
			{
				//retrieve the expression input and then start parsing its children
				FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
				GetVisibleMaterialParametersFromExpression(Input->Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, FunctionInputs, VisibleFunctionInputExpressions);
			}
		}
	}
	else
	{
		const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
		for(int32 ExpressionInputIndex = 0; ExpressionInputIndex < ExpressionInputs.Num(); ExpressionInputIndex++)
		{
			//retrieve the expression input and then start parsing its children
			FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
			GetVisibleMaterialParametersFromExpression(Input->Expression, MaterialInstance, VisibleExpressions, ProcessedExpressions, FunctionInputs, VisibleFunctionInputExpressions);
		}
	}

	UMaterialExpression* TopExpression = ProcessedExpressions.Pop();
	//ensure that the top of the stack matches what we expect (the same as MaterialExpression)
	check(MaterialExpression == TopExpression);
}

TSharedPtr<class IMaterialEditor> FMaterialEditorUtilities::GetIMaterialEditorForObject(const UObject* ObjectToFocusOn)
{
	check(ObjectToFocusOn);

	// Find the associated Material
	UMaterial* Material = Cast<UMaterial>(ObjectToFocusOn->GetOuter());

	TSharedPtr<IMaterialEditor> MaterialEditor;
	if (Material != NULL)
	{
		TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(Material);
		if (FoundAssetEditor.IsValid())
		{
			MaterialEditor = StaticCastSharedPtr<IMaterialEditor>(FoundAssetEditor);
		}
	}
	return MaterialEditor;
}

void FMaterialEditorUtilities::AddMaterialExpressionCategory(FGraphActionMenuBuilder& ActionMenuBuilder, FString CategoryName, TArray<struct FMaterialExpression>* MaterialExpressions, bool bMaterialFunction)
{
	for (int32 Index = 0; Index < MaterialExpressions->Num(); ++Index)
	{
		const FMaterialExpression& MaterialExpression = (*MaterialExpressions)[Index];
		if (IsAllowedExpressionType(MaterialExpression.MaterialClass, bMaterialFunction))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), FText::FromString( *MaterialExpression.Name ));
			const FText ToolTip = FText::Format( LOCTEXT( "NewMaterialExpressionTooltip", "Adds a {Name} node here" ), Arguments );
			TSharedPtr<FMaterialGraphSchemaAction_NewNode> NewNodeAction(new FMaterialGraphSchemaAction_NewNode(
				CategoryName,
				MaterialExpression.Name,
				ToolTip.ToString(), 0));
			ActionMenuBuilder.AddAction(NewNodeAction);
			NewNodeAction->MaterialExpressionClass = MaterialExpression.MaterialClass;
			NewNodeAction->Keywords = CastChecked<UMaterialExpression>(MaterialExpression.MaterialClass->GetDefaultObject())->GetKeywords();
		}
	}
}

#undef LOCTEXT_NAMESPACE
