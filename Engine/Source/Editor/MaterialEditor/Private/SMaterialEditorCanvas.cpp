// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MaterialEditorModule.h"
#include "MaterialEditor.h"
#include "SMaterialEditorCanvas.h"
#include "SMaterialEditorViewport.h"
#include "MaterialEditorActions.h"
#include "MaterialEditorUtilities.h"
#include "MaterialExpressionClasses.h"
#include "LinkedObjDrawUtils.h"
#include "BusyCursor.h"
#include "ScopedTransaction.h"
#include "ShaderCompiler.h"
#include "PreviewScene.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "AssetSelection.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#define LOCTEXT_NAMESPACE "MaterialEditor"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static helpers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

/** Location of the realtime preview icon, relative to the upper-hand corner of the material expression node. */
static const FIntPoint PreviewIconLoc( 1, 6 );

/** Width of the realtime preview icon. */
static const int32 PreviewIconWidth = 12;

static const FColor ConnectionNormalColor(0,0,0);
static const FColor ConnectionOptionalColor(100,100,100);
static const FColor ConnectionSelectedColor(255,255,0);
static const FColor ConnectionMarkedColor(40,150,40);

} // namespace
	
namespace {

/**
 * Hitproxy for the realtime preview icon on material expression nodes.
 */
struct HCollapsedProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	UMaterialExpression*	MaterialExpression;

	HCollapsedProxy(UMaterialExpression* InMaterialExpression)
		:	HHitProxy( HPP_UI )
		,	MaterialExpression( InMaterialExpression )
	{}
	virtual void Serialize(FArchive& Ar)
	{
		Ar << MaterialExpression;
	}
};
IMPLEMENT_HIT_PROXY( HCollapsedProxy, HHitProxy );

/**
 * Returns a reference to the material to be used as the material expression copy-paste buffer.
 */
static UMaterial* GetCopyPasteBuffer()
{
	if ( !GUnrealEd->MaterialCopyPasteBuffer )
	{
		GUnrealEd->MaterialCopyPasteBuffer = ConstructObject<UMaterial>( UMaterial::StaticClass() );
	}
	return GUnrealEd->MaterialCopyPasteBuffer;
}

/** 
 *	The corresponding material input index for emissive color when connections are hidden. 
 *	
 *  NOTE: Please ensure that these values are correct when updating the material input indexes below. Incorrect values
 *	will cause the material expression preview to function incorrectly.
 */

// Used when DiffuseColor, SpecularColor material inputs are active ('r.UseDiffuseSpecularMaterialInputs' is true).
#define MATERIAL_INDEX_EMISSIVECOLOR  3
// Used when physically based inputs are active. ('r.UseDiffuseSpecularMaterialInputs' is false). 
#define MATERIAL_PHYSICALLYBASED_INDEX_EMISSIVECOLOR 4

/**
 * Finds the material expression that maps to the given index when connections are hidden. 
 *
 * When connections are hidden, the given input index does not correspond to the
 * visible input node presented to the user. This function translates it to the correct input.
 */
static FExpressionInput* GetMaterialInputConditional(UMaterial* Material, int32 Index, bool bAreUnusedConnectorsHidden)
{
	FExpressionInput* MaterialInput = NULL;
	int32 VisibleNodesToFind = Index;
	int32 InputIndex = 0;

	// When VisibleNodesToFind is zero, then we found the corresponding input.
	while (VisibleNodesToFind >= 0)
	{
		MaterialInput = FMaterialEditorUtilities::GetMaterialInput( Material, InputIndex );

		if( FMaterialEditorUtilities::IsInputVisible( Material, InputIndex ) &&
			(!bAreUnusedConnectorsHidden || MaterialInput->IsConnected()) )
		{
			VisibleNodesToFind--;
		}

		InputIndex++;
	}

	// If VisibleNodesToFind is less than zero, then the loop to find the matching material input 
	// terminated prematurely. The material input could be pointing to the incorrect input.
	check(VisibleNodesToFind < 0);

	return MaterialInput;
}

/** 
 * Helper struct that contains a reference to an expression and a subset of its inputs.
 */
struct FMaterialExpressionReference
{
public:
	FMaterialExpressionReference(UMaterialExpression* InExpression) :
		Expression(InExpression)
	{}

	FMaterialExpressionReference(FExpressionInput* InInput) :
		Expression(NULL)
	{
		Inputs.Add(InInput);
	}

	UMaterialExpression* Expression;
	TArray<FExpressionInput*> Inputs;
};

/**
 * Assembles a list of UMaterialExpressions and their FExpressionInput objects that refer to the specified FExpressionOutput.
 */
static void GetListOfReferencingInputs(
	const UMaterialExpression* InMaterialExpression, 
	UMaterial* Material, 
	TArray<FMaterialExpressionReference>& OutReferencingInputs, 
	const FExpressionOutput* MaterialExpressionOutput, 
	int32 OutputIndex)
{
	OutReferencingInputs.Empty();
	const bool bOutputIndexIsValid = OutputIndex != INDEX_NONE;

	// Gather references from other expressions
	for ( int32 ExpressionIndex = 0 ; ExpressionIndex < Material->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = Material->Expressions[ ExpressionIndex ];

		const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
		FMaterialExpressionReference NewReference(MaterialExpression);
		for ( int32 ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
		{
			FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];

			if ( Input->Expression == InMaterialExpression &&
				(!MaterialExpressionOutput ||
				bOutputIndexIsValid && Input->OutputIndex == OutputIndex ||
				!bOutputIndexIsValid &&
				Input->Mask == MaterialExpressionOutput->Mask &&
				Input->MaskR == MaterialExpressionOutput->MaskR &&
				Input->MaskG == MaterialExpressionOutput->MaskG &&
				Input->MaskB == MaterialExpressionOutput->MaskB &&
				Input->MaskA == MaterialExpressionOutput->MaskA) )
			{
				NewReference.Inputs.Add(Input);
			}
		}

		if (NewReference.Inputs.Num() > 0)
		{
			OutReferencingInputs.Add(NewReference);
		}
	}

	// Gather references from material inputs
#define __GATHER_REFERENCE_TO_EXPRESSION( Mat, Prop ) \
	if ( Mat->Prop.Expression == InMaterialExpression && \
		(!MaterialExpressionOutput || \
		bOutputIndexIsValid && Mat->Prop.OutputIndex == OutputIndex || \
		!bOutputIndexIsValid && \
		Mat->Prop.Mask == MaterialExpressionOutput->Mask && \
		Mat->Prop.MaskR == MaterialExpressionOutput->MaskR && \
		Mat->Prop.MaskG == MaterialExpressionOutput->MaskG && \
		Mat->Prop.MaskB == MaterialExpressionOutput->MaskB && \
		Mat->Prop.MaskA == MaterialExpressionOutput->MaskA )) \
	{ OutReferencingInputs.Add(FMaterialExpressionReference(&(Mat->Prop))); }

	__GATHER_REFERENCE_TO_EXPRESSION( Material, DiffuseColor );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, SpecularColor );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, BaseColor );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Metallic );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Specular );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Roughness );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, EmissiveColor );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Opacity );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, OpacityMask );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Normal );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, WorldPositionOffset );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, WorldDisplacement );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, TessellationMultiplier );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, SubsurfaceColor );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, AmbientOcclusion );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, Refraction );
	__GATHER_REFERENCE_TO_EXPRESSION( Material, MaterialAttributes );

	for (int32 UVIndex = 0; UVIndex < ARRAY_COUNT(Material->CustomizedUVs); UVIndex++)
	{
		__GATHER_REFERENCE_TO_EXPRESSION( Material, CustomizedUVs[UVIndex] );
	}
#undef __GATHER_REFERENCE_TO_EXPRESSION
}

/**
 * Assembles a list of FExpressionInput objects that refer to the specified FExpressionOutput.
 */
static void GetListOfReferencingInputs(
	const UMaterialExpression* InMaterialExpression, 
	UMaterial* Material, 
	TArray<FExpressionInput*>& OutReferencingInputs, 
	const FExpressionOutput* MaterialExpressionOutput = NULL,
	int32 OutputIndex = INDEX_NONE)
{
	TArray<FMaterialExpressionReference> References;
	GetListOfReferencingInputs(InMaterialExpression, Material, References, MaterialExpressionOutput, OutputIndex);
	OutReferencingInputs.Empty();
	for (int32 ReferenceIndex = 0; ReferenceIndex < References.Num(); ReferenceIndex++)
	{
		for (int32 InputIndex = 0; InputIndex < References[ReferenceIndex].Inputs.Num(); InputIndex++)
		{
			OutReferencingInputs.Add(References[ReferenceIndex].Inputs[InputIndex]);
		}
	}
}

/**
 * Swaps all links to OldExpression with NewExpression.  NewExpression may be NULL.
 */
static void SwapLinksToExpression(UMaterialExpression* OldExpression, UMaterialExpression* NewExpression, UMaterial* Material)
{
	check(OldExpression);
	check(Material);

	Material->Modify();

	{
		// Move any of OldExpression's inputs over to NewExpression
		const TArray<FExpressionInput*>& OldExpressionInputs = OldExpression->GetInputs();

		TArray<FExpressionInput*> NewExpressionInputs;
		if (NewExpression)
		{
			NewExpression->Modify();
			NewExpressionInputs = NewExpression->GetInputs();
		}

		// Copy the inputs over, matching them up based on the order they are declared in each class
		for (int32 InputIndex = 0; InputIndex < OldExpressionInputs.Num() && InputIndex < NewExpressionInputs.Num(); InputIndex++)
		{
			*NewExpressionInputs[InputIndex] = *OldExpressionInputs[InputIndex];
		}
	}	
	
	// Move any links from other expressions to OldExpression over to NewExpression
	TArray<FExpressionOutput>& Outputs = OldExpression->GetOutputs();
	TArray<FExpressionOutput> NewOutputs;
	if (NewExpression)
	{
		NewOutputs = NewExpression->GetOutputs();
	}
	else
	{
		NewOutputs.Add(FExpressionOutput(false));
	}

	for (int32 OutputIndex = 0; OutputIndex < Outputs.Num(); OutputIndex++)
	{
		const FExpressionOutput& CurrentOutput = Outputs[OutputIndex];
		
		FExpressionOutput NewOutput(false);
		bool bFoundMatchingOutput = false;

		// Try to find an equivalent output in NewExpression
		for (int32 NewOutputIndex = 0; NewOutputIndex < NewOutputs.Num(); NewOutputIndex++)
		{
			const FExpressionOutput& CurrentNewOutput = NewOutputs[NewOutputIndex];
			if(	CurrentOutput.Mask == CurrentNewOutput.Mask
				&& CurrentOutput.MaskR == CurrentNewOutput.MaskR
				&& CurrentOutput.MaskG == CurrentNewOutput.MaskG
				&& CurrentOutput.MaskB == CurrentNewOutput.MaskB
				&& CurrentOutput.MaskA == CurrentNewOutput.MaskA )
			{
				NewOutput = CurrentNewOutput;
				bFoundMatchingOutput = true;
			}
		}
		// Couldn't find an equivalent output in NewExpression, just pick the first
		// The user will have to fix up any issues from the mismatch
		if (!bFoundMatchingOutput && NewOutputs.Num() > 0)
		{
			NewOutput = NewOutputs[0];
		}

		TArray<FMaterialExpressionReference> ReferencingInputs;
		GetListOfReferencingInputs(OldExpression, Material, ReferencingInputs, &CurrentOutput, OutputIndex);
		for (int32 ReferenceIndex = 0; ReferenceIndex < ReferencingInputs.Num(); ReferenceIndex++)
		{
			FMaterialExpressionReference& CurrentReference = ReferencingInputs[ReferenceIndex];
			if (CurrentReference.Expression)
			{
				CurrentReference.Expression->Modify();
			}
			// Move the link to OldExpression over to NewExpression
			for (int32 InputIndex = 0; InputIndex < CurrentReference.Inputs.Num(); InputIndex++)
			{
				SMaterialEditorCanvas::ConnectExpressions(*CurrentReference.Inputs[InputIndex], NewOutput, OutputIndex, NewExpression);
			}
		}
	}
}

} // namespace


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SMaterialEditorCanvas
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int32 SMaterialEditorCanvas::NumFunctionOutputsSupported = 100;

void SMaterialEditorCanvas::Construct(const FArguments& InArgs)
{
	MaterialEditorPtr = InArgs._MaterialEditor;
	
	PreviewExpression = NULL;
	bHomeNodeSelected = false;
	ConnObj = NULL;
	ConnType = LOC_INPUT;
	ConnIndex = 0;
	MarkedObject = NULL;
	MarkedConnType = LOC_INPUT;
	MarkedConnIndex = 0;
	bAlwaysRefreshAllPreviews = false;
	bHideUnusedConnectors = false;
	bShowStats = true;
	bShowMobileStats = false;
	SelectedSearchResult = 0;
	DblClickObject = NULL;
	DblClickConnType = INDEX_NONE;
	DblClickConnIndex = INDEX_NONE;
	ScopedTransaction = NULL;
	bUseUnsortedMenus = false;

	ColorPickerObject = NULL;

	BindCommands();

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(ViewportWidget, SViewport)
			.EnableGammaCorrection(false)
			.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
			.ShowEffectWhenDisabled(false)
		]
	];



	// Create linked-object tree window.
	LinkedObjVC = MakeShareable( new FLinkedObjViewportClient(this) );

	LinkedObjVC->SetRealtime( false );
	LinkedObjVC->VisibilityDelegate.BindSP( this, &SMaterialEditorCanvas::IsVisible );

	Viewport = MakeShareable( new FSceneViewport( LinkedObjVC.Get(), ViewportWidget ) );
	LinkedObjVC->Viewport = Viewport.Get();

	// The viewport widget needs an interface so it knows what should render
	ViewportWidget->SetViewportInterface( Viewport.ToSharedRef() );


	LinkedObjVC->ToolTipDelayMS = 333;
	LinkedObjVC->SetRedrawInTick( false );


	
	MaterialEditorPtr.Pin()->SetPreviewMaterial( GetMaterial() );

	// Mark material as being the preview material to avoid unnecessary work in UMaterial::PostEditChange. This property is duplicatetransient so we don't have
	// to worry about resetting it when propagating the preview to the original material after editing.
	// Note:  The material editor must reattach the preview meshes through RefreshPreviewViewport() after calling Material::PEC()!
	GetMaterial()->bIsPreviewMaterial = true;

	// Make sure the material is the list of material expressions it references.
	FMaterialEditorUtilities::InitExpressions( GetMaterial() );

	NodeCollapsedTexture = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorMaterials/TreeArrow_Collapsed.TreeArrow_Collapsed"), NULL, LOAD_None, NULL);
	NodeExpandedTexture = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorMaterials/TreeArrow_Expanded.TreeArrow_Expanded"), NULL, LOAD_None, NULL);
	NodeExpandedTextureRealtimePreview = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorMaterials/TreeArrow_ExpandedRealtimePreview.TreeArrow_ExpandedRealtimePreview"), NULL, LOAD_None, NULL);

	// Initialize the material input list.
	MaterialInputs.Add( FMaterialInputInfo( TEXT("DiffuseColor"), &GetMaterial()->DiffuseColor, MP_DiffuseColor ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("SpecularColor"), &GetMaterial()->SpecularColor, MP_SpecularColor ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("BaseColor"), &GetMaterial()->BaseColor, MP_BaseColor ) );	
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Metallic"), &GetMaterial()->Metallic, MP_Metallic ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Specular"), &GetMaterial()->Specular, MP_Specular ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Roughness"), &GetMaterial()->Roughness, MP_Roughness ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("EmissiveColor"), &GetMaterial()->EmissiveColor, MP_EmissiveColor ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Opacity"), &GetMaterial()->Opacity, MP_Opacity ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("OpacityMask"), &GetMaterial()->OpacityMask, MP_OpacityMask ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Normal"), &GetMaterial()->Normal, MP_Normal ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("WorldPositionOffset"), &GetMaterial()->WorldPositionOffset, MP_WorldPositionOffset ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("WorldDisplacement"), &GetMaterial()->WorldDisplacement, MP_WorldDisplacement ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("TessellationMultiplier"), &GetMaterial()->TessellationMultiplier, MP_TessellationMultiplier ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("SubsurfaceColor"), &GetMaterial()->SubsurfaceColor, MP_SubsurfaceColor ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("AmbientOcclusion"), &GetMaterial()->AmbientOcclusion, MP_AmbientOcclusion ) );
	MaterialInputs.Add( FMaterialInputInfo( TEXT("Refraction"), &GetMaterial()->Refraction, MP_Refraction) );

	for (int32 UVIndex = 0; UVIndex < ARRAY_COUNT(GetMaterial()->CustomizedUVs); UVIndex++)
	{
		MaterialInputs.Add( FMaterialInputInfo( *FString::Printf(TEXT("CustomizedUV%u"), UVIndex), &GetMaterial()->CustomizedUVs[UVIndex], (EMaterialProperty)(MP_CustomizedUVs0 + UVIndex)) );
	}

	MaterialInputs.Add( FMaterialInputInfo( TEXT("MaterialAttributes"), &GetMaterial()->MaterialAttributes, MP_MaterialAttributes) );
}

UMaterial* SMaterialEditorCanvas::GetMaterial()
{
	return MaterialEditorPtr.IsValid() ? MaterialEditorPtr.Pin()->Material : NULL;
}

UMaterialFunction* SMaterialEditorCanvas::GetMaterialFunction()
{
	return MaterialEditorPtr.IsValid() ? MaterialEditorPtr.Pin()->MaterialFunction : NULL;
}

FReply SMaterialEditorCanvas::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FExpressionDragDropOp>(DragDropEvent.GetOperation()) )
	{
		return FReply::Handled();
	}
	else
	{
		TArray< FAssetData > DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );

		bool bCanDrop = DroppedAssetData.Num() > 0;

		for (int32 DroppedAssetIdx = 0; DroppedAssetIdx < DroppedAssetData.Num(); ++DroppedAssetIdx)
		{
			const FAssetData& AssetData = DroppedAssetData[DroppedAssetIdx];
			
			UClass* AssetClass = AssetData.GetClass();
			check( AssetClass != NULL );

			UObject* ClassDefaultObject = AssetClass->GetDefaultObject();
			if ( !( ClassDefaultObject->IsA( UMaterialFunction::StaticClass() ) || ClassDefaultObject->IsA( UTexture2D::StaticClass() ) ) )
			{
				bCanDrop = false;
			}
		}

		if (bCanDrop)
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SMaterialEditorCanvas::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	FMaterialExpression* MaterialExpression = NULL;
	TArray<UObject*> DroppedObjects;

	if ( DragDrop::IsTypeMatch<FExpressionDragDropOp>(DragDropEvent.GetOperation()) )
	{
		auto Operation = StaticCastSharedPtr<FExpressionDragDropOp>(DragDropEvent.GetOperation());

		MaterialExpression = Operation->MaterialExpression;

		FFunctionEntryInfo* FunctionEntryInfo = Operation->FunctionEntryInfo;
		if (FunctionEntryInfo)
		{
			DroppedObjects.Add(LoadObject<UMaterialFunction>(NULL, *FunctionEntryInfo->Path, NULL, 0, NULL));
		}
	}
	else
	{
		TArray< FAssetData > DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );
		bool bCanDrop = DroppedAssetData.Num() > 0;

		if ( !bCanDrop )
		{
			return FReply::Unhandled();
		}

		for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
		{
			UObject* Object = DroppedAssetData[ AssetIdx ].GetAsset();

			if ( Object != NULL )
			{
				DroppedObjects.Add( Object );
			}
		}
	}
	
	FVector2D DropLoc = MyGeometry.AbsoluteToLocal( DragDropEvent.GetScreenSpacePosition() );

	const int32 LocX = (DropLoc.X - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
	const int32 LocY = (DropLoc.Y - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
	FIntPoint Loc = FIntPoint(LocX, LocY);
	const int32 LocOffsetBetweenNodes = 32;

	if (MaterialExpression)
	{
		UClass* NewExpressionClass = MaterialExpression->MaterialClass;
		CreateNewMaterialExpression( NewExpressionClass, false, true, true, Loc );
		Loc.X += LocOffsetBetweenNodes;
		Loc.Y += LocOffsetBetweenNodes;
	}

	// Find all dropped Material Functions and Textures
	for ( auto ObjIt = DroppedObjects.CreateConstIterator(); ObjIt; ++ObjIt )
	{
		UMaterialFunction* Func = Cast<UMaterialFunction>(*ObjIt);
		UTexture* Tex = Cast<UTexture>(*ObjIt);
		UMaterialParameterCollection* ParameterCollection = Cast<UMaterialParameterCollection>(*ObjIt);

		bool bAddedNode = false;
		if ( Func )
		{
			UMaterialExpressionMaterialFunctionCall* FunctionNode = CastChecked<UMaterialExpressionMaterialFunctionCall>(
				CreateNewMaterialExpression(UMaterialExpressionMaterialFunctionCall::StaticClass(), false, true, false, Loc));

			if (!FunctionNode->MaterialFunction)
			{
				if(!FunctionNode->SetMaterialFunction(GetMaterialFunction(), NULL, Func))
				{
					SelectedExpressions.Add(FunctionNode);
					DeleteSelectedObjects();

					continue;
				}
			}

			bAddedNode = true;
		}
		else if ( Tex )
		{
			UMaterialExpressionTextureSample* TextureSampleNode = CastChecked<UMaterialExpressionTextureSample>(
				CreateNewMaterialExpression( UMaterialExpressionTextureSample::StaticClass(), false, true, true, Loc ) );
			TextureSampleNode->Texture = Tex;
			TextureSampleNode->AutoSetSampleType();

			ForceRefreshExpressionPreviews();

			bAddedNode = true;
		}
		else if ( ParameterCollection )
		{
			UMaterialExpressionCollectionParameter* CollectionParameterNode = CastChecked<UMaterialExpressionCollectionParameter>(
				CreateNewMaterialExpression( UMaterialExpressionCollectionParameter::StaticClass(), false, true, true, Loc ) );
			CollectionParameterNode->Collection = ParameterCollection;

			ForceRefreshExpressionPreviews();

			bAddedNode = true;
		}

		if ( bAddedNode )
		{
			Loc.X += LocOffsetBetweenNodes;
			Loc.Y += LocOffsetBetweenNodes;
		}
	}

	return FReply::Unhandled();
}

void SMaterialEditorCanvas::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	// Call parent implementation
	SWidget::OnMouseLeave( MouseEvent );

	LinkedObjVC->MouseOverObject = NULL;
	Viewport->InvalidateDisplay();
}
	
SMaterialEditorCanvas::SMaterialEditorCanvas()
{
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->RegisterForUndo( this );
	}
}

SMaterialEditorCanvas::~SMaterialEditorCanvas()
{
	PreviewExpression = NULL;

	{
		SCOPED_SUSPEND_RENDERING_THREAD(true);

		ExpressionPreviews.Empty();
	}
	
	check( !ScopedTransaction );

	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->UnregisterForUndo( this) ;
	}

	if (LinkedObjVC.IsValid())
	{
		LinkedObjVC->Viewport = NULL;
	}

	Viewport.Reset();

	// Release our reference to the viewport client
	LinkedObjVC.Reset();
}

TSharedRef<SWidget> SMaterialEditorCanvas::BuildMenuWidgetEmpty()
{
	// Get all menu extenders for this context menu from the material editor module
	IMaterialEditorModule& MaterialEditor = FModuleManager::GetModuleChecked<IMaterialEditorModule>( TEXT("MaterialEditor") );
	TArray<IMaterialEditorModule::FMaterialMenuExtender> MenuExtenderDelegates = MaterialEditor.GetAllMaterialCanvasMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(MaterialEditorPtr.Pin()->GetToolkitCommands()));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bShouldCloseWindowAfterMenuSelection = true;	// Set the menu to automatically close when the user commits to a choice
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, MaterialEditorPtr.Pin()->GetToolkitCommands(), MenuExtender);
	{
		NewShaderExpressionsMenu(MenuBuilder);

		MenuBuilder.BeginSection("MaterialEditorCanvasEmpty2");
		{
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().NewComment);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("MaterialEditorCanvasEmpty3");
		{
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().PasteHere);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SMaterialEditorCanvas::BuildMenuWidgetObjects()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;	// Set the menu to automatically close when the user commits to a choice
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, MaterialEditorPtr.Pin()->GetToolkitCommands());
	{
		// Are any expressions selected?
		if( SelectedExpressions.Num() > 0 )
		{
			bool bFoundTexture = false;
			// Present "Use Current Texture" if a texture is selected in the generic browser and at
			// least on Texture Sample node is selected in the material editor.
			for ( int32 ExpressionIndex = 0 ; ExpressionIndex < SelectedExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* Expression = SelectedExpressions[ExpressionIndex];
				if ( Expression->IsA(UMaterialExpressionTextureBase::StaticClass()))
				{
					MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().UseCurrentTexture);
					
					bFoundTexture = true;

					// Add a 'Convert To Texture' option for convertible types
					if (GetMaterialFunction() == NULL)
					{
						MenuBuilder.BeginSection("MaterialEditorMenu0");
						{
							if ( Expression->IsA(UMaterialExpressionTextureSample::StaticClass()))
							{
								MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().ConvertToTextureObjects);
							}
							else if ( Expression->IsA(UMaterialExpressionTextureObject::StaticClass()))
							{
								MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().ConvertToTextureSamples);
							}
						}
						MenuBuilder.EndSection();
					}
					break;
				}
			}

			// Add a 'Convert To Parameter' option for convertible types
			bool bFoundConstant = false;
			for (int32 ExpressionIndex = 0; ExpressionIndex < SelectedExpressions.Num(); ++ExpressionIndex)
			{
				UMaterialExpression* Expression = SelectedExpressions[ExpressionIndex];
				if (Expression->IsA(UMaterialExpressionConstant::StaticClass())
					|| Expression->IsA(UMaterialExpressionConstant2Vector::StaticClass())
					|| Expression->IsA(UMaterialExpressionConstant3Vector::StaticClass())
					|| Expression->IsA(UMaterialExpressionConstant4Vector::StaticClass())
					|| Expression->IsA(UMaterialExpressionTextureSample::StaticClass())
					|| Expression->IsA(UMaterialExpressionComponentMask::StaticClass()))
				{
					bFoundConstant = true;

					if (GetMaterialFunction() == NULL)
					{
						MenuBuilder.BeginSection("MaterialEditorMenu1");
						{
							MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().ConvertObjects);
						}
						MenuBuilder.EndSection();
					}
					break;
				}
			}
		}

		if( SelectedExpressions.Num() == 1 )
		{
			MenuBuilder.BeginSection("MaterialEditorMenu2");
			{
				// Don't show preview option for bools
				if (!SelectedExpressions[0]->IsA(UMaterialExpressionStaticBool::StaticClass())
					&& !SelectedExpressions[0]->IsA(UMaterialExpressionStaticBoolParameter::StaticClass()))
				{
					// Add a preview node option if only one node is selected
					if( PreviewExpression == SelectedExpressions[0] )
					{
						// If we are already previewing the selected node, the menu option should tell the user that this will stop previewing
						MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().StopPreviewNode);
					}
					else
					{
						// The menu option should tell the user this node will be previewed.
						MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().StartPreviewNode);
					}
				}

				if (SelectedExpressions[0]->bRealtimePreview)
				{
					MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().DisableRealtimePreviewNode);
				}
				else
				{
					MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().EnableRealtimePreviewNode);
				}
			}
			MenuBuilder.EndSection();
		}
		
		// Break all links
		MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().BreakAllLinks);
		
		// Are any expressions selected?
		if( SelectedExpressions.Num() > 0 )
		{
			// Separate the above frequently used options from the below less frequently used common options
			MenuBuilder.BeginSection("MaterialEditorMenu3");
			{
			// Duplicate selected.
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().DuplicateObjects);

			// Delete selected.
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().DeleteObjects);

			// Select upstream and downstream nodes
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().SelectDownstreamNodes);
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().SelectUpstreamNodes);
		}
			MenuBuilder.EndSection();
		}

		// Handle the favorites options
		if( SelectedExpressions.Num() == 1 )
		{
			MenuBuilder.BeginSection("MaterialEditorMenuFavorites");
			{
			UMaterialExpression* Expression = SelectedExpressions[0];
			if (MaterialExpressionClasses::Get()->IsMaterialExpressionInFavorites(Expression))
			{
				MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().RemoveFromFavorites);
			}
			else
			{
				MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().AddToFavorites);
			}
		}
			MenuBuilder.EndSection();
		}
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SMaterialEditorCanvas::BuildMenuWidgetConnector()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;	// Set the menu to automatically close when the user commits to a choice
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, MaterialEditorPtr.Pin()->GetToolkitCommands());
	{
		UMaterialExpression* MaterialExpression = ConnObj.IsValid() ? Cast<UMaterialExpression>( ConnObj.Get() ) : NULL;
		UMaterial* MaterialNode = ConnObj.IsValid() ? Cast<UMaterial>( ConnObj.Get() ) : NULL;

		bool bHasExpressionConnection = false;
		bool bHasMaterialConnection = false;

		if( ConnType == LOC_OUTPUT ) // Items on left side of node.
		{
			if ( MaterialExpression )
			{
				bHasExpressionConnection = true;
			}
			if ( MaterialNode )
			{
				bHasMaterialConnection = true;
			}
		}
		else if ( ConnType == LOC_INPUT ) // Items on right side of node.
		{
			// Only expressions can have connectors on the left side.
			check( MaterialExpression );
			bHasExpressionConnection = true;
		}

		// Break link.
		if( bHasExpressionConnection || bHasMaterialConnection )
		{
			MenuBuilder.BeginSection("MaterialEditorMenuConnector");
			{
			MenuBuilder.AddMenuEntry(FMaterialEditorCommands::Get().BreakLink);
			}
			MenuBuilder.EndSection();

			// add menu items to expression output for material connection
			if ( ConnType == LOC_INPUT ) // left side
			{
				MenuBuilder.BeginSection("MaterialEditorMenuConnector2");
				{
				// If we are editing a material function, display options to connect to function outputs
				if (GetMaterialFunction())
				{
					int32 FunctionOutputIndex = 0;
					for (int32 ExpressionIndex = 0; ExpressionIndex < GetMaterial()->Expressions.Num() && FunctionOutputIndex < NumFunctionOutputsSupported; ExpressionIndex++)
					{
						UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(GetMaterial()->Expressions[ExpressionIndex]);
						if (FunctionOutput)
						{
							FFormatNamedArguments Arguments;
							Arguments.Add(TEXT("Name"), FText::FromString( *FunctionOutput->OutputName ));
							const FText Label = FText::Format( LOCTEXT( "ConnectToFunction", "Connect To {Name}" ), Arguments );
							const FText ToolTip = FText::Format( LOCTEXT( "ConnectToFunctionTooltip", "Connects to the function output {Name}" ), Arguments );
							MenuBuilder.AddMenuEntry(Label,
								ToolTip,
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnConnectToFunctionOutput, ExpressionIndex)));
							FunctionOutputIndex++;
						}
					}
				}
				else
				{
					int32 ConnectionIndex = 0;
					for (int32 i = 0; i < MaterialInputs.Num(); ++i)
					{
						if( FMaterialEditorUtilities::IsInputVisible( GetMaterial(), i ) )
						{
							FFormatNamedArguments Arguments;
							Arguments.Add(TEXT("Name"), FText::FromString( MaterialInputs[i].Name ));
							const FText Label = FText::Format( LOCTEXT( "ConnectToInput", "Connect To {Name}" ), Arguments );
							const FText ToolTip = FText::Format( LOCTEXT( "ConnectToInputTooltip", "Connects to the material input {Name}" ), Arguments );
							MenuBuilder.AddMenuEntry(Label,
								ToolTip,
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnConnectToMaterial, ConnectionIndex++)));
						}
					}
				}
			}
				MenuBuilder.EndSection(); //MaterialEditorMenuConnector2
			}
		}

		// If an input connect was clicked on, append options to create new shader expressions.
		if(ConnObj.IsValid())
		{
			if ( ConnType == LOC_OUTPUT )
			{
				MenuBuilder.BeginSection("MaterialEditorMenuExpressions");
				{
					MenuBuilder.AddSubMenu(LOCTEXT("AddMaterialExpression", "Add New Material Expression"),
						LOCTEXT("AddMaterialExpressionTooltip", "Adds a material expression here"),
						FNewMenuDelegate::CreateSP(this, &SMaterialEditorCanvas::NewShaderExpressionsMenu));
				}
				MenuBuilder.EndSection();
			}
		}
	}

	return MenuBuilder.MakeWidget();
}

void SMaterialEditorCanvas::SetMaterialDirty()
{
	MaterialEditorPtr.Pin()->bMaterialDirty = true;
}

/**
 * Connects the specified input expression to the specified output expression.
 */
void SMaterialEditorCanvas::ConnectExpressions(FExpressionInput& Input, const FExpressionOutput& Output, int32 OutputIndex, UMaterialExpression* Expression)
{
	Input.Expression = Expression;
	Input.OutputIndex = OutputIndex;
	Input.Mask = Output.Mask;
	Input.MaskR = Output.MaskR;
	Input.MaskG = Output.MaskG;
	Input.MaskB = Output.MaskB;
	Input.MaskA = Output.MaskA;
}

/**
 * Connects the MaterialInputIndex'th material input to the MaterialExpressionOutputIndex'th material expression output.
 */
void SMaterialEditorCanvas::ConnectMaterialToMaterialExpression(UMaterial* Material,
												int32 MaterialInputIndex,
												UMaterialExpression* MaterialExpression,
												int32 MaterialExpressionOutputIndex,
												bool bUnusedConnectionsHidden)
{
	// Assemble a list of outputs this material expression has.
	TArray<FExpressionOutput>& Outputs = MaterialExpression->GetOutputs();

	if (Outputs.Num() > 0)
	{
		const FExpressionOutput& ExpressionOutput = Outputs[ MaterialExpressionOutputIndex ];
		FExpressionInput* MaterialInput = GetMaterialInputConditional( Material, MaterialInputIndex, bUnusedConnectionsHidden );

		ConnectExpressions( *MaterialInput, ExpressionOutput,MaterialExpressionOutputIndex,  MaterialExpression );
	}
}

/** Refreshes the viewport containing the material expression graph. */
void SMaterialEditorCanvas::RefreshExpressionViewport()
{
	LinkedObjVC->Viewport->Invalidate();
}

void SMaterialEditorCanvas::SetPreviewExpression(UMaterialExpression* NewPreviewExpression)
{
	UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(NewPreviewExpression);

	if( PreviewExpression == NewPreviewExpression )
	{
		if (FunctionOutput)
		{
			FunctionOutput->bLastPreviewed = false;
		}
		// If we are already previewing the selected expression toggle previewing off
		PreviewExpression = NULL;
		MaterialEditorPtr.Pin()->ExpressionPreviewMaterial->Expressions.Empty();
		MaterialEditorPtr.Pin()->SetPreviewMaterial( GetMaterial() );
		// Recompile the preview material to get changes that might have been made during previewing
		UpdatePreviewMaterial();
	}
	else if (NewPreviewExpression)
	{
		UMaterial* PreviewMaterial = MaterialEditorPtr.Pin()->ExpressionPreviewMaterial;
		if( PreviewMaterial == NULL )
		{
			// Create the expression preview material if it hasnt already been created
			PreviewMaterial = MaterialEditorPtr.Pin()->ExpressionPreviewMaterial = (UMaterial*)StaticConstructObject( UMaterial::StaticClass(), GetTransientPackage(), NAME_None, RF_Public);
			PreviewMaterial->bIsPreviewMaterial = true;
		}

		if (FunctionOutput)
		{
			FunctionOutput->bLastPreviewed = true;
		}
		else
		{
			//Hooking up the output of the break expression doesn't make much sense, preview the expression feeding it instead.
			UMaterialExpressionBreakMaterialAttributes* BreakExpr = Cast<UMaterialExpressionBreakMaterialAttributes>(NewPreviewExpression);
			if( BreakExpr && BreakExpr->GetInput(0) && BreakExpr->GetInput(0)->Expression )
			{
				NewPreviewExpression = BreakExpr->GetInput(0)->Expression;
			}
		}


		// The expression preview material's expressions array must stay up to date before recompiling 
		// So that RebuildMaterialFunctionInfo will see all the nested material functions that may need to be updated
		PreviewMaterial->Expressions = GetMaterial()->Expressions;

		// The preview window should now show the expression preview material
		MaterialEditorPtr.Pin()->SetPreviewMaterial( PreviewMaterial );

		// Set the preview expression
		PreviewExpression = NewPreviewExpression;

		// Recompile the preview material
		UpdatePreviewMaterial();
	}
}

/**
 * Refreshes the preview for the specified material expression.  Does nothing if the specified expression
 * has a bRealtimePreview of false.
 *
 * @param	MaterialExpression		The material expression to update.
 */
void SMaterialEditorCanvas::RefreshExpressionPreview(UMaterialExpression* MaterialExpression, bool bRecompile)
{
	if ( (MaterialExpression->bRealtimePreview || MaterialExpression->bNeedToUpdatePreview) && !MaterialExpression->bCollapsed )
	{
		for( int32 PreviewIndex = 0 ; PreviewIndex < ExpressionPreviews.Num() ; ++PreviewIndex )
		{
			FMatExpressionPreview& ExpressionPreview = ExpressionPreviews[PreviewIndex];
			if( ExpressionPreview.GetExpression() == MaterialExpression )
			{
				// we need to make sure the rendering thread isn't drawing this tile
				SCOPED_SUSPEND_RENDERING_THREAD(true);
				ExpressionPreviews.RemoveAt( PreviewIndex );
				MaterialExpression->bNeedToUpdatePreview = false;
				if (bRecompile)
				{
					bool bNewlyCreated;
					GetExpressionPreview(MaterialExpression, bNewlyCreated);
				}
				break;
			}
		}
	}
}

/**
 * Refreshes material expression previews.  Refreshes all previews if bAlwaysRefreshAllPreviews is true.
 * Otherwise, refreshes only those previews that have a bRealtimePreview of true.
 */
void SMaterialEditorCanvas::RefreshExpressionPreviews()
{
	const FScopedBusyCursor BusyCursor;

	if ( bAlwaysRefreshAllPreviews )
	{
		// we need to make sure the rendering thread isn't drawing these tiles
		SCOPED_SUSPEND_RENDERING_THREAD(true);

		// Refresh all expression previews.
		ExpressionPreviews.Empty();
	}
	else
	{
		// Only refresh expressions that are marked for realtime update.
		for ( int32 ExpressionIndex = 0 ; ExpressionIndex < GetMaterial()->Expressions.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* MaterialExpression = GetMaterial()->Expressions[ ExpressionIndex ];
			RefreshExpressionPreview( MaterialExpression, false );
		}
	}

	TArray<FMatExpressionPreview*> ExpressionPreviewsBeingCompiled;
	ExpressionPreviewsBeingCompiled.Empty(50);

	// Go through all expression previews and create new ones as needed, and maintain a list of previews that are being compiled
	for( int32 ExpressionIndex = 0; ExpressionIndex < GetMaterial()->Expressions.Num(); ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = GetMaterial()->Expressions[ ExpressionIndex ];
		if (MaterialExpression && !MaterialExpression->IsA(UMaterialExpressionComment::StaticClass()) )
		{
			bool bNewlyCreated;
			FMatExpressionPreview* Preview = GetExpressionPreview( MaterialExpression, bNewlyCreated );
			if (bNewlyCreated && Preview)
			{
				ExpressionPreviewsBeingCompiled.Add(Preview);
			}
		}
	}
}

/**
 * Refreshes all material expression previews, regardless of whether or not realtime previews are enabled.
 */
void SMaterialEditorCanvas::ForceRefreshExpressionPreviews()
{
	// Initialize expression previews.
	const bool bOldAlwaysRefreshAllPreviews = bAlwaysRefreshAllPreviews;
	bAlwaysRefreshAllPreviews = true;
	RefreshExpressionPreviews();
	bAlwaysRefreshAllPreviews = bOldAlwaysRefreshAllPreviews;
}

/**
 * @param		InMaterialExpression	The material expression to query.
 * @param		ConnType				Type of the connection (LOC_INPUT or LOC_OUTPUT).
 * @param		ConnIndex				Index of the connection to query
 * @return								A viewport location for the connection.
 */
FIntPoint SMaterialEditorCanvas::GetExpressionConnectionLocation(UMaterialExpression* InMaterialExpression, int32 InConnType, int32 InConnIndex)
{
	const FMaterialNodeDrawInfo& ExpressionDrawInfo = GetDrawInfo( InMaterialExpression );

	FIntPoint Result(0,0);
	if ( InConnType == LOC_OUTPUT ) // connectors on the right side of the material
	{
		Result.X = InMaterialExpression->MaterialExpressionEditorX + ExpressionDrawInfo.DrawWidth + LO_CONNECTOR_LENGTH;
		Result.Y = ExpressionDrawInfo.RightYs[InConnIndex];
	}
	else if ( InConnType == LOC_INPUT ) // connectors on the left side of the material
	{
		Result.X = InMaterialExpression->MaterialExpressionEditorX - LO_CONNECTOR_LENGTH,
		Result.Y = ExpressionDrawInfo.LeftYs[InConnIndex];
	}
	return Result;
}

/**
 * @param		InMaterial	The material to query.
 * @param		ConnType	Type of the connection (LOC_OUTPUT).
 * @param		ConnIndex	Index of the connection to query
 * @return					A viewport location for the connection.
 */
FIntPoint SMaterialEditorCanvas::GetMaterialConnectionLocation(UMaterial* InMaterial, int32 InConnType, int32 InConnIndex)
{
	FIntPoint Result(0,0);
	if ( InConnType == LOC_OUTPUT ) // connectors on the right side of the material
	{
		Result.X = InMaterial->EditorX + MaterialDrawInfo.DrawWidth + LO_CONNECTOR_LENGTH;
		Result.Y = MaterialDrawInfo.RightYs[ InConnIndex ];
	}
	return Result;
}

/**
 * Returns the expression preview for the specified material expression.
 */
FMatExpressionPreview* SMaterialEditorCanvas::GetExpressionPreview(UMaterialExpression* MaterialExpression, bool& bNewlyCreated)
{
	bNewlyCreated = false;
	if (!MaterialExpression->bHidePreviewWindow && !MaterialExpression->bCollapsed)
	{
		FMatExpressionPreview*	Preview = NULL;
		for( int32 PreviewIndex = 0 ; PreviewIndex < ExpressionPreviews.Num() ; ++PreviewIndex )
		{
			FMatExpressionPreview& ExpressionPreview = ExpressionPreviews[PreviewIndex];
			if( ExpressionPreview.GetExpression() == MaterialExpression )
			{
				Preview = &ExpressionPreviews[PreviewIndex];
				break;
			}
		}

		if( !Preview )
		{
			bNewlyCreated = true;
			Preview = new(ExpressionPreviews) FMatExpressionPreview(MaterialExpression);
			Preview->CacheShaders(GRHIShaderPlatform, true);
		}
		return Preview;
	}

	return NULL;
}

/**
* Returns the drawinfo object for the specified expression, creating it if one does not currently exist.
*/
SMaterialEditorCanvas::FMaterialNodeDrawInfo& SMaterialEditorCanvas::GetDrawInfo(UMaterialExpression* MaterialExpression)
{
	FMaterialNodeDrawInfo* ExpressionDrawInfo = MaterialNodeDrawInfo.Find( MaterialExpression );
	return ExpressionDrawInfo ? *ExpressionDrawInfo : MaterialNodeDrawInfo.Add( MaterialExpression, FMaterialNodeDrawInfo(MaterialExpression->MaterialExpressionEditorY) );
}

/**
 * Disconnects and removes the given expressions and comments.
 */
void SMaterialEditorCanvas::DeleteObjects( const TArray<UMaterialExpression*>& Expressions, const TArray<UMaterialExpressionComment*>& Comments)
{
	const bool bHaveExpressionsToDelete	= Expressions.Num() > 0;
	const bool bHaveCommentsToDelete		= Comments.Num() > 0;
	if ( bHaveExpressionsToDelete || bHaveCommentsToDelete )
	{
		{
			FString FunctionWarningString;
			bool bFirstExpression = true;
			for (int32 MaterialExpressionIndex = 0; MaterialExpressionIndex < Expressions.Num(); ++MaterialExpressionIndex)
			{
				UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>(Expressions[MaterialExpressionIndex]);
				UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(Expressions[MaterialExpressionIndex]);

				if (FunctionInput)
				{
					if (!bFirstExpression)
					{
						FunctionWarningString += TEXT(", ");
					}
					bFirstExpression = false;
					FunctionWarningString += FunctionInput->InputName;
				}

				if (FunctionOutput)
				{
					if (!bFirstExpression)
					{
						FunctionWarningString += TEXT(", ");
					}
					bFirstExpression = false;
					FunctionWarningString += FunctionOutput->OutputName;
				}
			}

			if (FunctionWarningString.Len() > 0)
			{
				if (EAppReturnType::Yes != FMessageDialog::Open( EAppMsgType::YesNo,
						FText::Format(
							NSLOCTEXT("UnrealEd", "Prompt_MaterialEditorDeleteFunctionInputs", "Delete function inputs or outputs \"{0}\"?\nAny materials which use this function will lose their connections to these function inputs or outputs once deleted."),
							FText::FromString(FunctionWarningString) )))
				{
					// User said don't delete
					return;
				}
			}
		}
		
		// If we are previewing an expression and the expression being previewed was deleted
		bool bPreviewExpressionDeleted			= false;

		{
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorDelete", "Material Editor: Delete") );
			GetMaterial()->Modify();

			// Whack selected expressions.
			for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Expressions.Num() ; ++MaterialExpressionIndex )
			{
				UMaterialExpression* MaterialExpression = Expressions[ MaterialExpressionIndex ];

				DestroyColorPicker();

				if( PreviewExpression == MaterialExpression )
				{
					// The expression being previewed is also being deleted
					bPreviewExpressionDeleted = true;
				}

				MaterialExpression->Modify();
				SwapLinksToExpression(MaterialExpression, NULL, GetMaterial());
				GetMaterial()->Expressions.Remove( MaterialExpression );
				GetMaterial()->RemoveExpressionParameter(MaterialExpression);
				// Make sure the deleted expression is caught by gc
				MaterialExpression->MarkPendingKill();
			}	

			// Whack selected comments.
			for( int32 CommentIndex = 0 ; CommentIndex < Comments.Num() ; ++CommentIndex )
			{
				UMaterialExpressionComment* Comment = Comments[ CommentIndex ];
				Comment->Modify();
				GetMaterial()->EditorComments.Remove( Comment );
			}
		} // ScopedTransaction

		// Deselect all expressions and comments.
		EmptySelection();

		if ( bHaveExpressionsToDelete )
		{
			if( bPreviewExpressionDeleted )
			{
				// The preview expression was deleted.  Null out our reference to it and reset to the normal preview mateiral
				PreviewExpression = NULL;
				MaterialEditorPtr.Pin()->SetPreviewMaterial( GetMaterial() );
			}
			RefreshSourceWindowMaterial();
			UpdateSearch(false);
		}
		UpdatePreviewMaterial();
		GetMaterial()->MarkPackageDirty();
		SetMaterialDirty();

		UpdatePropertyWindow();

		if ( bHaveExpressionsToDelete )
		{
			RefreshExpressionPreviews();
		}
		RefreshExpressionViewport();
	}
}

/**
 * Disconnects and removes the selected material expressions.
 */
void SMaterialEditorCanvas::DeleteSelectedObjects()
{
	DeleteObjects(SelectedExpressions, SelectedComments);
}

/**
 * Pastes into this material from the editor's material copy buffer.
 *
 * @param	PasteLocation	If non-NULL locate the upper-left corner of the new nodes' bound at this location.
 */
void SMaterialEditorCanvas::PasteExpressionsIntoMaterial(const FIntPoint* PasteLocation)
{
	if ( GetCopyPasteBuffer()->Expressions.Num() > 0 || GetCopyPasteBuffer()->EditorComments.Num() > 0 )
	{
		// Empty the selection set, as we'll be selecting the newly created expressions.
		EmptySelection();

		{
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorPaste", "Material Editor: Paste") );
			GetMaterial()->Modify();

			// Copy the expressions in the material copy buffer into this material.
			TArray<UMaterialExpression*> NewExpressions;
			TArray<UMaterialExpression*> NewComments;

			UMaterialExpression::CopyMaterialExpressions( GetCopyPasteBuffer()->Expressions, GetCopyPasteBuffer()->EditorComments, GetMaterial(), GetMaterialFunction(), NewExpressions, NewComments );

			// Append the comments list to the expressions list so we can position all of the items at once.
			NewExpressions.Append(NewComments);
				
			// Locate and select the newly created nodes.
			const FIntRect NewExpressionBounds( GetBoundingBoxOfExpressions( NewExpressions ) );
			for ( int32 ExpressionIndex = 0 ; ExpressionIndex < NewExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* NewExpression = NewExpressions[ ExpressionIndex ];
				if ( PasteLocation )
				{
					// We're doing a paste here.
					NewExpression->MaterialExpressionEditorX += -NewExpressionBounds.Min.X + PasteLocation->X;
					NewExpression->MaterialExpressionEditorY += -NewExpressionBounds.Min.Y + PasteLocation->Y;
				}
				else
				{
					// We're doing a duplicate or straight-up paste; offset the nodes by a fixed amount.
					const int32 DuplicateOffset = 30;
					NewExpression->MaterialExpressionEditorX += DuplicateOffset;
					NewExpression->MaterialExpressionEditorY += DuplicateOffset;
				}
				AddToSelection( NewExpression );
				GetMaterial()->AddExpressionParameter(NewExpression);
			}
		}

		// Update the current preview material
		UpdatePreviewMaterial();
		RefreshSourceWindowMaterial();
		UpdateSearch(false);
		GetMaterial()->MarkPackageDirty();

		UpdatePropertyWindow();

		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

/**
 * Duplicates the selected material expressions.  Preserves internal references.
 */
void SMaterialEditorCanvas::DuplicateSelectedObjects()
{
	// Clear the material copy buffer and copy the selected expressions into it.
	TArray<UMaterialExpression*> NewExpressions;
	TArray<UMaterialExpression*> NewComments;

	GetCopyPasteBuffer()->Expressions.Empty();
	GetCopyPasteBuffer()->EditorComments.Empty();
	UMaterialExpression::CopyMaterialExpressions( SelectedExpressions, SelectedComments, GetCopyPasteBuffer(), GetMaterialFunction(), NewExpressions, NewComments );

	// Paste the material copy buffer into this material.
	PasteExpressionsIntoMaterial( NULL );
}

/**
 * Deletes any disconnected material expressions.
 */
void SMaterialEditorCanvas::CleanUnusedExpressions()
{
	EmptySelection();

	// The set of material expressions to visit.
	TArray<UMaterialExpression*> Stack;

	// Populate the stack with inputs to the material.
	for ( int32 MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs[MaterialInputIndex];
		if( IsVisibleMaterialInput( MaterialInput ) )
		{
			UMaterialExpression* Expression = MaterialInput.Input->Expression;
			if ( Expression )
			{
				Stack.Push( Expression );
			}
		}
		else
		{
			// Disconnect invisible inputs
			MaterialInput.Input->Expression = NULL;
		}
	}

	if (GetMaterialFunction())
	{
		for (int32 ExpressionIndex = 0; ExpressionIndex < GetMaterial()->Expressions.Num(); ExpressionIndex++)
		{
			UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(GetMaterial()->Expressions[ExpressionIndex]);
			if (FunctionOutput)
			{
				Stack.Push(FunctionOutput);
			}
		}
	}

	// Depth-first traverse the material expression graph.
	TArray<UMaterialExpression*> NewExpressions;
	TMap<UMaterialExpression*, int32> ReachableExpressions;
	while ( Stack.Num() > 0 )
	{
		UMaterialExpression* MaterialExpression = Stack.Pop();
		int32* AlreadyVisited = ReachableExpressions.Find( MaterialExpression );
		if ( !AlreadyVisited )
		{
			// Mark the expression as reachable.
			ReachableExpressions.Add( MaterialExpression, 0 );
			NewExpressions.Add( MaterialExpression );

			// Iterate over the expression's inputs and add them to the pending stack.
			const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
			for( int32 ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
			{
				FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
				UMaterialExpression* InputExpression = Input->Expression;
				if ( InputExpression )
				{
					Stack.Push( InputExpression );
				}
			}
		}
	}

	// Check to see if the expressions list was modified, Note: We can't use the operator!= here, as the order isn't important.
	bool bModified = false;
	if(GetMaterial()->Expressions.Num() != NewExpressions.Num())
	{
		bModified = true;
	}
	else
	{
		for(int32 Index = 0;Index < NewExpressions.Num();Index++)
		{
			if(!GetMaterial()->Expressions.Contains(NewExpressions[Index]))
			{
				bModified = true;
				break;
			}
		}
	}

	if ( bModified )
	{
		// Kill off expressions referenced by the material that aren't reachable.
		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorCleanUnusedExpressions", "Material Editor: Clean Unused Expressions") );
		GetMaterial()->Modify();
		GetMaterial()->Expressions = NewExpressions;		
		SetMaterialDirty();
	}

	RefreshExpressionViewport();
}

/** Draws the root node -- that is, the node corresponding to the final material. */
void SMaterialEditorCanvas::DrawMaterialRoot(bool bIsSelected, FCanvas* Canvas)
{
	// Construct the FLinkedObjDrawInfo for use by the linked-obj drawing utils.
	FLinkedObjDrawInfo ObjInfo;
	ObjInfo.ObjObject = GetMaterial();

	// Check if we want to pan, and our mouse is over a material input.
	bool bPanMouseOverInput = DblClickObject==GetMaterial() && DblClickConnType==LOC_OUTPUT;

	const FColor MaterialInputColor( 0, 0, 0 );
	for ( int32 MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs[MaterialInputIndex];
		bool bShouldAddInputConnector = (!bHideUnusedConnectors || MaterialInput.Input->Expression) && IsVisibleMaterialInput( MaterialInput );

		if( MaterialInput.Property == MP_MaterialAttributes )
		{
			bShouldAddInputConnector &= GetMaterial() && GetMaterial()->bUseMaterialAttributes;
		}
		else
		{
			bShouldAddInputConnector &= GetMaterial() && !GetMaterial()->bUseMaterialAttributes;
		}

		if ( bShouldAddInputConnector )
		{
			if( bPanMouseOverInput && MaterialInput.Input->Expression && DblClickConnIndex==ObjInfo.Outputs.Num() )
			{
				PanLocationOnscreen( MaterialInput.Input->Expression->MaterialExpressionEditorX+50, MaterialInput.Input->Expression->MaterialExpressionEditorY+50, 100 );
			}
			ObjInfo.Outputs.Add( FLinkedObjConnInfo(*MaterialInput.Name, MaterialInputColor, false, true, 0, 0, IsActiveMaterialInput(MaterialInput)) );
		}
	}

	// Generate border color
	const FColor BorderColor( bIsSelected ? FColor( 255, 255, 0 ) : FColor( 0, 0, 0 ) );

	// Highlight the currently moused over connector
	HighlightActiveConnectors( ObjInfo );

	// Use util to draw box with connectors, etc.
	const FIntPoint MaterialPos( GetMaterial()->EditorX, GetMaterial()->EditorY );
	FLinkedObjDrawUtils::DrawLinkedObj( Canvas, ObjInfo, *GetMaterial()->GetName(), NULL, BorderColor, FColor(112,112,112), MaterialPos );

	// Copy off connector values for use
	MaterialDrawInfo.DrawWidth	= ObjInfo.DrawWidth;
	MaterialDrawInfo.RightYs	= ObjInfo.OutputY;
}

/**
 * Render connections between the material's inputs and the material expression outputs connected to them.
 */
void SMaterialEditorCanvas::DrawMaterialRootConnections(FCanvas* Canvas)
{
	// Compensates for the difference between the number of rendered inputs
	// (based on bHideUnusedConnectors) and the true number of inputs.
	int32 ActiveInputCounter = -1;

	TArray<FExpressionInput*> ReferencingInputs;
	for ( int32 MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
	{
		const FMaterialInputInfo& MaterialInput = MaterialInputs[MaterialInputIndex];
		UMaterialExpression* MaterialExpression = MaterialInput.Input->Expression;
		bool bWasAddedInputConnector = (!bHideUnusedConnectors || MaterialExpression) && IsVisibleMaterialInput( MaterialInput );
		if( MaterialInput.Property == MP_MaterialAttributes )
		{
			bWasAddedInputConnector &= GetMaterial() && GetMaterial()->bUseMaterialAttributes;
		}
		else
		{
			bWasAddedInputConnector &= GetMaterial() && !GetMaterial()->bUseMaterialAttributes;
		}

		if (bWasAddedInputConnector)
		{
			++ActiveInputCounter;

			if (MaterialExpression)
			{
				TArray<FExpressionOutput>& Outputs = MaterialExpression->GetOutputs();

				if (Outputs.Num() > 0)
				{
					int32 ActiveOutputCounter = -1;
					int32 OutputIndex = 0;
					const bool bOutputIndexIsValid = Outputs.IsValidIndex(MaterialInput.Input->OutputIndex)
						// Attempt to handle legacy connections before OutputIndex was used that had a mask
						&& (MaterialInput.Input->OutputIndex != 0 || MaterialInput.Input->Mask == 0);

					for( ; OutputIndex < Outputs.Num() ; ++OutputIndex )
					{
						const FExpressionOutput& Output = Outputs[OutputIndex];

						// If unused connectors are hidden, the material expression output index needs to be transformed
						// to visible index rather than absolute.
						if ( bHideUnusedConnectors )
						{
							// Get a list of inputs that refer to the selected output.
							GetListOfReferencingInputs( MaterialExpression, GetMaterial(), ReferencingInputs, &Output, OutputIndex );
							if ( ReferencingInputs.Num() > 0 )
							{
								++ActiveOutputCounter;
							}
						}

						if(	bOutputIndexIsValid && OutputIndex == MaterialInput.Input->OutputIndex
							|| !bOutputIndexIsValid
							&& Output.Mask == MaterialInput.Input->Mask
							&& Output.MaskR == MaterialInput.Input->MaskR
							&& Output.MaskG == MaterialInput.Input->MaskG
							&& Output.MaskB == MaterialInput.Input->MaskB
							&& Output.MaskA == MaterialInput.Input->MaskA )
						{
							break;
						}
					}
					
					if (OutputIndex >= Outputs.Num())
					{
						// Work around for non-reproducible crash where OutputIndex would be out of bounds
						OutputIndex = Outputs.Num() - 1;
					}

					const FIntPoint Start( GetMaterialConnectionLocation(GetMaterial(),LOC_OUTPUT,ActiveInputCounter) );

					const int32 ExpressionOutputLookupIndex		= bHideUnusedConnectors ? ActiveOutputCounter : OutputIndex;
					const FIntPoint End( GetExpressionConnectionLocation(MaterialExpression,LOC_INPUT,ExpressionOutputLookupIndex) );


					// If either of the connection ends are highlighted then highlight the line.
					FColor Color;

					if (!IsActiveMaterialInput(MaterialInput))
					{
						// always color connections to inactive inputs gray
						Color = ConnectionOptionalColor;
					}
					else if(IsConnectorHighlighted(GetMaterial(), LOC_OUTPUT, ActiveInputCounter) || IsConnectorHighlighted(MaterialExpression, LOC_INPUT, ExpressionOutputLookupIndex))
					{
						Color = ConnectionSelectedColor;
					}
					else if(IsConnectorMarked(GetMaterial(), LOC_OUTPUT, ActiveInputCounter) || IsConnectorMarked(MaterialExpression, LOC_INPUT, ExpressionOutputLookupIndex))
					{
						Color = ConnectionMarkedColor;
					}
					else
					{
						Color = ConnectionNormalColor;
					}

					// DrawCurves
					{
						const float Tension = FMath::Abs<int32>( Start.X - End.X );
						FLinkedObjDrawUtils::DrawSpline( Canvas, End, -Tension*FVector2D(1,0), Start, -Tension*FVector2D(1,0), Color, true );
					}
				}
			}
		}
	}
}

void SMaterialEditorCanvas::DrawMaterialExpressionLinkedObj(FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo, const TArray<FString>& Captions, const TCHAR* Comment, const FColor& BorderColor, const FColor& TitleBkgColor, UMaterialExpression* MaterialExpression, bool bRenderPreview)
{
	static const int32 ExpressionPreviewSize = 96;
	static const int32 ExpressionPreviewBorder = 1;

	// If an expression is currently being previewed
	const bool bPreviewing = (MaterialExpression == PreviewExpression);
	const bool bCollapsed = MaterialExpression->bCollapsed;
	const bool bIsShaderInput = MaterialExpression->bShaderInputData;
	const bool bHitTesting = Canvas->IsHitTesting();

	const FIntPoint Pos( MaterialExpression->MaterialExpressionEditorX, MaterialExpression->MaterialExpressionEditorY );
	const FIntPoint TitleSize	= FLinkedObjDrawUtils::GetTitleBarSize(Canvas, Captions);
	const FIntPoint LogicSize	= FLinkedObjDrawUtils::GetLogicConnectorsSize(ObjInfo);
	// Draw nodes marked as shader inputs with a larger minimum height, since they will have an indicator draw inside
	const int32 MinBodyHeight = bIsShaderInput ? 32 : 24;

	// Includes one-pixel border on left and right and a three-pixel border between the preview icon and the title text.
	ObjInfo.DrawWidth	= 2 + 3 + PreviewIconWidth + FMath::Max(FMath::Max(TitleSize.X, LogicSize.X), bCollapsed ? 0 : ExpressionPreviewSize+2*ExpressionPreviewBorder);
	const int32 BodyHeight = FMath::Max(2 + FMath::Max(LogicSize.Y, bCollapsed ? 0 : ExpressionPreviewSize+2*ExpressionPreviewBorder), MinBodyHeight);

	// Includes one-pixel spacer between title and body.
	ObjInfo.DrawHeight	= TitleSize.Y + 1 + BodyHeight;

	if (bHitTesting) 
	{
		Canvas->SetHitProxy( new HLinkedObjProxy(ObjInfo.ObjObject) );
	}

	// Added array of comments
	TArray<FString> Comments;

	if (bPreviewing)
	{
		TArray<FString> PreviewingCaption;
		PreviewingCaption.Add(TEXT("Previewing"));
		// Draw a box on top of the normal title bar that indicates we are currently previewing this node
		FLinkedObjDrawUtils::DrawTitleBar( Canvas, FIntPoint(Pos.X, Pos.Y - TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, TitleSize.Y), BorderColor, FColor(70,100,200), PreviewingCaption, Comments );
	}

	Comments.Add(FString(Comment));
	FLinkedObjDrawUtils::DrawTitleBar(Canvas, Pos, FIntPoint(ObjInfo.DrawWidth, TitleSize.Y), BorderColor, TitleBkgColor, Captions, Comments);

	FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X,		Pos.Y + TitleSize.Y + 1,	ObjInfo.DrawWidth,		BodyHeight,		FVector2D( 0.0f, 0.0f ), FVector2D( 0.0f, 0.0f ), BorderColor );
	FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X + 1,	Pos.Y + TitleSize.Y + 2,	ObjInfo.DrawWidth - 2,	BodyHeight-2,	FVector2D( 0.0f, 0.0f ), FVector2D( 0.0f, 0.0f ), FColor(140,140,140) );

	if (bRenderPreview && !bCollapsed)
	{
		bool bNewlyCreated;
		FMatExpressionPreview* ExpressionPreview = GetExpressionPreview( MaterialExpression, bNewlyCreated);
		FLinkedObjDrawUtils::DrawTile( Canvas, Pos.X + 1 + ExpressionPreviewBorder,	Pos.Y + TitleSize.Y + 2 + ExpressionPreviewBorder,	ExpressionPreviewSize,	ExpressionPreviewSize,	FVector2D( 0.0f, 0.0f ), FVector2D( 1.0f, 1.0f ), ExpressionPreview );
	}
	else if (bIsShaderInput)
	{
		// Draw an indicator inside the body of a node marked input data
		const TCHAR* InputString = TEXT("Input Data");
		int32 XL, YL;
		StringSize( FLinkedObjDrawUtils::GetFont(), XL, YL, InputString );

		const FIntPoint StringPos( Pos.X + (ObjInfo.DrawWidth - XL) / 2, Pos.Y + TitleSize.Y + (BodyHeight - YL) / 2 );
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, StringPos.X, StringPos.Y, XL, YL ) )
		{
			FLinkedObjDrawUtils::DrawString( Canvas, StringPos.X, StringPos.Y, InputString, FLinkedObjDrawUtils::GetFont(), TitleBkgColor );
		}
	}

	if (bHitTesting) 
	{
		Canvas->SetHitProxy( NULL );
	}

	// Highlight the currently moused over connector
	HighlightActiveConnectors( ObjInfo );

	FLinkedObjDrawUtils::DrawLogicConnectors(Canvas, ObjInfo, Pos + FIntPoint(0, TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, LogicSize.Y), NULL);
}

/**
 * Draws messages on the specified viewport and canvas.
 */
void SMaterialEditorCanvas::DrawMessages( FViewport* InViewport, FCanvas* Canvas )
{
	if( PreviewExpression != NULL )
	{
		Canvas->PushAbsoluteTransform( FMatrix::Identity );

		// The message to display in the viewport.
		FString Name = FString::Printf( TEXT("Previewing: %s"), *PreviewExpression->GetName() );

		// Size of the tile we are about to draw.  Should extend the length of the view in X.
		const FIntPoint TileSize( InViewport->GetSizeXY().X, 25);
		
		const FColor PreviewColor( 70,100,200 );
		const FColor FontColor( 255,255,128 );

		UFont* FontToUse = GEditor->EditorFont;
		
		Canvas->DrawTile(  0.0f, 0.0f, TileSize.X, TileSize.Y, 0.0f, 0.0f, 0.0f, 0.0f, PreviewColor );

		int32 XL, YL;
		StringSize( FontToUse, XL, YL, *Name );
		if( XL > TileSize.X )
		{
			// There isn't enough room to show the preview expression name
			Name = TEXT("Previewing");
			StringSize( FontToUse, XL, YL, *Name );
		}
		
		// Center the string in the middle of the tile.
		const FIntPoint StringPos( (TileSize.X-XL)/2, ((TileSize.Y-YL)/2)+1 );
		// Draw the preview message
		Canvas->DrawShadowedString(  StringPos.X, StringPos.Y, *Name, FontToUse, FontColor );

		Canvas->PopTransform();
	}
}

/**
 * Called when the user double-clicks an object in the viewport
 *
 * @param	Obj		the object that was double-clicked on
 */
void SMaterialEditorCanvas::DoubleClickedObject(UObject* Obj)
{
	check(Obj);

	UMaterialExpressionConstant3Vector* Constant3Expression = Cast<UMaterialExpressionConstant3Vector>(Obj);
	UMaterialExpressionConstant4Vector* Constant4Expression = Cast<UMaterialExpressionConstant4Vector>(Obj);
	UMaterialExpressionFunctionInput* InputExpression = Cast<UMaterialExpressionFunctionInput>(Obj);
	UMaterialExpressionVectorParameter* VectorExpression = Cast<UMaterialExpressionVectorParameter>(Obj);
	
	FColorChannels ChannelEditStruct;

	if( Constant3Expression )
	{
		ChannelEditStruct.Red = &Constant3Expression->Constant.R;
		ChannelEditStruct.Green = &Constant3Expression->Constant.G;
		ChannelEditStruct.Blue = &Constant3Expression->Constant.B;
	}
	else if( Constant4Expression )
	{
		ChannelEditStruct.Red = &Constant4Expression->Constant.R;
		ChannelEditStruct.Green = &Constant4Expression->Constant.G;
		ChannelEditStruct.Blue = &Constant4Expression->Constant.B;
		ChannelEditStruct.Alpha = &Constant4Expression->Constant.A;
	}
	else if (InputExpression)
	{
		ChannelEditStruct.Red = &InputExpression->PreviewValue.X;
		ChannelEditStruct.Green = &InputExpression->PreviewValue.Y;
		ChannelEditStruct.Blue = &InputExpression->PreviewValue.Z;
		ChannelEditStruct.Alpha = &InputExpression->PreviewValue.W;
	}
	else if (VectorExpression)
	{
		ChannelEditStruct.Red = &VectorExpression->DefaultValue.R;
		ChannelEditStruct.Green = &VectorExpression->DefaultValue.G;
		ChannelEditStruct.Blue = &VectorExpression->DefaultValue.B;
		ChannelEditStruct.Alpha = &VectorExpression->DefaultValue.A;
	}

	if (ChannelEditStruct.Red || ChannelEditStruct.Green || ChannelEditStruct.Blue || ChannelEditStruct.Alpha)
	{
		TArray<FColorChannels> Channels;
		Channels.Add(ChannelEditStruct);

		ColorPickerObject = Obj;

		// Open a color picker that only sends updates when Ok is clicked, 
		// Since it is too slow to recompile preview expressions as the user is picking different colors
		FColorPickerArgs PickerArgs;
		PickerArgs.ParentWidget = AsShared();
		PickerArgs.bUseAlpha = Constant4Expression != NULL || VectorExpression != NULL;
		PickerArgs.bOnlyRefreshOnOk = true;
		PickerArgs.DisplayGamma = TAttribute<float>::Create( TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma) );
		PickerArgs.ColorChannelsArray = &Channels;
		PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SMaterialEditorCanvas::OnColorPickerCommitted);
		PickerArgs.PreColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SMaterialEditorCanvas::PreColorPickerCommit);

		OpenColorPicker(PickerArgs);
	}

	UMaterialExpressionTextureSample* TextureExpression = Cast<UMaterialExpressionTextureSample>(Obj);
	UMaterialExpressionTextureSampleParameter* TextureParameterExpression = Cast<UMaterialExpressionTextureSampleParameter>(Obj);
	UMaterialExpressionMaterialFunctionCall* FunctionExpression = Cast<UMaterialExpressionMaterialFunctionCall>(Obj);
	UMaterialExpressionCollectionParameter* CollectionParameter = Cast<UMaterialExpressionCollectionParameter>(Obj);

	TArray<UObject*> ObjectsToView;
	UObject* ObjectToEdit = NULL;

	if (TextureExpression && TextureExpression->Texture)
	{
		ObjectsToView.Add(TextureExpression->Texture);
	}
	else if (TextureParameterExpression && TextureParameterExpression->Texture)
	{
		ObjectsToView.Add(TextureParameterExpression->Texture);
	}
	else if (FunctionExpression && FunctionExpression->MaterialFunction)
	{
		ObjectToEdit = FunctionExpression->MaterialFunction;
	}
	else if (CollectionParameter && CollectionParameter->Collection)
	{
		ObjectToEdit = CollectionParameter->Collection;
	}

	if (ObjectsToView.Num() > 0)
	{
		GEditor->SyncBrowserToObjects(ObjectsToView);
	}
	if (ObjectToEdit)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(ObjectToEdit);
	}
}

/**
* Called when double-clicking a connector.
* Used to pan the connection's link into view
*
* @param	Connector	The connector that was double-clicked
*/
void SMaterialEditorCanvas::DoubleClickedConnector(FLinkedObjectConnector& Connector)
{
	DblClickObject = Connector.ConnObj;
	DblClickConnType = Connector.ConnType;
	DblClickConnIndex = Connector.ConnIndex;
}

/** Draws the specified material expression node. */
void SMaterialEditorCanvas::DrawMaterialExpression(UMaterialExpression* MaterialExpression, bool bExpressionSelected, FCanvas* Canvas, FLinkedObjDrawInfo& ObjInfo)
{
	// Construct the FLinkedObjDrawInfo for use by the linked-obj drawing utils.
	ObjInfo.ObjObject = MaterialExpression;

	if (LinkedObjVC->MouseOverObject == MaterialExpression 
		&& LinkedObjVC->MouseOverConnIndex == INDEX_NONE
		&& (FPlatformTime::Seconds() - LinkedObjVC->MouseOverTime) > LinkedObjVC->ToolTipDelayMS * .001f)
	{
		MaterialExpression->GetExpressionToolTip(ObjInfo.ToolTips);
	}

	// Check if we want to pan, and our mouse is over an input for this expression.
	bool bPanMouseOverInput = DblClickObject==MaterialExpression && DblClickConnType==LOC_OUTPUT;

	// Material expression inputs, drawn on the right side of the node.
	const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
	for( int32 ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
	{
		FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
		UMaterialExpression* InputExpression = Input->Expression;
		const bool bShouldAddInputConnector = (!bHideUnusedConnectors || InputExpression) && MaterialExpression->bShowInputs;
		if ( bShouldAddInputConnector )
		{
			if( bPanMouseOverInput && Input->Expression && DblClickConnIndex==ObjInfo.Outputs.Num() )
			{
				PanLocationOnscreen( Input->Expression->MaterialExpressionEditorX+50, Input->Expression->MaterialExpressionEditorY+50, 100 );
			}

			FString InputName = MaterialExpression->GetInputName(ExpressionInputIndex);
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

			const FColor InputConnectorColor = MaterialExpression->IsInputConnectionRequired(ExpressionInputIndex) ? ConnectionNormalColor : ConnectionOptionalColor;
			ObjInfo.Outputs.Add( FLinkedObjConnInfo(*InputName, InputConnectorColor) );

			if (IsConnectorHighlighted(MaterialExpression, LOC_OUTPUT, ObjInfo.Outputs.Num() - 1)
				&& (FPlatformTime::Seconds() - LinkedObjVC->MouseOverTime) > LinkedObjVC->ToolTipDelayMS * .001f)
			{
				MaterialExpression->GetConnectorToolTip(ExpressionInputIndex, INDEX_NONE, ObjInfo.Outputs.Last().ToolTips);
			}
		}
	}

	// Material expression outputs, drawn on the left side of the node
	TArray<FExpressionOutput>& Outputs = MaterialExpression->GetOutputs();

	// Check if we want to pan, and our mouse is over an output for this expression.
	bool bPanMouseOverOutput = DblClickObject==MaterialExpression && DblClickConnType==LOC_INPUT;

	TArray<FExpressionInput*> ReferencingInputs;
	for( int32 OutputIndex = 0 ; OutputIndex < Outputs.Num() ; ++OutputIndex )
	{
		const FExpressionOutput& ExpressionOutput = Outputs[OutputIndex];
		bool bShouldAddOutputConnector = true;
		if ( bHideUnusedConnectors )
		{
			// Get a list of inputs that refer to the selected output.
			GetListOfReferencingInputs( MaterialExpression, GetMaterial(), ReferencingInputs, &ExpressionOutput, OutputIndex );
			bShouldAddOutputConnector = ReferencingInputs.Num() > 0;
		}

		if ( bShouldAddOutputConnector && MaterialExpression->bShowOutputs )
		{
			const TCHAR* OutputName = TEXT("");
			FColor OutputColor( 0, 0, 0 );

			if( ExpressionOutput.Mask )
			{
				if		( ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// R
					OutputColor = FColor( 255, 0, 0 );
					//OutputName = TEXT("R");
				}
				else if	(!ExpressionOutput.MaskR &&  ExpressionOutput.MaskG && !ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// G
					OutputColor = FColor( 0, 255, 0 );
					//OutputName = TEXT("G");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG &&  ExpressionOutput.MaskB && !ExpressionOutput.MaskA)
				{
					// B
					OutputColor = FColor( 0, 0, 255 );
					//OutputName = TEXT("B");
				}
				else if	(!ExpressionOutput.MaskR && !ExpressionOutput.MaskG && !ExpressionOutput.MaskB &&  ExpressionOutput.MaskA)
				{
					// A
					OutputColor = FColor( 255, 255, 255 );
					//OutputName = TEXT("A");
				}
				else
				{
					// RGBA
					//OutputName = TEXT("RGBA");
				}
			}

			if (MaterialExpression->bShowOutputNameOnPin)
			{
				OutputName = *(ExpressionOutput.OutputName);
			}

			// If this is the output we're hovering over, pan its first connection into view.
			if( bPanMouseOverOutput && DblClickConnIndex==ObjInfo.Inputs.Num() )
			{
				// Find what this output links to.
				TArray<FMaterialExpressionReference> References;
				GetListOfReferencingInputs(MaterialExpression, GetMaterial(), References, &ExpressionOutput, OutputIndex);
				if( References.Num() > 0 )
				{
					if( References[0].Expression == NULL )
					{
						// connects to the root node
						PanLocationOnscreen( GetMaterial()->EditorX+50, GetMaterial()->EditorY+50, 100 );
					}
					else
					{
						PanLocationOnscreen( References[0].Expression->MaterialExpressionEditorX+50, References[0].Expression->MaterialExpressionEditorY+50, 100 );
					}
				}
			}

			// We use the "Inputs" array so that the connectors are drawn on the left side of the node.
			ObjInfo.Inputs.Add( FLinkedObjConnInfo(OutputName, OutputColor) );

			if (IsConnectorHighlighted(MaterialExpression, LOC_INPUT, ObjInfo.Inputs.Num() - 1)
				&& (FPlatformTime::Seconds() - LinkedObjVC->MouseOverTime) > LinkedObjVC->ToolTipDelayMS * .001f)
			{
				MaterialExpression->GetConnectorToolTip(INDEX_NONE, OutputIndex, ObjInfo.Inputs.Last().ToolTips);
			}
		}
	}

	// Determine the texture dependency length for the material and the expression.
	const FMaterialResource* MaterialResource = GetMaterial()->GetMaterialResource(GRHIFeatureLevel);
	const bool bCurrentSearchResult = SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() && SearchResults[SelectedSearchResult] == MaterialExpression;

	const FMaterialResource* ErrorMaterialResource = PreviewExpression ? MaterialEditorPtr.Pin()->ExpressionPreviewMaterial->GetMaterialResource(GRHIFeatureLevel) : MaterialResource;
	bool bIsErrorExpression = ErrorMaterialResource->GetErrorExpressions().Find(MaterialExpression) != INDEX_NONE;

	if (bShowMobileStats)
	{
		const FMaterialResource* ErrorMaterialResourceES2 = NULL;
		if (PreviewExpression)
		{
			ErrorMaterialResourceES2 = MaterialEditorPtr.Pin()->ExpressionPreviewMaterial->GetMaterialResource(GRHIFeatureLevel);
		}
		else
		{
			ErrorMaterialResourceES2 = GetMaterial()->GetMaterialResource(GRHIFeatureLevel);
		}
		bIsErrorExpression |= (ErrorMaterialResourceES2 && ErrorMaterialResourceES2->GetErrorExpressions().Find(MaterialExpression) != INDEX_NONE);
	}

	// Generate border color
	FColor BorderColor = MaterialExpression->BorderColor;
	BorderColor.A = 255;
	
	if (bExpressionSelected)
	{
		BorderColor = FColor( 128, 128, 255 );
	}
	else if (bIsErrorExpression)
	{
		// Outline expressions that caused errors in red
		BorderColor = FColor( 255, 0, 0 );
	}
	else if (bCurrentSearchResult)
	{
		BorderColor = FColor( 64, 64, 255 );
	}
	else if (PreviewExpression == MaterialExpression)
	{
		// If we are currently previewing a node, its border should be the preview color.
		BorderColor = FColor( 70, 100, 200 );
	}
	else if (UMaterial::IsParameter(MaterialExpression))
	{
		if (GetMaterial()->HasDuplicateParameters(MaterialExpression))
		{
			BorderColor = FColor( 0, 255, 255 );
		}
		else
		{
			BorderColor = FColor( 0, 128, 128 );
		}
	}
	else if (UMaterial::IsDynamicParameter(MaterialExpression))
	{
		if (GetMaterial()->HasDuplicateDynamicParameters(MaterialExpression))
		{
			BorderColor = FColor( 0, 255, 255 );
		}
		else
		{
			BorderColor = FColor( 0, 128, 128 );
		}
	}

	// Use util to draw box with connectors, etc.
	TArray<FString> Captions;
	MaterialExpression->GetCaption(Captions);
	DrawMaterialExpressionLinkedObj( Canvas, ObjInfo, Captions, NULL, BorderColor, bCurrentSearchResult ? BorderColor : FColor(112,112,112), MaterialExpression, !(MaterialExpression->bHidePreviewWindow) );

	// Read back the height of the first connector on the left side of the node,
	// for use later when drawing connections to this node.
	FMaterialNodeDrawInfo& ExpressionDrawInfo	= GetDrawInfo( MaterialExpression );
	ExpressionDrawInfo.LeftYs					= ObjInfo.InputY;
	ExpressionDrawInfo.RightYs					= ObjInfo.OutputY;
	ExpressionDrawInfo.DrawWidth				= ObjInfo.DrawWidth;

	check( bHideUnusedConnectors || ExpressionDrawInfo.RightYs.Num() == ExpressionInputs.Num() || !MaterialExpression->bShowInputs );

	// Draw collapsed indicator above the node.
	if (MaterialExpression->bHidePreviewWindow == false)
	{
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X, MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y, PreviewIconWidth, PreviewIconWidth ) )
		{
			const bool bHitTesting = Canvas->IsHitTesting();
			if( bHitTesting )  
			{
				Canvas->SetHitProxy( new HCollapsedProxy( MaterialExpression ) );
			}

			UTexture2D* UsedTexture = NULL;
			
			if (MaterialExpression->bRealtimePreview)
			{
				UsedTexture = MaterialExpression->bCollapsed ? NodeCollapsedTexture : NodeExpandedTextureRealtimePreview;
			}
			else
			{
				UsedTexture = MaterialExpression->bCollapsed ? NodeCollapsedTexture : NodeExpandedTexture;
			}

			FLinkedObjDrawUtils::DrawTile( Canvas, MaterialExpression->MaterialExpressionEditorX+PreviewIconLoc.X, MaterialExpression->MaterialExpressionEditorY+PreviewIconLoc.Y, PreviewIconWidth,	PreviewIconWidth, FVector2D( 0.0f, 0.0f ), FVector2D( 1.0f, 1.0f ), FLinearColor::White, UsedTexture->Resource );

			if( bHitTesting )  
			{
				Canvas->SetHitProxy( NULL );
			}
		}
	}
}

/**
 * Render connectors between this material expression's inputs and the material expression outputs connected to them.
 */
void SMaterialEditorCanvas::DrawMaterialExpressionConnections(UMaterialExpression* MaterialExpression, FCanvas* Canvas)
{
	// Compensates for the difference between the number of rendered inputs
	// (based on bHideUnusedConnectors) and the true number of inputs.
	int32 ActiveInputCounter = -1;

	TArray<FExpressionInput*> ReferencingInputs;

	if(!MaterialExpression)
	{
		return;
	}

	const TArray<FExpressionInput*>& ExpressionInputs = MaterialExpression->GetInputs();
	for( int32 ExpressionInputIndex = 0 ; ExpressionInputIndex < ExpressionInputs.Num() ; ++ExpressionInputIndex )
	{
		FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
		UMaterialExpression* InputExpression = Input->Expression;
		if ( InputExpression )
		{
			++ActiveInputCounter;

			// Find the matching output.
			TArray<FExpressionOutput>& Outputs = InputExpression->GetOutputs();

			if (Outputs.Num() > 0)
			{
				int32 ActiveOutputCounter = -1;
				int32 OutputIndex = 0;
				const bool bOutputIndexIsValid = Outputs.IsValidIndex(Input->OutputIndex)
					// Attempt to handle legacy connections before OutputIndex was used that had a mask
					&& (Input->OutputIndex != 0 || Input->Mask == 0);

				for( ; OutputIndex < Outputs.Num() ; ++OutputIndex )
				{
					const FExpressionOutput& Output = Outputs[OutputIndex];

					// If unused connectors are hidden, the material expression output index needs to be transformed
					// to visible index rather than absolute.
					if ( bHideUnusedConnectors )
					{
						// Get a list of inputs that refer to the selected output.
						GetListOfReferencingInputs( InputExpression, GetMaterial(), ReferencingInputs, &Output, OutputIndex );
						if ( ReferencingInputs.Num() > 0 )
						{
							++ActiveOutputCounter;
						}
					}

					if (bOutputIndexIsValid && Input->OutputIndex == OutputIndex
						|| !bOutputIndexIsValid 
						&& Output.Mask == Input->Mask
						&& Output.MaskR == Input->MaskR
						&& Output.MaskG == Input->MaskG
						&& Output.MaskB == Input->MaskB
						&& Output.MaskA == Input->MaskA )
					{
						break;
					}
				}

				if (OutputIndex >= Outputs.Num())
				{
					// Work around for non-reproducible crash where OutputIndex would be out of bounds
					OutputIndex = Outputs.Num() - 1;
				}

				const int32 ExpressionInputLookupIndex		= bHideUnusedConnectors ? ActiveInputCounter : ExpressionInputIndex;
				const FIntPoint Start( GetExpressionConnectionLocation(MaterialExpression,LOC_OUTPUT,ExpressionInputLookupIndex) );
				const int32 ExpressionOutputLookupIndex		= bHideUnusedConnectors ? ActiveOutputCounter : OutputIndex;
				const FIntPoint End( GetExpressionConnectionLocation(InputExpression,LOC_INPUT,ExpressionOutputLookupIndex) );


				// If either of the connection ends are highlighted then highlight the line.
				FColor Color;

				if(IsConnectorHighlighted(MaterialExpression, LOC_OUTPUT, ExpressionInputLookupIndex) || IsConnectorHighlighted(InputExpression, LOC_INPUT, ExpressionOutputLookupIndex))
				{
					Color = ConnectionSelectedColor;
				}
				else if(IsConnectorMarked(MaterialExpression, LOC_OUTPUT, ExpressionInputLookupIndex) || IsConnectorMarked(InputExpression, LOC_INPUT, ExpressionOutputLookupIndex))
				{
					Color = ConnectionMarkedColor;
				}
				else
				{
					Color = ConnectionNormalColor;
				}

				// DrawCurves
				{
					const float Tension = FMath::Abs<int32>( Start.X - End.X );
					FLinkedObjDrawUtils::DrawSpline( Canvas, End, -Tension*FVector2D(1,0), Start, -Tension*FVector2D(1,0), Color, true );
				}
			}
		}
	}
}

/** Draws comments for the specified material expression node. */
void SMaterialEditorCanvas::DrawMaterialExpressionComments(UMaterialExpression* MaterialExpression, FCanvas* Canvas)
{
	// Draw the material expression comment string unzoomed.
	if( MaterialExpression->Desc.Len() > 0 )
	{
		const float OldZoom2D = FLinkedObjDrawUtils::GetUniformScaleFromMatrix(Canvas->GetFullTransform());

		int32 XL, YL;
		StringSize( GEditor->EditorFont, XL, YL, *MaterialExpression->Desc );

		// We always draw comment-box text at normal size (don't scale it as we zoom in and out.)
		const int32 x = FMath::Trunc( MaterialExpression->MaterialExpressionEditorX*OldZoom2D );
		const int32 y = FMath::Trunc( MaterialExpression->MaterialExpressionEditorY*OldZoom2D - YL );

		// Viewport cull at a zoom of 1.0, because that's what we'll be drawing with.
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, MaterialExpression->MaterialExpressionEditorX, MaterialExpression->MaterialExpressionEditorY - YL, XL, YL ) )
		{
			Canvas->PushRelativeTransform(FScaleMatrix(1.0f / OldZoom2D));
			{
				FLinkedObjDrawUtils::DrawString( Canvas, x, y, *MaterialExpression->Desc, GEditor->EditorFont, FColor(64,64,192) );
			}
			Canvas->PopTransform();
		}
	}
}

/** Draws tooltips for material expressions. */
void SMaterialEditorCanvas::DrawMaterialExpressionToolTips(const TArray<FLinkedObjDrawInfo>& LinkedObjDrawInfos, FCanvas* Canvas)
{
	for (int32 DrawInfoIndex = 0; DrawInfoIndex < LinkedObjDrawInfos.Num(); DrawInfoIndex++)
	{
		const FLinkedObjDrawInfo& ObjInfo = LinkedObjDrawInfos[DrawInfoIndex];
		UMaterialExpression* MaterialExpression = Cast<UMaterialExpression>(ObjInfo.ObjObject);
		if (MaterialExpression)
		{
			const FIntPoint Pos( MaterialExpression->MaterialExpressionEditorX, MaterialExpression->MaterialExpressionEditorY );

			TArray<FString> Captions;
			MaterialExpression->GetCaption(Captions);
			const FIntPoint TitleSize	= FLinkedObjDrawUtils::GetTitleBarSize(Canvas, Captions);

			FLinkedObjDrawUtils::DrawToolTips(Canvas, ObjInfo, Pos + FIntPoint(0, TitleSize.Y + 1), FIntPoint(ObjInfo.DrawWidth, ObjInfo.DrawHeight));
		}
	}
}

/** Draws UMaterialExpressionComment nodes. */
void SMaterialEditorCanvas::DrawCommentExpressions(FCanvas* Canvas)
{
	const bool bHitTesting = Canvas->IsHitTesting();
	FCanvasLineItem LineItem;		
	for ( int32 CommentIndex = 0 ; CommentIndex < GetMaterial()->EditorComments.Num() ; ++CommentIndex )
	{
		UMaterialExpressionComment* Comment = GetMaterial()->EditorComments[CommentIndex];

		const bool bSelected = SelectedComments.Contains( Comment );
		const bool bCurrentSearchResult = SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() && SearchResults[SelectedSearchResult] == Comment;

		const FColor FrameColor = bCurrentSearchResult ? FColor( 128, 255, 128 ) : bSelected ? FColor(255,255,0) : FColor(128, 128, 128);
		FCanvasItemTestbed::bTestState = !FCanvasItemTestbed::bTestState;
		LineItem.SetColor( FrameColor );		
		for(int32 i=0; i<1; i++)
		{
			LineItem.Draw( Canvas, FVector2D(Comment->MaterialExpressionEditorX,						Comment->MaterialExpressionEditorY + i),					FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX,		Comment->MaterialExpressionEditorY + i) );
			LineItem.Draw( Canvas, FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX - i,	Comment->MaterialExpressionEditorY),						FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX - i,	Comment->MaterialExpressionEditorY + Comment->SizeY)		);
			LineItem.Draw( Canvas, FVector2D(Comment->MaterialExpressionEditorX + Comment->SizeX,		Comment->MaterialExpressionEditorY + Comment->SizeY - i),	FVector2D(Comment->MaterialExpressionEditorX,						Comment->MaterialExpressionEditorY + Comment->SizeY - i)	);
			LineItem.Draw( Canvas, FVector2D(Comment->MaterialExpressionEditorX + i,					Comment->MaterialExpressionEditorY + Comment->SizeY),		FVector2D(Comment->MaterialExpressionEditorX + i,					Comment->MaterialExpressionEditorY - 1)					);			
		}

		// Draw little sizing triangle in bottom left.
		const int32 HandleSize = 16;
		const FIntPoint A(Comment->MaterialExpressionEditorX + Comment->SizeX,				Comment->MaterialExpressionEditorY + Comment->SizeY);
		const FIntPoint B(Comment->MaterialExpressionEditorX + Comment->SizeX,				Comment->MaterialExpressionEditorY + Comment->SizeY - HandleSize);
		const FIntPoint C(Comment->MaterialExpressionEditorX + Comment->SizeX - HandleSize,	Comment->MaterialExpressionEditorY + Comment->SizeY);
		const uint8 TriangleAlpha = (bSelected) ? 255 : 32; // Make it more transparent if comment is not selected.

		if(bHitTesting)  Canvas->SetHitProxy( new HLinkedObjProxySpecial(Comment, 1) );
		FCanvasTriangleItem TriItem( A, B, C, GWhiteTexture );
		TriItem.SetColor( FLinearColor( 0.0f, 0.0f, 0.0f, (bSelected) ? 1.0f : 0.125f ) );
		TriItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem( TriItem );
		if(bHitTesting)  Canvas->SetHitProxy( NULL );

		// Check there are some valid chars in string. If not - we can't select it! So we force it back to default.
		bool bHasProperChars = false;
		for( int32 i = 0 ; i < Comment->Text.Len() ; ++i )
		{
			if ( Comment->Text[i] != ' ' )
			{
				bHasProperChars = true;
				break;
			}
		}
		if ( !bHasProperChars )
		{
			Comment->Text = TEXT("Comment");
		}

		const float OldZoom2D = FLinkedObjDrawUtils::GetUniformScaleFromMatrix(Canvas->GetFullTransform());

		int32 XL, YL;
		StringSize( GEditor->EditorFont, XL, YL, *Comment->Text );

		// We always draw comment-box text at normal size (don't scale it as we zoom in and out.)
		const int32 x = FMath::Trunc(Comment->MaterialExpressionEditorX*OldZoom2D + 2);
		const int32 y = FMath::Trunc(Comment->MaterialExpressionEditorY*OldZoom2D - YL - 2);

		// Viewport cull at a zoom of 1.0, because that's what we'll be drawing with.
		if ( FLinkedObjDrawUtils::AABBLiesWithinViewport( Canvas, Comment->MaterialExpressionEditorX+2, Comment->MaterialExpressionEditorY-YL-2, XL, YL ) )
		{
			Canvas->PushRelativeTransform(FScaleMatrix(1.0f / OldZoom2D));
			{
				// We only set the hit proxy for the comment text.
				if ( bHitTesting ) Canvas->SetHitProxy( new HLinkedObjProxy(Comment) );
				Canvas->DrawShadowedString( x, y, *Comment->Text, GEditor->EditorFont, FColor(64,64,192) );
				if ( bHitTesting ) Canvas->SetHitProxy( NULL );
			}
			Canvas->PopTransform();
		}
	}
}

/**
 * Highlights any active logic connectors in the specified linked object.
 */
void SMaterialEditorCanvas::HighlightActiveConnectors(FLinkedObjDrawInfo &ObjInfo)
{
	for(int32 InputIdx=0; InputIdx<ObjInfo.Inputs.Num(); InputIdx++)
	{
		if((LinkedObjVC->MouseOverConnType==LOC_INPUT || DblClickConnType==LOC_INPUT)
			&& IsConnectorHighlighted(ObjInfo.ObjObject, LOC_INPUT, InputIdx))
		{
			ObjInfo.Inputs[InputIdx].Color = ConnectionSelectedColor;
		}
		else if(IsConnectorMarked(ObjInfo.ObjObject, LOC_INPUT, InputIdx))
		{
			ObjInfo.Inputs[InputIdx].Color = ConnectionMarkedColor;
		}
	}

	for(int32 OutputIdx=0; OutputIdx<ObjInfo.Outputs.Num(); OutputIdx++)
	{
		if((LinkedObjVC->MouseOverConnType==LOC_OUTPUT || DblClickConnType==LOC_OUTPUT)
			&& IsConnectorHighlighted(ObjInfo.ObjObject, LOC_OUTPUT, OutputIdx))
		{
			ObjInfo.Outputs[OutputIdx].Color = ConnectionSelectedColor;
		}
		else if(IsConnectorMarked(ObjInfo.ObjObject, LOC_OUTPUT, OutputIdx))
		{
			ObjInfo.Outputs[OutputIdx].Color = ConnectionMarkedColor;
		}
	}
}

/** @return Returns whether the specified connector is currently highlighted or not. */
bool SMaterialEditorCanvas::IsConnectorHighlighted(UObject* Object, uint8 InConnType, int32 InConnIndex)
{
	bool bResult = false;

	if((Object==LinkedObjVC->MouseOverObject && InConnIndex==LinkedObjVC->MouseOverConnIndex && InConnType==LinkedObjVC->MouseOverConnType)
	||(Object==DblClickObject && InConnIndex==DblClickConnIndex && InConnType==DblClickConnType))
	{
		bResult = true;
	}

	return bResult;
}

/** @return Returns whether the specified connector is currently marked or not. */
bool SMaterialEditorCanvas::IsConnectorMarked(UObject* Object, uint8 InConnType, int32 InConnIndex)
{
	bool bResult = false;

	if (Object == MarkedObject && InConnIndex == MarkedConnIndex && InConnType == MarkedConnType)
	{
		bResult = true;
	}

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FLinkedObjViewportClient interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SMaterialEditorCanvas::DrawObjects(FViewport* InViewport, FCanvas* Canvas)
{
	Canvas->Clear(  FColor(210,210,210) );

	// Draw comments.
	DrawCommentExpressions( Canvas );

	if (!GetMaterialFunction())
	{
		// Draw the root node -- that is, the node corresponding to the final material.
		DrawMaterialRoot( false, Canvas );
	}

	TArray<UMaterialExpression*> SortedMaterialExpressions = GetMaterial()->Expressions;

	struct FCompareUMaterialExpressionByEditorY
	{
		FORCEINLINE bool operator()( const UMaterialExpression& A, const UMaterialExpression& B ) const
		{
			return A.MaterialExpressionEditorY < B.MaterialExpressionEditorY;
		}
	};
	SortedMaterialExpressions.Sort( FCompareUMaterialExpressionByEditorY() );
	
	TArray<FLinkedObjDrawInfo> LinkedObjDrawInfos;
	LinkedObjDrawInfos.Empty(GetMaterial()->Expressions.Num());
	LinkedObjDrawInfos.AddZeroed(GetMaterial()->Expressions.Num());

	// Draw the material expression nodes.
	for( int32 ExpressionIndex = 0 ; ExpressionIndex < SortedMaterialExpressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = SortedMaterialExpressions[ ExpressionIndex ];
		// @hack DB: For some reason, materials that were resaved in gemini have comment expressions in the material's
		// @hack DB: expressions list.  The check below is required to prevent them from rendering as normal expression nodes.
		if ( MaterialExpression && !MaterialExpression->IsA(UMaterialExpressionComment::StaticClass()) )
		{
			const bool bExpressionSelected = SelectedExpressions.Contains( MaterialExpression );
			DrawMaterialExpression( MaterialExpression, bExpressionSelected, Canvas, LinkedObjDrawInfos[ExpressionIndex] );
		}
	}

	if (!GetMaterialFunction())
	{
		// Render connections between the material's inputs and the material expression outputs connected to them.
		DrawMaterialRootConnections( Canvas );
	}

	// Render connectors between material expressions' inputs and the material expression outputs connected to them.
	for( int32 ExpressionIndex = 0 ; ExpressionIndex < SortedMaterialExpressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = SortedMaterialExpressions[ ExpressionIndex ];
		const bool bExpressionSelected = SelectedExpressions.Contains( MaterialExpression );
		DrawMaterialExpressionConnections( MaterialExpression, Canvas );
	}

	// Draw the material expression comments.
	for( int32 ExpressionIndex = 0 ; ExpressionIndex < GetMaterial()->Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = GetMaterial()->Expressions[ ExpressionIndex ];
		DrawMaterialExpressionComments( MaterialExpression, Canvas );
	}

	if (LinkedObjVC->MouseOverObject)
	{
		DrawMaterialExpressionToolTips(LinkedObjDrawInfos, Canvas);
	}

	const FMaterialResource* MaterialResources[2] = {0};
	int32 NumResources = 0;
	if (bShowStats)
	{
		MaterialResources[NumResources++] = GetMaterial()->GetMaterialResource(GRHIFeatureLevel);
	}
	if (bShowMobileStats)
	{
		MaterialResources[NumResources++] = GetMaterial()->GetMaterialResource(ERHIFeatureLevel::ES2);
	}
	
	if(NumResources > 0)
	{
		int32 DrawPositionY = 5;
		
		Canvas->PushAbsoluteTransform(FMatrix::Identity);

		TArray<FString> CompileErrors;
		
		if (GetMaterialFunction() && MaterialEditorPtr.Pin()->ExpressionPreviewMaterial)
		{
			// Add a compile error message for functions missing an output
			CompileErrors = MaterialEditorPtr.Pin()->ExpressionPreviewMaterial->GetMaterialResource(GRHIFeatureLevel)->GetCompileErrors();

			bool bFoundFunctionOutput = false;
			for (int32 ExpressionIndex = 0; ExpressionIndex < GetMaterial()->Expressions.Num(); ExpressionIndex++)
			{
				if (GetMaterial()->Expressions[ExpressionIndex]->IsA(UMaterialExpressionFunctionOutput::StaticClass()))
				{
					bFoundFunctionOutput = true;
					break;
				}
			}

			if (!bFoundFunctionOutput)
			{
				CompileErrors.Add(TEXT("Missing a function output"));
			}
			
			// Only draw material stats if enabled.
			FMaterialEditor::DrawMaterialInfoStrings(Canvas, GetMaterial(), MaterialResources[0], CompileErrors, DrawPositionY, false);
		}
		else
		{
			for (int32 i = 0; i < NumResources; ++i)
			{
				CompileErrors = MaterialResources[i]->GetCompileErrors();

				// Only draw material stats if enabled.
				FMaterialEditor::DrawMaterialInfoStrings(Canvas, GetMaterial(), MaterialResources[i], CompileErrors, DrawPositionY, true);
			}
		}

		Canvas->PopTransform();
	}
}

/**
 * Called when the user right-clicks on an empty region of the expression viewport.
 */
void SMaterialEditorCanvas::OpenNewObjectMenu()
{
	// The user has clicked on the background, so clear the last connector object reference so that
	// any newly created material expression node will not be automatically connected to the
	// connector last clicked upon.
	ConnObj = NULL;
	
	// @todo: Should actually use the location from a click event instead!
	const FVector2D MouseCursorLocation = FSlateApplication::Get().GetCursorPos();

	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		BuildMenuWidgetEmpty(),
		MouseCursorLocation,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
		);
}


/**
 * Called when the user right-clicks on an object in the viewport.
 */
void SMaterialEditorCanvas::OpenObjectOptionsMenu()
{
	const FVector2D MouseCursorLocation = FSlateApplication::Get().GetCursorPos();

	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		BuildMenuWidgetObjects(),
		MouseCursorLocation,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
		);
}

/**
 * Called when the user right-clicks on an object connector in the viewport.
 */
void SMaterialEditorCanvas::OpenConnectorOptionsMenu()
{
	const FVector2D MouseCursorLocation = FSlateApplication::Get().GetCursorPos();

	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		BuildMenuWidgetConnector(),
		MouseCursorLocation,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
		);
}


void SMaterialEditorCanvas::UpdatePropertyWindow()
{
	TArray<UObject*> SelectedObjects;

	if( SelectedExpressions.Num() == 0 && SelectedComments.Num() == 0 )
	{
		UObject* EditObject = GetMaterial();
		UMaterialFunction* MaterialFunction = GetMaterialFunction();
		if (MaterialFunction)
		{
			EditObject = MaterialFunction;
		}

		SelectedObjects.Add(EditObject);
	}
	else
	{
		// Expressions
		for ( int32 Idx = 0 ; Idx < SelectedExpressions.Num() ; ++Idx )
		{
			SelectedObjects.Add( SelectedExpressions[Idx] );
		}

		// Comments
		for ( int32 Idx = 0 ; Idx < SelectedComments.Num() ; ++Idx )
		{
			SelectedObjects.Add( SelectedComments[Idx] );
		}
	}
	const auto& DetailView = MaterialEditorPtr.Pin()->GetDetailView();
	DetailView->SetObjects( SelectedObjects, true );
}

void SMaterialEditorCanvas::UpdatePreviewMaterial()
{
	UMaterial* Material = MaterialEditorPtr.Pin()->ExpressionPreviewMaterial;
	if( PreviewExpression && Material )
	{
		PreviewExpression->ConnectToPreviewMaterial(Material,0);
	}

	MaterialEditorPtr.Pin()->UpdatePreviewMaterial();
}

/**
 * Deselects all selected material expressions and comments.
 */
void SMaterialEditorCanvas::EmptySelection()
{
	SelectedExpressions.Empty();
	SelectedComments.Empty();
	bHomeNodeSelected = false;
}

/**
 * Add the specified object to the list of selected objects
 */
void SMaterialEditorCanvas::AddToSelection(UObject* Obj)
{
	bHomeNodeSelected = false;
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		SelectedComments.AddUnique( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		SelectedExpressions.AddUnique( static_cast<UMaterialExpression*>(Obj) );
	}
	else if( Obj == GetMaterial() || Obj == GetMaterialFunction())
	{
		bHomeNodeSelected = true;
	}
}

/**
 * Remove the specified object from the list of selected objects.
 */
void SMaterialEditorCanvas::RemoveFromSelection(UObject* Obj)
{
	bHomeNodeSelected = false;
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		SelectedComments.Remove( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		SelectedExpressions.Remove( static_cast<UMaterialExpression*>(Obj) );
	}
}

/**
 * Checks whether the specified object is currently selected.
 *
 * @return	true if the specified object is currently selected
 */
bool SMaterialEditorCanvas::IsInSelection(UObject* Obj) const
{
	bool bIsInSelection = false;
	if( Obj->IsA(UMaterialExpressionComment::StaticClass()) )
	{
		bIsInSelection = SelectedComments.Contains( static_cast<UMaterialExpressionComment*>(Obj) );
	}
	else if( Obj->IsA(UMaterialExpression::StaticClass()) )
	{
		bIsInSelection = SelectedExpressions.Contains( static_cast<UMaterialExpression*>(Obj) );
	}
	return bIsInSelection;
}

/**
 * Returns the number of selected objects
 */
int32 SMaterialEditorCanvas::GetNumSelected() const
{
	return SelectedExpressions.Num() + SelectedComments.Num();
}

/**
 * Called when the user clicks on an unselected link connector.
 *
 * @param	Connector	the link connector that was clicked on
 */
void SMaterialEditorCanvas::SetSelectedConnector(FLinkedObjectConnector& Connector)
{
	ConnObj = Connector.ConnObj;
	ConnType = Connector.ConnType;
	ConnIndex = Connector.ConnIndex;
}

/**
 * Gets the position of the selected connector.
 *
 * @return	an FIntPoint that represents the position of the selected connector, or (0,0)
 *			if no connectors are currently selected
 */
FIntPoint SMaterialEditorCanvas::GetSelectedConnLocation(FCanvas* Canvas)
{
	if( ConnObj.IsValid() )
	{
		UMaterialExpression* ExpressionNode = Cast<UMaterialExpression>( ConnObj.Get() );

		if( ExpressionNode )
		{
			return GetExpressionConnectionLocation( ExpressionNode, ConnType, ConnIndex );
		}

		UMaterial* MaterialNode =Cast<UMaterial>( ConnObj.Get() );
		if( MaterialNode )
		{
			return GetMaterialConnectionLocation( MaterialNode, ConnType, ConnIndex );
		}
	}

	return FIntPoint(0,0);
}

/**
 * Gets the EConnectorHitProxyType for the currently selected connector
 *
 * @return	the type for the currently selected connector, or 0 if no connector is selected
 */
int32 SMaterialEditorCanvas::GetSelectedConnectorType()
{
	return ConnType;
}

/**
 * Makes a connection between connectors.
 */
void SMaterialEditorCanvas::MakeConnectionToConnector(FLinkedObjectConnector& Connector)
{
	MakeConnectionToConnector(Connector.ConnObj, Connector.ConnType, Connector.ConnIndex);
}

/** Makes a connection between the currently selected connector and the passed in end point. */
void SMaterialEditorCanvas::MakeConnectionToConnector(TWeakObjectPtr<UObject> EndObject, int32 EndConnType, int32 EndConnIndex)
{
	// Nothing to do if object pointers are NULL.
	if ( !EndObject.IsValid() || !ConnObj.IsValid() )
	{
		return;
	}

	// Avoid connections to yourself.
	if( EndObject == ConnObj )
	{
		if (ConnType == EndConnType)
		{
			//allow for moving a connection
		}
		else
		{
			return;
		}
	}

	bool bConnectionWasMade = false;

	UMaterial* EndConnMaterial						= Cast<UMaterial>( EndObject.Get() );
	UMaterialExpression* EndConnMaterialExpression	= Cast<UMaterialExpression>( EndObject.Get() );
	check( EndConnMaterial || EndConnMaterialExpression );

	UMaterial* ConnMaterial							= Cast<UMaterial>( ConnObj.Get() );
	UMaterialExpression* ConnMaterialExpression		= Cast<UMaterialExpression>( ConnObj.Get() );
	check( ConnMaterial || ConnMaterialExpression );

	// Material to . . .
	if ( ConnMaterial )
	{
		// Materials are only connected along their right side.
		check( ConnType == LOC_OUTPUT ); 

		// Material to expression.
		if ( EndConnMaterialExpression )
		{
			// Material expressions can only connect to their materials on their left side.
			if ( EndConnType == LOC_INPUT ) 
			{
				const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );
				GetMaterial()->Modify();
				ConnectMaterialToMaterialExpression( ConnMaterial, ConnIndex, EndConnMaterialExpression, EndConnIndex, bHideUnusedConnectors );
				bConnectionWasMade = true;
			}
		}
	}
	// Expression to . . .
	else if ( ConnMaterialExpression )
	{
		// Expression to expression.
		if ( EndConnMaterialExpression )
		{
			if ( ConnType != EndConnType )
			{
				UMaterialExpression* InputExpression;
				UMaterialExpression* OutputExpression;
				int32 InputIndex;
				int32 OutputIndex;

				if( ConnType == LOC_OUTPUT && EndConnType == LOC_INPUT )
				{
					InputExpression = ConnMaterialExpression;
					InputIndex = ConnIndex;
					OutputExpression = EndConnMaterialExpression;
					OutputIndex = EndConnIndex;
				}
				else
				{
					InputExpression = EndConnMaterialExpression;
					InputIndex = EndConnIndex;
					OutputExpression = ConnMaterialExpression;
					OutputIndex = ConnIndex;
				}

				// Input expression.				
				FExpressionInput* ExpressionInput = InputExpression->GetInput(InputIndex);

				// Output expression.
				TArray<FExpressionOutput>& Outputs = OutputExpression->GetOutputs();
				if (Outputs.Num() > 0)
				{
					const FExpressionOutput& ExpressionOutput = Outputs[ OutputIndex ];

					const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );
					InputExpression->Modify();
					ConnectExpressions( *ExpressionInput, ExpressionOutput, OutputIndex, OutputExpression );

					bConnectionWasMade = true;
				}
			}
			else if (ConnType == LOC_INPUT && EndConnType == LOC_INPUT)
			{
				//if we are attempting to move outbound (leftside) connections from one connection to another
				UMaterialExpression* SourceMaterialExpression = ConnMaterialExpression;
				UMaterialExpression* DestMaterialExpression = EndConnMaterialExpression;

				//get the output from our SOURCE
				TArray<FExpressionOutput>& SourceOutputs = SourceMaterialExpression->GetOutputs();
				int32 SourceIndex = ConnIndex;
				FExpressionOutput& SourceExpressionOutput = SourceOutputs[ SourceIndex ];

				//get a list of inputs that reference the SOURCE output
				TArray<FMaterialExpressionReference> ReferencingInputs;
				GetListOfReferencingInputs( SourceMaterialExpression, GetMaterial(), ReferencingInputs, &SourceExpressionOutput,SourceIndex );
				if (ReferencingInputs.Num())
				{
					const FScopedTransaction MoveConnectionTransaction( NSLOCTEXT("UnrealEd", "MaterialEditorMoveConnections", "Material Editor: Move Connections") );
					for ( int32 RefIndex = 0; RefIndex < ReferencingInputs.Num(); RefIndex++ )
					{
						if (ReferencingInputs[RefIndex].Expression)
						{
							ReferencingInputs[RefIndex].Expression->Modify();
						}
						else
						{
							GetMaterial()->Modify();
						}

						//Get the inputs associated with this reference...
						FMaterialExpressionReference &InputExpressionReference = ReferencingInputs[RefIndex];
						for ( int32 InputIndex = 0; InputIndex < InputExpressionReference.Inputs.Num(); InputIndex++ )
						{
							FExpressionInput* Input = InputExpressionReference.Inputs[InputIndex];
							check( Input->Expression == SourceMaterialExpression );	
							
							if (InputExpressionReference.Expression != DestMaterialExpression) //ensure we're not connecting to ourselves
							{
								//break the existing link				
								Input->Expression = NULL;

								//connect to our DEST
								TArray<FExpressionOutput>& DestOutputs = DestMaterialExpression->GetOutputs();
								int32 DestIndex = EndConnIndex;
								FExpressionOutput& DestExpressionOutput = DestOutputs[ DestIndex ];

								const FScopedTransaction MakeConnectionTransaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );
								DestMaterialExpression->Modify();
								ConnectExpressions( *Input, DestExpressionOutput, DestIndex, DestMaterialExpression );

								bConnectionWasMade = true;
							}
						}
					}
				}
			}
			else if (ConnType == LOC_OUTPUT && EndConnType == LOC_OUTPUT)
			{
				//if we are attempting to move inbound (rightside) connections from one connection to another 
				UMaterialExpression* SourceMaterialExpression = ConnMaterialExpression;
				UMaterialExpression* DestMaterialExpression = EndConnMaterialExpression;
				int32 SourceIndex = ConnIndex;
				int32 DestIndex = EndConnIndex;

				//get the input from our SOURCE
				FExpressionInput* SourceExpressionInput = SourceMaterialExpression->GetInput(SourceIndex);

				if (SourceExpressionInput->Expression && SourceExpressionInput->Expression != DestMaterialExpression)
				{
					const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialMoveConnections", "Material Editor: Move Connection") );
					SourceMaterialExpression->Modify();
					DestMaterialExpression->Modify();
					
					//get the connected output
					int32 OutputIndex = SourceExpressionInput->OutputIndex;
					TArray<FExpressionOutput>& Outputs = SourceExpressionInput->Expression->GetOutputs();
					check(Outputs.Num() > 0);
					FExpressionOutput& ExpressionOutput = Outputs[ OutputIndex ];
						
					//get the DEST input
					FExpressionInput* DestExpressionInput = DestMaterialExpression->GetInput(DestIndex);
					
					//make the new connection
					ConnectExpressions(*DestExpressionInput, ExpressionOutput, OutputIndex, SourceExpressionInput->Expression);
					
					//break the old connections
					SourceExpressionInput->Expression = NULL;

					bConnectionWasMade = true;
				}
											
			}
			
		}
		// Expression to Material.
		else if ( EndConnMaterial )
		{
			// Materials are only connected along their right side.
			check( EndConnType == LOC_OUTPUT ); 

			// Material expressions can only connect to their materials on their left side.
			if ( ConnType == LOC_INPUT ) 
			{
				const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );
				GetMaterial()->Modify();
				ConnectMaterialToMaterialExpression( EndConnMaterial, EndConnIndex, ConnMaterialExpression, ConnIndex, bHideUnusedConnectors );
				bConnectionWasMade = true;
			}
		}
	}
	
	if ( bConnectionWasMade )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();

		GetMaterial()->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

/**
 * Breaks the link currently indicated by ConnObj/ConnType/ConnIndex.
 */
void SMaterialEditorCanvas::BreakConnLink(bool bOnlyIfMouseOver)
{
	if ( !ConnObj.IsValid() )
	{
		return;
	}

	bool bConnectionWasBroken = false;

	UMaterialExpression* MaterialExpression	= Cast<UMaterialExpression>( ConnObj.Get() );
	UMaterial* MaterialNode					= Cast<UMaterial>( ConnObj.Get() );

	if ( ConnType == LOC_OUTPUT ) // right side
	{
		if (MaterialNode && (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialNode, ConnType, ConnIndex)))
		{
			// Clear the material input.
			FExpressionInput* MaterialInput = GetMaterialInputConditional( MaterialNode, ConnIndex, bHideUnusedConnectors );
			if ( MaterialInput->Expression != NULL )
			{
				const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorBreakConnection", "Material Editor: Break Connection") );
				GetMaterial()->Modify();
				MaterialInput->Expression = NULL;
				MaterialInput->OutputIndex = INDEX_NONE;

				bConnectionWasBroken = true;
			}
		}
		else if (MaterialExpression && (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialExpression, ConnType, ConnIndex)))
		{
			FExpressionInput* Input = MaterialExpression->GetInput(ConnIndex);

			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorBreakConnection", "Material Editor: Break Connection") );
			MaterialExpression->Modify();
			Input->Expression = NULL;
			Input->OutputIndex = INDEX_NONE;

			bConnectionWasBroken = true;
		}
	}
	else if ( ConnType == LOC_INPUT ) // left side
	{
		// Only expressions can have connectors on the left side.
		check( MaterialExpression );
		if (!bOnlyIfMouseOver || IsConnectorHighlighted(MaterialExpression, ConnType, ConnIndex))
		{
			TArray<FExpressionOutput>& Outputs = MaterialExpression->GetOutputs();
			const FExpressionOutput& ExpressionOutput = Outputs[ ConnIndex ];

			// Get a list of inputs that refer to the selected output.
			TArray<FMaterialExpressionReference> ReferencingInputs;
			GetListOfReferencingInputs( MaterialExpression, GetMaterial(), ReferencingInputs, &ExpressionOutput, ConnIndex );

			bConnectionWasBroken = ReferencingInputs.Num() > 0;
			if ( bConnectionWasBroken )
			{
				// Clear all referencing inputs.
				const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorBreakConnection", "Material Editor: Break Connection") );
				for ( int32 RefIndex = 0 ; RefIndex < ReferencingInputs.Num() ; ++RefIndex )
				{
					if (ReferencingInputs[RefIndex].Expression)
					{
						ReferencingInputs[RefIndex].Expression->Modify();
					}
					else
					{
						GetMaterial()->Modify();
					}
					for ( int32 InputIndex = 0 ; InputIndex < ReferencingInputs[RefIndex].Inputs.Num() ; ++InputIndex )
					{
						FExpressionInput* Input = ReferencingInputs[RefIndex].Inputs[InputIndex];
						check( Input->Expression == MaterialExpression );
						Input->Expression = NULL;
						Input->OutputIndex = INDEX_NONE;
					}
				}
			}
		}
	}

	if ( bConnectionWasBroken )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();

		GetMaterial()->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
		ConnObj = NULL;
	}
}

/** Updates the marked connector based on what's currently under the mouse, and potentially makes a new connection. */
void SMaterialEditorCanvas::UpdateMarkedConnector()
{
	if (LinkedObjVC->MouseOverObject == MarkedObject && LinkedObjVC->MouseOverConnType == MarkedConnType && LinkedObjVC->MouseOverConnIndex == MarkedConnIndex)
	{
		MarkedObject = NULL;
		MarkedConnType = INDEX_NONE;
		MarkedConnIndex = INDEX_NONE;
	}
	else
	{
		// Only make a connection if both ends are valid
		if (MarkedConnType >= 0 && MarkedConnIndex >= 0)
		{
			MakeConnectionToConnector(MarkedObject, MarkedConnType, MarkedConnIndex);
		}
		
		MarkedObject = LinkedObjVC->MouseOverObject;
		MarkedConnType = LinkedObjVC->MouseOverConnType;
		MarkedConnIndex = LinkedObjVC->MouseOverConnIndex;
	}
}

/**
 * Connects an expression output to the specified material input slot.
 *
 * @param	MaterialInputIndex		Index to the material input (Diffuse=0, Emissive=1, etc).
 */
void SMaterialEditorCanvas::OnConnectToMaterial(int32 MaterialInputIndex)
{
	// Checks if over expression connection.
	UMaterialExpression* MaterialExpression = NULL;
	if ( ConnObj.IsValid() )
	{
		MaterialExpression = Cast<UMaterialExpression>( ConnObj.Get() );
	}

	bool bConnectionWasMade = false;
	if ( MaterialExpression && ConnType == LOC_INPUT )
	{
		UMaterial* MaterialNode = Cast<UMaterial>( ConnObj.Get() );

		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );
		GetMaterial()->Modify();
		ConnectMaterialToMaterialExpression(GetMaterial(), MaterialInputIndex, MaterialExpression, ConnIndex, bHideUnusedConnectors);
		bConnectionWasMade = true;
	}

	if ( bConnectionWasMade && MaterialExpression )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();
		GetMaterial()->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

/**
 * Breaks all connections to/from selected material expressions.
 */
void SMaterialEditorCanvas::BreakAllConnectionsToSelectedExpressions()
{
	if ( SelectedExpressions.Num() > 0 || bHomeNodeSelected )
	{
		{
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorBreakAllConnections", "Material Editor: Break All Connections") );
			
			if (bHomeNodeSelected)
			{
				for (int32 MatInputIdx = 0; MatInputIdx < MaterialInputs.Num(); ++MatInputIdx)
				{
					const FMaterialInputInfo& MaterialInput = MaterialInputs[MatInputIdx];
					MaterialInput.Input->Expression = NULL;
					MaterialInput.Input->OutputIndex = INDEX_NONE;
				}
			}
			
			for( int32 ExpressionIndex = 0 ; ExpressionIndex < SelectedExpressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* MaterialExpression = SelectedExpressions[ ExpressionIndex ];
				MaterialExpression->Modify();

				// Break links to expression (outgoing links)
				SwapLinksToExpression(MaterialExpression, NULL, GetMaterial());
				
				// break links going in to the expression as well
				const TArray<FExpressionInput*>& Inputs = MaterialExpression->GetInputs();
				for (int32 InputIndex = 0; InputIndex < Inputs.Num(); InputIndex++)
				{
					FExpressionInput* CurrentInput = Inputs[InputIndex];
					if (CurrentInput->Expression)
					{
						CurrentInput->Expression = NULL;
						CurrentInput->OutputIndex = INDEX_NONE;
					}
				}
			}
		}
	
		// Update the current preview material. 
		UpdatePreviewMaterial();
		GetMaterial()->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

/** Removes the selected expression from the favorites list. */
void SMaterialEditorCanvas::RemoveSelectedExpressionFromFavorites()
{
	if ( SelectedExpressions.Num() == 1 )
	{
		UMaterialExpression* MaterialExpression = SelectedExpressions[0];
		if (MaterialExpression)
		{
			MaterialExpressionClasses::Get()->RemoveMaterialExpressionFromFavorites(MaterialExpression->GetClass());
			MaterialEditorPtr.Pin()->EditorOptions->FavoriteExpressions.Remove(MaterialExpression->GetClass()->GetName());
			MaterialEditorPtr.Pin()->EditorOptions->SaveConfig();
		}
	}
}

/** Adds the selected expression to the favorites list. */
void SMaterialEditorCanvas::AddSelectedExpressionToFavorites()
{
	if ( SelectedExpressions.Num() == 1 )
	{
		UMaterialExpression* MaterialExpression = SelectedExpressions[0];
		if (MaterialExpression)
		{
			MaterialExpressionClasses::Get()->AddMaterialExpressionToFavorites(MaterialExpression->GetClass());
			MaterialEditorPtr.Pin()->EditorOptions->FavoriteExpressions.AddUnique(MaterialExpression->GetClass()->GetName());
			MaterialEditorPtr.Pin()->EditorOptions->SaveConfig();
		}
	}
}

/**
 * Called when the user releases the mouse over a link connector and is holding the ALT key.
 * Commonly used as a shortcut to breaking connections.
 *
 * @param	Connector	The connector that was ALT+clicked upon.
 */
void SMaterialEditorCanvas::AltClickConnector(FLinkedObjectConnector& Connector)
{
	BreakConnLink(false);
}

/**
 * Called when the user performs a draw operation while objects are selected.
 *
 * @param	DeltaX	the X value to move the objects
 * @param	DeltaY	the Y value to move the objects
 */
void SMaterialEditorCanvas::MoveSelectedObjects(int32 DeltaX, int32 DeltaY)
{
	const bool bFirstMove = LinkedObjVC->DistanceDragged < 4;
	if ( bFirstMove )
	{
		ExpressionsLinkedToComments.Empty();
	}

	TArray<UMaterialExpression*> ExpressionsToMove;

	// Add selected expressions.
	for( int32 ExpressionIndex = 0 ; ExpressionIndex < SelectedExpressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* Expression = SelectedExpressions[ExpressionIndex];
		ExpressionsToMove.Add( Expression );
	}

	if ( SelectedComments.Num() > 0 )
	{
		TArray<FIntRect> CommentRects;

		// Add selected comments.  At the same time, create rects for the comments.
		for( int32 CommentIndex = 0 ; CommentIndex < SelectedComments.Num() ; ++CommentIndex )
		{
			UMaterialExpressionComment* Comment = SelectedComments[CommentIndex];
			ExpressionsToMove.Add( Comment );
			CommentRects.Add( FIntRect( FIntPoint(Comment->MaterialExpressionEditorX, Comment->MaterialExpressionEditorY),
											FIntPoint(Comment->MaterialExpressionEditorX+Comment->SizeX, Comment->MaterialExpressionEditorY+Comment->SizeY) ) );
		}

		// If this is the first move, generate a list of expressions that are contained by comments.
		if ( bFirstMove )
		{
			for( int32 ExpressionIndex = 0 ; ExpressionIndex < GetMaterial()->Expressions.Num() ; ++ExpressionIndex )
			{
				UMaterialExpression* Expression = GetMaterial()->Expressions[ExpressionIndex];
				const FIntPoint ExpressionPos( Expression->MaterialExpressionEditorX, Expression->MaterialExpressionEditorY );
				for( int32 CommentIndex = 0 ; CommentIndex < CommentRects.Num() ; ++CommentIndex )
				{
					const FIntRect& CommentRect = CommentRects[CommentIndex];
					if ( CommentRect.Contains(ExpressionPos) )
					{
						ExpressionsLinkedToComments.AddUnique( Expression );
						break;
					}
				}
			}

			// Also check comments to see if they are contained by other comments which are selected
			for ( int32 CommentIndex = 0; CommentIndex < GetMaterial()->EditorComments.Num(); ++CommentIndex )
			{
				UMaterialExpressionComment* CurComment = GetMaterial()->EditorComments[ CommentIndex ];
				const FIntPoint ExpressionPos( CurComment->MaterialExpressionEditorX, CurComment->MaterialExpressionEditorY );
				for ( int32 CommentRectIndex = 0; CommentRectIndex < CommentRects.Num(); ++CommentRectIndex )
				{
					const FIntRect& CommentRect = CommentRects[CommentRectIndex];
					if ( CommentRect.Contains( ExpressionPos ) )
					{
						ExpressionsLinkedToComments.AddUnique( CurComment );
						break;
					}
				}

			}
		}

		// Merge the expression lists.
		for( int32 ExpressionIndex = 0 ; ExpressionIndex < ExpressionsLinkedToComments.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* Expression = ExpressionsLinkedToComments[ExpressionIndex];
			ExpressionsToMove.AddUnique( Expression );
		}
	}

	// Perform the move.
	if ( ExpressionsToMove.Num() > 0 )
	{
		for( int32 ExpressionIndex = 0 ; ExpressionIndex < ExpressionsToMove.Num() ; ++ExpressionIndex )
		{
			UMaterialExpression* Expression = ExpressionsToMove[ExpressionIndex];
			Expression->MaterialExpressionEditorX += DeltaX;
			Expression->MaterialExpressionEditorY += DeltaY;
		}
		GetMaterial()->MarkPackageDirty();
		SetMaterialDirty();
	}
}

bool SMaterialEditorCanvas::EdHandleKeyInput(FViewport* InViewport, FKey Key, EInputEvent Event)
{
	bool bHandled = true;
	const bool bCtrlDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
	if( Event == IE_Pressed )
	{
		if ( bCtrlDown )
		{
			if( Key == EKeys::C )
			{
				// Clear the material copy buffer and copy the selected expressions into it.
				TArray<UMaterialExpression*> NewExpressions;
				TArray<UMaterialExpression*> NewComments;
				GetCopyPasteBuffer()->Expressions.Empty();
				GetCopyPasteBuffer()->EditorComments.Empty();

				UMaterialExpression::CopyMaterialExpressions( SelectedExpressions, SelectedComments, GetCopyPasteBuffer(), GetMaterialFunction(), NewExpressions, NewComments );
			}
			else if ( Key == EKeys::V )
			{
				// Paste the material copy buffer into this material.
				PasteExpressionsIntoMaterial( NULL );
			}
			else if( Key == EKeys::W )
			{
				DuplicateSelectedObjects();
			}
			else if( Key == EKeys::Y )
			{
				Redo();
			}
			else if( Key == EKeys::Z )
			{
				Undo();
			}
			else
			{
				bHandled = false;
			}
		}
		else
		{
			if( Key == EKeys::Delete )
			{
				DeleteSelectedObjects();
			}
			else if ( Key == EKeys::SpaceBar )
			{
				// Spacebar force-refreshes previews.
				ForceRefreshExpressionPreviews();
				RefreshExpressionViewport();
				MaterialEditorPtr.Pin()->RefreshPreviewViewport();
			}
			else
			{
				bHandled = false;
			}
		}

		if ( Key == EKeys::Enter )
		{
			if (MaterialEditorPtr.Pin()->bMaterialDirty)
			{
				MaterialEditorPtr.Pin()->UpdateOriginalMaterial();
				bHandled = true;
			}
		}
	}
	else if ( Event == IE_Released )
	{
		// LMBRelease + hotkey places certain material expression types.
		if( Key == EKeys::LeftMouseButton )
		{
			const bool bShiftDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);
			if( bShiftDown && InViewport->KeyState(EKeys::C))
			{
				CreateNewCommentExpression();
			}
			else if ( bCtrlDown )
			{
				BreakConnLink(true);
			}
			else if ( bShiftDown )
			{
				UpdateMarkedConnector();
			}
			else if (LinkedObjVC->DistanceDragged < 4)
			{
				{
					// Check if the "Toggle Collapsed" icon was clicked on.
					InViewport->Invalidate();
					const int32 HitX = InViewport->GetMouseX();
					const int32 HitY = InViewport->GetMouseY();
					HHitProxy*	HitResult = InViewport->GetHitProxy( HitX, HitY );
					if ( HitResult && HitResult->IsA( HCollapsedProxy::StaticGetType() ) )
					{
						// Toggle the material expression's realtime preview state.
						UMaterialExpression* MaterialExpression = static_cast<HCollapsedProxy*>( HitResult )->MaterialExpression;
						check( MaterialExpression );
						{
							const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorToggleCollapsed", "Material Editor: Toggle Collapsed") );
							MaterialExpression->Modify();
							MaterialExpression->bCollapsed = !MaterialExpression->bCollapsed;
						}
						MaterialExpression->PreEditChange( NULL );
						MaterialExpression->PostEditChange();
						MaterialExpression->MarkPackageDirty();
						SetMaterialDirty();

						// Update the preview.
						RefreshExpressionPreview( MaterialExpression, true );
						MaterialEditorPtr.Pin()->RefreshPreviewViewport();

						// Clear the double click selection next time the mouse is clicked
						DblClickObject = NULL;
						DblClickConnType = INDEX_NONE;
						DblClickConnIndex = INDEX_NONE;
					}
				}

				struct FMaterialExpressionHotkey
				{
					FKey		Key;
					UClass*		MaterialExpressionClass;
				};
				static FMaterialExpressionHotkey MaterialExpressionHotkeys[] =
				{
					{ EKeys::A, UMaterialExpressionAdd::StaticClass() },
					{ EKeys::B, UMaterialExpressionBumpOffset::StaticClass() },
					{ EKeys::C, UMaterialExpressionComponentMask::StaticClass() },
					{ EKeys::D, UMaterialExpressionDivide::StaticClass() },
					{ EKeys::E, UMaterialExpressionPower::StaticClass() },
					{ EKeys::F, UMaterialExpressionMaterialFunctionCall::StaticClass() },
					{ EKeys::I, UMaterialExpressionIf::StaticClass() },
					{ EKeys::L, UMaterialExpressionLinearInterpolate::StaticClass() },
					{ EKeys::M, UMaterialExpressionMultiply::StaticClass() },
					{ EKeys::N, UMaterialExpressionNormalize::StaticClass() },
					{ EKeys::O, UMaterialExpressionOneMinus::StaticClass() },
					{ EKeys::P, UMaterialExpressionPanner::StaticClass() },
					{ EKeys::R, UMaterialExpressionReflectionVectorWS::StaticClass() },
					{ EKeys::S, UMaterialExpressionScalarParameter::StaticClass() },
					{ EKeys::T, UMaterialExpressionTextureSample::StaticClass() },
					{ EKeys::U, UMaterialExpressionTextureCoordinate::StaticClass() },
					{ EKeys::V, UMaterialExpressionVectorParameter::StaticClass() },
					{ EKeys::One, UMaterialExpressionConstant::StaticClass() },
					{ EKeys::Two, UMaterialExpressionConstant2Vector::StaticClass() },
					{ EKeys::Three, UMaterialExpressionConstant3Vector::StaticClass() },
					{ EKeys::Four, UMaterialExpressionConstant4Vector::StaticClass() },
					{ EKeys::Invalid, NULL },
				};

				// Iterate over all expression hotkeys, reporting the first expression that has a key down.
				UClass* NewMaterialExpressionClass = NULL;
				for ( int32 Index = 0 ; ; ++Index )
				{
					const FMaterialExpressionHotkey& ExpressionHotKey = MaterialExpressionHotkeys[Index];
					if ( ExpressionHotKey.MaterialExpressionClass )
					{
						if ( InViewport->KeyState(ExpressionHotKey.Key) )
						{
							NewMaterialExpressionClass = ExpressionHotKey.MaterialExpressionClass;
							break;
						}
					}
					else
					{
						// A NULL MaterialExpressionClass indicates end of list.
						break;
					}
				}

				// Create a new material expression if the associated hotkey was found to be down.
				if ( NewMaterialExpressionClass )
				{
					const int32 LocX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
					const int32 LocY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
					CreateNewMaterialExpression( NewMaterialExpressionClass, false, true, true, FIntPoint(LocX, LocY) );
				}
			}
			else
			{
				bHandled = false;
			}
		}
		else if ( Key == EKeys::RightMouseButton )
		{
			// Clear the double click selection next time the mouse is clicked
			DblClickObject = NULL;
			DblClickConnType = INDEX_NONE;
			DblClickConnIndex = INDEX_NONE;
		}
		else
		{
			bHandled = false;
		}
	}
	else
	{
		bHandled = false;
	}

	return bHandled;
}

/**
 * Called once the user begins a drag operation.  Transactions expression and comment position.
 */
void SMaterialEditorCanvas::BeginTransactionOnSelected()
{
	check( !ScopedTransaction );
	ScopedTransaction = new FScopedTransaction( NSLOCTEXT("UnrealEd", "LinkedObjectModify", "LinkedObject Modify") );

	GetMaterial()->Modify();
	for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < GetMaterial()->Expressions.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpression* MaterialExpression = GetMaterial()->Expressions[ MaterialExpressionIndex ];
		MaterialExpression->Modify();
	}
	for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < GetMaterial()->EditorComments.Num() ; ++MaterialExpressionIndex )
	{
		UMaterialExpressionComment* Comment = GetMaterial()->EditorComments[ MaterialExpressionIndex ];
		Comment->Modify();
	}
}

/**
 * Called when the user releases the mouse button while a drag operation is active.
 */
void SMaterialEditorCanvas::EndTransactionOnSelected()
{
	check( ScopedTransaction );
	delete ScopedTransaction;
	ScopedTransaction = NULL;
}


/**
 *	Handling for dragging on 'special' hit proxies. Should only have 1 object selected when this is being called. 
 *	In this case used for handling resizing handles on Comment objects. 
 *
 *	@param	DeltaX			Horizontal drag distance this frame (in pixels)
 *	@param	DeltaY			Vertical drag distance this frame (in pixels)
 *	@param	SpecialIndex	Used to indicate different classes of action. @see HLinkedObjProxySpecial.
 */
void SMaterialEditorCanvas::SpecialDrag( int32 DeltaX, int32 DeltaY, int32 NewX, int32 NewY, int32 SpecialIndex )
{
	// Can only 'special drag' one object at a time.
	if( SelectedComments.Num() == 1 )
	{
		if ( SpecialIndex == 1 )
		{
			// Apply dragging to the comment size.
			UMaterialExpressionComment* Comment = SelectedComments[0];
			Comment->SizeX += DeltaX;
			Comment->SizeX = FMath::Max<int32>(Comment->SizeX, 64);

			Comment->SizeY += DeltaY;
			Comment->SizeY = FMath::Max<int32>(Comment->SizeY, 64);
			Comment->MarkPackageDirty();
			SetMaterialDirty();
		}
	}
}

void SMaterialEditorCanvas::PostUndo( bool bSuccess )
{
	EmptySelection();

	GetMaterial()->BuildEditorParameterList();

	UpdateSearch(false);

	// Update the current preview material.
	UpdatePreviewMaterial();

	UpdatePropertyWindow();

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	SetMaterialDirty();
}

/**
 * Uses the global Undo transactor to reverse changes, update viewports etc.
 */
void SMaterialEditorCanvas::Undo()
{
	EmptySelection();

	int32 NumExpressions = GetMaterial()->Expressions.Num();
	GEditor->UndoTransaction();

	if(NumExpressions != GetMaterial()->Expressions.Num())
	{
		GetMaterial()->BuildEditorParameterList();
	}

	UpdateSearch(false);

	// Update the current preview material.
	UpdatePreviewMaterial();

	UpdatePropertyWindow();

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	SetMaterialDirty();
}

/**
 * Uses the global Undo transactor to redo changes, update viewports etc.
 */
void SMaterialEditorCanvas::Redo()
{
	EmptySelection();

	int32 NumExpressions = GetMaterial()->Expressions.Num();
	GEditor->RedoTransaction();

	if(NumExpressions != GetMaterial()->Expressions.Num())
	{
		GetMaterial()->BuildEditorParameterList();
	}

	// Update the current preview material.
	UpdatePreviewMaterial();

	UpdatePropertyWindow();

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	SetMaterialDirty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FGCObject interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SMaterialEditorCanvas::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( int32 Index = 0; Index < SelectedExpressions.Num(); Index++ )
	{
		Collector.AddReferencedObject( SelectedExpressions[ Index ] );
	}
	for( int32 Index = 0; Index < SelectedComments.Num(); Index++ )
	{
		Collector.AddReferencedObject( SelectedComments[ Index ] );
	}
	for( int32 Index = 0; Index < ExpressionPreviews.Num(); Index++ )
	{
		ExpressionPreviews[ Index ].AddReferencedObjects( Collector );
	}
	for( int32 Index = 0; Index < ExpressionsLinkedToComments.Num(); Index++ )
	{
		Collector.AddReferencedObject( ExpressionsLinkedToComments[ Index ] );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FNotifyHook interface
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SMaterialEditorCanvas::NotifyPreChange(UProperty* PropertyAboutToChange)
{
	check( !ScopedTransaction );
	ScopedTransaction = new FScopedTransaction( NSLOCTEXT("UnrealEd", "MaterialEditorEditProperties", "Material Editor: Edit Properties") );
	FlushRenderingCommands();
}

void SMaterialEditorCanvas::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	check( ScopedTransaction );
	
	if ( PropertyThatChanged )
	{
		const FName NameOfPropertyThatChanged( *PropertyThatChanged->GetName() );
		if ( NameOfPropertyThatChanged == FName(TEXT("PreviewMesh")) ||
			 NameOfPropertyThatChanged == FName(TEXT("bUsedWithSkeletalMesh")) )
		{
			if ( MaterialEditorPtr.IsValid() )
			{
				TSharedRef<FMaterialEditor> LocalMaterialEditor = MaterialEditorPtr.Pin().ToSharedRef();
				LocalMaterialEditor->OriginalMaterial->PreviewMesh = GetMaterial()->PreviewMesh;
				FMaterialEditor::UpdateThumbnailInfoPreviewMesh(LocalMaterialEditor->OriginalMaterial);

				// SetPreviewMesh will return false if the material has bUsedWithSkeletalMesh and
				// a skeleton was requested, in which case revert to a sphere static mesh.
				if ( !LocalMaterialEditor->SetPreviewMesh( *GetMaterial()->PreviewMesh.AssetLongPathname ) )
				{
					LocalMaterialEditor->SetPreviewMesh( GUnrealEd->GetThumbnailManager()->EditorSphere, NULL );
				}
			}
		}
		
		for (int32 i = 0; i < SelectedExpressions.Num(); ++i)
		{
			auto Expression = SelectedExpressions[i];
			if(Expression)
			{
				if(NameOfPropertyThatChanged == FName(TEXT("ParameterName")))
				{
					GetMaterial()->UpdateExpressionParameterName(Expression);
				}
				else if (NameOfPropertyThatChanged == FName(TEXT("ParamNames")))
				{
					GetMaterial()->UpdateExpressionDynamicParameterNames(Expression);
				}
				else
				{
					GetMaterial()->PropagateExpressionParameterChanges(Expression);
				}
			}
		}
	}

	// Prevent constant recompilation of materials while properties are being interacted with
	if( PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		// Update the current preview material.
		UpdatePreviewMaterial();

		UpdateSearch(false);

		RefreshExpressionPreviews();
		RefreshSourceWindowMaterial();
	
	}

	delete ScopedTransaction;
	ScopedTransaction = NULL;

	GetMaterial()->MarkPackageDirty();
	SetMaterialDirty();
}


/** 
 * Computes a bounding box for the selected material expressions.  Output is not sensible if no expressions are selected.
 */
FIntRect SMaterialEditorCanvas::GetBoundingBoxOfSelectedExpressions()
{
	return GetBoundingBoxOfExpressions( SelectedExpressions );
}

/** 
 * Computes a bounding box for the specified set of material expressions.  Output is not sensible if the set is empty.
 */
FIntRect SMaterialEditorCanvas::GetBoundingBoxOfExpressions(const TArray<UMaterialExpression*>& Expressions)
{
	FIntRect Result(0, 0, 0, 0);
	bool bResultValid = false;

	for( int32 ExpressionIndex = 0 ; ExpressionIndex < Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpression* Expression = Expressions[ExpressionIndex];
		// @todo DB: Remove these hardcoded extents.
		const FIntRect ObjBox( Expression->MaterialExpressionEditorX-30, Expression->MaterialExpressionEditorY-20, Expression->MaterialExpressionEditorX+150, Expression->MaterialExpressionEditorY+150 );

		if( bResultValid )
		{
			// Expand result box to encompass
			Result.Min.X = FMath::Min(Result.Min.X, ObjBox.Min.X);
			Result.Min.Y = FMath::Min(Result.Min.Y, ObjBox.Min.Y);

			Result.Max.X = FMath::Max(Result.Max.X, ObjBox.Max.X);
			Result.Max.Y = FMath::Max(Result.Max.Y, ObjBox.Max.Y);
		}
		else
		{
			// Use this objects bounding box to initialise result.
			Result = ObjBox;
			bResultValid = true;
		}
	}

	return Result;
}

/**
 * Creates a new material expression comment frame.
 */
void SMaterialEditorCanvas::CreateNewCommentExpression()
{
	TSharedRef<STextEntryPopup> TextEntry = 
		SNew(STextEntryPopup)
		.Label(LOCTEXT("NewComment", "Comment"))
		.OnTextCommitted(this, &SMaterialEditorCanvas::NewCommentCommitted)
		.ClearKeyboardFocusOnCommit( false );
	
	NameEntryPopupWindow = FSlateApplication::Get().PushMenu(
		AsShared(),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SMaterialEditorCanvas::NewCommentCommitted(const FText& CommentText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		UMaterialExpressionComment* NewComment = NULL;
		{
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorNewComment", "Material Editor: New Comment") );
			GetMaterial()->Modify();

			UObject* ExpressionOuter = GetMaterial();
			if (GetMaterialFunction())
			{
				ExpressionOuter = GetMaterialFunction();
			}

			NewComment = ConstructObject<UMaterialExpressionComment>( UMaterialExpressionComment::StaticClass(), ExpressionOuter, NAME_None, RF_Transactional );

			// Add to the list of comments associated with this material.
			GetMaterial()->EditorComments.Add( NewComment );

			if ( SelectedExpressions.Num() > 0 )
			{
				const FIntRect SelectedBounds = GetBoundingBoxOfSelectedExpressions();
				NewComment->MaterialExpressionEditorX = SelectedBounds.Min.X;
				NewComment->MaterialExpressionEditorY = SelectedBounds.Min.Y;
				NewComment->SizeX = SelectedBounds.Max.X - SelectedBounds.Min.X;
				NewComment->SizeY = SelectedBounds.Max.Y - SelectedBounds.Min.Y;
			}
			else
			{

				NewComment->MaterialExpressionEditorX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
				NewComment->MaterialExpressionEditorY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
				NewComment->SizeX = 128;
				NewComment->SizeY = 128;
			}

			NewComment->Text = CommentText.ToString();
		}

		// Select the new comment.
		EmptySelection();
		AddToSelection( NewComment );

		GetMaterial()->MarkPackageDirty();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}

	if (NameEntryPopupWindow.IsValid())
	{
		NameEntryPopupWindow->RequestDestroyWindow();
	}
}

UMaterialExpression* SMaterialEditorCanvas::CreateNewMaterialExpression(UClass* NewExpressionClass, bool bAutoConnect, bool bAutoSelect, bool bAutoAssignResource, const FIntPoint& NodePos)
{
	check( NewExpressionClass->IsChildOf(UMaterialExpression::StaticClass()) );

	if (!IsAllowedExpressionType(NewExpressionClass, GetMaterialFunction() != NULL))
	{
		// Disallowed types should not be visible to the ui to be placed, so we don't need a warning here
		return NULL;
	}

	// Clear the selection
	if ( bAutoSelect )
	{
		EmptySelection();
	}

	// Create the new expression.
	UMaterialExpression* NewExpression = NULL;
	{
		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorNewExpression", "Material Editor: New Expression") );
		GetMaterial()->Modify();

		UObject* ExpressionOuter = GetMaterial();
		if (GetMaterialFunction())
		{
			ExpressionOuter = GetMaterialFunction();
		}

		NewExpression = ConstructObject<UMaterialExpression>( NewExpressionClass, ExpressionOuter, NAME_None, RF_Transactional );
		GetMaterial()->Expressions.Add( NewExpression );
		NewExpression->Material = GetMaterial();

		if (GetMaterialFunction() != NULL)
		{
			// Parameters currently not supported in material functions
			check(!NewExpression->bIsParameterExpression);
		}

		// If the new expression is created connected to an input tab, offset it by this amount.
		int32 NewConnectionOffset = 0;

		if (bAutoConnect)
		{
			// If an input tab was clicked on, bind the new expression to that tab.
			if ( ConnType == LOC_OUTPUT && ConnObj.IsValid() )
			{
				UMaterial* ConnMaterial							= Cast<UMaterial>( ConnObj.Get() );
				UMaterialExpression* ConnMaterialExpression		= Cast<UMaterialExpression>( ConnObj.Get() );
				check( ConnMaterial  || ConnMaterialExpression );

				if ( ConnMaterial )
				{
					ConnectMaterialToMaterialExpression( ConnMaterial, ConnIndex, NewExpression, 0, bHideUnusedConnectors );
				}
				else if ( ConnMaterialExpression )
				{
					UMaterialExpression* InputExpression = ConnMaterialExpression;
					UMaterialExpression* OutputExpression = NewExpression;

					int32 InputIndex = ConnIndex;
					int32 OutputIndex = 0;

					// Input expression.
					FExpressionInput* ExpressionInput = InputExpression->GetInput(InputIndex);

					// Output expression.
					TArray<FExpressionOutput>& Outputs = OutputExpression->GetOutputs();
					const FExpressionOutput& ExpressionOutput = Outputs[ OutputIndex ];

					InputExpression->Modify();
					ConnectExpressions( *ExpressionInput, ExpressionOutput, OutputIndex, OutputExpression );
				}

				// Offset the new expression it by this amount.
				NewConnectionOffset = 30;
			}
		}

		// Set the expression location.
		NewExpression->MaterialExpressionEditorX = NodePos.X + NewConnectionOffset;
		NewExpression->MaterialExpressionEditorY = NodePos.Y + NewConnectionOffset;

		if (bAutoAssignResource)
		{
			// If the user is adding a texture, automatically assign the currently selected texture to it.
			UMaterialExpressionTextureBase* METextureBase = Cast<UMaterialExpressionTextureBase>( NewExpression );
			if( METextureBase )
			{
				FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
				UTexture* CurrentSelectedTexture = GEditor->GetSelectedObjects()->GetTop<UTexture>();
				if(CurrentSelectedTexture != NULL)
				{
					METextureBase->Texture = CurrentSelectedTexture;
				}
				METextureBase->AutoSetSampleType();
			}

			UMaterialExpressionMaterialFunctionCall* MEMaterialFunction = Cast<UMaterialExpressionMaterialFunctionCall>( NewExpression );
			if( MEMaterialFunction )
			{
				FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
				MEMaterialFunction->SetMaterialFunction(GetMaterialFunction(), NULL, GEditor->GetSelectedObjects()->GetTop<UMaterialFunction>());
			}

			UMaterialExpressionCollectionParameter* MECollectionParameter = Cast<UMaterialExpressionCollectionParameter>( NewExpression );
			if( MECollectionParameter )
			{
				FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
				MECollectionParameter->Collection = GEditor->GetSelectedObjects()->GetTop<UMaterialParameterCollection>();
			}
		}

		UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>( NewExpression );
		if( FunctionInput )
		{
			FunctionInput->ConditionallyGenerateId(true);
			FunctionInput->ValidateName();
		}

		UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>( NewExpression );
		if( FunctionOutput )
		{
			FunctionOutput->ConditionallyGenerateId(true);
			FunctionOutput->ValidateName();
		}

		NewExpression->UpdateParameterGuid(true, true);

		UMaterialExpressionTextureSampleParameter* TextureParameterExpression = Cast<UMaterialExpressionTextureSampleParameter>( NewExpression );
		if( TextureParameterExpression )
		{
			// Change the parameter's name on creation to mirror the object's name; this avoids issues of having colliding parameter
			// names and having the name left as "None"
			TextureParameterExpression->ParameterName = TextureParameterExpression->GetFName();
		}

		UMaterialExpressionComponentMask* ComponentMaskExpression = Cast<UMaterialExpressionComponentMask>( NewExpression );
		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		if( ComponentMaskExpression )
		{
			ComponentMaskExpression->R = true;
			ComponentMaskExpression->G = true;
		}

		UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskExpression = Cast<UMaterialExpressionStaticComponentMaskParameter>( NewExpression );
		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		if( StaticComponentMaskExpression )
		{
			StaticComponentMaskExpression->DefaultR = true;
		}

		UMaterialExpressionRotateAboutAxis* RotateAboutAxisExpression = Cast<UMaterialExpressionRotateAboutAxis>( NewExpression );
		if( RotateAboutAxisExpression )
		{
			// Create a default expression for the Position input
			UMaterialExpressionWorldPosition* WorldPositionExpression = ConstructObject<UMaterialExpressionWorldPosition>( UMaterialExpressionWorldPosition::StaticClass(), ExpressionOuter, NAME_None, RF_Transactional );
			GetMaterial()->Expressions.Add( WorldPositionExpression );
			WorldPositionExpression->Material = GetMaterial();
			RotateAboutAxisExpression->Position.Expression = WorldPositionExpression;
			WorldPositionExpression->MaterialExpressionEditorX = RotateAboutAxisExpression->MaterialExpressionEditorX + 250;
			WorldPositionExpression->MaterialExpressionEditorY = RotateAboutAxisExpression->MaterialExpressionEditorY + 73;
			if ( bAutoSelect )
			{
				AddToSelection( WorldPositionExpression );
			}
		}

		// Setup defaults for the most likely use case
		// Can't change default properties as that will affect existing content
		UMaterialExpressionTransformPosition* PositionTransform = Cast<UMaterialExpressionTransformPosition>(NewExpression);
		if (PositionTransform)
		{
			PositionTransform->TransformSourceType = TRANSFORMPOSSOURCE_Local;
			PositionTransform->TransformType = TRANSFORMPOSSOURCE_World;
		}

		GetMaterial()->AddExpressionParameter(NewExpression);
	}

	// Select the new node.
	if ( bAutoSelect )
	{
		AddToSelection( NewExpression );
	}

	RefreshSourceWindowMaterial();
	UpdateSearch(false);

	// Update the current preview material.
	UpdatePreviewMaterial();
	GetMaterial()->MarkPackageDirty();

	UpdatePropertyWindow();


	RefreshExpressionPreviews();
	RefreshExpressionViewport();
	SetMaterialDirty();
	return NewExpression;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Event handlers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SMaterialEditorCanvas::OnNewMaterialExpression(UClass* MaterialExpression)
{
	if (MaterialExpression)
	{
		const int32 LocX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
		const int32 LocY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;
		CreateNewMaterialExpression( MaterialExpression, true, true, true, FIntPoint(LocX, LocY) );
	}
}

void SMaterialEditorCanvas::OnNewComment()
{
	CreateNewCommentExpression();
}

void SMaterialEditorCanvas::OnUseCurrentTexture()
{
	// Set the currently selected texture in the generic browser
	// as the texture to use in all selected texture sample expressions.
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
	UTexture* SelectedTexture = GEditor->GetSelectedObjects()->GetTop<UTexture>();
	if ( SelectedTexture )
	{
		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "UseCurrentTexture", "Use Current Texture") );
		for( int32 MaterialExpressionIndex = 0 ; MaterialExpressionIndex < SelectedExpressions.Num() ; ++MaterialExpressionIndex )
		{
			UMaterialExpression* MaterialExpression = SelectedExpressions[ MaterialExpressionIndex ];
			if ( MaterialExpression->IsA(UMaterialExpressionTextureBase::StaticClass()) )
			{
				UMaterialExpressionTextureBase* TextureBase = static_cast<UMaterialExpressionTextureBase*>(MaterialExpression);
				TextureBase->Modify();
				TextureBase->Texture = SelectedTexture;
				TextureBase->AutoSetSampleType();
			}
		}

		// Update the current preview material. 
		UpdatePreviewMaterial();
		GetMaterial()->MarkPackageDirty();
		RefreshSourceWindowMaterial();
		UpdatePropertyWindow();
		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

void SMaterialEditorCanvas::OnDuplicateObjects()
{
	DuplicateSelectedObjects();
}

void SMaterialEditorCanvas::OnPasteHere()
{
	const FIntPoint ClickLocation( (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D, (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D );
	PasteExpressionsIntoMaterial( &ClickLocation );
}

void SMaterialEditorCanvas::OnConvertObjects()
{
	if (SelectedExpressions.Num() > 0)
	{
		const FScopedTransaction Transaction( LOCTEXT("MaterialEditorConvert", "Material Editor: Convert to Parameter") );
		GetMaterial()->Modify();
		TArray<UMaterialExpression*> ExpressionsToDelete;
		TArray<UMaterialExpression*> ExpressionsToSelect;
		for (int32 ExpressionIndex = 0; ExpressionIndex < SelectedExpressions.Num(); ++ExpressionIndex)
		{
			// Look for the supported classes to convert from
			UMaterialExpression* CurrentSelectedExpression = SelectedExpressions[ExpressionIndex];
			UMaterialExpressionConstant* Constant1Expression = Cast<UMaterialExpressionConstant>(CurrentSelectedExpression);
			UMaterialExpressionConstant2Vector* Constant2Expression = Cast<UMaterialExpressionConstant2Vector>(CurrentSelectedExpression);
			UMaterialExpressionConstant3Vector* Constant3Expression = Cast<UMaterialExpressionConstant3Vector>(CurrentSelectedExpression);
			UMaterialExpressionConstant4Vector* Constant4Expression = Cast<UMaterialExpressionConstant4Vector>(CurrentSelectedExpression);
			UMaterialExpressionTextureSample* TextureSampleExpression = Cast<UMaterialExpressionTextureSample>(CurrentSelectedExpression);
			UMaterialExpressionComponentMask* ComponentMaskExpression = Cast<UMaterialExpressionComponentMask>(CurrentSelectedExpression);
			UMaterialExpressionParticleSubUV* ParticleSubUVExpression = Cast<UMaterialExpressionParticleSubUV>(CurrentSelectedExpression);

			// Setup the class to convert to
			UClass* ClassToCreate = NULL;
			if (Constant1Expression)
			{
				ClassToCreate = UMaterialExpressionScalarParameter::StaticClass();
			}
			else if (Constant2Expression || Constant3Expression || Constant4Expression)
			{
				ClassToCreate = UMaterialExpressionVectorParameter::StaticClass();
			}
			else if (ParticleSubUVExpression) // Has to come before the TextureSample comparison...
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterSubUV::StaticClass();
			}
			else if (TextureSampleExpression && TextureSampleExpression->Texture && TextureSampleExpression->Texture->IsA(UTextureCube::StaticClass()))
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameterCube::StaticClass();
			}	
			else if (TextureSampleExpression)
			{
				ClassToCreate = UMaterialExpressionTextureSampleParameter2D::StaticClass();
			}	
			else if (ComponentMaskExpression)
			{
				ClassToCreate = UMaterialExpressionStaticComponentMaskParameter::StaticClass();
			}

			if (ClassToCreate)
			{
				UMaterialExpression* NewExpression = CreateNewMaterialExpression(ClassToCreate, false, false, true, FIntPoint(CurrentSelectedExpression->MaterialExpressionEditorX, CurrentSelectedExpression->MaterialExpressionEditorY));
				if (NewExpression)
				{
					SwapLinksToExpression(CurrentSelectedExpression, NewExpression, GetMaterial());
					bool bNeedsRefresh = false;

					// Copy over expression-specific values
					if (Constant1Expression)
					{
						bNeedsRefresh = true;
						CastChecked<UMaterialExpressionScalarParameter>(NewExpression)->DefaultValue = Constant1Expression->R;
					}
					else if (Constant2Expression)
					{
						bNeedsRefresh = true;
						CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = FLinearColor(Constant2Expression->R, Constant2Expression->G, 0);
					}
					else if (Constant3Expression)
					{
						bNeedsRefresh = true;
						CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = Constant3Expression->Constant;
						CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue.A = 1.0f;
					}
					else if (Constant4Expression)
					{
						bNeedsRefresh = true;
						CastChecked<UMaterialExpressionVectorParameter>(NewExpression)->DefaultValue = Constant4Expression->Constant;
					}
					else if (TextureSampleExpression)
					{
						bNeedsRefresh = true;
						UMaterialExpressionTextureSampleParameter* NewTextureExpr = CastChecked<UMaterialExpressionTextureSampleParameter>(NewExpression);
						NewTextureExpr->Texture = TextureSampleExpression->Texture;
						NewTextureExpr->Coordinates = TextureSampleExpression->Coordinates;
						NewTextureExpr->AutoSetSampleType();
						NewTextureExpr->IsDefaultMeshpaintTexture = TextureSampleExpression->IsDefaultMeshpaintTexture;
						NewTextureExpr->TextureObject = TextureSampleExpression->TextureObject;
						NewTextureExpr->MipValue = TextureSampleExpression->MipValue;
						NewTextureExpr->MipValueMode = TextureSampleExpression->MipValueMode;
					}
					else if (ComponentMaskExpression)
					{
						bNeedsRefresh = true;
						UMaterialExpressionStaticComponentMaskParameter* ComponentMask = CastChecked<UMaterialExpressionStaticComponentMaskParameter>(NewExpression);
						ComponentMask->DefaultR = ComponentMaskExpression->R;
						ComponentMask->DefaultG = ComponentMaskExpression->G;
						ComponentMask->DefaultB = ComponentMaskExpression->B;
						ComponentMask->DefaultA = ComponentMaskExpression->A;
					}
					else if (ParticleSubUVExpression)
					{
						bNeedsRefresh = true;
						CastChecked<UMaterialExpressionTextureSampleParameterSubUV>(NewExpression)->Texture = ParticleSubUVExpression->Texture;
					}

					if (bNeedsRefresh)
					{
						// Refresh the expression preview if we changed its properties after it was created
						NewExpression->bNeedToUpdatePreview = true;
						RefreshExpressionPreview( NewExpression, true );
					}

					ExpressionsToDelete.AddUnique(CurrentSelectedExpression);
					ExpressionsToSelect.Add(NewExpression);
				}
			}
		}
		// Delete the replaced expressions
		TArray< UMaterialExpressionComment *> Comments;

		DeleteObjects(ExpressionsToDelete, Comments );

		// Select each of the newly converted expressions
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		// If only one expression was converted, select it's "Parameter Name" property in the property window
		if (ExpressionsToSelect.Num() == 1)
		{
			// Update the property window so that it displays the properties for the new expression
			UpdatePropertyWindow();
		}
	}
}

void SMaterialEditorCanvas::OnConvertTextures()
{
	if (SelectedExpressions.Num() > 0)
	{
		const FScopedTransaction Transaction( LOCTEXT("MaterialEditorConvertTexture", "Material Editor: Convert to Texture") );
		GetMaterial()->Modify();
		TArray<UMaterialExpression*> ExpressionsToDelete;
		TArray<UMaterialExpression*> ExpressionsToSelect;
		for (int32 ExpressionIndex = 0; ExpressionIndex < SelectedExpressions.Num(); ++ExpressionIndex)
		{
			// Look for the supported classes to convert from
			UMaterialExpression* CurrentSelectedExpression = SelectedExpressions[ExpressionIndex];
			UMaterialExpressionTextureSample* TextureSampleExpression = Cast<UMaterialExpressionTextureSample>(CurrentSelectedExpression);
			UMaterialExpressionTextureObject* TextureObjectExpression = Cast<UMaterialExpressionTextureObject>(CurrentSelectedExpression);

			// Setup the class to convert to
			UClass* ClassToCreate = NULL;
			if (TextureSampleExpression)
			{
				ClassToCreate = UMaterialExpressionTextureObject::StaticClass();
			}
			else if (TextureObjectExpression)
			{
				ClassToCreate = UMaterialExpressionTextureSample::StaticClass();
			}

			if (ClassToCreate)
			{
				UMaterialExpression* NewExpression = CreateNewMaterialExpression(ClassToCreate, false, false, true, FIntPoint(CurrentSelectedExpression->MaterialExpressionEditorX, CurrentSelectedExpression->MaterialExpressionEditorY));
				if (NewExpression)
				{
					SwapLinksToExpression(CurrentSelectedExpression, NewExpression, GetMaterial());
					bool bNeedsRefresh = false;

					// Copy over expression-specific values
					if (TextureSampleExpression)
					{
						bNeedsRefresh = true;
						UMaterialExpressionTextureObject* NewTextureExpr = CastChecked<UMaterialExpressionTextureObject>(NewExpression);
						NewTextureExpr->Texture = TextureSampleExpression->Texture;
						NewTextureExpr->AutoSetSampleType();
						NewTextureExpr->IsDefaultMeshpaintTexture = TextureSampleExpression->IsDefaultMeshpaintTexture;
					}
					else if (TextureObjectExpression)
					{
						bNeedsRefresh = true;
						UMaterialExpressionTextureSample* NewTextureExpr = CastChecked<UMaterialExpressionTextureSample>(NewExpression);
						NewTextureExpr->Texture = TextureObjectExpression->Texture;
						NewTextureExpr->AutoSetSampleType();
						NewTextureExpr->IsDefaultMeshpaintTexture = TextureObjectExpression->IsDefaultMeshpaintTexture;
						NewTextureExpr->MipValueMode = TMVM_None;
					}

					if (bNeedsRefresh)
					{
						// Refresh the expression preview if we changed its properties after it was created
						NewExpression->bNeedToUpdatePreview = true;
						RefreshExpressionPreview( NewExpression, true );
					}

					ExpressionsToDelete.AddUnique(CurrentSelectedExpression);
					ExpressionsToSelect.Add(NewExpression);
				}
			}
		}
		// Delete the replaced expressions
		TArray< UMaterialExpressionComment *> Comments;

		DeleteObjects(ExpressionsToDelete, Comments );

		// Select each of the newly converted expressions
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		// If only one expression was converted, select it's "Parameter Name" property in the property window
		if (ExpressionsToSelect.Num() == 1)
		{
			// Update the property window so that it displays the properties for the new expression
			UpdatePropertyWindow();
		}
	}
}

void SMaterialEditorCanvas::OnSelectDownsteamNodes()
{
	TArray<UMaterialExpression*> ExpressionsToEvaluate;	// Graph nodes that need to be traced downstream
	TArray<UMaterialExpression*> ExpressionsEvalated;	// Keep track of evaluated graph nodes so we don't re-evaluate them
	TArray<UMaterialExpression*> ExpressionsToSelect;	// Downstream graph nodes that will end up being selected

	// Add currently selected graph nodes to the "need to be traced downstream" list
	for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(SelectedExpressions); ExpressionIter; ++ExpressionIter )
	{
		ExpressionsToEvaluate.Add(*ExpressionIter);
	}

	// Generate a list of downstream nodes
	while (ExpressionsToEvaluate.Num() > 0)
	{
		UMaterialExpression* CurrentExpression = ExpressionsToEvaluate.Last();
		if (CurrentExpression)
		{
			TArray<FExpressionOutput>& Outputs = CurrentExpression->GetOutputs();
			
			for (int32 OutputIndex = 0; OutputIndex < Outputs.Num(); OutputIndex++)
			{
				const FExpressionOutput& CurrentOutput = Outputs[OutputIndex];
				TArray<FMaterialExpressionReference> ReferencingInputs;
				GetListOfReferencingInputs(CurrentExpression, GetMaterial(), ReferencingInputs, &CurrentOutput, OutputIndex);

				for (int32 ReferenceIndex = 0; ReferenceIndex < ReferencingInputs.Num(); ReferenceIndex++)
				{
					FMaterialExpressionReference& CurrentReference = ReferencingInputs[ReferenceIndex];
					if (CurrentReference.Expression)
					{
						int32 index = -1;
						ExpressionsEvalated.Find(CurrentReference.Expression, index);

						if (index < 0)
						{
							// This node is a downstream node (so, we'll need to select it) and it's children need to be traced as well
							ExpressionsToSelect.Add(CurrentReference.Expression);
							ExpressionsToEvaluate.Add(CurrentReference.Expression);
						}
					}
				}
			}
		}

		// This graph node has now been examined
		ExpressionsEvalated.Add(CurrentExpression);
		ExpressionsToEvaluate.Remove(CurrentExpression);
	}

	// Select all downstream nodes
	if (ExpressionsToSelect.Num() > 0)
	{
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		UpdatePropertyWindow();
	}
}

void SMaterialEditorCanvas::OnSelectUpsteamNodes()
{
	TArray<UMaterialExpression*> ExpressionsToEvaluate;	// Graph nodes that need to be traced upstream
	TArray<UMaterialExpression*> ExpressionsEvalated;	// Keep track of evaluated graph nodes so we don't re-evaluate them
	TArray<UMaterialExpression*> ExpressionsToSelect;	// Upstream graph nodes that will end up being selected

	// Add currently selected graph nodes to the "need to be traced upstream" list
	for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(SelectedExpressions); ExpressionIter; ++ExpressionIter )
	{
		ExpressionsToEvaluate.Add(*ExpressionIter);
	}

	// Generate a list of upstream nodes
	while (ExpressionsToEvaluate.Num() > 0)
	{
		UMaterialExpression* CurrentExpression = ExpressionsToEvaluate.Last();
		if (CurrentExpression)
		{
			const TArray<FExpressionInput*>& Intputs = CurrentExpression->GetInputs();

			for (int32 InputIndex = 0; InputIndex < Intputs.Num(); InputIndex++)
			{
				const FExpressionInput* CurrentInput = Intputs[InputIndex];
				if (CurrentInput->Expression)
				{
					int32 index = -1;
					ExpressionsEvalated.Find(CurrentInput->Expression, index);

					if (index < 0)
					{
						// This node is an upstream node (so, we'll need to select it) and it's children need to be traced as well
						ExpressionsToSelect.Add(CurrentInput->Expression);
						ExpressionsToEvaluate.Add(CurrentInput->Expression);
					}
				}
			}
		}

		// This graph node has now been examined
		ExpressionsEvalated.Add(CurrentExpression);
		ExpressionsToEvaluate.Remove(CurrentExpression);
	}

	// Select all upstream nodes
	if (ExpressionsToSelect.Num() > 0)
	{
		for ( TArray<UMaterialExpression*>::TConstIterator ExpressionIter(ExpressionsToSelect); ExpressionIter; ++ExpressionIter )
		{
			AddToSelection(*ExpressionIter);
		}

		UpdatePropertyWindow();
	}
}

void SMaterialEditorCanvas::OnDeleteObjects()
{
	DeleteSelectedObjects();
}

void SMaterialEditorCanvas::OnBreakLink()
{
	BreakConnLink(false);
}

void SMaterialEditorCanvas::OnBreakAllLinks()
{
	BreakAllConnectionsToSelectedExpressions();
}

void SMaterialEditorCanvas::OnRemoveFromFavorites()
{
	RemoveSelectedExpressionFromFavorites();
}

void SMaterialEditorCanvas::OnAddToFavorites()
{
	AddSelectedExpressionToFavorites();
}

void SMaterialEditorCanvas::OnPreviewNode()
{
	if (SelectedExpressions.Num() == 1)
	{
		SetPreviewExpression(SelectedExpressions[0]);
	}
}

void SMaterialEditorCanvas::OnToggleRealtimePreview()
{
	if (SelectedExpressions.Num() == 1)
	{
		SelectedExpressions[0]->bRealtimePreview = !SelectedExpressions[0]->bRealtimePreview;

		if (SelectedExpressions[0]->bRealtimePreview)
		{
			SelectedExpressions[0]->bCollapsed = false;
		}

		RefreshExpressionPreviews();
		RefreshExpressionViewport();
		SetMaterialDirty();
	}
}

void SMaterialEditorCanvas::OnConnectToFunctionOutput(int32 MaterialInputIndex) 
{
	if (ConnObj.IsValid())
	{
		const int32 SelectedFunctionOutputIndex = MaterialInputIndex;

		int32 FunctionOutputIndex = 0;
		UMaterialExpressionFunctionOutput* FunctionOutput = NULL;

		for (int32 ExpressionIndex = 0; ExpressionIndex < GetMaterial()->Expressions.Num() && FunctionOutputIndex < NumFunctionOutputsSupported; ExpressionIndex++)
		{
			FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(GetMaterial()->Expressions[ExpressionIndex]);
			if (FunctionOutput)
			{
				if (FunctionOutputIndex == SelectedFunctionOutputIndex)
				{
					break;
				}
				FunctionOutputIndex++;
			}
		}

		if (FunctionOutput)
		{
			UMaterialExpression* SelectedMaterialExpression = Cast<UMaterialExpression>(ConnObj.Get());

			// Input expression.
			FExpressionInput* ExpressionInput = FunctionOutput->GetInput(0);

			const int32 OutputIndex = ConnIndex;

			// Output expression.
			TArray<FExpressionOutput>& Outputs = SelectedMaterialExpression->GetOutputs();
			const FExpressionOutput& ExpressionOutput = Outputs[OutputIndex];

			{
				const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "MaterialEditorMakeConnection", "Material Editor: Make Connection") );

				FunctionOutput->Modify();
				ConnectExpressions(*ExpressionInput, ExpressionOutput, OutputIndex, SelectedMaterialExpression);
			}
			
			// Update the current preview material.
			UpdatePreviewMaterial();
			GetMaterial()->MarkPackageDirty();
			RefreshSourceWindowMaterial();
			RefreshExpressionPreviews();
			RefreshExpressionViewport();
			SetMaterialDirty();
		}
	}
}

void SMaterialEditorCanvas::OnSearchChanged( const FString& InFilterText )
{
	SearchQuery = InFilterText;
	UpdateSearch(true);
}

void SMaterialEditorCanvas::OnAddedToTab( const TSharedRef<SDockTab>& OwnerTab )
{
	ParentTab = OwnerTab;
}

void SMaterialEditorCanvas::ShowSearchResult()
{
	if( SelectedSearchResult >= 0 && SelectedSearchResult < SearchResults.Num() )
	{
		UMaterialExpression* Expression = SearchResults[SelectedSearchResult];

		// Select the selected search item
		EmptySelection();
		AddToSelection(Expression);
		UpdatePropertyWindow();

		PanLocationOnscreen( Expression->MaterialExpressionEditorX+50, Expression->MaterialExpressionEditorY+50, 100 );
	}
}

/**
* PanLocationOnscreen: pan the viewport if necessary to make the particular location visible
*
* @param	X, Y		Coordinates of the location we want onscreen
* @param	Border		Minimum distance we want the location from the edge of the screen.
*/
void SMaterialEditorCanvas::PanLocationOnscreen( int32 LocationX, int32 LocationY, int32 Border )
{
	// Find the bound of the currently visible area of the expression viewport
	int32 X1 = -LinkedObjVC->Origin2D.X + FMath::Round((float)Border*LinkedObjVC->Zoom2D);
	int32 Y1 = -LinkedObjVC->Origin2D.Y + FMath::Round((float)Border*LinkedObjVC->Zoom2D);
	int32 X2 = -LinkedObjVC->Origin2D.X + LinkedObjVC->Viewport->GetSizeXY().X - FMath::Round((float)Border*LinkedObjVC->Zoom2D);
	int32 Y2 = -LinkedObjVC->Origin2D.Y + LinkedObjVC->Viewport->GetSizeXY().Y - FMath::Round((float)Border*LinkedObjVC->Zoom2D);

	int32 X = FMath::Round(((float)LocationX) * LinkedObjVC->Zoom2D);
	int32 Y = FMath::Round(((float)LocationY) * LinkedObjVC->Zoom2D);

	// See if we need to pan the viewport to show the selected search result.
	LinkedObjVC->DesiredOrigin2D = LinkedObjVC->Origin2D;
	bool bChanged = false;
	if( X < X1 )
	{
		LinkedObjVC->DesiredOrigin2D.X += (X1 - X);
		bChanged = true;
	}
	if( Y < Y1 )
	{
		LinkedObjVC->DesiredOrigin2D.Y += (Y1 - Y);
		bChanged = true;
	}
	if( X > X2 )
	{
		LinkedObjVC->DesiredOrigin2D.X -= (X - X2);
		bChanged = true;
	}
	if( Y > Y2 )
	{
		LinkedObjVC->DesiredOrigin2D.Y -= (Y - Y2);
		bChanged = true;
	}
	if( bChanged )
	{
		// Pan to the result in 0.1 seconds
		LinkedObjVC->DesiredPanTime = 0.1f;
	}
	RefreshExpressionViewport();
}

bool SMaterialEditorCanvas::IsActiveMaterialInput(const FMaterialInputInfo& InputInfo)
{
	return GetMaterial()->IsPropertyActive(InputInfo.Property);
}

bool SMaterialEditorCanvas::IsVisibleMaterialInput(const FMaterialInputInfo& InputInfo)
{
	static const auto UseDiffuseSpecularMaterialInputs = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UseDiffuseSpecularMaterialInputs"));

	if (GetMaterial() 
		&& InputInfo.Property >= MP_CustomizedUVs0 
		&& InputInfo.Property <= MP_CustomizedUVs7)
	{
		return InputInfo.Property - MP_CustomizedUVs0 < GetMaterial()->NumCustomizedUVs;
	}

	switch (InputInfo.Property)
	{
		case MP_DiffuseColor:
		case MP_SpecularColor:
			return UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() != 0;
		case MP_BaseColor:
		case MP_Metallic:
		case MP_Specular:
			return UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() == 0;
		default:
			return true;
	}
}

void SMaterialEditorCanvas::UpdateSearch( bool bQueryChanged )
{
	SearchResults.Empty();

	if( SearchQuery.Len() == 0 )
	{
		if( bQueryChanged )
		{
			// We just cleared the search
			SelectedSearchResult = 0;
			RefreshExpressionViewport();
		}
	}
	else
	{
		// Search expressions
		for( int32 Index=0;Index<GetMaterial()->Expressions.Num();Index++ )
		{
			if(GetMaterial()->Expressions[Index]->MatchesSearchQuery(*SearchQuery) )
			{
				SearchResults.Add(GetMaterial()->Expressions[Index]);
			}
		}

		// Search comments
		for( int32 Index=0;Index<GetMaterial()->EditorComments.Num();Index++ )
		{
			if(GetMaterial()->EditorComments[Index]->MatchesSearchQuery(*SearchQuery) )
			{
				SearchResults.Add(GetMaterial()->EditorComments[Index]);
			}
		}

		// Comparison function used to sort search results
		struct FCompareUMaterialExpressionByPos
		{
			FORCEINLINE bool operator()( const UMaterialExpression& A, const UMaterialExpression& B ) const
			{
				// Divide into grid cells and step horizontally and then vertically.
				int32 AGridX = A.MaterialExpressionEditorX / 100;
				int32 AGridY = A.MaterialExpressionEditorY / 100;
				int32 BGridX = B.MaterialExpressionEditorX / 100;
				int32 BGridY = B.MaterialExpressionEditorY / 100;
    
				if( AGridY < BGridY )
				{
					return true;
				}
				else
				if( AGridY > BGridY )
				{
					return false;
				}
				else
				{
					return AGridX < BGridX;
				}
			}
		};

		SearchResults.Sort( FCompareUMaterialExpressionByPos() );

		if( bQueryChanged )
		{
			// This is a new query rather than a material change, so navigate to first search result.
			SelectedSearchResult = 0;

			if( SearchResults.Num() > 0 )
			{
				ShowSearchResult();
			}
			else
			{
				RefreshExpressionViewport();
			}
		}
		else
		{
			if( SelectedSearchResult < 0 || SelectedSearchResult >= SearchResults.Num() )
			{
				SelectedSearchResult = 0;
			}
		}
	}
}

void SMaterialEditorCanvas::RefreshSourceWindowMaterial()
{
	MaterialEditorPtr.Pin()->RegenerateCodeView();
}

void SMaterialEditorCanvas::NewShaderExpressionsSubMenu(FMenuBuilder& MenuBuilder, TArray<struct FMaterialExpression>* MaterialExpressions)
{
	for (int32 i = 0; i < MaterialExpressions->Num(); ++i)
	{
		const FMaterialExpression& MaterialExpression = (*MaterialExpressions)[i];
		if (IsAllowedExpressionType(MaterialExpression.MaterialClass, GetMaterialFunction() != NULL))
		{
			const FText MaterialName = FText::FromString( *MaterialExpression.Name );

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), MaterialName );
			const FText ToolTip = FText::Format( LOCTEXT( "NewMaterialExpressionTooltip", "Adds a {Name} node here" ), Arguments );
			MenuBuilder.AddMenuEntry(
				MaterialName,
				ToolTip,
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnNewMaterialExpression, MaterialExpression.MaterialClass)));
		}
	}
}

void SMaterialEditorCanvas::NewShaderExpressionsMenu(FMenuBuilder& MenuBuilder)
{
	MaterialExpressionClasses* ExpressionClasses = MaterialExpressionClasses::Get();

	if (bUseUnsortedMenus)
	{
		NewShaderExpressionsSubMenu(MenuBuilder, &ExpressionClasses->AllExpressionClasses);
	}
	else
	{
		// Add the Favorites sub-menu
		MenuBuilder.BeginSection("MaterialEditorExpressionsFavorites");
		{
			if (ExpressionClasses->FavoriteExpressionClasses.Num() > 0)
			{
				MenuBuilder.AddSubMenu(LOCTEXT("FavoritesMenu", "Favorites"),
					LOCTEXT("FavoritesMenuTooltip", "Adds a favorited material expression node here"),
					FNewMenuDelegate::CreateSP(this, &SMaterialEditorCanvas::NewShaderExpressionsSubMenu, &ExpressionClasses->FavoriteExpressionClasses));
			}
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("MaterialEditorExpressionsNewShaders");
		{
			if (ExpressionClasses->CategorizedExpressionClasses.Num() > 0)
			{
				for (int32 CategoryIndex = 0; CategoryIndex < ExpressionClasses->CategorizedExpressionClasses.Num(); CategoryIndex++)
				{
					FCategorizedMaterialExpressionNode* CategoryNode = &(ExpressionClasses->CategorizedExpressionClasses[CategoryIndex]);
					const FText CategoryName = FText::FromString( CategoryNode->CategoryName );
					MenuBuilder.AddSubMenu( CategoryName,
						FText::Format( LOCTEXT("CategoryNodeTooltip", "Adds a %s material expression here"), CategoryName),
						FNewMenuDelegate::CreateSP(this, &SMaterialEditorCanvas::NewShaderExpressionsSubMenu, &CategoryNode->MaterialExpressions));
				}
			}

			if (ExpressionClasses->UnassignedExpressionClasses.Num() > 0)
			{
				NewShaderExpressionsSubMenu(MenuBuilder, &ExpressionClasses->UnassignedExpressionClasses);
			}
		}
		MenuBuilder.EndSection();
	}
}

void SMaterialEditorCanvas::PreColorPickerCommit(FLinearColor LinearColor)
{
	// Begin a property edit transaction.
	if ( GEditor )
	{
		GEditor->BeginTransaction( LOCTEXT("ModifyColorPicker", "Modify Color Picker Value") );
	}

	NotifyPreChange(NULL);

	UObject* Object = ColorPickerObject.Get(false);
	if( Object )
	{
		Object->PreEditChange(NULL);
	}
}

void SMaterialEditorCanvas::OnColorPickerCommitted(FLinearColor LinearColor)
{	
	UObject* Object = ColorPickerObject.Get(false);
	if( Object )
	{
		Object->MarkPackageDirty();
		Object->PostEditChange();
	}

	NotifyPostChange(NULL,NULL);

	if ( GEditor )
	{
		GEditor->EndTransaction();
	}

	RefreshExpressionPreviews();
	RefreshExpressionViewport();
}

void SMaterialEditorCanvas::RecenterCamera(bool bCenterY)
{
	int32 XLocation = -GetMaterial()->EditorX + 90;
	int32 YLocation = -GetMaterial()->EditorY + (bCenterY ? 0 : 90);

	if (bCenterY)
	{
		int32 InputsToUse = 0;
		for ( int32 MaterialInputIndex = 0 ; MaterialInputIndex < MaterialInputs.Num() ; ++MaterialInputIndex )
		{
			const FMaterialInputInfo& MaterialInput = MaterialInputs[MaterialInputIndex];
			const bool bShouldAddInputConnector = !bHideUnusedConnectors || MaterialInput.Input->Expression;
			if ( bShouldAddInputConnector )
			{
				++InputsToUse;
			}
		}
		YLocation -= 11 * InputsToUse;
	}

	LinkedObjVC->Origin2D = FIntPoint(XLocation * LinkedObjVC->Zoom2D, YLocation * LinkedObjVC->Zoom2D + (bCenterY ? LinkedObjVC->Viewport->GetSizeXY().Y / 2 : 0));
}

bool SMaterialEditorCanvas::IsVisible() const
{
	return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground());
}

void SMaterialEditorCanvas::BindCommands()
{
	const FMaterialEditorCommands& Commands = FMaterialEditorCommands::Get();

	const TSharedRef<FUICommandList>& UICommandList = MaterialEditorPtr.Pin()->GetToolkitCommands();

	UICommandList->MapAction(
		Commands.CameraHome,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::OnCameraHome ),
		FCanExecuteAction() );
	
	UICommandList->MapAction(
		Commands.CleanUnusedExpressions,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::OnCleanUnusedExpressions ),
		FCanExecuteAction() );
	
	UICommandList->MapAction(
		Commands.ShowHideConnectors,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::OnShowConnectors ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SMaterialEditorCanvas::IsOnShowConnectorsChecked ) );
	
	UICommandList->MapAction(
		Commands.ToggleRealtimeExpressions,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::ToggleRealTimeExpressions ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SMaterialEditorCanvas::IsToggleRealTimeExpressionsChecked ) );
	
	UICommandList->MapAction(
		Commands.AlwaysRefreshAllPreviews,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::OnAlwaysRefreshAllPreviews ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SMaterialEditorCanvas::IsOnAlwaysRefreshAllPreviews ) );

	UICommandList->MapAction(
		Commands.ToggleStats,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::ToggleStats ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SMaterialEditorCanvas::IsToggleStatsChecked ) );
	
	UICommandList->MapAction(
		Commands.ToggleStats,
		FExecuteAction::CreateSP( this, &SMaterialEditorCanvas::ToggleMobileStats ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SMaterialEditorCanvas::IsToggleMobileStatsChecked ) );

	UICommandList->MapAction(
		Commands.NewComment,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnNewComment));
	
	UICommandList->MapAction(
		Commands.PasteHere,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnPasteHere));
	
	UICommandList->MapAction(
		Commands.UseCurrentTexture,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnUseCurrentTexture));
	
	UICommandList->MapAction(
		Commands.ConvertObjects,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnConvertObjects));
	
	UICommandList->MapAction(
		Commands.ConvertToTextureObjects,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnConvertTextures));

	UICommandList->MapAction(
		Commands.ConvertToTextureSamples,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnConvertTextures));

	UICommandList->MapAction(
		Commands.StopPreviewNode,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnPreviewNode));
	
	UICommandList->MapAction(
		Commands.StartPreviewNode,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnPreviewNode));
	
	UICommandList->MapAction(
		Commands.EnableRealtimePreviewNode,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnToggleRealtimePreview));
	
	UICommandList->MapAction(
		Commands.DisableRealtimePreviewNode,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnToggleRealtimePreview));
	
	UICommandList->MapAction(
		Commands.BreakAllLinks,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnBreakAllLinks));
	
	UICommandList->MapAction(
		Commands.DuplicateObjects,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnDuplicateObjects));
	
	UICommandList->MapAction(
		Commands.DeleteObjects,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnDeleteObjects));
	
	UICommandList->MapAction(
		Commands.SelectDownstreamNodes,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnSelectDownsteamNodes));
	
	UICommandList->MapAction(
		Commands.SelectUpstreamNodes,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnSelectUpsteamNodes));
	
	UICommandList->MapAction(
		Commands.RemoveFromFavorites,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnRemoveFromFavorites));
	
	UICommandList->MapAction(
		Commands.AddToFavorites,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnAddToFavorites));
	
	UICommandList->MapAction(
		Commands.BreakLink,
		FExecuteAction::CreateSP(this, &SMaterialEditorCanvas::OnBreakLink));
}

void SMaterialEditorCanvas::OnCameraHome()
{
	RecenterCamera();
	RefreshExpressionViewport();
}

void SMaterialEditorCanvas::OnCleanUnusedExpressions()
{
	CleanUnusedExpressions();
}

void SMaterialEditorCanvas::OnShowConnectors()
{
	bHideUnusedConnectors = !bHideUnusedConnectors;
	RefreshExpressionViewport();
}

bool SMaterialEditorCanvas::IsOnShowConnectorsChecked() const
{
	return bHideUnusedConnectors == false;
}

void SMaterialEditorCanvas::ToggleRealTimeExpressions()
{
	LinkedObjVC->ToggleRealtime();
}

bool SMaterialEditorCanvas::IsToggleRealTimeExpressionsChecked() const
{
	return LinkedObjVC->IsRealtime() == true;
}

void SMaterialEditorCanvas::OnAlwaysRefreshAllPreviews()
{
	bAlwaysRefreshAllPreviews = !bAlwaysRefreshAllPreviews;
	if ( bAlwaysRefreshAllPreviews )
	{
		RefreshExpressionPreviews();
	}
	RefreshExpressionViewport();
}

bool SMaterialEditorCanvas::IsOnAlwaysRefreshAllPreviews() const
{
	return bAlwaysRefreshAllPreviews == true;
}

void SMaterialEditorCanvas::ToggleStats()
{
	// Toggle the showing of material stats each time the user presses the show stats button
	bShowStats = !bShowStats;
	RefreshExpressionViewport();
}

bool SMaterialEditorCanvas::IsToggleStatsChecked() const
{
	return bShowStats == true;
}

void SMaterialEditorCanvas::ToggleMobileStats()
{
	// Toggle the showing of material stats each time the user presses the show stats button
	bShowMobileStats = !bShowMobileStats;
	RefreshExpressionViewport();
}

bool SMaterialEditorCanvas::IsToggleMobileStatsChecked() const
{
	return bShowMobileStats == true;
}

void SMaterialEditorCanvas::OnSearch(SSearchBox::SearchDirection Direction)
{
	if (SearchQuery.Len() > 0)
	{
		if (Direction == SSearchBox::Previous)
		{
			SelectedSearchResult--;
			if( SelectedSearchResult < 0 )
			{
				SelectedSearchResult = FMath::Max<int32>(SearchResults.Num()-1,0);
			}
		}
		else
		{
			SelectedSearchResult++;
			if( SelectedSearchResult >= SearchResults.Num() )
			{
				SelectedSearchResult = 0;
			}
		}
		ShowSearchResult();
	}
}



#undef LOCTEXT_NAMESPACE
