/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License
 *
 *  Copyright (c) 2002 Intel Corporation
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *      Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTEL OR ITS
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */
/*
 * @author Phil Buonadonna
 * @author Andrew Barton-Sweeney  (abs@cs.yale.edu)
 *
 */

#include <hardware_proc.h>
#include <hardware_once.h>
#include <string.h>	//memcpy

// Until we implement jump table...hardcoded
//extern void OSTIrq_fired(void);
//extern void GPIOIrq0_fired(void);
//extern void GPIOIrq1_fired(void);
//extern void GPIOIrq_fired(void);
//extern void HPLSTUART_Interrupt_fired(void);
//extern void PI2CInterrupt_fired(void);
//extern void PXA27XQuickCaptIntIrq_fired(void);

void (*irq_callback[40])(void);

/*
 * Core PXA27X interrupt dispatch vectors
 * DO NOT change the name of these functions
 */

void __attribute__((interrupt("IRQ"))) hplarmv_irq(void)
{
	uint32_t IRQPending;
	IRQPending = ICHP;  // Determine which interrupt to service
	IRQPending >>= 16;  // Right justify to the IRQ portion
	while (IRQPending & (1 << 15)) {
		uint8_t PeripheralID = (IRQPending & 0x3f); // Get rid of the Valid bit
		//PXA27XIrq_fired[PeripheralID]();     // Handler is responsible for clearing interrupt
		void (*callback)(void) = irq_callback[PeripheralID];
		if (callback)
			callback();
#if 0
		switch (PeripheralID) {
			case PPID_OST_4_11: {
				OSTIrq_fired();
				break;
			}
			case PPID_GPIO_0: {
				//GPIOIrq0_fired();
				break;
			}
			case PPID_GPIO_1: {
				//GPIOIrq1_fired();
				break;
			}
			case PPID_GPIO_X: {
				//GPIOIrq_fired();
				break;
			}
			case PPID_STUART: {
				HPLSTUART_Interrupt_fired();
				break;
			}
			case PPID_PWR_I2C: {
				PI2CInterrupt_fired();
				break;
			}
			case PPID_CIF: {
				PXA27XQuickCaptIntIrq_fired();
				break;
			}
		}
#endif
		IRQPending = ICHP;  // Determine which interrupt to service
		IRQPending >>= 16;  // Right justify to the IRQ portion
	}
	return;
}

void hplarmv_fiq(void)
{
	uint32_t FIQPending;
	FIQPending = ICHP;   // Determine which interrupt to service
	FIQPending &= 0xFF;  // Mask off the IRQ portion
	while (FIQPending & (1 << 15)) {
		//uint8_t PeripheralID = (FIQPending & 0x3f); // Get rid of the Valid bit
		//signal PXA27XFiq.fired[PeripheralID]();	  // Handler is responsible for clearing interrupt
		FIQPending = ICHP;
		FIQPending &= 0xFF;
	}
	return;
}

static uint8_t usedPriorities = 0;

/* Helper functions */
/* NOTE: Read-back of all register writes is necessary to ensure the data latches */

int8_t allocate(uint8_t id, int8_t level, uint8_t priority, void(*callback)(void))			// we should pass the higher layer handler here to store in the table!
{
	uint32_t tmp;
	int8_t result = 0;	//FAIL;
	uint8_t i;
	if(usedPriorities == 0){//assumed that the table will have some entries
		uint8_t PriorityTable[40], DuplicateTable[40];
		for(i = 0; i < 40; i++){
			DuplicateTable[i] = PriorityTable[i] = 0xFF;
		}
		for(i = 0; i < 40; i++)
			if(SOS_IRP_TABLE[i] != 0xff){
				if(PriorityTable[SOS_IRP_TABLE[i]] != 0xFF)/*duplicate priorities
							  in the table, mark
							  for later fixing*/
					DuplicateTable[i] = PriorityTable[SOS_IRP_TABLE[i]];
				else
					PriorityTable[SOS_IRP_TABLE[i]] = i;
			}
		//compress table
		for(i = 0; i < 40; i++){
			if(PriorityTable[i] != 0xff){
				PriorityTable[usedPriorities] = PriorityTable[i];
				if(i != usedPriorities)
					PriorityTable[i] = 0xFF;
				usedPriorities++;
			}
		}
		for(i = 0; i < 40; i++)
			if(DuplicateTable[i] != 0xFF){
				uint8_t j, ExtraTable[40];
				for(j = 0; DuplicateTable[i] != PriorityTable[j]; j++);
				memcpy(ExtraTable + j + 1, PriorityTable + j, usedPriorities - j);
				memcpy(PriorityTable + j + 1, ExtraTable + j + 1, usedPriorities - j);
				PriorityTable[j] = i;
				usedPriorities++;
			}
		for(i = 0; i < usedPriorities; i++){
			IPR(i) = (IPR_VALID | PriorityTable[i]);
			tmp = IPR(i);
		}
	}
	if (id < 34){
		if(priority == 0xff){
			priority = usedPriorities;
			usedPriorities++;
			IPR(priority) = (IPR_VALID | (id));
			tmp = IPR(priority);
		}
		if (level) {
			_ICLR(id) |= _PPID_Bit(id);
			tmp = _ICLR(id);
		}
		irq_callback[id] = callback;
		result = 0 /* SUCCESS */;
	}
    return result;
}

void enable(uint8_t id)
{
	uint32_t tmp;
	//atomic {
	if (id < 34) {
		_ICMR(id) |= _PPID_Bit(id);
		tmp = _ICMR(id);
	}
	//}
	return;
}

void disable(uint8_t id)
{
	uint32_t tmp;
	//atomic {
	if (id < 34) {
		_ICMR(id) &= ~(_PPID_Bit(id));
		tmp = _ICMR(id);
	}
	//}
	return;
}

/* Interface implementation */

int8_t PXA27XIrq_allocate(uint8_t id, void(*callback)(void))
{
	return allocate(id, 0 /* FALSE */, SOS_IRP_TABLE[id], callback);
}

void PXA27XIrq_enable(uint8_t id)
{
	enable(id);
	return;
}

void PXA27XIrq_disable(uint8_t id)
{
	disable(id);
	return;
}

int8_t PXA27XFiq_allocate(uint8_t id, void(*callback)(void))
{
	return allocate(id, 1 /* TRUE */, SOS_IRP_TABLE[id],callback);
}

void PXA27XFiq_enable(uint8_t id)
{
	enable(id);
	return;
}

void PXA27XFiq_disable(uint8_t id)
{
	disable(id);
	return;
}

// void PXA27XIrq_clear(uint8_t id)
