#pragma once
/*
* ESPixelStick.h
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2016, 2022 Shelby Merrick
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
#include "./backported.h"
 
// The following template functions' first parameter is defined
// as a **reference** to an array of characters.  The size of
// the array is the template parameter.  This allows the function
// to statically assert that the passed-in buffer is large enough
// to always succeed.  Else, a compilation will occur.
//
// Allowing the compiler to deduce the template parameters has
// multiple benefits:
// * Users can ignore that the function is a template
// * Code remains easy to read
// * Compiler doesn't deduce wrong size
// * Even if a user manually enters a larger template size,
//   the compiler will report an error, such as:
//   error: invalid initialization of reference of type 'char (&)[15]'
//          from expression of type 'char [12]'



// Safer RGB to HTML string (e.g., "#FF8833") conversion function
//
// Example use:
//     char foo[8];
//     ESP_ERROR_CHECK(saferRgbToHtmlColorString(foo, led.r, led.g, led.b));
//
template <size_t N>
inline esp_err_t saferRgbToHtmlColorString(char (&output)[N], uint8_t r, uint8_t g, uint8_t b) {
    // Including the trailing null, this string requires eight (8) characters.
    //
    // The output is formatted as "#RRGGBB", where RR, GG, and BB are two hex digits
    // for the red, green, and blue components, respectively.
    static_assert(N >= 8);
    static_assert(sizeof(int) <= sizeof(size_t)); // casting non-negative int to size_t is safe
    int wouldHaveWrittenChars = snprintf(output, N, "#%02x%02x%02x", r, g, b);
    if (likely((wouldHaveWrittenChars > 0) && (((size_t)wouldHaveWrittenChars) < N))) {
        return ESP_OK;
    }
    // TODO: assert ((wouldHaveWrittenChars > 0) && (wouldHaveWrittenChars < N))
    return ESP_FAIL;
}
// Safer seconds to "Minutes:Seconds" string conversion function
//
// Example use:
//     char foo[12];
//     ESP_ERROR_CHECK(saferSecondsToFormattedMinutesAndSecondsString(foo, seconds));
//
template <size_t N>
inline esp_err_t saferSecondsToFormattedMinutesAndSecondsString(char (&output)[N], uint32_t seconds) {

    // Including the trailing null, the string may require up to twelve (12) characters.
    //
    // The output is formatted as "{minutes}:{seconds}".
    // uint32_t seconds is in range [0..4294967295].
    // therefore, minutes is in range [0..71582788] (eight characters).
    // seconds is always exactly two characters.
    static_assert(N >= 12);
    static_assert(sizeof(int) <= sizeof(size_t)); // casting non-negative int to size_t is safe
    uint32_t m = seconds / 60u;
    uint8_t  s = seconds % 60u;
    int wouldHaveWrittenChars = snprintf(output, N, "%u:%02u", m, s);
    if (likely((wouldHaveWrittenChars > 0) && (((size_t)wouldHaveWrittenChars) < N))) {
        return ESP_OK;
    }
    // TODO: assert ((wouldHaveWrittenChars > 0) && (wouldHaveWrittenChars < N))
    return ESP_FAIL;
}
// Safer mac to string (e.g., "00:11:22:33:44:55") string conversion function
//
// Example use:
//     uint8_t mac[6];
//     GetMacAddress(mac); // fill the values
//     char foo[18];
//     ESP_ERROR_CHECK(saferMacAddressToString(foo, mac));
//
template <size_t N, size_t M>
inline esp_err_t saferMacAddressToString(char (&output)[N], uint8_t (&mac)[M]) {
    static_assert(N >= 18); // Including the trailing null, the string requires exactly 18 characters.
    static_assert(M >=  6); // MAC address input must be at least 6 bytes
    int wouldHaveWrittenChars =
        snprintf(
            output, N,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
            );
    if (likely((wouldHaveWrittenChars > 0) && (((size_t)wouldHaveWrittenChars) < N))) {
        return ESP_OK;
    }
    // TODO: assert ((wouldHaveWrittenChars > 0) && (wouldHaveWrittenChars < N))
    return ESP_FAIL;
}
