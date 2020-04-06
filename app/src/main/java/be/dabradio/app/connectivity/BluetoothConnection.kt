package be.dabradio.app.connectivity

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream

object BluetoothConnection {

//    const val REQUEST_ENABLE_BLUETOOTH = 256
    private val btAdapter : BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
    const val MAC_ADDRESS = "84:0D:8E:E3:AA:26"
    private var connectedWithESP : Boolean = false
    private var btBroadcast : BluetoothSocket? = null
    private var btInputStream : InputStream? = null
    private var btOutputStream : OutputStream? = null

    private val pendingCommands = ArrayList<Command>(16)
    private var currentCommandIndex = 0

    fun hasBluetoothSupport() : Boolean { // returns whether bluetooth is supported
        return btAdapter != null
    }

    fun enableBluetooth() : Boolean {
        return btAdapter!!.enable()
    }

    fun init() {
        CoroutineScope(IO).launch { connectWithESP32() }
    }

    fun startCommunication() : Flow<Command> = flow {
        withContext(IO) {
            delay(500)
            while (connectedWithESP) {
                if (connectedWithESP && pendingCommands.size > 16) {
                    // no responses received for the commands, exiting loop to prevent overflow
                    pendingCommands.clear()
                    break
                }
                if (connectedWithESP && btInputStream!!.available() > 0) {
                    // there is at least 1 response or log available
                    val buffer = btInputStream!!.readBytes()
                    //TODO : check the rest of the buffer if it contains anything other then the log message
                    if (buffer[0] == Command.LOG) {
                        // command is een log
                        println("TODO: Received a Log")
                    } else {
                        // see which command from the requested commands got its answer
                        val requestedCommand : Command? = pendingCommands.first { it.responseCode == buffer[0] }
                        if (requestedCommand == null) {
                            println("Error: received unexpected response code ${buffer[0]}")
                        } else {
                            // TODO : decrease the buffer if necessary, so the other parts can be
                            // TODO : checked for other command responses as well
                            requestedCommand.response = buffer
                            emit(requestedCommand)
                            if (pendingCommands.indexOf(requestedCommand) >= currentCommandIndex) {
                                --currentCommandIndex
                            }
                            pendingCommands.remove(requestedCommand)
                        }
                    }
                }
                if (connectedWithESP && pendingCommands.size > currentCommandIndex + 1) {
                    // send 1 command
                    if (pendingCommands[currentCommandIndex].instruction == Command.DISCONNECT) {
                        btOutputStream!!.write(pendingCommands[currentCommandIndex].fullArray)
                        delay(100)
                        println("Disconnecting...")
                        btBroadcast!!.close()
                        break
                    } else {
                        btOutputStream!!.write(pendingCommands[currentCommandIndex].fullArray)
                        ++currentCommandIndex
                    }
                }
                delay(100)
            }
        }
    }

    private suspend fun connectWithESP32() : Boolean {
        return withContext(IO) {
            if (!connectedWithESP) {
                // TODO : make it so the user can select a device from a list of available devices
                // TODO : if the ESP32 cannot be detected
                val esp32: BluetoothDevice? = btAdapter!!.bondedDevices.first { it.address == MAC_ADDRESS || it.name == "DAB_RADIO" }
                connectedWithESP = if (esp32 == null) {
                    false
                } else {
                    btBroadcast = esp32.createRfcommSocketToServiceRecord(esp32.uuids.first().uuid) // FIXME : de UUIDS beter gebruiken
                    if (btBroadcast != null) {
                        try {
                            btBroadcast!!.connect()
                            btInputStream = btBroadcast!!.inputStream
                            btOutputStream = btBroadcast!!.outputStream
                            send(Command.CONNECT)
                            pendingCommands.clear()
                            true
                        } catch (e: IOException) {
                            e.printStackTrace()
                            false
                        }
                    } else {
                        false
                    }
                }
                connectedWithESP
            } else {
                true
            }
        }
    }

    fun disconnect() {
        if (connectedWithESP) {
            send(Command.DISCONNECT)
            connectedWithESP = false
        }
    }

    private fun send(command: Command) {
        pendingCommands.add(command)
//        if (connectedWithESP) {
//            if (command.responseCode != Command.NO_RESPONSE_REQUESTED) {
//                pendingCommands.add(command)
//            }
//            withContext(IO) {
//                btOutputStream!!.write(command.fullArray)
//            }
//        }
    }

    private fun send(instruction: Byte) {
        pendingCommands.add(Command(instruction))
//        if (connectedWithESP) {
//            withContext(IO) {
//                val bArray = ByteArray(1)
//                bArray[0] = instruction
//                btOutputStream!!.write(bArray)
//            }
//        }
    }
}