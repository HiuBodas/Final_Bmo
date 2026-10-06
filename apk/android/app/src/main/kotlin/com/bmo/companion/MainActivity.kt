package com.bmo.companion

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.annotation.NonNull
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

class MainActivity: FlutterActivity() {

    companion object {
        private const val METHOD_CHANNEL = "com.bmo.companion/control"
        private const val EVENT_CHANNEL = "com.bmo.companion/notifications"

        private var eventSink: EventChannel.EventSink? = null

        fun sendNotificationListenerStatus(isConnected: Boolean) {
            val data = HashMap<String, Any>()
            data["type"] = "listener_status"
            data["connected"] = isConnected
            eventSink?.success(data)
        }

        fun sendNotificationEvent(
            packageName: String,
            appName: String,
            title: String,
            text: String,
            timestamp: Long
        ) {
            val data = HashMap<String, Any>()
            data["type"] = "notification"
            data["package"] = packageName
            data["app"] = appName
            data["title"] = title
            data["text"] = text
            data["timestamp"] = timestamp
            eventSink?.success(data)
        }
    }

    override fun configureFlutterEngine(@NonNull flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, METHOD_CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "isNotificationListenerEnabled" -> {
                    result.success(isNotificationListenerEnabled(this))
                }
                "openNotificationListenerSettings" -> {
                    openNotificationListenerSettings(this)
                    result.success(true)
                }
                "openAppDetailsSettings" -> {
                    openAppDetailsSettings(this)
                    result.success(true)
                }
                "startForegroundService" -> {
                    BmoForegroundService.startService(this)
                    result.success(true)
                }
                "stopForegroundService" -> {
                    BmoForegroundService.stopService(this)
                    result.success(true)
                }
                else -> {
                    result.notImplemented()
                }
            }
        }

        EventChannel(flutterEngine.dartExecutor.binaryMessenger, EVENT_CHANNEL).setStreamHandler(
            object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                    eventSink = events
                }

                override fun onCancel(arguments: Any?) {
                    eventSink = null
                }
            }
        )
    }

    private fun isNotificationListenerEnabled(context: Context): Boolean {
        val flat = Settings.Secure.getString(context.contentResolver, "enabled_notification_listeners") ?: return false
        val myComponent = ComponentName(context, BmoNotificationListener::class.java).flattenToString()
        return flat.contains(myComponent)
    }

    private fun openNotificationListenerSettings(context: Context) {
        try {
            val intent = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
                Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
            } else {
                Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS")
            }
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            context.startActivity(intent)
        } catch (e: Exception) {
            openAppDetailsSettings(context)
        }
    }

    private fun openAppDetailsSettings(context: Context) {
        try {
            val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
                data = Uri.fromParts("package", context.packageName, null)
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        } catch (_: Exception) {}
    }
}
