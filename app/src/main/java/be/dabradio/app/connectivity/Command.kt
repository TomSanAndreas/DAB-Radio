package be.dabradio.app.connectivity

import be.dabradio.app.Service

class Command(val commandHeader: Byte, val commandData : CommandData) {

    companion object {

//        const val NO_RESPONSE_REQUESTED : Byte = 0
//        const val VOLUME : Byte = 1
//        const val CHANNEL : Byte = 2

//    const val ESP32_VOLUME : Byte = 3
//    const val ESP32_CHANNEL_LIST : Byte = 4
//    const val ESP32_SONG_NAME : Byte = 5
//    const val ESP32_SONG_ARTIST : Byte = 6
//    const val ESP32_SONG_ARTWORK : Byte = 7
//    const val ESP32_CHANNEL : Byte = 8

        const val REQUEST_CURRENT_SERVICE_LIST : Byte = 64
        const val REQUEST_CURRENT_SONG_NAME : Byte = 65
        const val REQUEST_CURRENT_SONG_ARTIST : Byte = 66
        const val REQUEST_CURRENT_SONG_ARTWORK : Byte = 67
        const val REQUEST_CURRENT_CHANNEL : Byte = 68
        const val REQUEST_CURRENT_VOLUME : Byte = 69

        const val MUTE_AUDIO : Byte = 95
        const val UNMUTE_AUDIO : Byte = 96
        const val SET_VOLUME : Byte = 97
        const val INCREASE_VOLUME : Byte = 98
        const val DECREASE_VOLUME : Byte = 99

        const val SERVICE_LIST : Byte = 100
        const val CURRENT_VOLUME : Byte = 101
        const val CURRENT_SERVICE : Byte = 102

        const val FULL_SCAN : Byte = 109
        const val SET_SERVICE : Byte = 110
        const val STOP_SERVICE : Byte = 111

        const val LOG : Byte = 125
        const val CONNECT : Byte = 126
        const val DISCONNECT : Byte = 127

    }

    sealed class CommandData() {
        // command data types die ontvangen kunnen worden
        class Log(val logText : String) : CommandData()
        class Volume(val newVolume : Int) : CommandData()
        class ServiceList(val serviceList : List<Service>) : CommandData()
        class CurrentService(val currentService : Service?) : CommandData()
    }

}