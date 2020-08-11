/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

/** \ingroup group_class
 *  \defgroup ClassDriver_Audio Audio
 *            Currently only MIDI subclass is supported
 *  @{ */

#ifndef _TUSB_AUDIO_H__
#define _TUSB_AUDIO_H__

#include "common/tusb_common.h"

#ifdef __cplusplus
 extern "C" {
#endif

/// Audio Interface Subclass Codes
typedef enum
{
  AUDIO_SUBCLASS_CONTROL = 0x01  , ///< Audio Control
  AUDIO_SUBCLASS_STREAMING       , ///< Audio Streaming
  AUDIO_SUBCLASS_MIDI_STREAMING  , ///< MIDI Streaming
} audio_subclass_type_t;

/// Audio Protocol Codes
typedef enum
{
  AUDIO_PROTOCOL_V1                   = 0x00, ///< Version 1.0
  AUDIO_PROTOCOL_V2                   = 0x20, ///< Version 2.0
  AUDIO_PROTOCOL_V3                   = 0x30, ///< Version 3.0
} audio_protocol_type_t;

typedef enum
{
  AUDIO1_CS_REQUEST_CODE_UNDEFINED    = 0x00,
  AUDIO1_CS_REQUEST_SET_CUR           = 0x01,
  AUDIO1_CS_REQUEST_GET_CUR           = 0x81,
  AUDIO1_CS_REQUEST_SET_MIN           = 0x02,
  AUDIO1_CS_REQUEST_GET_MIN           = 0x82,
  AUDIO1_CS_REQUEST_SET_MAX           = 0x03,
  AUDIO1_CS_REQUEST_GET_MAX           = 0x83,
  AUDIO1_CS_REQUEST_SET_RES           = 0x04,
  AUDIO1_CS_REQUEST_GET_RES           = 0x84,
  AUDIO1_CS_REQUEST_SET_MEM           = 0x05,
  AUDIO1_CS_REQUEST_GET_MEM           = 0x85,
  AUDIO1_CS_REQUEST_GET_STAT          = 0xFF,

  AUDIO2_CS_REQUEST_CODE_UNDEFINED    = 0x00,
  AUDIO2_CS_REQUEST_CUR               = 0x01,
  AUDIO2_CS_REQUEST_RANGE             = 0x02,
} audio_class_specific_request_code_t;

/// Audio Function Category Codes
typedef enum
{
  AUDIO_FUNC_DESKTOP_SPEAKER    = 0x01,
  AUDIO_FUNC_HOME_THEATER       = 0x02,
  AUDIO_FUNC_MICROPHONE         = 0x03,
  AUDIO_FUNC_HEADSET            = 0x04,
  AUDIO_FUNC_TELEPHONE          = 0x05,
  AUDIO_FUNC_CONVERTER          = 0x06,
  AUDIO_FUNC_SOUND_RECODER      = 0x07,
  AUDIO_FUNC_IO_BOX             = 0x08,
  AUDIO_FUNC_MUSICAL_INSTRUMENT = 0x09,
  AUDIO_FUNC_PRO_AUDIO          = 0x0A,
  AUDIO_FUNC_AUDIO_VIDEO        = 0x0B,
  AUDIO_FUNC_CONTROL_PANEL      = 0x0C
} audio_function_t;

/// Audio Class-Specific AC Interface Descriptor Subtypes
typedef enum
{
  AUDIO_CS_INTERFACE_HEADER                = 0x01,
  AUDIO_CS_INTERFACE_INPUT_TERMINAL        = 0x02,
  AUDIO_CS_INTERFACE_OUTPUT_TERMINAL       = 0x03,
  AUDIO_CS_INTERFACE_MIXER_UNIT            = 0x04,
  AUDIO_CS_INTERFACE_SELECTOR_UNIT         = 0x05,
  AUDIO_CS_INTERFACE_FEATURE_UNIT          = 0x06,
  AUDIO_CS_INTERFACE_EFFECT_UNIT           = 0x07,
  AUDIO_CS_INTERFACE_PROCESSING_UNIT       = 0x08,
  AUDIO_CS_INTERFACE_EXTENSION_UNIT        = 0x09,
  AUDIO_CS_INTERFACE_CLOCK_SOURCE          = 0x0A,
  AUDIO_CS_INTERFACE_CLOCK_SELECTOR        = 0x0B,
  AUDIO_CS_INTERFACE_CLOCK_MULTIPLIER      = 0x0C,
  AUDIO_CS_INTERFACE_SAMPLE_RATE_CONVERTER = 0x0D,
} audio_cs_interface_subtype_t;

typedef enum
{
  AUDIO_TT_USB_UNDEFINED                  = 0x0100,
  AUDIO_TT_USB_STREAMING                  = 0x0101,
  AUDIO_TT_USB_VENDOR_SPECIFIC            = 0x01FF,

  AUDIO_TT_INPUT_UNDEFINED                = 0x0200,
  AUDIO_TT_MICROPHONE                     = 0x0201,
  AUDIO_TT_DESKTOP_MICROPHONE             = 0x0202,
  AUDIO_TT_PERSONAL_MICROPHONE            = 0x0203,
  AUDIO_TT_OMNI_DIRECTIONAL_MICROPHONE    = 0x0204,
  AUDIO_TT_MICROPHONE_ARRAY               = 0x0205,
  AUDIO_TT_PROCESSING_MICROPHONE_ARRAY    = 0x0206,

  AUDIO_TT_OUTPUT_UNDEFINED               = 0x0300,
  AUDIO_TT_SPEAKER                        = 0x0301,
  AUDIO_TT_HEADPHONES                     = 0x0302,
  AUDIO_TT_HEAD_MOUNTED_DISPLAY_AUDIO     = 0x0303,
  AUDIO_TT_DESKTOP_SPEAKER                = 0x0304,
  AUDIO_TT_ROOM_SPEAKER                   = 0x0305,
  AUDIO_TT_COMMUNICATION_SPEAKER          = 0x0306,
  AUDIO_TT_LOW_FREQUENCY_EFFECTS_SPEAKER  = 0x0307,
} audio_terminal_type_t;

typedef enum
{
  AUDIO_AS_AS_DESCRIPTOR_UNDEFINED = 0x00,
  AUDIO_AS_AS_GENERAL              = 0x01,
  AUDIO_AS_FORMAT_TYPE             = 0x02,
  AUDIO_AS_ENCODER                 = 0x03,
  AUDIO_AS_DECODER                 = 0x04,
} audio_as_interface_subtype_t;

typedef enum
{
  AUDIO_FT_FORMAT_TYPE_UNDEFINED = 0x00,
  AUDIO_FT_FORMAT_TYPE_I         = 0x01,
  AUDIO_FT_FORMAT_TYPE_II        = 0x02,
  AUDIO_FT_FORMAT_TYPE_III       = 0x03,
  AUDIO_FT_FORMAT_TYPE_IV        = 0x04,
  AUDIO_FT_EXT_FORMAT_TYPE_I     = 0x81,
  AUDIO_FT_EXT_FORMAT_TYPE_II    = 0x82,
  AUDIO_FT_EXT_FORMAT_TYPE_III   = 0x83,
} audio_format_type_t;

typedef enum
{
  AUDIO1_FUCS_FU_CONTROL_UNDEFINED        = 0x00,
  AUDIO1_FUCS_MUTE_CONTROL                = 0x01,
  AUDIO1_FUCS_VOLUME_CONTROL              = 0x02,
  AUDIO1_FUCS_BASS_CONTROL                = 0x03,
  AUDIO1_FUCS_MID_CONTROL                 = 0x04,
  AUDIO1_FUCS_TREBLE_CONTROL              = 0x05,
  AUDIO1_FUCS_GRAPHIC_EQUALIZER_CONTROL   = 0x06,
  AUDIO1_FUCS_AUTOMATIC_GAIN_CONTROL      = 0x07,
  AUDIO1_FUCS_DELAY_CONTROL               = 0x08,
  AUDIO1_FUCS_BASS_BOOST_CONTROL          = 0x09,
  AUDIO1_FUCS_LOUDNESS_CONTROL            = 0x0A,
} audio1_feature_unit_control_selector_t;

typedef enum
{
  AUDIO2_CSCS_CONTROL_UNDEFINED           = 0x00,
  AUDIO2_CSCS_SAM_FREQ_CONTROL            = 0x01,
  AUDIO2_CSCS_CLOCK_VALID_CONTROL         = 0x02,
} audio2_clock_source_unit_control_selector_t;

#define AUDIO_DATA_FORMAT_TYPE_I_PCM        0x00000001u
#define AUDIO_DATA_FORMAT_TYPE_I_PCM8       0x00000002u
#define AUDIO_DATA_FORMAT_TYPE_I_IEEE_FLOAT 0x00000004u
#define AUDIO_DATA_FORMAT_TYPE_I_ALAW       0x00000008u
#define AUDIO_DATA_FORMAT_TYPE_I_MULAW      0x00000010u
#define AUDIO_DATA_FORMAT_TYPE_I_RAW_DATA   0x80000000u

/** @} */

#ifdef __cplusplus
 }
#endif

#endif

/** @} */
