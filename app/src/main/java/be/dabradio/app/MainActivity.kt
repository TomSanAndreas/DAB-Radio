package be.dabradio.app

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.os.Bundle
import android.view.KeyEvent
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.snackbar.Snackbar
import kotlinx.android.synthetic.main.main_activity.*
import be.dabradio.app.ESP32.CURRENT_CHANNEL
import be.dabradio.app.ESP32.CURRENT_CHANNEL_LIST
import be.dabradio.app.ESP32.CURRENT_SONG_ARTIST
import be.dabradio.app.ESP32.CURRENT_SONG_NAME
import be.dabradio.app.ESP32.CURRENT_SONG_ARTWORK
import be.dabradio.app.ESP32.CURRENT_VOLUME
import be.dabradio.app.ESP32.VOLUME
import be.dabradio.app.ESP32.CHANNEL
import be.dabradio.app.ESP32.currentVolume
import java.io.IOException
import java.util.*


class MainActivity : AppCompatActivity() {

    private val REQUEST_ENABLE_BLUETOOTH = 256

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main_activity)

//        setVolume.setOnClickListener {
//            val randomPercent = Random().nextFloat() % 1
//            setVolume.text = "Current volume: $randomPercent%"
//            ESP32.setVolume(randomPercent)
//        }
    }

    private val btAdapter : BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()

    override fun onStart() {
        super.onStart()
        if (btAdapter == null) {
            Snackbar.make(root, "Dit apparaat ondersteunt bluetooth niet.", Snackbar.LENGTH_INDEFINITE).show()
        } else {
            if (!btAdapter.isEnabled) {
                val enableBluetooth = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
                startActivityForResult(enableBluetooth, REQUEST_ENABLE_BLUETOOTH)
            } else {
                initialiseConnection()
            }
        }
    }

    override fun onStop() {
        ESP32.disconnect()
        super.onStop()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == REQUEST_ENABLE_BLUETOOTH) {
            initialiseConnection()
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        val action: Int = event.action
        return when (event.keyCode) {
            KeyEvent.KEYCODE_VOLUME_UP -> {
                if (action == KeyEvent.ACTION_DOWN) {
                    ESP32.set(VOLUME, ESP32.currentVolume + 5)
                    currentVolume.text = "Current volume: ${ESP32.currentVolume}%"
                }
                true
            }
            KeyEvent.KEYCODE_VOLUME_DOWN -> {
                if (action == KeyEvent.ACTION_DOWN) {
                    ESP32.set(VOLUME, ESP32.currentVolume - 5)
                    currentVolume.text = "Current volume: ${ESP32.currentVolume}%"
                }
                true
            }
            else -> super.dispatchKeyEvent(event)
        }
    }

    private fun initialiseConnection() {
        var esp32 : BluetoothDevice? = null
        btAdapter!!.bondedDevices.forEach { btDevice : BluetoothDevice ->
            if (btDevice.address == ESP32.MAC_ADDRESS || btDevice.name == "DAB_RADIO") {
                esp32 = btDevice
            }
        }
        if (esp32 == null) {
            Snackbar.make(root, "Pair eerst met de DAB-Radio", Snackbar.LENGTH_INDEFINITE)
        } else {
            ESP32.init(esp32)
            ESP32.connect(object : ESP32.ConnectCallback {
                override fun onSuccess() {
                    runOnUiThread {
                        Toast.makeText(baseContext, "Verbonden met ESP32", Toast.LENGTH_LONG).show()
                    }
                }

                override fun onNewLogEntry(newText: String) {
                    runOnUiThread {
                        log.text = log.text.toString() + newText
                    }
                }

                override fun onFailure(e: IOException) {
                    runOnUiThread {
                        Snackbar.make(root, e.toString(), Snackbar.LENGTH_INDEFINITE).show()
                    }
                }
            })
        }
    }

}
