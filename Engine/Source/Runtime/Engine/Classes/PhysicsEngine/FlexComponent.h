// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FlexContainer.h"
#include "FlexAsset.h"

#include "FlexComponent.generated.h"

struct FFlexContainerInstance;
class FFlexMeshSceneProxy;

struct NvFlexExtInstance;
struct NvFlexExtMovingFrame;
struct NvFlexExtTearingMeshEdit;

UCLASS(Blueprintable, hidecategories = (Object), meta=(BlueprintSpawnableComponent))
class ENGINE_API UFlexComponent : public UStaticMeshComponent, public IFlexContainerClient
{
	GENERATED_UCLASS_BODY()		

public:

	struct FlexParticleAttachment
	{
		TWeakObjectPtr<USceneComponent> Primitive;
		int32 ItemIndex;
		int32 ParticleIndex;
		float OldMass;
		FVector LocalPos;
	};

	/** Override the FlexAsset's container / phase / attachment properties */
	UPROPERTY(EditAnywhere, Category = Flex)
	bool OverrideAsset;

	/** The simulation container to spawn any flex data contained in the static mesh into */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Flex, meta=(editcondition = "OverrideAsset", ToolTip="If the static mesh has Flex data then it will be spawned into this simulation container."))
	UFlexContainer* ContainerTemplate;

	/** The phase to assign to particles spawned for this mesh */
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	FFlexPhase Phase;

	/** The per-particle mass to use for the particles, for clothing this value be multiplied by 0-1 dependent on the vertex color*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	float Mass;

	/** If true then the particles will be attached to any overlapping shapes on spawn*/
	UPROPERTY(EditAnywhere, Category = Flex, meta = (editcondition = "OverrideAsset"))
	bool AttachToRigids;

	/** Multiply the asset's over pressure amount for inflatable meshes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	float InflatablePressureMultiplier;

	/** Multiply the asset's max strain before tearing, this can be used to script breaking by lowering the max strain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	float TearingMaxStrainMultiplier;

	/** The number of tearing events that have occured */
	UPROPERTY(BlueprintReadWrite, Category = Flex)
	int32 TearingBreakCount;

	UFUNCTION(BlueprintNativeEvent, Category = Flex)
	void OnTear();
	
	/** Instance of a FlexAsset referencing particles and constraints in a solver */
	NvFlexExtInstance* AssetInstance;

	/* Clone of the cloth asset for tearing meshes */
	NvFlexExtAsset* TearingAsset;

	/* The simulation container the instance belongs to */
	FFlexContainerInstance* ContainerInstance; 

	/* Simulated particle positions  */
	TArray<FVector4> SimPositions;
	/* Simulated particle normals */
	TArray<FVector> SimNormals;

	/* Pre-simulated particle positions  */
	UPROPERTY(NonPIEDuplicateTransient)
	TArray<FVector> PreSimPositions;

	UPROPERTY(NonPIEDuplicateTransient)
	TArray<FVector> PreSimShapeTranslations;

	UPROPERTY(NonPIEDuplicateTransient)
	TArray<FQuat> PreSimShapeRotations;

	/* Pre-simulated transform of the component  */
	UPROPERTY(NonPIEDuplicateTransient)
	FVector PreSimRelativeLocation;

	UPROPERTY(NonPIEDuplicateTransient)
	FRotator PreSimRelativeRotation;

	UPROPERTY(NonPIEDuplicateTransient)
	FTransform PreSimTransform;

	/* transform of the component before keep simulation */
	UPROPERTY(NonPIEDuplicateTransient)
	FVector SavedRelativeLocation;

	UPROPERTY(NonPIEDuplicateTransient)
	FRotator SavedRelativeRotation;

	UPROPERTY(NonPIEDuplicateTransient)
	FTransform SavedTransform;

	/** Whether this component will simulate in the local space of a parent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	bool bLocalSpace;

	/** Control local interial force scale */
	UPROPERTY(EditAnywhere, Category = Flex)
	FFlexInertialScale InertialScale;

	/* For local space simulation */
	NvFlexExtMovingFrame* MovingFrame;

	/* Shape transforms */
	TArray<FQuat> ShapeRotations;
	TArray<FVector> ShapeTranslations;

	/* Attachments to rigid bodies */
	TArray<FlexParticleAttachment> Attachments;

	/* Cached local bounds */
	FBoxSphereBounds LocalBounds;

	// sends updated simulation data to the rendering proxy
	void UpdateSceneProxy(FFlexMeshSceneProxy* SceneProxy);

	// Begin UActorComponent Interface
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void SendRenderTransform_Concurrent() override;
	// End UActorComponent Interface

	// Begin UPrimitiveComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override;
	virtual FMatrix GetRenderMatrix() const override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual bool CanEditSimulatePhysics() override { return false; }
	// End UPrimitiveComponent Interface

	// Begin IFlexContainerClient Interface
	virtual bool IsEnabled() { return AssetInstance != NULL; }
	virtual FBoxSphereBounds GetBounds() { return Bounds; }
	virtual void Synchronize();
	// End IFlexContainerClient Interface

	virtual void DisableSim();
	virtual void EnableSim();
	
	virtual bool IsTearingCloth();

	// attach particles to a component within a radius)
	virtual void AttachToComponent(USceneComponent* Component, float Radius);

		/// Returns true if the component is in editor world or conversely not in a game world.
		/// Will return true if GetWorld() is null    
	bool IsInEditorWorld() const;

	/**
	* Get the FleX container template
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual UFlexContainer* GetContainerTemplate();

	/**
	* Constant acceleration applied to all particles
	* @param Gravity - The new Gravity value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetGravity(FVector Gravity);

	/**
	* Wind applied to all particles
	* @param Wind - The new Wind value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetWind(FVector Wind);

	/**
	* The radius of particles
	* @param Radius - The new Radius value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetRadius(float Radius);

	/**
	* The viscosity of particles
	* @param Viscosity - The new Viscosity value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetViscosity(float Viscosity);

	/**
	* The coefficient of friction used when colliding against shapes
	* @param ShapeFriction - The new ShapeFriction value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetShapeFriction(float ShapeFriction);

	/**
	* Multiplier for friction of particles against other particles
	* @param ParticleFriction - The new ParticleFriction value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetParticleFriction(float ParticleFriction);

	/**
	* Particles with a velocity magnitude < this threshold will be considered fixed
	* @param SleepThreshold - The new SleepThreshold value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetSleepThreshold(float SleepThreshold);

	/**
	* Particle velocity will be clamped to this value at the end of each step
	* @param MaxVelocity - The new MaxVelocity value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetMaxVelocity(float MaxVelocity);

	/**
	* Control the convergence rate of the parallel solver, for global relaxation values < 1.0 should be used, e.g: (0.25), high values will converge faster but may cause divergence
	* @param RelaxationFactor - The new RelaxationFactor value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetRelaxationFactor(float RelaxationFactor);

	/**
	* Viscous damping applied to all particles
	* @param Damping - The new Damping value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetDamping(float Damping);

	/**
	* Distance particles maintain against shapes
	* @param CollisionDistance - The new CollisionDistance value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetCollisionDistance(float CollisionDistance);

	/**
	* Coefficient of restitution used when colliding against shapes
	* @param Restitution - The new Restitution value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetRestitution(float Restitution);

	/**
	* Control how strongly particles stick to surfaces they hit, affects both fluid and non-fluid particles, default 0.0, range [0.0, +inf]
	* @param Adhesion - The new Adhesion value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetAdhesion(float Adhesion);

	/**
	* Artificially decrease the mass of particles based on height from a fixed reference point, this makes stacks and piles converge faster
	* @param ShockPropagation - The new ShockPropagation value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetShockPropagation(float ShockPropagation);

	/**
	* Damp particle velocity based on how many particle contacts it has
	* @param Dissipation - The new Dissipation value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetDissipation(float Dissipation);

	/**
	* Drag force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the negative velocity direction
	* @param Drag - The new Drag value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetDrag(float Drag);

	/**
	* Lift force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the direction perpendicular to velocity and (if possible), parallel to the plane normal
	* @param Lift - The new Lift value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetLift(float Lift);

	/**
	* Controls the distance fluid particles are spaced at the rest density, the absolute distance is given by this value*radius, must be in the range (0, 1)
	* @param RestDistance - The new RestDistance value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetRestDistance(float RestDistance);

	/**
	* Control how strongly particles hold each other together, default: 0.025, range [0.0, +inf]
	* @param Cohesion - The new Cohesion value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetCohesion(float Cohesion);

	/**
	* Controls how strongly particles attempt to minimize surface area, default: 0.0, range: [0.0, +inf]
	* @param SurfaceTension - The new SurfaceTension value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetSurfaceTension(float SurfaceTension);

	/**
	* Increases vorticity by applying rotational forces to particles
	* @param VorticityConfinement - The new VorticityConfinement value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetVorticityConfinement(float VorticityConfinement);

	/**
	* Add pressure from solid surfaces to particles
	* @param SolidPressure - The new SolidPressure value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetSolidPressure(float SolidPressure);

	/**
	* Anisotropy scale for ellipsoid surface generation, default 0.0 disables anisotropy computation.
	* @param AnisotropyScale - The new AnisotropyScale value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetAnisotropyScale(float AnisotropyScale);

	/**
	* Anisotropy minimum scale, this is specified as a fraction of the particle radius, the scale of the particle will be clamped to this minimum in each direction.
	* @param AnisotropyMin - The new AnisotropyMin value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetAnisotropyMin(float AnisotropyMin);

	/**
	* Anisotropy maximum scale, this is specified as a fraction of the particle radius, the scale of the particle will be clamped to this minimum in each direction.
	* @param AnisotropyMax - The new AnisotropyMax value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetAnisotropyMax(float AnisotropyMax);

	/**
	* Scales smoothing of particle positions for surface rendering, default 0.0 disables smoothing.
	* @param PositionSmoothing - The new PositionSmoothing value.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Flex")
	virtual void SetPositionSmoothing(float PositionSmoothing);

private:
	void UpdateSimPositions();
	void SynchronizeAttachments();
	void ApplyLocalSpace();
};
