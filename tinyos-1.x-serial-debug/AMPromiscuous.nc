// $Id: AMPromiscuous.nc,v 1.9 2003/10/07 21:46:36 idgay Exp $

/*									tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 */
/*
 *
 * Authors:		Jason Hill, David Gay, Philip Levis
 * Date last modified:  6/25/02
 *
 */

//This is an AM messaging layer implementation that understands multiple
// output devices.  All packets addressed to TOS_UART_ADDR are sent to the UART
// instead of the radio.


/**
 * @author Jason Hill
 * @author David Gay
 * @author Philip Levis
 */
#include <DlrcSerialDebugRadio.h>

module AMPromiscuous
{
  provides {
    interface StdControl as Control;
    interface CommControl;

    // The interface are as parameterised by the active message id
    interface SendMsg[uint8_t id];
    interface ReceiveMsg[uint8_t id];

    // How many packets were received in the past second
    command uint16_t activity();

  }

  uses {
    // signaled after every send completion for components which wish to
    // retry failed sends
    event result_t sendDone();

    interface StdControl as UARTControl;
    interface BareSendMsg as UARTSend;
    interface ReceiveMsg as UARTReceive;

    interface StdControl as RadioControl;
    interface BareSendMsg as RadioSend;
    interface ReceiveMsg as RadioReceive;
    interface Leds;
    interface StdControl as TimerControl;
    interface Timer as ActivityTimer;
    interface PowerManagement;
  }
}
implementation
{
  bool state;
  TOS_MsgPtr buffer;
  uint16_t lastCount;
  uint16_t counter;
  bool promiscuous_mode;
  bool crc_check;
#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND || DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_RECV
  DLRC_MSG_PTR_QUEUE queue;
  uint8_t	semaphore;
#endif
  
  // Initialization of this component
  command result_t Control.init() {
    result_t ok1, ok2;
    call TimerControl.init();
    ok1 = call UARTControl.init();
    ok2 = call RadioControl.init();
    state = FALSE;
    lastCount = 0;
    counter = 0;
    promiscuous_mode = FALSE;
    crc_check = TRUE;
    dbg(DBG_BOOT, "AM Module initialized\n");

#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND || DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_RECV
	DlrcMsgPtrQueueInit(&queue);
	semaphore = 0;
#endif
    return rcombine(ok1, ok2);
  }
  

  // Command to be used for power managment
  command result_t Control.start() {
    result_t ok0,ok1,ok2,ok3;

    ok0 = call TimerControl.start();
    ok1 = call UARTControl.start();
    ok2 = call RadioControl.start();
    ok3 = call ActivityTimer.start(TIMER_REPEAT, 1000);
    call PowerManagement.adjustPower();
    
    //HACK -- unset start here to work around possible lost calls to 
    // sendDone which seem to occur when using power management.  SRM 4.4.03
    state = FALSE; 

    return rcombine4(ok0, ok1, ok2, ok3);
  }

  
  command result_t Control.stop() {
    result_t ok1,ok2,ok3;
    if (state) return FALSE;
    ok1 = call UARTControl.stop();
    ok2 = call RadioControl.stop();
    ok3 = call ActivityTimer.stop();
    // call TimerControl.stop();
    call PowerManagement.adjustPower();
    return rcombine3(ok1, ok2, ok3);
  }

  command result_t CommControl.setCRCCheck(bool value) {
    crc_check = value;
    return SUCCESS;
  }

  command bool CommControl.getCRCCheck() {
    return crc_check;
  }

  command result_t CommControl.setPromiscuous(bool value) {
    promiscuous_mode = value;
    return SUCCESS;
  }

  command bool CommControl.getPromiscuous() {
    return promiscuous_mode;
  }

  command uint16_t activity() {
    return lastCount;
  }
  
  void dbgPacket(TOS_MsgPtr data) {
    uint8_t i;

    for(i = 0; i < sizeof(TOS_Msg); i++)
      {
	dbg_clear(DBG_AM, "%02hhx ", ((uint8_t *)data)[i]);
      }
    dbg(DBG_AM, "\n");
  }

  // Handle the event of the completion of a message transmission
  result_t reportSendDone(TOS_MsgPtr msg, result_t success) {
    dbg(DBG_AM, "AM report send done for message to 0x%x, type %d.\n", msg->addr, msg->type);
    state = FALSE;
    signal SendMsg.sendDone[msg->type](msg, success);
    signal sendDone();

    return SUCCESS;
  }

  event result_t ActivityTimer.fired() {
    lastCount = counter;
    counter = 0;
    return SUCCESS;
  }
  
  default event result_t SendMsg.sendDone[uint8_t id](TOS_MsgPtr msg, result_t success) {
    return SUCCESS;
  }
  default event result_t sendDone() {
    return SUCCESS;
  }

  // This task schedules the transmission of the Active Message
  task void sendTask() {
    result_t ok;

    if (buffer->addr == TOS_UART_ADDR)
      ok = call UARTSend.send(buffer);
    else
      ok = call RadioSend.send(buffer);

    if (ok == FAIL) // failed, signal completion immediately
      reportSendDone(buffer, FAIL);
  }

  // Command to accept transmission of an Active Message
  command result_t SendMsg.send[uint8_t id](uint16_t addr, uint8_t length, TOS_MsgPtr data) {
    if (!state) {
      state = TRUE;
      call Leds.greenToggle();

      if (length > DATA_LENGTH) {
	dbg(DBG_AM, "AM: Send length too long: %i. Fail.\n", (int)length);
	state = FALSE;
	return FAIL;
      }
      if (!(post sendTask())) {
	dbg(DBG_AM, "AM: post sendTask failed.\n");
	state = FALSE;
	return FAIL;
      }
      else {
#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND
	semaphore++;
#endif
	buffer = data;
	data->length = length;
	data->addr = addr;
	data->type = id;
	buffer->group = TOS_AM_GROUP;
	dbgPacket(data);
	dbg(DBG_AM, "Sending message: %hx, %hhx\n\t", addr, id);
      }
      return SUCCESS;
    }
    
    return FAIL;
  }

#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND || DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_RECV
  task void SerialDebugTask()
  {
		result_t ok;
		TOS_Msg		*pMsg;
		
		pMsg = (TOS_Msg *)DlrcMsgPtrQueuePop(&queue);
		if (pMsg == NULL)
			return;
			
	  ok = call UARTSend.send(pMsg);
	  if (ok == SUCCESS)
	  	DlrcMsgPtrQueueDelHead(&queue);		// send success, delete queue head
	  else
	  	post SerialDebugTask();			// send fail, don't update delete head, and repost a task
	}
#endif

  event result_t UARTSend.sendDone(TOS_MsgPtr msg, result_t success) {
    return reportSendDone(msg, success);
  }
  event result_t RadioSend.sendDone(TOS_MsgPtr msg, result_t success) {
#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND
	if (semaphore != 0)
	{
		semaphore--;
		if (DlrcMsgPtrQueuePush(&queue, msg, success))
			post SerialDebugTask();	//send radio msg by UART
	}
#endif
    return reportSendDone(msg, success);
  }

  // Handle the event of the reception of an incoming message
  TOS_MsgPtr prom_received(TOS_MsgPtr packet)  __attribute__ ((C, spontaneous)) {
    counter++;
    dbg(DBG_AM, "AM_address = %hx, %hhx; counter:%i\n", packet->addr, packet->type, (int)counter);

    if (packet->group == TOS_AM_GROUP &&
	(promiscuous_mode == TRUE || 
 	 packet->addr == TOS_BCAST_ADDR ||
	 packet->addr == TOS_LOCAL_ADDRESS) &&
	(crc_check == FALSE || packet->crc == 1)) 
      {
	uint8_t type = packet->type;
	TOS_MsgPtr tmp;
    
	// Debugging output
	dbg(DBG_AM, "Received message:\n\t");
	dbgPacket(packet);
	dbg(DBG_AM, "AM_type = %d\n", type);

#if DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_RECV
	if (DlrcMsgPtrQueuePush(&queue, packet, DLRC_SERIAL_DBG_RADIO_RECEIVE))
		post SerialDebugTask();
#endif

	// dispatch message
	tmp = signal ReceiveMsg.receive[type](packet);
	if (tmp) 
	  packet = tmp;
      }
    return packet;
  }

  // default do-nothing message receive handler
  default event TOS_MsgPtr ReceiveMsg.receive[uint8_t id](TOS_MsgPtr msg) {
    return msg;
  }

  event TOS_MsgPtr UARTReceive.receive(TOS_MsgPtr packet) {
	packet->group = TOS_AM_GROUP;
    return prom_received(packet);
  }
  event TOS_MsgPtr RadioReceive.receive(TOS_MsgPtr packet) {
    return prom_received(packet);
  }
}

