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

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Point;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Display;
import android.view.Surface;

/**
 * Makes it easier to work with Tango from within Unreal.
 */
public class TangoUnrealHelper {

    // Note: keep this in sync with required tangocore features
    static final int TANGO_MINIMUM_VERSION_CODE = 14694;

    private static final String INTENT_CLASS_PACKAGE = "com.google.tango";
    // Key string for load/save Area Description Files.
    private static final String AREA_LEARNING_PERMISSION =
            "ADF_LOAD_SAVE_PERMISSION";

    // Permission request action.
    public static final int REQUEST_CODE_TANGO_PERMISSION = 77897;
    public static final int REQUEST_CODE_TANGO_IMPORT = 77898;
    public static final int REQUEST_CODE_TANGO_EXPORT = 77899;
    private static final String REQUEST_PERMISSION_ACTION =
            "android.intent.action.REQUEST_TANGO_PERMISSION";

    // On current Tango devices, camera id 0 is the color camera.
    private static final int COLOR_CAMERA_ID = 0;
    private static final String TAG = "TangoUnrealHelper";

    private static final String TANGO_PERMISSION_ACTIVITY =
            "com.google.atap.tango.RequestPermissionActivity";
    private static final String AREA_DESCRIPTION_IMPORT_EXPORT_ACTIVITY =
            "com.google.atap.tango.RequestImportExportActivity";
    private static final String EXTRA_KEY_SOURCEUUID = "SOURCE_UUID";
    private static final String EXTRA_KEY_SOURCEFILE = "SOURCE_FILE";
    private static final String EXTRA_KEY_DESTINATIONFILE = "DESTINATION_FILE";

    private static boolean hasTangoPermission(Context context, String permissionType) {
        Uri uri = Uri.parse("content://com.google.atap.tango.PermissionStatusProvider/" +
                permissionType);
        Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
        if (cursor == null) {
            return false;
        } else {
            return true;
        }
    }

    private void getTangoPermission(String permissionType) {
        Intent intent = new Intent();
        intent.setPackage(INTENT_CLASS_PACKAGE);
        intent.setAction(REQUEST_PERMISSION_ACTION);
        intent.putExtra("PERMISSIONTYPE", permissionType);

        // After the permission activity is dismissed, we will receive a callback
        // function onActivityResult() with user's result.
        mParent.startActivityForResult(intent, REQUEST_CODE_TANGO_PERMISSION);
    }

    public boolean hasAreaLearningPermission() {
      return hasTangoPermission(mParent.getApplicationContext(), AREA_LEARNING_PERMISSION);
    }

    public void getAreaLearningPermissionIfNecessary() {
      mParent.runOnUiThread(new Runnable() {
          public void run() {
            if (!hasTangoPermission(mParent.getApplicationContext(), AREA_LEARNING_PERMISSION)) {
              getTangoPermission(AREA_LEARNING_PERMISSION);
            }
          }
        });
    }

    public void requestImportAreaDescription(String filePath) {
      final Intent exportIntent = new Intent();
      exportIntent.setClassName(INTENT_CLASS_PACKAGE, AREA_DESCRIPTION_IMPORT_EXPORT_ACTIVITY);
      exportIntent.putExtra(EXTRA_KEY_SOURCEFILE, filePath);
      mParent.startActivityForResult(exportIntent, REQUEST_CODE_TANGO_IMPORT);
    }

    public void requestExportAreaDescription(String UUID, String filePath) {
      final Intent exportIntent = new Intent();
      exportIntent.setClassName(INTENT_CLASS_PACKAGE, AREA_DESCRIPTION_IMPORT_EXPORT_ACTIVITY);
      exportIntent.putExtra(EXTRA_KEY_SOURCEUUID, UUID);
      exportIntent.putExtra(EXTRA_KEY_DESTINATIONFILE, filePath);
      mParent.startActivityForResult(exportIntent, REQUEST_CODE_TANGO_EXPORT);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_TANGO_PERMISSION) {
          TangoNativeEngineMethodWrapper.onAreaDescriptionPermissionResult(resultCode != Activity.RESULT_CANCELED);
        }
        else if (requestCode == REQUEST_CODE_TANGO_IMPORT) {
          TangoNativeEngineMethodWrapper.onAreaDescriptionImportResult(resultCode == -1);
        }
        else if (requestCode == REQUEST_CODE_TANGO_EXPORT) {
          TangoNativeEngineMethodWrapper.onAreaDescriptionExportResult(resultCode == -1);
        }
    }


    /**
     * Interface for listener of Tango service connection events.
     */
    public interface TangoServiceLifecycleListener {
      public void onTangoServiceConnected(IBinder binder);
      public void onTangoServiceDisconnected();
    }

    private Activity mParent;

    private TangoServiceLifecycleListener mTangoServiceLifecycleListener;

    private IBinder mTangoServiceBinder;

    public TangoUnrealHelper(Activity unrealActivity) {
        mParent = unrealActivity;
    }

    public int getDisplayRotation() {
        Display display = mParent.getWindowManager().getDefaultDisplay();
        return display.getRotation();
    }

    public int getColorCameraRotation() {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(COLOR_CAMERA_ID, info);
        return info.orientation;
    }

    public int getDeviceDefaultOrientation() {
        Display display = mParent.getWindowManager().getDefaultDisplay();

        int ret = mParent.getResources().getConfiguration().orientation + display.getRotation();
        ret = ret % 4;

        // odd number represents portrait, even number represents landscape.
        return ret;
    }

    public int getTangoCoreVersionCode() {
      try {
        PackageManager pm = mParent.getPackageManager();
        PackageInfo info = pm.getPackageInfo(getTangoCorePackageName(), 0);
        return info.versionCode;
      } catch (NameNotFoundException e) {
        return 0;
      }
    }

    public boolean isTangoCorePresent() {
        int version = getTangoCoreVersionCode();
        return version > 0;
	}

    public boolean isTangoCoreUpToDate() {
        int version = getTangoCoreVersionCode();
        return version > TANGO_MINIMUM_VERSION_CODE;
    }

    public String getTangoCoreVersionName() {
      try {
        PackageManager pm = mParent.getPackageManager();
        PackageInfo info = pm.getPackageInfo(getTangoCorePackageName(), 0);
        return info.versionName;
      } catch (NameNotFoundException e) {
        return "";
      }
    }

    public void attachTangoServiceLifecycleListener(TangoServiceLifecycleListener listener) {
      mTangoServiceLifecycleListener = listener;
    }

    public boolean bindTangoService() {
      Log.d(TAG, "Binding to Tango Android service.");
      return TangoInitialization.bindService(mParent, mTangoServiceConnection);
    }

    public void unbindTangoService() {
      Log.d(TAG, "Unbinding from Tango Android service.");
      if (mTangoServiceBinder != null) {
        Log.d(TAG, "Really unbinding from Tango Android service.");
        mParent.unbindService(mTangoServiceConnection);
        mTangoServiceBinder = null;
      }
    }

    private ServiceConnection mTangoServiceConnection = new ServiceConnection() {
      @Override
      public void onServiceConnected(ComponentName className, IBinder service) {
        Log.d(TAG, "TangoService connected");
        mTangoServiceBinder = service;

        if (mTangoServiceLifecycleListener != null) {
          mTangoServiceLifecycleListener.onTangoServiceConnected(service);
        }
      }

      @Override
      public void onServiceDisconnected(ComponentName className) {
        Log.e(TAG, "TangoService has disconnected");
        mTangoServiceBinder = null;

        if (mTangoServiceLifecycleListener != null) {
          mTangoServiceLifecycleListener.onTangoServiceDisconnected();
        }
      }
    };

    private String getTangoCorePackageName() {
      final String tangoCorePackage = "com.google.tango";
      final String tangoCoreOldPackage = "com.projecttango.tango";

      try {
        PackageManager pm = mParent.getPackageManager();
        pm.getPackageInfo(tangoCorePackage, 0);
        Log.d(TAG, "Using package:" + tangoCorePackage);
        return tangoCorePackage;
      } catch (NameNotFoundException e) {
        Log.d(TAG, "Using package:" + tangoCoreOldPackage);
        return tangoCoreOldPackage;
      }
    }
}
