package com.bmo.companion

import android.app.Notification
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log

class BmoNotificationListener : NotificationListenerService() {

    companion object {
        private const val TAG = "BmoNotifListener"
        const val ACTION_NOTIFICATION = "com.bmo.companion.NOTIFICATION_EVENT"
        const val EXTRA_PACKAGE = "package_name"
        const val EXTRA_TITLE = "title"
        const val EXTRA_TEXT = "text"
        const val EXTRA_APP_NAME = "app_name"
        const val EXTRA_TIME = "timestamp"

        var isConnectedToSystem = false
            private set
    }

    override fun onListenerConnected() {
        super.onListenerConnected()
        isConnectedToSystem = true
        Log.d(TAG, "BMO NotificationListener Connected")
        MainActivity.sendNotificationListenerStatus(true)
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        isConnectedToSystem = false
        Log.d(TAG, "BMO NotificationListener Disconnected")
        MainActivity.sendNotificationListenerStatus(false)
    }

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        if (sbn == null) return

        try {
            val pkg = sbn.packageName ?: return

            // Abaikan notifikasi dari aplikasi ini sendiri agar tidak looping
            if (pkg == packageName) return

            // Abaikan notifikasi sistem umum atau ongoing/progress bar jika bukan pesan penting
            val notification = sbn.notification ?: return
            val isOngoing = (notification.flags and Notification.FLAG_ONGOING_EVENT) != 0

            // Abaikan media player / download progress berjalan
            if (isOngoing) return

            val extras = notification.extras ?: Bundle()
            val title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString() ?: ""
            val text = (extras.getCharSequence(Notification.EXTRA_TEXT)
                ?: extras.getCharSequence(Notification.EXTRA_BIG_TEXT)
                ?: extras.getCharSequence(Notification.EXTRA_SUB_TEXT))?.toString() ?: ""

            // Jika judul & teks kosong, abaikan
            if (title.isBlank() && text.isBlank()) return

            val pm = applicationContext.packageManager
            val appLabel = try {
                val appInfo = pm.getApplicationInfo(pkg, 0)
                pm.getApplicationLabel(appInfo).toString()
            } catch (e: Exception) {
                pkg
            }

            // Kirim ke Flutter via MainActivity event stream / method channel
            MainActivity.sendNotificationEvent(
                packageName = pkg,
                appName = appLabel,
                title = title,
                text = text,
                timestamp = sbn.postTime
            )
        } catch (e: Exception) {
            Log.e(TAG, "Error handling notification: ${e.message}", e)
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        // Notifikasi dihapus di HP - BMO tidak perlu menghapus instan agar pesan tetap bisa dibaca
    }
}
