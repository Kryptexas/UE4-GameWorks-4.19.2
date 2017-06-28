/* Copyright 2017 Google Inc.
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

package com.projecttango.unreal;

import android.os.IBinder;

public class TangoNativeEngineMethodWrapper
{
    // Native methods (in Unreal source):
    public static native void onApplicationCreated();
    public static native void onApplicationDestroyed();
    public static native void onApplicationPause();
    public static native void onApplicationResume();
    public static native void onApplicationStop();
    public static native void onApplicationStart();
    public static native void reportTangoServiceConnect(IBinder binder);
    public static native void reportTangoServiceDisconnect();
    public static native void onAreaDescriptionPermissionResult(boolean bWasGranted);
    public static native void onAreaDescriptionExportResult(boolean bSucceded);
    public static native void onAreaDescriptionImportResult(boolean bSucceded);
}
