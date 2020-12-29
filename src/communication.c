#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "communication.h"
#include "Drivers/BLDC.h"
#include "Drivers/A4988.h"
#include "Drivers/LED.h"
#include "Drivers/CRC8.h"
#include "Drivers/XY-KPWM.h"
#include <string.h>

volatile uint8_t uart_pointer=0;
volatile uint8_t UART_cmdReceived=0;
volatile uint8_t UART_cmd_buff[CMD_BUFF_SIZE];
#define cmd_is(x) (memcmp((void*)UART_cmd_buff,x,sizeof(x)-1)==0)



//---------------  Internal functions prototypes -------------------
static void STEP_Move_Sub(uint32_t (*MoveFun)(uint32_t x), int * a, char * Response);

/**
 * \brief Print command/response with CRC attached
 * \param command
 */
static	uint8_t comm_print_cmd(char *Command){
//Use CRC in response is enabled
char buf[strlen(Command) + 6];

#if (USE_CRC > 0)
		uint16_t c = 0;
		uint8_t CRC_calc = 0;
	
		//Calculate CRC
		for (c = 0; c < CMD_BUFF_SIZE; c++){
			//Get characters up to end of string
			if(Command[c] == '\0') break; 
			//calculate CRC
			crc8(&CRC_calc, Command[c]);
			//Debug CRC
			//printf("%c:%x ",UART_cmd_buff[c], CRC_calc);
		}
		snprintf(buf, sizeof(buf), "%s:%02X\r\n", Command, CRC_calc);

//Use CRC in response is not enabled
#else 
		snprintf(buf, sizeof(buf), "%s\r\n", Command);
#endif
		board_serial_print(buf);
		return 1;
}

void comm_receivedByte(uint8_t received)
{
	//Blink communication LED n times
	LED_Blink(LED_BLUE, 5);
	
	if(received=='\n'){
		UART_cmdReceived=uart_pointer;
		uart_pointer=0;
	}
	else
	if ((uart_pointer<20)&&(UART_cmdReceived==0)&&(received>=' ')){
		UART_cmd_buff[uart_pointer++]=received;
	}
}

void communication_callback(void){
	int a = 0, CRC_received = 0;
	uint16_t CmdLength = 0, c = 0;
	uint8_t CRC_calc = 0;
	char Response[CMD_BUFF_SIZE];

	if(UART_cmdReceived > 0){
		//Store Value
		CmdLength = UART_cmdReceived;

		// Clear command string received flag
		UART_cmdReceived = 0;
	
		//Calculate CRC
		for (c = 0; c < CmdLength; c++){
			//Get characters up to CRC_SEPARATOR
			if(UART_cmd_buff[c] == CRC_SEPARATOR) break;
			//calculate CRC
			crc8(&CRC_calc, UART_cmd_buff[c]);
		}
		//Debug CRC
		//printf("CRC_calc:%X\r\n", CRC_calc);
		
		//Continue if CRC is received and is correct or CRC check is disabled
		if(((sscanf((void*)&UART_cmd_buff[c + 1],"%X",&CRC_received) == 1) && (CRC_received == CRC_calc)) ||
			(USE_CRC < 2)){
			/* **********************************************************************
			 *													General
			************************************************************************/
			//------------------------- Get serial (uC ID) -------------------------------------
			if cmd_is(UART_CMD_GetSerial){
				uint8_t c;
				char CRC_str[3] = {1,1,0};
				//Send Device ID as response
				sprintf(Response, "OK,");
				for (c = 0; c < 12; c++){
					//sprintf(CRC_str, "%02X", (uint8_t) boot_signature_byte_get(0x3 + c));
					strncat(Response, CRC_str, 2);
				}
				comm_print_cmd(Response);
			}		
			else
			//------------------------- Get Version ----------------------------------------	
			if cmd_is(UART_CMD_GetVersion){
				sprintf(Response, CMD_VERSION);
				comm_print_cmd(Response);
			}
			/* **********************************************************************
			 *													BLDC
			************************************************************************/
			else
			//------------------------- Set RPM ----------------------------------------
			if cmd_is(UART_CMD_BLDCSetRPM){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_BLDCSetRPM)-1],",%d",&a)==1){
					if (BLDC_setRPM(a) == 1)
						sprintf(Response, "OK,%d",a);
					else 
						sprintf(Response, "ERR,%d", CMD_INVALID_STATE);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get RPM ----------------------------------------
			if cmd_is(UART_CMD_BLDCGetRPM){
					sprintf(Response, "OK,%d",BLDC_getRPM());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set PWM ----------------------------------------
			if cmd_is(UART_CMD_BLDCSetPWM){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_BLDCSetPWM)-1],",%d",&a)==1){
					if (a > 1000)
						sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					else {	
						if (BLDC_setPWM(a) == 1)
							sprintf(Response, "OK,%d",a);
						else 
							sprintf(Response, "ERR,%d", CMD_INVALID_STATE);
					}
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get PWM ----------------------------------------
			if cmd_is(UART_CMD_BLDCGetPWM){
					sprintf(Response, "OK,%d",BLDC_getPWM());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set StartupPWM ----------------------------------------
			if cmd_is(UART_CMD_BLDCSetStartPWM){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_BLDCSetStartPWM)-1],",%d",&a)==1){
					if (BLDC_setStartupPWM(a) == 1)
						sprintf(Response, "OK,%d",a);
					else 
						sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get StartupPWM ----------------------------------------
			if cmd_is(UART_CMD_BLDCGetStartPWM){
					sprintf(Response, "OK,%d",BLDC_getStartupPWM());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set Slope ----------------------------------------
			if cmd_is(UART_CMD_BLDCSetSlope){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_BLDCSetSlope)-1],",%d",&a)==1){
					BLDC_setSlope(a);
					sprintf(Response, "OK,%d",a);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get Slope ----------------------------------------
			if cmd_is(UART_CMD_BLDCGetSlope){
					sprintf(Response, "OK,%d",BLDC_getSlope());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set Intercept ----------------------------------------
			if cmd_is(UART_CMD_BLDCSetIntercept){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_BLDCSetIntercept)-1],",%d",&a)==1){
					BLDC_setIntercept(a);
					sprintf(Response, "OK,%d",a);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get Intercept ----------------------------------------
			if cmd_is(UART_CMD_BLDCGetIntercept){
					sprintf(Response, "OK,%d",BLDC_getIntercept());
					comm_print_cmd(Response);
			}
			else
			//------------------------- BLDC ON ----------------------------------------		
			if cmd_is(UART_CMD_BLDCon){
					XYEnabled = 0;
					BLDC_powerOn();
					sprintf(Response, "OK,%d", CMD_OK);
					comm_print_cmd(Response);
			}	
			else
			//------------------------- BLDC OFF ----------------------------------------	
			if cmd_is(UART_CMD_BLDCoff){
					XYEnabled = 1;
					BLDC_powerOff();
					comm_print_cmd("OK,0");
			}			
			else
			//------------------------- Set Motor Direction ----------------------------------------		
			if cmd_is(UART_CMD_SetDIR){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_SetDIR)-1],",%d",&a)==1){
					switch (a){
						case 0:
							BLDC_SetDirection(BLDC_COUNTER_CLOCKWISE);
							sprintf(Response, "OK,%d", a);
							comm_print_cmd(Response);
							break;
						case 1:
							BLDC_SetDirection(BLDC_CLOCKWISE);
							sprintf(Response, "OK,%d", a);
							comm_print_cmd(Response);
							break;
						default:
							sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
							comm_print_cmd(Response);
						break;
					}
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}	
			else
			//------------------------- Get Motor Direction ----------------------------------------	
			if cmd_is(UART_CMD_GetDIR){
					sprintf(Response, "OK,%d", BLDC_GetDirection());
					comm_print_cmd(Response);
			}			
				
			/* **********************************************************************
			 *													STEP
			************************************************************************/
			else
			//------------------------- Set freq ----------------------------------------
			if cmd_is(UART_CMD_STEPSetFreq){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPSetFreq)-1],",%d",&a)==1){
					STEP_setFreq(a);
					sprintf(Response, "OK,%d",a);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get freq ----------------------------------------
			if cmd_is(UART_CMD_STEPGetFreq){
					sprintf(Response, "OK,%d",STEP_getFreq());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set Steps per Dist ----------------------------------------
			if cmd_is(UART_CMD_STEPSetStepsPDist){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPSetStepsPDist)-1],",%d",&a)==1){
					STEP_setStepPDist(a);
					sprintf(Response, "OK,%d",a);
					comm_print_cmd(Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get Steps per Dist ----------------------------------------
			if cmd_is(UART_CMD_STEPGetStepsPDist){
					sprintf(Response, "OK,%d",STEP_getStepPDist());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Set microstepping mode ----------------------------------------
			if cmd_is(UART_CMD_STEPSetMicro){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPSetMicro)-1],",%d",&a)==1){
					if (a <= STEP_SIXTEENTH){
						STEP_MicroSet((STEP_MicroModeTypeDef)a);
						sprintf(Response, "OK,%d", a);
						comm_print_cmd(Response);
					} else {
						sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
						comm_print_cmd(Response);
					}
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get microstepping mode ----------------------------------------
			if cmd_is(UART_CMD_STEPGetMicro){
					sprintf(Response, "OK,%d",(uint8_t)STEP_MicroGet());
					comm_print_cmd(Response);
			}
			else
			//------------------------- Move UP ----------------------------------------
			if cmd_is(UART_CMD_STEPMoveUp){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPMoveUp)-1],",%d",&a)==1){
					STEP_SetDirection(STEP_CLOCKWISE);
					STEP_Move_Sub(STEP_Move, &a, Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Move DOWN ----------------------------------------
			if cmd_is(UART_CMD_STEPMoveDown){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPMoveDown)-1],",%d",&a)==1){
					STEP_SetDirection(STEP_COUNTER_CLOCKWISE);
					STEP_Move_Sub(STEP_Move, &a, Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Move UP Dist ----------------------------------------
			if cmd_is(UART_CMD_STEPMoveUpDist){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPMoveUpDist)-1],",%d",&a)==1){
					STEP_SetDirection(STEP_CLOCKWISE);
					STEP_Move_Sub(STEP_MoveDist, &a, Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Move DOWN Dist ----------------------------------------
			if cmd_is(UART_CMD_STEPMoveDownDist){
				if(sscanf((void*)&UART_cmd_buff[sizeof(UART_CMD_STEPMoveDownDist)-1],",%d",&a)==1){
					STEP_SetDirection(STEP_COUNTER_CLOCKWISE);
					STEP_Move_Sub(STEP_MoveDist, &a, Response);
				}
				else
				{
					sprintf(Response, "ERR,%d", CMD_INVALID_PARAMETER);
					comm_print_cmd(Response);
				}
			}
			else
			//------------------------- Get Status ----------------------------------------
			if cmd_is(UART_CMD_STEPGetStatus){
				STEP_DirectionTypeDef Direction;
				uint32_t Steps_moved, Steps_requested;
				STEP_getStatus(&Direction, &Steps_moved, &Steps_requested);
				sprintf(Response, "OK,%d,%d,%d", (uint8_t)Direction, Steps_moved, Steps_requested);
				comm_print_cmd(Response);
			}
			
			else
			//------------------------- STEP ON ----------------------------------------		
			if cmd_is(UART_CMD_STEPon){
					XYEnabled = 0;
					STEP_enableOn();
					STEP_sleepOff();
					comm_print_cmd("OK,0");
			}	
			else
			//------------------------- STEP OFF ----------------------------------------	
			if cmd_is(UART_CMD_STEPoff){
					XYEnabled = 1;
					STEP_enableOff();
					comm_print_cmd("OK,0");
			}			
			else
			//------------------------- STEP Sleep ON ----------------------------------------		
			if cmd_is(UART_CMD_STEPSleepOn){
					STEP_sleepOn();
					comm_print_cmd("OK,0");
			}	
			else
			//------------------------- STEP Sleep OFF ----------------------------------------	
			if cmd_is(UART_CMD_STEPSleepOff){
					STEP_sleepOff();
					comm_print_cmd("OK,0");
			}
			else
			//------------------------- STEP Get Sleep Status ----------------------------------------	
			if cmd_is(UART_CMD_STEPGetSleep){
					sprintf(Response, "OK,%d", STEP_getSleep());
					comm_print_cmd(Response);
			}			
			/* **********************************************************************
			 *													Debug
			************************************************************************/
			else
			//------------------------- Print out FG timer registers ----------------------------------------		
			if cmd_is(UART_CMD_GETTIM17){
				comm_print_cmd("OK,0");
				/*
				printf("CR1: %04x\r\n", TIM17->CR1);
				printf("CR2: %04x\r\n", TIM17->CR2);
				printf("SMCR: %04x\r\n", TIM17->SMCR);
				printf("DIER: %04x\r\n", TIM17->DIER);
				printf("SR: %04x\r\n", TIM17->SR);
				printf("EGR: %04x\r\n", TIM17->EGR);
				printf("CCMR1: %04x\r\n", TIM17->CCMR1);
				printf("CCMR2: %04x\r\n", TIM17->CCMR2);
				printf("CCER: %04x\r\n", TIM17->CCER);
				printf("CNT: %04x\r\n", TIM17->EGR);
				printf("PSC: %04x\r\n", TIM17->PSC);
				printf("ARR: %04x\r\n", TIM17->ARR);
				printf("RCR: %04x\r\n", TIM17->RCR);
				printf("CCR1: %04x\r\n", TIM17->CCR1);
				printf("CCR2: %04x\r\n", TIM17->CCR2);
				printf("CCR3: %04x\r\n", TIM17->CCR3);
				printf("CCR4: %04x\r\n", TIM17->CCR4);
				printf("BDTR: %04x\r\n", TIM17->BDTR);
				printf("DCR: %04x\r\n", TIM17->DCR);
				printf("DMAR: %04x\r\n", TIM17->DMAR);
				printf("OR: %04x\r\n", TIM17->OR);
				*/
			}
			else {
				//Unrecognized command
				sprintf(Response, "ERR,%d", CMD_UNRECOGNIZED_CMD);
				comm_print_cmd(Response);
			}

		}	else {
				//Invalid CRC
				sprintf(Response, "ERR,%d", CMD_INVALID_CRC);
				comm_print_cmd(Response);
		}
		
		//clear buffer
		memset((void*)UART_cmd_buff,0,sizeof(UART_cmd_buff));
	}
}


/**
 * \brief Helper function for StepperMove implementation
 * \param MoveFun - pointer to required move function
 * \param a - pointer to received paramter 
 * \param Response - pointer to Response string
 */
static void STEP_Move_Sub(uint32_t (*MoveFun)(uint32_t x), int * a, char * Response){
		int request = *a; //save requested value
	
		*a = MoveFun(*a);
		//If no steps were requested OR steps requested and motor moved
		if ((request == 0) || (*a != 0)) 
			sprintf(Response, "OK,%d",*a);
		//Steps were requested, but invalid state was returned
		else 
			sprintf(Response, "ERR,%d", CMD_INVALID_STATE);
		
		comm_print_cmd(Response);
}
