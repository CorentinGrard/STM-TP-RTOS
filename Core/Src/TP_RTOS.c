#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "usart.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

#define FLAG_BOUTON_BLEU 1
#define TAILLE_MESSAGE 50

void SystemClock_Config(void);
extern UART_HandleTypeDef huart2;
extern RNG_HandleTypeDef hrng;
volatile uint8_t Nombre_Aleatoire_Present = 0;
osThreadId_t ID_Thread_Btn_Time;

osMessageQueueId_t queue_id;
osMessageQueueAttr_t queue_attr = { .name = "queue" };

osThreadAttr_t Config_Str_Rnd_Thread = { .name = "Str_Rnd_Thread", .priority =
		osPriorityNormal, .stack_size = 128 * 4 };

osThreadAttr_t Config_Btn_Time_Thread = { .name = "Btn_Time_Thread", .priority =
		osPriorityNormal, .stack_size = 128 * 4 };

osThreadAttr_t Config_Send_Messages_Thread = { .name = "Send_Messages_Thread",
		.priority = osPriorityLow, .stack_size = 128 * 4 };

void Str_Rnd_Thread(void *P_Info) {
	HAL_RNG_GenerateRandomNumber_IT(&hrng);
	char message[TAILLE_MESSAGE];
	uint32_t rng;
	while (1) {
		if (Nombre_Aleatoire_Present) {
			Nombre_Aleatoire_Present = 0;
			rng = HAL_RNG_ReadLastRandomNumber(&hrng);
			sprintf(message, "Rng : %ld", rng);
			osMessageQueuePut(queue_id, message, 0, osWaitForever);
			HAL_RNG_GenerateRandomNumber_IT(&hrng);
			osDelay(500);
		}
	}
	osThreadTerminate(NULL);
}

void Btn_Time_Thread(void *P_Info) {
	char message[TAILLE_MESSAGE];
	while (1) {
		osThreadFlagsWait( FLAG_BOUTON_BLEU, osFlagsWaitAny, HAL_MAX_DELAY);
		uint32_t time = osKernelSysTick();
		sprintf(message, "Time : %ld", time);
		osMessageQueuePut(queue_id, message, 0, osWaitForever);

	}
	osThreadTerminate(NULL);
}

void Send_Messages_Thread(void *P_Info) {
	char message[TAILLE_MESSAGE];
	while (1) {
		osMessageQueueGet(queue_id, message, 0, osWaitForever);
		printf("%s\r\n", message);
	}
	osThreadTerminate(NULL);
}

int main() {
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_RNG_Init();

	queue_id = osMessageQueueNew(100, TAILLE_MESSAGE, &queue_attr);

	osKernelInitialize();
	osThreadNew(Str_Rnd_Thread, NULL, &Config_Str_Rnd_Thread);
	ID_Thread_Btn_Time = osThreadNew(Btn_Time_Thread, NULL,
			&Config_Btn_Time_Thread);
	osThreadNew(Send_Messages_Thread, NULL, &Config_Send_Messages_Thread);
	printf("Start OS...\r\n");
	osKernelStart();

	while (1)
		;
}

void HAL_GPIO_EXTI_Callback(uint16_t P_Numero_GPIO) {
	if (P_Numero_GPIO == BOUTON_BLEU_Pin) {
		osThreadFlagsSet(ID_Thread_Btn_Time, FLAG_BOUTON_BLEU);
	}
}

void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *P_Handle, uint32_t P_Valeur) {
	UNUSED(P_Valeur);
	if (P_Handle == &hrng) {
		Nombre_Aleatoire_Present = 1;
	}
}

int _write(int P_Flux, char *P_Message, int P_Taille) {
	HAL_StatusTypeDef Etat;
	Etat = HAL_UART_Transmit(&huart2, (uint8_t*) P_Message, P_Taille,
	HAL_MAX_DELAY);
	if (Etat == HAL_OK)
		return P_Taille;
	else
		return -1;
}
