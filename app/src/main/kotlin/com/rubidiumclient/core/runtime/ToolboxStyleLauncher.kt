package com.rubidiumclient.core.runtime

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.util.Log
import android.widget.Toast
import com.rubidiumclient.ui.overlay.OverlayService
import com.rubidiumclient.utils.ErrorLog

/**
 * Starts the real installed Minecraft activity.
 *
 * The previous revision attempted to load Minecraft's libminecraftpe.so into
 * an E-Client-created process. That is not a valid Android Minecraft runtime:
 * the library expects Minecraft's own application/activity/package environment.
 * That path caused the post-Start crash.
 *
 * This launcher remains only a development fallback. The real client architecture
 * is now Minecraft-hosted: a patched Minecraft APK loads libeclient_host inside
 * the actual Minecraft process. We do not claim that an external overlay is the
 * native client.
 */
object ToolboxStyleLauncher {
    private const val TAG = "EClientToolboxLaunch"
    const val MINECRAFT_PACKAGE = "com.mojang.minecraftpe"
    const val TARGET_VERSION = "1.21.80.3"
    const val EXTRA_CONTROLLED_SESSION = "com.rubidiumclient.controlled_session"

    fun launch(activity: Activity, targetPackage: String = MINECRAFT_PACKAGE): Boolean {
        val info = try {
            activity.packageManager.getPackageInfo(targetPackage, 0)
        } catch (_: PackageManager.NameNotFoundException) {
            toast(activity, "Minecraft is not installed")
            ErrorLog.record(activity, "Launch failed: Minecraft package not installed")
            return false
        }

        if (targetPackage == MINECRAFT_PACKAGE && info.versionName != TARGET_VERSION) {
            toast(activity, "E-Client requires Minecraft $TARGET_VERSION (found ${info.versionName})")
            ErrorLog.record(activity, "Launch rejected: expected $TARGET_VERSION, found ${info.versionName}")
            return false
        }

        val launchIntent = activity.packageManager.getLaunchIntentForPackage(targetPackage)
            ?: run {
                toast(activity, "Minecraft launcher activity was not found")
                ErrorLog.record(activity, "Launch failed: no launch intent for $targetPackage")
                return false
            }

        runCatching { OverlayService.start(activity) }
            .onFailure { ErrorLog.record(activity, "Overlay startup failed", it) }

        launchIntent.addFlags(
            Intent.FLAG_ACTIVITY_NEW_TASK or
            Intent.FLAG_ACTIVITY_NEW_DOCUMENT or
            Intent.FLAG_ACTIVITY_MULTIPLE_TASK
        )
        launchIntent.putExtra(EXTRA_CONTROLLED_SESSION, true)
        launchIntent.putExtra("eclient_target_version", TARGET_VERSION)

        return runCatching {
            activity.startActivity(launchIntent)
            Log.i(TAG, "Started real Minecraft task for ${info.versionName}; E-Client overlay enabled")
            ErrorLog.record(activity, "Started Minecraft $TARGET_VERSION task with E-Client overlay")
            true
        }.getOrElse {
            Log.e(TAG, "Minecraft launch failed", it)
            ErrorLog.record(activity, "Minecraft launch failed", it)
            toast(activity, "Minecraft could not be started: ${it.javaClass.simpleName}")
            false
        }
    }

    private fun toast(context: Context, message: String) {
        Toast.makeText(context, message, Toast.LENGTH_LONG).show()
    }
}
