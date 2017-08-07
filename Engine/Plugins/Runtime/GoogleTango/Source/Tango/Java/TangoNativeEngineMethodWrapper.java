// Copyright 2017 Google Inc.

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
