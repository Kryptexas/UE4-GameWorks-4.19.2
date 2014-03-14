// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_SUBSTEPPING

#include "PhysXSupport.h"
#include "PhysSubstepTasks.h"

#if WITH_PHYSX
FPhysSubstepTask::FPhysSubstepTask(PxScene * GivenPScene) :
	NumSubsteps(0),
	SubTime(0.f),
	DeltaSeconds(0.f),
	External(0),
	FullSimulationTask(0),
	Alpha(0.f),
	StepScale(0.f),
	TotalSubTime(0.f),
	CurrentSubStep(0),
#if WITH_APEX
	ApexScene(NULL),
#endif
	PScene(GivenPScene)
{
	check(PScene);
}

#endif

void FPhysSubstepTask::SwapBuffers()
{
	External = !External;
}

void FPhysSubstepTask::RemoveBodyInstance(FBodyInstance * BodyInstance)
{
	
	PhysTargetBuffers[External].Remove(BodyInstance);
	
	//This function gets called when we terminate a body instance. (FBodyInstance::TermBody)
	//To do that FBodyInstance needs to be holding on to the scene write lock.
	//This is the same lock that we use when iterating over the internal buffer - so it's safe to modify this buffer from this thread.
	//If this assumption changes you need to take care of accessing the buffer from both threads
	PhysTargetBuffers[!External].Remove(BodyInstance);
}

void FPhysSubstepTask::SetKinematicTarget(FBodyInstance * Body, const FTransform & TM)
{
	check(Body);
	TM.DiagnosticCheckNaN_All();

	PxRigidDynamic * PRigidDynamic = Body->GetPxRigidDynamic();
	SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
	//We only interpolate kinematic actors
	if (!IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		FKinematicTarget KinmaticTarget(Body, TM);
		FPhysTarget & TargetState = PhysTargetBuffers[External].FindOrAdd(Body);
		TargetState.bKinematicTarget = true;
		TargetState.KinematicTarget = KinmaticTarget;
	}

}

void FPhysSubstepTask::AddForce(FBodyInstance * Body, const FVector & Force)
{
	check(Body);
	PxRigidDynamic * PRigidDynamic = Body->GetPxRigidDynamic();
	SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
	//We should only apply forces on non kinematic actors
	if (IsRigidDynamicNonKinematic(PRigidDynamic))
	{

		FForceTarget ForceTarget;
		ForceTarget.bPosition = false;
		ForceTarget.Force = Force;

		FPhysTarget & TargetState = PhysTargetBuffers[External].FindOrAdd(Body);
		TargetState.Forces.Add(ForceTarget);
	}
}

void FPhysSubstepTask::AddForceAtPosition(FBodyInstance * Body, const FVector & Force, const FVector & Position)
{
	check(Body);

	PxRigidDynamic * PRigidDynamic = Body->GetPxRigidDynamic();
	SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
	//We should only apply forces on non kinematic actors
	if (IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		FForceTarget ForceTarget;
		ForceTarget.bPosition = true;
		ForceTarget.Force = Force;
		ForceTarget.Position = Position;

		FPhysTarget & TargetState = PhysTargetBuffers[External].FindOrAdd(Body);
		TargetState.Forces.Add(ForceTarget);
	}

}
void FPhysSubstepTask::AddTorque(FBodyInstance * Body, const FVector & Torque)
{
	check(Body);

	PxRigidDynamic * PRigidDynamic = Body->GetPxRigidDynamic();
	SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
	//We should only apply torque on non kinematic actors
	if (IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		FTorqueTarget TorqueTarget;
		TorqueTarget.Torque = Torque;

		FPhysTarget & TargetState = PhysTargetBuffers[External].FindOrAdd(Body);
		TargetState.Torques.Add(TorqueTarget);
	}
}

/** Applies forces - Assumes caller has obtained writer lock */
void FPhysSubstepTask::ApplyForces(const FPhysTarget & PhysTarget, FBodyInstance * BodyInstance)
{
#if WITH_PHYSX
	/** Apply Forces */
	PxRigidDynamic * PRigidDynamic = BodyInstance->GetPxRigidDynamic();

	for (int32 i = 0; i < PhysTarget.Forces.Num(); ++i)
	{
		const FForceTarget & ForceTarget = PhysTarget.Forces[i];

		if (ForceTarget.bPosition)
		{
			PxRigidBodyExt::addForceAtPos(*PRigidDynamic, U2PVector(ForceTarget.Force), U2PVector(ForceTarget.Position), PxForceMode::eFORCE, true);
		}
		else
		{
			PRigidDynamic->addForce(U2PVector(ForceTarget.Force), PxForceMode::eFORCE, true);
		}
	}
#endif
}

/** Applies torques - Assumes caller has obtained writer lock */
void FPhysSubstepTask::ApplyTorques(const FPhysTarget & PhysTarget, FBodyInstance * BodyInstance)
{
#if WITH_PHYSX
	/** Apply Torques */
	PxRigidDynamic * PRigidDynamic = BodyInstance->GetPxRigidDynamic();

	for (int32 i = 0; i < PhysTarget.Torques.Num(); ++i)
	{
		const FTorqueTarget & TorqueTarget = PhysTarget.Torques[i];
		PRigidDynamic->addTorque(U2PVector(TorqueTarget.Torque), PxForceMode::eFORCE, true);
	}
#endif
}

/** Interpolates kinematic actor transform - Assumes caller has obtained writer lock */
void FPhysSubstepTask::InterpolateKinematicActor(const FPhysTarget & PhysTarget, FBodyInstance * BodyInstance, float Alpha)
{
#if WITH_PHYSX
	PxRigidDynamic * PRigidDynamic = BodyInstance->GetPxRigidDynamic();
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

	/** Interpolate kinematic actors */
	if (PhysTarget.bKinematicTarget)
	{
		//We should only be interpolating kinematic actors
		check(!IsRigidDynamicNonKinematic(PRigidDynamic));
		const FKinematicTarget & KinematicTarget = PhysTarget.KinematicTarget;
		const FTransform & TargetTM = KinematicTarget.TargetTM;
		const FTransform & StartTM = KinematicTarget.OriginalTM;
		FTransform InterTM = FTransform::Identity;

		InterTM.SetLocation(FMath::Lerp(StartTM.GetLocation(), TargetTM.GetLocation(), Alpha));
		InterTM.SetRotation(FMath::Lerp(StartTM.GetRotation(), TargetTM.GetRotation(), Alpha));

		const PxTransform PNewPose = U2PTransform(InterTM);
		check(PNewPose.isValid());
		PRigidDynamic->setKinematicTarget(PNewPose);
	}
#endif
}

void FPhysSubstepTask::SubstepInterpolation(float Alpha)
{
#if WITH_PHYSX
	PhysTargetMap & Targets = PhysTargetBuffers[!External];

	/** Note: We lock the entire scene before iterating. The assumption is that removing an FBodyInstance from the map will also be wrapped by this lock */
	SCENE_LOCK_WRITE(PScene);

	for (PhysTargetMap::TIterator Itr = Targets.CreateIterator(); Itr; ++Itr)
	{
		FPhysTarget & PhysTarget = Itr.Value();
		FBodyInstance * BodyInstance = Itr.Key();
		PxRigidDynamic * PRigidDynamic = BodyInstance->GetPxRigidDynamic();

		if (PRigidDynamic == NULL)
		{
			continue;
		}

		//We should only be iterating over actors that belong to this scene
		check(PRigidDynamic->getScene() == PScene);

		ApplyForces(PhysTarget, BodyInstance);
		ApplyTorques(PhysTarget, BodyInstance);
		InterpolateKinematicActor(PhysTarget, BodyInstance, Alpha);
	}

	/** Final substep */
	if (Alpha >= 1.f)
	{
		Targets.Empty(Targets.Num());
	}

	SCENE_UNLOCK_WRITE(PScene);
#endif
}

float FPhysSubstepTask::UpdateTime(float UseDelta)
{
	float FrameRate = 1.f;
	uint32 MaxSubSteps = 1;

	UPhysicsSettings * PhysSetting = UPhysicsSettings::Get();
	FrameRate = PhysSetting->MaxSubstepDeltaTime;
	MaxSubSteps = PhysSetting->MaxSubsteps;
	
	float FrameRateInv = 1.f / FrameRate;

	//Figure out how big dt to make for desired framerate
	DeltaSeconds = UseDelta;
	NumSubsteps = FMath::Ceil(UseDelta * FrameRateInv);
	NumSubsteps = FMath::Max(NumSubsteps > MaxSubSteps ? MaxSubSteps : NumSubsteps, (uint32) 1);
	SubTime = UseDelta / NumSubsteps;

	return SubTime;
}

#if WITH_APEX
void FPhysSubstepTask::StepSimulation(NxApexScene * GivenApexScene, PhysXCompletionTask * Task)
{
	check(SubTime > 0.f);
	check(DeltaSeconds > 0.f);

	FullSimulationTask = Task;
	Alpha = 0.f;
	StepScale = SubTime / DeltaSeconds;
	TotalSubTime = 0.f;
	CurrentSubStep = 0;
	ApexScene = GivenApexScene;

	SubstepSimulationStart();
}

void FPhysSubstepTask::SubstepSimulationStart()
{
	check(SubTime > 0.f);
	check(DeltaSeconds > 0.f);
	
	check(!CompletionEvent.GetReference());	//should be done
	CompletionEvent = FGraphEvent::CreateGraphEvent();
	PhysXCompletionTask* SubstepTask = new PhysXCompletionTask(CompletionEvent, ApexScene->getTaskManager());
	FDelegateGraphTask::CreateAndDispatchWhenReady(FDelegateGraphTask::FDelegate::CreateRaw(this, &FPhysSubstepTask::SubstepSimulationEnd), TEXT("ProcessPhysSubstepSimulation"), CompletionEvent);

	++CurrentSubStep;
	if (CurrentSubStep < NumSubsteps)
	{
		
		Alpha += StepScale;
		TotalSubTime += SubTime;
		SubstepInterpolation(Alpha);
		ApexScene->simulate(SubTime, false, SubstepTask);	
		SubstepTask->removeReference();
	}
	else
	{
		SubstepInterpolation(1.f);
		ApexScene->simulate(DeltaSeconds - TotalSubTime, true, SubstepTask);
		SubstepTask->removeReference();
	}
}

void FPhysSubstepTask::SubstepSimulationEnd(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	CompletionEvent = NULL;
	if (CurrentSubStep < NumSubsteps)
	{
		uint32 OutErrorCode = 0;
		ApexScene->fetchResults(true, &OutErrorCode);
		if (OutErrorCode != 0)
		{
			UE_LOG(LogPhysics, Log, TEXT("PHYSX FETCHRESULTS ERROR: %d"), OutErrorCode);
		}

		SubstepSimulationStart();
	}
	else
	{
		//final step we call fetch on in game thread
		FullSimulationTask->removeReference();
	}

}
#endif

#endif //if WITH_SUBSTEPPING