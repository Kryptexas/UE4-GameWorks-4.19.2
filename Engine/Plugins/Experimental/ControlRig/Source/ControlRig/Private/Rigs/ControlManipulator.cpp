// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlManipulator.h"
#include "SceneManagement.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"

void UControlManipulator::Initialize(UObject* InContainer)
{
	CacheProperty(InContainer);
}

void UControlManipulator::CacheProperty(const UObject* InContainer) const
{
	if (InContainer)
	{
		CachedProperty = InContainer->GetClass()->FindPropertyByName(PropertyToManipulate);
	}
}

void UControlManipulator::SetLocation(const FVector& InLocation, UObject* InContainer)
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Vector)
		{
			FVector& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FVector>(InContainer, 0);
			if (!PropertyValue.Equals(InLocation))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = InLocation;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Transform)
		{
			FTransform& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
			if (!PropertyValue.GetLocation().Equals(InLocation))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue.SetLocation(InLocation);
				NotifyPostEditChangeProperty(InContainer);
			}
		}
	}
}

FVector UControlManipulator::GetLocation(const UObject* InContainer) const
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Vector)
		{
			return *CachedProperty->ContainerPtrToValuePtr<FVector>(InContainer, 0);
		}
		else if (PropertyName == NAME_Transform)
		{
			return CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0)->GetLocation();
		}
	}

	return FVector::ZeroVector;
}

void UControlManipulator::SetRotation(const FRotator& InRotation, UObject* InContainer)
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Rotator)
		{
			FRotator& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FRotator>(InContainer, 0);
			if (!PropertyValue.Equals(InRotation))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = InRotation;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Transform)
		{
			FTransform& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
			FQuat RotationAsQuat = InRotation.Quaternion();
			if (!PropertyValue.GetRotation().Equals(RotationAsQuat))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue.SetRotation(RotationAsQuat);
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Quat)
		{
			FQuat& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FQuat>(InContainer, 0);
			FQuat RotationAsQuat = InRotation.Quaternion();
			if (!PropertyValue.Equals(RotationAsQuat))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = RotationAsQuat;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
	}
}

FRotator UControlManipulator::GetRotation(const UObject* InContainer) const
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Rotator)
		{
			return *CachedProperty->ContainerPtrToValuePtr<FRotator>(InContainer, 0);
		}
		else if (PropertyName == NAME_Transform)
		{
			return CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0)->GetRotation().Rotator();
		}
		else if (PropertyName == NAME_Quat)
		{
			return CachedProperty->ContainerPtrToValuePtr<FQuat>(InContainer, 0)->Rotator();
		}
	}

	return FRotator::ZeroRotator;
}

void UControlManipulator::SetQuat(const FQuat& InQuat, UObject* InContainer)
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Rotator)
		{
			FRotator& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FRotator>(InContainer, 0);
			FRotator QuatAsRotator = InQuat.Rotator();
			if (!PropertyValue.Equals(QuatAsRotator))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = QuatAsRotator;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Transform)
		{
			FTransform& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
			if (!PropertyValue.GetRotation().Equals(InQuat))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue.SetRotation(InQuat);
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Quat)
		{
			FQuat& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FQuat>(InContainer, 0);
			if (!PropertyValue.Equals(InQuat))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = InQuat;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
	}
}

FQuat UControlManipulator::GetQuat(const UObject* InContainer) const
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Rotator)
		{
			return CachedProperty->ContainerPtrToValuePtr<FRotator>(InContainer, 0)->Quaternion();
		}
		else if (PropertyName == NAME_Transform)
		{
			return CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0)->GetRotation();
		}
		else if (PropertyName == NAME_Quat)
		{
			return *CachedProperty->ContainerPtrToValuePtr<FQuat>(InContainer, 0);
		}
	}

	return FQuat::Identity;
}

void UControlManipulator::SetScale(const FVector& InScale, UObject* InContainer)
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Vector)
		{
			FVector& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FVector>(InContainer, 0);
			if (!PropertyValue.Equals(InScale))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue = InScale;
				NotifyPostEditChangeProperty(InContainer);
			}
		}
		else if (PropertyName == NAME_Transform)
		{
			FTransform& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
			if (!PropertyValue.GetScale3D().Equals(InScale))
			{
				NotifyPreEditChangeProperty(InContainer);
				PropertyValue.SetScale3D(InScale);
				NotifyPostEditChangeProperty(InContainer);
			}
		}
	}
}

FVector UControlManipulator::GetScale(const UObject* InContainer) const
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>())
	{
		const FName PropertyName = Cast<UStructProperty>(CachedProperty)->Struct->GetFName();
		if (PropertyName == NAME_Vector)
		{
			return *CachedProperty->ContainerPtrToValuePtr<FVector>(InContainer, 0);
		}
		else if (PropertyName == NAME_Transform)
		{
			return CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0)->GetScale3D();
		}
	}

	return FVector(1.0f);
}

void UControlManipulator::SetTransform(const FTransform& InTransform, UObject* InContainer)
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>() && Cast<UStructProperty>(CachedProperty)->Struct->GetFName() == NAME_Transform)
	{
		FTransform& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
		if (!PropertyValue.Equals(InTransform))
		{
			NotifyPreEditChangeProperty(InContainer);
			PropertyValue = InTransform;
			NotifyPostEditChangeProperty(InContainer);
		}
	}
	else
	{
		SetLocation(InTransform.GetLocation(), InContainer);
		SetRotation(InTransform.GetRotation().Rotator(), InContainer);
		SetScale(InTransform.GetScale3D(), InContainer);
	}
}

FTransform UControlManipulator::GetTransform(const UObject* InContainer) const
{
	CacheProperty(InContainer);
	if (InContainer && CachedProperty && CachedProperty->IsA<UStructProperty>() && Cast<UStructProperty>(CachedProperty)->Struct->GetFName() == NAME_Transform)
	{
		return *CachedProperty->ContainerPtrToValuePtr<FTransform>(InContainer, 0);
	}
	else
	{
		return FTransform(GetRotation(InContainer), GetLocation(InContainer), GetScale(InContainer));
	}
}

#if WITH_EDITOR

bool UControlManipulator::SupportsTransformComponent(ETransformComponent InComponent) const
{
	return (bUsesTranslation && InComponent == ETransformComponent::Translation) || (bUsesRotation && InComponent == ETransformComponent::Rotation) || (bUsesScale && InComponent == ETransformComponent::Scale);
}

#endif

void UControlManipulator::NotifyPreEditChangeProperty(UObject* InContainer)
{
#if WITH_EDITOR
	if (CachedProperty && InContainer)
	{
		FEditPropertyChain EditPropertyChain;
		EditPropertyChain.AddHead(CachedProperty);
		EditPropertyChain.SetActivePropertyNode(CachedProperty);
		InContainer->PreEditChange(EditPropertyChain);
	}
#endif
}

void UControlManipulator::NotifyPostEditChangeProperty(UObject* InContainer)
{
#if WITH_EDITOR
	if (CachedProperty && InContainer)
	{
		FPropertyChangedEvent PropertyChangedEvent(CachedProperty);
		InContainer->PostEditChangeProperty(PropertyChangedEvent);
	}
#endif
}

UColoredManipulator::UColoredManipulator()
	: Color(0.9f, 0.9f, 0.9f)
	, SelectedColor(0.728f, 0.364f, 0.003f)
{
}

void UColoredManipulator::Initialize(UObject* InContainer)
{
	Super::Initialize(InContainer);

#if WITH_EDITOR
	ColorMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, NULL);
#endif
}

USphereManipulator::USphereManipulator()
	: UColoredManipulator()
	, Radius(5.0f)
{
}

#if WITH_EDITOR

void USphereManipulator::Draw(const FTransform& InTransform, const FSceneView* View, FPrimitiveDrawInterface* PDI, bool bIsSelected)
{
	ColorMaterial->SetVectorParameterValue("GizmoColor", FVector(bIsSelected ? SelectedColor : Color));

	const float Scale = View->WorldToScreen(InTransform.GetLocation()).W * (Radius / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
	DrawSphere(PDI, InTransform.GetLocation(), FVector(Radius) * Scale, 64, 64, ColorMaterial->GetRenderProxy(false), SDPG_Foreground);
}

UBoxManipulator::UBoxManipulator()
	: UColoredManipulator()
	, BoxExtent(2.5f)
{
}

void UBoxManipulator::Draw(const FTransform& InTransform, const FSceneView* View, FPrimitiveDrawInterface* PDI, bool bIsSelected)
{
	ColorMaterial->SetVectorParameterValue("GizmoColor", FVector(bIsSelected ? SelectedColor : Color));

	const float Scale = View->WorldToScreen(InTransform.GetLocation()).W * (BoxExtent.GetMax() / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
	DrawBox(PDI, InTransform.ToMatrixWithScale(), BoxExtent * Scale, ColorMaterial->GetRenderProxy(false), SDPG_Foreground);
}
#endif