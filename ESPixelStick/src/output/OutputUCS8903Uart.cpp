/*
* UCS8903Uart.cpp - UCS8903 driver code for ESPixelStick UART
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2015 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include "../ESPixelStick.h"

#if defined(SUPPORT_OutputType_UCS8903) && defined(SUPPORT_UART_OUTPUT)

#include "OutputUCS8903Uart.hpp"

#if defined(ARDUINO_ARCH_ESP8266)
extern "C" {
#   include <eagle_soc.h>
#   include <ets_sys.h>
#   include <uart.h>
#   include <uart_register.h>
}
#elif defined(ARDUINO_ARCH_ESP32)
#   include <soc/uart_reg.h>

// Define ESP8266 style macro conversions to limit changes in the rest of the code.
#   define UART_CONF0           UART_CONF0_REG
#   define UART_CONF1           UART_CONF1_REG
#   define UART_INT_ENA         UART_INT_ENA_REG
#   define UART_INT_CLR         UART_INT_CLR_REG
#   define UART_INT_ST          UART_INT_ST_REG
#   define UART_TX_FIFO_SIZE    UART_FIFO_LEN
#endif

#ifndef UART_INV_MASK
#   define UART_INV_MASK  (0x3f << 19)
#endif // ndef UART_INV_MASK

// TX FIFO trigger level. 40 bytes gives 100us before the FIFO goes empty
// We need to fill the FIFO at a rate faster than 0.3us per byte (1.2us/pixel)
#define PIXEL_FIFO_TRIGGER_LEVEL (16)

/*
 * Inverted 8N1 UART lookup table for UCS8903.
 * Start and stop bits are part of the pixel stream.
 */
char Convert2BitIntensityToUCS8903UartDataStream[] =
{
    0b11111100, // 0
    0b11000000  // 1
};
#define UCS8903_NUM_UART_BITS_PER_INTENSITY_BIT 10

// forward declaration for the isr handler
static void IRAM_ATTR uart_intr_handler (void* param);

//----------------------------------------------------------------------------
c_OutputUCS8903Uart::c_OutputUCS8903Uart (c_OutputMgr::e_OutputChannelIds OutputChannelId,
    gpio_num_t outputGpio,
    uart_port_t uart,
    c_OutputMgr::e_OutputType outputType) :
    c_OutputUCS8903 (OutputChannelId, outputGpio, uart, outputType)
{
    // DEBUG_START;

    // DEBUG_END;
} // c_OutputUCS8903Uart

//----------------------------------------------------------------------------
c_OutputUCS8903Uart::~c_OutputUCS8903Uart ()
{
    // DEBUG_START;

    if (HasBeenInitialized)
    {
        // Disable all interrupts for this uart.
        CLEAR_PERI_REG_MASK(UART_INT_ENA(UartId), UART_INTR_MASK);
        // DEBUG_V ("");

        // Clear all pending interrupts in the UART
        WRITE_PERI_REG(UART_INT_CLR(UartId), UART_INTR_MASK);
        // DEBUG_V ("");

#ifdef ARDUINO_ARCH_ESP8266
        Serial1.end();
#else

        // make sure no existing low level driver is running
        ESP_ERROR_CHECK(uart_disable_tx_intr(UartId));
        // DEBUG_V ("");

        ESP_ERROR_CHECK(uart_disable_rx_intr(UartId));
        // DEBUG_V (String("UartId: ") + String(UartId));

        // todo: put back uart_isr_free (UartId);

#endif // def ARDUINO_ARCH_ESP32
    }

    // DEBUG_END;
} // ~c_OutputUCS8903Uart

//----------------------------------------------------------------------------
/* shell function to set the 'this' pointer of the real ISR
   This allows me to use non static variables in the ISR.
 */
static void IRAM_ATTR uart_intr_handler (void* param)
{
    reinterpret_cast <c_OutputUCS8903Uart*>(param)->ISR_Handler ();
} // uart_intr_handler

//----------------------------------------------------------------------------
/* Use the current config to set up the output port
*/
void c_OutputUCS8903Uart::Begin ()
{
    // DEBUG_START;
    // DEBUG_V(String("UCS8903_PIXEL_NS_BIT_TOTAL: ") + String(UCS8903_PIXEL_NS_BIT_TOTAL));
    // DEBUG_V(String("   UCS8903_PIXEL_DATA_RATE: ") + String(UCS8903_PIXEL_DATA_RATE));

#ifdef ARDUINO_ARCH_ESP8266
    InitializeUart(UCS8903_NUM_UART_BITS_PER_INTENSITY_BIT * UCS8903_PIXEL_DATA_RATE,
                   SERIAL_6N1,
                   SERIAL_TX_ONLY,
                   PIXEL_FIFO_TRIGGER_LEVEL);
#else
    /* Serial rate is 4x 800KHz for UCS8903 */
    uart_config_t uart_config;
    memset ((void*)&uart_config, 0x00, sizeof (uart_config));
    uart_config.baud_rate = int((UCS8903_NUM_UART_BITS_PER_INTENSITY_BIT)*UCS8903_PIXEL_DATA_RATE);
    //    uart_config.data_bits = uart_word_length_t::UART_DATA_6_BITS;
    uart_config.data_bits = uart_word_length_t::UART_DATA_8_BITS;
    uart_config.stop_bits = uart_stop_bits_t::UART_STOP_BITS_1;
    InitializeUart (uart_config, PIXEL_FIFO_TRIGGER_LEVEL);
#endif

    // Atttach interrupt handler
    RegisterUartIsrHandler(uart_intr_handler, this, UART_TXFIFO_EMPTY_INT_ENA | ESP_INTR_FLAG_IRAM);

    // invert the output
    CLEAR_PERI_REG_MASK (UART_CONF0 (UartId), UART_INV_MASK);
    SET_PERI_REG_MASK (UART_CONF0 (UartId), (BIT (22)));

#ifdef testPixelInsert
    static const uint32_t FrameStartData = 0;
    static const uint32_t FrameEndData = 0xFFFFFFFF;
    static const uint8_t  PixelStartData = 0xC0;

    SetFramePrependInformation ( (uint8_t*)&FrameStartData, sizeof (FrameStartData));
    SetFrameAppendInformation ( (uint8_t*)&FrameEndData, sizeof (FrameEndData));
    SetPixelPrependInformation (&PixelStartData, sizeof (PixelStartData));
#endif // def testPixelInsert

    HasBeenInitialized = true;

} // init

//----------------------------------------------------------------------------
/* Process the config
*
*   needs
*       reference to string to process
*   returns
*       true - config has been accepted
*       false - Config rejected. Using defaults for invalid settings
*/
bool c_OutputUCS8903Uart::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

    bool response = c_OutputUCS8903::SetConfig (jsonConfig);

#ifdef ARDUINO_ARCH_ESP32
    ESP_ERROR_CHECK (uart_set_pin (UartId, DataPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
#endif

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
void c_OutputUCS8903Uart::SetOutputBufferSize (uint16_t NumChannelsAvailable)
{
    // DEBUG_START;
    // DEBUG_V (String ("NumChannelsAvailable: ") + String (NumChannelsAvailable));
    // DEBUG_V (String ("   GetBufferUsedSize: ") + String (c_OutputCommon::GetBufferUsedSize ()));
    // DEBUG_V (String ("         pixel_count: ") + String (pixel_count));
    // DEBUG_V (String ("       BufferAddress: ") + String ((uint32_t)(c_OutputCommon::GetBufferAddress ())));

    do // once
    {
        // are we changing size?
        if (NumChannelsAvailable == OutputBufferSize)
        {
            // DEBUG_V ("NO Need to change the ISR buffer");
            break;
        }

        // Stop current output operation
#ifdef ARDUINO_ARCH_ESP8266
        CLEAR_PERI_REG_MASK (UART_INT_ENA (UartId), UART_TXFIFO_EMPTY_INT_ENA);
#else
        ESP_ERROR_CHECK (uart_disable_tx_intr (UartId));
#endif
        c_OutputUCS8903::SetOutputBufferSize (NumChannelsAvailable);

    } while (false);

    // DEBUG_END;

} // SetOutputBufferSize

//----------------------------------------------------------------------------
/*
     * Fill the FIFO with as many intensity values as it can hold.
     */
void IRAM_ATTR c_OutputUCS8903Uart::ISR_Handler ()
{
    // Process if the desired UART has raised an interrupt
    if (READ_PERI_REG (UART_INT_ST (UartId)))
    {
        // Fill the FIFO with new data
        // free space in the FIFO divided by the number of data bytes per intensity
        // gives the max number of intensities we can add to the FIFO
        uint32_t NumEmptyIntensitySlots = ((((uint16_t)UART_TX_FIFO_SIZE) - (getFifoLength)) / UCS8903_NUM_UART_DATA_BYTES_PER_INTENSITY_VALUE);
        uint8_t OneValue  = Convert2BitIntensityToUCS8903UartDataStream[1];
        uint8_t ZeroValue = Convert2BitIntensityToUCS8903UartDataStream[0];

        while ((NumEmptyIntensitySlots--) && (ISR_MoreDataToSend()))
        {
            // cant use map function in an ISR. It crashes.
            uint32_t IntensityValue = uint32_t(ISR_GetNextIntensityToSend()) * IntensityMultiplier;
            for (uint16_t mask = 1 << (UCS8903_INTENSITY_DATA_WIDTH - 1); 0 != mask; mask >>= 1)
            {
                // convert the intensity data into UART data
                enqueueUart(((IntensityValue & mask) ? OneValue : ZeroValue));
            }
        } // end while there is data to be sent

        if (!ISR_MoreDataToSend())
        {
            CLEAR_PERI_REG_MASK (UART_INT_ENA (UartId), UART_TXFIFO_EMPTY_INT_ENA);
        }

        // Clear all interrupts flags for this uart
        WRITE_PERI_REG (UART_INT_CLR (UartId), UART_INTR_MASK);

    } // end Our uart generated an interrupt

} // HandleUCS8903Interrupt

//----------------------------------------------------------------------------
void c_OutputUCS8903Uart::Render ()
{
    // DEBUG_START;

    // DEBUG_V (String ("RemainingIntensityCount: ") + RemainingIntensityCount)

    if (gpio_num_t (-1) == DataPin) { return; }
    if (!canRefresh ()) { return; }

    // get the next frame started
    StartNewFrame ();

    // enable interrupts
    WRITE_PERI_REG (UART_CONF1 (UartId), PIXEL_FIFO_TRIGGER_LEVEL << UART_TXFIFO_EMPTY_THRHD_S);
    SET_PERI_REG_MASK (UART_INT_ENA (UartId), UART_TXFIFO_EMPTY_INT_ENA);

    ReportNewFrame ();

    // DEBUG_END;

} // render

//----------------------------------------------------------------------------
void c_OutputUCS8903Uart::PauseOutput ()
{
    // DEBUG_START;

    CLEAR_PERI_REG_MASK (UART_INT_ENA (UartId), UART_TXFIFO_EMPTY_INT_ENA);

    // DEBUG_END;
} // PauseOutput

#endif // defined(SUPPORT_OutputType_UCS8903) && defined(SUPPORT_UART_OUTPUT)
