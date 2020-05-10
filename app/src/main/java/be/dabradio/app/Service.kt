package be.dabradio.app

import be.dabradio.app.connectivity.BluetoothConnection.send
import be.dabradio.app.connectivity.Command
import java.nio.ByteBuffer

class Service(val name: String, val serviceId: Int) {

    fun setActive() {
        val byteArray = ByteArray(5)
        byteArray[0] = Command.SET_SERVICE
        ByteBuffer.allocate(4).putInt(serviceId).array().copyInto(byteArray, 1)
        send(byteArray)
    }

    companion object {
        val availableServices : ArrayList<Service> = ArrayList<Service>()
    }

}