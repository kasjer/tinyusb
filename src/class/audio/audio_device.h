/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
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

#ifndef _TUSB_AUDIO_DEVICE_H_
#define _TUSB_AUDIO_DEVICE_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

#include "class/audio/audio.h"

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+
#ifndef CFG_TUD_AUDIO_EPSIZE
#define CFG_TUD_AUDIO_EPSIZE 1023
#endif

#ifdef __cplusplus
 extern "C" {
#endif

/** \addtogroup MIDI_Serial Serial
 *  @{
 *  \defgroup   MIDI_Serial_Device Device
 *  @{ */

typedef struct
{
  uint8_t bmin;
  uint8_t bmax;
  uint8_t bres;
} audio_control_range_1_t;

typedef struct
{
  uint16_t wmin;
  uint16_t wmax;
  uint16_t wres;
} audio_control_range_2_t;

typedef struct
{
  uint32_t dmin;
  uint32_t dmax;
  uint32_t dres;
} audio_control_range_3_t;

#define RANGE1(min, max, res) min, max, res
#define RANGE2(min, max, res) U16_TO_U8S_LE(min), U16_TO_U8S_LE(max), U16_TO_U8S_LE(res)
#define RANGE4(min, max, res) U32_TO_U8S_LE(min), U32_TO_U8S_LE(max), U32_TO_U8S_LE(res)
#define RANGE1_1(r1) U16_TO_U8S_LE(1), RANGE1(r1)
#define RANGE2_1(r1) U16_TO_U8S_LE(1), RANGE2(r1)
#define RANGE4_1(r1) U16_TO_U8S_LE(1), RANGE4(r1)

typedef struct
{
  uint8_t id;
  uint8_t value_size;
  uint8_t *cur;
  uint8_t *ranges;
} audio_unit_t;

typedef struct
{
  uint32_t cur;
  audio_control_range_3_t *ranges;
} audio_clock_freq_t;

typedef struct
{
  uint8_t id;
  audio_clock_freq_t freq;
} audio_clock_t;

//--------------------------------------------------------------------+
// Application API (Multiple Interfaces)
// CFG_TUD_AUDIO > 1
//--------------------------------------------------------------------+
uint32_t tud_audio_itf_write       (uint8_t as_itf_num, uint8_t const *buffer, uint32_t bufsize);
uint32_t tud_audio_itf_write_flush (uint8_t as_itf_num);

//--------------------------------------------------------------------+
// Application API (Interface0)
//--------------------------------------------------------------------+
static inline uint32_t tud_audio_write      (uint8_t const *buffer, uint32_t bufsize);

//--------------------------------------------------------------------+
// Application Callback API (weak is optional)
//--------------------------------------------------------------------+
TU_ATTR_WEAK void tud_audio_rx_cb(uint8_t as_itf_num);
TU_ATTR_WEAK void tud_audio_tx_cb(uint8_t as_itf_num);
TU_ATTR_WEAK void tud_audio_interface_connected(uint8_t as_itf_num, tusb_dir_t dir, uint16_t ep_size);
//TU_ATTR_WEAK void tud_audio_data_received_cb();

//--------------------------------------------------------------------+
// Inline Functions
//--------------------------------------------------------------------+

static inline uint32_t tud_audio_write (uint8_t const * buffer, uint32_t bufsize)
{
  return tud_audio_itf_write(0, buffer, bufsize);
}

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void     audiod_init             (void);
void     audiod_reset            (uint8_t rhport);
uint16_t audiod_open             (uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t max_len);
bool     audiod_control_request  (uint8_t rhport, tusb_control_request_t const * request);
bool     audiod_control_complete (uint8_t rhport, tusb_control_request_t const * request);
bool     audiod_xfer_cb          (uint8_t rhport, uint8_t edpt_addr, xfer_result_t result, uint32_t xferred_bytes);

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_AUDIO_DEVICE_H_ */

/** @} */
/** @} */
