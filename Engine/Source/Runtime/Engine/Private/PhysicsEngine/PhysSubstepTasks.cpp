// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysicsPublic.h"

#if WITH_SUBSTEPPING

#if WITH_PHYSX
#include "PhysXSupport.h"
#include "../Vehicles/PhysXVehicleManager.h"
#endif

#include "PhysSubstepTasks.h"

#if WITH_PHYSX
FPhysSubstepTask::FPhysSubstepTask(PxApexScene * GivenScene) :
	NumSubsteps(0),
	SubTime(0.f),
	DeltaSeconds(0.f),
	External(0),
	FullSimulationTask(0),
	Alpha(0.f),
	StepScale(0.f),
	TotalSubTime(0.f),
	CurrentSubStep(0),
	VehicleManager(NULL),
	PAScene(GivenScene)
{
	check(PAScene);
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
#if WITH_PHYSX
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
#endif
}

void FPhysSubstepTask::AddForce(FBodyInstance * Body, const FVector & Force)
{
#if WITH_PHYSX
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
#endif
}

void FPhysSubstepTask::AddForceAtPosition(FBodyInstance * Body, const FVector & Force, const FVector & Position)
{
#if WITH_PHYSX
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
#endif
}
void FPhysSubstepTask::AddTorque(FBodyInstance * Body, const FVector & Torque)
{
#if WITH_PHYSX
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
#endif
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
void FPhysSubstepTask::InterpolateKinematicActor(const FPhysTarget & PhysTarget, FBodyInstance * BodyInstance, float InAlpha)
{
#if WITH_PHYSX
	PxRigidDynamic * PRigidDynamic = BodyInstance->GetPxRigidDynamic();
	InAlpha = FMath::Clamp(InAlpha, 0.f, 1.f);

	/** Interpolate kinematic actors */
	if (PhysTarget.bKinematicTarget)
	{
		//It's possible that the actor is no longer kinematic and is now simulating. In that case do nothing
		if (!IsRigidDynamicNonKinematic(PRigidDynamic))
		{
			const FKinematicTarget & KinematicTarget = PhysTarget.KinematicTarget;
			const FTransform & TargetTM = KinematicTarget.TargetTM;
			const FTransform & StartTM = KinematicTarget.OriginalTM;
			FTransform InterTM = FTransform::Identity;

			InterTM.SetLocation(FMath::Lerp(StartTM.GetLocation(), TargetTM.GetLocation(), InAlpha));
			InterTM.SetRotation(FMath::Lerp(StartTM.GetRotation(), TargetTM.GetRotation(), InAlpha));

			const PxTransform PNewPose = U2PTransform(InterTM);
			check(PNewPose.isValid());
			PRigidDynamic->setKinematicTarget(PNewPose);
		}
	}
#endif
}

void FPhysSubstepTask::SubstepInterpolation(float InAlpha)
{
#if WITH_PHYSX
#if WITH_APEX
	PxScene * PScene = PAScene->getPhysXScene();
#else
	PxScene * PScene = PAScene;
#endif

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
		InterpolateKinematicActor(PhysTarget, BodyInstance, InAlpha);
	}

	/** Final substep */
	if (InAlpha >= 1.f)
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
	NumSubsteps = FMath::CeilToInt(UseDelta * FrameRateInv);
	NumSubsteps = FMath::Max(NumSubsteps > MaxSubSteps ? MaxSubSteps : NumSubsteps, (uint32) 1);
	SubTime = UseDelta / NumSubsteps;

	return SubTime;
}

#if WITH_PHYSX
void FPhysSubstepTask::StepSimulation(PhysXCompletionTask * Task)
{
	check(SubTime > 0.f);
	check(DeltaSeconds > 0.f);

	FullSimulationTask = Task;
	Alpha = 0.f;
	StepScale = SubTime / DeltaSeconds;
	TotalSubTime = 0.f;
	CurrentSubStep = 0;

	SubstepSimulationStart();
}
#endif

void FPhysSubstepTask::SubstepSimulationStart()
{
#if WITH_PHYSX
	check(SubTime > 0.f);
	check(DeltaSeconds > 0.f);
	
	check(!CompletionEvent.GetReference());	//should be done
	CompletionEvent = FGraphEvent::CreateGraphEvent();
	PhysXCompletionTask* SubstepTask = new PhysXCompletionTask(CompletionEvent, PAScene->getTaskManager());
	ENamedThreads::Type NamedThread = PhysSingleThreadedMode() ? ENamedThreads::GameThread : ENamedThreads::AnyThread;
	FDelegateGraphTask::CreateAndDispatchWhenReady(FDelegateGraphTask::FDelegate::CreateRaw(this, &FPhysSubstepTask::SubstepSimulationEnd), TEXT("ProcessPhysSubstepSimulation"), CompletionEvent, NamedThread, NamedThread);

	++CurrentSubStep;	

	bool bLastSubstep = CurrentSubStep >= NumSubsteps;

	if (!bLastSubstep)
	{

		Alpha += StepScale;
		TotalSubTime += SubTime;
	}

	float DeltaTime = bLastSubstep ? (DeltaSeconds - TotalSubTime) : SubTime;
	float Interpolation = bLastSubstep ? 1.f : Alpha;

#if WITH_VEHICLE
	if (VehicleManager)
	{
		VehicleManager->Update(DeltaTime);
	}
#endif

	SubstepInterpolation(Interpolation);

#if WITH_APEX
	PAScene->simulate(DeltaTime, bLastSubstep, SubstepTask);
#else
	PAScene->lockWrite();
	PAScene->simulate(DeltaTime, SubstepTask);
	PAScene->unlockWrite();
#endif
	SubstepTask->removeReference();
#endif
}

void FPhysSubstepTask::SubstepSimulationEnd(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
#if WITH_PHYSX
	CompletionEvent = NULL;
	if (CurrentSubStep < NumSubsteps)
	{
		uint32 OutErrorCode = 0;
#if WITH_APEX
		PAScene->fetchResults(true, &OutErrorCode);
#else
		PAScene->lockWrite();
		PAScene->fetchResults(true, &OutErrorCode);
		PAScene->unlockWrite();
#endif
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
#endif
}


void FPhysSubstepTask::SetVehicleManager(FPhysXVehicleManager * InVehicleManager)
{
	VehicleManager = InVehicleManager;
}

#endif //if WITH_SUBSTEPPING