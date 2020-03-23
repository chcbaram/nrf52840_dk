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

#include "tusb_option.h"

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_CDC)

#include "cdc_device.h"
#include "device/usbd_pvt.h"


volatile bool usb_rx_full = false;

volatile uint8_t  txd_buffer[CFG_TUD_CDC_TX_BUFSIZE];
volatile uint32_t txd_length    = CFG_TUD_CDC_TX_BUFSIZE;
volatile uint32_t txd_BufPtrIn  = 0;
volatile uint32_t txd_BufPtrOut = 0;


volatile uint8_t  rxd_buffer[CFG_TUD_CDC_RX_BUFSIZE];
volatile uint32_t rxd_length    = CFG_TUD_CDC_RX_BUFSIZE;
volatile uint32_t rxd_BufPtrIn  = 0;
volatile uint32_t rxd_BufPtrOut = 0;


uint32_t CDC_Itf_GetBaud(void);
uint32_t CDC_Itf_TxAvailable( void );
uint32_t CDC_Itf_RxAvailable( void );
int32_t  CDC_Itf_Write( uint8_t *p_buf, uint32_t length );
uint8_t  CDC_Itf_Getch( void );
uint8_t  CDC_Itf_Read( void );
uint32_t CDC_Itf_TxBufLengh( void );
uint8_t  CDC_Itf_TxRead( void );
bool CDC_Itf_IsConnected(void);
void CDC_Itf_Flush( void );
void CDC_Itf_TxISR(void);



//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
  uint8_t itf_num;
  uint8_t ep_notif;
  uint8_t ep_in;
  uint8_t ep_out;

  // Bit 0:  DTR (Data Terminal Ready), Bit 1: RTS (Request to Send)
  uint8_t line_state;

  /*------------- From this point, data is not cleared by bus reset -------------*/
  char    wanted_char;
  cdc_line_coding_t line_coding;

  // FIFO
  tu_fifo_t rx_ff;
  tu_fifo_t tx_ff;

  uint8_t rx_ff_buf[CFG_TUD_CDC_RX_BUFSIZE];
  uint8_t tx_ff_buf[CFG_TUD_CDC_TX_BUFSIZE];

#if CFG_FIFO_MUTEX
  osal_mutex_def_t rx_ff_mutex;
  osal_mutex_def_t tx_ff_mutex;
#endif

  // Endpoint Transfer buffer
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_CDC_EPSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_CDC_EPSIZE];

}cdcd_interface_t;

#define ITF_MEM_RESET_SIZE   offsetof(cdcd_interface_t, wanted_char)

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
CFG_TUSB_MEM_SECTION static cdcd_interface_t _cdcd_itf[CFG_TUD_CDC];

static void _prep_out_transaction (uint8_t itf)
{
  cdcd_interface_t* p_cdc = &_cdcd_itf[itf];

  // skip if previous transfer not complete
  if ( usbd_edpt_busy(TUD_OPT_RHPORT, p_cdc->ep_out) ) return;

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  uint16_t max_read = tu_fifo_remaining(&p_cdc->rx_ff);
  if ( max_read >= CFG_TUD_CDC_EPSIZE )
  {
    usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_out, p_cdc->epout_buf, CFG_TUD_CDC_EPSIZE);
  }
  else
  {
    usb_rx_full = true;
  }
}
#if 1
static void _prep_out_transaction_sof (uint8_t itf)
{
  cdcd_interface_t* p_cdc = &_cdcd_itf[itf];

  // skip if previous transfer not complete
  if ( usbd_edpt_busy(TUD_OPT_RHPORT, p_cdc->ep_out) ) return;

  if (usb_rx_full == true)
  {
    // Prepare for incoming data but only allow what we can store in the ring buffer.
    uint16_t max_read = tu_fifo_remaining(&p_cdc->rx_ff);
    if ( max_read >= CFG_TUD_CDC_EPSIZE )
    {
      usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_out, p_cdc->epout_buf, CFG_TUD_CDC_EPSIZE);
      usb_rx_full = false;
    }
  }
}
#endif

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_cdc_n_connected(uint8_t itf)
{
  // DTR (bit 0) active  is considered as connected
  return tud_ready() && tu_bit_test(_cdcd_itf[itf].line_state, 0);
}

uint8_t tud_cdc_n_get_line_state (uint8_t itf)
{
  return _cdcd_itf[itf].line_state;
}

void tud_cdc_n_get_line_coding (uint8_t itf, cdc_line_coding_t* coding)
{
  (*coding) = _cdcd_itf[itf].line_coding;
}

void tud_cdc_n_set_wanted_char (uint8_t itf, char wanted)
{
  _cdcd_itf[itf].wanted_char = wanted;
}


//--------------------------------------------------------------------+
// READ API
//--------------------------------------------------------------------+
uint32_t tud_cdc_n_available(uint8_t itf)
{
  return tu_fifo_count(&_cdcd_itf[itf].rx_ff);
}

uint32_t tud_cdc_n_read(uint8_t itf, void* buffer, uint32_t bufsize)
{
  uint32_t num_read = tu_fifo_read_n(&_cdcd_itf[itf].rx_ff, buffer, bufsize);
  //_prep_out_transaction(itf);
  return num_read;
}

bool tud_cdc_n_peek(uint8_t itf, int pos, uint8_t* chr)
{
  return tu_fifo_peek_at(&_cdcd_itf[itf].rx_ff, pos, chr);
}

void tud_cdc_n_read_flush (uint8_t itf)
{
  tu_fifo_clear(&_cdcd_itf[itf].rx_ff);
  _prep_out_transaction(itf);
}

//--------------------------------------------------------------------+
// WRITE API
//--------------------------------------------------------------------+
uint32_t tud_cdc_n_write(uint8_t itf, void const* buffer, uint32_t bufsize)
{
  uint16_t ret = tu_fifo_write_n(&_cdcd_itf[itf].tx_ff, buffer, bufsize);

#if 0 // TODO issue with circuitpython's REPL
  // flush if queue more than endpoint size
  if ( tu_fifo_count(&_cdcd_itf[itf].tx_ff) >= CFG_TUD_CDC_EPSIZE )
  {
    tud_cdc_n_write_flush(itf);
  }
#endif

  return ret;
}

bool tud_cdc_n_write_flush (uint8_t itf)
{
  cdcd_interface_t* p_cdc = &_cdcd_itf[itf];
  TU_VERIFY( !usbd_edpt_busy(TUD_OPT_RHPORT, p_cdc->ep_in) ); // skip if previous transfer not complete

  uint16_t count = tu_fifo_read_n(&_cdcd_itf[itf].tx_ff, p_cdc->epin_buf, CFG_TUD_CDC_EPSIZE);
  if ( count )
  {
    TU_VERIFY( tud_cdc_n_connected(itf) ); // fifo is empty if not connected
    TU_ASSERT( usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_in, p_cdc->epin_buf, count) );
  }

  return true;
}

uint32_t tud_cdc_n_write_available (uint8_t itf)
{
  return tu_fifo_remaining(&_cdcd_itf[itf].tx_ff);
}


//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void cdcd_init(void)
{
  tu_memclr(_cdcd_itf, sizeof(_cdcd_itf));

  for(uint8_t i=0; i<CFG_TUD_CDC; i++)
  {
    cdcd_interface_t* p_cdc = &_cdcd_itf[i];

    p_cdc->wanted_char = -1;

    // default line coding is : stop bit = 1, parity = none, data bits = 8
    p_cdc->line_coding.bit_rate = 115200;
    p_cdc->line_coding.stop_bits = 0;
    p_cdc->line_coding.parity    = 0;
    p_cdc->line_coding.data_bits = 8;

    // config fifo
    tu_fifo_config(&p_cdc->rx_ff, p_cdc->rx_ff_buf, CFG_TUD_CDC_RX_BUFSIZE, 1, false);
    tu_fifo_config(&p_cdc->tx_ff, p_cdc->tx_ff_buf, CFG_TUD_CDC_TX_BUFSIZE, 1, false);

#if CFG_FIFO_MUTEX
    tu_fifo_config_mutex(&p_cdc->rx_ff, osal_mutex_create(&p_cdc->rx_ff_mutex));
    tu_fifo_config_mutex(&p_cdc->tx_ff, osal_mutex_create(&p_cdc->tx_ff_mutex));
#endif
  }
}

void cdcd_reset(uint8_t rhport)
{
  (void) rhport;

  for(uint8_t i=0; i<CFG_TUD_CDC; i++)
  {
    tu_memclr(&_cdcd_itf[i], ITF_MEM_RESET_SIZE);
    tu_fifo_clear(&_cdcd_itf[i].rx_ff);
    tu_fifo_clear(&_cdcd_itf[i].tx_ff);
  }
}

bool cdcd_open(uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t *p_length)
{
  // Only support ACM subclass
  TU_ASSERT ( CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL == itf_desc->bInterfaceSubClass);

  // Only support AT commands, no protocol and vendor specific commands.
  TU_ASSERT(tu_within(CDC_COMM_PROTOCOL_NONE, itf_desc->bInterfaceProtocol, CDC_COMM_PROTOCOL_ATCOMMAND_CDMA) ||
            itf_desc->bInterfaceProtocol == 0xff);

  // Find available interface
  cdcd_interface_t * p_cdc = NULL;
  uint8_t cdc_id;
  for(cdc_id=0; cdc_id<CFG_TUD_CDC; cdc_id++)
  {
    if ( _cdcd_itf[cdc_id].ep_in == 0 )
    {
      p_cdc = &_cdcd_itf[cdc_id];
      break;
    }
  }
  TU_ASSERT(p_cdc);

  //------------- Control Interface -------------//
  p_cdc->itf_num = itf_desc->bInterfaceNumber;

  uint8_t const * p_desc = tu_desc_next( itf_desc );
  (*p_length) = sizeof(tusb_desc_interface_t);

  // Communication Functional Descriptors
  while ( TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc) )
  {
    (*p_length) += tu_desc_len(p_desc);
    p_desc = tu_desc_next(p_desc);
  }

  if ( TUSB_DESC_ENDPOINT == tu_desc_type(p_desc) )
  {
    // notification endpoint if any
    TU_ASSERT( dcd_edpt_open(rhport, (tusb_desc_endpoint_t const *) p_desc) );

    p_cdc->ep_notif = ((tusb_desc_endpoint_t const *) p_desc)->bEndpointAddress;

    (*p_length) += p_desc[DESC_OFFSET_LEN];
    p_desc = tu_desc_next(p_desc);
  }

  //------------- Data Interface (if any) -------------//
  if ( (TUSB_DESC_INTERFACE == p_desc[DESC_OFFSET_TYPE]) &&
       (TUSB_CLASS_CDC_DATA == ((tusb_desc_interface_t const *) p_desc)->bInterfaceClass) )
  {
    // next to endpoint descriptor
    p_desc = tu_desc_next(p_desc);

    // Open endpoint pair
    TU_ASSERT( usbd_open_edpt_pair(rhport, p_desc, 2, TUSB_XFER_BULK, &p_cdc->ep_out, &p_cdc->ep_in) );

    (*p_length) += sizeof(tusb_desc_interface_t) + 2*sizeof(tusb_desc_endpoint_t);
  }

  // Prepare for incoming data
  _prep_out_transaction(cdc_id);

  return true;
}

// Invoked when class request DATA stage is finished.
// return false to stall control endpoint (e.g Host send non-sense DATA)
bool cdcd_control_complete(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;

  //------------- Class Specific Request -------------//
  TU_VERIFY (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);

  uint8_t itf = 0;
  cdcd_interface_t* p_cdc = _cdcd_itf;

  // Identify which interface to use
  for ( ; ; itf++, p_cdc++)
  {
    if (itf >= TU_ARRAY_SIZE(_cdcd_itf)) return false;

    if ( p_cdc->itf_num == request->wIndex ) break;
  }

  // Invoke callback
  if ( CDC_REQUEST_SET_LINE_CODING == request->bRequest )
  {
    if ( tud_cdc_line_coding_cb ) tud_cdc_line_coding_cb(itf, &p_cdc->line_coding);
  }

  return true;
}

// Handle class control request
// return false to stall control endpoint (e.g unsupported request)
bool cdcd_control_request(uint8_t rhport, tusb_control_request_t const * request)
{
  // Handle class request only
  TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);

  uint8_t itf = 0;
  cdcd_interface_t* p_cdc = _cdcd_itf;

  // Identify which interface to use
  for ( ; ; itf++, p_cdc++)
  {
    if (itf >= TU_ARRAY_SIZE(_cdcd_itf)) return false;

    if ( p_cdc->itf_num == request->wIndex ) break;
  }

  switch ( request->bRequest )
  {
    case CDC_REQUEST_SET_LINE_CODING:
      TU_LOG2("  Set Line Coding\r\n");
      tud_control_xfer(rhport, request, &p_cdc->line_coding, sizeof(cdc_line_coding_t));
    break;

    case CDC_REQUEST_GET_LINE_CODING:
      TU_LOG2("  Get Line Coding\r\n");
      tud_control_xfer(rhport, request, &p_cdc->line_coding, sizeof(cdc_line_coding_t));
    break;

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
    {
      // CDC PSTN v1.2 section 6.3.12
      // Bit 0: Indicates if DTE is present or not.
      //        This signal corresponds to V.24 signal 108/2 and RS-232 signal DTR (Data Terminal Ready)
      // Bit 1: Carrier control for half-duplex modems.
      //        This signal corresponds to V.24 signal 105 and RS-232 signal RTS (Request to Send)
      bool const dtr = tu_bit_test(request->wValue, 0);
      bool const rts = tu_bit_test(request->wValue, 1);

      p_cdc->line_state = (uint8_t) request->wValue;

      TU_LOG2("  Set Control Line State: DTR = %d, RTS = %d\r\n", dtr, rts);

      tud_control_status(rhport, request);

      // Invoke callback
      if ( tud_cdc_line_state_cb) tud_cdc_line_state_cb(itf, dtr, rts);
    }
    break;

    default: return false; // stall unsupported request
  }

  return true;
}

bool cdcd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void) rhport;
  (void) result;

  uint8_t itf = 0;
  cdcd_interface_t* p_cdc = _cdcd_itf;

  // Identify which interface to use
  for ( ; ; itf++, p_cdc++)
  {
    if (itf >= TU_ARRAY_SIZE(_cdcd_itf)) return false;

    if ( ( ep_addr == p_cdc->ep_out ) || ( ep_addr == p_cdc->ep_in ) ) break;
  }

  // Received new data
  if ( ep_addr == p_cdc->ep_out )
  {
#if 0
    for(uint32_t i=0; i<xferred_bytes; i++)
    {
      tu_fifo_write(&p_cdc->rx_ff, &p_cdc->epout_buf[i]);

      // Check for wanted char and invoke callback if needed
      if ( tud_cdc_rx_wanted_cb && ( ((signed char) p_cdc->wanted_char) != -1 ) && ( p_cdc->wanted_char == p_cdc->epout_buf[i] ) )
      {
        tud_cdc_rx_wanted_cb(itf, p_cdc->wanted_char);
      }
    }

    // invoke receive callback (if there is still data)
    if (tud_cdc_rx_cb && tu_fifo_count(&p_cdc->rx_ff) ) tud_cdc_rx_cb(itf);

    // prepare for OUT transaction
    _prep_out_transaction(itf);
#else
    for(int i=0; i<xferred_bytes; i++ )
    {
      rxd_buffer[rxd_BufPtrIn] = p_cdc->epout_buf[i];

      rxd_BufPtrIn++;

      /* To avoid buffer overflow */
      if(rxd_BufPtrIn == rxd_length)
      {
        rxd_BufPtrIn = 0;
      }
    }

    uint32_t rx_buf_len;

    rx_buf_len = rxd_length - CDC_Itf_RxAvailable() - 1;


    if (rx_buf_len >= CFG_TUD_CDC_EPSIZE)
    {
      /* Schedule buffer for next receive event */
      usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_out, p_cdc->epout_buf, CFG_TUD_CDC_EPSIZE);
    }
    else
    {
      usb_rx_full = true;
    }
#endif
  }

  // Data sent to host, we could continue to fetch data tx fifo to send.
  // But it will cause incorrect baudrate set in line coding.
  // Though maybe the baudrate is not really important !!!
  if ( ep_addr == p_cdc->ep_in )
  {
    //tud_cdc_write_flush();
    return true;
  }

  // nothing to do with notif endpoint for now

  return true;
}


void cdcd_sof(uint8_t rhport)
{
  cdcd_interface_t* p_cdc = _cdcd_itf;

  CDC_Itf_TxISR();
  //_prep_out_transaction_sof(rhport);

  uint32_t rx_buf_length;


  rx_buf_length = rxd_length - CDC_Itf_RxAvailable() - 1;

  // 수신버퍼가 USB 전송 패킷 이상 남았을때만 수신하도록 함.
  if (usb_rx_full == true)
  {
    if (rx_buf_length >= CFG_TUD_CDC_EPSIZE)
    {
      usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_out, p_cdc->epout_buf, CFG_TUD_CDC_EPSIZE);
      usb_rx_full = false;
    }
  }

}







void CDC_Itf_Init(void)
{
  rxd_length      = 0;
  rxd_BufPtrIn    = 0;
  rxd_BufPtrOut   = 0;

  txd_length      = 0;
  txd_BufPtrIn    = 0;
  txd_BufPtrOut   = 0;
}



uint32_t CDC_Itf_TxAvailable( void )
{
  uint32_t length = 0;


  length = (CFG_TUD_CDC_TX_BUFSIZE + txd_BufPtrIn - txd_BufPtrOut) % txd_length;
  length = CFG_TUD_CDC_TX_BUFSIZE - length - 1;

  return length;
}


uint32_t CDC_Itf_RxAvailable( void )
{
  uint32_t length = 0;


  if( rxd_BufPtrIn != rxd_BufPtrOut )
  {
    length = (rxd_length + rxd_BufPtrIn - rxd_BufPtrOut) % rxd_length;
  }

  return length;
}

uint8_t CDC_Itf_TxRead( void )
{
  uint8_t ch = 0;
  uint32_t buffptr;


  buffptr = txd_BufPtrOut;

  ch = txd_buffer[buffptr];


  txd_BufPtrOut += 1;
  if (txd_BufPtrOut >= txd_length)
  {
    txd_BufPtrOut = 0;
  }

  return ch;
}

int32_t  CDC_Itf_Write( uint8_t *p_buf, uint32_t length )
{
  uint32_t i;
  uint32_t ptr_index;

  if (tud_cdc_n_connected(0) != true)
  {
    return -1;
  }

  if (length >= CDC_Itf_TxAvailable())
  {
    return 0;
  }

  ptr_index = txd_BufPtrIn;


  for (i=0; i<length; i++)
  {

    txd_buffer[ptr_index] = p_buf[i];

    ptr_index++;

    /* To avoid buffer overflow */
    if(ptr_index == txd_length)
    {
      ptr_index = 0;
    }
  }
  txd_BufPtrIn = ptr_index;



  return length;
}

uint32_t CDC_Itf_TxBufLengh( void )
{
  uint32_t length = 0;


  length = (txd_length + txd_BufPtrIn - txd_BufPtrOut) % txd_length;

  return length;
}


uint8_t  CDC_Itf_Getch( void )
{
  while(1)
  {
    if( CDC_Itf_RxAvailable() ) break;
  }

  return CDC_Itf_Read();
}

uint8_t  CDC_Itf_Read( void )
{
  uint8_t ch = 0;
  uint32_t buffptr;


  if( rxd_BufPtrIn == rxd_BufPtrOut ) return 0;


  buffptr = rxd_BufPtrOut;

  ch = rxd_buffer[buffptr];

  rxd_BufPtrOut += 1;
  if (rxd_BufPtrOut >= rxd_length)
  {
    rxd_BufPtrOut = 0;
  }


  return ch;
}

void CDC_Itf_TxISR(void)
{
  uint32_t buffsize;
  cdcd_interface_t* p_cdc = &_cdcd_itf[0];


  if (usbd_edpt_busy(TUD_OPT_RHPORT, p_cdc->ep_in) == true)
  {
    return;
  }


  buffsize = CDC_Itf_TxBufLengh();

  if (buffsize == 0) return;
  if (buffsize > CFG_TUD_CDC_EPSIZE) buffsize = CFG_TUD_CDC_EPSIZE;

  // TODO: 보낼데이터가 64의 배수이면 제로패킷을 보내야 해서, 64의 배수가 되지 않도록 임식 변경
  if (buffsize%64 == 0)
  {
    buffsize -= 1;
  }

  for (int i=0; i<buffsize; i++)
  {
    p_cdc->epin_buf[i] = CDC_Itf_TxRead();
  }


  usbd_edpt_xfer(TUD_OPT_RHPORT, p_cdc->ep_in, p_cdc->epin_buf, buffsize);
}


#endif
