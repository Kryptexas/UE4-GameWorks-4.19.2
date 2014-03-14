// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialGraphNode.cpp
=============================================================================*/

#include "UnrealEd.h"
#include "MaterialEditorUtilities.h"
#include "MaterialEditorActions.h"
#include "GraphEditorActions.h"

#define LOCTEXT_NAMESPACE "MaterialGraphNode"

/////////////////////////////////////////////////////
// UMaterialGraphNode

UMaterialGraphNode::UMaterialGraphNode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bPreviewNeedsUpdate(false)
	, bIsErrorExpression(false)
	, bIsCurrentSearchResult(false)
	, bIsPreviewExpression(false)
{
}

void UMaterialGraphNode::PostCopyNode()
{
	// Make sure the MaterialExpression goes back to being owned by the Material after copying.
	ResetMaterialExpressionOwner();
}

FMaterialRenderProxy* UMaterialGraphNode::GetExpressionPreview()
{
	return FMaterialEditorUtilities::GetExpressionPreview(GetGraph(), MaterialExpression);
}

void UMaterialGraphNode::RecreateAndLinkNode()
{
	// Throw away the original pins
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		Pin->Modify();
		Pin->BreakAllPinLinks();

#if 0
		UEdGraphNode::ReturnPinToPool(Pin);
#else
		Pin->Rename(NULL, GetTransientPackage(), REN_None);
		Pin->RemoveFromRoot();
		Pin->MarkPendingKill();
#endif
	}
	Pins.Empty();

	AllocateDefaultPins();

	CastChecked<UMaterialGraph>(GetGraph())->LinkGraphNodesFromMaterial();
}

void UMaterialGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if (PropertyName == FName(TEXT("NodeComment")))
		{
			if (MaterialExpression)
			{
				MaterialExpression->Modify();
				MaterialExpression->Desc = NodeComment;
			}
		}
	}
}

void UMaterialGraphNode::PostEditImport()
{
	// Make sure this MaterialExpression is owned by the Material it's being pasted into.
	ResetMaterialExpressionOwner();
}

void UMaterialGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();
	}
}

bool UMaterialGraphNode::CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const
{
	if (Super::CanPasteHere(TargetGraph, Schema))
	{
		const UMaterialGraph* MaterialGraph = Cast<const UMaterialGraph>(TargetGraph);
		if (MaterialGraph)
		{
			// Check whether we're trying to paste a material function into a function that depends on it
			UMaterialExpressionMaterialFunctionCall* FunctionExpression = Cast<UMaterialExpressionMaterialFunctionCall>(MaterialExpression);
			bool bIsValidFunctionExpression = true;

			if (MaterialGraph->MaterialFunction 
				&& FunctionExpression 
				&& FunctionExpression->MaterialFunction
				&& FunctionExpression->MaterialFunction->IsDependent(MaterialGraph->MaterialFunction))
			{
				bIsValidFunctionExpression = false;
			}

			if (bIsValidFunctionExpression && IsAllowedExpressionType(MaterialExpression->GetClass(), MaterialGraph->MaterialFunction != NULL))
			{
				return true;
			}
		}
	}
	return false;
}

FString UMaterialGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	TArray<FString> Captions;
	MaterialExpression->GetCaption(Captions);

	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return GetParameterName();
	}
	else if (TitleType == ENodeTitleType::ListView)
	{
		return MaterialExpression->GetClass()->GetDescription();
	}
	else
	{
		// More useful to display multi line parameter captions in reverse order
		// TODO: May have to choose order based on expression type if others need correct order
		int32 CaptionIndex = Captions.Num() -1;

		FString NodeTitle = Captions[CaptionIndex];
		for (; CaptionIndex > 0; )
		{
			CaptionIndex--;
			NodeTitle += TEXT("\n");
			NodeTitle += Captions[CaptionIndex];
		}

		if ( MaterialExpression->bShaderInputData && (MaterialExpression->bHidePreviewWindow || MaterialExpression->bCollapsed))
		{
			NodeTitle += TEXT("\nInput Data");
		}

		if (bIsPreviewExpression)
		{
			NodeTitle += LOCTEXT("PreviewExpression", "\nPreviewing").ToString();
		}

		return NodeTitle;
	}
}

FLinearColor UMaterialGraphNode::GetNodeTitleColor() const
{
	UMaterial* Material = CastChecked<UMaterialGraph>(GetGraph())->Material;

	// Generate title color
	FColor TitleColor = MaterialExpression->BorderColor;
	TitleColor.A = 255;

	if (bIsErrorExpression)
	{
		// Outline expressions that caused errors in red
		TitleColor = FColor( 255, 0, 0 );
	}
	else if (bIsCurrentSearchResult)
	{
		TitleColor = FColor( 64, 64, 255 );
	}
	else if (bIsPreviewExpression)
	{
		// If we are currently previewing a node, its border should be the preview color.
		TitleColor = FColor( 70, 100, 200 );
	}
	else if (UMaterial::IsParameter(MaterialExpression))
	{
		if (Material->HasDuplicateParameters(MaterialExpression))
		{
			TitleColor = FColor( 0, 255, 255 );
		}
		else
		{
			TitleColor = FColor( 0, 128, 128 );
		}
	}
	else if (UMaterial::IsDynamicParameter(MaterialExpression))
	{
		if (Material->HasDuplicateDynamicParameters(MaterialExpression))
		{
			TitleColor = FColor( 0, 255, 255 );
		}
		else
		{
			TitleColor = FColor( 0, 128, 128 );
		}
	}

	return FLinearColor(TitleColor);
}

FString UMaterialGraphNode::GetTooltip() const
{
	if (MaterialExpression)
	{
		TArray<FString> ToolTips;
		MaterialExpression->GetExpressionToolTip(ToolTips);

		if (ToolTips.Num() > 0)
		{
			FString ToolTip = ToolTips[0];

			for (int32 Index = 1; Index < ToolTips.Num(); ++Index)
			{
				ToolTip += TEXT("\n");
				ToolTip += ToolTips[Index];
			}

			return ToolTip;
		}
	}
	return TEXT("");
}

void UMaterialGraphNode::PrepareForCopying()
{
	if (MaterialExpression)
	{
		// Temporarily take ownership of the MaterialExpression, so that it is not deleted when cutting
		MaterialExpression->Rename(NULL, this, REN_DontCreateRedirectors);
	}
}

void UMaterialGraphNode::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(GetGraph());

	if (Context.Node)
	{
		if (MaterialExpression)
		{
			if (MaterialExpression->IsA(UMaterialExpressionTextureBase::StaticClass()))
			{
				Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().UseCurrentTexture);

				// Add a 'Convert To Texture' option for convertible types
				if (MaterialGraph->MaterialFunction == NULL)
				{
					Context.MenuBuilder->BeginSection("MaterialEditorMenu0");
					{
						if ( MaterialExpression->IsA(UMaterialExpressionTextureSample::StaticClass()))
						{
							Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().ConvertToTextureObjects);
						}
						else if ( MaterialExpression->IsA(UMaterialExpressionTextureObject::StaticClass()))
						{
							Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().ConvertToTextureSamples);
						}
					}
					Context.MenuBuilder->EndSection();
				}
			}

			// Add a 'Convert To Parameter' option for convertible types
			if (MaterialExpression->IsA(UMaterialExpressionConstant::StaticClass())
				|| MaterialExpression->IsA(UMaterialExpressionConstant2Vector::StaticClass())
				|| MaterialExpression->IsA(UMaterialExpressionConstant3Vector::StaticClass())
				|| MaterialExpression->IsA(UMaterialExpressionConstant4Vector::StaticClass())
				|| MaterialExpression->IsA(UMaterialExpressionTextureSample::StaticClass())
				|| MaterialExpression->IsA(UMaterialExpressionComponentMask::StaticClass()))
			{
				if (MaterialGraph->MaterialFunction == NULL)
				{
					Context.MenuBuilder->BeginSection("MaterialEditorMenu1");
					{
						Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().ConvertObjects);
					}
					Context.MenuBuilder->EndSection();
				}
			}

			Context.MenuBuilder->BeginSection("MaterialEditorMenu2");
			{
				// Don't show preview option for bools
				if (!MaterialExpression->IsA(UMaterialExpressionStaticBool::StaticClass())
					&& !MaterialExpression->IsA(UMaterialExpressionStaticBoolParameter::StaticClass()))
				{
					// Add a preview node option if only one node is selected
					if (bIsPreviewExpression)
					{
						// If we are already previewing the selected node, the menu option should tell the user that this will stop previewing
						Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().StopPreviewNode);
					}
					else
					{
						// The menu option should tell the user this node will be previewed.
						Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().StartPreviewNode);
					}
				}

				if (MaterialExpression->bRealtimePreview)
				{
					Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().DisableRealtimePreviewNode);
				}
				else
				{
					Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().EnableRealtimePreviewNode);
				}
			}
			Context.MenuBuilder->EndSection();
		}

		// Break all links
		Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);

		// Separate the above frequently used options from the below less frequently used common options
		Context.MenuBuilder->BeginSection("MaterialEditorMenu3");
		{
			Context.MenuBuilder->AddMenuEntry( FGenericCommands::Get().Delete );
			Context.MenuBuilder->AddMenuEntry( FGenericCommands::Get().Cut );
			Context.MenuBuilder->AddMenuEntry( FGenericCommands::Get().Copy );
			Context.MenuBuilder->AddMenuEntry( FGenericCommands::Get().Duplicate );

			// Select upstream and downstream nodes
			Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().SelectDownstreamNodes);
			Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().SelectUpstreamNodes);
		}
		Context.MenuBuilder->EndSection();

		// Handle the favorites options
		if (MaterialExpression)
		{
			Context.MenuBuilder->BeginSection("MaterialEditorMenuFavorites");
			{
				if (FMaterialEditorUtilities::IsMaterialExpressionInFavorites(MaterialExpression))
				{
					Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().RemoveFromFavorites);
				}
				else
				{
					Context.MenuBuilder->AddMenuEntry(FMaterialEditorCommands::Get().AddToFavorites);
				}
			}
			Context.MenuBuilder->EndSection();
		}
	}
}

void UMaterialGraphNode::CreateInputPins()
{
	const TArray<FExpressionInput*> ExpressionInputs = MaterialExpression->GetInputs();

	for (int32 Index = 0; Index < ExpressionInputs.Num() ; ++Index)
	{
		FExpressionInput* Input = ExpressionInputs[Index];
		FString InputName = MaterialExpression->GetInputName(Index);

		// Shorten long expression input names.
		if ( !FCString::Stricmp( *InputName, TEXT("Coordinates") ) )
		{
			InputName = TEXT("UVs");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("TextureObject") ) )
		{
			InputName = TEXT("Tex");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("Input") ) )
		{
			InputName = TEXT("");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("Exponent") ) )
		{
			InputName = TEXT("Exp");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("AGreaterThanB") ) )
		{
			InputName = TEXT("A>=B");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("AEqualsB") ) )
		{
			InputName = TEXT("A==B");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("ALessThanB") ) )
		{
			InputName = TEXT("A<B");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("MipLevel") ) )
		{
			InputName = TEXT("Level");
		}
		else if ( !FCString::Stricmp( *InputName, TEXT("MipBias") ) )
		{
			InputName = TEXT("Bias");
		}

		const UMaterialGraphSchema* Schema = CastChecked<UMaterialGraphSchema>(GetSchema());
		FString PinCategory = MaterialExpression->IsInputConnectionRequired(Index) ? Schema->PC_Required : Schema->PC_Optional;
		FString PinSubCategory = TEXT("");

		UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinCategory, PinSubCategory, NULL, /*bIsArray=*/ false, /*bIsReference=*/ false, InputName);
		if (NewPin->PinName.IsEmpty())
		{
			// Makes sure pin has a name for lookup purposes but user will never see it
			NewPin->PinName = CreateUniquePinName(TEXT("Input"));
			NewPin->PinFriendlyName = TEXT(" ");
		}
	}
}

void UMaterialGraphNode::CreateOutputPins()
{
	TArray<FExpressionOutput>& Outputs = MaterialExpression->GetOutputs();

	for( int32 Index = 0 ; Index < Outputs.Num() ; ++Index )
	{
		const FExpressionOutput& ExpressionOutput = Outputs[Index];
		FString PinCategory = TEXT("");
		FString PinSubCategory = TEXT("");
		FString OutputName = TEXT("");

		const UMaterialGraphSchema* Schema = CastChecked<UMaterialGraphSchema>(GetSchema());
		
		if( ExpressionOutput.Mask )
		{
			PinCategory = Schema->PC_Mask;

			if ( ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
			{
				PinSubCategory = Schema->PSC_Red;
			}
			else if	(!ExpressionOutput.MaskR &&  ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
			{
				PinSubCategory = Schema->PSC_Green;
			}
			else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG &&  ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
			{
				PinSubCategory = Schema->PSC_Blue;
			}
			else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB &&  ExpressionOutput.MaskA)
			{
				PinSubCategory = Schema->PSC_Alpha;
			}
		}

		if (MaterialExpression->bShowOutputNameOnPin)
		{
			OutputName = ExpressionOutput.OutputName;
		}

		UEdGraphPin* NewPin = CreatePin(EGPD_Output, PinCategory, PinSubCategory, NULL, /*bIsArray=*/ false, /*bIsReference=*/ false, OutputName);
		if (NewPin->PinName.IsEmpty())
		{
			// Makes sure pin has a name for lookup purposes but user will never see it
			NewPin->PinName = CreateUniquePinName(TEXT("Output"));
			NewPin->PinFriendlyName = TEXT(" ");
		}
	}
}

int32 UMaterialGraphNode::GetOutputIndex(const UEdGraphPin* OutputPin)
{
	TArray<UEdGraphPin*> OutputPins;
	GetOutputPins(OutputPins);

	for (int32 Index = 0; Index < OutputPins.Num(); ++Index)
	{
		if (OutputPin == OutputPins[Index])
		{
			return Index;
		}
	}

	return -1;
}

uint32 UMaterialGraphNode::GetOutputType(const UEdGraphPin* OutputPin)
{
	return MaterialExpression->GetOutputType(GetOutputIndex(OutputPin));
}

int32 UMaterialGraphNode::GetInputIndex(const UEdGraphPin* InputPin) const
{
	TArray<UEdGraphPin*> InputPins;
	GetInputPins(InputPins);

	for (int32 Index = 0; Index < InputPins.Num(); ++Index)
	{
		if (InputPin == InputPins[Index])
		{
			return Index;
		}
	}

	return -1;
}

uint32 UMaterialGraphNode::GetInputType(const UEdGraphPin* InputPin) const
{
	return MaterialExpression->GetInputType(GetInputIndex(InputPin));
}

void UMaterialGraphNode::ResetMaterialExpressionOwner()
{
	if (MaterialExpression)
	{
		// Ensures MaterialExpression is owned by the Material or Function
		UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(GetGraph());
		UObject* ExpressionOuter = MaterialGraph->Material;
		if (MaterialGraph->MaterialFunction)
		{
			ExpressionOuter = MaterialGraph->MaterialFunction;
		}
		MaterialExpression->Rename(NULL, ExpressionOuter, REN_DontCreateRedirectors);

		// Set up the back pointer for newly created material nodes
		MaterialExpression->GraphNode = this;
	}
}

void UMaterialGraphNode::PostPlacedNewNode()
{
	if (MaterialExpression)
	{
		NodeComment = MaterialExpression->Desc;
		NodePosX = MaterialExpression->MaterialExpressionEditorX;
		NodePosY = MaterialExpression->MaterialExpressionEditorY;
		bCanRenameNode = UMaterial::IsParameter(MaterialExpression);
	}
}

void UMaterialGraphNode::OnRenameNode(const FString& NewName)
{
	MaterialExpression->Modify();
	SetParameterName(NewName);
	MaterialExpression->MarkPackageDirty();
	MaterialDirtyDelegate.ExecuteIfBound();
}

void UMaterialGraphNode::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	Super::GetPinHoverText(Pin, HoverTextOut);

	if (HoverTextOut.IsEmpty())
	{
		TArray<FString> ToolTips;

		int32 DirIndex = -1;
		int32 PinIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Pins.Num(); ++Index)
		{
			if (Pin.Direction == Pins[Index]->Direction)
			{
				++DirIndex;
				if (Pins[Index] == &Pin)
				{
					PinIndex = DirIndex;
					break;
				}
			}
		}

		if (Pin.Direction == EEdGraphPinDirection::EGPD_Input)
		{
			MaterialExpression->GetConnectorToolTip(PinIndex, INDEX_NONE, ToolTips);
		}
		else
		{
			MaterialExpression->GetConnectorToolTip(INDEX_NONE, PinIndex, ToolTips);
		}

		if (ToolTips.Num() > 0)
		{
			HoverTextOut = ToolTips[0];

			for (int32 Index = 1; Index < ToolTips.Num(); ++Index)
			{
				HoverTextOut += TEXT("\n");
				HoverTextOut += ToolTips[Index];
			}
		}
	}
}

FString UMaterialGraphNode::GetParameterName() const
{
	if (const UMaterialExpressionParameter* Parameter = Cast<const UMaterialExpressionParameter>(MaterialExpression))
	{
		return Parameter->ParameterName.ToString();
	}
	else if (const UMaterialExpressionTextureSampleParameter* TexParameter = Cast<const UMaterialExpressionTextureSampleParameter>(MaterialExpression))
	{
		return TexParameter->ParameterName.ToString();
	}
	else if (const UMaterialExpressionFontSampleParameter* FontParameter = Cast<const UMaterialExpressionFontSampleParameter>(MaterialExpression))
	{
		return FontParameter->ParameterName.ToString();
	}
	// Should have been able to get parameter name to edit
	check(false);
	return TEXT("");
}

void UMaterialGraphNode::SetParameterName(const FString& NewName)
{
	if (UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(MaterialExpression))
	{
		Parameter->ParameterName = *NewName;
	}
	else if (UMaterialExpressionTextureSampleParameter* TexParameter = Cast<UMaterialExpressionTextureSampleParameter>(MaterialExpression))
	{
		TexParameter->ParameterName = *NewName;
	}
	else if (UMaterialExpressionFontSampleParameter* FontParameter = Cast<UMaterialExpressionFontSampleParameter>(MaterialExpression))
	{
		FontParameter->ParameterName = *NewName;
	}

	CastChecked<UMaterialGraph>(GetGraph())->Material->UpdateExpressionParameterName(MaterialExpression);
}

#undef LOCTEXT_NAMESPACE
