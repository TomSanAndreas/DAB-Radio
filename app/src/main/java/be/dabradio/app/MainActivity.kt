package be.dabradio.app

import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import be.dabradio.app.connectivity.BluetoothConnection
import be.dabradio.app.connectivity.Command
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers.Main
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch


class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main_activity)

//        setVolume.setOnClickListener {
//            val randomPercent = Random().nextFloat() % 1
//            setVolume.text = "Current volume: $randomPercent%"
//            ESP32.setVolume(randomPercent)
//        }
    }



    override fun onStart() {
        super.onStart()
//        if (btAdapter == null) {
//            Snackbar.make(root, "Dit apparaat ondersteunt bluetooth niet.", Snackbar.LENGTH_INDEFINITE).show()
//        } else {
//            if (!btAdapter.isEnabled) {
//                val enableBluetooth = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
//                startActivityForResult(enableBluetooth, REQUEST_ENABLE_BLUETOOTH)
//            } else {
//                initialiseConnection()
//            }
//        }

        // Check if device has bluetooth support, and if so enable it
        if (BluetoothConnection.hasBluetoothSupport() && BluetoothConnection.enableBluetooth()) {
            init()
        }
    }

    override fun onStop() {
//        ESP32.disconnect()
        BluetoothConnection.disconnect()
        super.onStop()
    }

//    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
//        if (requestCode == REQUEST_ENABLE_BLUETOOTH) {
//            initialiseConnection()
//        }
//        super.onActivityResult(requestCode, resultCode, data)
//    }

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
