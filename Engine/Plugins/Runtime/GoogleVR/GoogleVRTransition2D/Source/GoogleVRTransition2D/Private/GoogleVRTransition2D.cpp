/* Copyright 2016 Google Inc.
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

#include "GoogleVRTransition2D.h"
#include "GoogleVRTransition2DBPLibrary.h"

#define LOCTEXT_NAMESPACE "FGoogleVRTransition2DModule"

DEFINE_LOG_CATEGORY(LogGoogleVRTransition2D);

void FGoogleVRTransition2DModule::StartupModule()
{
	UGoogleVRTransition2DBPLibrary::Initialize();
}

void FGoogleVRTransition2DModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGoogleVRTransition2DModule, GoogleVRTransition2D)