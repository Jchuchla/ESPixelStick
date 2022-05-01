#pragma once
/*
 * OutputUart.hpp - Uart driver code for ESPixelStick Uart Channel
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
#ifdef SUPPORT_UART_OUTPUT

#include "OutputPixel.hpp"
#include "OutputSerial.hpp"

class c_OutputUart
{
public:
    enum UartDataSize_t
    {
        OUTPUT_UART_5N1 = 0,
        OUTPUT_UART_5N2,
        OUTPUT_UART_6N1,
        OUTPUT_UART_6N2,
        OUTPUT_UART_7N1,
        OUTPUT_UART_7N2,
        OUTPUT_UART_8N1,
        OUTPUT_UART_8N2,
    };

// TX FIFO trigger level. 40 bytes gives 100us before the FIFO goes empty
// We need to fill the FIFO at a rate faster than 0.3us per byte (1.2us/pixel)
#define DEFAULT_UART_FIFO_TRIGGER_LEVEL (16)

    struct OutputUartConfig_t
    {
        c_OutputCommon::OID_t ChannelId              = c_OutputCommon::OID_t(-1);
        gpio_num_t     DataPin                       = gpio_num_t(-1);
        uart_port_t    UartId                        = uart_port_t(-1);
        size_t         IntensityDataWidth            = 8; // 8 bits in a byte
        size_t         NumUartSlotsPerIntensityValue = 1;
        UartDataSize_t UartDataSize                  = UartDataSize_t::OUTPUT_UART_8N2;
        bool           TranslateIntensityData        = false;
        bool           InvertOutputPolarity          = false;
        uint32_t       Baudrate                      = 57600; // current transmit rate
        c_OutputPixel *pPixelDataSource              = nullptr;
        uint32_t       FiFoTriggerLevel              = DEFAULT_UART_FIFO_TRIGGER_LEVEL;

#if defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
        c_OutputSerial *pSerialDataSource = nullptr;
#endif // defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    };

    c_OutputUart                 ();
    virtual ~c_OutputUart        ();
    void Begin                   (c_OutputUart::OutputUartConfig_t & config);
    bool SetConfig               (ArduinoJson::JsonObject &jsonConfig);
    void GetConfig               (ArduinoJson::JsonObject &jsonConfig);
    void GetDriverName           (String &sDriverName) { sDriverName = String(F("OutputUart")); }
    void GetStatus               (ArduinoJson::JsonObject &jsonStatus);
    void PauseOutput             (bool State);
    void SetMinFrameDurationInUs (uint32_t value);
    void StartNewFrame           ();
    void SetSendBreak            (bool value);
    void SetIntensityDataWidth   (uint32_t DataWidth, size_t _NumUartSlotsPerIntensityValue = 1);

    void IRAM_ATTR ISR_Handler();

    enum UartDataBitTranslationId_t
    {
        Uart_DATA_BIT_00_ID = 0,
        Uart_DATA_BIT_01_ID,
        Uart_DATA_BIT_10_ID,
        Uart_DATA_BIT_11_ID,
        Uart_LIST_END,
        Uart_INVALID_VALUE,
        Uart_NUM_BIT_TYPES = Uart_LIST_END,
    };
    void SetIntensity2Uart(uint8_t value, UartDataBitTranslationId_t ID);

private:
    void StartUart              ();
    bool RegisterUartIsrHandler ();
    void InitializeUart         ();
    void set_pin                ();
    void TerminateUartOperation ();
    void ReportNewFrame         ();
    void StartBreak             ();
    void EndBreak               ();
    void GenerateBreak          (uint32_t DurationInUs, uint32_t MarkDurationInUs);

    OutputUartConfig_t OutputUartConfig;

    uint8_t Intensity2Uart[UartDataBitTranslationId_t::Uart_LIST_END];
    bool           OutputIsPaused               = false;
    bool           SendBreak                    = false;
    uint32_t       LastFrameStartTime           = 0;
    uint32_t       FrameMinDurationInMicroSec   = 25000;
    long           IntensityMapDstMax           = 255;
    uint32_t       TxIntensityDataStartingMask  = 0x80;
    bool           HasBeenInitialized           = false;
#if defined(ARDUINO_ARCH_ESP32)
    intr_handle_t  IsrHandle                     = nullptr;
#endif // defined(ARDUINO_ARCH_ESP32)

void IRAM_ATTR ISR_Handler_SendIntensityData();

inline bool IRAM_ATTR MoreDataToSend()
{
    if (nullptr != OutputUartConfig.pPixelDataSource)
    {
        return OutputUartConfig.pPixelDataSource->ISR_MoreDataToSend();
    }
#if defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
        return OutputUartConfig.pSerialDataSource->ISR_MoreDataToSend();
#else
        return false;
#endif // defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    }

inline uint8_t IRAM_ATTR GetNextIntensityToSend()
{
    if (nullptr != OutputUartConfig.pPixelDataSource)
    {
        return OutputUartConfig.pPixelDataSource->ISR_GetNextIntensityToSend();
    }
#if defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    else
    {
        return OutputUartConfig.pSerialDataSource->ISR_GetNextIntensityToSend();
    }
#else
    return false;
#endif // defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    }

inline void IRAM_ATTR StartNewDataFrame()
{
    if (nullptr != OutputUartConfig.pPixelDataSource)
    {
        OutputUartConfig.pPixelDataSource->StartNewFrame();
    }
#if defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    else
    {
        OutputUartConfig.pSerialDataSource->StartNewFrame();
    }
#endif // defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
    }

// #define USE_UART_DEBUG_COUNTERS
#ifdef USE_UART_DEBUG_COUNTERS
    // debug counters
    uint32_t DataCallbackCounter = 0;
    uint32_t DataTaskcounter = 0;
    uint32_t DataISRcounter = 0;
    uint32_t FrameThresholdCounter = 0;
    uint32_t FrameEndISRcounter = 0;
    uint32_t FrameStartCounter = 0;
    uint32_t RxIsr = 0;
    uint32_t ErrorIsr = 0;
    uint32_t IsrIsNotForUs = 0;
    uint32_t IntensityBytesSent = 0;
    uint32_t IntensityBitsSent = 0;
    uint32_t IntensityBytesSentLastFrame = 0;
    uint32_t IntensityBitsSentLastFrame = 0;
    uint32_t IncompleteFrame = 0;
    uint32_t IncompleteFrameLastFrame = 0;
    uint32_t EnqueueCounter = 0;
#endif // def USE_UART_DEBUG_COUNTERS

#define DisableUartInterrupts CLEAR_PERI_REG_MASK(UART_INT_ENA(OutputUartConfig.UartId), UART_TXFIFO_EMPTY_INT_ENA);
#define EnableUartInterrupts  SET_PERI_REG_MASK  (UART_INT_ENA(OutputUartConfig.UartId), UART_TXFIFO_EMPTY_INT_ENA);

#ifndef ESP_INTR_FLAG_IRAM
#   define ESP_INTR_FLAG_IRAM 0
#endif // ndef ESP_INTR_FLAG_IRAM

#ifdef ARDUINO_ARCH_ESP8266
    /* Returns number of bytes waiting in the TX FIFO of UART1 */
#define getUartFifoLength ((uint16_t)((U1S >> USTXC) & 0xff))

    /* Append a byte to the TX FIFO of UART1 */
    inline void IRAM_ATTR enqueueUartData(uint8_t value)
    {
#ifdef USE_UART_DEBUG_COUNTERS
    EnqueueCounter++;
#endif // def USE_UART_DEBUG_COUNTERS
    (U1F = (char)(value));
}

#elif defined(ARDUINO_ARCH_ESP32)

    /* Returns number of bytes waiting in the TX FIFO of UART1 */
#   define getUartFifoLength ((uint16_t)((READ_PERI_REG(UART_STATUS_REG(OutputUartConfig.UartId)) & UART_TXFIFO_CNT_M) >> UART_TXFIFO_CNT_S))

    /* Append a byte to the TX FIFO of UART1 */
inline void IRAM_ATTR enqueueUartData(uint8_t value)
{
#ifdef USE_UART_DEBUG_COUNTERS
    EnqueueCounter++;
#endif // def USE_UART_DEBUG_COUNTERS
    (*((volatile uint32_t *)(UART_FIFO_AHB_REG(OutputUartConfig.UartId)))) = (uint32_t)(value);
}

#endif //  defined(ARDUINO_ARCH_ESP32)
};
#endif // def #ifdef SUPPORT_UART_OUTPUT
