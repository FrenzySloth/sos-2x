/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <module.h>
 */
#include <module.h>
#include <sys_module.h>
#include <codemem.h>
#include <loader/loader.h>


#define THIS_MODULE_ID  DFLT_APP_ID3


static int8_t module(void *state, Message *e);


static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = THIS_MODULE_ID,
  .state_size     = 0,
  .num_timers     = 0,
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .code_id        = ehtons(THIS_MODULE_ID),
  .module_handler = module,
};


static void load_script( uint8_t *script, uint8_t size );
static int8_t module(void *state, Message *msg)
{
  switch (msg->type){

  case MSG_INIT:
	{
	  // timer0 context 
	  uint8_t timer0_script_buf[227] = { 0xa5,0x1,0x4,0x2,0xdc,0x0,0x0,0x6e,0x24,0x45,0x0,0x0,0x0,0x64,0x21,0x42,0x55,0x2,0x6,0x1,0x4,0x21,0x45,0x0,0x0,0x0,0x0,0x43,0xa,0x6,0x1f,0x67,0x6,0x31,0x2b,0xda,0x1,0x0,0x67,0x31,0x6,0x2a,0x4d,0x1,0x0,0x68,0x31,0x7,0x2a,0x48,0x31,0x6,0x21,0x7,0x1d,0x1,0x4,0x21,0x7,0x1,0x4,0x21,0x42,0x12,0x6,0x1,0x4,0x21,0x42,0x12,0x20,0x6b,0x41,0xa,0x7,0x1f,0x68,0xb,0x27,0x6,0x1f,0x67,0xb,0x20,0x1,0x0,0x67,0x31,0x6,0x2a,0x94,0x1,0x0,0x68,0x1,0x0,0x61,0x1,0x1,0x69,0x1,0x1,0x8,0x2f,0x8f,0x31,0x7,0x2a,0x8f,0x35,0x34,0x31,0x6,0x21,0x7,0x1d,0x1,0x4,0x21,0x41,0x12,0x2a,0x77,0x4,0x1f,0x65,0x7,0x1f,0x68,0x4,0x33,0x32,0x2b,0x8d,0x1,0x4,0x6,0x21,0x45,0x0,0x0,0x0,0x64,0x43,0xa,0x1,0x0,0x69,0xb,0x5d,0x6,0x1f,0x67,0xb,0x50,0x45,0x0,0x0,0x0,0x0,0x65,0x1,0x0,0x69,0x1,0x0,0x67,0x31,0x8,0x2a,0xbf,0x1,0x4,0x8,0x21,0x43,0x12,0x1,0x1,0x2f,0xba,0x4,0x1,0x4,0x8,0x21,0x42,0x12,0x1d,0x65,0x6,0x1f,0x67,0x8,0x1f,0x69,0xb,0xa0,0x1,0x0,0x6,0x2e,0xc8,0x6,0x4,0x22,0x67,0x41,0x13,0x2,0x42,0x13,0x2,0x43,0x13,0x2,0x1,0x0,0x61,0x1,0x0,0x67,0x1,0x0,0x68,0x6f,0x0, };
	  load_script(timer0_script_buf, 227);
	  // reboot context 
	  uint8_t reboot_script_buf[29] = { 0xa5,0x0,0x0,0x0,0x16,0x0,0x0,0x1,0x4,0x39,0x31,0x45,0x0,0x0,0x0,0x28,0x1,0x1,0x20,0x21,0x3a,0x3b,0x1,0x64,0x49,0x1,0x80,0x16,0x0, };
	  load_script(reboot_script_buf, 29);
	  break;
	}
  case MSG_TIMER_TIMEOUT:
	{
	  break;
	}
  case MSG_FINAL:
	{
	  break;
	}
  default:
	return -EINVAL;
  }

  return SOS_OK;
}

static void load_script( uint8_t *script, uint8_t size )
{
  codemem_t cm = ker_codemem_alloc(size, CODEMEM_TYPE_EXECUTABLE);
  ker_codemem_write( cm, THIS_MODULE_ID, script, size, 0);
  ker_codemem_flush( cm, THIS_MODULE_ID);
  post_short(script[0], THIS_MODULE_ID, MSG_LOADER_DATA_AVAILABLE, script[1], cm, 0);
  return;
}

#ifndef _MODULE_
mod_header_ptr script_loader_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

