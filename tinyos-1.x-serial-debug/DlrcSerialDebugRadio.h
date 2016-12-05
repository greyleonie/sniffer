
#ifndef __DLRC_SERIAL_DEBUG_RADIO_H__
#define	__DLRC_SERIAL_DEBUG_RADIO_H__


#define	DLRC_AMSTANDARD_SERIAL_DBG_RADIO_SEND				0
#define	DLRC_AMSTANDARD_SERIAL_DBG_RADIO_RECV				1
#define	DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_SEND		1	
#define	DLRC_AMPROMISCUOUS_SERIAL_DBG_RADIO_RECV		0

// define serial debug message key word
#define	DLRC_SERIAL_DBG_RADIO_RECEIVE		0xFF	//this packet is received packet
#define	DLRC_SERIAL_DBG_RADIO_DATA_OVERFLOW	0xFE	//data packet length overflow

// define message pointer queue for serial debug task
#define DLRC_MSG_QUEUE_LENGTH	8

typedef struct _DLRC_MSG_PTR_QUEUE
{
	TOS_Msg	MsgQueue[DLRC_MSG_QUEUE_LENGTH];
	uint8_t Head;
	uint8_t	Tail;
}DLRC_MSG_PTR_QUEUE;

void DlrcMsgPtrQueueInit(DLRC_MSG_PTR_QUEUE *queue)
{
	queue->Head = 0;
	queue->Tail = 0;
}

bool DlrcMsgPtrQueuePush(DLRC_MSG_PTR_QUEUE *queue, TOS_Msg *p, result_t success)
{
	TOS_Msg	*pMsg;
	uint8_t	used;

	if (((queue->Tail + 1) & (DLRC_MSG_QUEUE_LENGTH - 1)) == queue->Head)	// queue full
		return FALSE;

	pMsg = &queue->MsgQueue[queue->Tail];
	queue->Tail = ((queue->Tail + 1) & (DLRC_MSG_QUEUE_LENGTH - 1));

	// calculate message queue used item
	used = (queue->Tail > queue->Head) ? (queue->Tail - queue->Head)
		: (queue->Tail + DLRC_MSG_QUEUE_LENGTH - queue->Head);		

	// config message 
	memcpy(pMsg, p, sizeof(TOS_Msg));
	if (pMsg->length > (TOSH_DATA_LENGTH - 5))
	{
		pMsg->data[TOSH_DATA_LENGTH - 5] = ((uint8_t *)&TOS_LOCAL_ADDRESS)[0];
		pMsg->data[TOSH_DATA_LENGTH - 4] = ((uint8_t *)&TOS_LOCAL_ADDRESS)[1];
		pMsg->data[TOSH_DATA_LENGTH - 3] = TOS_AM_GROUP;
		pMsg->data[TOSH_DATA_LENGTH - 2] = DLRC_SERIAL_DBG_RADIO_DATA_OVERFLOW;
		pMsg->data[TOSH_DATA_LENGTH - 1] = used;
		pMsg->length = TOSH_DATA_LENGTH;
	}
	else
	{
		pMsg->data[pMsg->length++] = ((uint8_t *)&TOS_LOCAL_ADDRESS)[0];
		pMsg->data[pMsg->length++] = ((uint8_t *)&TOS_LOCAL_ADDRESS)[1];
		pMsg->data[pMsg->length++] = TOS_AM_GROUP;
		pMsg->data[pMsg->length++] = success;	// append success to data array
		pMsg->data[pMsg->length++] = used;
	}
		
	return TRUE;
}

TOS_Msg *DlrcMsgPtrQueuePop(DLRC_MSG_PTR_QUEUE *queue)
{
	TOS_Msg *pMsg = NULL;
	if (queue->Head != queue->Tail)
	{
		pMsg = &queue->MsgQueue[queue->Head];
	}
	return pMsg;
}

void DlrcMsgPtrQueueDelHead(DLRC_MSG_PTR_QUEUE *queue)
{
	queue->Head = ((queue->Head + 1) & (DLRC_MSG_QUEUE_LENGTH - 1));
}


#endif
