package be.dabradio.app.connectivity

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import be.dabradio.app.Service
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.lang.StringBuilder

object BluetoothConnection {

//    const val REQUEST_ENABLE_BLUETOOTH = 256
    private val btAdapter : BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
    const val MAC_ADDRESS = "84:0D:8E:E3:AA:26"
    private var connectedWithESP : Boolean = false
    private var btBroadcast : BluetoothSocket? = null
    private var btInputStream : InputStream? = null
    private var btOutputStream : OutputStream? = null
    private var muted = false

    private val increaseVolumeCommand = ByteArray(1)
    private val decreaseVolumeCommand = ByteArray(1)
    private val muteAudio = ByteArray(1)
    private val unmuteAudio = ByteArray(1)
    private val setVolumeCommand = ByteArray(2)

    init {
        // preset commando's klaarzetten, zoals het volume regelen

        increaseVolumeCommand[0] = Command.INCREASE_VOLUME
        decreaseVolumeCommand[0] = Command.DECREASE_VOLUME


        muteAudio[0] = Command.MUTE_AUDIO
        unmuteAudio[0] = Command.UNMUTE_AUDIO


        setVolumeCommand[0] = Command.SET_VOLUME
        //usage: setVolumeCommand[1] = currentVolume.toByte(), max waarde van het volume (63) zit binnen de grenzen van een signed byte, is dus geen probleem hier!
    }

    fun increaseVolume() {
        send(increaseVolumeCommand)
        muted = false
    }

    fun decreaseVolume() {
        send(decreaseVolumeCommand)
        muted = false
    }

    fun setVolume(newVolume : Int) {
        if (connectedWithESP) {
            setVolumeCommand[1] = newVolume.toByte()
            btOutputStream!!.write(setVolumeCommand)
            muted = false
        }
    }

    fun toggleMute() : Boolean {
        if (connectedWithESP) {
            if (muted) {
                muted = false
                btOutputStream!!.write(unmuteAudio)

            } else {
                muted = true
                btOutputStream!!.write(muteAudio)
            }
            return muted
        }
        return false
    }

    private var currentVolume : Int = 63 // max waarde

    fun hasBluetoothSupport() : Boolean { // returns whether bluetooth is supported
        return btAdapter != null
    }

    fun isBluetoothEnabled() : Boolean {
        return btAdapter!!.isEnabled
    }

    fun enableBluetooth() : Boolean {
        return btAdapter!!.enable()
    }

    fun disableBluetooth() : Boolean {
        return btAdapter!!.disable()
    }

    fun init() : Flow<Command>? {
        return connectWithESP32()
    }

    private fun getLength() : Int {
        return (btInputStream!!.read().shl(8).or(btInputStream!!.read()))
    }

    @ExperimentalCoroutinesApi
    private fun startCommunication() : Flow<Command> = flow {
        delay(500)
        val connectedCmd = ByteArray(1)
        connectedCmd[0] = Command.CONNECT
        btOutputStream!!.write(connectedCmd)
        delay(1000)
        if (connectedWithESP) {

            // start data verwerken, zoals de lijst met services, indien de ESP heeft gereageerd
            if (btInputStream!!.available() > 0) {
                while (connectedWithESP) {
                    try {
                        val cmdType = btInputStream!!.read()
                        when (cmdType) {
                            Command.SERVICE_LIST.toInt() -> {
                                val serviceListByteLength = getLength()
                                val nServices = btInputStream!!.read()
                                val bArray = ByteArray(serviceListByteLength - 1)
                                if (20 * nServices + 1 == serviceListByteLength) {
                                    val availableServices = ArrayList<Service>()
                                    for (i in 0 until nServices) {
                                        val currentServiceLabel = StringBuilder()
                                        var serviceId = 0
                                        for (j in 0 until 16) {
                                            currentServiceLabel.append(
                                                btInputStream!!.read().toChar()
                                            )
                                        }
                                        for (j in 0 until 4) {
                                            serviceId = serviceId.shl(8)
                                            serviceId = serviceId.or(btInputStream!!.read())
                                        }
//                                    println("CURRENTSERVICELABEL: $currentServiceLabel  ${!currentServiceLabel.any { !(it.isLetterOrDigit() || it.isWhitespace() || it == '\'' || it == '-') }}")
                                        if (!currentServiceLabel.any { !(it.isLetterOrDigit() || it.isWhitespace() || it == '\'' || it == '-') }) {
//                                    if (currentServiceLabel.contains(' ')) {
                                            availableServices.add(
                                                Service(
                                                    currentServiceLabel.toString().trim(),
                                                    serviceId
                                                )
                                            )
                                        }
                                    }
                                    emit(
                                        Command(
                                            Command.SERVICE_LIST,
                                            Command.CommandData.ServiceList(availableServices)
                                        )
                                    )
                                } else {
                                    println("Lengte en verwachte lengte komen niet overeen!")
                                }
                            }
                            Command.CURRENT_VOLUME.toInt() -> {
                                if (getLength() == 1) {
                                    currentVolume = btInputStream!!.read()
                                    emit(
                                        Command(
                                            Command.CURRENT_VOLUME,
                                            Command.CommandData.Volume(currentVolume)
                                        )
                                    )
                                } else {
                                    println("Lengte en verwachte lengte komen niet overeen!")
                                }
                            }
                            Command.CURRENT_SERVICE.toInt() -> {
                                if (getLength() == 4) {
                                    val activeServiceId =
                                        btInputStream!!.read().shl(24).or(
                                            btInputStream!!.read().shl(16).or(
                                                btInputStream!!.read().shl(8)
                                                    .or(btInputStream!!.read())
                                            )
                                        )
                                    emit(
                                        Command(
                                            Command.CURRENT_SERVICE,
                                            Command.CommandData.CurrentService(Service.availableServices.firstOrNull { it.serviceId == activeServiceId })
                                        )
                                    )
                                } else {
                                    println("Lengte en verwachte lengte komen niet overeen!")
                                }
                            }
                        }
                    } catch (e : IOException) {
                        e.printStackTrace()
                        break
                    }
                }
            } else {
                connectedWithESP = false
            }
        }
        println("Exiting bluetooth monitoring...")
    }.flowOn(Dispatchers.IO)

    private fun connectWithESP32() : Flow<Command>? {
//        return if (!connectedWithESP) {
        // TODO : make it so the user can select a device from a list of available devices
        // TODO : if the ESP32 cannot be detected
        val esp32: BluetoothDevice? =
            btAdapter!!.bondedDevices.firstOrNull { it.address == MAC_ADDRESS || it.name == "DAB_RADIO" }
        connectedWithESP = if (esp32 == null) {
            false
        } else {
            btBroadcast =
                esp32.createRfcommSocketToServiceRecord(esp32.uuids.first().uuid) // FIXME : de UUIDS beter gebruiken
            if (btBroadcast != null) {
                try {
                    btBroadcast!!.connect()
                    btInputStream = btBroadcast!!.inputStream
                    btOutputStream = btBroadcast!!.outputStream
                    true
                } catch (e: IOException) {
                    e.printStackTrace()
                    false
                }
            } else {
                false
            }
        }
        return if (connectedWithESP) {
            startCommunication()
        } else {
            null
        }
//            } else {
//                startCommunication()
//            }
        }

    fun disconnect() {
        if (connectedWithESP) {
            connectedWithESP = false
            val disconnectCmd = ByteArray(1)
            disconnectCmd[0] = Command.DISCONNECT
            btOutputStream!!.write(disconnectCmd)
            btBroadcast!!.close()
        }
    }

    fun send(byteArray: ByteArray) {
        if (connectedWithESP) {
            btOutputStream!!.write(byteArray)
        }
    }
}