package app.zxtune.ui

import android.content.ActivityNotFoundException
import android.content.Intent
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import androidx.appcompat.app.AlertDialog
import androidx.core.view.MenuProvider
import androidx.fragment.app.FragmentActivity
import app.zxtune.Features
import app.zxtune.MainService
import app.zxtune.PreferencesActivity
import app.zxtune.R
import app.zxtune.ResultActivity
import app.zxtune.analytics.Analytics
import app.zxtune.device.PowerManagement
import app.zxtune.ui.utils.item
import kotlin.system.exitProcess

class ApplicationMenu(private val activity: FragmentActivity) : MenuProvider {

    override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
        menuInflater.inflate(R.menu.main, menu).also {
            // onPrepareMenu called only on showing
            menu.item(R.id.action_problems).let { item ->
                PowerManagement.create(activity).hasProblem.observe(activity) {
                    item.isVisible = it
                }
            }
            menu.item(R.id.action_rate).isVisible = Features.UserRating.isAvailable()
        }

    override fun onPrepareMenu(menu: Menu) = menu.run {
        item(R.id.action_prefs).intent = PreferencesActivity.createIntent(activity)
    }

    override fun onMenuItemSelected(menuItem: MenuItem) = when (menuItem.itemId) {
        R.id.action_problems -> {
            setupPowerManagement()
            true
        }

        R.id.action_about -> {
            AboutFragment.show(activity)
            Analytics.sendUiEvent(Analytics.UiAction.ABOUT)
            true
        }

        R.id.action_rate -> {
            rate()
            true
        }

        R.id.action_quit -> {
            quit()
            true
        }

        else -> false
    }

    private fun setupPowerManagement() = AlertDialog.Builder(activity).apply {
        setTitle(R.string.problems)
        setMessage(R.string.problem_power_management)
        setPositiveButton(R.string.problem_power_management_fixit) { _, _ ->
            tryOpenIntent(ResultActivity.createSetupPowerManagementIntent(activity))
        }
    }.show()

    private fun rate() {
        if (tryOpenIntent(Features.UserRating.intent)) {
            Analytics.sendUiEvent(Analytics.UiAction.RATE)
        }
    }

    private fun tryOpenIntent(intent: Intent) = try {
        activity.startActivity(intent)
        true
    } catch (_: ActivityNotFoundException) {
        false
    }

    private fun quit(): Unit = activity.run {
        Analytics.sendUiEvent(Analytics.UiAction.QUIT)
        stopService(MainService.createIntent(this, null))
        finishAffinity()
        exitProcess(0)
    }
}
