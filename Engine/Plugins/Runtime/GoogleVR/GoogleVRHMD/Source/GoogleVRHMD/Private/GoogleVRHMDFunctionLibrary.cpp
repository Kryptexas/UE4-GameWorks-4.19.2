/* Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GoogleVRHMDPCH.h"
#include "Classes/GoogleVRHMDFunctionLibrary.h"
#include "GoogleVRHMD.h"

UGoogleVRHMDFunctionLibrary::UGoogleVRHMDFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGoogleVRHMD* GetHMD()
{
	if (GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->GetVersionString().Contains(TEXT("GoogleVR")) )
	{
		return static_cast<FGoogleVRHMD*>(GEngine->HMDDevice.Get());
	}

	return nullptr;
}

void UGoogleVRHMDFunctionLibrary::SetChromaticAberrationCorrectionEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetChromaticAberrationCorrectionEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetDistortionCorrectionEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetDistortionCorrectionEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetUiLayerEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetUiLayerEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetUiLayerAlignmentMarkerEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetUiLayerAlignmentMarkerEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetUiLayerSettingButtonEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetUiLayerSettingButtonEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetUiLayerTransitionViewEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetUiLayerTransitionViewEnabled(bEnable);
	}
}

void UGoogleVRHMDFunctionLibrary::SetUiLayerBackButtonEnabled(bool bEnable)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		HMD->SetUiLayerBackButtonEnabled(bEnable);
	}
}

bool UGoogleVRHMDFunctionLibrary::SetDefaultViewerProfile(const FString& ViewerProfileURL)
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->SetDefaultViewerProfile(ViewerProfileURL);
	}

	return false;
}

void UGoogleVRHMDFunctionLibrary::SetDistortionMeshSize(EDistortionMeshSizeEnum MeshSize)
{
	FGoogleVRHMD* HMD = GetHMD();
	if (HMD)
	{
		HMD->SetDistortionMeshSize(MeshSize);
	}
}

bool UGoogleVRHMDFunctionLibrary::GetDistortionCorrectionEnabled()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetDistortionCorrectionEnabled();
	}

	return false;
}

bool UGoogleVRHMDFunctionLibrary::GetUiLayerTransitionViewEnabled()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetUiLayerTransitionViewEnabled();
	}

	return false;
}

bool UGoogleVRHMDFunctionLibrary::GetUiLayerSettingsButtonEnabled()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetUiLayerSettingsButtonEnabled();
	}

	return false;
}

bool UGoogleVRHMDFunctionLibrary::GetUiLayerAlignmentMarkerEnabled()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetUiLayerAlignmentMarkerEnabled();
	}

	return false;
}

bool UGoogleVRHMDFunctionLibrary::GetUiLayerBackButtonEnabled()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetUiLayerBackButtonEnabled();
	}

	return false;
}

FString UGoogleVRHMDFunctionLibrary::GetViewerModel()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetViewerModel();
	}

	return TEXT("");
}

FString UGoogleVRHMDFunctionLibrary::GetViewerVendor()
{
	FGoogleVRHMD* HMD = GetHMD();
	if(HMD)
	{
		return HMD->GetViewerVendor();
	}

	return TEXT("");
}
