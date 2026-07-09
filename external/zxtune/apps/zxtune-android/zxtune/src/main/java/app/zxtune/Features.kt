package app.zxtune

import android.content.Intent
import android.os.Build
import androidx.core.net.toUri

object Features {
    object StorageAccessFramework {
        const val REQUIRED_SDK = 30

        @JvmStatic
        fun isEnabled() = Build.VERSION.SDK_INT >= REQUIRED_SDK
    }

    object StorageManagerVolumes {
        const val REQUIRED_SDK = 24

        @JvmStatic
        fun isEnabled() = Build.VERSION.SDK_INT >= REQUIRED_SDK
    }

    object DozeMode {
        const val REQUIRED_SDK = 23

        fun isAvailable() = Build.VERSION.SDK_INT >= REQUIRED_SDK
    }

    object UserRating {
        fun isAvailable() = BuildConfig.FLAVOR_packaging != "fdroid"
        val intent
            get() = when (BuildConfig.FLAVOR_packaging) {
                "google" -> Intent(Intent.ACTION_VIEW).apply {
                    data =
                        "https://play.google.com/store/apps/details?id=${BuildConfig.APPLICATION_ID}".toUri()
                    `package` = "com.android.vending"
                }

                "rustore" -> Intent(Intent.ACTION_VIEW).apply {
                    data =
                        "https://www.rustore.ru/catalog/app/${BuildConfig.APPLICATION_ID}".toUri()
                    `package` = "ru.vk.store"
                }

                else -> Intent(Intent.ACTION_VIEW).apply {
                    data = "market://details?id=${BuildConfig.APPLICATION_ID}".toUri()
                }
            }
    }
}
