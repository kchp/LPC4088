
#include <stdio.h>
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

//	CAN parameters:
#define CAN_CTRL_NO         0
#define LPC_CAN             (LPC_CAN1)
#define LPC_CAN_2             (LPC_CAN2)

#define FULL_CAN_AF_USED    1
#define CAN_TX_MSG_STD_ID (0x80)
#define CAN_TX_MSG_REMOTE_STD_ID (0x300)
#define CAN_TX_MSG_EXT_ID (0x10000200)
#define CAN_RX_MSG_ID (0x100)

//	NVIC parameters:
#define GPIO_IRQ_HANDLER  			GPIO_IRQHandler		/* GPIO interrupt IRQ function name */
#define GPIO_INTERRUPT_NVIC_NAME    GPIO_IRQn			/* GPIO interrupt NVIC interrupt name */

CAN_BUFFER_ID_T TxBuf;			//	Buffer
typedef struct
{
	unsigned int ID;
	unsigned char ID_ext;
	unsigned char DLC;
	unsigned data[8];
} can_frame;

xQueueHandle queue_handle;		//	Declare queuehandler
xSemaphoreHandle binary_sem;	//	Declare gatekeeper
long task_woken = 0;

/**
 * GPIO Interrupt handler/ISR
 */
void GPIO_IRQ_HANDLER(void)
{
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, 2, 1 << 10);
	xSemaphoreGiveFromISR(binary_sem, &task_woken);
}

/** Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
}

/** transmit thread */
static void TX_task(void *pvParameters) {

	can_frame test;
	test.ID = 14;
	test.ID_ext = 0;
	test.data[0] = 'K';
	test.data[1] = 'i';
	test.data[2] = 'm';
	test.DLC = sizeof(test.data);

	while (1)
	{
		if(xSemaphoreTake(binary_sem, 9999999))
		{
			long ok = xQueueSend(queue_handle, &test, 500);
			puts(ok ? "OK" : "FAILED");
			puts("Button was pressed!\n");
		}
	}
}

/** receive thread */
static void RX_task(void *pvParameters) {
	can_frame test;

	while (1)
	{
		if(xQueueReceive(queue_handle, &test, 500))
		{
			puts("Received data:");
			TxBuf = Chip_CAN_GetFreeTxBuf(LPC_CAN);		//	Find free buffer
			Chip_CAN_Send(LPC_CAN, TxBuf, &test);		//	Send data
		}

		vTaskDelay(configTICK_RATE_HZ / 14);
	}
}

/**
 * @name main
 * @return
 */
int main(void)
{
	prvSetupHardware();

	DigitalIn(2, 10);	//	Set pindirection as input
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, 2, 1 << 10);

	NVIC_ClearPendingIRQ(GPIO_INTERRUPT_NVIC_NAME);
	NVIC_EnableIRQ(GPIO_INTERRUPT_NVIC_NAME);
	NVIC_SetPriority(GPIO_INTERRUPT_NVIC_NAME, 5);	//	Since FreeRTOS only can handle priorities between 0-5, GPIO interrupt is given the highest

	//	Setup CAN:
	Chip_CAN_Init(LPC_CAN, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_SetBitRate(LPC_CAN, 500000);
	Chip_CAN_EnableInt(LPC_CAN, CAN_IER_BITMASK);

	Board_CAN_Init(LPC_CAN);						//	Initialize CAN channel 1
	Board_CAN_Init(LPC_CAN_2);						//	Initialize CAN channel 2
	Chip_CAN_Init(LPC_CAN, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_Init(LPC_CAN_2, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_SetBitRate(LPC_CAN, 125000);			//	125 kbit/s on channel 1
	Chip_CAN_SetBitRate(LPC_CAN_2, 125000);			//	125 kbit/s on channel 2
	Chip_CAN_EnableInt(LPC_CAN, CAN_IER_BITMASK);
	Chip_CAN_EnableInt(LPC_CAN_2, CAN_IER_BITMASK);

	queue_handle = xQueueCreate(2, sizeof(can_frame));	//	Install the queue
	vSemaphoreCreateBinary(binary_sem);					//	Install binary semaphore to use sync mechanisms

	/** tx task:*/
	xTaskCreate(TX_task, (signed char *) "TX_task", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), (xTaskHandle *) NULL);
	/** rx task:*/
	xTaskCreate(RX_task, (signed char *) "RX_task", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), (xTaskHandle *) NULL);

	/** Start the scheduler */
	vTaskStartScheduler();

	return 1;	//	Should never arrive here.
}
