// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "MouseDeltaTracker.h"
#include "ScopedTransaction.h"
#include "SurfaceIterators.h"
#include "EngineSplineClasses.h"
#include "SoundDefinitions.h"
#include "Editor/PlacementMode/Public/IPlacementModeModule.h"
#include "Editor/LandscapeEditor/Public/LandscapeEditorModule.h"
#include "Editor/BspMode/Public/IBspModeModule.h"
#include "Editor/MeshPaint/Public/MeshPaintModule.h"
#include "Editor/GeometryMode/Public/GeometryEdMode.h"
#include "Editor/TextureAlignMode/Public/TextureAlignEdMode.h"
#include "LevelEditor.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorLevelUtils.h"

#include "Editor/FoliageEdit/Public/FoliageEditModule.h"
#include "ActorEditorUtils.h"
#include "EditorStyle.h"


DEFINE_LOG_CATEGORY_STATIC(LogEditorModes, Log, All);

// Builtin editor mode constants
const FEditorModeID FBuiltinEditorModes::EM_None = NAME_None;
const FEditorModeID FBuiltinEditorModes::EM_Default(TEXT("EM_Default"));
const FEditorModeID FBuiltinEditorModes::EM_Placement(TEXT("PLACEMENT"));
const FEditorModeID FBuiltinEditorModes::EM_Bsp(TEXT("BSP"));
const FEditorModeID FBuiltinEditorModes::EM_Geometry(TEXT("EM_Geometry"));
const FEditorModeID FBuiltinEditorModes::EM_InterpEdit(TEXT("EM_InterpEdit"));
const FEditorModeID FBuiltinEditorModes::EM_Texture(TEXT("EM_Texture"));
const FEditorModeID FBuiltinEditorModes::EM_MeshPaint(TEXT("EM_MeshPaint"));
const FEditorModeID FBuiltinEditorModes::EM_Landscape(TEXT("EM_Landscape"));
const FEditorModeID FBuiltinEditorModes::EM_Foliage(TEXT("EM_Foliage"));
const FEditorModeID FBuiltinEditorModes::EM_Level(TEXT("EM_Level"));
const FEditorModeID FBuiltinEditorModes::EM_StreamingLevel(TEXT("EM_StreamingLevel"));
const FEditorModeID FBuiltinEditorModes::EM_Physics(TEXT("EM_Physics"));
const FEditorModeID FBuiltinEditorModes::EM_ActorPicker(TEXT("EM_ActorPicker"));


/** Hit proxy used for editable properties */
struct HPropertyWidgetProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	/** Name of property this is the widget for */
	FString	PropertyName;

	/** If the property is an array property, the index into that array that this widget is for */
	int32	PropertyIndex;

	/** This property is a transform */
	bool	bPropertyIsTransform;

	HPropertyWidgetProxy(FString InPropertyName, int32 InPropertyIndex, bool bInPropertyIsTransform)
		: HHitProxy(HPP_Foreground)
		, PropertyName(InPropertyName)
		, PropertyIndex(InPropertyIndex)
		, bPropertyIsTransform(bInPropertyIsTransform)
	{}

	/** Show cursor as cross when over this handle */
	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}
};

IMPLEMENT_HIT_PROXY(HPropertyWidgetProxy, HHitProxy);

const FName FEdMode::MD_MakeEditWidget(TEXT("MakeEditWidget"));
const FName FEdMode::MD_ValidateWidgetUsing(TEXT("ValidateWidgetUsing"));

namespace
{
	/**
	 * Returns a reference to the named property value data in the given container.
	 */
	template<typename T>
	T* GetPropertyValuePtrByName(const UStruct* InStruct, void* InContainer, FString PropertyName, int32 ArrayIndex)
	{
		T* ValuePtr = NULL;

		// Extract the vector ptr recursively using the property name
		int32 DelimPos = PropertyName.Find(TEXT("."));
		if(DelimPos != INDEX_NONE)
		{
			// Parse the property name and (optional) array index
			int32 SubArrayIndex = 0;
			FString NameToken = PropertyName.Left(DelimPos);
			int32 ArrayPos = NameToken.Find(TEXT("["));
			if(ArrayPos != INDEX_NONE)
			{
				FString IndexToken = NameToken.RightChop(ArrayPos + 1).LeftChop(1);
				SubArrayIndex = FCString::Atoi(*IndexToken);

				NameToken = PropertyName.Left(ArrayPos);
			}

			// Obtain the property info from the given structure definition
			UProperty* CurrentProp = FindField<UProperty>(InStruct, FName(*NameToken));

			// Check first to see if this is a simple structure (i.e. not an array of structures)
			UStructProperty* StructProp = Cast<UStructProperty>(CurrentProp);
			if(StructProp != NULL)
			{
				// Recursively call back into this function with the structure property and container value
				ValuePtr = GetPropertyValuePtrByName<T>(StructProp->Struct, StructProp->ContainerPtrToValuePtr<void>(InContainer), PropertyName.RightChop(DelimPos + 1), ArrayIndex);
			}
			else
			{
				// Check to see if this is an array
				UArrayProperty* ArrayProp = Cast<UArrayProperty>(CurrentProp);
				if(ArrayProp != NULL)
				{
					// It is an array, now check to see if this is an array of structures
					StructProp = Cast<UStructProperty>(ArrayProp->Inner);
					if(StructProp != NULL)
					{
						FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, InContainer);
						if(ArrayHelper.IsValidIndex(SubArrayIndex))
						{
							// Recursively call back into this function with the array element and container value
							ValuePtr = GetPropertyValuePtrByName<T>(StructProp->Struct, ArrayHelper.GetRawPtr(SubArrayIndex), PropertyName.RightChop(DelimPos + 1), ArrayIndex);
						}
					}
				}
			}
		}
		else
		{
			UProperty* Prop = FindField<UProperty>(InStruct, FName(*PropertyName));
			if(Prop != NULL)
			{
				if( UArrayProperty* ArrayProp = Cast<UArrayProperty>(Prop) )
				{
					check(ArrayIndex != INDEX_NONE);

					// Property is an array property, so make sure we have a valid index specified
					FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, InContainer);
					if( ArrayHelper.IsValidIndex(ArrayIndex) )
					{
						ValuePtr = (T*)ArrayHelper.GetRawPtr(ArrayIndex);
					}
				}
				else
				{
					// Property is a vector property, so access directly
					ValuePtr = Prop->ContainerPtrToValuePtr<T>(InContainer);
				}
			}
		}

		return ValuePtr;
	}

	/**
	 * Returns the value of the property with the given name in the given Actor instance.
	 */
	template<typename T>
	T GetPropertyValueByName(AActor* Actor, FString PropertyName, int32 PropertyIndex)
	{
		T Value;
		T* ValuePtr = GetPropertyValuePtrByName<T>(Actor->GetClass(), Actor, PropertyName, PropertyIndex);
		if(ValuePtr)
		{
			Value = *ValuePtr;
		}
		return Value;
	}

	/**
	 * Sets the property with the given name in the given Actor instance to the given value.
	 */
	template<typename T>
	void SetPropertyValueByName(AActor* Actor, FString PropertyName, int32 PropertyIndex, const T& InValue)
	{
		T* ValuePtr = GetPropertyValuePtrByName<T>(Actor->GetClass(), Actor, PropertyName, PropertyIndex);
		if(ValuePtr)
		{
			*ValuePtr = InValue;
		}
	}
}

//////////////////////////////////
// FEdMode

FEdMode::FEdMode()
	: Name( NSLOCTEXT("UnrealEd", "N/A", "N/A") )
	, CurrentTool( NULL )
	, ID( FBuiltinEditorModes::EM_None )
	, EditedPropertyName( TEXT("") )
	, EditedPropertyIndex( INDEX_NONE )
	, bEditedPropertyIsTransform( false )
	, bVisible( false )
	, PriorityOrder( MAX_int32 )
{
	bDrawKillZ = true;
}

FEdMode::~FEdMode()
{
}

bool FEdMode::MouseEnter( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y )
{
	if( GetCurrentTool() )
	{
		return GetCurrentTool()->MouseEnter( ViewportClient, Viewport, x, y );
	}

	return false;
}

bool FEdMode::MouseLeave( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport )
{
	if( GetCurrentTool() )
	{
		return GetCurrentTool()->MouseLeave( ViewportClient, Viewport );
	}

	return false;
}

bool FEdMode::MouseMove(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y)
{
	if( GetCurrentTool() )
	{
		return GetCurrentTool()->MouseMove( ViewportClient, Viewport, x, y );
	}

	return false;
}


/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdMode::CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY )
{
	if( GetCurrentTool() )
	{
		return GetCurrentTool()->CapturedMouseMove( InViewportClient, InViewport, InMouseX, InMouseY );
	}

	return false;
}


bool FEdMode::InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if( GetCurrentTool() && GetCurrentTool()->InputKey( ViewportClient, Viewport, Key, Event ) )
	{
		return true;
	}
	else
	{
		// Pass input up to selected actors if not in a tool mode
		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

		for( TArray<AActor*>::TIterator it(SelectedActors); it; ++it )
		{
			// Tell the object we've had a key press
			(*it)->EditorKeyPressed(Key, Event);
		}
	}

	return 0;
}

bool FEdMode::InputAxis(FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime)
{
	FModeTool* Tool = GetCurrentTool();
	if (Tool)
	{
		return Tool->InputAxis(InViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime);
	}

	return false;
}

bool FEdMode::InputDelta(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if(UsesPropertyWidgets())
	{
		AActor* SelectedActor = GetFirstSelectedActorInstance();
		if(SelectedActor != NULL && InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
		{
			GEditor->NoteActorMovement();

			if (EditedPropertyName != TEXT(""))
			{
				FTransform LocalTM = FTransform::Identity;

				if(bEditedPropertyIsTransform)
				{
					LocalTM = GetPropertyValueByName<FTransform>(SelectedActor, EditedPropertyName, EditedPropertyIndex);
				}
				else
				{					
					FVector LocalPos = GetPropertyValueByName<FVector>(SelectedActor, EditedPropertyName, EditedPropertyIndex);
					LocalTM = FTransform(LocalPos);
				}

				// Get actor transform (actor to world)
				FTransform ActorTM = SelectedActor->ActorToWorld();
				// Calculate world transform
				FTransform WorldTM = LocalTM * ActorTM;
				// Calc delta specified by drag
				//FTransform DeltaTM(InRot.Quaternion(), InDrag);
				// Apply delta in world space
				WorldTM.SetTranslation(WorldTM.GetTranslation() + InDrag);
				WorldTM.SetRotation(InRot.Quaternion() * WorldTM.GetRotation());
				// Convert new world transform back into local space
				LocalTM = WorldTM.GetRelativeTransform(ActorTM);
				// Apply delta scale
				LocalTM.SetScale3D(LocalTM.GetScale3D() + InScale);

				SelectedActor->PreEditChange(NULL);

				if(bEditedPropertyIsTransform)
				{
					SetPropertyValueByName<FTransform>(SelectedActor, EditedPropertyName, EditedPropertyIndex, LocalTM);
				}
				else
				{
					SetPropertyValueByName<FVector>(SelectedActor, EditedPropertyName, EditedPropertyIndex, LocalTM.GetLocation());
				}

				SelectedActor->PostEditChange();

				return true;
			}
		}
	}

	if( GetCurrentTool() )
	{
		return GetCurrentTool()->InputDelta(InViewportClient,InViewport,InDrag,InRot,InScale);
	}

	return 0;
}

/**
 * Lets each tool determine if it wants to use the editor widget or not.  If the tool doesn't want to use it, it will be
 * fed raw mouse delta information (not snapped or altered in any way).
 */

bool FEdMode::UsesTransformWidget() const
{
	if( GetCurrentTool() )
	{
		return GetCurrentTool()->UseWidget();
	}

	return 1;
}

/**
 * Lets each mode selectively exclude certain widget types.
 */
bool FEdMode::UsesTransformWidget(FWidget::EWidgetMode CheckMode) const
{
	if(UsesPropertyWidgets())
	{
		AActor* SelectedActor = GetFirstSelectedActorInstance();
		if(SelectedActor != NULL)
		{
			// If editing a vector (not a transform)
			if(EditedPropertyName != TEXT("") && !bEditedPropertyIsTransform)
			{
				return (CheckMode == FWidget::WM_Translate);
			}
		}
	}

	return true;
}

/**
 * Allows each mode/tool to determine a good location for the widget to be drawn at.
 */

FVector FEdMode::GetWidgetLocation() const
{
	if(UsesPropertyWidgets())
	{
		AActor* SelectedActor = GetFirstSelectedActorInstance();
		if(SelectedActor != NULL)
		{
			if(EditedPropertyName != TEXT(""))
			{
				FVector LocalPos = FVector::ZeroVector;

				if(bEditedPropertyIsTransform)
				{
					FTransform LocalTM = GetPropertyValueByName<FTransform>(SelectedActor, EditedPropertyName, EditedPropertyIndex);
					LocalPos = LocalTM.GetLocation();
				}
				else
				{
					LocalPos = GetPropertyValueByName<FVector>(SelectedActor, EditedPropertyName, EditedPropertyIndex);
				}

				FTransform ActorToWorld = SelectedActor->ActorToWorld();
				FVector WorldPos = ActorToWorld.TransformPosition(LocalPos);
				return WorldPos;
			}
		}
	}

	//UE_LOG(LogEditorModes, Log, TEXT("In FEdMode::GetWidgetLocation"));
	return GEditorModeTools().PivotLocation;
}

/**
 * Lets the mode determine if it wants to draw the widget or not.
 */

bool FEdMode::ShouldDrawWidget() const
{
	return (GEditor->GetSelectedActors()->GetTop<AActor>() != NULL || GEditor->GetSelectedComponents()->GetTop<USceneComponent>() != NULL);
}

/**
 * Allows each mode to customize the axis pieces of the widget they want drawn.
 *
 * @param	InwidgetMode	The current widget mode
 *
 * @return	A bitfield comprised of AXIS_ values
 */

EAxisList::Type FEdMode::GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const
{
	return EAxisList::All;
}

/**
 * Lets each mode/tool handle box selection in its own way.
 *
 * @param	InBox	The selection box to use, in worldspace coordinates.
 * @return		true if something was selected/deselected, false otherwise.
 */
bool FEdMode::BoxSelect( FBox& InBox, bool InSelect )
{
	bool bResult = false;
	if( GetCurrentTool() )
	{
		bResult = GetCurrentTool()->BoxSelect( InBox, InSelect );
	}
	return bResult;
}

/**
 * Lets each mode/tool handle frustum selection in its own way.
 *
 * @param	InFrustum	The selection frustum to use, in worldspace coordinates.
 * @return	true if something was selected/deselected, false otherwise.
 */
bool FEdMode::FrustumSelect( const FConvexVolume& InFrustum, bool InSelect )
{
	bool bResult = false;
	if( GetCurrentTool() )
	{
		bResult = GetCurrentTool()->FrustumSelect( InFrustum, InSelect );
	}
	return bResult;
}

void FEdMode::SelectNone()
{
	if( GetCurrentTool() )
	{
		GetCurrentTool()->SelectNone();
	}
}

void FEdMode::Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime)
{
	if( GetCurrentTool() )
	{
		GetCurrentTool()->Tick(ViewportClient,DeltaTime);
	}
}

void FEdMode::ActorSelectionChangeNotify()
{
	EditedPropertyName = TEXT("");
	EditedPropertyIndex = INDEX_NONE;
	bEditedPropertyIsTransform = false;
}

bool FEdMode::HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click)
{
	if(UsesPropertyWidgets() && HitProxy)
	{
		if( HitProxy->IsA(HPropertyWidgetProxy::StaticGetType()) )
		{
			HPropertyWidgetProxy* PropertyProxy = (HPropertyWidgetProxy*)HitProxy;
			EditedPropertyName = PropertyProxy->PropertyName;
			EditedPropertyIndex = PropertyProxy->PropertyIndex;
			bEditedPropertyIsTransform = PropertyProxy->bPropertyIsTransform;
			return true;
		}
		// Left clicking on an actor, stop editing a property
		else if( HitProxy->IsA(HActor::StaticGetType()) )
		{
			EditedPropertyName = TEXT("");
			EditedPropertyIndex = INDEX_NONE;
			bEditedPropertyIsTransform = false;
		}
	}

	return 0;
}

void FEdMode::Enter()
{
	// Update components for selected actors, in case the mode we just exited
	// was hijacking selection events selection and not updating components.
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* SelectedActor = static_cast<AActor*>( *It );
		checkSlow( SelectedActor->IsA(AActor::StaticClass()) );
		SelectedActor->MarkComponentsRenderStateDirty();
	}

	FEditorDelegates::EditorModeEnter.Broadcast();
	const bool bIsEnteringMode = true;
	GEditorModeTools().BroadcastEditorModeChanged( this, bIsEnteringMode );
}

void FEdMode::Exit()
{
	const bool bIsEnteringMode = false;
	GEditorModeTools().BroadcastEditorModeChanged( this, bIsEnteringMode );
	FEditorDelegates::EditorModeExit.Broadcast();
}

void FEdMode::SetCurrentTool( EModeTools InID )
{
	CurrentTool = FindTool( InID );
	check( CurrentTool );	// Tool not found!  This can't happen.

	CurrentToolChanged();
}

void FEdMode::SetCurrentTool( FModeTool* InModeTool )
{
	CurrentTool = InModeTool;
	check(CurrentTool);

	CurrentToolChanged();
}

FModeTool* FEdMode::FindTool( EModeTools InID )
{
	for( int32 x = 0 ; x < Tools.Num() ; ++x )
	{
		if( Tools[x]->GetID() == InID )
		{
			return Tools[x];
		}
	}

	UE_LOG(LogEditorModes, Fatal, TEXT("FEdMode::FindTool failed to find tool %d"), (int32)InID);
	return NULL;
}

void FEdMode::Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI)
{
	if( GEditor->bShowBrushMarkerPolys )
	{
		// Draw translucent polygons on brushes and volumes

		for( FActorIterator It(GetWorld()); It; ++ It )
		{
			ABrush* Brush = Cast<ABrush>( *It );

			// Brush->Brush is checked to safe from brushes that were created without having their brush members attached.
			if( Brush && Brush->Brush && (FActorEditorUtils::IsABuilderBrush(Brush) || Brush->IsVolumeBrush()) && GEditor->GetSelectedActors()->IsSelected(Brush) )
			{
				// Build a mesh by basically drawing the triangles of each 
				FDynamicMeshBuilder MeshBuilder;
				int32 VertexOffset = 0;

				for( int32 PolyIdx = 0 ; PolyIdx < Brush->Brush->Polys->Element.Num() ; ++PolyIdx )
				{
					const FPoly* Poly = &Brush->Brush->Polys->Element[PolyIdx];

					if( Poly->Vertices.Num() > 2 )
					{
						const FVector Vertex0 = Poly->Vertices[0];
						FVector Vertex1 = Poly->Vertices[1];

						MeshBuilder.AddVertex(Vertex0, FVector2D::ZeroVector, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FColor(255,255,255));
						MeshBuilder.AddVertex(Vertex1, FVector2D::ZeroVector, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FColor(255,255,255));

						for( int32 VertexIdx = 2 ; VertexIdx < Poly->Vertices.Num() ; ++VertexIdx )
						{
							const FVector Vertex2 = Poly->Vertices[VertexIdx];
							MeshBuilder.AddVertex(Vertex2, FVector2D::ZeroVector, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FColor(255,255,255));
							MeshBuilder.AddTriangle(VertexOffset,VertexOffset + VertexIdx,VertexOffset+VertexIdx-1);
							Vertex1 = Vertex2;
						}

						// Increment the vertex offset so the next polygon uses the correct vertex indices.
						VertexOffset += Poly->Vertices.Num();
					}
				}

				// Allocate the material proxy and register it so it can be deleted properly once the rendering is done with it.
				FDynamicColoredMaterialRenderProxy* MaterialProxy = new FDynamicColoredMaterialRenderProxy(GEngine->EditorBrushMaterial->GetRenderProxy(false),Brush->GetWireColor());
				PDI->RegisterDynamicResource( MaterialProxy );

				// Flush the mesh triangles.
				MeshBuilder.Draw(PDI, Brush->ActorToWorld().ToMatrixWithScale(), MaterialProxy, SDPG_World, 0.f);
			}
		}
	}

	if (GEditorModeTools().ShouldDrawBrushVertices())
	{
		UTexture2D* VertexTexture = GetVertexTexture();
		const float TextureSizeX = VertexTexture->GetSizeX() * 0.170f;
		const float TextureSizeY = VertexTexture->GetSizeY() * 0.170f;

		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			AActor* SelectedActor = static_cast<AActor*>(*It);
			checkSlow(SelectedActor->IsA(AActor::StaticClass()));

			ABrush* Brush = Cast< ABrush >(SelectedActor);
			if (Brush && Brush->Brush && !FActorEditorUtils::IsABuilderBrush(Brush))
			{
				for (int32 p = 0; p < Brush->Brush->Polys->Element.Num(); ++p)
				{
					FTransform BrushTransform = Brush->ActorToWorld();

					FPoly* poly = &Brush->Brush->Polys->Element[p];
					for (int32 VertexIndex = 0; VertexIndex < poly->Vertices.Num(); ++VertexIndex)
					{
						const FVector& PolyVertex = poly->Vertices[VertexIndex];
						const FVector WorldLocation = BrushTransform.TransformPosition(PolyVertex);
						
						const float Scale = View->WorldToScreen( WorldLocation ).W * ( 4.0f / View->ViewRect.Width() / View->ViewMatrices.ProjMatrix.M[0][0] );

						const FColor Color(Brush->GetWireColor());
						PDI->SetHitProxy(new HBSPBrushVert(Brush, &poly->Vertices[VertexIndex]));

						PDI->DrawSprite(WorldLocation, TextureSizeX * Scale, TextureSizeY * Scale, VertexTexture->Resource, Color, SDPG_World, 0.0f, 0.0f, 0.0f, 0.0f, SE_BLEND_Masked );

						PDI->SetHitProxy(NULL);
			
					}
				}
			}
		}
	}

	// Let the current mode tool render if it wants to
	FModeTool* tool = GetCurrentTool();
	if( tool )
	{
		tool->Render( View, Viewport, PDI );
	}

	AGroupActor::DrawBracketsForGroups(PDI, Viewport);

	if(UsesPropertyWidgets())
	{
		bool bHitTesting = PDI->IsHitTesting();
		AActor* SelectedActor = GetFirstSelectedActorInstance();
		if (SelectedActor != NULL)
		{
			UClass* Class = SelectedActor->GetClass();
			TArray<FPropertyWidgetInfo> WidgetInfos;
			GetPropertyWidgetInfos(Class, SelectedActor, WidgetInfos);
			FEditorScriptExecutionGuard ScriptGuard;
			for(int32 i=0; i<WidgetInfos.Num(); i++)
			{
				FString WidgetName = WidgetInfos[i].PropertyName;
				FName WidgetValidator = WidgetInfos[i].PropertyValidationName;
				int32 WidgetIndex = WidgetInfos[i].PropertyIndex;
				bool bIsTransform = WidgetInfos[i].bIsTransform;

				bool bSelected = (WidgetName == EditedPropertyName) && (WidgetIndex == EditedPropertyIndex);
				FColor WidgetColor = bSelected ? FColor(255,255,255) : FColor(128,128,255);

				FVector LocalPos = FVector::ZeroVector;
				if(bIsTransform)
				{
					FTransform LocalTM = GetPropertyValueByName<FTransform>(SelectedActor, WidgetName, WidgetIndex);
					LocalPos = LocalTM.GetLocation();
				}
				else
				{
					LocalPos = GetPropertyValueByName<FVector>(SelectedActor, WidgetName, WidgetIndex);
				}

				FTransform ActorToWorld = SelectedActor->ActorToWorld();
				FVector WorldPos = ActorToWorld.TransformPosition(LocalPos);

				UFunction* ValidateFunc= NULL;
				if(WidgetValidator != NAME_None && 
					(ValidateFunc = SelectedActor->FindFunction(WidgetValidator)) != NULL)
				{
					FString ReturnText;
					SelectedActor->ProcessEvent(ValidateFunc, &ReturnText);

					//if we have a negative result, the widget color is red.
					WidgetColor = ReturnText.IsEmpty() ? WidgetColor : FColor::Red;
				}

				FTranslationMatrix WidgetTM(WorldPos);

				const float WidgetSize = 0.035f;
				const float ZoomFactor = FMath::Min<float>(View->ViewMatrices.ProjMatrix.M[0][0], View->ViewMatrices.ProjMatrix.M[1][1]);
				const float WidgetRadius = View->Project(WorldPos).W * (WidgetSize / ZoomFactor);

				if(bHitTesting) PDI->SetHitProxy( new HPropertyWidgetProxy(WidgetName, WidgetIndex, bIsTransform) );
				DrawWireDiamond(PDI, WidgetTM, WidgetRadius, WidgetColor, SDPG_Foreground );
				if(bHitTesting) PDI->SetHitProxy( NULL );
			}
		}
	}
}

void FEdMode::DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas)
{
	// Render the drag tool.
	ViewportClient->RenderDragTool( View, Canvas );

	// Let the current mode tool draw a HUD if it wants to
	FModeTool* tool = GetCurrentTool();
	if( tool )
	{
		tool->DrawHUD( ViewportClient, Viewport, View, Canvas );
	}

	if (ViewportClient->IsPerspective() && GetDefault<ULevelEditorViewportSettings>()->bHighlightWithBrackets)
	{
		DrawBrackets( ViewportClient, Viewport, View, Canvas );
	}

	// If this viewport doesn't show mode widgets or the mode itself doesn't want them, leave.
	if( !(ViewportClient->EngineShowFlags.ModeWidgets) || !ShowModeWidgets() )
	{
		return;
	}

	// Clear Hit proxies
	const bool bIsHitTesting = Canvas->IsHitTesting();
	if ( !bIsHitTesting )
	{
		Canvas->SetHitProxy(NULL);
	}

	// Draw vertices for selected BSP brushes and static meshes if the large vertices show flag is set.
	if ( !ViewportClient->bDrawVertices )
	{
		return;
	}

	const bool bLargeVertices		= View->Family->EngineShowFlags.LargeVertices;
	const bool bShowBrushes			= View->Family->EngineShowFlags.Brushes;
	const bool bShowBSP				= View->Family->EngineShowFlags.BSP;
	const bool bShowBuilderBrush	= View->Family->EngineShowFlags.BuilderBrush != 0;

	UTexture2D* VertexTexture = GetVertexTexture();
	const float TextureSizeX		= VertexTexture->GetSizeX() * ( bLargeVertices ? 1.0f : 0.5f );
	const float TextureSizeY		= VertexTexture->GetSizeY() * ( bLargeVertices ? 1.0f : 0.5f );

	// Temporaries.
	TArray<FVector> Vertices;

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* SelectedActor = static_cast<AActor*>( *It );
		checkSlow( SelectedActor->IsA(AActor::StaticClass()) );

		if( bLargeVertices )
		{
			FCanvasItemTestbed::bTestState = !FCanvasItemTestbed::bTestState;

			// Static mesh vertices
			AStaticMeshActor* Actor = Cast<AStaticMeshActor>( SelectedActor );
			if( Actor && Actor->StaticMeshComponent && Actor->StaticMeshComponent->StaticMesh
				&& Actor->StaticMeshComponent->StaticMesh->RenderData )
			{
				FTransform ActorToWorld = Actor->ActorToWorld();
				Vertices.Empty();
				const FPositionVertexBuffer& VertexBuffer = Actor->StaticMeshComponent->StaticMesh->RenderData->LODResources[0].PositionVertexBuffer;
				for( uint32 i = 0 ; i < VertexBuffer.GetNumVertices() ; i++ )
				{
					Vertices.AddUnique( ActorToWorld.TransformPosition( VertexBuffer.VertexPosition(i) ) );
				}

				FCanvasTileItem TileItem( FVector2D( 0.0f, 0.0f ), FVector2D( 0.0f, 0.0f ), FLinearColor::White );
				TileItem.BlendMode = SE_BLEND_Translucent;
				for( int32 VertexIndex = 0 ; VertexIndex < Vertices.Num() ; ++VertexIndex )
				{				
					const FVector& Vertex = Vertices[VertexIndex];
					FVector2D PixelLocation;
					if(View->ScreenToPixel(View->WorldToScreen(Vertex),PixelLocation))
					{
						const bool bOutside =
							PixelLocation.X < 0.0f || PixelLocation.X > View->ViewRect.Width() ||
							PixelLocation.Y < 0.0f || PixelLocation.Y > View->ViewRect.Height();
						if( !bOutside )
						{
							const float X = PixelLocation.X - (TextureSizeX/2);
							const float Y = PixelLocation.Y - (TextureSizeY/2);
							if( bIsHitTesting ) 
							{
								Canvas->SetHitProxy( new HStaticMeshVert(Actor,Vertex) );
							}
							TileItem.Texture = VertexTexture->Resource;
							
							TileItem.Size = FVector2D( TextureSizeX, TextureSizeY );
							Canvas->DrawItem( TileItem, FVector2D( X, Y ) );							
							if( bIsHitTesting )
							{
								Canvas->SetHitProxy( NULL );
							}
						}
					}
				}
			}
		}
	}

	if(UsesPropertyWidgets())
	{
		AActor* SelectedActor = GetFirstSelectedActorInstance();
		if (SelectedActor != NULL)
		{
			FEditorScriptExecutionGuard ScriptGuard;

			const int32 HalfX = 0.5f * Viewport->GetSizeXY().X;
			const int32 HalfY = 0.5f * Viewport->GetSizeXY().Y;

			UClass* Class = SelectedActor->GetClass();		
			TArray<FPropertyWidgetInfo> WidgetInfos;
			GetPropertyWidgetInfos(Class, SelectedActor, WidgetInfos);
			for(int32 i=0; i<WidgetInfos.Num(); i++)
			{
				FString WidgetName = WidgetInfos[i].PropertyName;
				FName WidgetValidator = WidgetInfos[i].PropertyValidationName;
				int32 WidgetIndex = WidgetInfos[i].PropertyIndex;
				bool bIsTransform = WidgetInfos[i].bIsTransform;

				FVector LocalPos = FVector::ZeroVector;
				if(bIsTransform)
				{
					FTransform LocalTM = GetPropertyValueByName<FTransform>(SelectedActor, WidgetName, WidgetIndex);
					LocalPos = LocalTM.GetLocation();
				}
				else
				{
					LocalPos = GetPropertyValueByName<FVector>(SelectedActor, WidgetName, WidgetIndex);
				}

				FTransform ActorToWorld = SelectedActor->ActorToWorld();
				FVector WorldPos = ActorToWorld.TransformPosition(LocalPos);

				UFunction* ValidateFunc = NULL;
				FString FinalString;
				if(WidgetValidator != NAME_None && 
					(ValidateFunc = SelectedActor->FindFunction(WidgetValidator)) != NULL)
				{
					SelectedActor->ProcessEvent(ValidateFunc, &FinalString);
				}

				const FPlane Proj = View->Project( WorldPos );

				//do some string fixing
				const uint32 VectorIndex = WidgetInfos[i].PropertyIndex;
				FString WidgetDisplayName = WidgetName + ((VectorIndex != INDEX_NONE) ? FString::Printf(TEXT("[%d]"), VectorIndex) : TEXT(""));
				FinalString = FinalString.IsEmpty() ? WidgetDisplayName : FinalString;

				if(Proj.W > 0.f)
				{
					const int32 XPos = HalfX + ( HalfX * Proj.X );
					const int32 YPos = HalfY + ( HalfY * (Proj.Y * -1.f) );
					FCanvasTextItem TextItem( FVector2D( XPos + 5, YPos), FText::FromString( FinalString ), GEngine->GetSmallFont(), FLinearColor::White );
					Canvas->DrawItem( TextItem );
				}
			}
		}
	}
}

// Draw brackets around all selected objects
void FEdMode::DrawBrackets( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( int32 CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		AActor* SelectedActor = Cast<AActor>( SelectedActors.GetSelectedObject(CurSelectedActorIndex ) );
		if( SelectedActor != NULL )
		{
			// Draw a bracket for selected "paintable" static mesh actors
			const bool bIsValidActor = ( Cast< AStaticMeshActor >( SelectedActor ) != NULL );

			const FLinearColor SelectedActorBoxColor( 0.6f, 0.6f, 1.0f );
			const bool bDrawBracket = bIsValidActor;
			ViewportClient->DrawActorScreenSpaceBoundingBox( Canvas, View, Viewport, SelectedActor, SelectedActorBoxColor, bDrawBracket );
		}
	}
}

bool FEdMode::UsesToolkits() const
{
	return false;
}

bool FEdMode::StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bool bResult = false;
	if( GetCurrentTool() )
	{
		bResult = GetCurrentTool()->StartModify();
	}
	return bResult;
}

bool FEdMode::EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bool bResult = false;
	if( GetCurrentTool() )
	{
		bResult = GetCurrentTool()->EndModify();
	}
	return bResult;
}

FVector FEdMode::GetWidgetNormalFromCurrentAxis( void* InData )
{
	// Figure out the proper coordinate system.

	FMatrix matrix = FMatrix::Identity;
	if( GEditorModeTools().GetCoordSystem() == COORD_Local )
	{
		GetCustomDrawingCoordinateSystem( matrix, InData );
	}

	// Get a base normal from the current axis.

	FVector BaseNormal(1,0,0);		// Default to X axis
	switch( CurrentWidgetAxis )
	{
		case EAxisList::Y:	BaseNormal = FVector(0,1,0);	break;
		case EAxisList::Z:	BaseNormal = FVector(0,0,1);	break;
		case EAxisList::XY:	BaseNormal = FVector(1,1,0);	break;
		case EAxisList::XZ:	BaseNormal = FVector(1,0,1);	break;
		case EAxisList::YZ:	BaseNormal = FVector(0,1,1);	break;
		case EAxisList::XYZ:	BaseNormal = FVector(1,1,1);	break;
	}

	// Transform the base normal into the proper coordinate space.
	return matrix.TransformPosition( BaseNormal );
}

AActor* FEdMode::GetFirstSelectedActorInstance() const
{
	if(GEditor->GetSelectedActorCount() > 0)
	{
		return GEditor->GetSelectedActors()->GetTop<AActor>();
	}

	return NULL;
}

bool FEdMode::CanCreateWidgetForProperty(UProperty* InProp)
{
	UStructProperty* TestProperty = Cast<UStructProperty>(InProp);
	if( !TestProperty )
	{
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(InProp);
		if( ArrayProperty )
		{
			TestProperty = Cast<UStructProperty>(ArrayProperty->Inner);
		}
	}
	return (TestProperty != NULL) && (TestProperty->Struct->GetFName() == NAME_Vector || TestProperty->Struct->GetFName() == NAME_Transform);
}

bool FEdMode::ShouldCreateWidgetForProperty(UProperty* InProp)
{
	return CanCreateWidgetForProperty(InProp) && InProp->HasMetaData(MD_MakeEditWidget);
}

static bool IsTransformProperty(UProperty* InProp)
{
	UStructProperty* StructProp = Cast<UStructProperty>(InProp);
	return (StructProp != NULL && StructProp->Struct->GetFName() == NAME_Transform);

}

void FEdMode::GetPropertyWidgetInfos(const UStruct* InStruct, const void* InContainer, TArray<FPropertyWidgetInfo>& OutInfos, FString PropertyNamePrefix) const
{
	if(PropertyNamePrefix.Len() == 0)
	{
		OutInfos.Empty();
	}

	for (TFieldIterator<UProperty> PropertyIt(InStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* CurrentProp = *PropertyIt;
		if(	ShouldCreateWidgetForProperty(CurrentProp) )
		{
			if( UArrayProperty* ArrayProp = Cast<UArrayProperty>(CurrentProp) )
			{
				check(InContainer != NULL);

				FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, InContainer);

				// See how many widgets we need to make for the array property
				uint32 ArrayDim = ArrayHelper.Num();
				for( uint32 i = 0; i < ArrayDim; i++ )
				{
					//create a new widget info struct
					FPropertyWidgetInfo WidgetInfo;

					//fill it in with the struct name
					WidgetInfo.PropertyName = PropertyNamePrefix + CurrentProp->GetFName().ToString();

					//And see if we have any meta data that matches the MD_ValidateWidgetUsing name
					WidgetInfo.PropertyValidationName = FName(*CurrentProp->GetMetaData(MD_ValidateWidgetUsing));

					WidgetInfo.PropertyIndex = i;

					// See if its a transform
					WidgetInfo.bIsTransform = IsTransformProperty(ArrayProp->Inner);

					//Add it to our out array
					OutInfos.Add(WidgetInfo);
				}
			}
			else
			{

				//create a new widget info struct
				FPropertyWidgetInfo WidgetInfo;

				//fill it in with the struct name
				WidgetInfo.PropertyName = PropertyNamePrefix + CurrentProp->GetFName().ToString();

				//And see if we have any meta data that matches the MD_ValidateWidgetUsing name
				WidgetInfo.PropertyValidationName = FName(*CurrentProp->GetMetaData(MD_ValidateWidgetUsing));

				// See if its a transform
				WidgetInfo.bIsTransform = IsTransformProperty(CurrentProp);

				//Add it to our out array
				OutInfos.Add(WidgetInfo);
			}

		}
		else
		{
			UStructProperty* StructProp = Cast<UStructProperty>(CurrentProp);
			if(StructProp != NULL)
			{
				// Recursively traverse into structures, looking for additional vector properties to expose
				GetPropertyWidgetInfos(StructProp->Struct, StructProp->ContainerPtrToValuePtr<void>(InContainer), OutInfos, StructProp->GetFName().ToString() + TEXT("."));
			}
			else
			{
				// Recursively traverse into arrays of structures, looking for additional vector properties to expose
				UArrayProperty* ArrayProp = Cast<UArrayProperty>(CurrentProp);
				if(ArrayProp != NULL)
				{
					StructProp = Cast<UStructProperty>(ArrayProp->Inner);
					if(StructProp != NULL)
					{
						FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, InContainer);
						for(int32 ArrayIndex = 0; ArrayIndex < ArrayHelper.Num(); ++ArrayIndex)
						{
							if(ArrayHelper.IsValidIndex(ArrayIndex))
							{
								GetPropertyWidgetInfos(StructProp->Struct, ArrayHelper.GetRawPtr(ArrayIndex), OutInfos, StructProp->GetFName().ToString() + FString::Printf(TEXT("[%d]"), ArrayIndex) + TEXT("."));
							}
						}
					}
				}
			}
		}
	}
}

bool FEdMode::IsSnapRotationEnabled()
{
	return GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled;
}

/*------------------------------------------------------------------------------
	Default.
------------------------------------------------------------------------------*/

FEdModeDefault::FEdModeDefault()
{
	ID = FBuiltinEditorModes::EM_Default;
	Name = NSLOCTEXT("EditorModes", "DefaultMode", "Default");
	bDrawKillZ = true;
}

/*------------------------------------------------------------------------------
	FEditorModeTools.

	The master class that handles tracking of the current mode.
------------------------------------------------------------------------------*/

FEditorModeTools::FEditorModeTools()
	:	PivotShown( 0 )
	,	Snapping( 0 )
	,	SnappedActor( 0 )
	,	TranslateRotateXAxisAngle(0)
	,	CoordSystem( COORD_World )
	,	WidgetMode( FWidget::WM_Translate )
	,	OverrideWidgetMode( FWidget::WM_None )
	,	bShowWidget( 1 )
	,	bIsTracking( false )
	,	bHideViewportUI( false )
{}

FEditorModeTools::~FEditorModeTools()
{

}

void FEditorModeTools::RegisterMode(TSharedRef<FEdMode> ModeToRegister)
{
	check(ModeToRegister->ID != FBuiltinEditorModes::EM_None);
	check(Modes.Find(ModeToRegister) == INDEX_NONE);

	Modes.Add(ModeToRegister);

	BroadcastRegisteredModesChanged();
}

void FEditorModeTools::UnregisterMode(TSharedRef<FEdMode> ModeToUnregister)
{
	// See if we need to deactivate the mode that is being unregistered.
	DeactivateMode(ModeToUnregister->ID);

	for (int32 ModeIndex = 0; ModeIndex < Modes.Num(); )
	{
		if (Modes[ModeIndex] == ModeToUnregister)
		{
			Modes.RemoveAtSwap(ModeIndex);
		}
		else
		{
			++ModeIndex;
		}
	}

	BroadcastRegisteredModesChanged();
}

void FEditorModeTools::RegisterCompatibleModes( const FEditorModeID& Mode1, const FEditorModeID& Mode2 )
{
	ModeCompatabilityMap.Add( Mode1, Mode2 );
	ModeCompatabilityMap.Add( Mode2, Mode1 );
}

void FEditorModeTools::UnregisterCompatibleModes( const FEditorModeID& Mode1, const FEditorModeID& Mode2 )
{
	ModeCompatabilityMap.Remove( Mode1, Mode2 );
	ModeCompatabilityMap.Remove( Mode2, Mode1 );
}

// A simple macro to help set editor mode compatibility mappings
#define SET_MODE_COMPATABILITY( InMode1, InMode2  ) \
	ModeCompatabilityMap.Add( InMode1, InMode2 ); \
	ModeCompatabilityMap.Add( InMode2, InMode1 ); \

void FEditorModeTools::Init()
{
	// Load the last used settings
	LoadConfig();

	// Editor modes
	Modes.Add( MakeShareable(new FEdModeDefault()) );
	Modes.Add( MakeShareable(new FEdModeInterpEdit()) );

	// Load editor mode modules that will automatically register their editor modes, and clean themselves up on unload.
	//@TODO: ROCKET: These are probably good plugin candidates, that shouldn't have to be force-loaded here but discovery loaded somehow
	FModuleManager::LoadModuleChecked<IPlacementModeModule>(TEXT("PlacementMode"));
	FModuleManager::LoadModuleChecked<IBspModeModule>(TEXT("BspMode"));
	FModuleManager::LoadModuleChecked<FTextureAlignModeModule>(TEXT("TextureAlignMode"));
	FModuleManager::LoadModuleChecked<FGeometryModeModule>(TEXT("GeometryMode"));
	FModuleManager::LoadModuleChecked<IMeshPaintModule>(TEXT("MeshPaint"));
	FModuleManager::LoadModuleChecked<ILandscapeEditorModule>(TEXT("LandscapeEditor"));
	FModuleManager::LoadModuleChecked<IFoliageEditModule>(TEXT("FoliageEdit"));

	// Set which modes can run together
	
	// Interp edit and mesh paint can run together
	SET_MODE_COMPATABILITY( FBuiltinEditorModes::EM_InterpEdit, FBuiltinEditorModes::EM_MeshPaint );
	// Interp edit and geometry mode can run together
	SET_MODE_COMPATABILITY( FBuiltinEditorModes::EM_InterpEdit, FBuiltinEditorModes::EM_Geometry );
	// Interp and BSP mode can run together
	SET_MODE_COMPATABILITY( FBuiltinEditorModes::EM_InterpEdit, TEXT("BSP") );

	// Register our callback for actor selection changes
	USelection::SelectNoneEvent.AddRaw(this, &FEditorModeTools::OnEditorSelectNone);
	USelection::SelectionChangedEvent.AddRaw(this, &FEditorModeTools::OnEditorSelectionChanged);
	USelection::SelectObjectEvent.AddRaw(this, &FEditorModeTools::OnEditorSelectionChanged);
}

/**
 * Called from UUnrealEdEngine::PreExit
 */
void FEditorModeTools::Shutdown()
{
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectNoneEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);

	//@TODO: Should we exit any active modes here, or is it too late?

	// Clean up modes
	ActiveModes.Empty();
	ModeCompatabilityMap.Empty();
	Modes.Empty();
}

/**
 * Loads the state that was saved in the INI file
 */
void FEditorModeTools::LoadConfig(void)
{
	GConfig->GetBool(TEXT("FEditorModeTools"),TEXT("ShowWidget"),bShowWidget,
		GEditorUserSettingsIni);

	const bool bGetRawValue = true;
	int32 Bogus = (int32)GetCoordSystem(bGetRawValue);
	GConfig->GetInt(TEXT("FEditorModeTools"),TEXT("CoordSystem"),Bogus,
		GEditorUserSettingsIni);
	SetCoordSystem((ECoordSystem)Bogus);

	
	GConfig->GetBool( TEXT("UnEdViewport"), TEXT("InterpEdPanInvert"), bInterpPanInverted, GEditorUserSettingsIni );
	LoadWidgetSettings();
}

/**
 * Saves the current state to the INI file
 */
void FEditorModeTools::SaveConfig(void)
{
	GConfig->SetBool(TEXT("FEditorModeTools"),TEXT("ShowWidget"),bShowWidget,
		GEditorUserSettingsIni);

	const bool bGetRawValue = true;
	GConfig->SetInt(TEXT("FEditorModeTools"),TEXT("CoordSystem"),(int32)GetCoordSystem(bGetRawValue),
		GEditorUserSettingsIni);

	GConfig->SetBool( TEXT("UnEdViewport"), TEXT("InterpEdPanInvert"), bInterpPanInverted, GEditorUserSettingsIni );

	SaveWidgetSettings();
}

void FEditorModeTools::OnEditorSelectionChanged(UObject* NewSelection)
{
	// If selecting an actor, move the pivot location.
	AActor* Actor = Cast<AActor>(NewSelection);
	if (Actor != NULL)
	{
		//@fixme - why isn't this using UObject::IsSelected()?
		if ( GEditor->GetSelectedActors()->IsSelected( Actor ) )
		{
			SetPivotLocation( Actor->GetActorLocation(), false );

			// If this actor wasn't part of the original selection set during pie/sie, clear it now
			if ( GEditor->ActorsThatWereSelected.Num() > 0 )
			{
				AActor* EditorActor = EditorUtilities::GetEditorWorldCounterpartActor( Actor );
				if ( !EditorActor || !GEditor->ActorsThatWereSelected.Contains(EditorActor) )
				{
					GEditor->ActorsThatWereSelected.Empty();
				}
			}
		}
		else if ( GEditor->ActorsThatWereSelected.Num() > 0 )
		{
			// Clear the selection set
			GEditor->ActorsThatWereSelected.Empty();
		}
	}

	// Don't do auto-switch-to-cover-editing when in Matinee, as it will exit Matinee in a bad way.
	//@TODO: Is this still a valid limitation, especially given that cover editing does not exist?
	if( !IsModeActive(FBuiltinEditorModes::EM_InterpEdit) )
	{
		for (int32 ModeIndex = 0; ModeIndex < Modes.Num(); ++ModeIndex)
		{
			if (FEdMode* Mode = Modes[ModeIndex].Get())
			{
				Mode->PeekAtSelectionChangedEvent(NewSelection);
			}
		}
	}
}

void FEditorModeTools::OnEditorSelectNone()
{
	GEditor->SelectNone( false, true );
	GEditor->ActorsThatWereSelected.Empty();
}

/** 
 * Returns a list of active modes that are incompatible with the passed in mode.
 * 
 * @param InID 				The mode to check for incompatibilites.
 * @param IncompatibleModes	The list of incompatible modes.
 */
void FEditorModeTools::GetIncompatibleActiveModes( FEditorModeID InID, TArray<FEdMode*>& IncompatibleModes )
{
	// Find all modes which are compatible with the one passed in.
	TArray<FEditorModeID> CompatibleModes;
	ModeCompatabilityMap.MultiFind( InID, CompatibleModes );

	// Search through all active modes for incompatible ones
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		FEdMode* ActiveMode = ActiveModes[ModeIndex];

		if( !CompatibleModes.Contains( ActiveMode->GetID() ) )
		{
			// This active mode is not compatible with the one that was passed in.
			IncompatibleModes.Add ( ActiveMode );
		}
	}
}

/** 
 * Sets the pivot locations
 * 
 * @param Location 		The location to set
 * @param bIncGridBase	Whether or not to also set the GridBase
 */
void FEditorModeTools::SetPivotLocation( const FVector& Location, const bool bIncGridBase )
{
	CachedLocation = PivotLocation = SnappedLocation = Location;
	if ( bIncGridBase )
	{
		GridBase = Location;
	}
}

ECoordSystem FEditorModeTools::GetCoordSystem(bool bGetRawValue)
{
	if (!bGetRawValue && GetWidgetMode() == FWidget::WM_Scale )
	{
		return COORD_Local;
	}
	else
	{
		return CoordSystem;
	}
}

void FEditorModeTools::SetCoordSystem(ECoordSystem NewCoordSystem)
{
	CoordSystem = NewCoordSystem;
}

/**
 * Deactivates an editor mode. 
 * 
 * @param InID		The ID of the editor mode to deactivate.
 */
void FEditorModeTools::DeactivateMode( FEditorModeID InID )
{
	// If the mode is already inactive do nothing
	if( !IsModeActive(InID) )
	{
		return;
	}

	// Find the active mode from the ID and exit it.
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		FEdMode* Mode = ActiveModes[ModeIndex];
		if( Mode->GetID() == InID )
		{
			Mode->Exit();
			ActiveModes.Remove( Mode );
			break;
		}
	}

	if( ActiveModes.Num() == 0 )
	{
		// Ensure the default mode is active if there are no active modes.
		ActivateMode( FBuiltinEditorModes::EM_Default );
	}
}

void FEditorModeTools::DeactivateAllModes()
{
	TArray< FEdMode* > CurrentActiveModes;
	GetActiveModes(CurrentActiveModes);

	for( FEdMode* Mode : CurrentActiveModes )
	{
		DeactivateMode(Mode->GetID());
	}
}

/**
 * Activates an editor mode. Shuts down all other active modes which cannot run with the passed in mode.
 * 
 * @param InID		The ID of the editor mode to activate.
 * @param bToggle	true if the passed in editor mode should be toggled off if it is already active.
 */
void FEditorModeTools::ActivateMode( FEditorModeID InID, bool bToggle )
{
	// Check to see if the mode is already active
	if( IsModeActive(InID) )
	{
		// The mode is already active toggle it off if we should toggle off already active modes.
		if( bToggle )
		{
			DeactivateMode( InID );
		}
		// Nothing more to do
		return;
	}

	// Get a list of modes which are incompatible with the one we are trying to activate
	TArray<FEdMode*> IncompatibleModes;
	GetIncompatibleActiveModes( InID, IncompatibleModes );

	// Exit all incompatible modes
	for( int32 ModeIndex = 0; ModeIndex < IncompatibleModes.Num(); ++ModeIndex )
	{
		FEdMode* Mode = IncompatibleModes[ModeIndex];
		if ( ActiveModes.Contains( Mode ) )
		{
			Mode->Exit();
			ActiveModes.Remove( Mode );
		}
	}

	// Find the new mode from its ID
	FEdMode* NewMode = FindMode( InID );

	if( !NewMode )
	{
		// Couldn't find a valid mode. Activate the default mode
		UE_LOG(LogEditorModes, Log,  TEXT("FEditorModeTools::SetCurrentMode : Couldn't find mode '%s'.  Using default."), *InID.ToString() );
		// We must call this function again to remove any other active modes that aren't compatible with the default
		ActivateMode( FBuiltinEditorModes::EM_Default );
	}
	else
	{
		// Add the new mode to the list of active ones
		ActiveModes.Add( NewMode );

		// Enter the new mode
		NewMode->Enter();
	
		// Update the editor UI
		FEditorSupportDelegates::UpdateUI.Broadcast();
	}
}

bool FEditorModeTools::EnsureNotInMode(FEditorModeID ModeID, const FText& ErrorMsg, bool bNotifyUser) const
{
	// We're in a 'safe' mode if we're not in the specified mode.
	const bool bInASafeMode = !IsModeActive(ModeID);
	if( !bInASafeMode && !ErrorMsg.IsEmpty() )
	{
		// Do we want to display this as a notification or a dialog to the user
		if ( bNotifyUser )
		{
			FNotificationInfo Info( ErrorMsg );
			FSlateNotificationManager::Get().AddNotification( Info );
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, ErrorMsg );
		}		
	}
	return bInASafeMode;
}

FEdMode* FEditorModeTools::FindMode( FEditorModeID InID )
{
	for( int32 x = 0 ; x < Modes.Num() ; x++ )
	{
		if( Modes[x]->ID == InID )
		{
			return Modes[x].Get();
		}
	}

	return NULL;
}

/**
 * Returns a coordinate system that should be applied on top of the worldspace system.
 */

FMatrix FEditorModeTools::GetCustomDrawingCoordinateSystem()
{
	FMatrix matrix = FMatrix::Identity;

	switch( GetCoordSystem() )
	{
		case COORD_Local:
		{
			// Let the current mode have a shot at setting the local coordinate system.
			// If it doesn't want to, create it by looking at the currently selected actors list.

			bool CustomCoordinateSystemProvided = false;
			for (int Index = 0; Index < ActiveModes.Num(); Index++)
			{
				if( ActiveModes[Index]->GetCustomDrawingCoordinateSystem( matrix, NULL ) )
				{
					CustomCoordinateSystemProvided = true;
					break;
				}
			}

			if ( !CustomCoordinateSystemProvided )
			{
				const int32 Num = GEditor->GetSelectedActors()->CountSelections<AActor>();

				// Coordinate system needs to come from the last actor selected
				if( Num > 0 )
				{
					matrix = FRotationMatrix( GEditor->GetSelectedActors()->GetBottom<AActor>()->GetActorRotation() );
				}
			}

			if(!matrix.Equals(FMatrix::Identity))
			{
				matrix.RemoveScaling();
			}
		}
		break;

		case COORD_World:
			break;

		default:
			break;
	}

	return matrix;
}

FMatrix FEditorModeTools::GetCustomInputCoordinateSystem()
{
	return GetCustomDrawingCoordinateSystem();
}

/** Gets the widget axis to be drawn */
EAxisList::Type FEditorModeTools::GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const
{
	EAxisList::Type OutAxis = EAxisList::All;
	for (int Index = ActiveModes.Num() - 1; Index >= 0 ; Index--)
	{
		if ( ActiveModes[Index]->ShouldDrawWidget() )
		{
			OutAxis = ActiveModes[Index]->GetWidgetAxisToDraw( InWidgetMode );
			break;
		}
	}

	return OutAxis;
}

/** Mouse tracking interface.  Passes tracking messages to all active modes */
bool FEditorModeTools::StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bIsTracking = true;
	bool bTransactionHandled = false;

	CachedLocation = PivotLocation;	// Cache the pivot location

	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bTransactionHandled |= ActiveModes[ModeIndex]->StartTracking(InViewportClient, InViewport);
	}

	return bTransactionHandled;
}

/** Mouse tracking interface.  Passes tracking messages to all active modes */
bool FEditorModeTools::EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bIsTracking = false;
	bool bTransactionHandled = false;

	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bTransactionHandled |= ActiveModes[ModeIndex]->EndTracking(InViewportClient, InViewportClient->Viewport);
	}

	CachedLocation = PivotLocation;	// Clear the pivot location
	
	return bTransactionHandled;
}

/** Notifies all active modes that a map change has occured */
void FEditorModeTools::MapChangeNotify()
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->MapChangeNotify();
	}
}


/** Notifies all active modes to empty their selections */
void FEditorModeTools::SelectNone()
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->SelectNone();
	}
}

/** Notifies all active modes of box selection attempts */
bool FEditorModeTools::BoxSelect( FBox& InBox, bool InSelect )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->BoxSelect( InBox, InSelect );
	}
	return bHandled;
}

/** Notifies all active modes of frustum selection attempts */
bool FEditorModeTools::FrustumSelect( const FConvexVolume& InFrustum, bool InSelect )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->FrustumSelect( InFrustum, InSelect );
	}
	return bHandled;
}


/** true if any active mode uses a transform widget */
bool FEditorModeTools::UsesTransformWidget() const
{
	bool bUsesTransformWidget = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bUsesTransformWidget |= ActiveModes[ModeIndex]->UsesTransformWidget();
	}

	return bUsesTransformWidget;
}

/** true if any active mode uses the passed in transform widget */
bool FEditorModeTools::UsesTransformWidget( FWidget::EWidgetMode CheckMode ) const
{
	bool bUsesTransformWidget = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bUsesTransformWidget |= ActiveModes[ModeIndex]->UsesTransformWidget(CheckMode);
	}

	return bUsesTransformWidget;
}

/** Sets the current widget axis */
void FEditorModeTools::SetCurrentWidgetAxis( EAxisList::Type NewAxis )
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->SetCurrentWidgetAxis( NewAxis );
	}
}

/** Notifies all active modes of mouse click messages. */
bool FEditorModeTools::HandleClick(FLevelEditorViewportClient* InViewportClient,  HHitProxy *HitProxy, const FViewportClick& Click )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->HandleClick(InViewportClient, HitProxy, Click);
	}

	return bHandled;
}

/** true if the passed in brush actor should be drawn in wireframe */	
bool FEditorModeTools::ShouldDrawBrushWireframe( AActor* InActor ) const
{
	bool bShouldDraw = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bShouldDraw |= ActiveModes[ModeIndex]->ShouldDrawBrushWireframe( InActor );
	}

	if( ActiveModes.Num() == 0 )
	{
		// We can get into a state where there are no active modes at editor startup if the builder brush is created before the default mode is activated.
		// Ensure we can see the builder brush when no modes are active.
		bShouldDraw = true;
	}
	return bShouldDraw;
}

/** true if brush vertices should be drawn */
bool FEditorModeTools::ShouldDrawBrushVertices() const
{
	// Currently only geometry mode being active prevents vertices from being drawn.
	return !IsModeActive( FBuiltinEditorModes::EM_Geometry );
}

/** Ticks all active modes */
void FEditorModeTools::Tick( FLevelEditorViewportClient* ViewportClient, float DeltaTime )
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->Tick( ViewportClient, DeltaTime );
	}
}

/** Notifies all active modes of any change in mouse movement */
bool FEditorModeTools::InputDelta( FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->InputDelta( InViewportClient, InViewport, InDrag, InRot, InScale );
	}
	return bHandled;
}

/** Notifies all active modes of captured mouse movement */	
bool FEditorModeTools::CapturedMouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->CapturedMouseMove( InViewportClient, InViewport, InMouseX, InMouseY );
	}
	return bHandled;
}

/** Notifies all active modes of keyboard input */
bool FEditorModeTools::InputKey(FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->InputKey( InViewportClient, Viewport, Key, Event );
	}
	return bHandled;
}

/** Notifies all active modes of axis movement */
bool FEditorModeTools::InputAxis(FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime)
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->InputAxis( InViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime );
	}
	return bHandled;
}

bool FEditorModeTools::MouseEnter( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->MouseEnter( InViewportClient, Viewport, X, Y );
	}
	return bHandled;
}

bool FEditorModeTools::MouseLeave( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->MouseLeave( InViewportClient, Viewport );
	}
	return bHandled;
}

/** Notifies all active modes that the mouse has moved */
bool FEditorModeTools::MouseMove( FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 X, int32 Y )
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->MouseMove( InViewportClient, Viewport, X, Y );
	}
	return bHandled;
}

/** Draws all active mode components */	
void FEditorModeTools::DrawActiveModes( const FSceneView* InView, FPrimitiveDrawInterface* PDI )
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->Draw( InView, PDI );
	}
}

/** Renders all active modes */
void FEditorModeTools::Render( const FSceneView* InView, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->Render( InView, Viewport, PDI );
	}
}

/** Draws the HUD for all active modes */
void FEditorModeTools::DrawHUD( FLevelEditorViewportClient* InViewportClient,FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->DrawHUD( InViewportClient, Viewport, View, Canvas );
	}
}

/** Calls PostUndo on all active components */
void FEditorModeTools::PostUndo()
{
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		ActiveModes[ModeIndex]->PostUndo();
	}
}

/** true if we should allow widget move */
bool FEditorModeTools::AllowWidgetMove() const
{
	bool bAllow = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bAllow |= ActiveModes[ModeIndex]->AllowWidgetMove();
	}
	return bAllow;
}

bool FEditorModeTools::DisallowMouseDeltaTracking() const
{
	bool bDisallow = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bDisallow |= ActiveModes[ModeIndex]->DisallowMouseDeltaTracking();
	}
	return bDisallow;
}

bool FEditorModeTools::GetCursor(EMouseCursor::Type& OutCursor) const
{
	bool bHandled = false;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		bHandled |= ActiveModes[ModeIndex]->GetCursor(OutCursor);
	}
	return bHandled;
}

/**
 * Used to cycle widget modes
 */
void FEditorModeTools::CycleWidgetMode (void)
{
	//make sure we're not currently tracking mouse movement.  If we are, changing modes could cause a crash due to referencing an axis/plane that is incompatible with the widget
	for(int32 ViewportIndex = 0;ViewportIndex < GEditor->LevelViewportClients.Num();ViewportIndex++)
	{
		FLevelEditorViewportClient* ViewportClient = GEditor->LevelViewportClients[ ViewportIndex ];
		if (ViewportClient->IsTracking())
		{
			return;
		}
	}

	//only cycle when the mode is requesting the drawing of a widgeth
	if( GetShowWidget() )
	{
		const int32 CurrentWk = GetWidgetMode();
		int32 Wk = CurrentWk;
		do
		{
			Wk++;
			if ((Wk == FWidget::WM_TranslateRotateZ) && (!GEditor->GetEditorUserSettings().bAllowTranslateRotateZWidget))
			{
				Wk++;
			}
			// Roll back to the start if we go past FWidget::WM_Scale
			if( Wk >= FWidget::WM_Max)
			{
				Wk -= FWidget::WM_Max;
			}
		}
		while (!GEditorModeTools().UsesTransformWidget((FWidget::EWidgetMode)Wk) && Wk != CurrentWk);
		SetWidgetMode( (FWidget::EWidgetMode)Wk );
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	}
}

/**Save Widget Settings to Ini file*/
void FEditorModeTools::SaveWidgetSettings(void)
{
	GEditor->SaveEditorUserSettings();
}

/**Load Widget Settings from Ini file*/
void FEditorModeTools::LoadWidgetSettings(void)
{
}

/**
 * Returns a good location to draw the widget at.
 */

FVector FEditorModeTools::GetWidgetLocation() const
{
	for (int Index = ActiveModes.Num() - 1; Index >= 0 ; Index--)
	{
		if ( ActiveModes[Index]->UsesTransformWidget() )
		{
			 return ActiveModes[Index]->GetWidgetLocation();
		}
	}
	
	return FVector(EForceInit::ForceInitToZero);
}

/**
 * Changes the current widget mode.
 */

void FEditorModeTools::SetWidgetMode( FWidget::EWidgetMode InWidgetMode )
{
	WidgetMode = InWidgetMode;
}

/**
 * Allows you to temporarily override the widget mode.  Call this function again
 * with FWidget::WM_None to turn off the override.
 */

void FEditorModeTools::SetWidgetModeOverride( FWidget::EWidgetMode InWidgetMode )
{
	OverrideWidgetMode = InWidgetMode;
}

/**
 * Retrieves the current widget mode, taking overrides into account.
 */

FWidget::EWidgetMode FEditorModeTools::GetWidgetMode() const
{
	if( OverrideWidgetMode != FWidget::WM_None )
	{
		return OverrideWidgetMode;
	}

	return WidgetMode;
}

bool FEditorModeTools::GetShowFriendlyVariableNames() const
{
	return GetDefault<UEditorStyleSettings>()->bShowFriendlyNames;
}

/**
 * Sets a bookmark in the levelinfo file, allocating it if necessary.
 */

void FEditorModeTools::SetBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient )
{
	UWorld* World = InViewportClient->GetWorld();
	if ( World )
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();

		// Verify the index is valid for the bookmark
		if ( WorldSettings && InIndex < AWorldSettings::MAX_BOOKMARK_NUMBER )
		{
			// If the index doesn't already have a bookmark in place, create a new one
			if ( !WorldSettings->BookMarks[ InIndex ] )
			{
				WorldSettings->BookMarks[ InIndex ] = ConstructObject<UBookMark>( UBookMark::StaticClass(), WorldSettings );
			}

			UBookMark* CurBookMark = WorldSettings->BookMarks[ InIndex ];
			check(CurBookMark);
			check(InViewportClient);

			// Use the rotation from the first perspective viewport can find.
			FRotator Rotation(0,0,0);
			if( !InViewportClient->IsOrtho() )
			{
				Rotation = InViewportClient->GetViewRotation();
			}

			CurBookMark->Location = InViewportClient->GetViewLocation();
			CurBookMark->Rotation = Rotation;

			// Keep a record of which levels were hidden so that we can restore these with the bookmark
			CurBookMark->HiddenLevels.Empty();
			for ( int32 LevelIndex = 0 ; LevelIndex < World->StreamingLevels.Num() ; ++LevelIndex )
			{
				ULevelStreaming* StreamingLevel = World->StreamingLevels[LevelIndex];
				if ( StreamingLevel )
				{
					if( !StreamingLevel->bShouldBeVisibleInEditor )
					{
						CurBookMark->HiddenLevels.Add( StreamingLevel->GetFullName() );
					}
				}
			}
		}
	}
}

/**
 * Checks to see if a bookmark exists at a given index
 */

bool FEditorModeTools::CheckBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient )
{
	UWorld* World = InViewportClient->GetWorld();
	if ( World )
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();
		if ( WorldSettings && InIndex < AWorldSettings::MAX_BOOKMARK_NUMBER && WorldSettings->BookMarks[ InIndex ] )
		{
			return ( WorldSettings->BookMarks[ InIndex ] ? true : false );
		}
	}

	return false;
}

/**
 * Retrieves a bookmark from the list.
 */

void FEditorModeTools::JumpToBookmark( uint32 InIndex, bool bShouldRestoreLevelVisibility, FLevelEditorViewportClient* InViewportClient )
{
	UWorld* World = InViewportClient->GetWorld();
	if ( World )
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();

		// Can only jump to a pre-existing bookmark
		if ( WorldSettings && InIndex < AWorldSettings::MAX_BOOKMARK_NUMBER && WorldSettings->BookMarks[ InIndex ] )
		{
			const UBookMark* CurBookMark = WorldSettings->BookMarks[ InIndex ];
			check(CurBookMark);

			// Set all level editing cameras to this bookmark
			for( int32 v = 0 ; v < GEditor->LevelViewportClients.Num() ; v++ )
			{
				GEditor->LevelViewportClients[v]->SetViewLocation( CurBookMark->Location );
				if( !GEditor->LevelViewportClients[v]->IsOrtho() )
				{
					GEditor->LevelViewportClients[v]->SetViewRotation( CurBookMark->Rotation );
				}
				GEditor->LevelViewportClients[v]->Invalidate();
			}
		}
	}
}


/**
 * Clears a bookmark
 */
void FEditorModeTools::ClearBookmark( uint32 InIndex, FLevelEditorViewportClient* InViewportClient )
{
	UWorld* World = InViewportClient->GetWorld();
	if( World )
	{
		AWorldSettings* pWorldSettings = World->GetWorldSettings();

		// Verify the index is valid for the bookmark
		if ( pWorldSettings && InIndex < AWorldSettings::MAX_BOOKMARK_NUMBER )
		{
			pWorldSettings->BookMarks[ InIndex ] = NULL;
		}
	}
}

/**
* Clears all book marks
*/
void FEditorModeTools::ClearAllBookmarks( FLevelEditorViewportClient* InViewportClient )
{
	for( int i = 0; i <  AWorldSettings::MAX_BOOKMARK_NUMBER; ++i )
	{
		ClearBookmark( i , InViewportClient );
	}
}

/**
 * Serializes the components for all modes.
 */

void FEditorModeTools::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( int32 x = 0 ; x < Modes.Num() ; ++x )
	{
		Modes[x]->AddReferencedObjects( Collector );
	}
}

/**
 * Returns a pointer to an active mode specified by the passed in ID
 * If the editor mode is not active, NULL is returned
 */
FEdMode* FEditorModeTools::GetActiveMode( FEditorModeID InID )
{
	FEdMode* ActiveMode = NULL;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		if( ActiveModes[ModeIndex]->GetID() == InID )
		{
			ActiveMode = ActiveModes[ModeIndex];
			break;
		}
	}
	return ActiveMode;
}

/**
 * Returns a pointer to an active mode specified by the passed in ID
 * If the editor mode is not active, NULL is returned
 */
const FEdMode* FEditorModeTools::GetActiveMode( FEditorModeID InID ) const
{
	const FEdMode* ActiveMode = NULL;
	for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
	{
		if( ActiveModes[ModeIndex]->GetID() == InID )
		{
			ActiveMode = ActiveModes[ModeIndex];
			break;
		}
	}
	return ActiveMode;
}

/**
 * Returns the active tool of the passed in editor mode.
 * If the passed in editor mode is not active or the mode has no active tool, NULL is returned
 */
const FModeTool* FEditorModeTools::GetActiveTool( FEditorModeID InID ) const
{
	const FEdMode* ActiveMode = GetActiveMode( InID );
	const FModeTool* Tool = NULL;
	if( ActiveMode )
	{
		Tool = ActiveMode->GetCurrentTool();
	}
	return Tool;
}

/** 
 * Returns true if the passed in editor mode is active 
 */
bool FEditorModeTools::IsModeActive( FEditorModeID InID ) const
{
	return GetActiveMode( InID ) != NULL;
}

/** 
 * Returns an array of all active modes
 */
void FEditorModeTools::GetActiveModes( TArray<FEdMode*>& OutActiveModes )
{
	OutActiveModes.Empty();
	// Copy into an array.  Do not let users modify the active list directly.
	OutActiveModes = ActiveModes;
}

void FEditorModeTools::GetModes( TArray<FEdMode*>& OutModes )
{
	OutModes.Empty();
	// Copy into an array.  Do not let users modify the modes list directly.
	for(TSharedPtr<FEdMode> Mode : Modes)
	{
		OutModes.Add(Mode.Get());
	}
}
