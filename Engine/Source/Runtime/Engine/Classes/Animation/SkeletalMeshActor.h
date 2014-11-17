// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Components/SkeletalMeshComponent.h"
#include "Matinee/MatineeAnimInterface.h"
#include "SkeletalMeshActor.generated.h"

UCLASS(ClassGroup=ISkeletalMeshes, Blueprintable, ConversionRoot, meta=(ChildCanTick))
class ENGINE_API ASkeletalMeshActor : public AActor, public IMatineeAnimInterface
{
	GENERATED_UCLASS_BODY()

	/** Whether or not this actor should respond to anim notifies - CURRENTLY ONLY AFFECTS PlayParticleEffect NOTIFIES**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, AdvancedDisplay)
	uint32 bShouldDoAnimNotifies:1;

	UPROPERTY()
	uint32 bWakeOnLevelStart_DEPRECATED:1;

	UPROPERTY(Category=SkeletalMeshActor, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Mesh,Components|SkeletalMesh,Animation,Physics"))
	TSubobjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	/** Used to replicate mesh to clients */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedMesh, transient)
	class USkeletalMesh* ReplicatedMesh;

	/** Used to replicate physics asset to clients */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedPhysAsset, transient)
	class UPhysicsAsset* ReplicatedPhysAsset;

	/** used to replicate the material in index 0 */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedMaterial0)
	class UMaterialInterface* ReplicatedMaterial0;

	UPROPERTY(replicatedUsing=OnRep_ReplicatedMaterial1)
	class UMaterialInterface* ReplicatedMaterial1;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_ReplicatedMesh();

	UFUNCTION()
	virtual void OnRep_ReplicatedPhysAsset();

	UFUNCTION()
	virtual void OnRep_ReplicatedMaterial0();

	UFUNCTION()
	virtual void OnRep_ReplicatedMaterial1();


	// Begin UObject interface
protected:
	virtual FString GetDetailedInfoInternal() const;
public:
	// End UObject interface

	// Begin AActor interface
#if WITH_EDITOR
	virtual void CheckForErrors() override;
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const override;
	virtual void EditorReplacedActor(AActor* OldActor) override;
	virtual void LoadedFromAnotherClass(const FName& OldClassName) override;
#endif
	virtual void PostInitializeComponents() override;
	// End AActor interface

	// Begin IMatineeAnimInterface Interface
	virtual void PreviewBeginAnimControl(class UInterpGroup* InInterpGroup) override;
	virtual void PreviewSetAnimPosition(FName SlotName, int32 ChannelIndex, UAnimSequence* InAnimSequence, float InPosition, bool bLooping, bool bFireNotifies, float AdvanceTime) override;
	virtual void PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos) override;
	virtual void PreviewFinishAnimControl(class UInterpGroup* InInterpGroup) override;
	virtual void GetAnimControlSlotDesc(TArray<struct FAnimSlotDesc>& OutSlotDescs) override {};
	virtual void SetAnimWeights( const TArray<struct FAnimSlotInfo>& SlotInfos ) override;
	virtual void BeginAnimControl(class UInterpGroup* InInterpGroup) override;
	virtual void SetAnimPosition(FName SlotName, int32 ChannelIndex, class UAnimSequence* InAnimSequence, float InPosition, bool bFireNotifies, bool bLooping) override;
	virtual void FinishAnimControl(class UInterpGroup* InInterpGroup) override;
	// End IMatineeAnimInterface Interface

private:
	EAnimationMode::Type	SavedAnimationMode;
	// utility function to see if it can play animation or not
	bool CanPlayAnimation();
};



