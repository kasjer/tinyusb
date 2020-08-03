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

#include <bsp/stm32f4xx_hal_conf.h>
#include <bsp/bsp.h>
#include "tusb_option.h"

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_AUDIO)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/audio/audio.h"
#include "class/audio/audio_device.h"
#include "device/usbd_pvt.h"

#define CFG_AUDIO_MAX_ITF_COUNT       2
#define CFG_AUDIO_MAX_ALT_ITF_COUNT   2

#define CFG_TUD_AUDIO_FIFO_SIZE (512 + 192)
#define CFG_TUD_AUDIO_TX_BUFSIZE 2000

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

typedef struct
{
  uint8_t itf_num;
  uint8_t itf_alt;
  uint8_t alt_mask;
  tusb_desc_endpoint_t ep[CFG_AUDIO_MAX_ALT_ITF_COUNT];

  /*------------- From this point, data is not cleared by bus reset -------------*/
  // FIFO
  tu_fifo_t ff;
  uint8_t ff_buf[CFG_TUD_AUDIO_FIFO_SIZE];

  #if CFG_FIFO_MUTEX
  osal_mutex_def_t ff_mutex;
  #endif

  // Endpoint Transfer buffer
  CFG_TUSB_MEM_ALIGN uint8_t ep_buf[CFG_TUD_AUDIO_EPSIZE];
} audio_interface_t;

typedef struct
{
  // AC Interface number
  uint8_t ac_itf_num;
  // AS Interfaces
  audio_interface_t itf[CFG_AUDIO_MAX_ITF_COUNT];
} audio_device_t;

#define ITF_MEM_RESET_SIZE   offsetof(audio_interface_t, ff)

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
CFG_TUSB_MEM_SECTION audio_device_t _audio;

//--------------------------------------------------------------------+
// READ API
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// WRITE API
//--------------------------------------------------------------------+



//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void audiod_init(void)
{
  tu_memclr(&_audio, sizeof(_audio));

  for (int i = 0; i < CFG_AUDIO_MAX_ITF_COUNT; i++)
  {
    audio_interface_t *itf = &_audio.itf[i];

    itf->itf_alt = 0xFF;
    tu_fifo_config(&itf->ff, itf->ff_buf, TU_ARRAY_SIZE(itf->ff_buf), 1, true);

#if CFG_FIFO_MUTEX
    tu_fifo_config_mutex(&itf->ff, osal_mutex_create(&itf->ff_mutex));
#endif
  }
}

void audiod_reset(uint8_t rhport)
{
  (void) rhport;

  for (int i = 0; i < CFG_AUDIO_MAX_ITF_COUNT; i++)
  {
    audio_interface_t *itf = &_audio.itf[i];
    tu_memclr(itf, ITF_MEM_RESET_SIZE);
    itf->itf_alt = 0xFF;
    tu_fifo_clear(&itf->ff);
  }
}

static bool is_audio_streaming_itf(uint8_t const *p_desc)
{
  TU_VERIFY(TUSB_DESC_INTERFACE == tu_desc_type(p_desc));
  tusb_desc_interface_t const *desc_audio = (tusb_desc_interface_t const *)p_desc;

  TU_VERIFY(TUSB_CLASS_AUDIO         == desc_audio->bInterfaceClass    &&
            AUDIO_SUBCLASS_STREAMING == desc_audio->bInterfaceSubClass &&
            AUDIO_PROTOCOL_V2        == desc_audio->bInterfaceProtocol);
  return true;
}

uint16_t audiod_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len)
{
  // 1st Interface is Audio Control v12
  TU_VERIFY(TUSB_CLASS_AUDIO       == desc_itf->bInterfaceClass    &&
            AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass &&
            AUDIO_PROTOCOL_V2      == desc_itf->bInterfaceProtocol, 0);

  uint16_t drv_len = tu_desc_len(desc_itf);
  uint8_t const * p_desc = tu_desc_next(desc_itf);
  _audio.ac_itf_num = desc_itf->bInterfaceNumber;

  // Skip Class Specific descriptors
  while (TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc) && drv_len <= max_len)
  {
    drv_len += tu_desc_len(p_desc);
    p_desc   = tu_desc_next(p_desc);
  }

  uint8_t as_itf_ix = 0;
  uint8_t alt_settings = 0;

  // 2nd Interface is AUDIO Streaming
  while (drv_len < max_len && is_audio_streaming_itf(p_desc))
  {
    desc_itf = (tusb_desc_interface_t const *) p_desc;
    as_itf_ix = desc_itf->bInterfaceNumber - _audio.ac_itf_num - 1;
    TU_VERIFY(as_itf_ix < CFG_AUDIO_MAX_ITF_COUNT, 0);
    alt_settings = desc_itf->bAlternateSetting;
    TU_ASSERT(alt_settings < CFG_AUDIO_MAX_ALT_ITF_COUNT);

    // Alternate settings not seen yet
    TU_ASSERT((_audio.itf[as_itf_ix].alt_mask & (1u << alt_settings)) == 0);
    _audio.itf[as_itf_ix].alt_mask |= (1u << alt_settings);
    _audio.itf[as_itf_ix].itf_num = desc_itf->bInterfaceNumber;

    // next descriptor
    drv_len += tu_desc_len(p_desc);
    p_desc = tu_desc_next(p_desc);

    tu_memclr(&_audio.itf[as_itf_ix].ep[alt_settings], sizeof(tusb_desc_endpoint_t));

    while (drv_len < max_len)
    {
      if (tu_desc_type(p_desc) == TUSB_DESC_CS_INTERFACE ||
          tu_desc_type(p_desc) == TUSB_DESC_CS_ENDPOINT)
      {
        // TODO: skip for now
      }
      else if (tu_desc_type(p_desc) == TUSB_DESC_ENDPOINT)
      {
        // Open endpoint in set interface, store descriptor
        _audio.itf[as_itf_ix].ep[alt_settings] = *((tusb_desc_endpoint_t const *)p_desc);
      }
      else
      {
        break;
      }
      drv_len += tu_desc_len(p_desc);
      p_desc = tu_desc_next(p_desc);
    }
  }

  return drv_len;
}

bool audiod_control_complete(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;
  (void) p_request;
  return true;
}

static audio_interface_t *get_as_itf(uint8_t itf_num)
{
  audio_interface_t *itf = NULL;

  for (uint8_t itf_ix = 0; itf == NULL && itf_ix < CFG_AUDIO_MAX_ITF_COUNT; ++itf_ix)
  {
    if (_audio.itf[itf_ix].itf_num == itf_num) itf = &_audio.itf[itf_ix];
  }
  return itf;
}

static uint8_t audio_ep_addr(audio_interface_t const *itf)
{
  return itf->ep[itf->itf_alt].bEndpointAddress;
}

uint32_t tud_audio_itf_write_flush(uint8_t as_itf_num)
{
  audio_interface_t *itf = &_audio.itf[as_itf_num];

  TU_VERIFY(itf->itf_alt != 0xFF, 0);

  uint8_t ep_addr = audio_ep_addr(itf);

  if (!usbd_edpt_busy(0, ep_addr))
  {
    tusb_desc_endpoint_t const *ep = &itf->ep[itf->itf_alt];
    uint16_t count = tu_fifo_read_n(&itf->ff, itf->ep_buf, ep->wMaxPacketSize.size);
    if (count)
    {
      TU_ASSERT(usbd_edpt_xfer(TUD_OPT_RHPORT, ep->bEndpointAddress, itf->ep_buf, count), 0);
    }
  }
  return 0;
}

uint32_t tud_audio_itf_write(uint8_t as_itf_num, uint8_t const *buffer, uint32_t buf_size)
{
  audio_interface_t *itf = &_audio.itf[as_itf_num];

  TU_VERIFY(itf->itf_alt != 0xFF, 0);

  uint16_t ret = tu_fifo_write_n(&itf->ff, buffer, buf_size);

  if (tu_fifo_count(&itf->ff) >= itf->ep[itf->itf_alt].wMaxPacketSize.size)
  {
    tud_audio_itf_write_flush(as_itf_num);
  }

  return ret;
}

static audio_interface_t *get_as_itf_from_ep(uint8_t ep_addr)
{
  audio_interface_t *itf = &_audio.itf[0];
  audio_interface_t *itf_end = &_audio.itf[CFG_AUDIO_MAX_ITF_COUNT];

  for (; itf != itf_end; ++itf)
  {
    if (itf->ep[itf->itf_alt].bEndpointAddress == ep_addr)
    {
      return itf;
    }
  }
  return NULL;
}

uint8_t response_buf[20];
uint8_t response_buf_ix;

static void reponse_buf_put_v8(uint8_t v8)
{
  response_buf[response_buf_ix++] = v8;
}

static void reponse_buf_put_v16(uint16_t v16)
{
  reponse_buf_put_v8(v16);
  reponse_buf_put_v8(v16 >> 8u);
}

static void reponse_buf_put_v32(uint16_t v32)
{
  reponse_buf_put_v16(v32);
  reponse_buf_put_v16(v32 >> 16u);
}

static bool handle_ac_interface_request(uint8_t rhport, tusb_control_request_t const * request)
{
  uint8_t const id = (request->wIndex >> 8u);
//  find_audio_unit(id);
  if (id == 4)
  {
    if (request->bRequest == 2)
    {
      response_buf_ix = 0;
      reponse_buf_put_v16(1);
      reponse_buf_put_v32(48000);
      reponse_buf_put_v32(48000);
      reponse_buf_put_v32(0);
      return tud_control_xfer(rhport, request, response_buf, response_buf_ix);
    }
    else if (request->bRequest == 1)
    {
      response_buf_ix = 0;
      reponse_buf_put_v32(48000);
      return tud_control_xfer(rhport, request, response_buf, response_buf_ix);
    }
  }
  return false;
}

static bool handle_as_interface_request(uint8_t rhport, tusb_control_request_t const * request)
{
  return false;
}

bool audiod_control_request(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;
  audio_interface_t *itf = NULL;

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_STANDARD:
      switch (request->bRequest)
      {
        case TUSB_REQ_GET_INTERFACE:
        {
          itf = get_as_itf((uint8_t)request->wIndex);
          TU_VERIFY(itf != NULL);

          tud_control_xfer(rhport, request, &itf->itf_alt, 1);
          break;
        }

        case TUSB_REQ_SET_INTERFACE:
        {
          uint8_t const req_alt = (uint8_t)request->wValue;

          itf = get_as_itf((uint8_t)request->wIndex);
          TU_VERIFY(itf != NULL);
          TU_VERIFY((itf->alt_mask & (1u << req_alt)) != 0);

          if (itf->ep[req_alt].bEndpointAddress != 0)
          {
            TU_ASSERT(usbd_edpt_open(rhport, &itf->ep[req_alt]));
            if (tu_edpt_dir(itf->ep[req_alt].bEndpointAddress) == TUSB_DIR_OUT)
            {
              // Prepare for incoming data
              if (!usbd_edpt_xfer(rhport, itf->ep[req_alt].bEndpointAddress, itf->ep_buf, itf->ep[req_alt].wMaxPacketSize.size))
              {
                TU_LOG1_FAILED();
                TU_BREAKPOINT();
              }
            }
            itf->itf_alt = req_alt;
            tud_audio_interface_connected(itf - &_audio.itf[0], itf->ep[req_alt].bEndpointAddress, itf->ep[req_alt].wMaxPacketSize.size);
          }
          else
          {

            // TODO: close the endpoint pair
            // For now pretend that we did, this should have no harm since host won't try to
            // communicate with the endpoints again
          }

          tud_control_status(rhport, request);
          break;
        }

        default:
          // unsupported request
          return false;
      }
      break;

    case TUSB_REQ_TYPE_CLASS:
      switch (request->bmRequestType_bit.recipient)
      {
        // Interface request
        case 1:
          if ((uint8_t)request->wIndex == _audio.ac_itf_num)
          {
            return handle_ac_interface_request(rhport, request);
          }
          else
          {
            return handle_as_interface_request(rhport, request);
          }
          break;
        case 2: // ISO endpoint request
          return false;
          break;
      }
    default: return false;
  }

  return true;
}

bool audiod_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void) result;

  audio_interface_t *itf = get_as_itf_from_ep(ep_addr);

  if (itf != NULL)
  {
    int const as_itf_num = itf - &_audio.itf[0];

    if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT)
    {
      tud_audio_rx_cb(as_itf_num);
      // prepare for next data
//      TU_ASSERT(usbd_edpt_xfer(rhport, ep_addr, itf->ep_buf[0], itf->ep[itf->itf_alt].wMaxPacketSize.size));
    }
    else
    {
      tud_audio_itf_write_flush(as_itf_num);
      tud_audio_tx_cb(as_itf_num);
    }
  }

  return true;
}

#endif
