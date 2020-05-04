package be.dabradio.app

import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import be.dabradio.app.connectivity.BluetoothConnection
import be.dabradio.app.connectivity.Command
import kotlinx.android.synthetic.main.main_activity.*
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers.Main
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch


class MainActivity : AppCompatActivity() {

    // keep track of original bluetooth state, so when the app exits, the bluetooth setting will
    // revert to its original state
    var bluetoothOriginallyEnabled : Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main_activity)
    }



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
            button.apply {
                setOnClickListener {
                    init()
                    text = "Activated!"
                }
                text = "Click to activate"
            }
        } else {
            button.text = "BT not available"
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

//    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
//        val action: Int = event.action
//        return when (event.keyCode) {
//            KeyEvent.KEYCODE_VOLUME_UP -> {
//                if (action == KeyEvent.ACTION_DOWN) {
//                    ESP32.set(VOLUME, ESP32.currentVolume + 5)
//                    currentVolume.text = "Current volume: ${ESP32.currentVolume}%"
//                }
//                true
//            }
//            KeyEvent.KEYCODE_VOLUME_DOWN -> {
//                if (action == KeyEvent.ACTION_DOWN) {
//                    ESP32.set(VOLUME, ESP32.currentVolume - 5)
//                    currentVolume.text = "Current volume: ${ESP32.currentVolume}%"
//                }
//                true
//            }
//            else -> super.dispatchKeyEvent(event)
//        }
//    }

    private fun init() {
        BluetoothConnection.init()
        CoroutineScope(Main).launch {
            BluetoothConnection.startCommunication().collect {
                when (it.instruction) {
                    Command.VOLUME -> {
                        Toast.makeText(baseContext, "Received volume information: ${it.response!![1]}", Toast.LENGTH_LONG).show()
                    }
                }
            }
        }
    }

}
