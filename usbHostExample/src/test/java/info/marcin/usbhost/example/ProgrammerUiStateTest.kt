package info.marcin.usbhost.example

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ProgrammerUiStateTest {
    @Test
    fun `hex preview formats address bytes and printable ascii`() {
        val lines = formatHexPreview(0x08000000, byteArrayOf(0x41, 0x00, 0x7e, 0xff.toByte()), 4)

        assertEquals(1, lines.size)
        assertEquals(0x08000000, lines.single().address)
        assertEquals("41 00 7E FF", lines.single().hex)
        assertEquals("A.~.", lines.single().ascii)
    }

    @Test
    fun `operation state exposes stable busy and connected flags`() {
        assertTrue(ProgrammerUiState(operation = OperationState.CONNECTING).busy)
        assertTrue(ProgrammerUiState(operation = OperationState.CONNECTED).connected)
        assertFalse(ProgrammerUiState(operation = OperationState.ERROR).connected)
    }
}
