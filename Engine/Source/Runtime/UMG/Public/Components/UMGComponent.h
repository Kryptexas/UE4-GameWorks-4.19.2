// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMGComponent.generated.h"

/**
* UMG Component is a primitive component used to render User Widgets in the game world.
* Functionally, it maps the user widget content to an SWindow that is in turn mapped to a quad (FKBoxElem) in the world via render target.
*/
UCLASS(hidecategories=(Object,Activation,"Components|Activation",Sockets,Base,Rendering,Lighting,LOD,Mesh), editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UUMGComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:
	/* UPrimitiveComponent Interface */
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual UBodySetup* GetBodySetup() override;
	virtual FCollisionShape GetCollisionShape(float Inflation) const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void DestroyComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual TSharedPtr<FComponentInstanceDataBase> GetComponentInstanceData() const override;
	virtual FName GetComponentInstanceDataType() const override;
	virtual void ApplyComponentInstanceData(TSharedPtr<class FComponentInstanceDataBase> ComponentInstanceData ) override;

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif

	/** Ensures the user widget is initialized and updates its size and content. */
	void UpdateWidget();

	/** Ensure the render target is initialized and updates it if needed. */
	void UpdateRenderTarget();

	/** 
	* Ensures the body setup is initialized and updates it if needed.
	* @param bDrawSizeChanged Whether the draw size of this component has changed since the last update call.
	*/
	void UpdateBodySetup( bool bDrawSizeChanged = false );

	/** @return The class of the user widget displayed by this component */
	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

	/** @return The user widget object displayed by this component */
	UUserWidget* GetUMGWidgetObject() const { return Widget; }

	/** @return  */
	TArray<FArrangedWidget> GetHitWidgetPath( const FHitResult& HitResult, bool bIgnoreEnabledStatus );

	/** @return The render target to which the user widget is rendered */
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; };

	/** @return The dynamic material instance used to render the user widget */
	UMaterialInstanceDynamic* GetMaterialInstance() const { return MaterialInstance; }

	/** @return The window containing the user widget content */
	TSharedPtr<SWidget> GetSlateWidget() const;

	/** @return The draw size of the quad in the world */
	const FIntPoint& GetDrawSize() const { return DrawSize; }

private:
	/** The size of the displayed quad */
	UPROPERTY(EditAnywhere, Category=UI)
	FIntPoint DrawSize;

	/** The class of User Widget to create and display an instance of */
	UPROPERTY(EditAnywhere, Category=UI)
	TSubclassOf<UUserWidget> WidgetClass;

	/** The User Widget object displayed and managed by this component */
	UPROPERTY(transient, duplicatetransient)
	UUserWidget* Widget;

	/** The body setup of the displayed quad */
	UPROPERTY(transient, duplicatetransient)
	class UBodySetup* BodySetup;

	/**  */
	UPROPERTY()
	UMaterial* BaseMaterial;

	/** The target to which the user widget is rendered */
	UPROPERTY(transient, duplicatetransient)
	UTextureRenderTarget2D* RenderTarget;

	/** The dynamic instance of the material that the render target is attached to */
	UPROPERTY(transient, duplicatetransient)
	UMaterialInstanceDynamic* MaterialInstance;

	/**  */
	TSharedPtr<class FHittestGrid> HitTestGrid;

	/** The slate 3D renderer used to render the user slate widget */
	TSharedPtr<class ISlate3DRenderer> Renderer;
	
	/** The slate window that contains the user widget content */
	TSharedPtr<class SVirtualWindow> SlateWidget;

	/** The relative location of the last hit on this component */
	FVector2D LastLocalHitLocation;

	/** The hit tester to use for this component */
	static TSharedPtr<class FUMG3DHitTester> UMGHitTester;
};

 

