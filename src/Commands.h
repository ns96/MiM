	#ifndef _COMMANDS_H_
	#define _COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif	
	typedef enum {
		CMD_OK,
		CMD_UNRECOGNIZED_CMD,
		CMD_INVALID_PARAMETER,
		CMD_INVALID_CRC,
		CMD_INVALID_STATE,
	} CMD_StatusTypedef;
#ifdef __cplusplus
}
#endif
	
	#define CMD_VERSION		"MIM 1.0.0"	//Version string
	
	#define CRC_SEPARATOR	':' //This character separates CRC data from command
	#define CMD_BUFF_SIZE	64  //Size of buffers of received and printed commands (currently print serial is longest response)
	
	/* Use CRC in protocol. Valid parameters:
			0	- OFF: do not use CRC
			1 - IGNORE: allows received commands without CRC, but displays CRC in response
			2 - ON: full use of CRC
	*/
	#define	USE_CRC		1 		
	
		/* Command formats:
				Command,Parameter:CRC\r\n
				or
				Command:CRC\r\n
			
			Response format:
				Response,Status:CRC
				
				Response can be:
					OK,Status:CRC
					ERR,Status:CRC
						Status codes: (see CMD_StatusTypedef definition)
					
			CRC is 8 bit length value displayed in alphanumerical Hex format in uppercase characters
			
			Examples:
				STEPon:2C
					OK,0:E1
				GetRPM:98
					OK,1000:3B
				MoveDown,1000:D7
					OK,1000:3B
					
		*/
	
		/*		General			*/		
		#define UART_CMD_GetSerial 	"GetSerial"
		#define UART_CMD_GetVersion "GetVersion"
		
		/*		BLDC control			*/
		#define UART_CMD_BLDCon 					"BLDCon"
		#define UART_CMD_BLDCoff 					"BLDCoff"
		#define UART_CMD_BLDCSetRPM 			"SetRPM"
		#define UART_CMD_BLDCGetRPM 			"GetRPM"
		#define UART_CMD_BLDCSetPWM 			"SetPWM"		//PWM value: 0 - 1000
		#define UART_CMD_BLDCGetPWM 			"GetPWM"		//PWM value: 0 - 1000
		#define UART_CMD_BLDCSetStartPWM 	"SetStartPWM"
		#define UART_CMD_BLDCGetStartPWM 	"GetStartPWM"
		#define UART_CMD_BLDCSetSlope 		"SetSlope" //Set slope value multiplied by 100
		#define UART_CMD_BLDCGetSlope 		"GetSlope" //Get slope value multiplied by 100
		#define UART_CMD_BLDCSetIntercept "SetIntercept"
		#define UART_CMD_BLDCGetIntercept "GetIntercept"
		#define UART_CMD_SetDIR 					"SetDIR"
		#define UART_CMD_GetDIR 					"GetDIR"
		
		/*		STEP control			*/
		#define UART_CMD_STEPon 				"STEPon"
		#define UART_CMD_STEPoff 				"STEPoff"
		/*	Stepper status in format: Moving, StepsMoved, StepsRequested
				Moving = 0, 1, 2 (Stop, OneDirection, OppositeDirection)
				StepsMoved = 0 .. StepsRequested
				StepsRequested = 0 .. n
		*/
		#define UART_CMD_STEPGetStatus	"STEPStatus"
		#define UART_CMD_STEPSetFreq 		"SetFreq"
		#define UART_CMD_STEPGetFreq 		"GetFreq"
		#define UART_CMD_STEPSetStepsPDist 		"SetStepsPDist"
		#define UART_CMD_STEPGetStepsPDist 		"GetStepsPDist"
		/*	Microstepping parameter for SetMicro and GetMicro	
				FULL = 0
				HALF = 1
				QUARTER = 2
				EIGHTH = 3
				SIXTEENTH	= 4
		*/
		#define UART_CMD_STEPSetMicro		"SetMicro"
		#define UART_CMD_STEPGetMicro		"GetMicro"
		#define UART_CMD_STEPMoveUp			"MoveUp"
		#define UART_CMD_STEPMoveDown		"MoveDown"
		#define UART_CMD_STEPMoveUpDist			"MoveDistUp"
		#define UART_CMD_STEPMoveDownDist		"MoveDistDown"
		#define UART_CMD_STEPSleepOn 		"SleepOn"
		#define UART_CMD_STEPSleepOff 	"SleepOff"
		#define UART_CMD_STEPGetSleep		"GetSleep"
		
		/*		Debug			*/
		#define UART_CMD_GETTIM17 		"GetTIM17"
	#endif
