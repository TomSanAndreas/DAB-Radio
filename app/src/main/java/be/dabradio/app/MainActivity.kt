package be.dabradio.app

import android.app.ProgressDialog
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import android.widget.ProgressBar
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.GridLayoutManager
import be.dabradio.app.connectivity.BluetoothConnection
import be.dabradio.app.connectivity.Command
import kotlinx.android.synthetic.main.main_activity.*
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Dispatchers.Main
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.math.roundToInt


class MainActivity : AppCompatActivity() {

    // keep track of original bluetooth state, so when the app exits, the bluetooth setting will
    // revert to its original state
    var bluetoothOriginallyEnabled : Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main_activity)
    }

    lateinit var progressDialogue : ProgressDialog

    override fun onStart() {
        super.onStart()

        // check if the user has bluetooth enabled manually
        bluetoothOriginallyEnabled = if (BluetoothConnection.hasBluetoothSupport()) {
            BluetoothConnection.isBluetoothEnabled()
        } else {
            false
        }

        // Check if device has bluetooth support, and if so enable it if it was disabled originally
        if (BluetoothConnection.hasBluetoothSupport() && (bluetoothOriginallyEnabled || BluetoothConnection.enableBluetooth())) {
//            button.apply {
//                setOnClickListener {
//                    init()
//                    text = "Activated!"
//                }
//                text = "Click to activate"
//            }
            progressDialogue = ProgressDialog.show(this, "Verbinden", "Proberen te connecteren met de radio...", true)
            init()
        } else {
            currentVolume.text = "BT not available"
        }
    }

    override fun onStop() {
        BluetoothConnection.disconnect()
        // disable the bluetooth service if the user didn't have bluetooth enabled before they
        // opened the app
        if (!bluetoothOriginallyEnabled) {
            BluetoothConnection.disableBluetooth()
        }
        super.onStop()
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        val action: Int = event.action
        return when (event.keyCode) {
            KeyEvent.KEYCODE_VOLUME_UP -> {
                if (action == KeyEvent.ACTION_DOWN) {
                    BluetoothConnection.increaseVolume()
                }
                true
            }
            KeyEvent.KEYCODE_VOLUME_DOWN -> {
                if (action == KeyEvent.ACTION_DOWN) {
                    BluetoothConnection.decreaseVolume()
                }
                true
            }
            else -> super.dispatchKeyEvent(event)
        }
    }

    private fun init() {
        CoroutineScope(Dispatchers.IO).launch {
            with (BluetoothConnection.init()) {
                if (this != null) {
                    withContext(Dispatchers.Main) {
                        stop_button.setOnClickListener {
                            val byteArray = ByteArray(1);
                            byteArray[0] = Command.STOP_SERVICE
                            BluetoothConnection.send(byteArray)
                        }
                        refresh_button.setOnClickListener {
                            progressDialogue = ProgressDialog.show(this@MainActivity, "Vernieuwen", "Er wordt opnieuw achter radio stations gezocht", true)
                            val byteArray = ByteArray(1)
                            byteArray[0] = Command.FULL_SCAN
                            BluetoothConnection.send(byteArray)
                        }
                        volumeIcon.setOnClickListener {
                            if (BluetoothConnection.toggleMute()) {
                                // niet gemute
                                volumeIcon.setImageResource(R.drawable.ic_baseline_volume_up_24)
                            } else {
                                // wel gemute
                                volumeIcon.setImageResource(R.drawable.ic_baseline_volume_off_24)
                            }
                        }
                    }
                    this.collect {
                        withContext(Dispatchers.Main) {
                            when (it.commandHeader) {
                                Command.SERVICE_LIST -> {
                                    with(it.commandData as Command.CommandData.ServiceList) {
                                        Service.availableServices.clear()
                                        Service.availableServices.addAll(this.serviceList)
                                        serviceRecyclerView.adapter =
                                            ServiceListAdapter(Service.availableServices)
                                        serviceRecyclerView.layoutManager =
                                            GridLayoutManager(this@MainActivity, 2)
                                        if (progressDialogue.isShowing) {
                                            progressDialogue.dismiss()
                                        }
                                    }
                                }
                                Command.CURRENT_VOLUME -> {
                                    with(it.commandData as Command.CommandData.Volume) {
                                        currentVolume.text = "Volume: ${(this.newVolume / 63f * 100).roundToInt()}%"
                                        if (this.newVolume != 0) {
                                            volumeIcon.setImageResource(R.drawable.ic_baseline_volume_up_24)
                                        } else {
                                            volumeIcon.setImageResource(R.drawable.ic_baseline_volume_off_24)
                                        }
                                    }
                                }
                                Command.CURRENT_SERVICE -> {
                                    with(it.commandData as Command.CommandData.CurrentService) {
                                        if (this.currentService == null) {
                                            currently_playing.text = "Geen radiostation actief."
                                            currently_playing_title.visibility = View.GONE
                                        } else {
                                            currently_playing.text = this.currentService.name
                                            currently_playing_title.visibility = View.VISIBLE
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    withContext(Dispatchers.Main) {
                        if (progressDialogue.isShowing) {
                            progressDialogue.dismiss()
                        }
                        Toast.makeText(this@MainActivity, "Kon geen verbinding maken met de radio", Toast.LENGTH_LONG).show()
                        stop_button.setOnClickListener {
                            Toast.makeText(
                            this@MainActivity,
                            "Niet beschikbaar",
                            Toast.LENGTH_LONG
                            ).show()
                        }
                        refresh_button.setOnClickListener {
                            Toast.makeText(
                            this@MainActivity,
                            "Niet beschikbaar",
                            Toast.LENGTH_LONG
                            ).show()
                        }
                        volumeIcon.setOnClickListener {
                            Toast.makeText(
                            this@MainActivity,
                            "Niet beschikbaar",
                            Toast.LENGTH_LONG
                            ).show()
                        }
                    }
                }
            }
        }
    }

}
