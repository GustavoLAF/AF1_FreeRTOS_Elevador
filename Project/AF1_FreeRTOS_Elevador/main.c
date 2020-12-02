/* Standard includes. */
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

/* FreeRTOS.org includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/* Functions. */
#include "supporting_functions.h"

/* Windows */
#include "windows.h"

/*-----------------------------------------------------------*/

typedef enum {
	Stopped = 0,
	Down = 1,
	Up = 2
} DirecaoEnum;

typedef struct
{
	int AndarAtual;
	DirecaoEnum DirecaoAtual;
	int emUso;
} ElevadorSct;

typedef struct
{
	int AndarOrigem;
	int AndarDestino;
	int IndiceElevadorAtual;
} PassageiroSct;

ElevadorSct Elevadores[3];
PassageiroSct Passageiros[3];
PassageiroSct Passageiro;

static TimerHandle_t xChegadaOrigemTimer = NULL;
static TimerHandle_t xChegadaDestinoTimer = NULL;

/*-----------------------------------------------------------*/

/*
 * The callback function used by the timer.
 */
static void prvChegadaElevadorOrigemTimerCallback(TimerHandle_t xTimer);
static void prvChegadaElevadorDestinoTimerCallback(TimerHandle_t xTimer);

/*
 * A real application, running on a real target, would probably read button
 * pushes in an interrupt.  That allows the application to be event driven, and
 * prevents CPU time being wasted by polling for key presses when no keys have
 * been pressed.  It is not practical to use real interrupts when using the
 * FreeRTOS Windows port, so the vKeyHitTask() task is created to provide the
 * key reading functionality by simply polling the keyboard.
 */
static void vKeyHitTask();
static void ChegadaDoElevadorOrigem();
static void ChegadaDoElevadorDestino();
static void ApresentarInfoElevadores();
static void AtualizarElevadores();
static char* ConvertDirecaoIntToString(int direcao);
static int ElevadorAcionadoPelaAproximacao();

/*-----------------------------------------------------------*/

int main(void)
{
	Elevadores[0].AndarAtual = 1;
	Elevadores[0].DirecaoAtual = 0;

	Elevadores[1].AndarAtual = 2;
	Elevadores[1].DirecaoAtual = 1;

	Elevadores[2].AndarAtual = 7;
	Elevadores[2].DirecaoAtual = 2;

	ApresentarInfoElevadores();
	xTaskCreate(AtualizarElevadores, "Atualização em Background dos Elevadores FreeRTOS", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(vKeyHitTask, "Rotina de Operação dos Elevadores FreeRTOS", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

	vTaskStartScheduler();
	for (;; );
	return 0;
}

/*-----------------------------------------------------------*/

static int ElevadorAcionadoPelaAproximacao() {
	//Anotação: Levado em consideração que o passageiro sempre tem a intenção de descer
	int indexAndarMaisProximo = 0;
	int resultadoMaisProximo = 0;

	for (int i = 0; i < 3; i++) {
		int proximidade = abs(Elevadores[i].AndarAtual - Passageiro.AndarOrigem);
		if (i == 0 || proximidade < resultadoMaisProximo) {
			resultadoMaisProximo = proximidade;
			indexAndarMaisProximo = i;
		}
	}

	return indexAndarMaisProximo;
}

static void ChegadaDoElevadorOrigem() {
	ElevadorSct elevadorPassageiro = Elevadores[Passageiro.IndiceElevadorAtual];
	int tempoChegada = 0;

	if (elevadorPassageiro.AndarAtual > Passageiro.AndarOrigem) {
		tempoChegada = (elevadorPassageiro.AndarAtual - Passageiro.AndarOrigem) * 2;
		elevadorPassageiro.DirecaoAtual = 1;
	}
	else {
		tempoChegada = (Passageiro.AndarOrigem - elevadorPassageiro.AndarAtual) * 2;
		elevadorPassageiro.DirecaoAtual = 2;
	}

	printf("\nAguarde aproximadamente %i segundos ate que o elevador %i chegue ate voce...", tempoChegada, Passageiro.IndiceElevadorAtual + 1);

	tempoChegada = tempoChegada * 1000;
	xChegadaOrigemTimer = xTimerCreate("ChegadaDoElevadorOrigemTimer", pdMS_TO_TICKS(tempoChegada), NULL, 0, prvChegadaElevadorOrigemTimerCallback);
	xTimerStart(xChegadaOrigemTimer, 0);
}

static void ChegadaDoElevadorDestino() {
	ElevadorSct elevadorPassageiro = Elevadores[Passageiro.IndiceElevadorAtual];

	if (!elevadorPassageiro.emUso && elevadorPassageiro.AndarAtual == Passageiro.AndarOrigem) {
		ApresentarInfoElevadores();
		printf("EM USO %i", elevadorPassageiro.emUso);
		printf("Voce esta no %io andar", Passageiro.AndarOrigem);
		printf("\n\nO elevador %i acionado ja esta no seu andar!", Passageiro.IndiceElevadorAtual + 1);
		vPrintString("\n\nInforme agora o andar de destino: ");
	}

	int andarDestinoASC = _getch();
	if (andarDestinoASC >= 48 && andarDestinoASC <= 57) {
		Passageiro.AndarDestino = andarDestinoASC - 48;

		if (Passageiro.AndarDestino == elevadorPassageiro.AndarAtual) {
			vPrintString("\n\nVoce ja se encontra no destino. Obrigado por usar o elevador!");
		}
		else {
			int tempoChegada = 0;

			if (elevadorPassageiro.AndarAtual > Passageiro.AndarDestino) {
				tempoChegada = (elevadorPassageiro.AndarAtual - Passageiro.AndarDestino) * 2;
				elevadorPassageiro.DirecaoAtual = 1;
			}
			else {
				tempoChegada = (Passageiro.AndarDestino - elevadorPassageiro.AndarAtual) * 2;
				elevadorPassageiro.DirecaoAtual = 2;
			}

			printf("\n\nVoce selecionou o %i andar", Passageiro.AndarDestino);
			printf("\n\nAguarde aproximadamente %i segundos ate que o elevador %i chegue ate o %i andar...", tempoChegada, Passageiro.IndiceElevadorAtual + 1, Passageiro.AndarDestino);

			tempoChegada = tempoChegada * 1000;

			xChegadaDestinoTimer = xTimerCreate("ChegadaDoElevadorDestinoTimer", pdMS_TO_TICKS(tempoChegada), NULL, 0, prvChegadaElevadorDestinoTimerCallback);
			xTimerStart(xChegadaDestinoTimer, 0);
		}
	}
}

static void prvChegadaElevadorOrigemTimerCallback(TimerHandle_t xTimer)
{
	TickType_t xTimeNow = xTaskGetTickCount();

	Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual = Passageiro.AndarOrigem;

	if (Passageiro.AndarOrigem > Passageiro.AndarDestino) {
		Elevadores[Passageiro.IndiceElevadorAtual].DirecaoAtual = 1;
	}
	else if (Passageiro.AndarOrigem < Passageiro.AndarDestino) {
		Elevadores[Passageiro.IndiceElevadorAtual].DirecaoAtual = 2;
	}
	else {
		Elevadores[Passageiro.IndiceElevadorAtual].DirecaoAtual = 0;
	}

	ApresentarInfoElevadores();
	printf("O elevador %i chegou no andar %i! Tempo percorrido: %i segundos", Passageiro.IndiceElevadorAtual + 1, Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual, xTimeNow / 1000);
	vPrintString("\n\nInforme agora o andar de destino: ");
}

static void prvChegadaElevadorDestinoTimerCallback(TimerHandle_t xTimer)
{
	TickType_t xTimeNow = xTaskGetTickCount();

	Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual = Passageiro.AndarDestino;

	if (Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual == Passageiro.AndarDestino) {
		Elevadores[Passageiro.IndiceElevadorAtual].DirecaoAtual = 0;
	}

	Passageiro.AndarOrigem = Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual;

	ApresentarInfoElevadores();
	printf("O elevador %i chegou no andar %i! Tempo percorrido: %i segundos", Passageiro.IndiceElevadorAtual + 1, Elevadores[Passageiro.IndiceElevadorAtual].AndarAtual, xTimeNow / 1000);
	vPrintString("\n\nObrigado por utilizar o elevador!");

	Elevadores[Passageiro.IndiceElevadorAtual].emUso = 0;
}

static void ApresentarInfoElevadores() {
	system("cls");
	vPrintString("##### ELEVADOR FREERTOS #####\r\n");

	printf("Elevador 1 - AndarAtual: %i; DirecaoAtual: %s\n", Elevadores[0].AndarAtual, ConvertDirecaoIntToString(Elevadores[0].DirecaoAtual));
	printf("Elevador 2 - AndarAtual: %i; DirecaoAtual: %s\n", Elevadores[1].AndarAtual, ConvertDirecaoIntToString(Elevadores[1].DirecaoAtual));
	printf("Elevador 3 - AndarAtual: %i; DirecaoAtual: %s\n", Elevadores[2].AndarAtual, ConvertDirecaoIntToString(Elevadores[2].DirecaoAtual));
	printf("Passageiro - AndarAtual: %i; AndarDestino: %i; ElevadorAcionado: %i\n\n", Passageiro.AndarOrigem, Passageiro.AndarDestino, Passageiro.IndiceElevadorAtual + 1);
}

static char* ConvertDirecaoIntToString(int direcao) {
	if (direcao == 0) {
		return "Parado";
	}
	else if (direcao == 1) {
		return "Descendo";
	}
	else {
		return "Subindo";
	}
}

static void AtualizarElevadores() {
	const TickType_t xShortDelay = pdMS_TO_TICKS(2000);

	for (;; ) {
		for (int i = 0; i < 3; i++) {
			if (!Elevadores[i].emUso) {
				if (Elevadores[i].AndarAtual > 0 && Elevadores[i].AndarAtual < 9) {
					if (Elevadores[i].DirecaoAtual == 1) {
						Elevadores[i].AndarAtual--;
					}
					else if (Elevadores[i].DirecaoAtual == 2) {
						Elevadores[i].AndarAtual++;
					}
				}
				else if (Elevadores[i].AndarAtual == 0 || Elevadores[i].AndarAtual == 9) {
					Elevadores[i].DirecaoAtual = 0;
				}
			}
		}

		vTaskDelay(xShortDelay);
	}
}

static void vKeyHitTask()
{
	const TickType_t xShortDelay = pdMS_TO_TICKS(50);

	vPrintString("Pressione a tecla 'p' para acionar o elevador\r\n");

	for (;; )
	{
		if (_kbhit() != 0)
		{
			/*
			   p    112   0111 0000
			   0    48    0011 0000
			   1    49    0011 0001
			   2    50    0011 0010
			   3    51    0011 0011
			   4    52    0011 0100
			   5    53    0011 0101
			   6    54    0011 0110
			   7    55    0011 0111
			   8    56    0011 1000
			   9    57    0011 1001
			*/

			int currentKey = _getch();
			if (currentKey == 112) {
				vPrintString("\nVoce acionou o elevador. Informe o numero do andar em que voce esta:");

				int andarOrigemASC = _getch();
				if (andarOrigemASC >= 48 && andarOrigemASC <= 57) {
					Passageiro.AndarOrigem = andarOrigemASC - 48;
					printf("\n\nVoce esta no %io andar", Passageiro.AndarOrigem);

					int indiceElevadorMaisProximo = ElevadorAcionadoPelaAproximacao();
					printf("\n\nO elevador %i foi acionado.	\n", indiceElevadorMaisProximo + 1);
					Passageiro.IndiceElevadorAtual = indiceElevadorMaisProximo;

					ElevadorSct elevadorMaisProximo = Elevadores[indiceElevadorMaisProximo];
					Elevadores[indiceElevadorMaisProximo].emUso = 1;

					if (elevadorMaisProximo.AndarAtual == Passageiro.AndarOrigem) {
						ChegadaDoElevadorDestino();
					}
					else {
						ChegadaDoElevadorOrigem();
					}
				}
			}
			else if (currentKey >= 48 && currentKey <= 57) {
				ChegadaDoElevadorDestino();
			}
		}

		vTaskDelay(xShortDelay);
	}
}

/*-----------------------------------------------------------*/

