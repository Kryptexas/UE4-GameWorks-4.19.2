// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "UMGComponent.h"
#include "HittestGrid.h"
#include "ISlateRHIRendererModule.h"
#include "ISlate3DRenderer.h"
#include "DynamicMeshBuilder.h"


class SVirtualWindow : public SWindow
{
	SLATE_BEGIN_ARGS( SVirtualWindow )
		: _Size( FVector2D(100,100) )
	{}

		SLATE_ARGUMENT( FVector2D, Size )
	SLATE_END_ARGS()

public:
	void Construct( const FArguments& InArgs )
	{
		bIsPopupWindow = true;
		SetCachedSize( InArgs._Size );
		SetNativeWindow( MakeShareable( new FGenericWindow() ) );
		SetContent( SNullWidget::NullWidget );
	}
};


/** Represents a billboard sprite to the scene manager. */
class FUMG3DSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Initialization constructor. */
	FUMG3DSceneProxy(const UUMGComponent* InComponent, ISlate3DRenderer& InRenderer )
		: FPrimitiveSceneProxy(InComponent)
		, Renderer( InRenderer )
		, RenderTarget( InComponent->GetRenderTarget() )
		, MaterialInstance( InComponent->GetMaterialInstance() )
		, BodySetup( const_cast<UUMGComponent*>( InComponent )->GetBodySetup() )
	{
		bWillEverBeLit = false;
	}

	~FUMG3DSceneProxy()
	{
	}

	// FPrimitiveSceneProxy interface.
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		const FMatrix& LocalToWorld = GetLocalToWorld();
	
		FDynamicMeshBuilder MeshBuilder;

		if( RenderTarget )
		{
			FTextureResource* TextureResource = RenderTarget->Resource;
			if(TextureResource)
			{
				float U = 0.0f;
				float V = 0.0f;
				float UL = RenderTarget->SizeX;
				float VL = RenderTarget->SizeY;

				int32 VertexIndices[4];

				VertexIndices[0] = MeshBuilder.AddVertex(FVector(U, V, 0), FVector2D(0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[1] = MeshBuilder.AddVertex(FVector(U, VL, 0), FVector2D(0, 1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[2] = MeshBuilder.AddVertex(FVector(UL, VL, 0), FVector2D(1, 1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[3] = MeshBuilder.AddVertex(FVector(UL, V, 0), FVector2D(1, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);

				MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[1], VertexIndices[2]);
				MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[2], VertexIndices[3]);
			}


			for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if(VisibilityMap & (1 << ViewIndex))
				{
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					MeshBuilder.Draw(PDI, LocalToWorld, MaterialInstance->GetRenderProxy(IsSelected()), SDPG_World);
				}
			}
		}

	}
	/** 
	 * Draw the scene proxy as a dynamic element
	 *
	 * @param	PDI - draw interface to render to
	 * @param	View - current view
	 */
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override
	{
		if( RenderTarget )
		{
			FTextureResource* TextureResource = RenderTarget->Resource;
			if(TextureResource)
			{
				float U = 0.0f;
				float V = 0.0f;
				float UL = RenderTarget->SizeX;
				float VL = RenderTarget->SizeY;

				FDynamicMeshBuilder MeshBuilder;

				int32 VertexIndices[4];

				VertexIndices[0] = MeshBuilder.AddVertex(FVector(U, V, 0), FVector2D(0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[1] = MeshBuilder.AddVertex(FVector(U, VL, 0), FVector2D(0, 1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[2] = MeshBuilder.AddVertex(FVector(UL, VL, 0), FVector2D(1, 1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
				VertexIndices[3] = MeshBuilder.AddVertex(FVector(UL, V, 0), FVector2D(1, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);

				MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[1], VertexIndices[2]);
				MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[2], VertexIndices[3]);

				MeshBuilder.Draw(PDI, GetLocalToWorld(), MaterialInstance->GetRenderProxy(IsSelected()), SDPG_World);

			}
		}

		//FColor CollisionColor = FColor(157,149,223,255);
		//FTransform GeomTransform(GetLocalToWorld());
		//BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(CollisionColor, false, IsHovered()), nullptr, false, false, true );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), IsSelected());
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override
	{
		bool bVisible = true;// View->Family->EngineShowFlags.BillboardSprites;

		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && bVisible;
		Result.bOpaqueRelevance = false;
		Result.bNormalTranslucencyRelevance = true;
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);;
		Result.bEditorPrimitiveRelevance = false;
		return Result;
	}

	virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override
	{
		bDynamic = false;
		bRelevant = false;
		bLightMapped = false;
		bShadowMapped = false;
	}

	virtual void OnTransformChanged() override
	{
		Origin = GetLocalToWorld().GetOrigin();
	}

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
	FVector Origin;
	ISlate3DRenderer& Renderer;
	UTextureRenderTarget2D* RenderTarget;
	UMaterialInstanceDynamic* MaterialInstance;
	UBodySetup* BodySetup;
};

/**
* The hit tester used by all UMG Component objects.
*/
class FUMG3DHitTester : public ICustomHitTestPath
{
public:
	FUMG3DHitTester( UWorld* InWorld )
		: World( InWorld )
	{}

	// ICustomHitTestPath implementation
	virtual TArray<FArrangedWidget> GetBubblePath(const FGeometry& InGeometry, FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus) const override
	{
		if( World.IsValid() && ensure( World->IsGameWorld() ) )
		{
			ULocalPlayer* const TargetPlayer = GEngine->GetLocalPlayerFromControllerId(World.Get(), 0);
			if (TargetPlayer && TargetPlayer->PlayerController)
			{
				FHitResult HitResult;
				if (TargetPlayer->PlayerController->GetHitResultAtScreenPosition(InGeometry.AbsoluteToLocal(DesktopSpaceCoordinate), ECC_Visibility, true, HitResult))
				{
					UUMGComponent* UMGComponent = Cast<UUMGComponent>(HitResult.Component.Get());
					if (UMGComponent)
					{
						return UMGComponent->GetHitWidgetPath(HitResult, bIgnoreEnabledStatus);
					}
				}
			}
		}
		return TArray<FArrangedWidget>();
	}

	virtual void ArrangeChildren( FArrangedChildren& ArrangedChildren ) const override
	{
		for( TWeakObjectPtr<UUMGComponent> Component : RegisteredComponents )
		{
			UUMGComponent* UMGComponent = Component.Get();
			// Check if visible;
			if( UMGComponent && UMGComponent->GetSlateWidget().IsValid() )
			{
				FGeometry WidgetGeom;

				ArrangedChildren.AddWidget( FArrangedWidget( UMGComponent->GetSlateWidget().ToSharedRef(), WidgetGeom.MakeChild( UMGComponent->GetDrawSize(), FSlateLayoutTransform() ) ) );
			}
		}
	}

	virtual TSharedPtr<struct FVirtualCursorPosition> TranslateMouseCoordinateFor3DChild( const TSharedRef<SWidget>& ChildWidget, const FGeometry& ViewportGeometry, const FVector2D& ScreenSpaceMouseCoordinate, const FVector2D& LastScreenSpaceMouseCoordinate ) const override
	{
		if(World.IsValid() && ensure(World->IsGameWorld()))
		{
			ULocalPlayer* const TargetPlayer = GEngine->GetLocalPlayerFromControllerId(World.Get(), 0);
			if(TargetPlayer && TargetPlayer->PlayerController)
			{
				// Check for a hit against any umg components in the world
				for(TWeakObjectPtr<UUMGComponent> Component : RegisteredComponents)
				{
					UUMGComponent* UMGComponent = Component.Get();
					// Check if visible;
					if(UMGComponent && UMGComponent->GetSlateWidget() == ChildWidget)
					{
						FVector2D LocalMouseCoordinate = ViewportGeometry.AbsoluteToLocal(ScreenSpaceMouseCoordinate);
						FVector2D LocalLastMouseCoordinate = ViewportGeometry.AbsoluteToLocal( LastScreenSpaceMouseCoordinate );


						FHitResult HitResult;
						if(TargetPlayer->PlayerController->GetHitResultAtScreenPosition(LocalMouseCoordinate, ECC_Visibility, true, HitResult))
						{
							if(UMGComponent == HitResult.Component.Get())
							{
								TSharedPtr<FVirtualCursorPosition> VirtualCursorPos = MakeShareable(new FVirtualCursorPosition);

								FVector2D LocalHitLocation = FVector2D(UMGComponent->ComponentToWorld.InverseTransformPosition(HitResult.Location));

								VirtualCursorPos->CurrentCursorPosition = LocalHitLocation;
								VirtualCursorPos->LastCursorPosition = LocalHitLocation;

								return VirtualCursorPos;
							}
						}
					}
				}
			}
		}

		return nullptr;
	}
	// End ICustomHitTestPath

	void RegisterUMGComponent( UUMGComponent* InComponent )
	{
		RegisteredComponents.AddUnique( InComponent );
	}

	void UnregisterUMGComponent( UUMGComponent* InComponent )
	{
		RegisteredComponents.RemoveSingleSwap( InComponent );
	}

	uint32 GetNumRegisteredComponents() const { return RegisteredComponents.Num(); }
	
	UWorld* GetWorld() const { return World.Get(); }
private:
	TArray< TWeakObjectPtr<UUMGComponent> > RegisteredComponents;
	TWeakObjectPtr<UWorld> World;
};

UUMGComponent::UUMGComponent(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	DrawSize = FIntPoint(500,500);

	BodyInstance.SetCollisionProfileName(FName(TEXT("UI")));

	static ConstructorHelpers::FObjectFinder<UMaterial> Material( TEXT("/Engine/EngineMaterials/UMG3DPassThrough") );

	BaseMaterial = Material.Object;

	LastLocalHitLocation = FVector2D::ZeroVector;
	//bGenerateOverlapEvents = false;
	bUseEditorCompositing = false;
}

FPrimitiveSceneProxy* UUMGComponent::CreateSceneProxy()
{
	return new FUMG3DSceneProxy(this,*Renderer);
}

FBoxSphereBounds UUMGComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds( FVector( DrawSize.X/2.0f, DrawSize.Y/2.0f, .5f), FVector(DrawSize.X/2,DrawSize.Y/2,1.0f), DrawSize.Size()/2 ).TransformBy( LocalToWorld );
}

UBodySetup* UUMGComponent::GetBodySetup() 
{
	UpdateBodySetup();
	return BodySetup;
}

FCollisionShape UUMGComponent::GetCollisionShape(float Inflation) const
{
	FVector	Extent = ( FVector( DrawSize.Y, DrawSize.Y, 1.0f ) * ComponentToWorld.GetScale3D() ) + Inflation;
	if( Inflation < 0.0f )
	{
		// Don't shrink below zero size.
		Extent = Extent.ComponentMax(FVector::ZeroVector);
	}

	return FCollisionShape::MakeBox(Extent);
}

void UUMGComponent::OnRegister()
{
	Super::OnRegister();

	if( GetWorld()->IsGameWorld() )
	{
		TSharedPtr<SViewport> GameViewportWidget = GEngine->GetGameViewportWidget();

		if( GameViewportWidget.IsValid() )
		{
			TSharedPtr<ICustomHitTestPath> CustomHitTestPath = GameViewportWidget->GetCustomHitTestPath();
			if( !CustomHitTestPath.IsValid() )
			{
				CustomHitTestPath = MakeShareable(new FUMG3DHitTester(GetWorld()));
				GameViewportWidget->SetCustomHitTestPath( CustomHitTestPath );
			}

			TSharedPtr<FUMG3DHitTester> UMGHitTester = StaticCastSharedPtr<FUMG3DHitTester>(CustomHitTestPath);
			if( UMGHitTester->GetWorld() == GetWorld() )
			{
				UMGHitTester->RegisterUMGComponent( this );
			}

		}

	}

	if( !MaterialInstance )
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	}

	UpdateWidget();

	if( !Renderer.IsValid() )
	{
		Renderer = FModuleManager::Get().GetModuleChecked<ISlateRHIRendererModule>("SlateRHIRenderer").CreateSlate3DRenderer();
	}
}

void UUMGComponent::OnUnregister()
{
	if( GetWorld()->IsGameWorld() )
	{
		TSharedPtr<SViewport> GameViewportWidget = GEngine->GetGameViewportWidget();
		if( GameViewportWidget.IsValid() )
		{
			TSharedPtr<ICustomHitTestPath> CustomHitTestPath = GameViewportWidget->GetCustomHitTestPath();
			if( CustomHitTestPath.IsValid() )
			{
				TSharedPtr<FUMG3DHitTester> UMGHitTestPath = StaticCastSharedPtr<FUMG3DHitTester>(CustomHitTestPath);

				UMGHitTestPath->UnregisterUMGComponent( this );

				if( UMGHitTestPath->GetNumRegisteredComponents() == 0 )
				{
					GameViewportWidget->SetCustomHitTestPath( nullptr );
				}
			}
		}
	}

	if( Widget )
	{
		Widget->MarkPendingKill();
		Widget = NULL;
	}

	SlateWidget.Reset();
	Super::OnUnregister();
}

void UUMGComponent::DestroyComponent()
{
	Super::DestroyComponent();
	Renderer.Reset();
}

void UUMGComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	const float RenderTimeThreshold = .5f;
	if( IsVisible() && GetWorld()->TimeSince(LastRenderTime) <= RenderTimeThreshold ) // Don't bother ticking if it hasn't been rendered recently
	{
		SlateWidget->SlatePrepass();

		FGeometry WindowGeometry = FGeometry::MakeRoot( DrawSize, FSlateLayoutTransform() );

		SlateWidget->TickWidgetsRecursively(WindowGeometry, FApp::GetCurrentTime(), DeltaTime);

		// Ticking can cause geometry changes.  Recompute
		SlateWidget->SlatePrepass();

		// Get the free buffer & add our virtual window
		FSlateDrawBuffer& DrawBuffer = Renderer->GetDrawBuffer();
		FSlateWindowElementList& WindowElementList = DrawBuffer.AddWindowElementList(SlateWidget.ToSharedRef());

		// Prepare the test grid 
		HitTestGrid->BeginFrame(WindowGeometry.GetClippingRect());

		// Paint the window
		int32 MaxLayerId = 0;
		{
			MaxLayerId = SlateWidget->Paint(
				FPaintArgs(SlateWidget.ToSharedRef(), *HitTestGrid, FVector2D::ZeroVector),
				WindowGeometry, WindowGeometry.GetClippingRect(),
				WindowElementList,
				0,
				FWidgetStyle(),
				SlateWidget->IsEnabled());
		}

		Renderer->DrawWindow_GameThread(DrawBuffer);

		UpdateRenderTarget();

		// Enqueue a command to unlock the draw buffer after all windows have been drawn
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(UUMG3DComponentRenderToTexture,
			FSlateDrawBuffer&, InDrawBuffer, DrawBuffer,
			UTextureRenderTarget2D*, InRenderTarget, RenderTarget,
			ISlate3DRenderer*, InRenderer, Renderer.Get(),
			{
				InRenderer->DrawWindowToTarget_RenderThread( RHICmdList, InRenderTarget, InDrawBuffer );	
			});

	}

}

class FUMGComponentInstanceData : public FComponentInstanceDataBase
{
public:
	FUMGComponentInstanceData( const UUMGComponent* SourceComponent )
		: FComponentInstanceDataBase(SourceComponent)
		, WidgetClass ( SourceComponent->GetWidgetClass() )
		, RenderTarget( SourceComponent->GetRenderTarget() )
	{}

	virtual bool MatchesComponent( const UActorComponent* Component ) const override
	{
		return (CastChecked<UUMGComponent>(Component)->GetWidgetClass() == WidgetClass && FComponentInstanceDataBase::MatchesComponent(Component));
	}

public:
	TSubclassOf<UUserWidget> WidgetClass;
	UTextureRenderTarget2D* RenderTarget;
};

FName UUMGComponent::GetComponentInstanceDataType() const
{
	static const FName InstanceDataName(TEXT("UMGInstanceData"));
	return InstanceDataName;
}

TSharedPtr<FComponentInstanceDataBase> UUMGComponent::GetComponentInstanceData() const
{
	return MakeShareable( new FUMGComponentInstanceData( this ) );
}

void UUMGComponent::ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData)
{
	check(ComponentInstanceData.IsValid());

	// Note: ApplyComponentInstanceData is called while the component is registered so the rendering thread is already using this component
	// That means all component state that is modified here must be mirrored on the scene proxy, which will be recreated to receive the changes later due to MarkRenderStateDirty.
	TSharedPtr<FUMGComponentInstanceData> UMGInstanceData = StaticCastSharedPtr<FUMGComponentInstanceData>(ComponentInstanceData);

	RenderTarget = UMGInstanceData->RenderTarget;
	MaterialInstance->SetTextureParameterValue("SlateUI", RenderTarget);

	MarkRenderStateDirty();
}

#if WITH_EDITORONLY_DATA
void UUMGComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* Property = PropertyChangedEvent.MemberProperty;

	if( Property && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		static FName DrawSizeName("DrawSize");

		static FName WidgetClass("WidgetClass");

		if( Property->GetFName() == DrawSizeName || Property->GetFName() == WidgetClass )
		{
			UpdateWidget();

			if( Property->GetFName() == DrawSizeName )
			{
				UpdateBodySetup(true);
			}

			MarkRenderStateDirty();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUMGComponent::UpdateWidget()
{
	if(WidgetClass && !Widget && GetWorld() )
	{
		Widget = CreateWidget<UUserWidget>( GetWorld(), WidgetClass );
	}
	else if( !WidgetClass && Widget )
	{
		Widget = nullptr;
	}

	if(!SlateWidget.IsValid())
	{
		SlateWidget = SNew(SVirtualWindow).Size(DrawSize);

	}
	
	if( !HitTestGrid.IsValid() )
	{
		HitTestGrid = MakeShareable( new FHittestGrid );
	}

	{
		SlateWidget->Resize( DrawSize );

		if( Widget )
		{
			SlateWidget->SetContent( Widget->TakeWidget() );
		}
		else
		{
			SlateWidget->SetContent( SNullWidget::NullWidget );
		}

	}

	if( Widget && !GetWorld()->IsGameWorld() )
	{
		// Prevent native ticking of editor component previews
		Widget->SetIsDesignTime( true );
	}
}

void UUMGComponent::UpdateRenderTarget()
{
	if(!RenderTarget && DrawSize != FIntPoint::ZeroValue)
	{
		RenderTarget = ConstructObject<UTextureRenderTarget2D>(UTextureRenderTarget2D::StaticClass(), this);
		RenderTarget->InitCustomFormat(DrawSize.X, DrawSize.Y, PF_B8G8R8A8, false);
		MaterialInstance->SetTextureParameterValue("SlateUI", RenderTarget);
		MarkRenderStateDirty();
	}
	else if(DrawSize != FIntPoint::ZeroValue && (RenderTarget->SizeX != DrawSize.X || RenderTarget->SizeY != DrawSize.Y))
	{
		RenderTarget->InitCustomFormat(DrawSize.X, DrawSize.Y, PF_B8G8R8A8, false);
		MarkRenderStateDirty();
	}
}

void UUMGComponent::UpdateBodySetup( bool bDrawSizeChanged )
{
	if( !BodySetup || bDrawSizeChanged )
	{
		BodySetup = ConstructObject<UBodySetup>( UBodySetup::StaticClass(), this );
		BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		BodySetup->AggGeom.BoxElems.Add(FKBoxElem());

		FKBoxElem* BoxElem = BodySetup->AggGeom.BoxElems.GetData();

		BoxElem->SetTransform(FTransform::Identity);
		BoxElem->Center = FVector(DrawSize.X/2.0f, DrawSize.Y/2.0f, .5f);
		BoxElem->X = DrawSize.X;
		BoxElem->Y = DrawSize.Y;
		BoxElem->Z = 1.0f;
	}	
}

TArray<FArrangedWidget> UUMGComponent::GetHitWidgetPath( const FHitResult& HitResult, bool bIgnoreEnabledStatus  )
{
	FVector2D LocalHitLocation = FVector2D( ComponentToWorld.InverseTransformPosition( HitResult.Location ) );

	TSharedRef<FVirtualCursorPosition> VirtualMouseCoordinate = MakeShareable( new FVirtualCursorPosition );

	VirtualMouseCoordinate->CurrentCursorPosition = LocalHitLocation;
	VirtualMouseCoordinate->LastCursorPosition = LastLocalHitLocation;

	// Cache the location of the hit
	LastLocalHitLocation = LocalHitLocation;

	TArray<FArrangedWidget> ArrangedWidgets = HitTestGrid->GetBubblePath( LocalHitLocation, bIgnoreEnabledStatus );

	for( FArrangedWidget& Widget : ArrangedWidgets )
	{
		Widget.VirtualCursorPosition = VirtualMouseCoordinate;
	}

	return ArrangedWidgets;
}

TSharedPtr<SWidget> UUMGComponent::GetSlateWidget() const
{
	return SlateWidget;
}