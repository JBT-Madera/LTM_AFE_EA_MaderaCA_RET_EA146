/*/////////////////////////////////////////////////////////////////////////////////////////////
Customer:					Efficient Agitation Production Retort

Location:					Madera, CA
Machine Type:				JBT Steam Water Spray 6 basket Retort, with static or efficient 
							agitation modes in SWS.

Date of implementation:		January 2022
Programmed By:				Michael Fystrom
Commissioned By:			Michael Fystrom
Customer Contact:			Karen Brown / Kevin Carlson

Original Version:	14.1.0.2 - Dated 12-02-2022
Current Version:	14.1.0.2 - Dated 12-02-2022

Revisions after Startup:

1.	Date of Change:
	Change made by:
	Description of Change:



*/

//////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "pinesdisplay.h"
#include "pinesio.h"
#include "Timers.h"
#include "stdlib.h"
#include "tchar.h"
#include "math.h"
#include "TCIO.h"


//////////////////////////////////////////////////////////////////////////////////////////////
//	Global Declarations
//////////////////////////////////////////////////////////////////////////////////////////////
IUnknown* pIUnknownDisplay = NULL;
IUnknown* pIUnknownIO = NULL;
IUnknown* pIUnknownDIO = NULL;
IUnknown* pIUnknownPID = NULL;
IUnknown* pIUnknownPT = NULL;
IUnknown* pIUnknownPV = NULL;
IUnknown* pIUnknownTaskloop = NULL;
IUnknown* pIUnknownProfiler = NULL;
IUnknown* pIUnknownNumerical = NULL;
IUnknown* pIUnknownBall = NULL;
IUnknown* pIUnknownTunacal = NULL;

CString csVal = L"";
CString csVar1 = L"";
CString csVar2 = L"";
CString strIT;

LPCLSID pclsidDisplay = NULL;
LPCLSID pclsidAIO = NULL;
LPCLSID pclsidDIO = NULL;
LPCLSID pclsidPID = NULL;
LPCLSID pclsidPT = NULL;
LPCLSID pclsidPV = NULL;
LPCLSID pclsidTaskloop = NULL;
LPCLSID pclsidNumerical = NULL;
LPCLSID pclsidBall = NULL;
LPCLSID pclsidTunacal = NULL;
LPCLSID pclsidProfiler = NULL;

LPDISPATCH pDispatchDisplay = NULL;
LPDISPATCH pDispatchIO = NULL;
LPDISPATCH pDispatchDIO = NULL;
LPDISPATCH pDispatchPID = NULL;
LPDISPATCH pDispatchPT = NULL;
LPDISPATCH pDispatchPV = NULL;
LPDISPATCH pDispatchTaskloop = NULL;
LPDISPATCH pDispatchNumerical = NULL;
LPDISPATCH pDispatchBall = NULL;
LPDISPATCH pDispatchTunacal = NULL;
LPDISPATCH pDispatchProfiler = NULL;

IPSLDisplay			Display;
IAnalogIO			AnalogIO;
IDigitalIO			DigitalIO;
IPIDCommands		PID;
IProcessVariable	PV;
ITaskLoop			TaskLoop;
INumericalProcess	Numerical;
IBallProcess		Ball;
ITunacalProcess		Tuna;
IProfiler			Profiler;
TCIO				tcio;

CTimers PinesTime1;	// Phase, Step, OR Segment Timer (sec)
CTimers PinesTime2;	// Operator Entry Screen Timer (sec)
CTimers PinesTime3;	//
CTimers PinesTime4;	// Flow Alarm Filter Timer
CTimers PinesTime5;	// Total ComeUp through CoolDown Time
CTimers PinesTime6;	// Rocking/Rotation Function Timer
CTimers PinesTime7;	// Water Level Filter Alarm Timer - initial
CTimers PinesTime8;	// Flow SHUTDOWN Filter Timer
CTimers PinesTime9;	// RTD's Differ Alarm Filter Timer
CTimers PinesTime10;// Water HIGH Level Filter Alarm Timer - FILTER
CTimers PinesTime11;// Water LOW Level Filter Alarm Timer - FILTER
CTimers PinesTime12;// Used to track total cooling elapsed time
CTimers PinesTime13;// Used in Unloading
CTimers PinesTime14;// Used in Comeup and & Cool for Pressure only
CTimers PinesTime15;// Air Supply Pressure Alarm Delay Timer
CTimers PinesTime16;// Alarm Horn Disable "Acknowledged" Timer
CTimers PinesTime17;// Cook PSI Come Up
CTimers PinesTime18;// Cook low temp timer
CTimers PinesTime19;// Reel RPM Timer
CTimers PinesTime20;// Cool flow debounce time
CTimers PinesTime21;// In case of PSI halt time the hlat
CTimers PinesTime22;// Cool hold time
CTimers PinesTime23;// FI long preheat time
CTimers PinesTime24;// Acid Wash Timer
CTimers PinesTime33;// Chiller water supply control
CTimers PinesTime34;// Steam supply pressure debounce timer
CTimers PinesTime35;
CTimers PinesTime36;
CTimers PinesTime37;
CTimers PinesTime38;

//Declaration of LogStatus. Normal is the only status not displayed on the LogStatus column
enum LogStatus
{
	NORMAL = 1,
	DEVIATION = 2,
	LOGERROR = 3,
	ABORT = 4,
	OVERRIDE = 5,
	MANUAL = 6
};
LogStatus	SendLogStatus;

//Declaration of the different types of recipes available
enum RecipeTypeDescription
{
	NONE = -1,
	BALL = 0,
	GENERIC = 1,
	NUMERICAL = 2,
	TUNACAL = 3
};

RecipeTypeDescription RecipeType;

typedef enum {
	LoadingLabel = 1,
	PreHeatLabel = 2,
	CookLabel = 3,
	CoolingLabel = 4,
	ComeUpLabel = 5,
	DrainLabel = 6,
	ShutdownLabel = 7,
	EndProcessLabel = 8,
	StartLabel = 10,
	AddCookTimeLabel = 11,
	HydroLabel = 13,
	UnLoadingLabel = 14,
	FillLabel = 15,
	OpEntryLabel = 16,
	HydroPreheatLabel = 17,
	SensorCheckLabel = 18,
	FullDrainLabel = 19,
	HotRestartLabel = 20
}	OverrideLabel;

OverrideLabel OverrideSelection;

#define GoTo(OverrideSelection) {switch (OverrideSelection) {\
					case LoadingLabel:		goto LoadingLabel;\
					case CookLabel:			goto CookLabel;\
					case CoolingLabel:		goto CoolingLabel;\
					case ComeUpLabel:		goto ComeUpLabel;\
					case DrainLabel:		goto DrainLabel;\
					case FullDrainLabel:	goto FullDrainLabel;\
					case ShutdownLabel:		goto ShutdownLabel;\
					case EndProcessLabel:	goto EndProcessLabel;\
					case StartLabel:		goto StartLabel;\
					case UnLoadingLabel:	goto UnLoadingLabel;\
					case OpEntryLabel:		goto OpEntryLabel;\
					case HotRestartLabel:	goto HotRestartLabel;\
					case SensorCheckLabel:	goto SensorCheckLabel;}}

/***************************************Real IO**************************************/

/*==================== Definition of Analog Input (AI) Channels ====================*/
#define AI_RTD1						1		/*AI 1 */
#define AI_RTD2						2		/*AI 2 */
#define AI_PREHT_RTD				3		/*AI 3 */
#define AI_PRESSURE					4		/*AI 4 */	
#define AI_LEVEL					5		/*AI 5 */
#define AI_FLOW						6		/*AI 6 */
#define AI_ACCELEROMETER			7		/*AI 7 */
#define AI_EA_CYL_POS				8		/*AI 8 */ // This Input is being used just by the PLC for "EA Travel Acelerometer 

#define AI_HS_PRESS_1				9		/*AI 9 */	// Pressure sensors for trending behavior of vessel 
#define AI_HS_PRESS_2				10		/*AI 10 */	
#define AI_HS_PRESS_3				11		/*AI 11 */				
#define AI_HS_PRESS_4				12		/*AI 12 */

/*==================== Definition of Analog Output (AO) Channels ==================*/
#define AO_STEAM					1		/*AO 1 */
#define AO_VENT						2		/*AO 2 */
#define AO_AIR						3		/*AO 3 */
#define AO_WATER					4		/*AO 4 */
#define AO_SPLT_RNG					5		/*AO 5 */
#define	AO_SPEED_CHART				6		/*AO 6 */	// Todo: This could be used for EA trending
//#define AO_SPARE					7		/*AO 7 */
//#define AO_SPARE					8		/*AO 8 */

//#define AO_RESERVED				9		/*AO 9 */ // This Input is being used just by the PLC for "EA Proportional Valve"
//#define AO_SPARE					10		/*AO 10 */
//#define 					11		/*AO 11 */

/*==================== Definition of Digital Input (DI) Channels ==================*/
#define DI_ESTOP_SR					1		/*DI 1 */	// Safety relay status for pilot
#define DI_DOOR_CLOSED_SR			2		/*DI 2 */	// door closed - Todo: need to update logic for production
#define DI_DOOR_PIN_NOT_EXT			3		/*DI 3 */	// lockpin not extended - Todo: need to update logic for production
#define DI_ZERO_PRS					4		/*DI 4 */
//#define 			5		/*DI 5 */
//#define DI_LOCKPIN_ENGAGED			6		/*DI 6 */
#define DI_AIR_OK					7		/*DI 7 */
#define DI_STEAM_OK					8		/*DI 8 */
#define DI_WATER_OK					9		/*DI 9 */
#define DI_JOG_CCW_SW				10		/*DI 10 */	// logic handled in twinCAT
#define DI_JOG_CW_SW				11		/*DI 11 */
//#define 				12		/*DI 12 */
//#define DI_SPARE					13		/*DI 13 */	
#define DI_DRV_OL_OK				14		/*DI 14 */
#define UPS_ALARM_OK				15		/*DI 15 */	
#define ALARM_ACK_PB				16		/*DI 16 */

//#define 			17		/*DI 17 */	
//#define 			18		/*DI 18 */
//#define 			19		/*DI 19 */
//#define DI_SPARE					20		/*DI 20 */
//#define DI_SPARE					21		/*DI 21 */
//#define DI_SPARE					22		/*DI 22 */
#define DI_CIRC_PUMP_OL_OK_1		23		/*DI 23 */
#define DI_CIRC_PUMP_OL_OK_2		24		/*DI 24 */
//#define DI_SPARE					25		/*DI 25 */
//#define DI_SPARE					26		/*DI 26 */
//#define DI_SPARE					27		/*DI 27 */
//#define DI_SPARE					28		/*DI 28 */
//#define DI_SPARE					29		/*DI 29 */
//#define DI_SPARE					30		/*DI 30 */
//#define DI_SPARE					31		/*DI 31 */
//#define DI_SPARE					32		/*DI 32 */


/*=================== Definition of Digital Output (DO) Channels ==================*/
// FESTO VALVE BANK
#define DO_COND_VLV					1		/*DO 1 */	// VLV01A
#define DO_DRAIN_VLV				2		/*DO 2 */	// VLV13A
#define DO_STEAM_BLK_VLV			3		/*DO 3 */	// VLV02A
#define DO_STEAM_BYPASS_VLV			4		/*DO 4 */	// VLV02B
#define DO_PREHEAT_VLV				5		/*DO 5 */	// VLV03A
#define DO_WATER_FILL_VLV			6		/*DO 6 */	// VLV03B
#define DO_HE_BYPASS_OPEN			7		/*DO 7 */	// VLV07A - this is on for cooling
#define DO_HE_BYPASS_CLOSE			8		/*DO 8 */	// VLV07B - this is on for all other phases
//#define spare						9		/*DO 9 */
//#define spare						10		/*DO 10 */
#define DO_PUMP_1					11		/*DO 11 */
#define DO_PUMP_2					12		/*DO 12 */
//#define DO_BRAKE_REL				13		/*DO 13 */
#define DO_SFTY_RLY_RST				14		/*DO 14 */
#define LOGTECAlarm					15		/*DO 15 */			
//#define spare						16		/*DO 16 */
//17-32 spares


/***************************************Virtual IO (Communication Variables)**************************************/
/*============================== Virtual Analog Input (VAI) used to write to the PLC==============================*/

/*============================== Virtual Analog Output (VAO) used to write to the PLC=============================*/

#define VAO_EA_MotionProfile		19		/*AO 19 */	// Recipe index for the motion profile 
#define VAO_LTM_Phase				20		/*AO 20 */	// Phase of LTM script 1=loading,2=op entry,3=fill,4=pre-heat,5=come-up,6=cook,7=cool,8=drain,9=unloading,10=shutdown,11=full drain,12=cleaning,13=greetings
#define VAO_CycleNumber				21		/*AO 21*/   // LTM sending cycle number to help format Batch Code v.17 RMH

/*============================== Virtual Digital Input (VDI) used to write to the PLC=============================*/

#define VDI_StopEA_Ack				33		/*DI 33*/	// PLC acknowledged request to stop efficient agitation. This is used to update the motion profile when chaging from one phase to the next one.

/*============================== Virtual Digital Output (VDO) used to write to the PLC=============================*/

#define VDO_Shutdown				33		/*DO 33*/	
#define VDO_EndOfProcess			34		/*DO 34*/	
#define VDO_AgitationReq			35		/*DO 35*/	// LTM requesting agitation for the process
#define VDO_StopEA_Req				36		/*DO 36*/	// LTM requesting stop efficient agitation

/*===============================================================================*/

#define	True						1
#define	False						0
#define Onn							1
#define Off							0

// Constant Variables
#define	flFinalTemperatureLimit		120.0	// Temperature at which the door can be opened
#define flFinalPressureLimit		1.0		// Pressure at which the door can be opened	
/*==================== Definition of Programable Flags ===================*/
bool	bSensorCheckCompleteFlag = false,
bLoadingCompleteFlag = false,
bOpEntryCompleteFlag = true,
bOpEntryRunning = false,
bPreHeatTaskCompleteFlag = false,
bPreheatRunning = false,
bFillCompleteFlag = false,
bFillRunning = false,
bCookTimeCompleted = false,
bCookCompleteFlag = true,
bCookRunning = false,
bCoolPSIPhaseCompleteFlag = true,
bDrainCompleteFlag = false,
bDrainRunning = false,
bUnLoadingCompleteFlag = false,
bUnloadRunning = false,
bComeUpPSIStart = false,
bComeUpTimerRunning = false,
bFullDrainFlag = false,
bEarlyDrainFlag = false,	// Early Drain Flag
bSplitRangeCompleteFlag = true,
bSplitRangeRunning = false,
bCUTempCompleteFlag = true,
bCUTempRunningFlag = false,
bCUPSICompleteFlag = false,
bSegmentCompleteFlag = false,
bKillMajorProblemChecks = false,
bMajorPBChecksRunning = false,
bMiscTasksRunning = false,
bCycleRunning = false,
bPhaseCompleteFlag = true,	 // Always initialized true - first time.
bKillTheTaskFlag = false,
bKillCUPSITaskFlag = false,
bKillCoolPSITaskFlag = false,
bFillPhaseCompleteFlag = false,
bKillRotationTaskFlag = false,
bKillSplitRangeFlag = false,
bInitiateLowMigDev = false,
bAgitatingProcessFlag = false,
bBeginAgitationFlag = false,
bDisplayDeviationInfo = false,
bTempDeviationFlag = false,
bReadyForProcessFlag = false,
bCUPSIPhaseCompleteFlag = true,
bComeUpPSIRunning = false,
bProcessDeviationFlag = false,
bRecipeSelectionFlag = false,
bCoolDelayHaltPSIRampFlag = false,
bShutDownFlag = false,
bNoMoreValidProcessFlag = false,
bDoDeviationCheck = false,
bRunLevelControlFlag = false,
bFillLevelAdjustFlag = false,
bLowHeatingFlowFlag = false,
bLowCoolFlowFlag = false,
bRetort_PresFailFlag = false,
bRetort_TempFailFlag = false,
bAddCookTimeFlag = false,
bCoolTempCompleteFlag = false,
bCoolTempRunning = false,
bCoolPSICompleteFlag = false,
bNeedToRestartFlag = false,
bHighWaterLevel = false,
bDoor1SecuredMode = false,
bEnblCoolWaterFlow = false,
bAirAlmTestFlag = false,
bUpdateToCorrectRTD = false,
bEndLevelControlFlag = false,
bDoorLockFailureFlag = false,
bResetTimerFlag = false,	// reset timer # 14 during Cool PSI if we have temp problems cooling
bCriticalPSIFailureFlag = false,	// reset flag for critical vapor pressure alarm
bCheckCriticalPSIFlag = false,	// control flag for whebn do we check vapor pressure
bLowAirTrip = false,
bOKtoStartCoolingPSI = false,	// OK to start Cooling PSI
bProcessStarted = false,
bCook_PresFailFlag = false,
bCook_PressWarnFlag = false,
bCheckForMinPSIFlag = false,	// Checking for minimum pressure requirement ONLY in Cook
bVerifyYNFlag = false,
bInitFillWaitDone = false,
bJumpOutToDrainFlag = false,
bTestOneTC_Variable = false,
bKillTheMiscTaskLoop = false,
bProblemWithDataEntryFlag = false,
bVerifyExitEntry = false,
bChartVerified = false,
bVerifyYN = false,
bProcessStartedFlag = false,
bHighITProcessFlag = false,
bPreHeatOn = false,
bAlarmExists = false,	// For flashing of the alarm ack light
bE_StopPushed = false,
bSafety1RlyTrip = false,
bSafety1RlyRst = false,
bSafety2RlyTrip = false,
bSafety2RlyRST = false,
bSWSOnlyRecipeFlag = false,
bAutoClaveRecipeFlag = false,
bSteamOnlyRecipeFlag = false,
bHydroOnlyRecipeFlag = false,
bFullImmersionRecipeFlag = false,
bKillHydroMotion = false,
bHyPHPhaseCompleteFlag = false,
bInvertBasket = false,
bCookWaterFlow = false,
bRPMCalledFirst = false,
bDisplaysSet = false,
bFI_SprayCool = false,
//Hot restart flags
bVerifyPromptFlag = false,
bVerifySelectionYN = false,
bHOT_RestartFlag = false,
bFlashVentRecipeFlag = false, //LK
bFlashVentCoolCompleteFlag = false,
bAirCoolingCompleteFlag = false,	// ver 11 MDF
bAirCoolTestingFlag = false,	// ver 11 MDF
bOverrideSelectionComplete = false,
bCleaningRecipeSelected = false,
bNeutralizingTimerStarted = false,
bNeutralizeComplete = false,
bEA_Selected = false,
bTriggerEAChange = false,
bEndEAAgitation = false, // added ver 10 MDF
bEndRotation = false;


//=================Local Tags Made Global for iOPS==================//
bool bPSIAboveSetPointFlag = false,
bAI_PREHT_RTDFailFlag = false,
bRTD1HighTempFailFlag = false,
bRTD1FailedFlag = false,
bLowMIGEntryFlag = false,
bLongVentZeroTimeFlag = false,
bLongDrainTimeFlag = false,
bDifferentialPSIFromCookToCool = false,
bLongCoolingTimeFlag = false,
bTemperatureDropInOvershoot = false,
bLongComeUpTimeFlag = false,
bLongPressureTimeFlag = false,
bLongFillAlarm = false,
bRPMFailFlag = false,
/*------------------------v.16 RMH-----------------------------------*/
bComeUpTempStep1 = false,
bComeUpTempStep2 = false,
bComeUpTempStep3 = false,
bComeUpTempStep4 = false,
bComeUpTempStep5 = false,
bHotPSIStepA = false,
bHotPSIStepB = false,
bHotPSIStepC = false,
bHotPSIStepD = false,
bHotPSIStepE = false,
bHotPSIStepF = false,
bCOOLTempStep1 = false,
bCOOLTempStep2 = false,
bCOOLTempStep3 = false,
bCOOLTempStep4 = false,
bCOOLPSIStepA = false,
bCOOLPSIStepB = false,
bCOOLPSIStepC = false,
bCOOLPSIStepD = false,
bCOOLPSIStepE = false,
/*------------------------------------------------------------------*/
bInIdleStart = false,
bInIdleStop = false,
bInProcessStart = false,
bInProcessStop = false,
bPumpRunStart = false,
bPumpRunStop = false,
bMotorRunStart = false,
bMotorRunStop = false;
//=================================================================//

int		RETORT_TEMP = AI_RTD1,
nCorrectTuningTable = 0,
iCurrentHour = 0,
iProductNumber = 0,
iProductType = 0;




/*============== Definition of INTERNAL Variables (NOT PV's Directly) Related Variables ================*/
float	flLowCookTemperatureTime = 0.0, // declaration of Low Cook Temp Time variable
flCookLowestTemp = 0.0,
flCurrentTemperature = 0.0,
flCurrentTemp2 = 0.0,
flCurrentPreHeatTemp = 0.0,
flCoolingSourceTemp = 0.0,
flRPMPctSetpoint = 0.0,
flCurrentPressure = 0.0,
flHighestCUPress = 0.0,
flHighestCUTemp = 0.0,
flDisplayLoopTempSP = 0.0,	// Used to display current Temp Setpoint
flDisplayLoopPsiSP = 0.0,	// Used to display current PSI Setpoint
flAvgReelSpeed = 0.0,
flReelStaticPosition = 0.0,	// Reel Postion for Static use
flActualReelPosition = 0.0,
flTotalCycleTime = 0.0,
flMinimumIT = 32.0,
flExitTemperature = 0.0,
flMinGenIT = 32.0,
flMaxGenIT = 180.0,
flHighestITTemp = 0.0,
flDrainLevel = 0.0,
flFullDrainLevel = 4.0,	// Full Drain Level for Over Ride situation
flWaterHighFlowRate = 0.0,
flHeatingWaterLowFlowRate = 0.0,
flCoolWaterLowFlowRate = 0.0,
flCookWaterShutDownFlowRate = 0.0,
flWaterHighLevelAlarm = 0.0,
flWaterLowLevelAlarm = 0.0,
flWaterDrainOnLevel = 0.0,
flWaterDrainOffLevel = 0.0,
flCoolingAcumulatedTime = 0.0,	// starts at zero every time we start cool and adds the time while we are cooling
flCoolTempTotalTime = 0.0,	// Cool Phase Total Time
flMinProcessTime = 0.0,	// Minimum Process Time (min)
flMaxVentValveOutput = 0.0,	// Limit for Valve position %
flMaxAirValveOutput = 0.0,	// Limit for Valve position %
flZeroValueCrossOver = 0.0,	// Point for both Valves CLOSED position %
flDeadBandSetting = 0.0,
flMinimumOutputBias = 0.0,
flCurrentTempSetpoint = 0.0,
flLongFillTime = 0.0,
flLongSegmentTime = 0.0,
flLongDelayTime = 0.0,
flCTIMEAddedAccumSec = 0.0,
flMinVentTime = 0.0,
flMinVentTemp = 0.0,
flRampTime = 0.0,
flInitialTemperature = 0.0,
flPreHeatTemp = 0.0,
flNewCTEMP = 0.0,
flCTEMP = 0.0,
flMinimumCookTime = 0.0,
flCTIME = 0.0,
flRTintervals = 0.0,
flCurrentWaterLevel = 0.0,
flCurrentWaterFlow = 0.0,
flTimeLeftInSegment = 0.0,
flMIGTemperature = 0.0,
flRockRight = 0.0,
flRockLeft = 0.0,
flMotionProfile = 0.0,
flNewMP = 0.0,
flSpeedTemp[100];

long	lTemp = 0,	// Used for application overrides
lCurStepSecSP = 0,	// Used to display current Step Time Setpoint
lCurStepSecRemain = 0,	// Used to display current Step Time Remaining
lCycleNumber = 0,
lAirCoolingTime = 0,	// 
lPreRecipeLevel = 5,	// Level of water prior to choosing a recipe
lProcVar_RecipeType = 0;      //  Recipe Type Verification 0 = Ball, 1 = Generic and 2 =  NumeriCAL"

// ************************** PV variable declaration from HOST RECIPE PV TABLE ***********************
//									For SWS recipes
float	flInitialFillLevel = 0.0,     // PV1  Initial Fill Level
flInitialTempOffset = 0.0,     // PV2  Temperature bias for target temperature
flInitialPressure = 0.0,     // PV3  Initial vessel target pressure
flComeUpTempStep1 = 0.0,     // PV4  1st Comeup segment target temperature
flComeUpTempTime1 = 0.0,     // PV5  1st Comeup segment minimum time
flComeUpTempStep2 = 0.0,     // PV6  2nd Comeup segment target temperature
flComeUpTempTime2 = 0.0,     // PV7  2nd Comeup segment minimum time
flComeUpTempStep3 = 0.0,     // PV8  3rd Comeup segment target temperature
flComeUpTempTime3 = 0.0,     // PV9  3rd Comeup segment minimum time
flComeUpTempStep4 = 0.0,     // PV10 4th Comeup segment target temperature
flComeUpTempTime4 = 0.0,     // PV11 4th Comeup segment minimum time
flComeUpTempStep5 = 0.0,     // PV12 5th Comeup segment target temperature
flComeUpTempTime5 = 0.0,     // PV13 5th Comeup segment minimum time
flSteamBoostOff	 = 0.0,     // PV14 Temperature the Steam Boost turns off durring Come up"
flHotPSIStepA = 0.0,     // PV15 1st Hot segment target pressure
flHotPSITimeA = 0.0,     // PV16 1st Hot segment minimum time
flHotPSIStepB = 0.0,     // PV17 2nd Hot segment target pressure
flHotPSITimeB = 0.0,     // PV18 2nd Hot segment minimum time
flHotPSIStepC = 0.0,     // PV19 3rd Hot segment target pressure
flHotPSITimeC = 0.0,     // PV20 3rd Hot segment minimum time
flHotPSIStepD = 0.0,     // PV21 4th Hot segment target pressure
flHotPSITimeD = 0.0,     // PV22 4th Hot segment minimum time
flHotPSIStepE = 0.0,     // PV23 5th Hot segment target pressure
flHotPSITimeE = 0.0,     // PV24 5th Hot segment minimum time
flHotPSIStepF = 0.0,     // PV25 6th Hot segment target pressure
flHotPSITimeF = 0.0,     // PV26 6th Hot segment minimum time
flMinimumCookPSI = 0.0,     // PV27 Minimum cook pressure for alarm & deviation
flOverShootTime = 0.0,     // PV28 Overshoot duration before going into Cook
flRampDownToCTEMPTime = 0.0,     // PV29 Ramp duration from Overshoot to Cook
flCookStabilizationTime = 0.0,     // PV30 Cook stabilization time
flMicroCoolTime = 0.0,     // PV32 Micro Cool time
flCOOLTempStep1 = 0.0,     // PV33 1st Cool segment target temperature
flCOOLTempTime1 = 0.0,     // PV34 1st Cool segment minimum time
flCOOLTempStep2 = 0.0,     // PV35 2nd Cool segment target temperature
flCOOLTempTime2 = 0.0,     // PV36 2nd Cool segment minimum time
flCOOLTempStep3 = 0.0,     // PV37 3rd Cool segment target temperature
flCOOLTempTime3 = 0.0,     // PV38 3rd Cool segment minimum time
flCOOLTempStep4 = 0.0,     // PV39 4th Cool segment target temperature
flCOOLTempTime4 = 0.0,     // PV40 4th Cool segment minimum time
flCOOLTempHoldTime = 0.0,     // PV41 Last Cool segment minimum hold time
flCOOLPSIStepA = 0.0,     // PV42 1st Cool segment target pressure
flCOOLPSITimeA = 0.0,     // PV43 1st Cool segment minimum time
flCOOLPSIStepB = 0.0,     // PV44 2nd Cool segment target pressure
flCOOLPSITimeB = 0.0,     // PV45 2nd Cool segment minimum time
flCOOLPSIStepC = 0.0,     // PV46 3rd Cool segment target pressure
flCOOLPSITimeC = 0.0,     // PV47 3rd Cool segment minimum time
flCOOLPSIStepD = 0.0,     // PV48 4th Cool segment target pressure
flCOOLPSITimeD = 0.0,     // PV49 4th Cool segment minimum time
flCOOLPSIStepE = 0.0,     // PV50 5th Cool segment target pressure
flCOOLPSITimeE = 0.0,     // PV51 5th Cool segment minimum time
flComeUpMotionProfile = 0.0,     // PV53 Motion Profile used during Come Up
flCookMotionProfile = 0.0,     // PV54 Motion Profile used during Cook
flCoolingMotionProfile = 0.0,     // PV55 Motion Profile used during Pressure Cool


flCOOLTempStep5 = 0.0,
flCOOLTempTime5 = 0.0,
flWaterAtDamLevel = 0.0,
flCoolHoldTime = 0.0,
flFeedLegInletTemp = 0.0,
flFeedLegOutletTemp = 0.0,
flFeedLegPasses = 0.0,
flSteamDomePasses = 0.0,
flFeedLegTime = 0.0,
flRampToProcessTime = 0.0,
flRampToVentTime = 0.0,
flOvershootOffset = 0.0,

//									For Air Cooling Test ver 10 MDF
flVentOpenPercentage = 0.0,
flCoolingPressureTarget = 0.0,
flWaterValveOpenPercentage = 0.0,
flWaterLevelTarget = 0.0,
flFinalCoolTempTarget = 0.0,

//									For Full Immersion Processes
flDirectCoolLevel = 0.0,
flSprayCoolProcess = 0.0,


flPSICoolInletLegTime = 0.0, // PV11 Transition time between Cook and PSI Cool
flPSICoolInletTemp = 0.0, // PV12 Temp at end of transition between Cook and PSI Cool
flPressCoolPressure = 0.0, // PV13 PSI to hold during PSI Cool
flPressCoolTime = 0.0, // PV14 Time in PSI Cool
flPressCoolPasses = 0.0, // PV15 Number of inversions during PSI Cool
flPressCoolOutletTemp = 0.0, // PV16 Temp at end of PSI cool

flAtmosCoolInletLegTime = 0.0, // PV17 Transition time between PSI Cool and Atmos Cool
flAtmosCoolInletTemp = 0.0, // PV18 Temp at end of transition between PSI Cool and Atmos Cool
flAtmosCoolPressure = 0.0, // PV19 PSI to hold during Atmos Cool
flAtmosCoolTime = 0.0, // PV20 Time in Atmos Cool
flAtmosCoolPasses = 0.0, // PV21 Number of inversions during Atmos Cool
flAtmosCoolOutletTemp = 0.0; // PV22 Final Temp at end of Atmos Cool

/*==================== Definition of Constants ===================*/

const int	iMaxOperatorEntryTime = 300; // time before alarming for entry delay

int PVNumber = 0;

bool bPressureSegmentSet = false;

int j = PVNumber;
int h = 0;
float flComeUpPSIStep = 0.0,
flComeUpPSIStepTime = 0.0,
flPreviousComeUpPSIStep = 0.0,
flNextComeUpPSIStepTime = 0.0,
flRampPSISegment = 0.0,
flDrainProcess = 50.0,
flDrainEndOfProcess = 90.0;

/*==================== Definition of Strings ===================*/

CString	csiT = L"",
sStepLetter = L"",
strCTIME = L"",
csCTIME = L"",
csCTEMP = L"",
csBatchCode = L"Null",
strNumber,
strCTEMP,
strReelStatus = L"",
strReelMode = L"";

/* ================ Declaring Functions used by the main script =============== */
void InitObjects();
void DisplayInitialProcessScreen();
void DisplayCompleteProcessScreen();
void TurnAllDigitalOutputsOff();
void TurnAllAnalogOutputsOff();
void PromptForInitialOperatorEntries();
void PromptForInitialTemperature();
void PromptForCookOperatorEntries();
void ResetControlFlags_Variables();
void KillTheProcess();
void LoadProcessVariables();
void CommonInit();
void Func_WaitForEnterKey(const bool& bOverrideOut1, const bool& bOverrideOut2);

// *** BEGIN PHASES ***
void SensorChecks();
void LoadingPhase();
void OpEntryPhase();
void ComeUpPhaseSWSTemperature();	// SPLIT INTO TWO
void CookPhase();
void CoolingPhaseTemperature();		// SPLIT into TWO
void DrainPhase();
void UnLoadingPhase();
// *** END PHASES ***

void AddCookTime();
void GetNumberOfCodes();

float ConvertTemptoPress(float flTemp);
long SelectRTDTuningTable(long lPrimaryTable, long lBackupTable);
CString ConvertToString(float);
CString ConvertToString(double);
CString ConvertToString(long);
CString ConvertToString(int);

CString toStr(float);
CString toStr(double);
CString toStr(int);
CString toStr(long);
CString toStr(CString);

CString SecondsToStr(float);
CString SecondsToStr(double);
CString SecondsToStr(int);
CString SecondsToStr(long);
CString TempFtoStr(float);
CString TempFtoStr(double);
CString TempFtoStr(int);
CString TempFtoStr(long);

/* ============================ Definition of iOPS Variables =========================== */
//V5
bool			bEnableCommErrorLogs = true,
bLowCookTemperature = false,
bAlarmsUpdate_iOPS = false,
bPumpIsRunning = false,
bIsInIdle = false,	//machine is idle (true) only before Fill-phase and after Drain-phase.
bIsInProcess = false;	//machine is in process starting at come-up to end of cooling.

float			
flIdleHourTracker = 0,
flProcessHourTracker = 0,
flPumpHourTracker = 0,
flMotorHourTracker = 0;

CString			sPhaseMessage = L"Null",
sAgitationStyle = L"Null",
sProcessRecipeName = L"Null",
sRetortAlarmMessage = L"Null",
sAlarmMessage_Hold = L"Null",
sAlarmMessage_1 = L"Null",
sAlarmMessage_2 = L"Null",
sAlarmMessage_3 = L"Null",
sAlarmMessage_4 = L"Null",
sAlarmMessage_5 = L"Null";

/* ========= Declaring SpawnTask Functions used by the main script =========== */
void MajorProblemChecks();	// SpawnTask 1
void FillPhase();			// SpawnTask 2
							// SpawnTask 3 reserved for Process Phases
void TuningTableSwitch();	// SpawnTask 5
void ComeUpPhaseSWSPSI();	// SpawnTask 7
void SplitRangeControl();	// SpawnTask 8
void CoolingPhasePSI();		// SpawnTask 9
void PreHeatTask();			// SpawnTask 10
void MiscTasks();			// SpawnTask 11
void iOPSTagUpdate();		// SpawnTask 13 //V5
void iOPSTimeTracking();	// SpawnTask 15 //V5

//=====================================================================
//==================== Main Program Script begins =====================
//=====================================================================
void StartScript()
{
	InitObjects();
	Display.ShowMainWindow();
	Display.Display(15, L"Phase Time", L"PhaseTime");
	Display.SetAlarmChannel(LOGTECAlarm);
	Display.SnoozePeriod(300);
	Profiler.Init();
	bPressureSegmentSet = false;

	TaskLoop.SpawnTask((long)MiscTasks, 11);
	TaskLoop.SpawnTask((long)iOPSTagUpdate, 13); 
	TaskLoop.SpawnTask((long)iOPSTimeTracking, 15); 

	TurnAllAnalogOutputsOff();
	TurnAllDigitalOutputsOff();

//	************************ StartLabel: **************************
StartLabel:

	//Get the compile date, and compile time of current exe
	COleDateTime oleScriptRelease;
	CString csScriptReleaseDate = L"", csScriptVersion = L"";

	csScriptReleaseDate += __TIME__;  //compile time
	csScriptReleaseDate += L" ";
	csScriptReleaseDate += __DATE__;  //compile date
	oleScriptRelease.ParseDateTime(csScriptReleaseDate);
	csScriptReleaseDate.Format(L"Released on %d//%d//%d at %d:%d:%d", oleScriptRelease.GetMonth(), oleScriptRelease.GetDay(), oleScriptRelease.GetYear(), oleScriptRelease.GetHour(), oleScriptRelease.GetMinute(), oleScriptRelease.GetSecond());

	bKillMajorProblemChecks = true;
	bDoor1SecuredMode = false;
	bKillTheTaskFlag = true;
	bBeginAgitationFlag = false;

	Display.ResetProduct();


	Display.Phase(L"Greeting");
	sPhaseMessage = L"Greeting";
	if (!tcio.WriteINT(L"LTM.iPhase", 13)) 
	{
		Display.Log(L"Failure to set Phase to Greetings");
	}
	sProcessRecipeName = L"Null"; 
	Display.Display(0, L"");

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 13);

	while (!bPhaseCompleteFlag && !bShutDownFlag)
	{
		Sleep(100);
	}

	CString	csVar2 = L"", csHotRestartEntry = L"";
	ResetControlFlags_Variables();
	TurnAllDigitalOutputsOff();
	TurnAllAnalogOutputsOff();

	//Hot restart check
	if (AnalogIO.ReadAnalogIn(AI_PRESSURE) < 2)
		AnalogIO.SetAnalogOut(AO_VENT, 100.0);

	if (AnalogIO.ReadAnalogIn(AI_PRESSURE) > 10 || AnalogIO.ReadAnalogIn(AI_RTD1) > 180)
	{
		Display.KillEntry(); 

		do //(!bVerifyPromptFlag && !bShutDownFlag);
		{
			if (!bVerifyPromptFlag)
			{
				Display.KillEntry();
				Display.Display(20, L"");
				Display.Display(21, L"Is This A Hot Restart?");
				Display.Display(22, L"");
				Display.DisplayStatus(L"Enter Y or N");
				Display.Entry(3, 0);

				bVerifyPromptFlag = true;
			}
			while (!bVerifySelectionYN && !bShutDownFlag)
			{

				Func_WaitForEnterKey(bKillTheTaskFlag, false);

				csHotRestartEntry = Display.GetEnteredString(3);

				if ((csHotRestartEntry.CompareNoCase(L"Y") == 0) || (csHotRestartEntry.CompareNoCase(L"N") == 0))
				{
					bVerifySelectionYN = true;

					if (csHotRestartEntry.CompareNoCase(L"Y") == 0)
					{
						bHOT_RestartFlag = true;
						Display.Log(L"Hot Restart!", L"", NORMAL);

						Display.DisplayVariable(1, L"Retort Temp (F)", L"float", (LONG)&flCurrentTemperature);

						TaskLoop.SpawnTask((LONG)MajorProblemChecks, 1);

						Sleep(250);

						if (bSplitRangeCompleteFlag && !bShutDownFlag)	// Should be running - used to Reinitialize upon override.
						{
							bKillSplitRangeFlag = false;
							TaskLoop.SpawnTask((LONG)SplitRangeControl, 8);
							Sleep(250);

							nCorrectTuningTable = SelectRTDTuningTable(3, 4);
							PID.PIDSetPoint(AO_STEAM, flCurrentTemperature, 0);
							PID.PIDStart(AO_STEAM, nCorrectTuningTable);

							PID.PIDSetPoint(AO_SPLT_RNG, flCurrentPressure, 0);
							PID.PIDStart(AO_SPLT_RNG, 11);

							flDisplayLoopPsiSP = PID.GetPIDSetPoint(AO_SPLT_RNG);
							flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
						}
					}
					else
						bHOT_RestartFlag = false;
				}

				else //it's not a valid entry, prompt YN again
				{
					bVerifySelectionYN = true;		//it must be set to true to re-enter
					bVerifyPromptFlag = false;
				}

				Sleep(250);
			}

		} while (!bVerifyPromptFlag && !bShutDownFlag);
	}

	DisplayInitialProcessScreen();

	Display.KillEntry();

	lTemp = 0;
	Display.OverrideEnable();

	//////////////// Get time of day for Greeting
	COleDateTime oleDate = COleDateTime::GetCurrentTime();
	iCurrentHour = oleDate.GetHour();

	if (bHOT_RestartFlag && !bShutDownFlag)
	{
		GoTo(OpEntryLabel);
	}

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"Restart", StartLabel);
	Display.OverrideText(2, L"SensorCheck", SensorCheckLabel);
	Display.OverrideText(3, L"", -1);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"", -1);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"ShutDown", ShutdownLabel);
	Display.OverrideText(8, L"Full Drain", FullDrainLabel);

	if (iCurrentHour < 12)
	{
		Display.Display(20, L"Good Morning...");
	}
	else
	{
		if (iCurrentHour >= 12 && iCurrentHour <= 17)
		{
			Display.Display(20, L"Good Afternoon...");
		}
		else
			Display.Display(20, L"Good Evening...");
	}
	Display.Display(21, L"JBT FoodTech");
	Display.Display(22, L"LOG-TEC Momentum System");
	Display.Display(23, L"Efficient Agitating");
	Display.Display(24, L"Water Spray Retort");
	Display.DisplayStatus(L"Press Enter to Continue");

	Display.Entry(0, 0);


	while (!Display.IsEnterKeyPressed())
	{
		Sleep(250);

		if (bShutDownFlag)
		{
			Display.KillEntry();
			goto ShutdownLabel;
		}
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				Display.KillEntry();
				if (lTemp == FullDrainLabel)
					bFullDrainFlag = true;
				Sleep(250);
				GoTo(lTemp);

			}
		}
	}


	Display.KillEntry();

	//	********************* SensorCheckLabel: **************************
SensorCheckLabel:
	bKillMajorProblemChecks = true;
	bDoor1SecuredMode = false;
	bKillTheTaskFlag = true;

	if (bShutDownFlag)
		goto ShutdownLabel;

	while (!bPhaseCompleteFlag && !bShutDownFlag)
	{
		Sleep(250);
	}

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"Restart", StartLabel);
	Display.OverrideText(2, L"SensorCheck", SensorCheckLabel);
	Display.OverrideText(3, L"", -1);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"", -1);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"ShutDown", ShutdownLabel);
	Display.OverrideText(8, L"Full Drain", FullDrainLabel);

	if (!bShutDownFlag)
	{
		bSensorCheckCompleteFlag = false;
		TaskLoop.SpawnTask((long)SensorChecks, 3);
	}
	lTemp = 0;
	Display.OverrideEnable();

	while (!bSensorCheckCompleteFlag)
	{
		if (bShutDownFlag)
			goto ShutdownLabel;
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				if (lTemp == FullDrainLabel)
					bFullDrainFlag = true;
				Display.KillEntry();
				Sleep(250);
				GoTo(lTemp);
			}
		}
		Sleep(250);
	}
	//	************************ LoadingLabel: **************************
LoadingLabel:

	bKillMajorProblemChecks = true;
	bBeginAgitationFlag = false;
	bDoor1SecuredMode = false;
	Sleep(500);
	bKillTheTaskFlag = true;

	if (bShutDownFlag)
		goto ShutdownLabel;

	while (!bPhaseCompleteFlag && !bShutDownFlag)
	{
		Sleep(250);
	}

	if (!bMajorPBChecksRunning)
	{
		bKillMajorProblemChecks = false;
		TaskLoop.SpawnTask((long)MajorProblemChecks, 1);
	}

	if (!bMiscTasksRunning) 
	{
		TaskLoop.SpawnTask((long)MiscTasks, 11);
		Sleep(500);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 1);

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"Restart", StartLabel);
	Display.OverrideText(2, L"Sensor Check", SensorCheckLabel);
	Display.OverrideText(3, L"Restart Loading", LoadingLabel);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"", -1);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"", -1);

	if (!bShutDownFlag)
	{
		bLoadingCompleteFlag = false;
		TaskLoop.SpawnTask((long)LoadingPhase, 3);
	}
	lTemp = 0;
	Display.OverrideEnable();

	while (!bLoadingCompleteFlag)
	{
		if (bShutDownFlag)
			goto ShutdownLabel;
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				if (lTemp == FullDrainLabel)
					bFullDrainFlag = true;
				bDoor1SecuredMode = false;
				Display.KillEntry();
				Sleep(250);
				GoTo(lTemp);
			}
		}
		Sleep(250);
	}
	//	************************ OpEntryLabel: **************************
OpEntryLabel:

	bKillTheTaskFlag = true;
	bBeginAgitationFlag = false;

	while ((!bPhaseCompleteFlag || !bLoadingCompleteFlag) && !bShutDownFlag && !bHOT_RestartFlag)
	{
		if (bShutDownFlag)	goto ShutdownLabel;
		Sleep(300);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 2);

	lTemp = 0;//reinitialize variable
	if (bHOT_RestartFlag && !bShutDownFlag)
	{
		Display.OverrideEnable();
		Display.OverrideText(1, L"Restart", StartLabel);
		Display.OverrideText(2, L"", -1);
		Display.OverrideText(3, L"", -1);
		Display.OverrideText(4, L"", -1);
		Display.OverrideText(5, L"", -1);
		Display.OverrideText(6, L"", -1);
		Display.OverrideText(7, L"ShutDown", ShutdownLabel);
		Display.OverrideText(8, L"", -1);
	}
	else
	{
		Display.OverrideEnable();
		Display.OverrideText(1, L"Restart", StartLabel);
		Display.OverrideText(2, L"Load Retort", LoadingLabel);
		Display.OverrideText(3, L"Restart Operator Entries", OpEntryLabel);
		Display.OverrideText(4, L"", -1);
		Display.OverrideText(5, L"", -1);
		Display.OverrideText(6, L"", -1);
		Display.OverrideText(7, L"Shutdown", ShutdownLabel);
		Display.OverrideText(8, L"Full Drain", FullDrainLabel);
	}

	Sleep(200);

	if (!bMajorPBChecksRunning)
	{
		TaskLoop.SpawnTask((LONG)MajorProblemChecks, 1);
		Sleep(500);
	}

	if (!bMiscTasksRunning) 
	{
		TaskLoop.SpawnTask((long)MiscTasks, 11);
		Sleep(500);
	}

	if (!bShutDownFlag)
	{
		bOpEntryCompleteFlag = false;
		TaskLoop.SpawnTask((long)OpEntryPhase, 3);
	}
	lTemp = 0;

	Display.OverrideEnable();

	while (!bOpEntryCompleteFlag)
	{
		if (bShutDownFlag)
			goto ShutdownLabel;

		if (bJumpOutToDrainFlag)
			goto DrainLabel;

		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				if (lTemp == StartLabel)
					Display.ResetProduct();
				if (lTemp == FullDrainLabel)
				{
					bFullDrainFlag = true;
					bDoor1SecuredMode = false;
				}
				Display.KillEntry();
				Sleep(250);
				GoTo(lTemp);
			}
		}
		Sleep(250);
	}


	goto ComeUpLabel;

//	************************ HotRestartLabel: **************************
HotRestartLabel:
	//Operator has selected HotRestart and recipe
	//Now they must select which phase of the process 

	bKillTheTaskFlag = true;
	if (bShutDownFlag)
		goto ShutdownLabel;

	while (!bPhaseCompleteFlag && !bShutDownFlag)
	{
		Sleep(250);
	}

	Display.Phase(L"Recovery");
	sPhaseMessage = L"Recovery"; 

	DisplayInitialProcessScreen();

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"Restart", StartLabel);
	Display.OverrideText(2, L"", -1);
	Display.OverrideText(3, L"", -1);
	Display.OverrideText(4, L"Come Up", ComeUpLabel);
	Display.OverrideText(5, L"Cook", CookLabel);
	Display.OverrideText(6, L"Cooling", CoolingLabel);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"", -1);


	

	Display.KillEntry();
	Display.Display(20, L"** ATTENTION **");
	Display.Display(21, L"");
	Display.Display(22, L"Hot Restart Selected");
	Display.Display(23, L"");
	Display.Display(24, L"You Must Override To Continue");
	Display.DisplayStatus(L"");
	Display.Entry(0, 0, TRUE);

	while (!bOverrideSelectionComplete)
	{

		if (bSplitRangeCompleteFlag && !bShutDownFlag)	// Should be running - used to Reinitialize upon override.
		{
			flDisplayLoopPsiSP = 0.0;
		}

		flDisplayLoopPsiSP = PID.GetPIDSetPoint(AO_SPLT_RNG);

		if (bShutDownFlag)
			goto ShutdownLabel;

		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				Display.KillEntry();
				bOverrideSelectionComplete = true;


				if (lTemp == CoolingLabel)
				{
					Display.Log(L"Incomplete Process! Check records", L"", DEVIATION);
					bProcessDeviationFlag = true;
				}

				if (lTemp == DrainLabel)
				{
					Display.Log(L"Incomplete Process! Check records", L"", DEVIATION);
					bProcessDeviationFlag = true;
				}
				bHOT_RestartFlag = false;
				bOverrideSelectionComplete = false;
				GoTo(lTemp);

			}
		}

		Sleep(250);
	}



//	************************ ComeUpLabel **************************
ComeUpLabel:

	bProcessStarted = true;
	bBeginAgitationFlag = true;
	bKillTheTaskFlag = true;
	bKillCUPSITaskFlag = true;

	if (bShutDownFlag)
		goto ShutdownLabel;

	bDoor1SecuredMode = false;

	while (!bShutDownFlag && (!bPhaseCompleteFlag || !bCUPSIPhaseCompleteFlag || !bOpEntryCompleteFlag))
		Sleep(250);

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"", -1);
	Display.OverrideText(2, L"", -1);
	Display.OverrideText(3, L"Restart ComeUp", ComeUpLabel);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"Cooling", CoolingLabel);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"", -1);

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 5);

	if (!bShutDownFlag)
	{
		//Launch the relevant tasks for the recipe type
		if (bCleaningRecipeSelected || bSWSOnlyRecipeFlag )
		{
			TaskLoop.SpawnTask((long)ComeUpPhaseSWSTemperature, 3);
			Sleep(1000);
			TaskLoop.SpawnTask((long)ComeUpPhaseSWSPSI, 7);
			Sleep(1000);
			if (!bSplitRangeRunning)
			{
				TaskLoop.SpawnTask((long)SplitRangeControl, 8);			// Split Range & Pressure control PID runs ALL THROUGH process UNTIL END.
				Sleep(1000);
			}
			bCUPSICompleteFlag = false;
			bKillSplitRangeFlag = false;
		}
	
		bCUTempCompleteFlag = false;
	}

	lTemp = 0;
	Display.OverrideEnable();

	do // Must check once, and wait until Temp Phase to be done
	{
		if (bShutDownFlag)
			goto ShutdownLabel;

		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			// DEVIATION if Override to Drain or End Process
			if (lTemp == DrainLabel || lTemp == EndProcessLabel || lTemp == CoolingLabel || lTemp == CookLabel)
			{
				Display.Log(L"Please Review Process Cycle!", L"", DEVIATION);
				bProcessDeviationFlag = true;
			}
			if (lTemp == AddCookTimeLabel)
				bAddCookTimeFlag = true;
			else
				GoTo(lTemp);

		}
		Sleep(250);
	} while (!bCUTempCompleteFlag); // PSI gets terminated in Cook

//	************************ CookLabel: **************************
CookLabel:

	bKillTheTaskFlag = true;
	bBeginAgitationFlag = true;

	bDoor1SecuredMode = false;

	if (bShutDownFlag)
		goto ShutdownLabel;
	while (!bShutDownFlag && !bPhaseCompleteFlag)
	{
		Sleep(250);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 6);

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"", -1);
	Display.OverrideText(2, L"Add Cook Time", AddCookTimeLabel);
	Display.OverrideText(3, L"Restart Cook", CookLabel);
	Display.OverrideText(4, L"", -1);		// Hot Restart
	Display.OverrideText(5, L"Cooling", CoolingLabel);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"", -1);

	bPhaseCompleteFlag = false;

	if (!bShutDownFlag)
	{
		bCookCompleteFlag = false;
		TaskLoop.SpawnTask((long)CookPhase, 3);
	}
	lTemp = 0;
	Display.OverrideEnable();

	while (!bCookCompleteFlag && !bShutDownFlag)
	{
		if (bShutDownFlag) goto ShutdownLabel;
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			// DEVIATION if Override to Drain or End Process
			if (lTemp == CoolingLabel || lTemp == EndProcessLabel)
			{
				Display.Log(L"Please Review Process Cycle!", L"", DEVIATION);
				bProcessDeviationFlag = true;
			}
			if (lTemp == AddCookTimeLabel)
				bAddCookTimeFlag = true;
			else
				GoTo(lTemp);
		}
		Sleep(250);
	}
	//	************************ CoolingLabel: **************************
CoolingLabel:

	bKillTheTaskFlag = true;
	bBeginAgitationFlag = true;
	bKillCoolPSITaskFlag = true;
	bKillCUPSITaskFlag = true;
	if (bShutDownFlag)
		goto ShutdownLabel;
	bProcessStarted = false;

	bDoor1SecuredMode = false;

	while (!bShutDownFlag && (!bPhaseCompleteFlag || !bCUPSIPhaseCompleteFlag))
	{
		Sleep(250);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 7);

	if (!bShutDownFlag)
	{
		bCoolTempCompleteFlag = false;
		bCoolPSICompleteFlag = false;
		
		TaskLoop.SpawnTask((long)CoolingPhasePSI, 9);
		Sleep(250);
		TaskLoop.SpawnTask((long)CoolingPhaseTemperature, 3);
	}
	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"", -1);
	Display.OverrideText(2, L"", -1);
	Display.OverrideText(3, L"Restart Cooling", CoolingLabel);
	Display.OverrideText(4, L"", -1);	
	Display.OverrideText(5, L"Drain", DrainLabel);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"", -1);				

	do // Must check once, and wait until BOTH temp and PSI phases done.
	{
		if (bShutDownFlag)
			goto ShutdownLabel;

		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			Display.KillEntry();
			// DEVIATION if Override to Drain or End Process
			if (lTemp == DrainLabel || lTemp == EndProcessLabel)
			{
				Display.Log(L"Process Override from COOLING Phase!", L"", DEVIATION);
				bProcessDeviationFlag = true;
			}
			GoTo(lTemp);
		}
		Sleep(250);
	} while (!bCoolTempCompleteFlag || !bCoolPSICompleteFlag);

	PinesTime5.StopTimer(); // Total ComeUp through CoolDown Time
	csVar2 = PinesTime5.GetElapsedTimeInString();
	Display.Log(L"Total Cycle Time", csVar2, NORMAL);

	Profiler.StopProfiler(); // Cooling Off - End Data Collection


//	************************ DrainLabel: **************************
DrainLabel:
FullDrainLabel:

	bProcessStarted = false;
	bKillCoolPSITaskFlag = true;
	bKillTheTaskFlag = true;
	bBeginAgitationFlag = false;
	bKillRotationTaskFlag = true;

	if (bShutDownFlag)
		goto ShutdownLabel;

	while (!bShutDownFlag && (!bPhaseCompleteFlag || !bCoolPSIPhaseCompleteFlag))
	{
		Sleep(250);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 8);

	if (!bShutDownFlag)
	{
		if (bFullDrainFlag)
			bDrainCompleteFlag = true;
		else
			bDrainCompleteFlag = false;
		TaskLoop.SpawnTask((long)DrainPhase, 3);
	}

	lTemp = 0;
	Display.OverrideEnable();

	lTemp = 0;//reinitialize variable
	Display.OverrideText(1, L"", -1);
	Display.OverrideText(2, L"Full Drain", FullDrainLabel);
	Display.OverrideText(3, L"Restart Drain", DrainLabel);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"Unload Retort", UnLoadingLabel);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"End Cycle", EndProcessLabel);

	while (!bDrainCompleteFlag)
	{
		if (bShutDownFlag) goto ShutdownLabel;
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			GoTo(lTemp);
		}
		Sleep(250);
	}
	if (bFullDrainFlag || bJumpOutToDrainFlag)	// avoid going to UnLoading and reset
	{					// flag at StartLabel
		goto StartLabel;
	}
	//	************************ UnLoadingLabel: **************************
UnLoadingLabel:

	bBeginAgitationFlag = false;
	bKillRotationTaskFlag = true;
	bProcessStarted = false;

	if (!bKillSplitRangeFlag)
	{
		bKillSplitRangeFlag = true;
		while (!bSplitRangeCompleteFlag)
			Sleep(250);
	}
	bKillTheTaskFlag = true;
	if (bShutDownFlag)
		goto ShutdownLabel;
	while (!bPhaseCompleteFlag && !bShutDownFlag)
	{
		Sleep(250);
	}

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 9);

	lTemp = 0;//reinitialize variable
	Display.OverrideEnable();
	Display.OverrideText(1, L"", -1);
	Display.OverrideText(2, L"", -1);
	Display.OverrideText(3, L"Restart Unloading", UnLoadingLabel);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"", -1);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"Shutdown", ShutdownLabel);
	Display.OverrideText(8, L"End Cycle", EndProcessLabel);

	bDoor1SecuredMode = false;

	if (!bShutDownFlag)
	{
		bUnLoadingCompleteFlag = false;
		TaskLoop.SpawnTask((long)UnLoadingPhase, 3);
	}
	lTemp = 0;
	Display.OverrideEnable();

	while (!bUnLoadingCompleteFlag)
	{
		if (bShutDownFlag) goto ShutdownLabel;
		if (Display.IsOverRideScreenLaunched())
		{
			lTemp = Display.GetOverRideKey();
			if (lTemp > 0)
			{
				Display.KillEntry();
				GoTo(lTemp);
			}
		}
		Sleep(250);
	}
	//	************************ EndProcessLabel: **************************
EndProcessLabel:

	bKillTheTaskFlag = true;
	bRunLevelControlFlag = false;	// Separate Task Loop Monitors Level
	bFillLevelAdjustFlag = false;
	bEndLevelControlFlag = true;		// Kills Level Control Task Loop here
	bKillCUPSITaskFlag = true;
	bKillCoolPSITaskFlag = true;
	bKillSplitRangeFlag = true;
	bKillRotationTaskFlag = true;
	bBeginAgitationFlag = false;
	bProcessStarted = false;
	bKillTheMiscTaskLoop = true;

	if (bShutDownFlag) goto ShutdownLabel;

	while (!bPhaseCompleteFlag || !bCUPSIPhaseCompleteFlag)
	{
		Sleep(250);
	}

	Sleep(2000);

	Display.Log(L"End of Process");

	lTemp = 0;

	Display.OverrideDisable();
	KillTheProcess();

	goto StartLabel; // Back to Beginning

//	************************ ShutdownLabel: **************************
// *** Only Get here via a Problem? Then, available Overrides are via from whence you came ***
ShutdownLabel:

	bKillTheTaskFlag = true;	// Set TRUE by the OVERRiDE & MAJOR PROBLEM CHECKS
	bKillSplitRangeFlag = true;
	bKillRotationTaskFlag = true;
	bBeginAgitationFlag = false;
	bCriticalPSIFailureFlag = false;	// reset flag to start
	bCheckCriticalPSIFlag = false;	// set true for the phases we want to check the vapor PSI.
	bShutDownFlag = true;
	bRunLevelControlFlag = false;
	bKillTheMiscTaskLoop = true;

	// First - QUICKLY turn everything OFF!
	TurnAllDigitalOutputsOff();
	TurnAllAnalogOutputsOff();

	Display.KillEntry();

	// Now wait for the exited task loop to return - then we need to turn everything OFF, again.
	bKillCUPSITaskFlag = true;
	bKillCoolPSITaskFlag = true;

	while (!bPhaseCompleteFlag || !bCUPSIPhaseCompleteFlag ||
		!bCoolPSIPhaseCompleteFlag || !bCUTempCompleteFlag)
	{
		Sleep(250);
	}
	Display.Phase(L"ShutDwn");
	sPhaseMessage = L"ShutDwn"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 10)) 
	{
		Display.Log(L"Failure to set Phase to Shutdown");
	}
	Display.Log(L"PROCESS ABORTED", L"", ABORT);

	AnalogIO.SetAnalogOut(VAO_LTM_Phase, 10);

	TurnAllDigitalOutputsOff();
	TurnAllAnalogOutputsOff();

	// *** END OF COMMON (looking) INITIALIZATION STUFF

	Display.KillEntry();

	CString csVar1, csVar3;

	Display.OverrideEnable();
	lTemp = 0;

	PinesTime1.StopTimer();	// Phase, Step, or Segment Timer(sec)
	PinesTime2.StopTimer();	// Operator Entry Screen Timer (sec)
	PinesTime3.StopTimer();	// Delay - Phase, Step, or Segment Timer (sec)
	PinesTime4.StopTimer(); // Flow Alarm Filter Timer
	PinesTime5.StopTimer();	// Total ComeUp through CoolDown Time
	PinesTime6.StopTimer(); // Agitation/Rock Function Timer
	PinesTime7.StopTimer();	// Water Level Filter Alarm Timer - initial
	PinesTime8.StopTimer();	// Flow SHUTDOWN Filter Timer
	PinesTime9.StopTimer();	// RTD's Differ Alarm Filter Timer
	PinesTime10.StopTimer();// Water HiGH Level Filter Alarm Timer - FiLTER
	PinesTime11.StopTimer();// Water LOW Level Filter Alarm Timer - FiLTER
	PinesTime14.StopTimer();// ComeUp and Cooling PRESSURE phase timer
	PinesTime15.StopTimer();// Air Supply Pressure Alarm Delay Timer
	PinesTime16.StopTimer();// Alarm Horn Disable "Acknowledged" Timer
	PinesTime17.StopTimer();
	PinesTime18.StopTimer();
	PinesTime19.StopTimer();
	PinesTime20.StopTimer();
	PinesTime21.StopTimer();
	PinesTime22.StopTimer();
	PinesTime33.StopTimer();
	PinesTime34.StopTimer();

	Display.Alarm(2, 1, LOGTECAlarm, L"TOTAL SHUTDOWN!", DEVIATION);
	sRetortAlarmMessage = L"TOTAL SHUTDOWN!";  //RH v.iOPS
	sAlarmMessage_Hold = sRetortAlarmMessage;
	bAlarmsUpdate_iOPS = true;

	bProcessDeviationFlag = true;

	Display.OverrideEnable();
	Display.OverrideText(1, L"Restart", StartLabel);
	Display.OverrideText(2, L"", -1);
	Display.OverrideText(3, L"", -1);
	Display.OverrideText(4, L"", -1);
	Display.OverrideText(5, L"", -1);
	Display.OverrideText(6, L"", -1);
	Display.OverrideText(7, L"", -1);
	Display.OverrideText(8, L"", -1);




	do
	{
		Display.KillEntry(); 

		Display.Display(20, L"-- TOTAL SHUTDOWN! -- RETORT IS SEALED --");
		csVar2.Format(L" %-5.1f", flCurrentTemperature);
		csVar3.Format(L" %-5.1f", flCurrentPressure);
		csVar1 = "Sealed With a Temp of " + csVar2 + " and Pressure of " + csVar3;
		Display.Display(21, csVar1);
		Display.Display(22, L"Please Call Your Manager");
		Display.DisplayStatus(L"Must Use Over-Ride To Exit Shutdown Mode");
		Display.Entry(0, 0, TRUE);

		while (true) // No way out of here, except bShutDownFlag, stuck till Overriden
		{
			if (Display.IsOverRideScreenLaunched())
			{
				lTemp = Display.GetOverRideKey();

				// if RTD FAiLURE, message, break, and restate Shutdown
				if (lTemp > 0 && bRetort_TempFailFlag)
				{
					Display.KillEntry();

					Display.Display(20, L"-- TOTAL SHUTDOWN! -- RETORT IS SEALED --");
					Display.Display(22, L"Please Call Your Manager");
					Display.Display(23, L"RTD FAILURE! Cannot leave Shutdown.");
					Display.Display(24, L"Repair RTD ERROR - and restart!");

					Display.DisplayStatus(L"Press Enter to continue...");
					Display.Entry(0, 0);
					while (!Display.IsEnterKeyPressed())
					{
						Sleep(250);
					}
					bRetort_TempFailFlag = false;

					lTemp = 0;

				}
				// if RTD FAiLURE, message, break, and restate Shutdown
				if (lTemp > 0 && bRetort_PresFailFlag)
				{
					Display.KillEntry();

					Display.Display(20, L"-- TOTAL SHUTDOWN! -- RETORT IS SEALED --");
					Display.Display(22, L"Please Call Your Manager");
					Display.Display(23, L"PRESSURE FAILURE! Cannot leave Shutdown.");
					Display.Display(24, L"Repair PRESSURE ERROR - and continue!");

					Display.DisplayStatus(L"Press Enter to continue...");
					Display.Entry(0, 0);
					while (!Display.IsEnterKeyPressed())
					{
						Sleep(250);
					}
					lTemp = 0;

				}
				// if E-stop, message, break, and restate Shutdown
				if (lTemp > 0 && !DigitalIO.ReadDigitalIn(DI_ESTOP_SR))
				{
					bE_StopPushed = true;
					Display.KillEntry();

					Display.Display(20, L"-- TOTAL SHUTDOWN! -- RETORT IS SEALED --");
					Display.Display(22, L"Please Call Your Manager");
					Display.Display(23, L"E-STOP DEPRESSED!");
					Display.Display(24, L"You Must Clear E-STOP to Exit Shutdown Mode!");

					Display.DisplayStatus(L"Press Enter to continue...");
					Display.Entry(0, 0);
					while (!Display.IsEnterKeyPressed())
					{
						Sleep(250);
					}
					lTemp = 0;

				}
				if (lTemp > 0 && DigitalIO.ReadDigitalIn(DI_ESTOP_SR))
				{
					// Get here only by leaving with no estop
					if (lTemp == FullDrainLabel)	// set the Full Drain Flag true
					{
						bFullDrainFlag = true;
					}
					bE_StopPushed = false;
					bShutDownFlag = false;
					Display.KillEntry();
					GoTo(lTemp);
				}
			}

			Sleep(250);
		}
	} while (true);

	GoTo(lTemp);

	Display.KillEntry();

}	/****************** End of Main Program Script *************************/

//======================== LoadingPhase ===================================
void LoadingPhase()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bLoadingCompleteFlag	= false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		= false;	// Set in COOK
	bRunLevelControlFlag	= false;	// Separate Task Loop Monitors Level

	Display.Phase(L"Loading");
	sPhaseMessage = L"Loading";
	if (!tcio.WriteINT(L"LTM.iPhase", 1)) 
	{
		Display.Log(L"Failure to set Phase to Loading");
	}

	if (!bKillTheTaskFlag)
	{
		Display.Log(L"Begin Loading");

		Display.KillEntry(); 

		Display.Display(20, L"Waiting for");
		Display.Display(21, L"End of Loading");
		Display.DisplayStatus(L"Waiting for door to seal...");
		Display.Entry(0, 0, TRUE);

		while (!bKillTheTaskFlag && (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR) ||
			DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT)))
			Sleep(500);

		Display.KillEntry();
		Display.Log(L"Door Closed");
	}

	//COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag
	if (!bKillTheTaskFlag)
	{
		Display.Log(L"End Loading");
		bCycleRunning				= true;	// bCycleRunning -- Signals that the door has closed (It does not mean the process has started) It enables
											//					shutdown logic for dangerous conditions.

											// bProcessRunning-- Signals that the process has started. This flag enables deviation status for alarms during
											//  				 process).
	}

	TaskLoop.KillTask(3);
	Sleep(200);
	bLoadingCompleteFlag = true;
	bPhaseCompleteFlag = true;
}	// End void LoadingPhase()

//======================== OpEntryPhase ===================================
void OpEntryPhase()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bOpEntryCompleteFlag	= false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		= false;	// Set in COOK
	bRunLevelControlFlag	= false;	// Separate Task Loop Monitors Level

	Display.Phase(L"Oper Entry");
	sPhaseMessage = L"Oper Entry"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 2)) 
	{
		Display.Log(L"Failure to set Phase to Op Entry");
	}

	CommonInit();
	// *** END OF COMMON INITIALIZATION STUFF

	bPreHeatTaskCompleteFlag = false;
	bFillPhaseCompleteFlag = false;

	if (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR) && !bKillTheTaskFlag)
	{
		Display.KillEntry();

		Display.Display(20, L"Retort Door Open...");
		Display.Display(21, L"Close the Door To Continue");
		Display.Entry(0, 0, TRUE);

		while ((DigitalIO.ReadDigitalIn(!DI_DOOR_CLOSED_SR) || DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT)) && !bKillTheTaskFlag)
			Sleep(250);

		Display.KillEntry();
	}

	bool bVerifyEntry = false;
	CString csEntry = L"", csProdCode = L"", csNumOfContainers = L"", csSchedProcessTime = L"", csProdDisposition = L"";

	PromptForInitialOperatorEntries();		//Get Product Description and Process Variables

	Display.OverrideEnable();

	bIsInIdle = false;	//machine is idle (true) only before Fill-phase and after Drain-phase.

	if (!bHOT_RestartFlag)
	{
		TaskLoop.SpawnTask((long)FillPhase, 2);
	}

	while ((!bFillPhaseCompleteFlag || !bPreHeatTaskCompleteFlag ) && !bHOT_RestartFlag)
	{
		Sleep(250);
	}

	Display.KillEntry();

	if (!bHOT_RestartFlag)
	{
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
	}

	Display.KillEntry();
	TaskLoop.KillTask(3);
	bOpEntryCompleteFlag = true;
	bPhaseCompleteFlag = true;

} //end of OpEntryPhase()

/*======================== FillPhase ===================================
Add Water until reaching Target Setpoint
Done when: initial Fill Level Satisfied
NEVER ADD WATER - once FILLED
======================== FillPhase ===================================*/
void FillPhase()
{
	bool	bPressPIDLoopStarted = false;

	float	flDisplayPreheat = 0.0;

	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bFillCompleteFlag = false;	// Unique: Set false here & then true at end of function
	bFillPhaseCompleteFlag = false;	// Set false here & then true at end of function
	bDoDeviationCheck = false;	// Set in COOK

	// *** END OF COMMON INITIALIZATION STUFF
	bool bLongFillAlarm = false, bWasTooFullFlag = false;
	CString csEntry = L"", csVar1 = L"", csVar2 = L"", strEntry = L"";

	if (!bSplitRangeRunning)	// Should be running - used to Reinitialize upon override.
	{
		bKillSplitRangeFlag = false;
		TaskLoop.SpawnTask((long)SplitRangeControl, 8);
		Sleep(250);
	}

	PID.PIDStop(AO_SPLT_RNG);
	AnalogIO.SetAnalogOut(AO_SPLT_RNG, flZeroValueCrossOver);//Close air and vent control valves

	do
	{
		if (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR) && !bKillTheTaskFlag)
		{
			TurnAllAnalogOutputsOff();
			TurnAllDigitalOutputsOff();
		}

		while (!bKillTheTaskFlag && (!bRecipeSelectionFlag || !DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR)))
		{
			Sleep(250);
		}
		Sleep(250);

	} while (!bKillTheTaskFlag && !DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR));

	Display.Phase(L"Init Fill");
	if (!tcio.WriteINT(L"LTM.iPhase", 3))
	{
		Display.Log(L"Failure to set Phase to Fill");
	}
	sPhaseMessage = L"Init Fill";

	Display.KillEntry();
	Sleep(100);
	Display.Display(20, L"Waiting for Machine");
	Display.Display(21, L"To Reach");
	Display.Display(22, L"Level = " + toStr(flInitialFillLevel) + L" %");
	if (flInitialPressure > 0.0)
		Display.Display(23, L"Pressure = " + toStr(flInitialPressure) + L" psi");
	Display.Display(24, L"PreHeat Temp = " + TempFtoStr(flDisplayPreheat));
	Display.DisplayStatus(L"Please Wait...");
	Display.Entry(0, 0, TRUE);

	bProcessStarted = true;

	if ((flCurrentWaterLevel > (flInitialFillLevel + 2)) && !bKillTheTaskFlag)
	{
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
		bWasTooFullFlag = true;

		while ((flCurrentWaterLevel > (flInitialFillLevel + 3)) && !bKillTheTaskFlag)
		{
			Sleep(250);
		}
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	}

	if ((flCurrentWaterLevel < flInitialFillLevel) && !bKillTheTaskFlag)
	{
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Onn);
		PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
		PinesTime1.ZeroTime();

		if (flPreHeatTemp < 1)
			flDisplayPreheat = (float)AnalogIO.ReadAnalogIn(AI_PREHT_RTD);
		else
			flDisplayPreheat = flPreHeatTemp;

		while (!bKillTheTaskFlag && (flCurrentWaterLevel < (flInitialFillLevel - 0.5)))
		{			
			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;

			if ((PinesTime1.GetElapsedTime() > flLongFillTime) && !bLongFillAlarm && !bKillTheTaskFlag )
			{
				Display.KillEntry();

				while (!bKillTheTaskFlag && !bVerifyYNFlag)
				{
					bLongFillAlarm = true;
					DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
					Display.Alarm(2, 1, LOGTECAlarm, L"Fill Delay!", NORMAL);
					sRetortAlarmMessage = L"Fill Delay!"; 
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					Display.Log(L"Water Fill Paused - Inspect Level");

					Display.Display(20, L"Filling Has Been Suspended.");
					Display.Display(21, L"The Water Level Has Not Reached");
					Display.Display(22, L"The Setpoint After 5 Minutes...");
					Display.Display(23, L"Press (R) To RESUME Filling");
					Display.Display(24, L"Or (D) To DRAIN the Retort");
					Display.DisplayStatus(L"Enter R or D");
					Display.Entry(3, 0);

					Func_WaitForEnterKey(bKillTheTaskFlag, false);

					strEntry = Display.GetEnteredString(3);

					Display.KillEntry();

					if (strEntry.CompareNoCase(L"R") == 0)
					{
						bVerifyYNFlag = true;
						bInitFillWaitDone = false;
						DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Onn);
						Display.Log(L"Water Fill Resumed By Operator");

						PinesTime1.StartTimer();
						PinesTime1.ZeroTime();
					}
					else // Any choice other than "R" goes here...
					{
						bVerifyYNFlag = false;

						if (strEntry.CompareNoCase(L"D") == 0) // If the other choice happens to be "D"...
						{
							Display.Log(L"Drain Selected By Operator");
							bVerifyYNFlag = true;
							bJumpOutToDrainFlag = true;
						}
					}
					Sleep(250);
				}
			}

			if ((PinesTime1.GetElapsedTime() > 900) && !bLongFillAlarm && !bKillTheTaskFlag )
			{
				Display.KillEntry();

				while (!bKillTheTaskFlag && !bVerifyYNFlag)
				{
					bLongFillAlarm = true;
					DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
					Display.Alarm(2, 1, LOGTECAlarm, L"Fill Delay!", SendLogStatus);
					sRetortAlarmMessage = L"Fill Delay!";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					Display.Log(L"Water Fill Paused - Inspect Level");

					Display.Display(20, L"Filling Has Been Suspended.");
					Display.Display(21, L"The Water Level Has Not Reached");
					Display.Display(22, L"The Setpoint After 15 Minutes...");
					Display.Display(23, L"Press (R) To RESUME Filling");
					Display.Display(24, L"Or (D) To DRAIN the Retort");
					Display.DisplayStatus(L"Enter R or D");
					Display.Entry(3, 0);

					Func_WaitForEnterKey(bKillTheTaskFlag, false);

					strEntry = Display.GetEnteredString(3);

					Display.KillEntry();

					if (strEntry.CompareNoCase(L"R") == 0)
					{
						bVerifyYNFlag = true;
						bInitFillWaitDone = false;
						DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Onn);
						Display.Log(L"Water Fill Resumed By Operator");

						PinesTime1.StartTimer();
						PinesTime1.ZeroTime();
					}
					else // Any choice other than "R" goes here...
					{
						bVerifyYNFlag = false;

						if (strEntry.CompareNoCase(L"D") == 0) // If the other choice happens to be "D"...
						{
							Display.Log(L"Drain Selected By Operator");
							bVerifyYNFlag = true;
							bJumpOutToDrainFlag = true;
						}
					}
					Sleep(250);
				}
			}
			Sleep(250);

		}
		bInitFillWaitDone = false;
		Display.KillEntry();
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	}

	if (!bKillTheTaskFlag)
	{
		csVar2.Format(L"End Fill at %-5.1f%%", flCurrentWaterLevel);
		Display.Log(csVar2, L"", SendLogStatus);

		Display.KillEntry();
		Sleep(100);
		Display.Display(20, L"Waiting for Machine");
		Display.Display(21, L"To Reach");
		Display.Display(22, L"");
		if (flInitialPressure > 0.0)
			Display.Display(23, L"Pressure = " + toStr(flInitialPressure) + L" psi");
		Display.Display(24, L"PreHeat Temp = " + TempFtoStr(flDisplayPreheat));
		Display.DisplayStatus(L"Please Wait...");
		Display.Entry(0, 0, TRUE);
	}

	bool bLongPressureTimeFlag = false;
	float flLongPressureTime = 300; // 5 min max
	CString csVar3 = L"", csVar4 = L"";

	if (!bKillTheTaskFlag && !bPressPIDLoopStarted)
	{
		PinesTime1.StartTimer();	// Phase, Step, OR Segment Timer (sec)
		PinesTime1.ZeroTime();

		PID.PIDSetPoint(AO_SPLT_RNG, flInitialPressure, 0.0);
		PID.PIDStart(AO_SPLT_RNG, 9);
		bPressPIDLoopStarted = true;
	}



	// Loop Here Until -> Temp + Pressure & NOT Killed
	while ((flCurrentPressure < (flInitialPressure - (float)0.25)) && !bKillTheTaskFlag)
	{
		if (PinesTime1.GetElapsedTime() > flLongPressureTime && !bLongPressureTimeFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"Long Pressurizing Time!", SendLogStatus);
			sRetortAlarmMessage = L"Long Pressurizing Time!";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bLongPressureTimeFlag = true;
		}
		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;
		Sleep(250);
		flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
	} // Done - Clean Up and go on to next phase...

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	if (!bKillTheTaskFlag)
	{
		if (flInitialPressure > 0)
			Display.Log(L"At or above Initial Pressure", L"", SendLogStatus);
		Profiler.StartProfiler();

		Display.KillEntry();
		Sleep(100);
		Display.Display(20, L"Waiting for Machine");
		Display.Display(21, L"To Reach");
		Display.Display(22, L"");
		Display.Display(23, L"");
		Display.Display(24, L"PreHeat Temp = " + TempFtoStr(flDisplayPreheat));
		Display.DisplayStatus(L"Please Wait...");
		Display.Entry(0, 0, TRUE);
	}

	if (flCurrentPreHeatTemp < flPreHeatTemp)
	{
		bPreHeatTaskCompleteFlag = false;
		TaskLoop.SpawnTask((long)PreHeatTask, 10);
	}
	else
	{
		bPreHeatTaskCompleteFlag = true; //In case of override or shutdown the PreHeatTask does not start but
										//we set the flag true so the code is not stuck at the end of OpEntryPhase
	}

	while (!bPreHeatTaskCompleteFlag && !bKillTheTaskFlag)
		Sleep(250);

	if ((flCurrentWaterLevel > (flInitialFillLevel + 2)) && !bKillTheTaskFlag)
	{
		Display.KillEntry();

		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);

		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);

		Display.Alarm(2, 1, LOGTECAlarm, L"Water Level Is Too High", SendLogStatus);
		sRetortAlarmMessage = L"Water Level Is Too High";  
		sAlarmMessage_Hold = sRetortAlarmMessage;
		bAlarmsUpdate_iOPS = true;

		Display.Display(21, L"The Retort Has Filled");
		Display.Display(22, L"With Too Much Water");
		Display.Display(23, L"Please Wait While Level Is Adjusted.");
		Display.Display(24, L"- Draining Water -");
		Display.Entry(0, 0, TRUE);

		while ((flCurrentWaterLevel > (flInitialFillLevel + 2)) && !bKillTheTaskFlag)
		{
			Sleep(500);
		}
		Display.KillEntry();
	}

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);

	Display.Log(L"End Fill Phase");
	TaskLoop.KillTask(2);
	bFillCompleteFlag = true;
	bFillPhaseCompleteFlag = true;
} // End of FillPhase()

/*======================== PreHeatTask ===================================
Air Supply Valve Fixed at small constant % open - to mix with Steam
Air Vent PID Loop Set at small Pressure (Vent) SP to limit pressure
Done when: initial Temp & Pressure Satisfied
NEVER ADD WATER - once FILLED
======================== PreHeatTask ===================================*/

void PreHeatTask()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bPreHeatTaskCompleteFlag	= false;	// Set false here & then true at end of function
	bRunLevelControlFlag		= false;	// Separate Task Loop Monitors Level
	bDoDeviationCheck			= false;	// Set in COOK
	bCookTimeCompleted			= false;
	bPreheatRunning				= true;

	CommonInit();

	Display.Phase(L"PreHeat");
	sPhaseMessage = L"PreHeat"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 4)) 
	{
		Display.Log(L"Failure to set Phase to PreHeat");
	}

	// *** END OF COMMON INITIALIZATION STUFF

	bool bLongPreheatAlmFlag = false;

	// ===== Check doors once more ====

	if (DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT))
	{
		Display.KillEntry(); 

		Display.Display(21, L"Lockpin not extended.");
		Display.Display(22, L" ");
		Display.Display(23, L"Please close and seal door");
		Display.Display(24, L" ");
		Display.Entry(0, 0);

		while (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR) && !bKillTheTaskFlag)
		{
			TurnAllAnalogOutputsOff();
			TurnAllDigitalOutputsOff();
			Sleep(250);
		}
	}

	CString csVar1, csVar2, csVar3, csVar4;
	bool bLongPreHeatTimeFlag = false, bNotStarted = false;
	float flLongPreHeatTime = 600; // 10 min max

	Sleep(1000);

	if (!bKillTheTaskFlag)
	{
		PinesTime1.StartTimer();	// Phase, Step, OR Segment Timer (sec)
		PinesTime1.ZeroTime();
	}

	while ((flCurrentPreHeatTemp < flPreHeatTemp) && !bKillTheTaskFlag) //|| (flCurrentPressure < (flInitialPressure - (float) 0.5)))
	{

		//***************************Cushion the water in circ pump (prevent crashing) *********************
		PID.PIDSetPoint(AO_VENT, flInitialPressure, 0.0);
		PID.PIDStart(AO_VENT, 10);
		//***********************************************************************************************

		if (flCurrentPreHeatTemp < flPreHeatTemp)
		{
			DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Onn);
			AnalogIO.SetAnalogOut(AO_AIR, 5.0);	
		}
		else
		{
			DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
			AnalogIO.SetAnalogOut(AO_AIR, 0.0);			
		}

		if (PinesTime1.GetElapsedTime() > flLongPreHeatTime && !bLongPreheatAlmFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"Long Pre-Heating Time!", SendLogStatus);
			sRetortAlarmMessage = L"Long Pre-Heating Time!";  //V5
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;
			bLongPreheatAlmFlag = true;
			Sleep(250);
		}
		// Drain off water level if condensate fills too much
		if (flCurrentWaterLevel > (flInitialFillLevel + 2))
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
		if (flCurrentWaterLevel < (flInitialFillLevel + 0.5))
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);

		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;
		Sleep(250);
		flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_VENT);
	} // Done - Clean Up and go on to next phase...

	DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	bPreHeatOn = false;

	// Added to set the time on the main display screen
	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;

	// *** Clean up here - leave things in a safe state ***
	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	TaskLoop.KillTask(10);
	bPreheatRunning = false;
	bPreHeatTaskCompleteFlag = true;
}

/*======================== ComeUpPhaseSWS ===================================
Start Pump
Air Supply PID Loop & Air Vent PID Loop Set with 1 PSI separation
Done when: initial Temp & Pressure Satisfied for all segment steps
======================== ComeUpPhaseSWS ===================================*/
void ComeUpPhaseSWSTemperature()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bCUTempCompleteFlag		= false;	// Unique: Set false here & then true at end of function
	bCUTempRunningFlag		= true;
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		= false;	// Set in COOK
	bRunLevelControlFlag	= false;	// Delay - until level stabilized, then start separate task loop to control level
	bFillLevelAdjustFlag	= false;
	bIsInIdle				= false;	//machine is idle (true) only before Fill-phase and after Drain-phase.
	bIsInProcess			= true;		//machine is in process starting at come-up to end of cooling.

	CommonInit();
	// *** END OF COMMON INITIALIZATION STUFF

	Display.Phase(L"PumpStab");
	sPhaseMessage = L"PumpStab"; 

	float	flTempVar = 0.0;

	int		iSegNum = 0,
		iPVTable = 0;

	float	flOvershootTemperature = 0.0,
		flITCheckTemp = 0.0;

	float	flSegStartTemp = 0.0, flSegEndTemp = 0.0, flFinalTempSetpoint = 0.0;
	float	flSegStartPressure = 0.0, flSegEndPressure = 0.0;
	long	flSegTime = 0, lVal = 0;
	bool bSetOnce = false, bLongPreheatTimeFlag = false;
	CString csTmp, csVar1, csVar2, csVar3, csVar4, csVar5, csVar6, csVar7, csVar8, csVar9; 


	//*** First - a brief water stabilizing level delay. ***
	PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
	PinesTime1.ZeroTime();

	// Begin Agitation for ComeUp Phase
	flMotionProfile = flComeUpMotionProfile;
	csVar1 = L"Motion Profile for CU = " + toStr(flMotionProfile);
	Sleep(100);
	DigitalIO.SetDigitalOut(VDO_AgitationReq, Onn);	



	Display.KillEntry();
	Sleep(100);
	Display.Display(21, L"Pump Started - Stabilizing level for 20 sec...");
	Display.Display(22, L"Minimum Heating Flow Rate is " + toStr(flHeatingWaterLowFlowRate) + L"GPM");
	if (flMotionProfile > 0)
		Display.Display(22, csVar1);
	Display.Entry(0, 0, TRUE);

	while (!bKillTheTaskFlag &&
		((PinesTime1.GetElapsedTime() < 20) || (flCurrentWaterFlow < flHeatingWaterLowFlowRate)))
	{
		Sleep(250);
		// Added to set the time on the main display screen
		lCurStepSecSP = 20;

		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		// ========== Check AI_PREHT_RTD AGAINST INITIAL TEMPERATURE ========
		flITCheckTemp = flInitialTemperature;

		if (flCurrentPreHeatTemp < flITCheckTemp)
		{
			Display.KillEntry();
			Display.Alarm(2, 1, LOGTECAlarm, L"Water Temp Below Min Initial Temp!", DEVIATION);
			sRetortAlarmMessage = L"Water Temp Below Min Initial Temp!";  
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			Display.Display(21, L"Low Water Temperature!!!");
			Display.Display(22, L"Heating Water to");
			Display.Display(23, L"PreHeat Temp = " + TempFtoStr(flPreHeatTemp));
			Display.Display(24, L"Please Wait...");
			Display.DisplayStatus(L"Waiting...");
			Display.Entry(0, 0, TRUE);

			while ((flCurrentPreHeatTemp < flITCheckTemp) && !bKillTheTaskFlag)
			{
				if (flCurrentPreHeatTemp < flITCheckTemp && !bPreHeatOn)
				{
					DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Onn);
					bPreHeatOn = true;
				}
				else
				{
					DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
					bPreHeatOn = false;
				}
				Sleep(250);

				if ((PinesTime1.GetElapsedTime() > 120) && !bLongPreheatTimeFlag)
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"Long Pre-Heating Time!", SendLogStatus);
					sRetortAlarmMessage = L"Long Pre-Heating Time!";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bLongPreheatTimeFlag = true;
				}
			}
			DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
			bPreHeatOn = false;
		}
	}
	Display.KillEntry();
	//*** End of stabilizing level delay period

	///////////////////////////////////////////////////Acid Wash Add Acid Logic ///////////////////////////////////////////////////////
	//Operator Prompt to Add Acid after pump has been started and before come up begins.
	if (bCleaningRecipeSelected)
	{
		Display.Alarm(2, 1, LOGTECAlarm, L"Ready for Cleaning Agent");
		sRetortAlarmMessage = L"Ready for Cleaning Agent";  //RH v.iOPS
		sAlarmMessage_Hold = sRetortAlarmMessage;
		bAlarmsUpdate_iOPS = true;

		Display.KillEntry();
		Display.Display(20, L"Add Cleaning Agent Now");
		Display.Display(21, L"When Complete Press Enter");
		Display.Display(22, L"To Go To Come Up");
		Display.DisplayStatus(L"Waiting for Enter Key to Continue");
		Display.Entry(0, 0);

		while (!bKillTheTaskFlag && !Display.IsEnterKeyPressed())
			Sleep(250);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	Display.Phase(L"ComeUp");
	sPhaseMessage = L"ComeUp"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 5)) 
	{
		Display.Log(L"Failure to set Phase to Come Up");
	}
	Display.Log(L"Phase Started: ComeUp", L"", NORMAL);

	if (!bComeUpTimerRunning)
	{
		bComeUpTimerRunning = true;
		PinesTime5.StartTimer(); // Total ComeUp through CoolDown Time
		PinesTime5.ZeroTime();
	}

	//Reset value of iSegNum for Loop below and set Bit Identifiers to false
	iSegNum = 1;
	bComeUpTempStep1 = false;
	bComeUpTempStep2 = false;
	bComeUpTempStep3 = false;
	bComeUpTempStep4 = false;
	bComeUpTempStep5 = false;

	//*** Phase STEP Logic ***
	for (iSegNum = 1; iSegNum <= 5; iSegNum++) // Set up each segment, load, then wait til satisfied, then increment
	{
		Display.KillEntry();
		if (bKillTheTaskFlag)
			break;

		bool	bLongComeUpTimeFlag = false;

		bSetOnce = false;

		switch (iSegNum) // Each Phase Step via Process Variables
		{
		case 1: 
		{
			flSegStartTemp = flCurrentTemperature;
			flSegEndTemp = flComeUpTempStep1;
			flSegTime = (long)flComeUpTempTime1;
			bComeUpTempStep1 = true;
			break;
		}
		case 2:
		{
			flSegStartTemp = flComeUpTempStep1;
			flSegEndTemp = flComeUpTempStep2;
			flSegTime = (long)flComeUpTempTime2;
			bComeUpTempStep1 = false;
			bComeUpTempStep2 = true;
			break;
		}
		case 3: // Begin via Preheat
		{
			flSegStartTemp = flComeUpTempStep2;
			flSegEndTemp = flComeUpTempStep3;
			flSegTime = (long)flComeUpTempTime3;
			bComeUpTempStep2 = false;
			bComeUpTempStep3 = true;
			break;
		}
		case 4:
		{
			flSegStartTemp = flComeUpTempStep3;
			flSegEndTemp = flComeUpTempStep4;
			flSegTime = (long)flComeUpTempTime4;
			bComeUpTempStep3 = false;
			bComeUpTempStep4 = true;
			break;
		}
		case 5:
		{
			flSegStartTemp = flComeUpTempStep4;
			flSegEndTemp = flComeUpTempStep5;
			flSegTime = (long)flComeUpTempTime5;
			bComeUpTempStep4 = false;
			bComeUpTempStep5 = true;
			break;
		}
		}
		// Common Display/Log Stuff for each Phase Step
		flLongSegmentTime = (float)flSegTime * (float)2.0; // For Excess Time alarm 150% of Segment time
		if (flSegTime != 0 && flSegEndTemp != 0) // SKIP? Disabled if Step Time is zero
		{
			Display.KillEntry(); 

			csVar1.Format(L"%-5.1f", flSegEndTemp);
			csVar2.Format(L"ComeUp%d", iSegNum);
			//csVar6.Format(L"%-5.1f", flSegEndPressure);
			csVar3.Format(L"This Step = %d Seconds", flSegTime);
			csVar4 = "Final Temperature = " + csVar1;
			//csVar5="Final Pressure = "+ csVar6;
			csVar7.Format(L"Temperature ComeUp Step %d", iSegNum);
			Display.Display(20, csVar7);
			Display.Display(21, csVar3);
			Display.Display(22, csVar4);
			Display.Entry(0, 0, TRUE);

			csVar8.Format(L"Step %d Temp Ramp to " + csVar1 +"°F in % d Sec", iSegNum, flSegTime);
			Display.Log(csVar8, L"", SendLogStatus);

			PinesTime1.StartTimer(); // Phase, Step, or Segment Timer (sec)
			PinesTime1.ZeroTime();

			PID.PIDSetPoint(AO_STEAM, flSegStartTemp, 0);
			//Use different tuning parameters if the vessel is lees than 200 degrees
			if (flCurrentTemperature < 200)
				nCorrectTuningTable = SelectRTDTuningTable(1, 2);
			else
				nCorrectTuningTable = SelectRTDTuningTable(3, 4);

			PID.PIDStart(AO_STEAM, nCorrectTuningTable);
			flFinalTempSetpoint = flSegEndTemp + (float)1.0;

			flOvershootTemperature = flSegEndTemp;

			PID.RampSetpointPeriod(AO_STEAM, flFinalTempSetpoint, flSegTime);
			// Temp + Pressure + Time = then were done...with ThiS segment
			while (!bKillTheTaskFlag && (
				flCurrentTemperature < flSegEndTemp || PinesTime1.GetElapsedTime() < flSegTime))
			{
				if (PinesTime5.GetElapsedTime() >= 30.0)
				{
					bRunLevelControlFlag = true;
				}
				if (PinesTime5.GetElapsedTime() >= 60.0)
				{
					bCheckCriticalPSIFlag = true;	// start checking vapor PSI
				}
				if (PinesTime5.GetElapsedTime() >= 180.0)
				{
					bFillLevelAdjustFlag = true;	// allow time to adjust level for number of baskets. GMK
				}
				if (bUpdateToCorrectRTD) // Test if RTD Failed? if so - change table accordingly
				{
					nCorrectTuningTable = SelectRTDTuningTable(3, 4);
					PID.FetchTuningTable(AO_STEAM, nCorrectTuningTable);
				}
				if (PinesTime1.GetElapsedTime() > flLongSegmentTime && !bLongComeUpTimeFlag)
				{
					csTmp.Format(L"Long Temperature ComeUp Time - Step %d", iSegNum);
					Display.Alarm(2, 1, LOGTECAlarm, csTmp, SendLogStatus);
					sRetortAlarmMessage = L"Long Temperature ComeUp Time";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bLongComeUpTimeFlag = true;
				}
				flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
				// Added to set the time on the main display screen
				lCurStepSecSP = flSegTime;

				if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
				{
					lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
				}
				else
					lCurStepSecRemain = 0;

				if (bAddCookTimeFlag)
				{
					Display.KillEntry();
					AddCookTime();
				}
				if ((flCurrentWaterLevel > flWaterDrainOnLevel) && !bCleaningRecipeSelected)
				{
					if(flCurrentTemperature < 190.0)
						DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
					else
						DigitalIO.SetDigitalOut(DO_COND_VLV, Onn);
				}

				if (flCurrentWaterLevel < flWaterDrainOffLevel)
				{
					DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
					DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
				}
				if (flCurrentTemperature > flSteamBoostOff)
				{
					DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
				}
				Sleep(250);
			} // End of This Phase STEP
			Sleep(250);

			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
			DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		}
		else
		{
			break;
		}
	} // End of for Loop for all Phase STEPS

	bComeUpTempStep1 = false;
	bComeUpTempStep2 = false;
	bComeUpTempStep3 = false;
	bComeUpTempStep4 = false;
	bComeUpTempStep5 = false;

	PID.PIDSetPoint(AO_STEAM, flFinalTempSetpoint, 0);
	flDisplayLoopTempSP = flFinalTempSetpoint;

	DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);

	Display.KillEntry();

	// **** BEGIN OVERSHOOT DURATION ******************************
	bool	bTemperatureDropInOvershoot = false;
	int		iNumberOfOvershootTempAlarms = 0;

	PinesTime1.StartTimer(); // Phase, Step, or Segment Timer (sec)
	PinesTime1.ZeroTime();

	if (!bKillTheTaskFlag && flOverShootTime > 0)
	{
		Display.Log(L"Begin Overshoot Time", L"", SendLogStatus);

		// Added to set the time on the main display screen
		lCurStepSecSP = (long)flOverShootTime;

		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		Display.Display(20, L"Overshoot Segment");
		csVar7.Format(L"Waiting for %d seconds", (long)flOverShootTime);
		Display.Display(21, csVar7);
		Display.Entry(0, 0, TRUE);

		// Alarm and Reset the Overshoot Time if Temperature Drops below Overshoot Temperature
		bTemperatureDropInOvershoot = false;	// reset flag


		while (!bKillTheTaskFlag && ((flCurrentTemperature <= (flOvershootTemperature + (float)0.5))
			|| (PinesTime1.GetElapsedTime() < flOverShootTime)))
		{
			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;

			// Alarm if Overshoot Temperature drops below last segment's temperature set point
			if ((flCurrentTemperature < flOvershootTemperature) && !bTemperatureDropInOvershoot)
			{
				bTemperatureDropInOvershoot = true;
				Display.Alarm(2, 1, LOGTECAlarm, L"Low Temperature in Overshoot");
				sRetortAlarmMessage = L"Low Temperature in Overshoot";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				Display.OverrideText(4, L"Cook", CookLabel);
				PinesTime1.ZeroTime();
				PinesTime1.StopTimer();

				//Add the ability for an override into Cook. If we override into Cook then we need to Deviate.
			}
			// reset Overshoot time and increment alarm count
			if ((flCurrentTemperature >= (flOvershootTemperature + (float)0.3))
				&& !bKillTheTaskFlag && bTemperatureDropInOvershoot)
			{
				PinesTime1.StartTimer(); // Phase, Step, or Segment Timer (sec)
				PinesTime1.ZeroTime();
				bTemperatureDropInOvershoot = false;
			}
			if ((flCurrentWaterLevel > flWaterDrainOnLevel) && !bCleaningRecipeSelected)
			{
				if (flCurrentTemperature < 190.0)
					DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
				else
					DigitalIO.SetDigitalOut(DO_COND_VLV, Onn);
			}

			if (flCurrentWaterLevel < flWaterDrainOffLevel)
			{
				DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
				DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
			}
			Sleep(250);
		}
	}
	Display.KillEntry();

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	DigitalIO.SetDigitalOut(DO_COND_VLV, Off);

	// **** END OVERSHOOT DURATION ******************************

	// *** Clean up here - leave things in a safe state ***

	// Kill Only PID Loops That Should be killed.
	if (bKillTheTaskFlag)
		PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);

	// Added to set the time on the main display screen
	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	Display.KillEntry();
	TaskLoop.KillTask(3);
	PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	bCUTempCompleteFlag = true;
	bCUTempRunningFlag = false;
	Sleep(200);
	bPhaseCompleteFlag = true;

}	// End of Come-Up - SWS TEMPERATURE

/*==================
SPLIT OF INTO TWO TASKS - TEMP & PRESSURE
==================*/
void ComeUpPhaseSWSPSI()
{
	float	flTempVar = 0.0;
	int		iSegNum = 0, iPVTable = 0;
	bool	bPIDSetFlag = false;

	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bCUPSICompleteFlag			= false;	// Unique: Set false here & then true at end of function
	bCUPSIPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillCUPSITaskFlag			= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS


	bool bNotNeetToRampPress = false;

	// *** END OF COMMON INITIALIZATION STUFF

	if (bSplitRangeCompleteFlag)	// Should be running - used to Reinitialize upon override.
	{
		bKillSplitRangeFlag = false;
		TaskLoop.SpawnTask((long)SplitRangeControl, 8);
	}
	PinesTime14.StartTimer(); // ComeUp phase timer
	PinesTime14.ZeroTime();

	// this is ONLY the PRESSURE loop (task #7), running in parallel with the Temperature (task #3)
	float	flSegStartPressure = 0.0, flSegEndPressure = 0.0, flFinalCUPressSetpoint = 0.0;
	long	flSegTime = 0, lVal = 0;
	bool bSetOnce = false;
	CString csTmp, csVar1, csVar2, csVar3, csVar4, csVar5, csVar6, csVar7, csVar8;

	while (!bKillCUPSITaskFlag && !bRunLevelControlFlag)
	{
		Sleep(250);
	}
	//*** End of stabilizing level delay period

	bHotPSIStepA = false;
	bHotPSIStepB = false;
	bHotPSIStepC = false;
	bHotPSIStepD = false;
	bHotPSIStepE = false;
	bHotPSIStepF = false;

	//*** Phase STEP Logic ***
	for (iSegNum = 1; iSegNum <= 6; iSegNum++) // Set up each segment, load, then wait til satisfied, then increment
	{
		if (bKillTheTaskFlag)
			break;

		bool bLongComeUpTimeFlag = false;
		bSetOnce = false;

		switch (iSegNum) // Each Phase Step via Process Variables
		{
		case 1: // Begin via Preheat
		{
			sStepLetter = L"A";
			if (flCurrentPressure < flInitialPressure + 2)
				flSegStartPressure = flInitialPressure;
			else flSegStartPressure = flCurrentPressure;

			if (flCurrentPressure < flHotPSIStepA)
				flSegEndPressure = flHotPSIStepA;
			else flSegEndPressure = flCurrentPressure;

			//flSegTime			= (long)(flHotPSITimeA > flComeUpPSIHoldTimeA ? flHotPSITimeA : flComeUpPSIHoldTimeA); // RIP - OK This is a "Ternery operator". "If a > b, then a, else b"
			flSegTime = (long)flHotPSITimeA;
			bHotPSIStepA = true;
			break;
		}
		case 2:
		{
			sStepLetter = L"B";
			if (flCurrentPressure < flHotPSIStepA + 2)
				flSegStartPressure = flHotPSIStepA;
			else flSegStartPressure = flCurrentPressure;

			if (flCurrentPressure < flHotPSIStepB)
				flSegEndPressure = flHotPSIStepB;
			else flSegEndPressure = flCurrentPressure;

			flSegTime = (long)flHotPSITimeB;
			bHotPSIStepA = false;
			bHotPSIStepB = true;
			break;
		}
		case 3:
		{
			sStepLetter = L"C";
			if (flCurrentPressure < flHotPSIStepB + 1)
				flSegStartPressure = flHotPSIStepB;
			else flSegStartPressure = flCurrentPressure;

			if (flCurrentPressure < flHotPSIStepC)
				flSegEndPressure = flHotPSIStepC;
			else flSegEndPressure = flCurrentPressure;

			//flSegTime			= (long)(flHotPSITimeA > flComeUpPSIHoldTimeA ? flHotPSITimeA : flComeUpPSIHoldTimeA); // RIP - OK This is a "Ternery operator". "If a > b, then a, else b"
			flSegTime = (long)flHotPSITimeC;
			bHotPSIStepB = false;
			bHotPSIStepC = true;
			break;
		}
		case 4:
		{
			sStepLetter = L"D";
			if (flCurrentPressure < flHotPSIStepC)
				flSegStartPressure = flHotPSIStepC;
			else flSegStartPressure = flCurrentPressure;

			if (flCurrentPressure < flHotPSIStepD)
				flSegEndPressure = flHotPSIStepD;
			else flSegEndPressure = flCurrentPressure;

			flSegTime = (long)flHotPSITimeD;
			bHotPSIStepC = false;
			bHotPSIStepD = true;
			break;
		}
		case 5:
		{
			sStepLetter = L"E";
			if (flCurrentPressure < flHotPSIStepD)
				flSegStartPressure = flHotPSIStepD;
			else flSegStartPressure = flCurrentPressure;

			if (flCurrentPressure < flHotPSIStepE)
				flSegEndPressure = flHotPSIStepE;
			else flSegEndPressure = flCurrentPressure;

			flSegTime = (long)flHotPSITimeE;
			bHotPSIStepD = false;
			bHotPSIStepE = true;
			break;
		}
		case 6:
		{
			sStepLetter = L"F";
			flSegStartPressure = flCurrentPressure;
			flSegEndPressure = flHotPSIStepF;
			flSegTime = (long)flHotPSITimeF;
			bHotPSIStepE = false;
			bHotPSIStepF = true;
			break;
		}
		}
		// Common Display/Log Stuff for each Phase Step
		float flLongSegmentPSITime = (float)flSegTime * (float)2.0; // For Excess Time alarm 200% of Segment time
		if (flSegTime != 0 && flSegEndPressure != 0) // SKIP? Disabled if Step Time is zero
		{
			csVar6.Format(L"%-5.1f", flSegEndPressure);
			csVar8.Format(L"Step " + sStepLetter +" Pressure Ramp to " + csVar6 + "PSI in %d Sec", flSegTime);
			Display.Log(csVar8, L"", SendLogStatus);

			PinesTime14.StartTimer();
			PinesTime14.ZeroTime();

			PID.PIDSetPoint(AO_SPLT_RNG, flSegStartPressure, 0.0);
			PID.PIDStart(AO_SPLT_RNG, 11);
			

			flFinalCUPressSetpoint = flSegEndPressure;

			PID.RampSetpointPeriod(AO_SPLT_RNG, flSegEndPressure, flSegTime);
			// Temp + Pressure + Time = then were done...with THIS segment
			while (!bKillCUPSITaskFlag &&
				(flCurrentPressure < (flSegEndPressure - (float)0.5) || PinesTime14.GetElapsedTime() < flSegTime))
			{
				if (PinesTime14.GetElapsedTime() > flLongSegmentPSITime && !bLongComeUpTimeFlag)
				{
					csTmp.Format(L"Long Pressure Come-Up Time - Step %d", iSegNum);
					Display.Alarm(2, 1, LOGTECAlarm, csTmp, SendLogStatus);
					sRetortAlarmMessage = L"Long Pressure Come-Up Time";  
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bLongComeUpTimeFlag = true;
				}
				flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);

				Sleep(250);
			} // End of This Phase STEP
			Sleep(250);
		}
		else
		{
			break;
		}
	} // End of for Loop for all Phase STEPS

	bHotPSIStepA = false;
	bHotPSIStepB = false;
	bHotPSIStepC = false;
	bHotPSIStepD = false;
	bHotPSIStepE = false;
	bHotPSIStepF = false;

	PID.PIDSetPoint(AO_SPLT_RNG, flFinalCUPressSetpoint, 0.0);
	flDisplayLoopPsiSP = flFinalCUPressSetpoint;

	//flDisplayLoopPsiSP	= 0;
	// Added to set the time on the main display screen

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	TaskLoop.KillTask(7);
	PinesTime14.StopTimer();	// Phase, Step, OR Segment Timer (sec)
	bCUPSICompleteFlag = true;	//CUPSI continues up to Cool Phase
	Sleep(200);
	bCUPSIPhaseCompleteFlag = true; //CUPSI continues up to Cool Phase
}	// End of ComeUpPhaseSWSPSI()


/*======================== CookPhase ===================================
Pump ON
Stabilization Time = Ramp Down To CTEMP Step (sec)
Reset Cook Timer (& log) during Stab Time, if Temp less than CTEMP; then, DEVIATION
Water Flow Control restarted, at end of cook, (set false, then again true in cooling) to reset filter timer
Ask For Operator - at end of Stab Time AND & End of Cook - 10min (Only if > 30 Min cook).
Done when: Cook Temp & Time is Satisfied
in steam Cook Display Message -> check bleeders on screen?
======================== CookPhase ===================================*/
void CookPhase()
{
	int	iSegNum = 0;
	float	flTempVar = 0.0;
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bCookCompleteFlag	= false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag	= false;	// Set false here & then true at end of function
	bKillTheTaskFlag	= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bIsInProcess		= true;		//machine is in process starting at come-up to end of cooling.
	bIsInIdle			= false;	//machine is idle (true) only before Fill-phase and after Drain-phase.
	bCookRunning		= true;

	if (bSplitRangeCompleteFlag && (bSWSOnlyRecipeFlag || bCleaningRecipeSelected))	// Call if required for cook
	{
		bKillSplitRangeFlag = false;
		TaskLoop.SpawnTask((long)SplitRangeControl, 8);
	}
	//If pump was off, allow for the flow to stabilize before activating level control
	//and alarm functions.
	if (!DigitalIO.ReadDigitalOut(DO_PUMP_1) || !DigitalIO.ReadDigitalOut(DO_PUMP_2))
	{
		DigitalIO.SetDigitalOut(DO_PUMP_1, Onn);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Onn);
		bPumpIsRunning = true;

		Display.KillEntry(); 

		Display.Display(20, L"Waiting for minimum flow rate");
		Display.Entry(0, 0, TRUE);

		while ((AnalogIO.ReadAnalogIn(AI_FLOW) < flHeatingWaterLowFlowRate) &&
			!bKillTheTaskFlag)
			Sleep(250);

		Display.KillEntry();
	}

	//Pressure, level, and flow must be running prior to start the phase timer.
	Display.Phase(L"Cook");
	sPhaseMessage = L"Cook"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 6)) 
	{
		Display.Log(L"Failure to set Phase to Cook");
	}
	Display.Log(L"Begin Cook");

	CommonInit();
	// *** END OF COMMON INITIALIZATION STUFF

	bool bCookLogTwiceFlag = false, bOperatorPromptComplete = false, bDoOnce = false;
	bool bRPMOneChecFlag = false; // Chech the RPM difference only once

	CString csTmp, csVar1, csVar2, csVar3, csVar4, csVar5, csVar6, csVar7, csVar8,
		csVar9, csVar10, csVar11, csVar12; // added 8 and 9 for 2 PSI stages, and 10,11, for tracking Low Temp & 12 for RPM Check

	float	flSegStartPressure = 0.0, flSegEndPressure = 0.0,
		flInversionDelay = 0.0, flNextInversion = 0.0;
	int		iNumOfInv = 0, iInvDone = 0;
	long	lVal = 0;

	bDoDeviationCheck = false;	// Set in COOK
	bProcessStarted = true;
	bCheckForMinPSIFlag = true;

	// Now Begin Agitation for the Cook Phase
	if (flCookMotionProfile != flMotionProfile)
	{
		flNewMP = flCookMotionProfile;
		bTriggerEAChange = true;
	}
	

	PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
	PinesTime1.ZeroTime();

	flNewCTEMP = flCTEMP + (float)1.0;

	// Temp Setpoint via where we left off - and on to CTEMP over Stabilization Time
	PID.PIDSetPoint(AO_STEAM, PID.GetPIDSetPoint(AO_STEAM), 0);
	nCorrectTuningTable = SelectRTDTuningTable(7, 8);
	PID.PIDStart(AO_STEAM, nCorrectTuningTable);
	PID.RampSetpointPeriod(AO_STEAM, flCTEMP + (float)1.0, (long)flRampDownToCTEMPTime);

	// Correct the stabilization time and alarm
	if (flCookStabilizationTime > (flCTIME * 60))
	{
		flCookStabilizationTime = (float)((flCTIME - (float)2.0) * (float)60.0);
		Display.Alarm(2, 1, LOGTECAlarm, L" Stabilization Time Corrected", SendLogStatus);
		sRetortAlarmMessage = L" Stabilization Time Corrected";  
		sAlarmMessage_Hold = sRetortAlarmMessage;
		bAlarmsUpdate_iOPS = true;
	}
	csVar1.Format(L"%-5.1f", flCTEMP + 1.0);
	csVar2 = strCTIME;

	csVar6.Format(L"Stabilization Time = %d Seconds...", (long)flCookStabilizationTime);
	csVar3.Format(L"Ramping Down to Cook Temp in %d Seconds...", (int)flRampDownToCTEMPTime);
	csVar4 = "For a Cook Time of " + csVar2;
	csVar5 = " At a Temperature of " + csVar1;

	Display.KillEntry(); 

	Display.Display(20, csVar6);
	Display.Display(21, csVar3);
	Display.Display(22, csVar4);
	Display.Display(23, csVar5);
	Display.Entry(0, 0, TRUE);

	// *** Stabilization Step (sec) - No DEVIATIONs & Reset Cook Clock if below CTEMP ***
	while (PinesTime1.GetElapsedTime() < flCookStabilizationTime && !bKillTheTaskFlag)
	{
		if (!bDoOnce && PinesTime1.GetElapsedTime() > flRampDownToCTEMPTime)
		{
			bDoOnce = true;
			Display.KillEntry();

			Display.Display(20, csVar6);
			Display.Display(22, csVar4);
			Display.Display(23, csVar5);
			Display.Entry(0, 0, TRUE);
		}
		if (bUpdateToCorrectRTD) // Test if RTD Failed? if so - change table accordingly
		{
			nCorrectTuningTable = SelectRTDTuningTable(7, 8);
			PID.FetchTuningTable(AO_STEAM, nCorrectTuningTable); // via RTD-1 vs RTD-2
		}
		if (flCurrentTemperature < flCTEMP)
		{
			if (PinesTime1.GetElapsedTime() > 5) // Filter-Alarm ONLY this frequently. Hysteresis...
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"Low Cook Temperature Detected", SendLogStatus);
				sRetortAlarmMessage = L"Low Cook Temperature Detected";  
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;
				bLowCookTemperature = true;
			}
			while ((flCurrentTemperature < flCTEMP + (float)0.25) && !bKillTheTaskFlag)
			{
				Sleep(250);
			}
			PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
			PinesTime1.ZeroTime();
			Display.Phase(L"Cook");
			sPhaseMessage = L"Cook"; 
			if (!tcio.WriteINT(L"LTM.iPhase", 63)) 
			{
				Display.Log(L"Failure to set Phase to Cook");
			}
			Display.Log(L"Begin Cook");
		}
		flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
		flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
		// Added to set the time on the main display screen
		lCurStepSecSP = (long)flCookStabilizationTime;

		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		if (bAddCookTimeFlag)
		{
			Display.KillEntry();
			AddCookTime();
		}

		if ((flCurrentWaterLevel > flWaterDrainOnLevel) && !bCleaningRecipeSelected)
		{
			if (flCurrentTemperature < 190.0)
				DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
			else
				DigitalIO.SetDigitalOut(DO_COND_VLV, Onn);
		}

		if (flCurrentWaterLevel < flWaterDrainOffLevel)
		{
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
			DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		}
		Sleep(250);
	}
	// *** End StabilizationTime ***

	Display.KillEntry();

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	DigitalIO.SetDigitalOut(DO_COND_VLV, Off);

	bDisplayDeviationInfo = false;
	bTempDeviationFlag = false;
	flLowCookTemperatureTime = 0;
	bDoDeviationCheck = true;

	Display.OverrideText(2, L"Add Cook Time", AddCookTimeLabel);
	flCookLowestTemp = flCTEMP; // set the lowest temperature equal to cook temp

	//log cooking temp
	csVar10.Format(L"Cook Temp = %-5.1f °F", flCTEMP);
	Display.Log(csVar10);

	// *** Next COOK Step (sec) - DEVIATIONs if below CTEMP ***
	while ((PinesTime1.GetElapsedTime() < flCTIME
		|| !bOperatorPromptComplete || bTempDeviationFlag) && !bKillTheTaskFlag)
	{
		// First Op Entry: At Stability time plus 20 seconds
		// secondary if it happens here
		if (!bOperatorPromptComplete && PinesTime1.GetElapsedTime() >= (flCookStabilizationTime + 20))
		{
			PromptForCookOperatorEntries();
			bOperatorPromptComplete = true;
			//If MIG entries is recent, don't prompt a second time.
			if (fabs(flCTIME - PinesTime1.GetElapsedTime()) < 420) bCookLogTwiceFlag = true;
		}
		// Second Op Entry: if > 60 Min Cook, then at (CTIME - 5 min) we need a second Op Entry!
		// if(flCTIME > 3600) Removed per customer request
		if (bOperatorPromptComplete && fabs(flCTIME - PinesTime1.GetElapsedTime()) < 420 &&
			!bCookLogTwiceFlag && !bKillTheTaskFlag)
		{
			bCookLogTwiceFlag = true;
			PromptForCookOperatorEntries();
		}

		if (bAddCookTimeFlag)
		{
			Display.KillEntry();
			AddCookTime();
		}
		//This logic will only display at the end of cook if there were any temperature deviations and we are running a Generic process.
		//We are tracking the lowest temperature and the total time that we were in a deviation state.
		//This logic displays these values for the operator and then asks for them to add additional cook time.
		//If they add 0 minutes then the cook ends but if they add more time then we will wait until that time is up before ending cook.

		if (bDisplayDeviationInfo && PinesTime1.GetElapsedTime() > flCTIME)
		{
			bDisplayDeviationInfo = false;
			bTempDeviationFlag = false;
			Display.KillEntry();

			csVar10.Format(L"Lowest Temperature = %-5.1f °F", flCookLowestTemp);
			csVar11.Format(L"Total Deviation Time %-5.1f Sec", flLowCookTemperatureTime);
			Display.Log(csVar10);
			Display.Log(csVar11);

			Display.Display(20, L"-TEMPERATURE DEVIATION-");
			Display.Display(21, csVar10);
			Display.Display(22, csVar11);
			Display.Display(23, L"Press Enter to Continue...");
			Display.Entry(0, 0);

			while (!bKillTheTaskFlag && !Display.IsEnterKeyPressed())
			{
				flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
				flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
				// Added to set the time on the main display screen
				lCurStepSecSP = (long)flCTIME;

				if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
				{
					lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
				}
				else
					lCurStepSecRemain = 0;

				//Level Control
				if ((flCurrentWaterLevel > flWaterDrainOnLevel) && !bCleaningRecipeSelected)
				{
					if (flCurrentTemperature < 190.0)
						DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
					else
						DigitalIO.SetDigitalOut(DO_COND_VLV, Onn);
				}

				if (flCurrentWaterLevel < flWaterDrainOffLevel)
				{
					DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
					DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
				}

				Sleep(250);
			}
			Display.KillEntry();

			AddCookTime();
		}
		

		//Level Control
		if ((flCurrentWaterLevel > flWaterDrainOnLevel) && !bCleaningRecipeSelected)
		{
			if (flCurrentTemperature < 190.0)
				DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
			else
				DigitalIO.SetDigitalOut(DO_COND_VLV, Onn);
		}

		if (flCurrentWaterLevel < flWaterDrainOffLevel)
		{
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
			DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		}
		Sleep(250);
	}
	Display.KillEntry();

	// *** Clean up here - leave things in a safe state ***
	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);

	bDoDeviationCheck = false;

	flDisplayLoopTempSP = 0; // Reset this - for Display only
	flDisplayLoopPsiSP = 0;
	// Added to set the time on the main display screen
	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;

	/*** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***/
	if (!bKillTheTaskFlag)
		bCookTimeCompleted = true;
	Display.KillEntry();
	TaskLoop.KillTask(3);
	Sleep(150);

	PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	bKillCUPSITaskFlag = true;
	bCookCompleteFlag = true;
	bCheckForMinPSIFlag = false;
	Sleep(200);
	bPhaseCompleteFlag = true;
	bCookRunning = false;

	Display.Log(L"End of cook phase");

}	// End of CookPhase()

//======================== CoolingPhase ===================================
// Water Flow Control restarted, at end of cook, (set false, then again true in cooling) to reset filter timer
void CoolingPhaseTemperature()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bCoolTempCompleteFlag	= false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		= false;	// Set in COOK
	bRunLevelControlFlag	= true;		// Separate Task Loop Monitors Level
	bOKtoStartCoolingPSI	= true;		// flag that allows PSI cooling Phase to start
	bEnblCoolWaterFlow		= true;
	bCoolTempRunning		= true;

	float	flSegStartTemp = 0.0, flSegEndTemp = 0.0, flFinalCoolTempSetpoint = 0.0;
	long	flSegTime = 0;
	CString csTmp, csVar1, csVar2, csVar3, csVar4, csVar5, csVar6, csVar7, csVar8;
	bool bStopTimer12Flag = false, bSetOnce = false;
	flCoolingAcumulatedTime = 0.0;	// starts at zero every time we start cool and adds the time while we are cooling

	CommonInit();

	// *** END OF COMMON INITIALIZATION STUFF

	Display.Phase(L"Cooling");
	sPhaseMessage = L"Cooling"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 7)) 
	{
		Display.Log(L"Failure to set Phase to Cool");
	}
	Display.Log(L"Begin Cooling");

	PinesTime12.StartTimer();
	PinesTime12.ZeroTime();

	Sleep(150);

	if (flCoolingMotionProfile != flMotionProfile)
	{
		flNewMP = flCoolingMotionProfile;
		bTriggerEAChange = true;
	}

	// MICRO-COOL -> wait for pressure loop to stabilize

	PID.PIDSetPoint(AO_SPLT_RNG, flCurrentPressure, 0);
	PID.PIDStart(AO_SPLT_RNG, 12);

	Display.KillEntry(); 

	csVar3.Format(L"This Hold Period = %d Seconds", (long)flMicroCoolTime);
	Display.Display(20, L"Begin MicroCool...");
	Display.Display(21, csVar3);
	Display.Display(22, L"Please Wait...");
	Display.Entry(0, 0, TRUE);

	
	lCurStepSecSP = (long)flMicroCoolTime;
	PinesTime1.StartTimer(); // Delay - Phase, Step, or Segment Timer (sec)
	PinesTime1.ZeroTime();

	while (!bKillTheTaskFlag && PinesTime1.GetElapsedTime() < flMicroCoolTime)
	{
		
		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		Sleep(250);
	}
	// END OF MICRO-COOL

	PinesTime1.StopTimer();

	DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Off);
	DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Onn);

	bCOOLTempStep1 = false;
	bCOOLTempStep2 = false;
	bCOOLTempStep3 = false;
	bCOOLTempStep4 = false;

	//*** Phase STEP Logic ***
	for (int iSegNum = 1; iSegNum <= 4; iSegNum++) // Set up each segment, load, then wait til satisfied, then increment
	{
		Display.KillEntry();
		if (bKillTheTaskFlag)
			break;

		bool bLongCoolingTimeFlag = false;
		bSetOnce = false;

		switch (iSegNum) // Each Phase Step via Process Variables
		{
		case 1: 
		{
			flSegStartTemp = flCurrentTemperature;
			flSegEndTemp = flCOOLTempStep1;
			flSegTime = (long)flCOOLTempTime1;
			bCOOLTempStep1 = true;
			break;
		}
		case 2:
		{
			flSegStartTemp = flCOOLTempStep1;
			flSegEndTemp = flCOOLTempStep2;
			flSegTime = (long)flCOOLTempTime2;
			bCOOLTempStep1 = false;
			bCOOLTempStep2 = true;
			break;
		}
		case 3:
		{
			flSegStartTemp = flCOOLTempStep2;
			flSegEndTemp = flCOOLTempStep3;
			flSegTime = (long)flCOOLTempTime3;
			bCOOLTempStep2 = false;
			bCOOLTempStep3 = true;
			break;
		}
		case 4:
		{
			flSegStartTemp = flCOOLTempStep3;
			flSegEndTemp = flCOOLTempStep4;
			flSegTime = (long)flCOOLTempTime4;
			bCOOLTempStep3 = false;
			bCOOLTempStep4 = true;
			break;
		}
		}
		// Common Display/Log Stuff for each Phase Step
		flLongSegmentTime = (float)flSegTime * (float)2.0; // For Excess Time alarm % of Segment time
		if (flSegTime != 0 && flSegEndTemp != 0) // SKIP? Disabled if Step Time is zero
		{
			if (bStopTimer12Flag) //Restart the timer if it was stopped in the previous segment
			{
				PinesTime12.StartTimer();
				bStopTimer12Flag = false;
			}
			csVar1.Format(L"%-5.1f", flSegEndTemp);
			csVar2.Format(L"Cool Step %d Temp Ramp to %-5.1f °F in %d Sec", iSegNum, flSegEndTemp, flSegTime);
			csVar3.Format(L"This Step = %d Seconds", flSegTime);
			csVar4 = "Final Temperature = " + csVar1;
			Display.Log(csVar2, L"", SendLogStatus);
			csVar7.Format(L"Temperature Cool Step %d", iSegNum);
			Display.Display(20, csVar7);
			Display.Display(21, csVar3);
			Display.Display(22, csVar4);
			Display.Entry(0, 0, TRUE);

			PinesTime1.StartTimer(); // Delay - Phase, Step, or Segment Timer (sec)
			PinesTime1.ZeroTime();

			PID.PIDSetPoint(AO_WATER, flSegStartTemp, 0);

			nCorrectTuningTable = SelectRTDTuningTable(17,18);
			PID.PIDStart(AO_WATER, nCorrectTuningTable); // via RTD-1 vs RTD-2

			//flFinalCoolTempSetpoint = flSegEndTemp + (float) 1.0;
			flFinalCoolTempSetpoint = flSegEndTemp; //** GMK 03-06-06
			PID.RampSetpointPeriod(AO_WATER, flFinalCoolTempSetpoint, flSegTime);

			// Temp + Pressure + Time = then we're done...with THIS segment
			while (!bKillTheTaskFlag && (flCurrentTemperature > (flSegEndTemp + (float)0.5) || PinesTime1.GetElapsedTime() < flSegTime))
			{
				if (!bStopTimer12Flag && PinesTime1.GetElapsedTime() > flSegTime) //Stop the timer that starts the drain 5 min before cooling ends
				{
					bStopTimer12Flag = true;
					PinesTime12.StopTimer();
				}
				// Begin Check to halt the Pressure Ramping if temp cannot keep up
				if (!bCoolDelayHaltPSIRampFlag && !bAutoClaveRecipeFlag
					&& ((flCurrentTemperature - PID.GetPIDSetPoint(AO_WATER)) > 10))
				{
					bCoolDelayHaltPSIRampFlag = true;

					csTmp.Format(L"Temperature Lags by 10 °F in Step %d", iSegNum);
					Display.Alarm(2, 1, LOGTECAlarm, csTmp, SendLogStatus);
					sRetortAlarmMessage = L"Temperature Lags by 10 °F";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;
				}
				if (bCoolDelayHaltPSIRampFlag && ((flCurrentTemperature - PID.GetPIDSetPoint(AO_WATER)) < 2))
				{
					Display.Log(L"Temperature is OK");
					bCoolDelayHaltPSIRampFlag = false;
				}
				if (bUpdateToCorrectRTD) // Test if RTD Failed? if so - change table accordingly
				{
					nCorrectTuningTable = SelectRTDTuningTable(17, 18);
					PID.FetchTuningTable(AO_WATER, nCorrectTuningTable);
				}
				if (PinesTime1.GetElapsedTime() > flLongSegmentTime && !bLongCoolingTimeFlag)
				{
					csTmp.Format(L"Long Cooling Temp Step Time - Step %d", iSegNum);
					Display.Alarm(2, 1, LOGTECAlarm, csTmp, SendLogStatus);
					sRetortAlarmMessage = L"Long Cooling Temp Step Time";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bLongCoolingTimeFlag = true;
				}
				flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_WATER);
				// Added to set the time on the main display screen
				lCurStepSecSP = flSegTime;

				if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
				{
					lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
				}
				else
					lCurStepSecRemain = 0;

				Sleep(250);
			} // End of This Phase STEP
		}
		else
		{
			break;
		}
		Sleep(250);

	}	// End of for Loop for all Phase STEPS

	bCOOLTempStep1 = false;
	bCOOLTempStep2 = false;
	bCOOLTempStep3 = false;
	bCOOLTempStep4 = false;

	Display.KillEntry();

	PID.PIDSetPoint(AO_WATER, flFinalCoolTempSetpoint, 0);
	flDisplayLoopTempSP = flFinalCoolTempSetpoint;

	bCheckCriticalPSIFlag = false;	//stop checking the vapor PSI

	PinesTime1.StartTimer(); // Delay - Phase, Step, or Segment Timer (sec)
	PinesTime1.ZeroTime();

	flSegTime = (long)flCOOLTempHoldTime;

	if (!bKillTheTaskFlag && flSegTime > 0)
	{
		csVar2.Format(L"Begin Final Hold Period");
		csVar3.Format(L"This Hold Period = %d Seconds", flSegTime);
		Display.Log(csVar2, L"", SendLogStatus);
		csVar7.Format(L"Cooling Hold");
		Display.Display(20, csVar7);
		Display.Display(21, csVar3);
		Display.Display(22, L"Please Wait...");
		Display.Entry(0, 0, TRUE);

		while (!bKillTheTaskFlag && PinesTime1.GetElapsedTime() < flSegTime)
		{
			flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_WATER);
			// Added to set the time on the main display screen
			lCurStepSecSP = flSegTime;

			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;

			Sleep(250);
		} // End of This Phase STEP

	}
	Display.KillEntry();


	/////////////////////////////////////////////////////Acid Wash Logic Neutralization Logic ////////////////////////////////////////////////
	// Logic to hold process to add base. Then check every 10 minutes to see if solution has a pH of 7.0 +/- 0.5
	// Once acheived the solution can be discharged.
	if (bCleaningRecipeSelected)
	{
		Display.Alarm(2, 1, LOGTECAlarm, L"Ready for Neutralizing Agent");
		sRetortAlarmMessage = L"Ready for Neutralizing Agent";  
		sAlarmMessage_Hold = sRetortAlarmMessage;
		bAlarmsUpdate_iOPS = true;

		Display.KillEntry();
		Display.Display(20, L"Add Base Neutralizing Agent Now");
		Display.Display(21, L"When Complete Press Enter");
		Display.Display(22, L"To Start 10 Minute Timer");
		Display.DisplayStatus(L"Waiting for Enter Key to Continue");
		Display.Entry(0, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		bNeutralizingTimerStarted = false;
		do
		{

			if (!bNeutralizingTimerStarted)
			{
				PinesTime24.StartTimer();
				PinesTime24.ZeroTime();
				bNeutralizingTimerStarted = true;
			}

			Display.KillEntry();
			Display.Display(20, L"Base Has Been Added");
			Display.Display(21, L"Waiting to Complete");
			Display.Display(22, L"10 Minute Circulation");
			Display.DisplayStatus(L"Waiting");
			Display.Entry(0, 0);

			while (PinesTime24.GetElapsedTime() < 600)
				Sleep(500);


			if (PinesTime24.GetElapsedTime() > 600)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"-- CHECK pH LEVEL --");
				sRetortAlarmMessage = L"-- CHECK pH LEVEL --";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				Display.KillEntry();
				Display.Display(20, L"-- CHECK pH LEVEL --");
				Display.Display(21, L"Press Y to run another 10 min cycle");
				Display.Display(22, L"Press N to go to Discharge and Rinse Cycle");
				Display.DisplayStatus(L"Waiting...");
				Display.Entry(3, 0);


				Func_WaitForEnterKey(bKillTheTaskFlag, false);

				PinesTime24.StopTimer();
				PinesTime24.ZeroTime();

				csVar3 = Display.GetEnteredString(3);

				if (csVar3.CompareNoCase(L"Y") == 0)
				{
					Display.KillEntry();
					bNeutralizeComplete = false;
					bNeutralizingTimerStarted = false;
					Display.Log(L"Starting 10 Min Neutralizing Cycle");
				}
				else if (csVar3.CompareNoCase(L"N") == 0)
				{
					bNeutralizeComplete = true;
					Display.Log(L"Neutralization confirm OK");
				}
				else
				{
					Display.Log(L"Invalid Entry");
				}
			}

		} while (!bNeutralizeComplete);



	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// *** Clean up here - leave things in a safe state ***

	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);

	bEnblCoolWaterFlow = false;

	//flDisplayLoopTempSP = 0;
	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	Display.KillEntry();

	TaskLoop.KillTask(3);
	PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	bCoolTempCompleteFlag = true;
	bCoolTempRunning = false;
	bKillCoolPSITaskFlag = true;
	Sleep(400);
	bPhaseCompleteFlag = true;
	bOKtoStartCoolingPSI = false;
}	// End of CoolingPhaseTemperature()

// ================ Cooling PSI Function ===============
void CoolingPhasePSI()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bCoolPSICompleteFlag		= false;	// Unique: Set false here & then true at end of function
	bCoolPSIPhaseCompleteFlag	= false;	// Set false here & then true at end of function
	bKillCoolPSITaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bResetTimerFlag				= false;

	// *** END OF COMMON INITIALIZATION STUFF

	if (!bSplitRangeRunning)	
	{
		bKillSplitRangeFlag = false;
		TaskLoop.SpawnTask((long)SplitRangeControl, 8);
	}
	float	flSegStartPressure = 0.0, flSegEndPressure = 0.0, flFinalClPressSetpoint = 0.0;
	float	flCoolStartPressure = flCurrentPressure; // starting pressure kept as refernce
	float   flPressHoldSetpoint = 0.0;
	long	lTimeToTracPSIFromCookToCool = 60; // Variable time for tracking the Delta pressure Cook to Cool (2 min)
	long	flSegTime = 0;
	long	lRampPeriod = 0;
	bool	bDifferentialPSIFromCookToCool = false, bSetOnce = false;
	CString csTmp, csVar1, csVar2;

	PinesTime14.StartTimer();
	PinesTime14.ZeroTime();

	// waiting for Cool Phase to start

	PID.PIDSetPoint(AO_SPLT_RNG, flCurrentPressure, 0.0);

	while (!bKillCoolPSITaskFlag && !bOKtoStartCoolingPSI)
	{
		Sleep(500);
	}

	PID.PIDSetPoint(AO_SPLT_RNG, flCurrentPressure, 0.0);
	PID.PIDStart(AO_SPLT_RNG, 13);
	
	// MICRO-COOL
	while (!bKillCoolPSITaskFlag && PinesTime14.GetElapsedTime() < flMicroCoolTime)
	{
		// MICRO COOL - Just wait for air to stabilize (in OTHER task loop)

		flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
		// Alarm of differential pressure from Cook to Cool is higher than 4 PSI check during micro-cool
		if (((flCoolStartPressure - flCurrentPressure) > 4.0) && !bDifferentialPSIFromCookToCool)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"Differential Pressure from Cook to Cool Greater than 4 PSI", SendLogStatus);
			sRetortAlarmMessage = L"Differential Pressure from Cook to Cool Greater than 4 PSI";  
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bDifferentialPSIFromCookToCool = true;
		}
		Sleep(250);
	}
	flDisplayLoopPsiSP = 0.0;
	// END OF MICRO-COOL

	bCOOLPSIStepA = false;
	bCOOLPSIStepB = false;
	bCOOLPSIStepC = false;
	bCOOLPSIStepD = false;
	bCOOLPSIStepE = false;

	//*** Phase STEP Logic ***
	for (int iSegNum = 1; iSegNum <= 5; iSegNum++) // Set up each segment, load, then wait til satisfied, then increment
	{
		if (bKillCoolPSITaskFlag)
			break;

		bool bLongCoolingTimeFlag = false;
		bSetOnce = false;

		switch (iSegNum) // Each Phase Step via Process Variables
		{
		case 1: // Begin via Cook
		{
			sStepLetter = L"A";
			flSegStartPressure = flCurrentPressure; // Current Pressure
			flSegEndPressure = flCOOLPSIStepA;
			flSegTime = (long)flCOOLPSITimeA;
			bCOOLPSIStepA = true;
			break;
		}
		case 2:
		{
			sStepLetter = L"B";
			flSegStartPressure = flCOOLPSIStepA;
			flSegEndPressure = flCOOLPSIStepB;
			flSegTime = (long)flCOOLPSITimeB;
			bCOOLPSIStepA = false;
			bCOOLPSIStepB = true;
			break;
		}
		case 3:
		{
			sStepLetter = L"C";
			flSegStartPressure = flCOOLPSIStepB;
			flSegEndPressure = flCOOLPSIStepC;
			flSegTime = (long)flCOOLPSITimeC;
			bCOOLPSIStepB = false;
			bCOOLPSIStepC = true;
			break;
		}
		case 4:
		{
			sStepLetter = L"D";
			flSegStartPressure = flCOOLPSIStepC;
			flSegEndPressure = flCOOLPSIStepD;
			flSegTime = (long)flCOOLPSITimeD;
			bCOOLPSIStepC = false;
			bCOOLPSIStepD = true;
			break;
		}
		case 5:
		{
			sStepLetter = L"E";
			flSegStartPressure = flCOOLPSIStepD;
			flSegEndPressure = flCOOLPSIStepE;
			flSegTime = (long)flCOOLPSITimeE;
			bCOOLPSIStepD = false;
			bCOOLPSIStepE = true;
			break;
		}
		}
		// Common Display/Log Stuff for each Phase Step
		float flLongSegmentPSITime = (float)flSegTime * (float)2.0; // For Excess Time alarm % of Segment time
		if (flSegTime != 0) // SKIP? Disabled if Step Time is zero
		{
			csVar1.Format(L"%-5.1f", flSegEndPressure);
			csVar2.Format(L"Cool Step " + sStepLetter + " Pressure Ramp to " + csVar1 + " PSI in % d Sec", flSegTime);
			Display.Log(csVar2, L"", SendLogStatus);

			PinesTime14.StartTimer();
			PinesTime14.ZeroTime();

			// Air & Vent Loops Set Together & Use special Tuning on FIRST STEP ONLY

			PID.PIDSetPoint(AO_SPLT_RNG, flSegStartPressure, 0.0);
			if (iSegNum == 1)
			{
				PID.PIDStart(AO_SPLT_RNG, 13);
			}
			else
				PID.PIDStart(AO_SPLT_RNG, 14);

			flFinalClPressSetpoint = flSegEndPressure;
			PID.RampSetpointPeriod(AO_SPLT_RNG, flSegEndPressure, flSegTime);

			// Pressure + Time = then we're done...with THIS segment
			while (!bKillCoolPSITaskFlag &&
				(flCurrentPressure > (flSegEndPressure + (float)0.5) || PinesTime14.GetElapsedTime() < flSegTime))
			{
				// Hold PSI at current value until Temp reaches setpoint
				if (bCoolDelayHaltPSIRampFlag && !bResetTimerFlag)
				{
					flPressHoldSetpoint = (ConvertTemptoPress(flCurrentTemperature));

					if (17 < flPressHoldSetpoint)
					{
						flPressHoldSetpoint = 17;
					}
					if (flPressHoldSetpoint < flCurrentPressure)
					{
						flPressHoldSetpoint = flCurrentPressure;
					}


					PID.PIDSetPoint(AO_SPLT_RNG, flPressHoldSetpoint, 0.0);

					flTimeLeftInSegment = (float)(flSegTime - PinesTime14.GetElapsedTime());

					PinesTime14.StopTimer();
					PinesTime21.StartTimer();
					PinesTime21.ZeroTime();

					bResetTimerFlag = true;

					csVar1.Format(L"%-5.1f", flCurrentPressure);
					csVar2.Format(L"Press Step " + sStepLetter + " Hold at " + csVar1 + " PSI with %d Sec left",flTimeLeftInSegment);
					Display.Log(csVar2, L"", SendLogStatus);

				}
				if (!bCoolDelayHaltPSIRampFlag && bResetTimerFlag) // re-set timers and flags
				{
					lRampPeriod = flSegTime - PinesTime14.GetElapsedTime();

					if (lRampPeriod < 0)		// MDM Added to prevent negative numbers passing into the the pid.ramp
					{
						lRampPeriod = 1;
					}

					PID.RampSetpointPeriod(AO_SPLT_RNG, flSegEndPressure, lRampPeriod);
					PinesTime14.StartTimer();
					flTimeLeftInSegment = 0.0;
					bResetTimerFlag = false;

					csVar1.Format(L"%-5.1f", flCurrentPressure);
					csVar2.Format(L"Resume Press Step " + sStepLetter + " for %d Sec ", PinesTime21.GetElapsedTime());
					Display.Log(csVar2, L"", SendLogStatus);
					PinesTime21.StopTimer();
					PinesTime21.ZeroTime();
				}
				if (PinesTime14.GetElapsedTime() > flLongSegmentPSITime && !bLongCoolingTimeFlag)
				{
					csTmp.Format(L"Long Cooling Pressure Ramp Time - Step %d", iSegNum);
					Display.Alarm(2, 1, LOGTECAlarm, csTmp, SendLogStatus);
					sRetortAlarmMessage = L"Long Cooling Pressure Ramp Time";  
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bLongCoolingTimeFlag = true;
				}
				flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
				// Added to set the time on the main display screen

				Sleep(250);
			} // End of This Phase STEP

		}
		else
		{
			break;
		}
		Sleep(250);

	}	// End of for Loop for all Phase STEPS

	bCOOLPSIStepA = false;
	bCOOLPSIStepB = false;
	bCOOLPSIStepC = false;
	bCOOLPSIStepD = false;
	bCOOLPSIStepE = false;

	PinesTime14.StartTimer(); // Delay - Phase, Step, or Segment Timer (sec)
	PinesTime14.ZeroTime();

	PID.PIDSetPoint(AO_SPLT_RNG, flFinalClPressSetpoint, 0.0);
	flDisplayLoopPsiSP = flFinalClPressSetpoint;

	// *** Clean up here - leave things in a safe state ***
	// Kill Only PID Loops That Should be killed.

	flDisplayLoopTempSP = 0;
	flDisplayLoopPsiSP = 0;

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	Display.KillEntry();
	TaskLoop.KillTask(9);
	PinesTime14.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	bCoolPSICompleteFlag = true;
	Sleep(400);
	bCoolPSIPhaseCompleteFlag = true;
}	// End of CoolingPhasePSI()


//======================== DrainPhase ===================================
void DrainPhase()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bDrainCompleteFlag		= false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		= false;	// Set in COOK
	bRunLevelControlFlag	= false;	// Separate Task Loop Monitors Level
	bEndLevelControlFlag	= true;		// Kills Level Control Task Loop here
	bIsInProcess			= false;	//machine is in process starting at come-up to end of cooling.
	bDrainRunning			= true;

	Display.Phase(L"Drain");
	sPhaseMessage = L"Drain"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 8)) 
	{
		Display.Log(L"Failure to set Phase to Drain");
	}
	Display.Log(L"Begin Drain");

	CommonInit();

	CString csVar1, csVar2, csVar3;
	CString csEntry = "";
	float	flDrainPress = 2.0; // adjust to push water out!
	long	flDrainRampTime = 10;
	bool	bLongDrainTimeFlag = false;

	// in the case of FullDrain and set the drain level accordingly

	flDisplayLoopPsiSP = flDrainPress;

	if (bFullDrainFlag || bCleaningRecipeSelected)
	{
		flDrainLevel = flFullDrainLevel; // Change drain Level to Full Drain Level
		bDoor1SecuredMode = false;
	}
	if (!bFullDrainFlag)
	{
		if ( bSteamOnlyRecipeFlag || bAirCoolTestingFlag)
			flDrainLevel = flWaterAtDamLevel;
		else
			flDrainLevel = 9.5;
		bDoor1SecuredMode = true;
	}
	// wait here for the door to close and lock
	if (DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT) && !bKillTheTaskFlag)
	{
		Display.KillEntry(); 

		Display.Display(20, L"Waiting for Door");
		Display.Display(21, L"To Close And Lock");
		Display.DisplayStatus(L"Please wait...");
		Display.Entry(0, 0, TRUE);
	}

	while (DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT) && !bKillTheTaskFlag)
	{
		Sleep(250);
	}

	Display.KillEntry();

	//Sleep(2000);
	// *** END OF COMMON INITIALIZATION STUFF
	if (bSplitRangeCompleteFlag)	// Should be running - used to Reinitialize upon override.
	{
		bKillSplitRangeFlag = false;
		TaskLoop.SpawnTask((long)SplitRangeControl, 8);
	}
	PinesTime5.StopTimer(); // Total ComeUp through CoolDown Time. Must be Stopped...
	PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
	PinesTime1.ZeroTime();

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);

	PID.PIDSetPoint(AO_SPLT_RNG, flCurrentPressure, 0.0);
	PID.PIDStart(AO_SPLT_RNG, 15);
	PID.RampSetpointPeriod(AO_SPLT_RNG, flDrainPress, flDrainRampTime);

	csVar3.Format(L"Drain Pressure = %-5.1fPSI", flDrainPress);

	Display.KillEntry();

	Display.Display(20, L"Draining Retort...");
	Display.Display(21, csVar3);
	Display.DisplayStatus(L"Please wait...");
	Display.Entry(0, 0, TRUE);

	lCurStepSecSP = 0;

	DigitalIO.SetDigitalOut(VDO_AgitationReq, Off);	// Clear the request for motion
	DigitalIO.SetDigitalOut(VDO_StopEA_Req, Off);	// Clear the request to stop
	bEndEAAgitation = true; 

	//Delay so water can settle and we get an accurate level reading.
	while (!bKillTheTaskFlag && Display.GetPhaseTimer() <= 10)
	{
		Sleep(250);
	}

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);
	lCurStepSecRemain = 0;
	flCurrentWaterLevel = AnalogIO.ReadAnalogIn(AI_LEVEL);
	if (bCleaningRecipeSelected)
		flDrainLevel = 4.0;

	while (!bKillTheTaskFlag && flCurrentWaterLevel > flDrainLevel)
	{
		flCurrentWaterLevel = AnalogIO.ReadAnalogIn(AI_LEVEL);

		if (PinesTime1.GetElapsedTime() > 600 && !bLongDrainTimeFlag) // Excess Time alarm
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"Excessive Drain Time", SendLogStatus);
			sRetortAlarmMessage = L"Excessive Drain Time";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bLongDrainTimeFlag = true;
		}
		flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);


		Sleep(250);
	}
	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);

	//////////////////////////////////////ACID WASH RINSE LOGIC /////////////////////////////////////////////
	// Fill and Drain three times.
	if (bCleaningRecipeSelected)
	{
		Display.Log(L"Begin Acid Wash Rinse Cycle");
		int i = 0;
		do
		{
			Display.Log(L"Begin Fill");
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
			DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Onn);

			while (flCurrentWaterLevel < 50)
				Sleep(500);

			Display.Log(L"Begin Circulation");
			DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
			DigitalIO.SetDigitalOut(DO_PUMP_1, Onn);
			DigitalIO.SetDigitalOut(DO_PUMP_2, Onn);
			bPumpIsRunning = true;



			PinesTime24.StartTimer();
			PinesTime24.ZeroTime();

			while (PinesTime24.GetElapsedTime() < 600)
				Sleep(500);

			Display.Log(L"Begin Drain");
			DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
			DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
			DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
			bPumpIsRunning = false;
			DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Onn);



			while (flCurrentWaterLevel > 5)
				Sleep(500);
			i++;

		} while (i < 5);

		PinesTime24.StopTimer();
		PinesTime24.ZeroTime();
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////
	Display.KillEntry();

	flDisplayLoopPsiSP = 0.0;
	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);
	PID.PIDStop(AO_SPLT_RNG);

	if (flCurrentPressure < 10)
	{
		AnalogIO.SetAnalogOut(AO_VENT, 100.0);
		AnalogIO.SetAnalogOut(AO_AIR, 0.0);
	}
	else
	{
		AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		AnalogIO.SetAnalogOut(AO_AIR, 0.0);
	}

	/* in the case of full drain Override we need to vent out the pressure that we used in the drain,
	prior to returning back to start; under normal circumstances we releigh the pressure in unloading*/
	if (bFullDrainFlag)	// check to see if we had full drain
	{
		PID.PIDStart(AO_SPLT_RNG, 15);
		PID.RampSetpointPeriod(AO_SPLT_RNG, 0.0, flDrainRampTime);

		bool bLongVentZeroTimeFlag = false;
		PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
		PinesTime1.ZeroTime();
		Display.Display(20, L"Venting PSI < 1.0 PSI...");
		Display.Log(L"Venting PSI < 1.0 PSI");
		Display.Entry(0, 0, TRUE);
		lCurStepSecSP = 120;

		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		while (!bKillTheTaskFlag &&
			(flCurrentPressure > 1 || DigitalIO.ReadDigitalIn(DI_ZERO_PRS)))
		{
			flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
			if (PinesTime1.GetElapsedTime() > 600 && !bLongVentZeroTimeFlag) // Excess Time alarm
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"Venting Pressure Delay", SendLogStatus);
				sRetortAlarmMessage = L"Venting Pressure Delay";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bLongVentZeroTimeFlag = true;
			}
			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;

			Sleep(250);
		}
		Display.KillEntry();
		lCurStepSecSP = 0;
		lCurStepSecRemain = 0;
	}	// end of full drain PSI Venting

	if (!bSplitRangeCompleteFlag)	// Should NOT be running - for overrides.
	{
		bKillSplitRangeFlag = true;
	}

	Sleep(2000);

	AnalogIO.SetAnalogOut(AO_AIR, 0.0);
	AnalogIO.SetAnalogOut(AO_VENT, 100.0);

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);

	flDisplayLoopPsiSP = 0.0;

	// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***
	if (!bKillTheTaskFlag)
	{
		csVar1.Format(L"End Drain at less than %-5.1f%%", flDrainLevel);
		Display.Log(csVar1, L"", SendLogStatus);
	}

	PID.PIDStop(AO_SPLT_RNG);
	Display.KillEntry();
	TaskLoop.KillTask(3);
	PinesTime1.StopTimer(); // Phase, Step, OR Segment Timer (sec)
	bDrainCompleteFlag = true;
	Sleep(400);
	bDoor1SecuredMode = false;	// end checking door
	bPhaseCompleteFlag = true;
	bIsInIdle = true;	//machine is idle (true) only before Fill-phase and after Drain-phase.
	bDrainRunning = false;
}
//======================== UnloadingPhase ===================================
void UnLoadingPhase()
{
	// *** BEGiNNiNG OF COMMON INITIALIZATION STUFF (Keep this Section SiMiLAR in all routines!)
	bPhaseCompleteFlag		= false;	// Set false here & then true at end of function
	bKillTheTaskFlag		= false;	// Set false here - Set TRUE by the OVERRiDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck		 = false;	// Set in COOK
	bRunLevelControlFlag	= false;	// Separate Task Loop Monitors Level
	bEndLevelControlFlag	= true;		// Kills Level Control Task Loop here
	bIsInIdle				= true;		//machine is idle (true) only before Fill-phase and after Drain-phase.
	bIsInProcess			= false;	//machine is in process starting at come-up to end of cooling.
	bUnloadRunning			= true;

	Display.Phase(L"UnLoad");
	sPhaseMessage = L"Unload"; 
	if (!tcio.WriteINT(L"LTM.iPhase", 9)) 
	{
		Display.Log(L"Failure to set Phase to Unload");
	}

	CommonInit();
	// *** END OF COMMON INITIALIZATION STUFF

	if (!bSplitRangeCompleteFlag)	// Should NOT be running - for overrides.
	{
		bKillSplitRangeFlag = true;
	}
	bool bLongVentZeroTimeFlag = false;
	bool bFinalTemperatureTimeFlag = false; //alarm flag
	bool bFinalTemperatureTime5Flag = false; // if Temp > 105 after than 5 min
	bool bOperatorForceDoorToOpen = false;	// Operator forces the Door to Open
	CString csOption = "", csTmp1;

	PinesTime1.StartTimer(); // Phase, Step, OR Segment Timer (sec)
	PinesTime1.ZeroTime();

	// Wait For the Drain Pressure to Vent OFF FIRST.
	Display.KillEntry(); 

	Display.Display(20, L"Venting PSI < " + toStr(flFinalPressureLimit));
	Display.Display(21, L"Temperature < " + toStr(flFinalTemperatureLimit) + "°F");
	Display.Display(22, L"Waiting for Zero Pressure...");
	Display.DisplayStatus(L"Please Wait...");
	Display.Entry(0, 0, TRUE);
	lCurStepSecSP = 120;

	if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
	{
		lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
	}
	else
		lCurStepSecRemain = 0;

	while (!bKillTheTaskFlag && !bOperatorForceDoorToOpen && (flCurrentPressure > 1.0 || !DigitalIO.ReadDigitalIn(DI_ZERO_PRS))
		|| (flCurrentTemperature > flFinalTemperatureLimit)) // waiting for PSI and Temp to be below limits
	{
		if (PinesTime1.GetElapsedTime() > 600 && !bLongVentZeroTimeFlag) // Excess Time alarm for PSI
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"Venting Pressure Delay", SendLogStatus);
			sRetortAlarmMessage = L"Venting Pressure Delay";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bLongVentZeroTimeFlag = true;
		}
		if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
		{
			lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
		}
		else
			lCurStepSecRemain = 0;

		Sleep(250);
	}
	Display.KillEntry();

	Sleep(1000);

	bProcessStartedFlag = false; //reset flag

	if (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR))
	{
		Display.Display(20, L"Ready to Unload");
		Display.Display(21, L" ");
		Display.Display(22, L"Open Retort Door");

		Display.DisplayStatus(L"Please Wait...");
		Display.Entry(0, 0, TRUE);

		while (!bKillTheTaskFlag && !DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR))
		{
			Sleep(250);
		}
	}
	Display.KillEntry();

	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;
	// reset flags for next run
	bFinalTemperatureTimeFlag = false;	//alarm flag
	bFinalTemperatureTime5Flag = false;	// if Temp > 105 after than 5 min
	bOperatorForceDoorToOpen = false;	// Operator forces the Door to Open

	// *** Clean up here - leave things in a safe state ***
	// Kill Only PID Loops That Should be killed.

	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);
	PID.PIDStop(AO_SPLT_RNG);

	//if (DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR))
	//	DigitalIO.SetDigitalOut(DO_ROT_DRV_STOP, Onn); // Enable Manual Movement

	// *** COMMON ENDING TO ALL PHASE FUNCTiONS - Except Specific b(phase)CompleteFlag ***
	Display.KillEntry();
	TaskLoop.KillTask(3);
	bUnLoadingCompleteFlag = true;
	bUnloadRunning = false;
	Sleep(400);
	bPhaseCompleteFlag = true;
}	// End of UnLoadingPhase()

/* =====================DisplayinitialProcess Screen ==========*/
void DisplayInitialProcessScreen()
{
	// Field 1: Temperature RTD
	Display.DisplayVariable(1, L"Retort Temp °F", L"float", (long)&flCurrentTemperature);
	// Field 2: Temperature Setpoint
	Display.DisplayVariable(2, L"Setpoint Temp °F", L"float", (long)&flDisplayLoopTempSP);
	// Field 3: Step Period (sec)
	Display.DisplayVariable(3, L"Step Remain(Sec)", L"long", (long)&lCurStepSecRemain);
	// Field 4: Lock Pin Position
	Display.Display(4, L"Door Lckd", L"Unsealed|Sealed", FALSE, L"DI", L"3");
	// Field 5: Pressure
	Display.DisplayVariable(5, L"Retort PSI", L"float", (long)&flCurrentPressure);
	// Field 6: Pressure Setpoint
	Display.DisplayVariable(6, L"Setpoint PSI", L"float", (long)&flDisplayLoopPsiSP);
	// Field 9: Flow
	Display.DisplayVariable(9, L"Water Flow GPM", L"float", (long)&flCurrentWaterFlow);
	// Field 10: Water Level
	Display.DisplayVariable(10, L"Water Level %", L"float", (long)&flCurrentWaterLevel);
	// Field 11: PreHeat RTD
	Display.DisplayVariable(11, L"PreHeat RTD °F", L"float", (long)&flCurrentPreHeatTemp);
	// Field 13: Cycle No
	Display.Display(13, L"Cycle No", L"CycleNo");
	// Field 14: Phase Name
	Display.Display(14, L"Phase", L"PhaseName");
	// Field 15: Step Remaining (sec)
	bDisplaysSet = true;
} /* end of DisplaynitialProcessScreen

/* ========================== DisplayCompleteProcessScreen =============*/
void DisplayCompleteProcessScreen()
{
	CString	csScheduledProcess = L"",
		csProductType = L"",
		csVar1 = L"",
		strCTEMP = L"";


	float	flCTIMESeconds = 0.0;
	int		iCTIMEMinutes = 0, iCTIMESeconds = 0, iCTIMEHours = 0;

	if (iProductType == BALL)
		csProductType = L"Ball Process";
	if (iProductType == GENERIC)
		csProductType = L"Generic Process";

	csScheduledProcess = L"The Scheduled ";
	csScheduledProcess += csProductType;
	csScheduledProcess += L" is ";

	strCTIME = SecondsToStr(flCTIME);
	strCTEMP = TempFtoStr(flCTEMP);

	csiT = TempFtoStr(flInitialTemperature);

	csScheduledProcess += strCTIME;
	csScheduledProcess += L" at ";
	csScheduledProcess += strCTEMP;
	//	csScheduledProcess += L" with a min. IT of ";
	//	csScheduledProcess += csiT;

	Display.Display(0, csScheduledProcess);


} /* end of DisplayCompleteProcessScreen

/*============================== TurnAllioOff =======================*/
void TurnAllDigitalOutputsOff()
{
	long lDONumber = 1;

	while (lDONumber <= 100)
	{
		if (lDONumber != 0)
		{
			DigitalIO.SetDigitalOut(lDONumber, Off);
		}
		//The line below must be out of the conditional statement
		lDONumber = lDONumber + 1;
	}
}	/* TurnAllDigitalOutputsOff	*/

//======================== TurnAllAnalogOutputsOff ===================================
void TurnAllAnalogOutputsOff()
{
	long lAONumber = 1;

	while (lAONumber <= 10)
	{
		if (lAONumber != AO_SPLT_RNG) PID.PIDStop(lAONumber, true);
		lAONumber = lAONumber + 1;
		Sleep(10);
	}
	AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);

	while (lAONumber <= 100)
	{
		AnalogIO.SetAnalogOut(lAONumber, Off);
		lAONumber = lAONumber + 1;
		Sleep(10);
	}
}	/* TurnAllAnalogOutputsOff	*/

//================== ResetControlFlags_Variables =================
void ResetControlFlags_Variables()
{
	bComeUpTimerRunning = false;
	bEndLevelControlFlag = false;
	bReadyForProcessFlag = false;
	bDisplayDeviationInfo = false;
	bTempDeviationFlag = false;
	bRetort_PresFailFlag = false;
	bCriticalPSIFailureFlag = false;
	bCheckCriticalPSIFlag = false;
	bUpdateToCorrectRTD = false;
	bRetort_TempFailFlag = false;
	bDoor1SecuredMode = false;
	bEnblCoolWaterFlow = false;
	bProcessStarted = false;
	bInitiateLowMigDev = false;
	bProcessDeviationFlag = false;
	bNoMoreValidProcessFlag = false;
	bRunLevelControlFlag = false;	// Separate Task Loop Monitors Level
	bFillLevelAdjustFlag = false;
	bDoDeviationCheck = false;
	bShutDownFlag = false;
	bKillMajorProblemChecks = false;
	//bMajorPBChecksRunning = false,
	bKillTheTaskFlag = false;
	bLoadingCompleteFlag = false;
	bUnLoadingCompleteFlag = false;
	bOpEntryCompleteFlag = true;
	bCookCompleteFlag = true;
	bCoolPSICompleteFlag = false;
	bDrainCompleteFlag = false;
	bAddCookTimeFlag = false;
	bPhaseCompleteFlag = true;
	bCUPSIPhaseCompleteFlag = true;
	bCoolPSIPhaseCompleteFlag = true;
	bCUTempCompleteFlag = true;
	bCUPSICompleteFlag = false;
	bKillSplitRangeFlag = false;
	bVerifyExitEntry = false;
	bChartVerified = false;
	bNeedToRestartFlag = false;
	bSegmentCompleteFlag = false;
	bHighWaterLevel = false;
	bSplitRangeCompleteFlag = true;
	bResetTimerFlag = false;
	bDoorLockFailureFlag = false;
	bCook_PresFailFlag = false;
	bCook_PressWarnFlag = false;
	bCheckForMinPSIFlag = false;
	bVerifyYNFlag = false;
	bVerifyYN = false;
	bInitFillWaitDone = false;
	bProcessStartedFlag = false;
	bHighITProcessFlag = false;
	bCookTimeCompleted = false;
	bRecipeSelectionFlag = false;
	bFullDrainFlag = false;
	bJumpOutToDrainFlag = false;
	bCriticalPSIFailureFlag = false;
	bCheckCriticalPSIFlag = false;
	bDoorLockFailureFlag = false;
	bHOT_RestartFlag = false;
	bVerifyPromptFlag = false;
	bVerifySelectionYN = false;
	bDisplaysSet = false;
	bFI_SprayCool = false;
	bEA_Selected = false;
	bTriggerEAChange = false;
	bEndEAAgitation = false;
	bEndRotation = false;
	bIsInIdle = true;	//machine is idle (true) only before Fill-phase and after Drain-phase.
	bIsInProcess = false;	//machine is in process starting at come-up to end of cooling.


	iProductNumber = 0;
	iProductType = 0;

	flNewMP = 0.0;

	flCTIMEAddedAccumSec = 0.0;
	flInitialTemperature = 0.0;
	flNewCTEMP = 0.0;
	flCTEMP = 0.0;
	flCTIME = 0.0;
	flRTintervals = 0.0;
	flHighestCUPress = 0.0;
	flInitialTempOffset = 0.0;
	flPreHeatTemp = 0.0;
	flMinimumIT = 32.0;
	flExitTemperature = 0.0;
	flCurrentTempSetpoint = 0.0;

	flDisplayLoopTempSP = 0.0;
	flDisplayLoopPsiSP = 0.0;

	lTemp = 0;
	lCurStepSecSP = 0;
	lCurStepSecRemain = 0;


}		//End of ResetControlFlags_Variables

//================== PromptForInitialEntries =================
void PromptForInitialOperatorEntries()
{
	bool	bVerifyEntryYN = false,
		bProblemWithDataEntryFlag = true,
		bTerminateFunction = false,
		bBatchIDEntryEnabled = true;

	csBatchCode = L"Null";

	CString csVar1 = L"",
		csVar2 = L"",
		csProductType = L"",
		csEntry = L"",
		csTemp = L"",
		cs_i = L"",
		csProdCode = L"",
		csNumberRacks = L"";

	//---------------------------- Begin of function execution -------------
	Display.OverrideDisable();

	while (!bShutDownFlag && bProblemWithDataEntryFlag && !bFullDrainFlag)
	{
		bProblemWithDataEntryFlag = false;
		Display.Display(0, L"");
		//		Display.Display(0, L"Please Select the Correct Recipe From the List");
		iProductNumber = Display.GetProduct();

		Display.OverrideEnable();
		csVar1.Format(L"%-d", iProductNumber);
		cs_i.Format(L"%-d", iProductNumber);
		Display.Log(L"Recipe Number Selected:", csVar1);

		//Display current selection on the controller screen for verification
		iProductType = Display.GetProductType(iProductNumber);
		csProductType.Format(L"%-d", iProductType);

		switch (iProductType)
		{
		case NONE:
		{
			csVar2 += L"NONE";
			Display.Display(22, csVar2);
			bProblemWithDataEntryFlag = true;
			break;
		}
		case GENERIC:
		{
			PV.SetProduct(iProductNumber);
			csVar2 += L"GENERIC";
			Display.Display(22, csVar2);
			if (!bShutDownFlag)
			{
				//GENERIC time is labeled in minutes in the product configuration screen.
				PromptForInitialTemperature();
				flCTEMP = (float)PV.GetGenericTemp();
				//	flCTEMP	+= 1;	//adds the bias on the PID of 1 for the display line 0

				flCTIME = (float)(PV.GetGenericTime() * 60.0);

				csCTEMP.Format(L" %-5.1f", flCTEMP);
				//	flCTEMP	= flCTEMP - 1;	//removes the bias on the PID of 1 for the display line 0

				CTimeSpan ts((long)flCTIME);
				csCTIME = ts.Format(L"%H:%M:%S");

				//csCTIME.Format(L" %-5.2f", flCTIME / 60);
				//add units to the string
				csVar1 = csCTEMP;
				csVar2 = csCTIME;
				csVar1 += L"°F";
				//csVar2 += L" min";
				Display.SetCookTemp(csVar1);
				Display.SetCookTime(csVar2);
				Display.SetInitialTemp(flInitialTemperature);
			} //end of if !bShutDownFlag
			break;
		}
		case BALL:
		{
			PV.SetProduct(iProductNumber);
			csVar2 += L"BALL";
			Display.Display(22, csVar2);
			if (!bShutDownFlag)
			{
				//Since the IT is an argument for the functions below, it must be
				//prompted before calling the functions
				PromptForInitialTemperature();

				flCTEMP = (float)PV.GetBallTemp(iProductNumber);

				flCTIME = (float)PV.CalculateBallProcess(flInitialTemperature);

				flMinimumCookTime = ((float)Ball.GetMinProcessValue() * 60);
				if (flCTIME < flMinimumCookTime)
					flCTIME = flMinimumCookTime;

				csCTEMP = TempFtoStr(flCTEMP);
				csCTIME = SecondsToStr(flCTIME);

				//add units to the string
				csVar1 = csCTEMP;
				csVar2 = csCTIME;

				Display.SetCookTemp(csVar1);
				Display.SetCookTime(csVar2);
				Display.SetInitialTemp(flInitialTemperature);
			} //end of if bShutDownFlag
			break;
		}
		case TUNACAL:
		{
			csVar2 += L"NONE";
			Display.Display(22, csVar2);
			bProblemWithDataEntryFlag = true;
			break;
		}
		case NUMERICAL:
		{
			csVar2 += L"NONE"; //ToDo: needs NC capability
			Display.Display(22, csVar2);
			bProblemWithDataEntryFlag = true;
			break;
		}
		}//end of switch(iProductType)

		sProcessRecipeName = Display.GetProductDescription(); 

		LoadProcessVariables();
		Sleep(250);
		
	}//end of if((!bShutDownFlag) && (!bProblemWithDataEntryFlag))

	//tcio.ReadSTRING(L"iOPS.LTM_2_iOPS.csBatchCode", csBatchCode, 60); // v.17 RMH Added to read from TCIO tag dn then write as a log
	//Display.Log(csBatchCode);

}	// end of PromptForinitialOperatorEntries

/* ================= PromptForInitialTemperature() ======*/
void PromptForInitialTemperature()
{
	bool bVerifyEntryYN = false, bVerifyFlag = false, bEntryFlag = true, bReEnterFlag = false;

	CString csVar1 = L"", csEntry = L"", csCurrentTemp = L"", csMainEntry = L""; //, csTrayTime = L"";
	CString csDebugiTvar = L"", csDebugiTvar2 = L"";

	 

	while (!bVerifyFlag && !bKillTheTaskFlag)
	{
		bVerifyEntryYN = false;

		do
		{
			Display.KillEntry();
			Sleep(100);
			csEntry.Format(L"%-d", iProductNumber);
			csVar1 = L"For Recipe #: ";
			csVar1 += csEntry;
			Display.Display(20, L"Enter the Initial");
			Display.Display(21, L"Temperature");
			Display.Display(22, csVar1);
			Display.Entry(3, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);
			//end of prompt for recipe selection

			Display.KillEntry();

			flInitialTemperature = Display.GetEnteredData(3);
			// IT sanity check
			if (flInitialTemperature < flMinGenIT || flInitialTemperature > flMaxGenIT)
			{
				bVerifyFlag = false;
				Display.KillEntry();
				Sleep(100);
				csiT = TempFtoStr(flInitialTemperature);
				Display.Display(20, (csVar1 + L", " + csiT));
				Display.Display(21, L"is not within the valid IT Range.");
				Display.Display(22, L" ");
				Display.Display(23, L"Press Enter To Continue.");
				Display.Log(L"IT Outside of Range. Re-Enter IT");
				Display.Entry(0, 0);

				Func_WaitForEnterKey(bKillTheTaskFlag, false);

				Display.KillEntry();
			}
		} while ((flInitialTemperature < flMinimumIT || flInitialTemperature > flMaxGenIT) && !bKillTheTaskFlag);

		
		Display.KillEntry(); 

		if (bKillTheTaskFlag) break;

		while (!bVerifyEntryYN && !bKillTheTaskFlag)
		{
			csVar1 = L"The IT Entered is";
			csiT = TempFtoStr(flInitialTemperature);
			Display.Display(20, csVar1);
			Display.Display(21, (csiT));
			Display.Display(22, L" Is this Entry Correct?");
			Display.DisplayStatus(L"Enter Y or N");		//Version 18 MDF
			Display.Entry(3, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			csEntry = Display.GetEnteredString(3);

			if ((csEntry.CompareNoCase(L"Y") == 0) || (csEntry.CompareNoCase(L"N") == 0))
			{
				bVerifyEntryYN = true;
				if (csEntry.CompareNoCase(L"Y") == 0)
				{
					Display.Log(L"Initial Temperature ", csiT);
					//Display.Log(L"IT Tray Time ", csTrayTime);

					if (iProductType == GENERIC)
					{
						if (flInitialTemperature < flMinimumIT)
						{
							csDebugiTvar2 = L"IT is out of range < ";
							csDebugiTvar.Format(L" %-5.1f °F", flMinimumIT);
							csDebugiTvar2 += csDebugiTvar;
							//Only display alarm at re-entry
							if (bReEnterFlag)
								Display.Alarm(2, 1, LOGTECAlarm, csDebugiTvar2);

							sRetortAlarmMessage = L"IT is out of range";  //RH v.iOPS
							sAlarmMessage_Hold = sRetortAlarmMessage;
							bAlarmsUpdate_iOPS = true;

							Display.Display(20, L"WARNING");
							Display.Display(22, L"Initial Temperature Entry");
							csVar1 = L"is Out of Range";
							Display.Display(23, csVar1);
							Display.DisplayStatus(L"Press Enter to Continue ...");
							Display.Entry(0, 0);
							
							Func_WaitForEnterKey(bKillTheTaskFlag, false);

							if (!bReEnterFlag) bReEnterFlag = true;
							else bReEnterFlag = false;
						}
					}
					if (iProductType == BALL)
					{
						if (flInitialTemperature < Ball.GetITLowValue())
						{
							bProcessDeviationFlag = true;
							//Only display alarm at re-entry
							if (bReEnterFlag)
								Display.Alarm(2, 1, LOGTECAlarm, L"Initial Temperature is too low", DEVIATION);
							sRetortAlarmMessage = L"Initial Temperature is too low";  //RH v.iOPS
							sAlarmMessage_Hold = sRetortAlarmMessage;
							bAlarmsUpdate_iOPS = true;

							Display.Display(20, L"WARNING");
							Display.Display(22, L"Initial Temperature Entry");
							csVar1 = L"is Out of Range";
							Display.Display(23, csVar1);
							Display.DisplayStatus(L"Press Enter to Continue ...");
							Display.Entry(0, 0);
							
							Func_WaitForEnterKey(bKillTheTaskFlag, false);

							if (!bReEnterFlag) bReEnterFlag = true;
							else bReEnterFlag = false;
						}
						if (flInitialTemperature > Ball.GetITHighValue())
						{
							//Only display alarm at re-entry
							if (bReEnterFlag)
								Display.Alarm(2, 1, LOGTECAlarm, L"Initial Temperature is Out of Range", NORMAL);
							sRetortAlarmMessage = L"Initial Temperature is Out of Range";  //RH v.iOPS
							sAlarmMessage_Hold = sRetortAlarmMessage;
							bAlarmsUpdate_iOPS = true;

							Display.Display(20, L"WARNING");
							Display.Display(22, L"Initial Temperature Entry");
							csVar1 = L"is Out of Range";
							Display.Display(23, csVar1);
							Display.DisplayStatus(L"Press Enter to Continue ...");
							Display.Entry(0, 0);
							
							Func_WaitForEnterKey(bKillTheTaskFlag, false);

							if (!bReEnterFlag) bReEnterFlag = true;
							else
							{
								bReEnterFlag = false;
								flInitialTemperature = (float)Ball.GetITHighValue();
							}
						}
						if ((flInitialTemperature > Ball.GetITLowValue()) && (flInitialTemperature < Ball.GetITHighValue()))
						{
							bReEnterFlag = false;
							bVerifyFlag = true;
						}
					} //end of if product type = ball
					if (bReEnterFlag)
						bVerifyFlag = false;// prompt a second time
					else bVerifyFlag = true; //Get out of loop
				}
				else // The result is No, so do it again by setting bVerifyFlag = false
				{
					Display.Log(L"Re-Enter IT");
					bVerifyFlag = false;
				}
			} //end of ((csEntry.CompareNoCase(L"Y") == 0) ||
			else //it's not a valid entry, prompt YN again
			{
				bVerifyEntryYN = true;//it must be set to true to re-enter
				bVerifyFlag = false;
			}
		}
	}
}	//end of Function PromptForInitialTemperature()


//================== PromptForMiGEntries =================
void PromptForCookOperatorEntries()
{

	int	iLoopCounter = 0; //time before alarming operator for slow entries

	bool bEntryFlag = true, bEntryFlag1 = true, bEntryFlag2 = true,
		bCalibration2ProblemFlag = false, bLowMIGEntryFlag = false;

	float flChartTemperature = 0;

	CString csVar1 = L"", csEntry = L"", strVar = L"", csCurrentTemp = L"", strEntry = L"",
		csMiGTemperature = L"", csChartTemperature = L"", csMainEntry = L"";

	if (!bKillTheTaskFlag)
		Display.Log(L"Operator Entries - Cook", L"", SendLogStatus);

	Display.Alarm(2, 1, LOGTECAlarm, L"** ATTENTION **", -1, -1, false);
	Sleep(500);
	Display.CancelAlarm(); // Made our own entry delay. Don't need this one.


	Display.KillEntry(); 

	do		// while (bEntryFlag1 == false)
	{
		bEntryFlag1 = false;

		Display.Display(21, L"Enter MIG");
		Display.Display(22, L"Temperature (F) ");
		Display.Entry(3, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();
		if (bKillTheTaskFlag) break;

		Display.ResetSnooze();

		flMIGTemperature = Display.GetEnteredData(3);

		Display.Display(21, L"Enter Chart");
		Display.Display(22, L"Temperature (F) ");
		Display.Entry(3, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();
		if (bKillTheTaskFlag) break;

		flChartTemperature = Display.GetEnteredData(3);

		if (!bKillTheTaskFlag && ((flMIGTemperature < 170) || (flMIGTemperature > 270) ||
			(flChartTemperature < 170) || (flChartTemperature > 270)))
		{
			Display.Display(20, L"MIG Entry = " + toStr(flMIGTemperature) + L" F");
			Display.Display(21, L"Chart Entry = " + toStr(flChartTemperature) + L" F");
			Display.Display(22, L"Entry Out of Range");
			Display.Display(22, L"Valid Between 170 F and 270 F");
			Display.DisplayStatus(L"Press Enter to Try Again ...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
			if (bKillTheTaskFlag) break;

			bEntryFlag1 = false;
		}	// end of if temperature is within legal range...
		else
		{
			Display.Display(20, L"MIG Entry = " + TempFtoStr(flMIGTemperature));
			Display.Display(21, L"Chart Entry = " + TempFtoStr(flChartTemperature));
			Display.Display(23, L"Are These Entries Correct?");
			Display.DisplayStatus(L" Select Y or N and Press the Enter Key...");
			Display.Entry(3, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			strEntry = Display.GetEnteredString(3);

			if (strEntry.CompareNoCase(L"Y") == 0)
			{
				strVar = TempFtoStr(flMIGTemperature);
				Display.Log(L"MIG Entry ", strVar, NORMAL);
				strVar = TempFtoStr(flChartTemperature);
				Display.Log(L"Chart Entry ", strVar, NORMAL);
				bEntryFlag1 = true;
			}
			else
			{
				bEntryFlag1 = false;
			}
			Display.KillEntry();
			if (bKillTheTaskFlag) break;
		}
		Sleep(250);
	} while (!bKillTheTaskFlag && !bEntryFlag1);	// stay in this loop until the entry is correct

	if (!bKillTheTaskFlag && (flMIGTemperature < flChartTemperature))
	{
		bProcessDeviationFlag = true;
		Display.Alarm(2, 1, LOGTECAlarm, L"Calibration", DEVIATION);
		Display.Display(20, L"Calibration Error!");
		Display.Display(21, L"MIG Entry " + toStr(flMIGTemperature) + L" F");
		Display.Display(22, L"Chart Entry = " + toStr(flChartTemperature) + L" F");
		Display.Display(23, L"Check MIG/Chart/RTD");
		Display.Display(24, L"Calibration");
		Display.DisplayStatus(L"Press Enter to Continue...");
		Display.Entry(0, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();

	}	// end of if flMIGTemperature < flChartTemperature

	if (!bKillTheTaskFlag && (flMIGTemperature < flCTEMP))
	{
		bProcessDeviationFlag = true;
		Display.Alarm(2, 1, LOGTECAlarm, L"LOW MIG Temperature", DEVIATION);
		bLowMIGEntryFlag = true;	// to avoid repetitive entries
		Display.Display(20, L"Low MIG!");
		Display.Display(21, L"Re-Check MIG");
		Display.Display(22, L"Re-Enter MIG");
		Display.Display(23, L"Enter 2nd MIG Reading");
		Display.DisplayStatus(L"Call Supervisor ...");
		Display.Entry(3, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();

		flMIGTemperature = Display.GetEnteredData(3);

		if (!bKillTheTaskFlag && (flMIGTemperature < flCTEMP))
		{
			bProcessDeviationFlag = true;
			strVar = toStr(flMIGTemperature) + L" F";
			Display.Log(L"2nd MIG Entry", strVar, DEVIATION);
			Display.Log(L"MIG DEVIATION", strVar, DEVIATION);
			Display.Log(L"MUST BE REVIEWED", strVar, DEVIATION);
			bInitiateLowMigDev = true;
		}
		Display.KillEntry();

	} // end of if flMIGTemperature < flCTEMP
	else bLowMIGEntryFlag = false; //else flMIGTemperature < flCTEMP

	if (((flMIGTemperature < (flCurrentTemperature - 2)) || (flMIGTemperature > (flCurrentTemperature + 2)))
		&& !bKillTheTaskFlag)
	{
		Display.Alarm(2, 1, LOGTECAlarm, L"Calibration Problem!", LOGERROR);
		bCalibration2ProblemFlag = true;	// to avoid repetitive entries
		Display.Display(20, L"MIG disagrees with RTD");
		Display.Display(21, L"Re-Check MIG");
		Display.Display(22, L"Re-Enter MIG");
		Display.Display(23, L"Enter 2nd MIG Reading");
		Display.DisplayStatus(L"Call Supervisor ...");
		Display.Entry(3, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();

		flMIGTemperature = Display.GetEnteredData(3);

		if (!bKillTheTaskFlag && (fabs(flMIGTemperature - flCurrentTemperature) > 2))
		{
			strVar = toStr(flMIGTemperature) + L" F";
			Display.Log(L"2nd MIG Entry", strVar, LOGERROR);
			Display.Log(L"Calibration problem!", strVar, LOGERROR);
		}
		Display.KillEntry();

	} // end of if flMIGTemperature < flCTEMP
	else bCalibration2ProblemFlag = false;

}	// end of PromptForCookOperatorEntries

//======================== SpawnTask 1 - EVERYTHING MAJOR CHECKED HERE ===================================
void MajorProblemChecks()
{
	CString csCTIME = L"", /*csCTEMP = L"",*/ csNewCTEMP = L"", csRTintervals = L"",
		csVar1 = L"", csVar2 = L"", csUnits = L"", csCPRS = L"", strCTEMP = L"";

	float	flMaxRTDSignal = 279.0,
		flMinRTDSignal = 35.0,
		flMaxPressure = 74.0, // Changed it to a higher pressure requested by Foodlab 06/28/18
		flMinPressure = -1.0,

		flCookPressureWarning = (float)(flMinimumCookPSI + 3.0),
		flPreviousPressure = 0.0,
		flCTIMEInMinutes = 0.0,
		flPreviousWaterLevel = 0.0,
		flCTEMPTemporary = 0.0,
		flTemperatureDiff = 0.0,
		flCTIMETemp = 0.0,
		flTemporaryValue = 0.0;



	int		itx = 0,
		ipx = 0,
		iwx = 0;

	long	lPreviousTempTime = 0,
		lPreviousWaterLevelTime = 0,
		lNumberIntervalsAway = 0;

	bool	bTempDEVIATIONOnce = false,
		bCirPumpTrip = false,
		bReelMotorTrip = false,
		bRetortConveyorTrip = false,
		bHighLevelFlag = false,
		bLowLevelFlag = false,

		bHighFlowFlag = false,
		bHighFlowDeBounceFlag = false,
		bCookFlowDeBounceFlag = false,
		bCookSDFlowDeBounceFlag = false,
		bCoolFlowDeBounceFlag = false,

		bLowSDFlowFlag = false,
		bDoor1LockTrip = false,
		bDoor2LockTrip = false,
		bLowAirTrip = false,
		bLowSteamTrip = false,
		bLowWaterTrip = false,
		bRTDsDifferDeBounceFlag = false,
		bHighLevelDeBounceFlag = false,
		bLowLevelDeBounceFlag = false,
		bRTDDifferenceFlag = false,
		bRPMFailFlag = false,
		bRPMTimerFlag = false,
		bAI_PREHT_RTDFailFlag = false,
		bPreHeatRTDHighTempFlag = false,	// Preheat RTD flag
		bRTD2HighTempFailFlag = false,	// RTD2 reset Flag
		bRTD1HighTempFailFlag = false,	// RTD1 reset Flag
		bPSIAboveSetPointFlag = false,	// PSI reset Flag
		bCoolingTempRTDFailed = false,
		bProxOn = false,
		bRTD1FailedFlag = false,
		bRTD2FailedFlag = false,
		bResetRTD1Flag = false,
		bResetRTD2Flag = false,
		bPump1OLFailed = false,
		bPump2OLFailed = false,
		bDebounceSteamUtilty = false;

	bMajorPBChecksRunning = true;

	RETORT_TEMP = AI_RTD1; // Always start with RTD 1

	while (!bKillMajorProblemChecks) // Check everything you want during processing in here
	{
		flCookPressureWarning = (float)(flMinimumCookPSI + 3.0);
		flTotalCycleTime = (20 + flComeUpTempTime1 + flComeUpTempTime2
			+ flComeUpTempTime3 + flComeUpTempTime4 + flComeUpTempTime5
			+ flCTIME + flCOOLTempTime1 + flCOOLTempTime2 + flCOOLTempTime3
			+ flCOOLTempTime4 + flCOOLTempHoldTime); // / 60 for minutes

		//=================== Check E-STOP =================
		if (!DigitalIO.ReadDigitalIn(DI_ESTOP_SR) && !bE_StopPushed) // E_Stop Engaged
		{
			TurnAllAnalogOutputsOff();
			TurnAllDigitalOutputsOff();
			bShutDownFlag = true;

			Display.Alarm(2, 1, LOGTECAlarm, L"EMERGENCY STOP PUSHED!", DEVIATION);
			sRetortAlarmMessage = L"EMERGENCY STOP PUSHED!";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bProcessDeviationFlag = true;
			bE_StopPushed = true;
		}
		if (bE_StopPushed && DigitalIO.ReadDigitalIn(DI_ESTOP_SR))
		{
			bE_StopPushed = false;
			Display.Log(L"E-STOP OK", L"", SendLogStatus);
		}

		/*		//=================== Check ESTOP Safety Relay =================
				// Do we want an alarm if the relay fails to reset? - MDF

				if((!DigitalIO.ReadDigitalIn(DI_MOTOR_CONTACTS_OK) || DigitalIO.ReadDigitalIn(DI_ESTOP_SR))
					&& !bSafety1RlyTrip )
				{
					bSafety1RlyTrip = true;
				}
				// Reset the safety relay of the EStop PB is OK
				if(bSafety1RlyTrip && !DigitalIO.ReadDigitalIn(DI_ESTOP_SR))
				{
					DigitalIO.SetDigitalOut(DO_ESTOP_RST, Onn);
					Sleep(1000);
					bSafety1RlyRst = true;

				}
				if(bSafety1RlyRst && DigitalIO.ReadDigitalIn(DI_MOTOR_CONTACTS_OK))
				{
					DigitalIO.SetDigitalOut(DO_ESTOP_RST, Off);
					bSafety1RlyRst = false;
					bSafety1RlyTrip = false;
				} */

				//=================== Check For Door #1 Secured =================
				// made minor changes in order to differentiate between Door States
		if (!DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR)
			&& bDoor1SecuredMode && !bDoor1LockTrip) //We only set this flag in drain - MDF
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"DOOR NOT SEALED!", SendLogStatus);
			sRetortAlarmMessage = L"DOOR NOT SEALED!";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bDoor1LockTrip = true;
			bShutDownFlag = true;
		}
		if (bDoor1LockTrip && DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR) && bDoor1SecuredMode)
		{
			Display.Log(L"Door in Closed Postion", L"", SendLogStatus);
			bDoor1LockTrip = false;
		}
		//============= Check For Door #1 Lock Secured ====================
		if (DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT) && bDoor1SecuredMode && !bDoorLockFailureFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"DOOR LOCKPIN NOT EXTENDED!", SendLogStatus);
			sRetortAlarmMessage = L"DOOR LOCKPIN NOT EXTENDED!";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bDoorLockFailureFlag = true;
			bKillTheTaskFlag = true;
			bShutDownFlag = true;
		}
		if (bDoorLockFailureFlag && !DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT) && bDoor1SecuredMode)
		{
			Display.Log(L"Door in Locked Position", L"", SendLogStatus);
			bDoorLockFailureFlag = false;
		}

		//=================== Check Pump OL 1 =================
		if (!DigitalIO.ReadDigitalIn(DI_CIRC_PUMP_OL_OK_1) && !bPump1OLFailed)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"PUMP 1 OVERLOAD!", SendLogStatus);
			bPump1OLFailed = true;
		}
		if (DigitalIO.ReadDigitalIn(DI_CIRC_PUMP_OL_OK_1) && bPump1OLFailed)
		{
			Display.Log(L"Pump 1 Overload OK");
			bPump1OLFailed = false;
		}
		//=================== Check Pump OL 2 =================
		if (!DigitalIO.ReadDigitalIn(DI_CIRC_PUMP_OL_OK_2) && !bPump2OLFailed)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"PUMP 2 OVERLOAD!", SendLogStatus);
			bPump2OLFailed = true;
		}
		if (DigitalIO.ReadDigitalIn(DI_CIRC_PUMP_OL_OK_2) && bPump2OLFailed)
		{
			Display.Log(L"Pump 2 Overload OK");
			bPump2OLFailed = false;
		}
		
		//=================== Check RETORT_TEMP =================
		if (bRTD1FailedFlag)// && bRTD2FailedFlag)
		{
			if (!bRetort_TempFailFlag)
			{
				if (bProcessStarted)
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"RETORT TEMP FAILED!", DEVIATION);
					sRetortAlarmMessage = L"RETORT TEMP FAILED!";  //RH v.iOPS
					sAlarmMessage_Hold = sRetortAlarmMessage;
					bAlarmsUpdate_iOPS = true;

					bProcessDeviationFlag = true;
				}
				else Display.Alarm(2, 1, LOGTECAlarm, L"RETORT TEMP FAILED!", NORMAL);
				sRetortAlarmMessage = L"RETORT TEMP FAILED!";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bRetort_TempFailFlag = true;
				bShutDownFlag = true;
			}
		}
		//=================== Reset RETORT_TEMP ===================
		if (bRTD1FailedFlag && !bResetRTD1Flag)
		{
			if ((flCurrentTemperature < flMaxRTDSignal) && (flCurrentTemperature > flMinRTDSignal))
			{
				bRTD1FailedFlag = false;
				bRetort_TempFailFlag = false;
				bResetRTD1Flag = true;
				RETORT_TEMP = AI_RTD1;
				bUpdateToCorrectRTD = true;
				Display.Log(L"RTD-1 OK - Reset", L"", NORMAL);
				Display.HideDisplay(1);
				Sleep(200);
				Display.DisplayVariable(1, L"Retort Temp (F)", L"float", (long)&flCurrentTemperature);
			}
		}

		// ==================== Check if RTD 1 gets out of range===========

		if (((flCurrentTemperature > flMaxRTDSignal) || (flCurrentTemperature < flMinRTDSignal)) && !bRTD1FailedFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"RTD-1 Failure: Switched to RTD-2!", NORMAL);
			bRTD1FailedFlag = true;
			bResetRTD1Flag = false;
			//			RETORT_TEMP	= AI_RTD2;
			//			bUpdateToCorrectRTD = true; // Global switched flag. Once, set on RTD Failure, reset in SelectRTDTuningTable.
			//			Display.HideDisplay(1);
			Sleep(200);
			//			Display.DisplayVariable(1, L"Backup Temp (F)", L"float", (long) &flCurrentTemperature);
		}
		// =============== Check RTD1 and RTD2 Temp if +8 than the setpoint========
		if ((flCurrentTemperature > (PID.GetPIDSetPoint(AO_STEAM) + 8.0))
			&& bCheckCriticalPSIFlag)
		{
			if (RETORT_TEMP == 1 && !bRTD1HighTempFailFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"HIGH RTD 1 TEMP", SendLogStatus);
				sRetortAlarmMessage = L"HIGH RTD 1 TEMP";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bRTD1HighTempFailFlag = true;
			}
			/*			if(RETORT_TEMP == 3 && !bRTD2HighTempFailFlag)
						{
							Display.Alarm(2, 1, LOGTECAlarm , L"HIGH RTD 2 TEMP", SendLogStatus);
							bRTD2HighTempFailFlag = true;
						} */
		}
		if (bCheckCriticalPSIFlag
			&& (flCurrentTemperature < (PID.GetPIDSetPoint(AO_STEAM) + 3.0)))
		{
			if (RETORT_TEMP == AI_RTD1 && bRTD1HighTempFailFlag)
			{
				bRTD1HighTempFailFlag = false;
				Display.Log(L"RTD 1 back within Set Point + 3°F", L"", SendLogStatus);
			}
			/*			if(RETORT_TEMP == AI_RTD2 && bRTD2HighTempFailFlag)
						{
							bRTD2HighTempFailFlag = false;
							Display.Log(L"RTD 2 back within Set Point + 3°F",L"", SendLogStatus);
						} */
		} // end of (RTD 1 and RTD 2) +8F check

		//=================== Check AI_PREHT_RTD =================
		if ((flCurrentPreHeatTemp > flMaxRTDSignal) || (flCurrentPreHeatTemp < flMinRTDSignal))
		{
			if (!bAI_PREHT_RTDFailFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"PREHEAT RTD FAILED!", NORMAL);
				sRetortAlarmMessage = L"PREHEAT RTD FAILED!";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bProcessDeviationFlag = false;
				bAI_PREHT_RTDFailFlag = true;

				if (!bPreHeatTaskCompleteFlag)
				{
					bShutDownFlag = true;
				}
			}
		}

		// =============== Check Preheat Temp if +8 than the setpoint========

	/*	if((flCurrentPreHeatTemp > (PID.GetPIDSetPoint(AO_STEAM) + 8.0)) && !bPreHeatRTDHighTempFlag &&
			bCheckCriticalPSIFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm , L"HIGH PREHEAT RTD TEMP", SendLogStatus);
			bPreHeatRTDHighTempFlag = true;
		}
		if((flCurrentPreHeatTemp < (PID.GetPIDSetPoint(AO_STEAM) + 2)) && bPreHeatRTDHighTempFlag)
		{
			bPreHeatRTDHighTempFlag = false;
			Display.Log(L"Preheat RTD Below Set Point + 2°F",L"", SendLogStatus);
		} // end of PH RTD +8 check

		// Check PSI to be higher by 3 psi than the Steam Saturated Pressure at that temp
		//==================================================================================

	/*	// ============= Check if INCOMING WATER TEMP RTD is out of range========
		if( !bCoolingTempRTDFailed && (((flCoolingSourceTemp > flMaxRTDSignal) ||
			(flCoolingSourceTemp < flMinRTDSignal))) && !bKillTheTaskFlag)
		{
			bCoolingTempRTDFailed = true;
			csVar1 = L"Source Water RTD Probe Failure ";
			csUnits = L" F";
			csVar2.Format(L" %-6.1f", flCoolingSourceTemp);
			csVar1 = csVar1+csVar2+csUnits;
			Display.Alarm(2, 1, LOGTECAlarm , csVar1);
		}
		if( bCoolingTempRTDFailed && (flCoolingSourceTemp < flMaxRTDSignal) &&
			(flCoolingSourceTemp > flMinRTDSignal) )
		{
			csVar1 = L"Source Water RTD Probe OK ";
			csUnits = L" F";
			csVar2.Format(L" %-6.1f", flCoolingSourceTemp);
			csVar1 = csVar1+csVar2+csUnits;
			Display.Log(csVar1);
		}
		// end INCOMING WATER TEMP RTD check */

		if (bCheckCriticalPSIFlag)
		{
			if ((flCurrentPressure <= (ConvertTemptoPress(flCurrentTempSetpoint) + 2.0)) && !bCriticalPSIFailureFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"LOW Retort Pressure", SendLogStatus);
				sRetortAlarmMessage = L"LOW Retort Pressure";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bCriticalPSIFailureFlag = true;
			}
			if ((flCurrentPressure > (ConvertTemptoPress(flCurrentTempSetpoint) + 3.0)) && bCriticalPSIFailureFlag)
			{
				bCriticalPSIFailureFlag = false;
			}
			// ================ Check Pressure Sensor if higher than PSI Setpoint + 3psi===

			if ((flCurrentPressure > (PID.GetPIDSetPoint(AO_SPLT_RNG) + 3.0)) && !bPSIAboveSetPointFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"HIGH Retort Pressure", SendLogStatus);
				sRetortAlarmMessage = L"HIGH Retort Pressure";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bPSIAboveSetPointFlag = true;

			}
			if ((flCurrentPressure <= (PID.GetPIDSetPoint(AO_SPLT_RNG) + 2.0)) && bPSIAboveSetPointFlag)
			{
				csCTEMP.Format(L" %-5.1f", flCurrentPressure);
				Display.Log(L"Pressure OK " + csCTEMP);
				bPSIAboveSetPointFlag = false;
			}		// end of PSI check for if abobe Setpoint
		} // end vapor PSI check

		//=================== Check RETORT_PRESSURE =================
		if ((flCurrentPressure > flMaxPressure) && !bRetort_PresFailFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"RETORT PRESSURE ABOVE HIGH LIMIT", SendLogStatus);
			sRetortAlarmMessage = L"RETORT PRESSURE ABOVE HIGH LIMIT";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bRetort_PresFailFlag = true;
			bShutDownFlag = true;
		}
		if ((flCurrentPressure < flMinPressure) && !bRetort_PresFailFlag)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"RETORT PRESSURE BELOW LOW LIMIT", SendLogStatus);
			sRetortAlarmMessage = L"RETORT PRESSURE BELOW LOW LIMIT";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bRetort_PresFailFlag = true;
			bShutDownFlag = true;
		}
		if (bRetort_PresFailFlag && (flCurrentPressure < (flMaxPressure - 2)) &&
			(flCurrentPressure > (flMinPressure + 1)))
		{
			Display.Log(L"Pressure Within Safe Range", L"", SendLogStatus);
			bRetort_PresFailFlag = false;
		}
		//=================== Check Minimum Cook Pressure =================
		if ((flCurrentPressure < flCookPressureWarning) && bCheckForMinPSIFlag)
		{
			if ((flCurrentPressure < flMinimumCookPSI) && bCheckForMinPSIFlag && !bCook_PressWarnFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"Cook Pressure Is Below Setpoint", SendLogStatus);
				sRetortAlarmMessage = L"Cook Pressure Is Below Setpoint";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bCook_PressWarnFlag = true;
			}
			if ((flCurrentPressure < flMinimumCookPSI) && bCheckForMinPSIFlag && !bCook_PresFailFlag)
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"COOK PRESSURE IS TOO LOW!!", DEVIATION);
				sRetortAlarmMessage = L"COOK PRESSURE IS TOO LOW!!";  //RH v.iOPS
				sAlarmMessage_Hold = sRetortAlarmMessage;
				bAlarmsUpdate_iOPS = true;

				bCook_PresFailFlag = true;
				bProcessDeviationFlag = true;
			}
		}
		if ((flCurrentPressure >= flCookPressureWarning) && bCook_PressWarnFlag)
		{
			Display.Log(L"Cook Pressure Back Within Range", L"", SendLogStatus);
			bCook_PresFailFlag = false;
			bCook_PressWarnFlag = false;
		}
		//=================== Check For DEVIATIONs =================
		if ((iProductType == GENERIC) && !bTempDEVIATIONOnce && bDoDeviationCheck &&
			(flCurrentTemperature < flCTEMP))
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"TEMPERATURE DEVIATION!!!", DEVIATION);
			sRetortAlarmMessage = L"TEMPERATURE DEVIATION!!!";  //RH v.iOPS
			sAlarmMessage_Hold = sRetortAlarmMessage;
			bAlarmsUpdate_iOPS = true;

			bProcessDeviationFlag = true;
			bTempDEVIATIONOnce = true;
			bTempDeviationFlag = true;
			PinesTime18.StartTimer();	// low Cook Temp clock
			PinesTime18.ZeroTime();		// zero timer
		}
		if ((iProductType == GENERIC) && (flCurrentTemperature < flCTEMP)
			&& (flCurrentTemperature < flCookLowestTemp))
		{
			flCookLowestTemp = flCurrentTemperature;
		}
		if ((iProductType == GENERIC) && bTempDEVIATIONOnce
			&& bDoDeviationCheck && (flCurrentTemperature > (flCTEMP + (float).25)))
		{
			flLowCookTemperatureTime = flLowCookTemperatureTime + PinesTime18.GetElapsedTime();
			PinesTime18.ZeroTime();		// zero timer
			PinesTime18.StopTimer();

			Display.Log(L"Retort Temperature OK");
			bTempDEVIATIONOnce = false;
			bDisplayDeviationInfo = true;
		}
		//=================== Check For Deviations =================
		if (!bNoMoreValidProcessFlag && (iProductType == BALL) && bDoDeviationCheck
			&& ((flCurrentTemperature < flCTEMP) || bInitiateLowMigDev))
		{
			Ball.SetCurrentProduct(iProductNumber);
			flRTintervals = (float)Ball.GetRTIntValue();

			if (bInitiateLowMigDev)
				flTemperatureDiff = flCTEMP - flMIGTemperature;
			else
				flTemperatureDiff = flCTEMP - flCurrentTemperature;

			if (flRTintervals != 0)
				flTemporaryValue = flTemperatureDiff / flRTintervals;
			else flTemporaryValue = flTemperatureDiff;

			if ((flTemporaryValue - (int)flTemporaryValue) > 0)
			{
				lNumberIntervalsAway = (long)flTemporaryValue + 1;
				flCTEMPTemporary = flCTEMP - ((float)lNumberIntervalsAway * flRTintervals);
			}
			else
			{
				lNumberIntervalsAway = (long)flTemporaryValue;
				flCTEMPTemporary = flCTEMP - ((float)lNumberIntervalsAway * flRTintervals);
			}
			if (flCTEMPTemporary >= Ball.GetLowRTValue())
			{
				bNoMoreValidProcessFlag = false;
				flCTEMP = flCTEMPTemporary;

				if (bInitiateLowMigDev)
				{
					bInitiateLowMigDev = false;
					strCTEMP.Format(L" %-5.1f", flCTEMPTemporary);
				}
				else
				{
					PID.PIDSetPoint(AO_STEAM, flCTEMP, 1.0);
					strCTEMP.Format(L" %-5.1f", flCTEMP);
				}
				//In case of deviation use actual IT instead of minimum IT
				flCTIMETemp = (float)PV.CalculateDeviatedBallProcess(flCTEMP, flInitialTemperature);

				if (flCTIMETemp < flMinimumCookTime)
					flCTIMETemp = flMinimumCookTime;

				csCTIME = SecondsToStr(flCTIMETemp);
				Display.Alarm(2, 1, LOGTECAlarm, L"ALTERNATE PROCESS", 1);
				Display.Log(L"The New Process is" + csCTIME + " at " + strCTEMP + "°F");

				if (flCTIMEAddedAccumSec > 0)	//if cook time was added previously
				{
					flCTIMETemp = flCTIMETemp + flCTIMEAddedAccumSec;
					Display.Log(L"Plus Added Cook Time of " + toStr(flCTIMEAddedAccumSec) + L" Sec");
				}
				flCTIME = flCTIMETemp;

				DisplayCompleteProcessScreen();
			}
			else
			{
				if (!bNoMoreValidProcessFlag)
				{
					bProcessDeviationFlag = true;
					Display.Alarm(2, 1, LOGTECAlarm, L"NO MORE VALID ALTERNATE PROCESSES!", DEVIATION);
					Display.Alarm(2, 1, LOGTECAlarm, L"YOU MUST OVERRIDE TO COOLING!!!", DEVIATION);
				}
				bNoMoreValidProcessFlag = true;
			}
		}

		//=================== Check Reel Motor Overload =================
		/*if (!DigitalIO.ReadDigitalIn(DI_DRV_OL_OK) && !bReelMotorTrip)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"REEL MOTOR OVERLOAD TRIPPED!", SendLogStatus);
			bReelMotorTrip = true;
		}

		if (bReelMotorTrip && DigitalIO.ReadDigitalIn(DI_DRV_OL_OK))
		{
			Display.Log(L"Reel Motor Overload is OK");
			bReelMotorTrip = false;
		}*/

		//=================== Check High Water Level in Retort =================
		if (bRunLevelControlFlag && bFillLevelAdjustFlag) //wait 2 minutes for initial fill level to adjust if needed GMK
		{
			if ((flCurrentWaterLevel > flWaterHighLevelAlarm) && !bHighLevelFlag)
			{
				if (!bHighLevelDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime10.StartTimer(); // Water HIGH Level Filter Alarm Timer - FILTER
					PinesTime10.ZeroTime(); // Water HIGH Level Filter Alarm Timer - FILTER
					bHighLevelDeBounceFlag = true;
				}
				if (PinesTime10.GetElapsedTime() > 20) // Water HIGH Level Filter Alarm Timer - FILTER
				{
					if (!bEnblCoolWaterFlow)
					{
						Display.Alarm(2, 1, LOGTECAlarm, L"High Water Level in Heating", DEVIATION);
						bProcessDeviationFlag = true;
						PinesTime10.StopTimer();
						PinesTime10.ZeroTime();
					}
					else
					{
						Display.Alarm(2, 1, LOGTECAlarm, L"High Water Level in Cooling", NORMAL);
						PinesTime10.StopTimer();
						PinesTime10.ZeroTime();
					}
					bHighLevelFlag = true;
				}
			}
			//Reset if the level recovers before the timer is made
			if (bHighLevelDeBounceFlag && (flCurrentWaterLevel <= flWaterHighLevelAlarm) && !bHighLevelFlag)
			{
				bHighLevelDeBounceFlag = false;
				PinesTime10.StopTimer();
				PinesTime10.ZeroTime();
			}
			if (bHighLevelFlag && (flCurrentWaterLevel <= flWaterDrainOnLevel))
			{
				Display.Log(L"Water Level OK", L"", NORMAL);
				bHighLevelFlag = false;
				bHighLevelDeBounceFlag = false;
				PinesTime10.StopTimer();
				PinesTime10.ZeroTime();
			}
			//=================== Check Low Water Level in Retort =================
			if (DigitalIO.ReadDigitalOut(DO_PUMP_1) && (flCurrentWaterLevel < flWaterLowLevelAlarm) &&
				!bLowLevelFlag)
			{
				if (!bLowLevelDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime11.StartTimer(); // Water LOW Level Filter Alarm Timer - FILTER
					PinesTime11.ZeroTime(); // Water LOW Level Filter Alarm Timer - FILTER
					bLowLevelDeBounceFlag = true;
				}
				if (PinesTime11.GetElapsedTime() > 20) // Water LOW Level Filter Alarm Timer - FILTER
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"Low Water Level", SendLogStatus);
					bLowLevelFlag = true;
				}
			}
			if (bLowLevelDeBounceFlag && (flCurrentWaterLevel >= flWaterLowLevelAlarm) && !bLowLevelFlag)
			{
				PinesTime11.StopTimer();
				PinesTime11.ZeroTime();
				bLowLevelDeBounceFlag = false;
			}
			if (bLowLevelFlag && (flCurrentWaterLevel >= flWaterDrainOffLevel))
			{
				Display.Log(L"Water Level OK", L"", NORMAL);
				bLowLevelFlag = false;
				bLowLevelDeBounceFlag = false;
				PinesTime11.StopTimer();
				PinesTime11.ZeroTime();
			}
			//=================== Check HIGH Water Flow Alarm in Retort - in ComeUP, COOK OR COOL PHASE - BAD NOZZLES ===========
			if ((flCurrentWaterFlow > flWaterHighFlowRate) && !bHighFlowFlag)
			{
				if (!bHighFlowDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime4.StartTimer(); // Flow Alarm Filter Timer
					PinesTime4.ZeroTime(); // Flow Alarm Filter Timer
					bHighFlowDeBounceFlag = true;
				}
				if (PinesTime4.GetElapsedTime() > 10) // Flow Alarm Filter Timer
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"High Water Flow Detected", SendLogStatus);
					bHighFlowFlag = true;
				}
			}
			if (bHighFlowDeBounceFlag && (flCurrentWaterFlow <= flWaterHighFlowRate) && !bHighFlowFlag)
			{
				bHighFlowDeBounceFlag = false;
				PinesTime4.StopTimer();
				PinesTime4.ZeroTime();
			}
			if (bHighFlowFlag && (flCurrentWaterFlow < (flWaterHighFlowRate - 10)))
			{
				Display.Log(L"High Water Flow OK", L"", NORMAL);
				bHighFlowFlag = false;
				bHighFlowDeBounceFlag = false;
				PinesTime4.StopTimer();
				PinesTime4.ZeroTime();
			}
			//=================== Check Water Flow Alarm in Retort - in COOK PHASE =================
			if (!bEnblCoolWaterFlow && (flCurrentWaterFlow <= flHeatingWaterLowFlowRate) && !bLowHeatingFlowFlag)
			{
				if (!bCookFlowDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime4.StartTimer(); // Flow Alarm Filter Timer
					PinesTime4.ZeroTime(); // Flow Alarm Filter Timer
					bCookFlowDeBounceFlag = true;
				}
				if (PinesTime4.GetElapsedTime() > 10) // Flow Alarm Filter Timer
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"Low Water Flow Detected", DEVIATION);
					bProcessDeviationFlag = true;
					bLowHeatingFlowFlag = true;
				}
			}
			if (bCookFlowDeBounceFlag && (flCurrentWaterFlow > flHeatingWaterLowFlowRate) && !bLowHeatingFlowFlag)
			{
				bCookFlowDeBounceFlag = false;
				PinesTime4.StopTimer();
				PinesTime4.ZeroTime();
			}
			if (bLowHeatingFlowFlag && flCurrentWaterFlow > (flHeatingWaterLowFlowRate + 5))
			{
				Display.Log(L"Low Flow OK", L"", NORMAL);
				bLowHeatingFlowFlag = false;
				bCookFlowDeBounceFlag = false;
				PinesTime4.StopTimer();
				PinesTime4.ZeroTime();
			}
			//=================== Check Water Flow Alarm in Retort - in COOLING PHASE =================
			if (bEnblCoolWaterFlow && flCurrentWaterFlow <= flCoolWaterLowFlowRate && !bLowCoolFlowFlag)
			{
				if (!bCoolFlowDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime4.StartTimer(); // Flow Alarm Filter Timer
					PinesTime4.ZeroTime(); // Flow Alarm Filter Timer
					bCoolFlowDeBounceFlag = true;
				}
				if (PinesTime4.GetElapsedTime() > 10) // Flow Alarm Filter Timer
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"Low Cooling Water Flow Detected", SendLogStatus);
					bLowCoolFlowFlag = true;
				}
			}
			if (bCoolFlowDeBounceFlag && (flCurrentWaterFlow > flCoolWaterLowFlowRate) && !bLowCoolFlowFlag)
			{
				bCoolFlowDeBounceFlag = false;
				PinesTime20.StopTimer();
				PinesTime20.ZeroTime();
			}
			if (bLowCoolFlowFlag && flCurrentWaterFlow > (flCoolWaterLowFlowRate + 5))
			{
				Display.Log(L"Low Cool Flow OK", L"", NORMAL);
				bLowCoolFlowFlag = false;
				bCoolFlowDeBounceFlag = false;
				PinesTime20.StopTimer();
				PinesTime20.ZeroTime();

			}
			//=================== Check Water Flow Alarm in Retort - FOR Shutdown =================
			if (!bEnblCoolWaterFlow && flCurrentWaterFlow <= flCookWaterShutDownFlowRate && !bLowSDFlowFlag)
			{
				if (!bCookSDFlowDeBounceFlag)
				{
					// Start Debounce Timer
					PinesTime8.StartTimer(); // Flow SHUTDOWN Filter Timer
					PinesTime8.ZeroTime(); // Flow SHUTDOWN Filter Timer
					bCookSDFlowDeBounceFlag = true;
				}
				if (PinesTime8.GetElapsedTime() > 10) // Flow Alarm Filter Timer
				{
					Display.Alarm(2, 1, LOGTECAlarm, L"Very Low Water Flow Detected in Retort", DEVIATION);
					bProcessDeviationFlag = true;
					bLowSDFlowFlag = true;
				}
			}
			if (bCookSDFlowDeBounceFlag && (flCurrentWaterFlow > flCookWaterShutDownFlowRate) && !bLowSDFlowFlag)
			{
				bCookSDFlowDeBounceFlag = false;
				PinesTime8.StopTimer();
				PinesTime8.ZeroTime();
			}
			if (bLowSDFlowFlag && flCurrentWaterFlow > (flCookWaterShutDownFlowRate + 5))
			{
				//			Display.Log(L"Water Flow OK", L"", NORMAL);
				PinesTime8.StopTimer();
				PinesTime8.ZeroTime();

				bCookSDFlowDeBounceFlag = false;
				bLowSDFlowFlag = false;
			}
		} // End of water flow / level stabalized filter

		//=================== Check Air Supply Pressure =================
		if (!DigitalIO.ReadDigitalIn(DI_AIR_OK) && !DigitalIO.ReadDigitalIn(DI_DOOR_CLOSED_SR)
			&& !DigitalIO.ReadDigitalIn(DI_DOOR_PIN_NOT_EXT) && !bLowAirTrip)
		{
			if (!bAirAlmTestFlag)
			{
				PinesTime15.StartTimer(); // Air Supply Filter Alarm Timer
				PinesTime15.ZeroTime();	 // Air Supply Filter Alarm Timer
			}
			bAirAlmTestFlag = true;
			if (PinesTime15.GetElapsedTime() > 30) // Filter Time
			{
				Display.Alarm(2, 1, LOGTECAlarm, L"LOW AIR SUPPLY", SendLogStatus);
				bLowAirTrip = true;
			}
		}
		if (bLowAirTrip && DigitalIO.ReadDigitalIn(DI_AIR_OK))
		{
			Display.Log(L"Air Supply OK", L"", SendLogStatus);
			PinesTime15.StopTimer(); //ver12 replaced zeroTimer with StopTimer
			bLowAirTrip = false;
			bAirAlmTestFlag = false;
		}
		//=================== Check Steam Supply Pressure =================
		if (!DigitalIO.ReadDigitalIn(DI_STEAM_OK) && !bDebounceSteamUtilty)
		{
			bDebounceSteamUtilty = true;
			PinesTime34.StartTimer();
			PinesTime34.ZeroTime();
		}
		if ((bLowSteamTrip || bDebounceSteamUtilty) && DigitalIO.ReadDigitalIn(DI_STEAM_OK))
		{
			PinesTime34.StopTimer();
			PinesTime34.ZeroTime();
			bDebounceSteamUtilty = false;
			if (bLowSteamTrip)
			{
				Display.Log(L"Steam Supply OK", L"", SendLogStatus);
				bLowSteamTrip = false;
			}
		}
		if (!DigitalIO.ReadDigitalIn(DI_STEAM_OK) && (PinesTime34.GetElapsedTime() > 25) && !bLowSteamTrip)
		{
			PinesTime34.StopTimer();
			Display.Alarm(2, 1, LOGTECAlarm, L"LOW STEAM SUPPLY", SendLogStatus);
			bLowSteamTrip = true;
		}
		//=================== Check Water Supply Pressure =================
		if (!DigitalIO.ReadDigitalIn(DI_WATER_OK) && !bLowWaterTrip)
		{
			Display.Alarm(2, 1, LOGTECAlarm, L"LOW WATER SUPPLY", SendLogStatus);
			bLowWaterTrip = true;
		}
		if (bLowWaterTrip && DigitalIO.ReadDigitalIn(DI_WATER_OK))
		{
			Display.Log(L"Water Supply OK", L"", SendLogStatus);
			bLowWaterTrip = false;
		}

		Sleep(150);

	}	//	Process Complete Loop

	bMajorPBChecksRunning = false;

	Display.HideDisplay(12);
	TaskLoop.KillTask(1);

}	// Major Problems Task Loop

/*===================== Function GetGoodRTD ===============================
Function designed to select desired Tuning Table - based on RTD failed
Passed the two choices - returns correct table
input for the PID loop for AO_STEAM and AO_WATER valves.
Choose same Tuning Table value - just change RTD selection for the AiN
*/
long SelectRTDTuningTable(long lPrimaryTable, long lBackupTable)
{
	bUpdateToCorrectRTD = false; // Global trigger flag.- set in Major Problem Checks and reset here.

	if (RETORT_TEMP == 1)
		return lPrimaryTable;
	else
		return lBackupTable;
}


//================== KillTheProcess() =================
void KillTheProcess()
{
	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bKillTheTaskFlag = false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS
	bDoDeviationCheck = false;	// Set in COOK
	bCookTimeCompleted = false;

	Display.Phase(L"");
	sProcessRecipeName = L"Null"; 

	// Kill Only PID Loops - That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);
	PID.PIDStop(AO_SPLT_RNG);

	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
	DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
	DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
	DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
	bPumpIsRunning = false;


	// Now Set All AO to what is needed in this phase
	AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
	AnalogIO.SetAnalogOut(AO_AIR, 0.0);
	AnalogIO.SetAnalogOut(AO_VENT, 0.0);
	AnalogIO.SetAnalogOut(AO_WATER, 0.0);

	Sleep(2000);
	// *** END OF COMMON INITIALIZATION STUFF

	Display.KillEntry(); 

	if (bShutDownFlag)
	{
		SendLogStatus = ABORT;
		Display.Log(L"Process Aborted", L"", SendLogStatus);

		Display.Display(20, L"Restarting Process");
		Display.Display(21, L"");
		Display.Display(22, L"");
		Display.DisplayStatus(L"Please wait...");
		Display.Entry(0, 5);

		while (!Display.IsEnterKeyPressed())
			Sleep(250);
	}
	else
		Display.Phase(L"End");
	sPhaseMessage = L"End"; 

	PinesTime1.StopTimer();		// Phase, Step, OR Segment Timer (sec)
	PinesTime1.ZeroTime();		// Misc Timer for Major Problem Checks Use

	PinesTime2.StopTimer();		// Operator Entry Screen Timer (sec)
	PinesTime2.ZeroTime();		// Misc Timer for Major Problem Checks Use

	PinesTime3.StopTimer();		// Delay - Phase, Step, or Segment Timer (sec)
	PinesTime3.ZeroTime();		// Misc Timer for Major Problem Checks Use

	PinesTime4.StopTimer();		// Flow Alarm Filter Timer
	PinesTime4.ZeroTime();		// Flow Alarm Filter Timer

	PinesTime5.StopTimer();		// Total ComeUp through CoolDown Time
	PinesTime5.ZeroTime();		// Misc Timer for Major Problem Checks Use

	PinesTime6.StopTimer();		// Rocking/Rotation Function Timer
	PinesTime6.ZeroTime();		// Rocking/Rotation Function Timer

	PinesTime7.StopTimer();		// Water Level Filter Alarm Timer - initial
	PinesTime7.ZeroTime();		// Water Level Filter Alarm Timer - initial

	PinesTime8.StopTimer();		// Flow SHUTDOWN Filter Timer
	PinesTime8.ZeroTime();		// Flow SHUTDOWN Filter Timer

	PinesTime9.StopTimer();		// RTD's Differ Alarm Filter Timer
	PinesTime9.ZeroTime();		// RTD's Differ Alarm Filter Timer

	PinesTime10.StopTimer();	// Water HIGH Level Filter Alarm Timer - FiLTER
	PinesTime10.ZeroTime();		// Water HIGH Level Filter Alarm Timer - FiLTER

	PinesTime11.StopTimer();	// Water LOW Level Filter Alarm Timer - FiLTER
	PinesTime11.ZeroTime();		// Water LOW Level Filter Alarm Timer - FiLTER

	PinesTime14.StopTimer();	// ComeUP and cooling PRESSURE phase timer.
	PinesTime14.ZeroTime();		// ComeUP and cooling PRESSURE phase timer.

	PinesTime15.StopTimer();	// Air Supply Pressure Alarm Delay Timer
	PinesTime15.ZeroTime();		// Air Supply Pressure Alarm Delay Timer

	PinesTime16.StopTimer();
	PinesTime16.ZeroTime();

	PinesTime17.StopTimer();
	PinesTime17.ZeroTime();

	PinesTime18.StopTimer();
	PinesTime18.ZeroTime();

	PinesTime19.StopTimer();
	PinesTime19.ZeroTime();

	PinesTime20.StopTimer();
	PinesTime20.ZeroTime();

	PinesTime21.StopTimer();
	PinesTime21.ZeroTime();

	PinesTime22.StopTimer();
	PinesTime22.ZeroTime();

	PinesTime33.StopTimer();
	PinesTime33.ZeroTime();

	PinesTime34.StopTimer();
	PinesTime34.ZeroTime();

	Profiler.StopProfiler();

	//	Display.Log(L"End of Process", L"", SendLogStatus);
	Display.Display(0, L"");
	Display.HideDisplay(13);

	Sleep(3000);

	Display.EndControl(); // Clear the Product info

	Sleep(3000);

	Display.OverrideEnable();

	Display.KillEntry();

}// end of function KillTheProcess()

//================== LoadProcessVariables() =================
void LoadProcessVariables()
{
	bSWSOnlyRecipeFlag = false;
	bFullImmersionRecipeFlag = false;

	flDirectCoolLevel = 0.0;
	flWaterHighLevelAlarm = 0.0;
	flWaterLowLevelAlarm = 0.0;
	flWaterDrainOnLevel = 0.0;
	flWaterDrainOffLevel = 0.0;

	if (iProductNumber == 9999)
		bCleaningRecipeSelected = true;

	if ((iProductNumber > 1) && (iProductNumber < 9999))
		bSWSOnlyRecipeFlag = true;


	//Load the process variables that and condition variables for each unique process
	if (bSWSOnlyRecipeFlag || bCleaningRecipeSelected)
	{
		flInitialFillLevel =	 PV.MovePV(1);	// PV1  Initial Fill Level
		flInitialTempOffset =	 PV.MovePV(2);	// PV2  Temperature bias for target temperature
		flInitialPressure =		 PV.MovePV(3);	// PV3  Initial vessel target pressure
		flComeUpTempStep1 =		 PV.MovePV(4);	// PV4  1st Comeup segment target temperature
		flComeUpTempTime1 =		 PV.MovePV(5);	// PV5  1st Comeup segment minimum time
		flComeUpTempStep2 =		 PV.MovePV(6);	// PV6  2nd Comeup segment target temperature
		flComeUpTempTime2 =		 PV.MovePV(7);	// PV7  2nd Comeup segment minimum time
		flComeUpTempStep3 =		 PV.MovePV(8);	// PV8  3rd Comeup segment target temperature
		flComeUpTempTime3 =		 PV.MovePV(9);	// PV9  3rd Comeup segment minimum time
		flComeUpTempStep4 =		 PV.MovePV(10);	// PV10 4th Comeup segment target temperature
		flComeUpTempTime4 =		 PV.MovePV(11);	// PV11 4th Comeup segment minimum time
		flComeUpTempStep5 =		 PV.MovePV(12);	// PV12 5th Comeup segment target temperature
		flComeUpTempTime5 =		 PV.MovePV(13);	// PV13 5th Comeup segment minimum time
		flSteamBoostOff	=		 PV.MovePV(14);	// PV14 Temperature the Steam Boost turns off durring Come up
		flHotPSIStepA =			 PV.MovePV(15);	// PV15 1st Hot segment target pressure
		flHotPSITimeA =			 PV.MovePV(16);	// PV16 1st Hot segment minimum time
		flHotPSIStepB =			 PV.MovePV(17);	// PV17 2nd Hot segment target pressure
		flHotPSITimeB =			 PV.MovePV(18);	// PV18 2nd Hot segment minimum time
		flHotPSIStepC =			 PV.MovePV(19);	// PV19 3rd Hot segment target pressure
		flHotPSITimeC =			 PV.MovePV(20);	// PV20 3rd Hot segment minimum time
		flHotPSIStepD =			 PV.MovePV(21);	// PV21 4th Hot segment target pressure
		flHotPSITimeD =			 PV.MovePV(22);	// PV22 4th Hot segment minimum time
		flHotPSIStepE =			 PV.MovePV(23);	// PV23 5th Hot segment target pressure
		flHotPSITimeE =			 PV.MovePV(24);	// PV24 5th Hot segment minimum time
		flHotPSIStepF =			 PV.MovePV(25);	// PV25 6th Hot segment target pressure
		flHotPSITimeF =			 PV.MovePV(26);	// PV26 6th Hot segment minimum time
		flMinimumCookPSI =		 PV.MovePV(27);	// PV27 Minimum cook pressure for alarm & deviation
		flOverShootTime =		 PV.MovePV(28);	// PV28 Overshoot duration before going into Cook
		flRampDownToCTEMPTime =	 PV.MovePV(29);	// PV29 Ramp duration from Overshoot to Cook
		flCookStabilizationTime = PV.MovePV(30);	// PV30 Cook stabilization time
		flMicroCoolTime 	 =	PV.MovePV(32);	// PV32 Micro Cool time
		flCOOLTempStep1 =		 PV.MovePV(33);	// PV33 1st Cool segment target temperature
		flCOOLTempTime1 =		 PV.MovePV(34);	// PV34 1st Cool segment minimum time
		flCOOLTempStep2 =		 PV.MovePV(35);	// PV35 2nd Cool segment target temperature
		flCOOLTempTime2 =		 PV.MovePV(36);	// PV36 2nd Cool segment minimum time
		flCOOLTempStep3 =		 PV.MovePV(37);	// PV37 3rd Cool segment target temperature
		flCOOLTempTime3 =		 PV.MovePV(38);	// PV38 3rd Cool segment minimum time
		flCOOLTempStep4 =		 PV.MovePV(39);	// PV39 4th Cool segment target temperature
		flCOOLTempTime4 =		 PV.MovePV(40);	// PV40 4th Cool segment minimum time
		flCOOLTempHoldTime =	 PV.MovePV(41);	// PV41 Last Cool segment minimum hold time
		flCOOLPSIStepA =		 PV.MovePV(42);	// PV42 1st Cool segment target pressure
		flCOOLPSITimeA =		 PV.MovePV(43);	// PV43 1st Cool segment minimum time
		flCOOLPSIStepB =		 PV.MovePV(44);	// PV44 2nd Cool segment target pressure
		flCOOLPSITimeB =		 PV.MovePV(45);	// PV45 2nd Cool segment minimum time
		flCOOLPSIStepC =		 PV.MovePV(46);	// PV46 3rd Cool segment target pressure
		flCOOLPSITimeC =		 PV.MovePV(47);	// PV47 3rd Cool segment minimum time
		flCOOLPSIStepD =		 PV.MovePV(48);	// PV48 4th Cool segment target pressure
		flCOOLPSITimeD =		 PV.MovePV(49);	// PV49 4th Cool segment minimum time
		flCOOLPSIStepE =		 PV.MovePV(50);	// PV50 5th Cool segment target pressure
		flCOOLPSITimeE =		 PV.MovePV(51);	// PV51 5th Cool segment minimum time
		flComeUpMotionProfile =	 PV.MovePV(53);	// PV53 Motion Profile used during Come Up
		flCookMotionProfile =	 PV.MovePV(54);	// PV54 Motion Profile used during Cook
		flCoolingMotionProfile = PV.MovePV(55);	// PV55 Motion Profile used during Pressure Cool
		lProcVar_RecipeType	=(long) PV.MovePV(56); //  Recipe Type Verification 0 = Ball, 1 = Generic and 2 =  NumeriCAL

	}

	if (flCOOLTempTime1 == 0) flCoolTempTotalTime = 0;
	else
	{
		if (flCOOLTempTime2 == 0) flCoolTempTotalTime = flCOOLTempTime1;
		else
		{
			if (flCOOLTempTime3 == 0) flCoolTempTotalTime = flCOOLTempTime1 + flCOOLTempTime2;
			else
			{
				if (flCOOLTempTime4 == 0) flCoolTempTotalTime = flCOOLTempTime1 + flCOOLTempTime2 + flCOOLTempTime3;
				else
				{
					flCoolTempTotalTime = flCOOLTempTime1 + flCOOLTempTime2 + flCOOLTempTime3 + flCOOLTempTime4;
				}
			}
		}
	}
	if (flCOOLTempHoldTime != 0)
	{
		flCoolTempTotalTime = flCoolTempTotalTime + flCOOLTempHoldTime;
	}

	flPreHeatTemp = flInitialTemperature + flInitialTempOffset;

	flMaxVentValveOutput = 100.0;	// Limit for Valve position %
	flMaxAirValveOutput = 100.0;	// Limit for Valve position %
	flZeroValueCrossOver = 49.0;	// Point for both Valves CLOSED position %
	flDeadBandSetting = 2.0;		// 
	flMinimumOutputBias = 0.0;		// Minimum Output Bias %

	// Begin Section for All items that are "Process" Alarms & Non-Visable Variables
	flLongFillTime = 300;

	// These alarm values will change with every validated system
	flWaterHighFlowRate = 1800;
	flHeatingWaterLowFlowRate = 1400;
	flCoolWaterLowFlowRate = 1200;
	flCookWaterShutDownFlowRate = 1300;

	flWaterHighLevelAlarm = 65.0;
	flWaterLowLevelAlarm = 30.0;

	flWaterDrainOnLevel = 50.0;
	flWaterDrainOffLevel = 45.0;

	bRecipeSelectionFlag = true; // Successful Recipe & PV Load flag.

	lCycleNumber = Display.ReadCurrentCycleNumber();
	AnalogIO.SetAnalogOut(VAO_CycleNumber, (float)lCycleNumber); 

	DisplayCompleteProcessScreen();

} // End of LoadProcessVariables()

//======================== AddCookTime ===================================
void AddCookTime()
{
	float flAdditionalCookTime = 0.0;
	bool bVerifyFlag = false;
	CString strVar = L"", strEntry = L"", strAddedCookTime = L"";

	Display.KillEntry();

	while (!bKillTheTaskFlag && !bVerifyFlag)
	{
		Display.Display(20, L"Enter the");
		Display.Display(21, L"Additional Cook Time");
		Display.Display(22, L"In Minutes");
		Display.Entry(3, 0);

		while (!bKillTheTaskFlag && !Display.IsEnterKeyPressed())
		{
			flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
			flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
			// Added to set the time on the main display screen
			lCurStepSecSP = (long)flCTIME;

			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;

			Sleep(250);
		}
		flAdditionalCookTime = Display.GetEnteredData(3);
		strVar.Format(L"Cook time added by operator (min) %-5.1f", flAdditionalCookTime);

		Display.Display(21, strVar);
		Display.Display(22, L" Is This Entry Correct?");
		Display.Display(24, L"");
		Display.DisplayStatus(L"Enter Y or N");
		Display.Entry(3, 0);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		strEntry = Display.GetEnteredString(3);

		if (strEntry.CompareNoCase(L"Y") == 0)
		{
			// NOW - Add entered cook time to accumulated cycle additional cook time, update screen, log and exit
			bVerifyFlag = true;

			flCTIMEAddedAccumSec += flAdditionalCookTime * 60; // Global - throughout COOK cycle

			flCTIME += (flAdditionalCookTime * 60);	//	Dont forget your accumulation
			csCTIME.Format(L" %-5.1f", flCTIME);

			// Now - document the dirty work...
			strAddedCookTime.Format(L" %-5.1f", flAdditionalCookTime / 60);
			if (flAdditionalCookTime < 0) Display.Log(strVar, L"", DEVIATION);
			else Display.Log(strVar);
			DisplayCompleteProcessScreen();
		}
		else // The result is No (or just NOT Yes) - so do it again by setting bVerifyFlag = false
		{
			Display.Log(L"Re-Enter Add. Cook Time");
			bVerifyFlag = false;
		}
	}
	Display.KillEntry();
	bAddCookTimeFlag = false;
}	/* end of Add Cook Time */

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Global Init Function - I wouldn't touch this...
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void InitObjects()
{
	HRESULT hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hResult != S_OK)
		return;

	csVal = "PinesDisplay.PSLDisplay.1";
	pclsidDisplay = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidDisplay);
	hResult = CoCreateInstance(*pclsidDisplay, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownDisplay);
	if (hResult != S_OK)
		return;
	pIUnknownDisplay->QueryInterface(IID_IDispatch, (void**)&pDispatchDisplay);

	Display.m_bAutoRelease = FALSE;
	Display.AttachDispatch(pDispatchDisplay);

	//AfxMessageBox(L"Display Passed");
	csVal = "PinesIO.AnalogIO.1";
	pclsidAIO = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidAIO);
	hResult = CoCreateInstance(*pclsidAIO, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownIO);
	if (hResult != S_OK)
		return;
	pIUnknownIO->QueryInterface(IID_IDispatch, (void**)&pDispatchIO);

	AnalogIO.m_bAutoRelease = FALSE;
	AnalogIO.AttachDispatch(pDispatchIO);

	//AfxMessageBox(L"analogIO Passed");

	csVal = "PinesIO.DigitalIO.1";
	pclsidDIO = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidDIO);
	hResult = CoCreateInstance(*pclsidDIO, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownDIO);
	if (hResult != S_OK)
		return;
	pIUnknownDIO->QueryInterface(IID_IDispatch, (void**)&pDispatchDIO);

	DigitalIO.m_bAutoRelease = FALSE;
	DigitalIO.AttachDispatch(pDispatchDIO);

	//AfxMessageBox(L"DigitalIO Passed");

	csVal = "PinesIO.PIDCommands.1";
	pclsidPID = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidPID);
	hResult = CoCreateInstance(*pclsidPID, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownPID);
	if (hResult != S_OK)
		return;
	pIUnknownPID->QueryInterface(IID_IDispatch, (void**)&pDispatchPID);

	PID.m_bAutoRelease = FALSE;
	PID.AttachDispatch(pDispatchPID);

	//AfxMessageBox(L"PID Passed");

	csVal = "PinesIO.ProcessVariable.1";
	pclsidPV = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidPV);
	hResult = CoCreateInstance(*pclsidPV, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownPV);
	if (hResult != S_OK)
		return;
	pIUnknownPV->QueryInterface(IID_IDispatch, (void**)&pDispatchPV);

	PV.m_bAutoRelease = FALSE;
	PV.AttachDispatch(pDispatchPV);

	//Querying Three New Interfaces Here
	//NUMERICAL
	csVal = "PinesIO.NumericalProcess.1";
	pclsidNumerical = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidNumerical);
	hResult = CoCreateInstance(*pclsidNumerical, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownNumerical);
	if (hResult != S_OK)
		return;
	pIUnknownNumerical->QueryInterface(IID_IDispatch, (void**)&pDispatchNumerical);

	Numerical.m_bAutoRelease = FALSE;
	Numerical.AttachDispatch(pDispatchNumerical);

	//BALL
	csVal = "PinesIO.BallProcess.1";
	pclsidBall = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidBall);
	hResult = CoCreateInstance(*pclsidBall, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownBall);
	if (hResult != S_OK)
		return;
	pIUnknownBall->QueryInterface(IID_IDispatch, (void**)&pDispatchBall);

	Ball.m_bAutoRelease = FALSE;
	Ball.AttachDispatch(pDispatchBall);

	//TUNACAL
	csVal = "PinesIO.TunacalProcess.1";
	pclsidTunacal = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidTunacal);
	hResult = CoCreateInstance(*pclsidTunacal, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownTunacal);
	if (hResult != S_OK)
		return;
	pIUnknownTunacal->QueryInterface(IID_IDispatch, (void**)&pDispatchTunacal);

	Tuna.m_bAutoRelease = FALSE;
	Tuna.AttachDispatch(pDispatchTunacal);

	csVal = "PinesIO.TaskLoop.1";
	pclsidTaskloop = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidTaskloop);
	hResult = CoCreateInstance(*pclsidTaskloop, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownTaskloop);
	if (hResult != S_OK)
		return;
	pIUnknownTaskloop->QueryInterface(IID_IDispatch, (void**)&pDispatchTaskloop);

	TaskLoop.m_bAutoRelease = FALSE;
	TaskLoop.AttachDispatch(pDispatchTaskloop);

	csVal = "PinesIO.Profiler.1";
	pclsidProfiler = new CLSID;
	CLSIDFromProgID((LPCOLESTR)csVal.AllocSysString(), pclsidProfiler);
	hResult = CoCreateInstance(*pclsidProfiler, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pIUnknownProfiler);
	if (hResult != S_OK)
		return;
	pIUnknownProfiler->QueryInterface(IID_IDispatch, (void**)&pDispatchProfiler);

	Profiler.m_bAutoRelease = FALSE;
	Profiler.AttachDispatch(pDispatchProfiler);
}
//================== ConverTempToPress of Saturated Steam ===============
float ConvertTemptoPress(float flTemp)
{
	float flPress;

	if (flTemp < 212)
		flPress = 0;
	else
		flPress = (float)((5.643217 - (0.14430764 * flTemp) + (0.0015167123 * pow(flTemp, 2)) - (0.000007360282 * pow(flTemp, 3))
			+ (0.000000020605374 * pow(flTemp, 4))) - 14.697);
	return flPress;
}
//these are the actual functions NOTE: if you pass a float it will format it as 1 decimal if you pass a double it will format with 2 decimal places
CString ConvertToString(float TempConvertToStringVar)
{
	CString csConvertToStringRet = L"";
	csConvertToStringRet.Format(L"%5.1f", TempConvertToStringVar);
	return csConvertToStringRet;
}
CString ConvertToString(double TempConvertToStringVar)
{
	CString csConvertToStringRet = L"";
	csConvertToStringRet.Format(L"%5.2f", TempConvertToStringVar);
	return csConvertToStringRet;
}
CString ConvertToString(long TempConvertToStringVar)
{
	CString csConvertToStringRet = L"";
	csConvertToStringRet.Format(L"%d", TempConvertToStringVar);
	return csConvertToStringRet;
}
CString ConvertToString(int TempConvertToStringVar)
{
	CString csConvertToStringRet = L"";
	csConvertToStringRet.Format(L"%d", TempConvertToStringVar);
	return csConvertToStringRet;
}
//these are the actual functions NOTE: if you pass a float it will format it as 1 decimal if you pass a double it will format with 2 decimal places
CString toStr(float TemptoStrVar)
{
	CString csToStrRet = L"";
	csToStrRet.Format(L"%5.1f", TemptoStrVar);
	return csToStrRet;
}
CString toStr(double TemptoStrVar)
{
	CString csToStrRet = L"";
	csToStrRet.Format(L"%5.2f", TemptoStrVar);
	return csToStrRet;
}
CString toStr(long TemptoStrVar)
{
	CString csToStrRet = L"";
	csToStrRet.Format(L"%d", TemptoStrVar);
	return csToStrRet;
}
CString toStr(int TemptoStrVar)
{
	CString csToStrRet = L"";
	csToStrRet.Format(L"%d", TemptoStrVar);
	return csToStrRet;
}
CString toStr(CString strS)
{
	CString csToStrRet = L"";
	csToStrRet.Format(L"%s", strS);
	return csToStrRet;
}
CString SecondsToStr(double lSeconds)
{
	//return SecondsToStr((long)lSeconds);
	int iTimeMins = 0;
	int iTimeSeconds = 0;
	int iTimeHours = 0;
	CString csTempProcTimeSec = L"";
	CString csTempProcTimeMin = L"";
	CString csTempProcTimeHour = L"";
	CString csReturnTime = L"";
	CString csTempTimeVar = L"";

	iTimeMins = (int)(lSeconds / 60);

	iTimeSeconds = (int)(lSeconds - (iTimeMins * 60));

	float flSecsRound = (float)(lSeconds - (iTimeMins * 60));

	if ((flSecsRound - iTimeSeconds) >= .5)
		iTimeSeconds++;

	iTimeHours = int(iTimeMins / 60);
	iTimeMins = int(iTimeMins - (iTimeHours * 60));

	csTempProcTimeHour.Format(L"%d", iTimeHours);
	csTempProcTimeMin.Format(L"%d", iTimeMins);
	csTempProcTimeSec.Format(L"%d", iTimeSeconds);
	//this is to add the 0 place holder to the string if seconds are under 10 seconds
	if (iTimeSeconds < 10)
		csTempProcTimeSec = L"0" + csTempProcTimeSec;

	//this is to add the 0 place holder to the string if minutes are under 10 minutes
	if (iTimeMins < 10)
		csTempProcTimeMin = L"0" + csTempProcTimeMin;

	//this is to add the 0 place holder to the string if hours are under 10 hours
	if (iTimeHours < 10)
		csTempProcTimeHour = L"0" + csTempProcTimeHour;

	csReturnTime = csTempProcTimeHour + L":" + csTempProcTimeMin + L":" + csTempProcTimeSec;
	//csReturnTime=L"00"+L":"+csTempProcTimeMin+L":"+csTempProcTimeSec;

	return csReturnTime;

}
CString SecondsToStr(int lSeconds)
{
	return SecondsToStr((double)lSeconds);
}
CString SecondsToStr(long lSeconds)
{
	return SecondsToStr((double)lSeconds);
}
CString SecondsToStr(float lSeconds)
{
	return SecondsToStr((double)lSeconds);
}
CString TempFtoStr(float flTempFahrenheit)
{//this function takes a float Initial Temp and converts it to a string and appends degrees fahrenheit on the end
	CString csITF = L"";
	csITF.Format(L"%5.1f", flTempFahrenheit);
	csITF = csITF + L" °F";
	return csITF;
}
CString TempFtoStr(int flTempFahrenheit)
{
	return TempFtoStr((float)flTempFahrenheit);
}
CString TempFtoStr(long flTempFahrenheit)
{
	return TempFtoStr((float)flTempFahrenheit);
}
CString TempFtoStr(double flTempFahrenheit)
{
	return TempFtoStr((float)flTempFahrenheit);
}
/* ==================== Split Range control ======================== */
void SplitRangeControl()
{

	float	flCurrentPIDOutput = 0,
		flVentSlope = 0,
		flAirSlope = 0,
		flVentValveOutput = 0,
		flAirValveOutput = 0;

	bKillSplitRangeFlag = false;
	bSplitRangeCompleteFlag = false;
	bool	bShootOnce = false,
		bShootOnce2 = false;

	// *** IF NO RECIPE SELECTED => use default constant values until a RECIPE is selected ***
	flVentSlope = -1 * (100 / (50 - (3 / 2)));
	flAirSlope = 100 / (100 - (50 + (3 / 2)));

	//Display.Log(L"Split Range called");

	while (!bRecipeSelectionFlag && !bKillSplitRangeFlag && !bShutDownFlag)
	{
		if (!bShootOnce)
		{
			//Display.Log(L"Split Range no recipe");
			bShootOnce = true;
		}
		flAirValveOutput = flAirSlope * (AnalogIO.ReadAnalogOut(AO_SPLT_RNG) - (50 - (3 / 2)));
		flVentValveOutput = flVentSlope * AnalogIO.ReadAnalogOut(AO_SPLT_RNG) + 100;

		AnalogIO.SetAnalogOut(AO_AIR, flAirValveOutput);
		AnalogIO.SetAnalogOut(AO_VENT, flVentValveOutput);

		Sleep(250);
	}
	// *** IF RECIPE IS SELECTED => Then OK to use Process Variable values ***
	flVentSlope = -1 * (flMaxVentValveOutput / (flZeroValueCrossOver - (flDeadBandSetting / 2)));
	flAirSlope = flMaxAirValveOutput / (100 - (flZeroValueCrossOver + (flDeadBandSetting / 2)));
	while (!bShutDownFlag && !bKillSplitRangeFlag)
	{
		if (!bShootOnce2)
		{
			//Display.Log(L"Split Range with recipe");
			bShootOnce2 = true;
		}
		flAirValveOutput = flAirSlope * (AnalogIO.ReadAnalogOut(AO_SPLT_RNG) - (flZeroValueCrossOver - (flDeadBandSetting / 2)));
		if (flAirValveOutput < flMinimumOutputBias)
			flAirValveOutput = flMinimumOutputBias;

		flVentValveOutput = flVentSlope * AnalogIO.ReadAnalogOut(AO_SPLT_RNG) + flMaxVentValveOutput;
		if (flVentValveOutput < flMinimumOutputBias)
			flVentValveOutput = flMinimumOutputBias;

		AnalogIO.SetAnalogOut(AO_AIR, flAirValveOutput);
		AnalogIO.SetAnalogOut(AO_VENT, flVentValveOutput);

		Sleep(250);
	}

	PID.PIDStop(AO_SPLT_RNG);
	AnalogIO.SetAnalogOut(AO_VENT, 0.0);
	AnalogIO.SetAnalogOut(AO_AIR, 0.0);

	TaskLoop.KillTask(8);
	bSplitRangeCompleteFlag = true;
	Sleep(1000);
	bKillSplitRangeFlag = true;
} //end of function SplitRangeControl

void SensorChecks()
{
	//Don't allow the machine to be loaded if essential sensor checks have failed
	/* Local variable declarations */
	float	flCurrentLevel = 0.0,
		flTemporaryVariable = 0.0,
		flMaxRTDSignal = 279.0,
		flMinRTDSignal = 35.0,
		flMaxPressure = 74.0, //mdm change to 74 to run high pressure processes (Requested by Food Lab 06/27/18) 
		flMinPressure = -0.5,
		flMinFlowRate = -2.0,
		flMaxFlowRate = 1600.0,
		flMaxWaterLevel = 99.0,
		flMinWaterLevel = -0.5;

	int		intCurrentStatus = 0;

	bool	bSensorProblem = false;

	CString strVar1 = L"", strVar2 = L"", strUnits = L"";

	// *** BEGINNING OF COMMON INITIALIZATION STUFF (Keep this Section SIMILAR in all routines!)
	bSensorCheckCompleteFlag = false;	// Unique: Set false here & then true at end of function
	bPhaseCompleteFlag = false;	// Set false here & then true at end of function
	bKillTheTaskFlag = false;	// Set false here - Set TRUE by the OVERRIDE & MAJOR PROBLEM CHECKS

	Display.Phase(L"SensChck");
	sPhaseMessage = L"SensChck"; 

	Display.Log(L"Start Sensor Check");

	DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Off);
	DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
	DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
	DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
	bPumpIsRunning = false;
	DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
	DigitalIO.SetDigitalOut(DO_COND_VLV, Off);


	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_WATER, true);

	//Set All AO to what is needed in this phase
	AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
	AnalogIO.SetAnalogOut(AO_WATER, 0.0);
	AnalogIO.SetAnalogOut(AO_SPLT_RNG, 51.0);
	// *** END OF COMMON INITIALIZATION STUFF

	CString csOption = L"";

	do		// ((bSensorProblem == true) && (!bKillTheTaskFlag))
	{
		Display.KillEntry(); 

		Display.Display(20, L"Please Wait");
		Display.Display(21, L"Checking");
		Display.Display(22, L"Retort");
		Display.Display(23, L"Sensors");
		Display.DisplayStatus(L"Press Enter to Proceed...");
		Display.Entry(0, 2);

		Func_WaitForEnterKey(bKillTheTaskFlag, false);

		Display.KillEntry();

		bSensorProblem = false;

		// ============= Check if RTD 1 is out of range========
		if ((((flCurrentTemperature > flMaxRTDSignal) ||
			(flCurrentTemperature < flMinRTDSignal))) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 

			bSensorProblem = true;
			strVar1 = L"RTD 1 Probe Failure ";
			strUnits = L" F";
			strVar2.Format(L" %-6.1f", flCurrentTemperature);
			strVar1 = strVar1 + strVar2 + strUnits;
			Display.Alarm(2, 1, LOGTECAlarm, strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end RTD 1 check

		// ============= Check if RTD 2 is out of range========

		if((((flCurrentTemp2 > flMaxRTDSignal) ||
			(flCurrentTemp2 < flMinRTDSignal))) && !bKillTheTaskFlag)
		{
			bSensorProblem = true;
			strVar1 = L"RTD 2 Probe Failure ";
			strUnits = L" F";
			strVar2.Format(L" %-6.1f", flCurrentTemp2);
			strVar1 = strVar1+strVar2+strUnits;
			Display.Alarm(2, 1, LOGTECAlarm , strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end RTD 2 check 

		// ============= Check if Preheat RTD is out of range========
		if ((((flCurrentPreHeatTemp > flMaxRTDSignal) ||
			(flCurrentPreHeatTemp < flMinRTDSignal))) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 

			bSensorProblem = true;
			strVar1 = L"Preheat RTD Probe Failure ";
			strUnits = L" F";
			strVar2.Format(L" %-6.1f", flCurrentPreHeatTemp);
			strVar1 = strVar1 + strVar2 + strUnits;
			Display.Alarm(2, 1, LOGTECAlarm, strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end PreHeat RTD check

	/*	// ============= Check if INCOMING WATER TEMP RTD is out of range========
		if((((flCoolingSourceTemp > flMaxRTDSignal) ||
			(flCoolingSourceTemp < flMinRTDSignal))) && !bKillTheTaskFlag)
		{
			bSensorProblem = true;
			strVar1 = L"Source Water RTD Probe Failure ";
			strUnits = L" F";
			strVar2.Format(L" %-6.1f", flCoolingSourceTemp);
			strVar1 = strVar1+strVar2+strUnits;
			Display.Alarm(2, 1, LOGTECAlarm , strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end INCOMING WATER TEMP RTD check	*/

		// ============== Check if PSI is out of range =========
		if ((((flCurrentPressure > flMaxPressure) ||
			(flCurrentPressure < flMinPressure))) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 

			bSensorProblem = true;
			strVar1 = L"PSI Probe Failure ";
			strUnits = L" PSI";
			strVar2.Format(L" %-6.1f", flCurrentPressure);
			strVar1 = strVar1 + strVar2 + strUnits;
			Display.Alarm(2, 1, LOGTECAlarm, strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end Pressure Sensor check

			/*=================== Check Level Probe ==========================*/
		flCurrentLevel = AnalogIO.ReadAnalogIn(AI_LEVEL);
		if (((flCurrentLevel > flMaxWaterLevel) || (flCurrentLevel < flMinWaterLevel)) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 

			bSensorProblem = true;
			strVar1 = L"Level Probe Failure ";
			strUnits = L" %";
			strVar2.Format(L" %-6.1f", flCurrentLevel);
			strVar1 = strVar1 + strVar2 + strUnits;
			Display.Alarm(2, 1, LOGTECAlarm, strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();
		}
		// end Level Probe check

			/*=================== Check Flow Meter ==========================*/
		if (((flCurrentWaterFlow > flMaxFlowRate) || (flCurrentWaterFlow < flMinFlowRate)) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 
			bSensorProblem = true;
			strVar1 = L"Flow meter Failure ";
			strUnits = L" gpm";
			strVar2.Format(L" %-6.1f", flCurrentWaterFlow);
			strVar1 = strVar1 + strVar2 + strUnits;
			Display.Alarm(2, 1, LOGTECAlarm, strVar1);
			Display.Display(21, strVar1);
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();

		}
		// end Flow Meter check

		
			// ============= Check Pump ONN ==============================
		DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
		bPumpIsRunning = false;


		if (DigitalIO.ReadDigitalOut(DO_PUMP_1) && !bKillTheTaskFlag)
		{
			Display.KillEntry(); 
			bSensorProblem = true;
			Display.Alarm(2, 1, LOGTECAlarm, L"Circulation Pump Running", SendLogStatus);
			Display.Display(21, L"Circulation Pump is ON");
			Display.Display(22, L" Please Call Maintenance");
			Display.DisplayStatus(L"Press Enter to Resume Sensor Checks...");
			Display.Entry(0, 0);

			Func_WaitForEnterKey(bKillTheTaskFlag, false);

			Display.KillEntry();

		}	// end Pump Overload

		if (bKillTheTaskFlag)
		{
			Display.KillEntry();
			bSensorProblem = false;
			Display.Log(L"Sensor checks were bypassed by override", L"", OVERRIDE);
		}
	} while (bSensorProblem && !bKillTheTaskFlag); // If any sensor test fails, go back up to sweeping all sensors

	// *** Clean up here - leave things in a safe state ***
	// Kill Only PID Loops That Should be killed.
	PID.PIDStop(AO_STEAM, true);
	PID.PIDStop(AO_AIR, false);
	PID.PIDStop(AO_WATER, true);
	// In case of hot restart the split range must be active to maintain integrity of containers.

		// *** COMMON ENDING TO ALL PHASE FUNCTIONS - Except Specific b(phase)CompleteFlag ***

	Display.Log(L"Sensor Check Complete", L"", SendLogStatus);
	Display.KillEntry();
	TaskLoop.KillTask(3);
	bSensorCheckCompleteFlag = true;
	Sleep(200);
	bPhaseCompleteFlag = true;
}	// end of sensors checks funct

void MiscTasks()
{
	//Once these get started, the loop should always be running.
	//This Task will handle:
	//	the resetting of the safety relay
	//	updating the common critical control point variables

	bMiscTasksRunning = true;

	while (true)
	{

		flCurrentTemperature = AnalogIO.ReadAnalogIn(AI_RTD1);
		flCurrentTemp2 = AnalogIO.ReadAnalogIn(AI_RTD2);
		flCurrentPressure = AnalogIO.ReadAnalogIn(AI_PRESSURE);
		flCurrentWaterLevel = AnalogIO.ReadAnalogIn(AI_LEVEL);
		flCurrentWaterFlow = AnalogIO.ReadAnalogIn(AI_FLOW);
		flCurrentPreHeatTemp = AnalogIO.ReadAnalogIn(AI_PREHT_RTD);
		//	flCoolingSourceTemp		= AnalogIO.ReadAnalogIn(AI_INCOMING_WTR_TEMP);

	////////// SILENCE ALARMS AND RESET SAFETY RELAY
		if (DigitalIO.ReadDigitalIn(ALARM_ACK_PB))
		{
			Display.ClearAlarm(-1);		//Clears the alarm banner on the LTM
			DigitalIO.SetDigitalOut(DO_SFTY_RLY_RST, Onn);
			Sleep(1000);
			DigitalIO.SetDigitalOut(DO_SFTY_RLY_RST, Off);
		}

		////////// UPDATE AGITATION REQUEST
		if (!bBeginAgitationFlag)
			DigitalIO.SetDigitalOut(VDO_AgitationReq, Off);


		if (!DigitalIO.ReadDigitalOut(VDO_AgitationReq))
		{
			bAgitatingProcessFlag = false;
			TaskLoop.KillTask(14); //RH v.iOPS
		}
		if (bTriggerEAChange)
		{
			bTriggerEAChange = false;
			DigitalIO.SetDigitalOut(VDO_StopEA_Req, Onn);	// Request the motion to stop to home the position of the EA
			Sleep(500);
			DigitalIO.SetDigitalOut(VDO_AgitationReq, Off);	// Clear the request for motion

			if (flNewMP > 0.0)
			{
				flMotionProfile = flNewMP;
				AnalogIO.SetAnalogOut(VAO_EA_MotionProfile, flMotionProfile);	// Set the new motion profile
				Sleep(100);
				DigitalIO.SetDigitalOut(VDO_AgitationReq, Onn);	// Set the motion request high
				Sleep(100);
				Display.Log(L"Efficient Agitation = Motion Profile" + toStr(flMotionProfile));

			}
		}
		if (DigitalIO.ReadDigitalIn(VDI_StopEA_Ack))
		{
			DigitalIO.SetDigitalOut(VDO_StopEA_Req, Off);	// Clear the request to stop
		}

		if (bCookRunning)
		{
			flDisplayLoopTempSP = (float)PID.GetPIDSetPoint(AO_STEAM);
			flDisplayLoopPsiSP = (float)PID.GetPIDSetPoint(AO_SPLT_RNG);
			// Added to set the time on the main display screen
			lCurStepSecSP = (long)flCTIME;

			if ((lCurStepSecSP - PinesTime1.GetElapsedTime()) > 0)
			{
				lCurStepSecRemain = lCurStepSecSP - PinesTime1.GetElapsedTime();
			}
			else
				lCurStepSecRemain = 0;
		}

		Sleep(200);
		
	}

	bMiscTasksRunning = false;

	TaskLoop.KillTask(11);

}	//end of MiscTasks()


/*=====================================iOPS Update Tags Tasks============================================*/
void iOPSTimeTracking()
{
	float		flIdleMinuteTracker = 0,
		flProcessMinuteTracker = 0,
		flMotorMinuteTracker = 0,
		flPumpMinuteTracker = 0;

	bInIdleStart = false;
	bInIdleStop = false;
	bInProcessStart = false;
	bInProcessStop = false;
	bPumpRunStart = false;
	bPumpRunStop = false;
	bMotorRunStart = false;
	bMotorRunStop = false;

	PinesTime35.StopTimer(); // PinesTime35 Equipment Operation Timer
	PinesTime35.ZeroTime();
	PinesTime36.StopTimer(); // PinesTime36 Equipment Idle Timer
	PinesTime36.ZeroTime();
	PinesTime37.StopTimer(); // PinesTime37 Water Pump Running Timer
	PinesTime37.ZeroTime();
	PinesTime38.StopTimer(); // PinesTime38 Motor Rotation Hours
	PinesTime38.ZeroTime();

	while (true)
	{
		if (bIsInIdle) //Idle before fill phase and after drain phase
		{
			if (!bInIdleStart)
			{
				PinesTime36.StartTimer();
				bInIdleStart = true;
				bInIdleStop = false;
			}
		}
		else
		{
			if (!bInIdleStop)
			{
				PinesTime36.StopTimer();
				bInIdleStart = false;
				bInIdleStop = true;
			}
		}
		//in comeup true until cooling false
		if (bIsInProcess)
		{
			if (!bInProcessStart)
			{
				PinesTime35.StartTimer();
				bInProcessStart = true;
				bInProcessStop = false;
			}
		}
		else
		{
			if (!bInProcessStop)
			{
				PinesTime35.StopTimer();
				bInProcessStart = false;
				bInProcessStop = true;
			}
		}

		if (bPumpIsRunning)
		{
			if (!bPumpRunStart)
			{
				PinesTime37.StartTimer();
				bPumpRunStart = true;
				bPumpRunStop = false;
			}
		}
		else
		{
			if (!bPumpRunStop)
			{
				PinesTime37.StopTimer();
				bPumpRunStart = false;
				bPumpRunStop = true;
			}
		}

		flIdleMinuteTracker = (float)(PinesTime36.GetElapsedTime() / 60);
		flIdleHourTracker = flIdleMinuteTracker / 60;
		flProcessMinuteTracker = (float)(PinesTime35.GetElapsedTime() / 60);
		flProcessHourTracker = flProcessMinuteTracker / 60;
		flPumpMinuteTracker = (float)(PinesTime37.GetElapsedTime() / 60);
		flPumpHourTracker = flPumpMinuteTracker / 60;
		flMotorMinuteTracker = (float)(PinesTime38.GetElapsedTime() / 60);
		flMotorHourTracker = flMotorMinuteTracker / 60;
	}
}



void iOPSTagUpdate()
{
	int		iTCIOIssueCount = 0;

	bool	bTCIOVariableIssue = false;

	CString	strTCIOErrorMsg = L"";

	while (true)
	{
		if (bAlarmsUpdate_iOPS)
		{
			sAlarmMessage_1 = sAlarmMessage_2,
				sAlarmMessage_2 = sAlarmMessage_3,
				sAlarmMessage_3 = sAlarmMessage_4,
				sAlarmMessage_4 = sAlarmMessage_5,
				sAlarmMessage_5 = sAlarmMessage_Hold;
			bAlarmsUpdate_iOPS = false;
		}
		//   ==================================================================================================================
		//   ==============================================  REALS - LTM -> iOPS  ==============================================
		//   ==================================================================================================================
		//	 ==============================================  Analog In  =======================================================

		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAI_RTD1", AnalogIO.ReadAnalogIn(AI_RTD1))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Retort Temp 1 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAI_PREHT_RTD", AnalogIO.ReadAnalogIn(AI_PREHT_RTD))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Retort PreHeat Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAI_PRESSURE", AnalogIO.ReadAnalogIn(AI_PRESSURE))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Pressure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAI_LEVEL", AnalogIO.ReadAnalogIn(AI_LEVEL))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Level to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAI_FLOW", AnalogIO.ReadAnalogIn(AI_FLOW))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Flow to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		
		//	 ==============================================  Analog Out  =======================================================

		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAO_STEAM", AnalogIO.ReadAnalogOut(AO_STEAM))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Steam Valve Percent to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAO_VENT", AnalogIO.ReadAnalogOut(AO_VENT))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Vent Valve Percent to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAO_AIR", AnalogIO.ReadAnalogOut(AO_AIR))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Air Valve Percent to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAO_WATER", AnalogIO.ReadAnalogOut(AO_WATER))))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Water Valve Percent to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		//		==============================================================================================================
		//		==============================================  Integer Values  =================================================
		//		==============================================================================================================

		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.lCurStepSecRemain", (float)lCurStepSecRemain)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Current Remaining Step Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flDisplayLoopTempSP", flDisplayLoopTempSP)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Temperature Setpoint to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flDisplayLoopPsiSP", flDisplayLoopPsiSP)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Setpoint to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		if (!(tcio.WriteDINT(L"iOPS.LTM_2_iOPS.iProductNumber", iProductNumber))) 
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Product Number / Recipe to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		//		==============================================================================================================
		//		==============================================  Timer Values  ================================================
		//		==============================================================================================================
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flIdleHourTracker", flIdleHourTracker)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Idle Hours to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flProcessHourTracker", flProcessHourTracker)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Process Hours to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPumpHourTracker", flPumpHourTracker)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pump Hours to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flMotorHourTracker", flMotorHourTracker)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Motor Hours to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		//   ==================================================================================================================
		//   =============================================  BOOLEANS - LTM -> BTS  ============================================
		//   ==================================================================================================================
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bTempDeviationFlag", bTempDeviationFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Temperature Deviation Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bRPMFailFlag", bRPMFailFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing RPM Failure Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bCriticalPSIFailureFlag", bCriticalPSIFailureFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Critical Pressure Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bProcessDeviationFlag", bProcessDeviationFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Process Deviation Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongFillAlarm", bLongFillAlarm)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Long Fill Alarm Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongPressureTimeFlag", bLongPressureTimeFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Long Pressurization Time Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongComeUpTimeFlag", bLongComeUpTimeFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Long Come Up Time Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bTemperatureDropInOvershoot", bTemperatureDropInOvershoot)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Temperature Drop in Overshoot Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongCoolingTimeFlag", bLongCoolingTimeFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Long Cooling Time Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bDifferentialPSIFromCookToCool", bDifferentialPSIFromCookToCool)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Differential PSI from Cook Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongDrainTimeFlag", bLongDrainTimeFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Long Drain Time Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLongVentZeroTimeFlag", bLongVentZeroTimeFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Long Vent Zero Time Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bLowMIGEntryFlag", bLowMIGEntryFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Low MIG Entry to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bE_StopPushed", bE_StopPushed)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressed E_Stop to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bRTD1FailedFlag", bRTD1FailedFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing RTD 1 Failure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bRTD1HighTempFailFlag", bRTD1HighTempFailFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing RDT 1 Hight Temperature Failure Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bAI_PREHT_RTDFailFlag", bAI_PREHT_RTDFailFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pre Heat RTD Failure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bPSIAboveSetPointFlag", bPSIAboveSetPointFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing PSI Above Setpoint Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bRetort_PresFailFlag", bRetort_PresFailFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Pressure Failure Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bCook_PressWarnFlag", bCook_PressWarnFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cook Pressure Warning Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"iOPS.LTM_2_iOPS.bCook_PresFailFlag", bCook_PresFailFlag)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cook Pressure Failure Flag to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bEndEAAgitation", bEndEAAgitation)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing End EA Agitation Flag!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}		
		if (!(tcio.WriteBOOL(L"LTM.bComeUpTempStep1", bComeUpTempStep1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temp Step 1!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bComeUpTempStep2", bComeUpTempStep2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temp Step 2!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bComeUpTempStep3", bComeUpTempStep3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temp Step 3!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bComeUpTempStep4", bComeUpTempStep4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temp Step 4!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bComeUpTempStep5", bComeUpTempStep5)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temp Step 5!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepA", bHotPSIStepA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step A!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepB", bHotPSIStepB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step B!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepC", bHotPSIStepC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step C!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepD", bHotPSIStepD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step D!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepE", bHotPSIStepE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step E!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bHotPSIStepF", bHotPSIStepF)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Pressure Step F!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLTempStep1", bCOOLTempStep1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 1!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLTempStep2", bCOOLTempStep2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 2!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLTempStep3", bCOOLTempStep3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 3!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLTempStep4", bCOOLTempStep4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 4!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLPSIStepA", bCOOLPSIStepA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Pressure Step A!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLPSIStepB", bCOOLPSIStepB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Pressure Step B!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLPSIStepC", bCOOLPSIStepC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Pressure Step C!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLPSIStepD", bCOOLPSIStepD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Pressure Step D!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteBOOL(L"LTM.bCOOLPSIStepE", bCOOLPSIStepE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Pressure Step E!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		//   ==================================================================================================================
		//   =============================================  STRINGS - LTM -> iOPS  ============================================
		//   ==================================================================================================================

		if (!(tcio.WriteSTRING(L"iOPS.LTM_2_iOPS.CSRetortAlarmMessage", sRetortAlarmMessage)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Alarm Message to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteSTRING(L"iOPS.LTM_2_iOPS.sPhaseMessage", sPhaseMessage)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Phase Status to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteSTRING(L"iOPS.LTM_2_iOPS.sAgitationStyle", sAgitationStyle)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Agitation Style Message to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteSTRING(L"iOPS.LTM_2_iOPS.sProcessRecipeName", sProcessRecipeName)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Retort Process Recipe Name to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		/*if (!(tcio.WriteSTRING(L"iOPS.LTM_2_iOPS.csBatchCode", csBatchCode))) // v.16 RMH
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing the Batch Code to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}*/

		//   ==================================================================================================================
		//   =============================================  PVs - LTM -> iOPS  ============================================
		//   ==================================================================================================================

		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flInitialFillLevel", flInitialFillLevel)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Fill Level Setpoint to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flInitialTempOffset", flInitialTempOffset)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Initial Temperature Offset to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempStep1", flComeUpTempStep1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temperature Step 1 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempTime1", flComeUpTempTime1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Time Step 1 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempStep2", flComeUpTempStep2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temperature Step 2 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempTime2", flComeUpTempTime2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Time Step 2 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempStep3", flComeUpTempStep3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temperature Step 3 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempTime3", flComeUpTempTime3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Time Step 3 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempStep4", flComeUpTempStep4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temperature Step 4 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempTime4", flComeUpTempTime4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Time Step 4 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempStep5", flComeUpTempStep5)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Temperature Step 5 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpTempTime5", flComeUpTempTime5)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Time Step 5 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flMinimumCookPSI", flMinimumCookPSI)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Minimum Cook PSI to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flOverShootTime", flOverShootTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Over Shoot Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flRampDownToCTEMPTime", flRampDownToCTEMPTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Ramp To Cook Temp Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCookStabilizationTime", flCookStabilizationTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cook Stabilization Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flInitialPressure", flInitialPressure)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing initial Pressure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepA", flHotPSIStepA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step A to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeA", flHotPSITimeA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time A to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepB", flHotPSIStepB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step B to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeB", flHotPSITimeB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time B to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepC", flHotPSIStepC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step C to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeC", flHotPSITimeC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time C to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepD", flHotPSIStepD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step D to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeD", flHotPSITimeD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time D to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepE", flHotPSIStepE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step E to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeE", flHotPSITimeE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time E to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSIStepF", flHotPSIStepF)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Step F to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flHotPSITimeF", flHotPSITimeF)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Hot Pressure Time F to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flMicroCoolTime", flMicroCoolTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Micro Cool Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempStep1", flCOOLTempStep1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 1 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempTime1", flCOOLTempTime1)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Time 1 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempStep2", flCOOLTempStep2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 2 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempTime2", flCOOLTempTime2)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Time 2 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempStep3", flCOOLTempStep3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 3 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempTime3", flCOOLTempTime3)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Time 3 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempStep4", flCOOLTempStep4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Step 4 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempTime4", flCOOLTempTime4)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Time 4 to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLTempHoldTime", flCOOLTempHoldTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Temp Hold Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSIStepA", flCOOLPSIStepA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Step A to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSITimeA", flCOOLPSITimeA)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Time A to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSIStepB", flCOOLPSIStepB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Step B to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSITimeB", flCOOLPSITimeB)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Time B to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSIStepC", flCOOLPSIStepC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Step C to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSITimeC", flCOOLPSITimeC)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Time C to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSIStepD", flCOOLPSIStepD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Step D to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSITimeD", flCOOLPSITimeD)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Time D to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSIStepE", flCOOLPSIStepE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Step E to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCOOLPSITimeE", flCOOLPSITimeE)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Pressure Time E to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flComeUpMotionProfile", flComeUpMotionProfile)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Come Up Motion Profile to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCookMotionProfile", flCookMotionProfile)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cook Motion Profile to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCoolingMotionProfile", flCoolingMotionProfile)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cooling Motion Profile to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flMinVentTime", flMinVentTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Minimum Vent Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flMinVentTemp", flMinVentTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Minimum Vent Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flOvershootOffset", flOvershootOffset)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Overshoot Offset to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flRampToVentTime", flRampToVentTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Ramp to Vent Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flRampTime", flRampTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Ramp Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flWaterAtDamLevel", flWaterAtDamLevel)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Water at Dam Level to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPreHeatTemp", flPreHeatTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing PreHeat Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flFeedLegOutletTemp", flFeedLegOutletTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Feed Leg Outlet Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flFeedLegTime", flFeedLegTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Feed Leg Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flRampToProcessTime", flRampToProcessTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Ramp to Process Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flSteamDomePasses", flSteamDomePasses)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Steam Dome Passes to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPSICoolInletLegTime", flPSICoolInletLegTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Inlet Leg Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPSICoolInletTemp", flPSICoolInletTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Inlet Temperature to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPressCoolPressure", flPressCoolPressure)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Pressure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPressCoolTime", flPressCoolTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPressCoolPasses", flPressCoolPasses)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Passes to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flPressCoolOutletTemp", flPressCoolOutletTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Outlet Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAtmosCoolPressure", flAtmosCoolPressure)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Pressure Cool Pressure to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAtmosCoolTime", flAtmosCoolTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Atmos Cool Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAtmosCoolPasses", flAtmosCoolPasses)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Atmos Cool Passes to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAtmosCoolOutletTemp", flAtmosCoolOutletTemp)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Atmos Cool Outlet Temp to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flCoolHoldTime", flCoolHoldTime)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Cool Hold Time to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flSprayCoolProcess", flSprayCoolProcess)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Spray Cool Process to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		/*if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flAtmosCoolMotionProfile", flAtmosCoolMotionProfile)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Atmos Cool Motion Profile to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}*/
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flDirectCoolLevel", flDirectCoolLevel)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Direct Cool Level to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flWaterHighLevelAlarm", flWaterHighLevelAlarm)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Water High Level Alarm to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flWaterLowLevelAlarm", flWaterLowLevelAlarm)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Water Low Level Alarm to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flWaterDrainOnLevel", flWaterDrainOnLevel)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Water Drain On Level to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}
		if (!(tcio.WriteREAL(L"iOPS.LTM_2_iOPS.flWaterDrainOffLevel", flWaterDrainOffLevel)))
		{
			iTCIOIssueCount += 1;
			strTCIOErrorMsg = L"Problem Writing Water Drain Off Level to iOPS!!";
			if (!bTCIOVariableIssue && bEnableCommErrorLogs)
				Display.Log(strTCIOErrorMsg, L"", NORMAL);
			bTCIOVariableIssue = true;
		}

		//		================================================================================================

		if (bEnableCommErrorLogs && bTCIOVariableIssue && iTCIOIssueCount < 1)
		{
			Display.Log(L"Communication with BTS O.K.", L"", NORMAL);
			bTCIOVariableIssue = false;
		}
		Sleep(250);
	}
}

/* ==================== Common Initialization Stuff ======================== */
void CommonInit()
{
	if (bOpEntryRunning && !bHOT_RestartFlag)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
		bPumpIsRunning = false;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
		PID.PIDStop(AO_VENT, true);
		PID.PIDStop(AO_AIR, true);
		PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}
	
	if (bPreheatRunning)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Onn);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
		bPumpIsRunning = false;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
		PID.PIDStop(AO_VENT, true);
		PID.PIDStop(AO_AIR, true);
		PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}

	if (bCUTempRunningFlag)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Onn);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Onn);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Onn);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Onn);
		bPumpIsRunning = true;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
		PID.PIDStop(AO_VENT, true);
		PID.PIDStop(AO_AIR, true);
		//PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		//AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}

	if (bCookRunning)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Onn);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Onn);
		bPumpIsRunning = true;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, false);
		PID.PIDStop(AO_WATER, true);
		PID.PIDStop(AO_VENT, false);
		PID.PIDStop(AO_AIR, false);
		PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}

	if (bCoolTempRunning)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);// these will adjust after microcool
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Onn);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Onn);
		bPumpIsRunning = true;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, false);
		PID.PIDStop(AO_VENT, false);
		PID.PIDStop(AO_AIR, false);
		PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}

	if (bDrainRunning)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Onn);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
		bPumpIsRunning = false;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
		//PID.PIDStop(AO_VENT, true);
		//PID.PIDStop(AO_AIR, true);
		//PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		//AnalogIO.SetAnalogOut(AO_VENT, 0.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}

	if (bUnloadRunning)
	{
		// Set the vavles in the initial state for this phase
		DigitalIO.SetDigitalOut(DO_DRAIN_VLV, Off);
		DigitalIO.SetDigitalOut(DO_COND_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BLK_VLV, Off);
		DigitalIO.SetDigitalOut(DO_STEAM_BYPASS_VLV, Off);
		DigitalIO.SetDigitalOut(DO_PREHEAT_VLV, Off);
		DigitalIO.SetDigitalOut(DO_WATER_FILL_VLV, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_OPEN, Off);
		DigitalIO.SetDigitalOut(DO_HE_BYPASS_CLOSE, Off);

		DigitalIO.SetDigitalOut(DO_PUMP_1, Off);
		DigitalIO.SetDigitalOut(DO_PUMP_2, Off);
		bPumpIsRunning = false;

		// Now Set All AO to what is needed in this phase
		PID.PIDStop(AO_STEAM, true);
		PID.PIDStop(AO_WATER, true);
		PID.PIDStop(AO_VENT, false);
		PID.PIDStop(AO_AIR, true);
		PID.PIDStop(AO_SPLT_RNG, false);
		//AnalogIO.SetAnalogOut(AO_STEAM, 0.0);
		//AnalogIO.SetAnalogOut(AO_WATER, 0.0);
		AnalogIO.SetAnalogOut(AO_VENT, 100.0);
		//AnalogIO.SetAnalogOut(AO_AIR, 0.0);
		//AnalogIO.SetAnalogOut(AO_SPLT_RNG, 50);//Close air and vent control valves
	}
}

/*===================== Function WaitForEnterKey =====================*/

void Func_WaitForEnterKey(const bool& bOverrideOut1 = false, const bool& bOverrideOut2 = false)
{
	//	Pass this function a boolean (or two if necessary)
	//	variable from where it was called to allow it to be 
	//	overriden from
	//
	PinesTime2.StartTimer();
	PinesTime2.ZeroTime();

	while (!Display.IsEnterKeyPressed() && !bOverrideOut1 && !bOverrideOut2)
	{
		if (PinesTime2.GetElapsedTime() > iMaxOperatorEntryTime)
		{
			sRetortAlarmMessage = L"Operator Entry Delay";
			Display.Alarm(2, 1, LOGTECAlarm, sRetortAlarmMessage);
			PinesTime2.ZeroTime();
			bAlarmsUpdate_iOPS = true;
		}

		Sleep(300);
	}


	PinesTime2.StopTimer();
	PinesTime2.ZeroTime();

	Display.ResetSnooze();
	Display.CancelAlarm();
} // End of Func_WaitForEnterKey(const bool& bOverrideOut1, const bool& bOverrideOut2)