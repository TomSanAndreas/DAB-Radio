package be.dabradio.app.connectivity

class Command {

    companion object {

        const val NO_RESPONSE_REQUESTED : Byte = 0
        const val VOLUME : Byte = 1
        const val CHANNEL : Byte = 2

//    const val ESP32_VOLUME : Byte = 3
//    const val ESP32_CHANNEL_LIST : Byte = 4
//    const val ESP32_SONG_NAME : Byte = 5
//    const val ESP32_SONG_ARTIST : Byte = 6
//    const val ESP32_SONG_ARTWORK : Byte = 7
//    const val ESP32_CHANNEL : Byte = 8

        const val CURRENT_CHANNEL_LIST : Byte = 64
        const val CURRENT_SONG_NAME : Byte = 65
        const val CURRENT_SONG_ARTIST : Byte = 66
        const val CURRENT_SONG_ARTWORK : Byte = 67
        const val CURRENT_CHANNEL : Byte = 68
        const val CURRENT_VOLUME : Byte = 69

        const val LOG : Byte = 125
        const val CONNECT : Byte = 126
        const val DISCONNECT : Byte = 127

    }

    val instruction : Byte
    val fullArray : ByteArray
    val responseCode : Byte
    var response : ByteArray? = null

//    private var onResponse : ((ByteArray) -> Unit)? = null

    constructor(instruction : Byte, responseCode : Byte = NO_RESPONSE_REQUESTED) {
        this.instruction = instruction
        this.responseCode = responseCode
        this.fullArray = ByteArray(1)
        this.fullArray[0] = instruction
    }

    constructor(instruction: Byte, extraData : ByteArray, responseCode: Byte = NO_RESPONSE_REQUESTED) {
        this.instruction = instruction
        this.responseCode = responseCode
        this.fullArray = ByteArray(extraData.size + 1)
        this.fullArray[0] = instruction
        extraData.copyInto(this.fullArray, 1)
    }

    constructor(fullCommand : ByteArray, responseCode: Byte = NO_RESPONSE_REQUESTED) {
        this.responseCode = responseCode
        this.instruction = fullCommand[0]
        this.fullArray = fullCommand.copyOf()
    }

//    fun doOnResponse(action : (ByteArray) -> Unit) {
//        onResponse = action
//    }
//
//    fun hasReceivedResponse(response : ByteArray) {
//        onResponse?.let { it(response) }
//    }

}