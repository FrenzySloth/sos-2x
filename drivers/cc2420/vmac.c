/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @brief    virtual hal for radio
 * @author   Hubert Wu {huberwu@cs.ucla.edu}
 * @author   Dimitrios Lymberopoulos {dimitrios.lymberopoulos@yale.edu}
 */

#include <random.h>
#include <hardware.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <malloc.h>
#include <net_stack.h>
#include <timestamp.h>
#include <systime.h>
#include <sos_info.h>
#include <message.h>
#include <string.h> // for memcpy
#include <spi_hal.h>
#include <vmac.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <sys_module.h>
#include <module.h>
#ifdef SOS_USE_PREEMPTION
#include <priority.h>
#endif

#define VMAC_SEND_STATE_IDLE         0
#define VMAC_SEND_STATE_BACKOFF      1
#define VMAC_SEND_STATE_WAIT_FOR_ACK 2
#define MSG_VMAC_TX_NEXT_MSG         MOD_MSG_START
#define MSG_VMAC_TX_ACKED            (MOD_MSG_START+1)
//#define ENA_VMAC_UART_DEBUG

static Message *vmac_msg;
//static volatile uint8_t valid_msg=0;
static volatile uint8_t vmac_send_state;

/*************************************************************************
 * define the maximum number retry times for sending a packet            *
 *************************************************************************/
#define MAX_RETRIES		5

/*************************************************************************
 * define the maximum number of messages in the MAC queue                *
 *************************************************************************/
#define MAX_MSGS_IN_QUEUE	30

/*************************************************************************
 * define the timer ID                                                   *
 *************************************************************************/
#define WAKEUP_TIMER_TID    0 //!< Wakeup Timer TID

/*************************************************************************
 * declare the timer                                                     *
 *************************************************************************/
static sos_timer_t wakeup_timer;// = TIMER_INITIALIZER(RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);

/*************************************************************************
 * declare the message handler                                           *
 *************************************************************************/
static int8_t vmac_handler(void *state, Message *e);

/*************************************************************************
 * declare backoff mechanism functions                                   *
 *************************************************************************/
static int16_t MacBackoff_congestionBackoff(int8_t retries);
static void radio_msg_send(Message *msg);
static uint8_t getMsgNumOfQueue();

/*************************************************************************
 * define the MAC module header                                          *
 *************************************************************************/
static mod_header_t mod_header SOS_MODULE_HEADER ={
mod_id : RADIO_PID,
state_size : 0,
num_timers : 0,
num_sub_func : 0,
num_prov_func : 0,
module_handler: vmac_handler,
};

/*************************************************************************
 * declare the MAC module                                                *
 *************************************************************************/
#ifndef SOS_USE_PREEMPTION
static sos_module_t vmac_module;
#endif

/*************************************************************************
 * declare the queue used for MAC                                        *
 *************************************************************************/
static mq_t vmac_pq;

/*************************************************************************
 * declare the sequence number variable                                  *
 *************************************************************************/
static uint8_t seq_count;

/*************************************************************************
 * define the retry times variable                                       *
 *************************************************************************/
static uint8_t retry_count;


/*************************************************************************
 * Define arrays for duplicate count                                     *
 *************************************************************************/
#define NUM_DUP_CHECK  3
static uint16_t dup_addr[NUM_DUP_CHECK];
static uint8_t dup_seq[NUM_DUP_CHECK];
static uint8_t oldest_dup;

/*************************************************************************
 * get sequence number                                                   *
 *************************************************************************/
static uint8_t getSeq()
{
	uint8_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = seq_count;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * increase sequence number                                              *
 *************************************************************************/
static uint8_t incSeq()
{
	uint8_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	if(seq_count==0xFF) { // do not use 0x00 as a sequence number!
		ENTER_CRITICAL_SECTION();
		seq_count=1;
		ret=1;          // 0x00 will be considered ACKed by default!
	} else {
		ret = seq_count++;
	}
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * set sequence number to 0                                              *
 *************************************************************************/
static void resetSeq()
{
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	seq_count = 1 + ker_rand()%255;
	LEAVE_CRITICAL_SECTION();
}

/*************************************************************************
 * message dispatch function for backoff mechanism for Collision         *
 *   Avoidance implementation                                            *
 *************************************************************************/
static int8_t vmac_handler(void *state, Message *e)
{
HAS_CRITICAL_SECTION;
//   MsgParam *p = (MsgParam*)(e->data);
   switch(e->type){
       case MSG_TIMER_TIMEOUT:
		{

		ENTER_CRITICAL_SECTION();
		radio_msg_send(vmac_msg);
		LEAVE_CRITICAL_SECTION();
		break;
		}
	   case MSG_VMAC_TX_NEXT_MSG:
	   {
		ENTER_CRITICAL_SECTION();
		if( vmac_msg == NULL ) {
			retry_count = 0;
			vmac_msg = mq_dequeue(&vmac_pq);
			if( vmac_msg != NULL ) {
				radio_msg_send(vmac_msg);
			}
		}
		LEAVE_CRITICAL_SECTION();	   
	     break;
	   }
	   case MSG_VMAC_TX_ACKED:
	   {
		ENTER_CRITICAL_SECTION();
		ker_timer_stop(RADIO_PID, WAKEUP_TIMER_TID);
		vmac_send_state = VMAC_SEND_STATE_IDLE;
		retry_count = 0;
		msg_send_senddone(vmac_msg, 1, RADIO_PID);  //to release the memory for this msg
		vmac_msg = NULL;
		vmac_msg = mq_dequeue(&vmac_pq);
		if( vmac_msg != NULL ) {
			radio_msg_send(vmac_msg);
		}
		LEAVE_CRITICAL_SECTION();
		break;
	   }
       default:
		break;
   }
   return SOS_OK;
}

/*************************************************************************
 * GC support                                                            *
 *************************************************************************/
void radio_gc( void )
{
	mq_gc_mark_payload( &vmac_pq, RADIO_PID );  
	if( vmac_msg != NULL && vmac_msg->data != NULL 
			&& flag_msg_release(vmac_msg->flag) ) {
		ker_gc_mark( RADIO_PID, vmac_msg->data );
	}
	malloc_gc( RADIO_PID );
}

void radio_msg_gc( void )
{
	mq_gc_mark_hdr( &vmac_pq, RADIO_PID );
}


/*************************************************************************
 * define the callback function for receiving data                       *
 *************************************************************************/
void (*_mac_recv_callback)(Message *m) = 0;
void mac_set_recv_callback(void (*func)(Message *m))
{
	_mac_recv_callback = func;
}

/*************************************************************************
 * change packet's format from SOS message to MAC                        *
 *************************************************************************/
void sosmsg_to_mac(Message *msg, VMAC_PPDU *ppdu)
{
	ppdu->len = msg->len + PRE_PAYLOAD_LEN + POST_PAYLOAD_LEN;

	ppdu->mpdu.daddr = host_to_net(msg->daddr);
	ppdu->mpdu.saddr = host_to_net(msg->saddr);
	ppdu->mpdu.did = msg->did;
	ppdu->mpdu.sid = msg->sid;
	ppdu->mpdu.type = msg->type;

	ppdu->mpdu.data = msg->data;
}

/*************************************************************************
 * change packet's format from MAC to SOS message                        *
 *************************************************************************/
void mac_to_sosmsg(VMAC_PPDU *ppdu, Message *msg)
{
	msg->len = ppdu->len - (PRE_PAYLOAD_LEN + POST_PAYLOAD_LEN);

	msg->daddr = net_to_host(ppdu->mpdu.daddr);
	msg->saddr = net_to_host(ppdu->mpdu.saddr);
	msg->did = ppdu->mpdu.did;
	msg->sid = ppdu->mpdu.sid;
	msg->type = ppdu->mpdu.type;

	//msg->data = ppdu->mpdu.data;
	if(msg->len==0){
		msg->data = NULL;
	} else {
		msg->data = ppdu->mpdu.data;
	}
	msg->flag |= SOS_MSG_RELEASE;
    // RSSI is the most significant byte of the post payload
    // But, since the word is little endian, its the lower byte.
    msg->rssi = (ppdu->mpdu.fcs & 0x00ff);
}

/*************************************************************************
 * change packet's format from MAC to vhal                               *
 *************************************************************************/
void mac_to_vhal(VMAC_PPDU *ppdu, vhal_data *vd)
{
	vd->pre_payload_len = PRE_PAYLOAD_LEN;
	vd->pre_payload = (uint8_t*)&ppdu->mpdu.fcf;

	vd->post_payload_len = POST_PAYLOAD_LEN;
	vd->post_payload = (uint8_t*)&ppdu->mpdu.fcs;

	vd->payload_len = ppdu->len - vd->pre_payload_len - vd->post_payload_len;
	vd->payload = ppdu->mpdu.data;
}

/*************************************************************************
 * change packet's format from vhal to MAC                               *
 *************************************************************************/
void vhal_to_mac(vhal_data *vd, VMAC_PPDU *ppdu)
{
	ppdu->len = vd->payload_len + vd->pre_payload_len + vd->post_payload_len;
	memcpy((uint8_t*)&ppdu->mpdu.fcf, vd->pre_payload, PRE_PAYLOAD_LEN);

	ppdu->mpdu.data = vd->payload;
	memcpy((uint8_t*)&ppdu->mpdu.fcs, vd->post_payload, vd->post_payload_len);
}

/*************************************************************************
 * send the message by radio                                             *
 *************************************************************************/
static void radio_msg_send(Message *msg)
{
	int16_t timestamp;
	//construct the packet
	VMAC_PPDU ppdu;
	vhal_data vd;

	if( Radio_Check_CCA() ) {
		if( retry_count == 0 ) {
			incSeq();
		}
		if(msg->type == MSG_TIMESTAMP){
			uint32_t timestp = ker_systime32();
			memcpy(msg->data, (uint8_t*)(&timestp),sizeof(uint32_t));
		}
		sosmsg_to_mac(msg, &ppdu);

        if(msg->daddr==BCAST_ADDRESS || msg->type == MSG_TIMESTAMP) {
			ppdu.mpdu.fcf = BASIC_RF_FCF_NOACK;     //Broadcast: No Ack
		} else {
			//ppdu.mpdu.fcf = BASIC_RF_FCF_NOACK;     //Broadcast: No Ack
			ppdu.mpdu.fcf = BASIC_RF_FCF_ACK;       //Unicast: Ack
		}

		ppdu.mpdu.panid = host_to_net(VMAC_PANID); // PANID
		ppdu.mpdu.seq = getSeq();	//count by software
		ppdu.mpdu.fcs = 1;		//doesn't matter, hardware supports it

		mac_to_vhal(&ppdu, &vd);
		timestamp_outgoing(msg, ker_systime32());
		Radio_Send_Pack(&vd, &timestamp);
	
        // do not retransmit broadcast messages nor timestamps!
		if( msg->daddr == BCAST_ADDRESS || msg->type == MSG_TIMESTAMP) {
		//if( 1 ) {
			retry_count = 0;
			msg_send_senddone(msg, 1, RADIO_PID);
			vmac_msg = NULL;
			vmac_send_state = VMAC_SEND_STATE_IDLE;
			return;
		} else {
			
			if( retry_count < (uint8_t)MAX_RETRIES ) {
				vmac_send_state = VMAC_SEND_STATE_WAIT_FOR_ACK;
				vmac_msg = msg;
				//
				// We add TWO milliseconds for the ACK transmission time
				//
				if( ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, 
					MacBackoff_congestionBackoff(retry_count) + 2) != SOS_OK ) {
					ker_timer_start(RADIO_PID, WAKEUP_TIMER_TID,
							MacBackoff_congestionBackoff(retry_count) + 2);
				}
				retry_count++;
			} else {
				retry_count = 0;
				msg_send_senddone(vmac_msg, 0, RADIO_PID);  //to release the memory for this msg
				vmac_msg = NULL;
				vmac_send_state = VMAC_SEND_STATE_IDLE;
				if( getMsgNumOfQueue() != 0 ) {
					post_short( RADIO_PID, RADIO_PID, MSG_VMAC_TX_NEXT_MSG, 0, 0, 0);
				}
			}
		}
	} else {
		if( retry_count < (uint8_t)MAX_RETRIES ) {
			vmac_send_state = VMAC_SEND_STATE_BACKOFF;
			vmac_msg = msg;
			if( ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, 
				MacBackoff_congestionBackoff(retry_count)) != SOS_OK ) 	// setup backoff timer
			{
				ker_timer_start(RADIO_PID, WAKEUP_TIMER_TID,
						MacBackoff_congestionBackoff(retry_count));
			}
			retry_count++;
		} else {
			retry_count = 0;
			msg_send_senddone(vmac_msg, 0, RADIO_PID);  //to release the memory for this msg
			vmac_msg = NULL;
			vmac_send_state = VMAC_SEND_STATE_IDLE;
			if( getMsgNumOfQueue() != 0 ) {
				post_short( RADIO_PID, RADIO_PID, MSG_VMAC_TX_NEXT_MSG, 0, 0, 0);
			}
		}
	}
}

/*************************************************************************
 * get message number in the queue                                       *
 *************************************************************************/
static uint8_t getMsgNumOfQueue()
{
	uint8_t ret = 0;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = vmac_pq.msg_cnt;
	LEAVE_CRITICAL_SECTION();
	return ret;
}


/*************************************************************************
 * will be called by post_net, etc functions to send message             *
 *************************************************************************/
/*
 * If send state is not idle, queue the message.
 * If CCA is busy, perform backoff.
 * Otherwise, send the message and wait for ACK.
 */
void radio_msg_alloc(Message *msg)
{
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();
	if( vmac_send_state != VMAC_SEND_STATE_IDLE ) {
		mq_enqueue(&vmac_pq, msg);
		LEAVE_CRITICAL_SECTION();
		return;
	}

	retry_count = 0;
	radio_msg_send(msg);

	LEAVE_CRITICAL_SECTION();
}

void _MacRecvAck(uint8_t ack_seq)
{
	if( (vmac_send_state == VMAC_SEND_STATE_WAIT_FOR_ACK) && (getSeq() == ack_seq) ) {
		//LED_DBG(LED_RED_TOGGLE);
		post_short( RADIO_PID, RADIO_PID, MSG_VMAC_TX_ACKED, 0, 0, 0 );
	}
}

/*************************************************************************
 * the callback fnction for reading data from cc2420                     *
 *************************************************************************/
void _MacRecvCallBack(int16_t timestamp)
{
#ifdef SOS_USE_PREEMPTION
  HAS_PREEMPTION_SECTION;
  // disable preemption
  DISABLE_PREEMPTION();
#endif

	VMAC_PPDU ppdu;
	vhal_data vd;

	mac_to_vhal(&ppdu, &vd);   // note that vd.payload_len will have a bogus entry. It will be overwritten in Radio_Recv_Pack
	Radio_Disable_Interrupt();		//disable interrupt while reading data
	if( !Radio_Recv_Pack(&vd) ) {
		Radio_Enable_Interrupt();	//enable interrupt
		return;
	}
	Radio_Enable_Interrupt();   //enable interrupt
	vhal_to_mac(&vd, &ppdu);
/* This is now done on the chip with the PANID
	if( ppdu.mpdu.group     != node_group_id ) {	 
		ker_free(vd.payload);	 
		Radio_Enable_Interrupt();		//enable interrupt
		return;	 
	}	 
	*/
	// Andreas - filter node ID here, even before allocating any new memory 
	// if you're using sos/config/base you must comment this block out!
	if (net_to_host(ppdu.mpdu.daddr) != node_address && net_to_host(ppdu.mpdu.daddr) != BCAST_ADDRESS)
	{
		ker_free(vd.payload);
		return;
	}

	Message *msg = msg_create();
	if( msg == NULL ) {
		ker_free(vd.payload);
		return;
	}
	mac_to_sosmsg(&ppdu, msg);

	// Andreas - start debug
#ifdef ENA_VMAC_UART_DEBUG
	uint8_t *payload;
	uint8_t msg_len;
	msg_len=msg->len;
	payload = msg->data;

	//post_uart(msg->sid, msg->did, msg->type, msg_len, payload, SOS_MSG_RELEASE, msg->daddr);

	// Swap daddr with saddr, because daddr is useless when debugging.
	// This way, if sossrv says "dest addr: 15" that actually means the message SENDER was node 15
	post_uart(msg->sid, msg->did, msg->type, msg_len, payload, SOS_MSG_RELEASE, msg->saddr);
#endif

	if(msg->type == MSG_TIMESTAMP){
		uint32_t timestp = ker_systime32();
		memcpy(((uint8_t*)(msg->data) + sizeof(uint32_t)),(uint8_t*)(&timestp),sizeof(uint32_t));
	}
    timestamp_incoming(msg, ker_systime32());

	// Check for duplicates
    if (msg->saddr != BCAST_ADDRESS)
    {
        uint8_t found = 0;
        uint8_t i;
        for(i=0; i<NUM_DUP_CHECK; i++)
        {
            if(msg->saddr == dup_addr[i])
            {
                if(ppdu.mpdu.seq == dup_seq[i])
                {
                    // duplicate message
                    //ker_free(vd.payload);
                    //properly dispose of the message.
                    msg->flag |= SOS_MSG_RELEASE;
                    msg_dispose(msg);
                    return;
                } else {
                    // same address, but different seq
                    dup_seq[i] = ppdu.mpdu.seq;
                    found = 1;
                    break;
                }
            }
        }
        if(!found){
            // not an entry yet. Find an empty spot, or overwrite one
            found = 0;
            for(i=0; i<NUM_DUP_CHECK; i++)
            {
                if(dup_addr[i] == BCAST_ADDRESS){
                    dup_addr[i] = msg->saddr;
                    dup_seq[i] = ppdu.mpdu.seq;
                    found = 1;
                    break;
                }
            }
            if(!found){
                // overwrite oldest
                dup_addr[oldest_dup] = msg->saddr;
                dup_seq[oldest_dup] = ppdu.mpdu.seq;
                oldest_dup = (oldest_dup + 1)%NUM_DUP_CHECK;
            }
        }
    }

	handle_incoming_msg(msg, SOS_MSG_RADIO_IO);
#ifdef SOS_USE_PREEMPTION
	ENABLE_GLOBAL_INTERRUPTS();
	ENABLE_PREEMPTION(NULL);
#endif
}

/*************************************************************************
 * set radio timestamp                                                   *
 *************************************************************************/
int8_t radio_set_timestamp(bool on)
{
	return SOS_OK;
}

/*************************************************************************
 * Implement exponential backoff mechanism                               *
 *************************************************************************/
static int16_t MacBackoff_congestionBackoff(int8_t retries)
{
	int8_t i;
	int16_t masktime = 1;
	for(i=0; i<retries; i++)
		masktime *= 2;			//masktime = 2^retries
	masktime --;				//get the mask
	if( masktime > 1023 )
		masktime = 1023;		//max backoff 1023
//	return ((ker_rand() & 0xF) + TIMER_MIN_INTERVAL);
	return ((ker_rand() & masktime) + TIMER_MIN_INTERVAL);
}

/*************************************************************************
 * Initiate the radio and mac                                            *
 *************************************************************************/
void mac_init()
{
    uint8_t i;
	Radio_Init();
	Radio_Set_Channel(RADIO_CHANNEL);
	//Radio_Set_Channel(13);

#ifdef SOS_USE_PREEMPTION
	ker_register_module(sos_get_header_address(mod_header));
#else
	sched_register_kernel_module(&vmac_module, sos_get_header_address(mod_header), NULL);
#endif

	// Timer needs to be done after reigsteration
	ker_permanent_timer_init(&wakeup_timer, RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);

	mq_init(&vmac_pq);	//! Initialize sending queue
	resetSeq();		//set seq_count to random initial number
	retry_count = 0; 	//set retries 0
    for(i=0; i<NUM_DUP_CHECK; i++){
        dup_addr[i] = BCAST_ADDRESS;
        dup_seq[i] = 0;
    }
    oldest_dup = 0;

	vmac_send_state = VMAC_SEND_STATE_IDLE;
	vmac_msg = NULL;
	//enable interrupt for receiving data
	Radio_SetPackRecvedCallBack(_MacRecvCallBack);
	Radio_Enable_Interrupt();
}
