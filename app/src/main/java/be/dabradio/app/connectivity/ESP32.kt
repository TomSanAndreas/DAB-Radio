package be.dabradio.app.connectivity

import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.lang.Thread.sleep

//object ESP32 {
//
//    class RadioData(
//        val currentVolume : Int? = null,
//        val currentSongName : String? = null,
//        val currentSongArtist : String? = null,
//        val currentSongArtwork : String? = null,
//        val currentChannel : String? = null,
//        val currentChannelList : List<String>? = null
//    )

//    var currentVolume : Int = 50
//    var currentSong : String = ""
//    var currentSongArtist : String = ""
//    var currentSongArtwork : String = ""
//    var currentChannel : String = ""
//    var currentChannelList : List<String> = listOf("")
//
//    private var btDevice : BluetoothDevice? = null
//    private var btBroadcast : BluetoothSocket? = null
//    private var connectCallback : ConnectCallback? = null
//
//    private var inputStream : InputStream? = null
//    private var outputStream : OutputStream? = null

    /*
     * requestActive houdt bij of er zelf naar inkomende gegevens moet worden geluisterd of niet:
     * - false:  er werd geen data aangevraagd, data die inkomt wordt op de achtergrond verwerkt
     * - true:   er werd data aangevraagd door de gebruiker, data wordt niet op de achtergrond verwerkt
     */
//    private var requestActive = false
//    var connected = false
//
//    const val MAC_ADDRESS = "84:0D:8E:E3:AA:26"
//
//    interface ConnectCallback {
//        /*
//         * Callback voor het verbinden met een ESP32 module,
//         * met ondersteuning voor success & failure
//         */
//        fun onSuccess()
//        fun onNewLogEntry(newText : String)
//        fun onFailure(e: IOException)
//    }
//
//    fun init(btDevice : BluetoothDevice?) {//, dataCallback: DataCallback) {
//        if (btDevice != null) {
//            ESP32.btDevice = btDevice
//        }
//    }
//
//    fun connect(callback : ConnectCallback) {
//        connectCallback = callback
//        val startConnectionThread = Thread(startConnection)
//        startConnectionThread.start()
//    }
//
//    fun disconnect() {
//        send(DISCONNECT)
//        btBroadcast?.close()
//        connected = false
//    }
//
//    fun get(type: Byte, onResult: (RadioData) -> Unit) {
//        if (connected) {
//            requestActive = true
//            try {
//                when (type) {
//                    CURRENT_VOLUME -> {
//                        Thread {
//                            send(CURRENT_VOLUME)
//                            while (connected && inputStream!!.available() == 0) {
//                                sleep(100)
//                            }
//                            val currentVolume = inputStream!!.read()
//                            println("Current volume : $currentVolume")
//                            onResult(RadioData(currentVolume = currentVolume))
//                        }.start()
//                    }
//                    CURRENT_CHANNEL -> {
//                        println("Not yet implemented")
//                    }
//                    CURRENT_CHANNEL_LIST -> {
//                        println("Not yet implemented")
//                    }
//                    CURRENT_SONG_NAME -> {
//                        println("Not yet implemented")
//                    }
//                    CURRENT_SONG_ARTIST -> {
//                        println("Not yet implemented")
//                    }
//                    CURRENT_SONG_ARTWORK -> {
//                        println("Not yet implemented")
//                    }
//                    else -> {
//                        println("$type not recognized.")
//                    }
//                }
//                requestActive = false
//            } catch (e: IOException){
//                connectCallback!!.onFailure(e)
//            }
//        }
//    }
//
//    fun set(type : Byte, to : Int) {
//        if (connected) {
//            val command = ByteArray(2)
//            command[0] = type
//            when (type) {
//                VOLUME -> {
//                    if (to >= 100) {
//                        command[1] = 100
//                        currentVolume = 100
//                    } else if (to <= 0){
//                        command[1] = 0
//                        currentVolume = 0
//                    } else {
//                        command[1] = to.toByte()
//                        currentVolume = to
//                    }
//                }
//                CHANNEL -> {
//                    command[1] = to.toByte()
//                }
//            }
//            send(command)
//        } else {
//            println("Bluetooth is not connected!")
//        }
//    }
//
////    fun setVolume(value : Float) {
////        if (connected) {
////            val command = ByteArray(2)
////            command[0] = VOLUME
////            command[1] = (255 * (value - value.toInt())).toByte()
////            send(command)
////        } else {
////            println("Bluetooth is not connected!")
////        }
////    }
//
//    private fun send(instructie: Byte) {
//        if (connected) {
//            val bArray = ByteArray(1)
//            bArray[0] = instructie
//            outputStream!!.write(bArray)
//        }
//    }
//
//    private fun send(byteArray: ByteArray) {
//        if (connected) {
//            outputStream!!.write(byteArray)
//        }
//    }
//
//    private val startConnection = Runnable {
//        btBroadcast = btDevice!!.createRfcommSocketToServiceRecord(btDevice!!.uuids.first().uuid)
//        if (btBroadcast != null) {
//            try {
//                btBroadcast!!.connect()
//                connected = true
//                inputStream = btBroadcast!!.inputStream
//                outputStream = btBroadcast!!.outputStream
//                send(CONNECT)
//                connectCallback!!.onSuccess()
//
//                get(CURRENT_VOLUME) {
//                    currentVolume = it.currentVolume!!
//                    println("Current volume is $currentVolume")
//                }
//
//                Thread {
//                    while (connected) {
//                        sleep(500)
//                        // er komt data binnen via de inputstream die niet zelf werd gerequest
//                        while (connected && !requestActive && inputStream!!.available() > 0) {
//                            when (inputStream!!.read().toByte()) {
//                                LOG -> {
//                                    val size = inputStream!!.read() // aantal verwachte tekens, max 255
//                                    val logBytes = ByteArray(size)
//                                    inputStream!!.read(logBytes, 0, size)
//                                    connectCallback!!.onNewLogEntry(String(logBytes))
//                                }
//                                else -> {
//
//                                }
//                            }
//                        }
//                    }
//                }.start()
//            } catch (e: IOException) {
//                connected = false
//                inputStream = null
//                outputStream = null
//                connectCallback!!.onFailure(e)
//            }
//        }
//    }
//}