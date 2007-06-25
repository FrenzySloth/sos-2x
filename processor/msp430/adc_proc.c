/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/*
 * XXX need to implement a function to clean up ADC binding when module is removed
 */
#include <adc_proc_hw.h>
#include <adc_proc.h>
#include <msp430x16x.h>

#define LED_DEBUG
#include <led_dbg.h>

/**
 * This implements the ADC12 module found on the MSP430x13x, MSP430x15x, and 
 * MSP430x16x.
 *
 * channels can be selected by using the MUX values defined in adc_proc_hw.h
 */

// for normal operation the resolution is 12 bit
#define ADC_PROC_BITS 12

// null value for adc driver ports
#define NULL_PORT 0xff

/**
 * this driver will use the internal reference since without a regulated power
 * supply AVCC is meaningless.
 */
//#define ADC_proc_vref default_vref
#define ADC_PROC_VREF ADC_PROC_REF_AREF
// this should be defined in the plaform adc_proc_hal.h
// the divisor must result in a clock rate between 50k-200k
// with a 7.2MHz clock this yeild ~110k
#define ADC_PROC_PRESCALER ADC_PROC_CLK_64

// highest 2 bits reserved for calling process state
// to mux up to 4 sensors on a single ADC channel.
// sampling type
#define ADC_PROC_DMA_FLAG     0x80
#define ADC_PROC_PERODIC_FLAG 0x04
#define ADC_PROC_SINGLE_FLAG  0x02
#define ADC_PROC_ERROR_FLAG   0x01

/**
 * possible states for adc_proc
 */
enum {
	ADC_PROC_INIT=0,  // system uninitalized
	ADC_PROC_INIT_BUSY,  // system busy with initalization
	ADC_PROC_IDLE,    // system initalized and idle
	ADC_PROC_BUSY,
	ADC_PROC_DATA_RDY,
	ADC_PROC_ERROR,   // error state
};


//-----------------------------------------------------------------------------
// LOCAL VARIABLES
typedef struct adc_proc_state {
	func_cb_ptr cb[ADC_PROC_EXTENDED_PORTMAPSIZE];	
	uint8_t state;

	uint8_t portmap[ADC_PROC_EXTENDED_PORTMAPSIZE];
	uint8_t calling_pid[ADC_PROC_EXTENDED_PORTMAPSIZE];
	uint8_t pending_flag[ADC_PROC_EXTENDED_PORTMAPSIZE];
	uint8_t calling_flags;
	
	uint8_t reqPort;  // current port
	uint16_t portMask; // mask of mapped ports
	uint16_t reqMask;  // the mask for pending requested port
	uint16_t refVal;  // system reference value

	uint8_t sampleCnt;

	uint8_t flags;
} adc_proc_state_t;
static adc_proc_state_t s;

static int8_t adc_proc_msg_handler(void *state, Message *msg);

static sos_module_t adc_proc_module;
static const mod_header_t mod_header SOS_MODULE_HEADER =
{
  .mod_id = ADC_PROC_PID,
	.state_size = 0,
	.num_prov_func = 0,
	.num_sub_func = ADC_PROC_EXTENDED_PORTMAPSIZE,
	.module_handler= adc_proc_msg_handler,
	.funct = {
		// sensor 0
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 1
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 2
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 3
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 4
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 5
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 6
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 7
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 8
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 9
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 10
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 11
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
	},
};


static int8_t adc_proc_msg_handler(void *state, Message *msg) {
	return -EINVAL;
}


int8_t adc_proc_init() {
  HAS_CRITICAL_SECTION;
	int i;
	
	ENTER_CRITICAL_SECTION();
	s.state = ADC_PROC_INIT_BUSY;
	
  adc_proc_hardware_init();
  
	// set the defaults to single ended
	for (i = 0; i < ADC_PROC_EXTENDED_PORTMAPSIZE; i++){
		s.portmap[i] = ADC_PROC_HW_NULL_PORT;
		s.calling_pid[i] = NULL_PID;
	}
	sched_register_kernel_module(&adc_proc_module, sos_get_header_address(mod_header), &s.cb);
	
	s.portMask = 0;
	s.reqMask = 0;
	// wtf???
	s.refVal = 0x17d; // Reference value assuming 3.3 Volt power source
	s.state = ADC_PROC_IDLE;
	LEAVE_CRITICAL_SECTION();
	
	return SOS_OK;
}


int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid){
	HAS_CRITICAL_SECTION;

  if ((port > ADC_PROC_EXTENDED_PORTMAPSIZE) || (adcPort > ADC_PROC_HW_CH_MAX)) {
		return -EINVAL;
	}
	if ((s.state != ADC_PROC_IDLE) || ((s.calling_pid[port] != NULL_PID) && (s.calling_pid[port] != calling_id))) {
		return -EBUSY;
	}

	// try to register all necessary function calls
	if(ker_fntable_subscribe(ADC_PROC_PID, calling_id, cb_fid, port) < 0) {
		return -EINVAL;
	}

	ENTER_CRITICAL_SECTION();
	s.portmap[port] = adcPort;
	s.calling_pid[port] = calling_id;

	if (s.portMask == 0) {
		// if first user, clear any pending interrupts and enable ADC
		// to allow for startup delay (may need additional stabalization time)
    ADC12CTL0 = (ADC12ON+SHT0_0 + SHT1_0); // Turn on ADC12
    ADC12IFG = 0; // clear any pending interrupts
	}
  // configure ADC pin
  if (adcPort <= 7){
    // external ADC pin, enable it
    P6SEL |= (1 << adcPort); // adc function (instead of general IO)
    P6DIR &= ~(1 << adcPort); // input (instead of output
  }

	s.portMask |= (1<<port);
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}

int8_t ker_adc_proc_unbindPort(uint8_t port, sos_pid_t pid) {
	HAS_CRITICAL_SECTION;
	
  if ((port > ADC_PROC_EXTENDED_PORTMAPSIZE) || (s.calling_pid[port] != pid)) {
		return -EINVAL;
	}

	ENTER_CRITICAL_SECTION();
  if(s.portmap[port] <= 7){
    P6SEL &= ~(1 << s.portmap[port]);
    P6DIR |= (1 << s.portmap[port]);
  }

	s.portmap[port] = ADC_PROC_HW_NULL_PORT;
	s.calling_pid[port] = NULL_PID;
	s.portMask &= ~(1<<port);
	if (s.portMask == 0) {
		// if no users, clear any pending interrupts and disable everything
    ADC12CTL0 &= ~(ENC);
    ADC12IFG = 0;
	}

	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_adc_proc_getData(uint8_t port, uint8_t flags) {
  HAS_CRITICAL_SECTION;
	if ((port > ADC_PROC_EXTENDED_PORTMAPSIZE) || (!(s.portMask & (1<<port)))) {
		return -EINVAL;
	}

	ENTER_CRITICAL_SECTION();
	switch (s.state) {
		case ADC_PROC_INIT:
		case ADC_PROC_INIT_BUSY:
		case ADC_PROC_BUSY:
			s.reqMask |= (1 << port);
			s.pending_flag[port] = flags;
			return SOS_OK;
			break;
			
		case ADC_PROC_IDLE:
			s.state = ADC_PROC_BUSY;
			s.reqPort = port;
			s.sampleCnt = 1;
			s.calling_flags = flags;
      ADC12CTL0 = SHT0_8 + REFON + ADC12ON;
      ADC12CTL1 = CONSEQ_0 | SHP; // single channle, single conversion 
			ADC12MCTL0 = (SREF_1 + s.portmap[port]); //select Vcc ref + port
			//ADC12MCTL0 = (SREF_1 + INCH_10); //select Vcc ref + port
      ADC12IE = 0x001; // enable interrupt for mem location 0
      ADC12CTL0 |= ENC + ADC12SC; // start conversion
			break;

		case ADC_PROC_ERROR:
		default:
			break;
	}

  /* 

  ADC12CTL0 = SHT0_8 + REFON + ADC12ON;
  ADC12CTL1 = SHP; // enable sample timer
  ADC12MCTL0 = SREF_1 + INCH_10;
  ADC12IE = 0x001;
  ADC12CTL0 |= ENC;
  ADC12CTL0 |= ADC12SC;
  */
  //LED_DBG(LED_RED_TOGGLE);

	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_adc_proc_getPerodicData(uint8_t port, uint8_t prescaler, uint16_t count) {
 /* 
  HAS_CRITICAL_SECTION;
	if ((port > ADC_PROC_EXTENDED_PORTMAPSIZE) || (!(s.portMask & (1<<port))) || (prescaler > ADC_PROC_CLK_128) || (count == 0)) {
		return -EINVAL;
	}

  ENTER_CRITICAL_SECTION();
	switch (s.state) {
		case ADC_PROC_INIT:
		case ADC_PROC_INIT_BUSY:
		case ADC_PROC_BUSY:
			return -EBUSY
			break;
			
		case ADC_PROC_IDLE:
			s.STATE = adc_proc_busy;
			s.sampleCnt = count;
			ADMUX = (ADC_PROC_VREF | s.portmap[port]);
			if (prescaler != ADC_PROC_CLK_NULL) {
				ADCSRA &= ~(ADC_PROC_CLK_MSK); // clear current prescaler
				ADCSRA |= prescaler;
			}
			ADCSRA |= _BV(ADFR); // put into free running mode
			ADCSRA |= _BV(ADIE); // enable iterrupts
			ADCSRA |= _BV(ADSC); // start conversion
			break;

		case ADC_PROC_ERROR:
		default:
			break;
	}
	LEAVE_CRITICAL_SECTION();
*/	
	return SOS_OK;
}


int8_t ker_adc_proc_stopPerodicData(uint8_t port) {
/*
  HAS_CRITICAL_SECTION;
	if (port > ADC_PROC_EXTENDED_PORTMAPSIZE) {
		return -EINVAL;
	}
	
  ENTER_CRITICAL_SECTION();
	s.state = ADC_PROC_IDLE;
	ADCSRA &= ~_BV(ADFR);
	ADCSRA &= ~_BV(ADIE);
	LEAVE_CRITICAL_SECTION();
*/	
	return SOS_OK;
}
#include <led.h>

interrupt (ADC_VECTOR) adc_proc_interrupt() {
	uint16_t adcValue;
  uint16_t iv = ADC12IV;
  
  ADC12IFG = 0;  
  
  ADC12CTL0 &= ~(ENC); // disable converter

	if (s.state != ADC_PROC_BUSY) {
		s.state = ADC_PROC_IDLE;
		return;
	}
	s.state = ADC_PROC_DATA_RDY;

  switch(iv){
    case 2: // memory overflow
    case 4: // time overflow
      ker_panic();
  }

	adcValue = *((uint16_t*) ADC12MEM + 0);
	s.sampleCnt--;
	
	if (s.portmap[s.reqPort] == ADC_PROC_BANDGAP) {
		s.refVal = adcValue;
	}

	SOS_CALL(s.cb[s.reqPort], adc12_cb_t, s.reqPort, adcValue, s.calling_flags);

	if (!(s.sampleCnt > 0)) {
		s.state = ADC_PROC_IDLE;
    //ADC12IE = 1 << 0; // enable interrupt for mem location 0
    //ADC12CTL0 |= ENC | ADC12SC; // start conversion
	} else {
		return;
	}

	if( s.reqMask != 0 ) {
		// we have pending request
		uint8_t i;
		uint16_t m = 1;
		for( i = 0; i < ADC_PROC_EXTENDED_PORTMAPSIZE; i++, m<<=1) {
			if( m & s.reqMask ) {
				s.reqMask &= ~m;
				s.state = ADC_PROC_BUSY;
				s.reqPort = i;
				s.sampleCnt = 1;
				s.calling_flags = s.pending_flag[i];
        ADC12CTL1 = CONSEQ_0 | SHS0; // single channle, single conversion 
			  ADC12MCTL0 = (SREF_0 | s.portmap[i]); //select Vcc ref + port
        ADC12IFG = 0; // clear any pending interrupts
        ADC12IE = 1 << 0; // enable interrupt for mem location 0
        ADC12CTL0 |= ENC + ADC12SC; // start conversion
				return;
			}
		}
	}
}

