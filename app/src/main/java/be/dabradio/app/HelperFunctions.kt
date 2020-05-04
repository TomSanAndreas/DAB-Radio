package be.dabradio.app

import java.util.NoSuchElementException

// Function already exists within the Kotlin language...
// inline fun <T> Collection<T>.firstOrNull(predicate : (T) -> Boolean) : T? {
//    return try {
//        this.first(predicate)
//    } catch (e : NoSuchElementException) {
//        null
//    }
//}