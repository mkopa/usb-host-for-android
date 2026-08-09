package info.marcin.usbhost.example

import org.junit.Assert.assertFalse
import org.junit.Test
import java.io.File

class ReadOnlySampleContractTest {
    @Test
    fun `sample source exposes no target mutation commands`() {
        val sourceRoot = File("src/main/java/info/marcin/usbhost/example")
        val source = sourceRoot.walkTopDown().filter { it.extension == "kt" }
            .joinToString("\n") { it.readText() }.lowercase()
        listOf("mass_erase", "write_memory", "option_byte", "reset_target", "halt_target").forEach {
            assertFalse("Unexpected mutating command: $it", source.contains(it))
        }
    }
}
