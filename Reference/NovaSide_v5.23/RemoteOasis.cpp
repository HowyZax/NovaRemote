// RemoteOASIS.cpp
#define ALLOW_RX4_TOUCH

#include "stdio.h"
#include "math.h"
#include "string.h"     // for memcpy 
#include "stdlib.h"     //  for abs

#include "deva.h"
#include "devatypes.h"				   
#include "globals.h"
#include "TypeDefs.h"

#include "ATA-DISK.h"

#include "AudioMix.h"
#include "Effects.h"
#include "nvdata.h"

#include "remote.h"
#include "transport.h" 
#include "RS232.h"     
#include "debug.h"   
#include "PS2KeyBoard.h" 
#include "hardware.h" 
#include "hardware_LED.h" 

#include "Mix12.h"
#include "AudioPeak.h"

#include <time.h>
#include "RealTimeClock.h"
//#define PEAKS_ANALOG_INPUTS 0 // kludge...we will have to add special peak detects for the Mix12
//#define PEAKS_DISK_OUTPUTS  0

#include "ZaxNet.h"
#include "NomadUIsettings.h"
#include "N_SCIport.h" // for ForceSpecialTrimChannel()
#include "NOPT.h"
#include "MiscTools.h"
#include "DiskManager.h"
#include "DiskAPI.h"
#include "Mirror.h"

#include "ZaxLan.h"
#include "OasisKeys.h"

#include "Timecode.h"
#include "SharedRam.h" // for struct USB_KEYBOARD gKBD;
 
#include "..\UI\SOURCE\Include\std.h"  // for MAX
#include "..\UI\SOURCE\Include\types.h"
#include "..\UI\SOURCE\auto\message.h"
#include "..\UI\SOURCE\auto\uicore.h"										
#include "..\UI\SOURCE\auto\uiTables.hpp_" // for detecting which screen we are in
#include "..\UI\SOURCE\auto\nomad.h"
void GotoPage_ASYNC(UI_PAGE *p); // in tasks.cpp
void UIControlSetAbsolute_ASYNC(int DesiredIndex, bool ToggleTheCell); // in tasks.cpp
void ActivateSlate(int stateOrToggle);
void ActivateCom1(int stateOrToggle);

#include <std.h>
#include <mbx.h>
#include <tsk.h>
#include "MailBoxes.h" // needed for transport mbx

#define LONG_KEY_MS 500

//#include "ZlanCmdStringLUT.h"
#include "RemoteOasis.h" // for sending STN to OASIS
#include "protocol.h"    // for oasis serial settings codes
int inputChBoo=24-8;

/*
void putcom(char p);	
int  getcom();
int  peekcom(int idx);
int  get_com_count();
void flushcom_rx();
int  get_tx_space_left();
int  get_tx_items(); // chars in TX fifo
void WaitForTXEmpty();
*/
// kludge for this file which hopelfully only accesses RS422 port
void putcom(uchar c)    { PutFifo(&serport[RS422MAIN].tx, c);}
int  get_tx_space_left(){ return SpaceLeftInFifo(&serport[RS422MAIN].tx);}
int  get_tx_items()		{ return ItemsInFifo(&serport[RS422MAIN].tx);    }
int  getcom()		    { gZL.RxBytes++; return GetFifo(&serport[RS422MAIN].rx);   }
int  peekcom(int idx)   { return PeekFifo(&serport[RS422MAIN].rx, idx);   }
int  get_com_count()    { return ItemsInFifo(&serport[RS422MAIN].rx);     }
void WaitForTXEmpty()   { WaitForTXEmpty(0, &serport[RS422MAIN].rx);      }
void WaitForTXEmpty(int DesiredSpace) { WaitForTXEmpty(DesiredSpace, &serport[RS422MAIN].rx); }

//struct LEDs gLEDs;
extern TimeCode *TCDisplayed;
extern TimeCode *TCReader;
extern TimeCode *TCGenerator;
//extern int16 gTrackNameChange; // for debug
short gPrintRxedCmds=0;

/*
struct PCGUI_STATE {
	char nTrackMeters, nOutputMeters, nInputMeters;
	char TrackMetersVisible, OutputMetersVisible, InputMetersVisible;
	char InputMetersNum; // which input meter is visible
}gPCGUIstate; // indicates what we need to send to PCGUI program (Nomad Touch or Oasis software)
*/

//struct BANK_STATES BankStates;

extern int gUSER_GUI_event;
extern int gAllow_comprintf;
extern int gfCommandScr;
extern struct PRESETS gPresetCache;
uchar GetBytePeak(float f); //  zaxlan.cpp// input range from 0 to 1.0 (clipping)  returns 0 to 255 (where 245 = -10dBFS)

void SendMP3recModeStuff(int RecModeState);
void auto_send_server_Oasis();
void send_oasis_cmd(uint16 cmd, uint16 bytes, uchar *payloadMightBeNull);

int  CallHandlerViaEnum(int HandlerEnum, void *context, MESSAGE *msg);
void RequestSettingDataFromPC(uint16 SetID);
int  GetBankKey(int keycode); // returns -1 if not a bank key

void SendAllToTablet();
void OasisSegInfoMonitor();

void send_Oasis_Faders();
void send_Oasis_InputPeaks_MIX8();
void send_Oasis_InputPeaks_PC();
void send_Oasis_BusPeaks();
void send_Oasis_OutputPeaks();
void send_Oasis_Lights();
void send_Oasis_Brightness();
void OasisTChandler();
void PCnagScreenHandler();
void send_Oasis_HomeScreenStatus(); // SETTING_BATTERY_STATUS
void OasisProgramUpdater();
void SendNomadVersion();

void SendOasisSegEntry(int seg, char *scene, char *take, char *note);

void RequestVersion(); // this is a mix12 command but it will encapsulate when needed
void RequestInit();
void RequestPCSetting(int SetID);
    
int process_Oasis_version(uchar *str);
int process_Oasis_fader(struct ZAXLAN_CMD_STRUCT *z);

void process_Oasis_trims(struct ZAXLAN_CMD_STRUCT *z);
void process_Oasis_trims(uint16 fader, int val255);

int process_Oasis_outknobs(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_encoder(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_key(struct ZAXLAN_CMD_STRUCT *z); // 
//int process_Oasis_MsgHandlerCall(struct ZAXLAN_CMD_STRUCT *z); 
//int process_Oasis_Setting(struct ZAXLAN_CMD_STRUCT *z);
int process_QRXTX_RX12(struct ZAXLAN_CMD_STRUCT *z); 

int  process_USBkeyCode(struct ZAXLAN_CMD_STRUCT *z); 
int  process_USBkeyCode_FP7(struct ZAXLAN_CMD_STRUCT *z);
void USBKeyboardPortHandler(uint16 ExtKeyCode);
void USBKeyboardPortHandler_FP7(uint16 ExtKeyCode);

int FindLastTrackEnablesIndex();

void SetCrosspointFromOasis         (MixerType MixerBusType, MATRIX_CELL *cell);
void SetHeadPhoneCrosspointFromOasis(MixerType MixerBusType, InputBus InputBusType, MATRIX_CELL *cell);
void SetTapeMonoCrosspointFromOasis (MixerType MixerBusType, int IsRight, InputBus InputBusType, MATRIX_CELL *cell);
												
extern struct ZLAN_STATE gZL;  // mostly for status and debug stuff

int  ParseCommandString_Oasis(struct ZAXLAN_CMD_STRUCT *z); 
void SendKeypressMessage(MESSAGE *KeyMesg); // from tasks.cpp
void SendKeyMsg(MSG_TYPE type, uint16 value); // same thing but simpler

void   cmd_send_LED(uint16 ledcode, int state);
void   cmd_send_CommandSt3ring(char *str);
void   SendOasisMP3RecordFile(struct DIR *dd, char *FolderName, int seg, int FileNameMode);
void   SendOasisMP3RecordMetadata(struct DIR *dd, char *FolderName, int seg, int FileNameMode);
void   UpdateAriaLEDdata();

void   BytesToCell(MATRIX_CELL *cell, uchar *p);
int    CellToBytes(uchar *p, MATRIX_CELL *cell);
int    GetHPmatrix(int HPisRight, int IsCardBus, MixerType HPmixertype, MATRIX_CELL *cell);

float CalcLineEquation(float f1, float f2, float p1, float p2, float channelMHz);
void  OasisSTNupdateHandler();
char *FixVersionString(struct ZAXLAN_CMD_STRUCT *z);
void  PFLClearAll(int clearit) {  PFLSetActiveState(!clearit); } 
//void  USBKeyboardPortHandler();
void  UpdateOasisLEDs();
void  ResendOasisLEDs();
void  SetTrimFromExternalFader(int RawFaderNum, int trim255, int trim2030);
int   process_FP7_key(struct ZAXLAN_CMD_STRUCT *z);

extern enum  TRANSPORT_MODE DMtpmode;   // current transport mode             
extern int gFoundMix12;
extern int gfMix12SyncReq;
//extern int gMix12CharTimeoutMS;

uint32 gSendAllExitTime=0;
int   gMeterSendMS=50;
short gPrintSettingHex=0;     // print hex output of cSetting cmds that Nomad sends to PC
int   gHPrefreshRequest=0;    // force HP mem page to refresh when we change HP preset via PC
extern short gMetersConfigRequest; // home page update meter info
short gReqSetting=0,gForceSendAll=0;
extern short gWaitForTX;
short gQuietMode=0; // dont print debug stuff (during PC init)
float gProtocolVersionFloat = -1; // empty version code

#pragma DATA_SECTION("DATAIRAM");
char gOasisSTNchanged[255]; // flags indicating STN changed in a segment slot in dir[] struct

#define gHDRLEN 6

extern int gNudgeFadersReq;
// this looks for commands in the serial port input stream
int RemoteOasis_Handler()
{
   int i,k,tmp;        
   int16 idx,bytes2get,itemsinfifo; 
   uchar sum;            
   struct ZAXLAN_CMD_STRUCT z; // size is 6+MAX_ZLAN_BLENGTH = 106 bytes
   char *cmdstr;
   static int  inited=0, prevfnum = -1, InitedPC=0;
   static ALARM CharAlarm=0, alarm1S=0, alarm300=0;
   static ALARM MeterAlarm=0,LightAlarm=0, NagHandlerAlarm=0, NomadVersionAlarm=0;
   static SWATCH LastRxedCmdWatch=GMSECS;    // if no rxed cmds for 2 seconds then re-init everything 
   static SWATCH LastVerCmdSentWatch=GMSECS; // time stamp of last time we sent a Request Vesion command

   if(GMSECS<1000)  // make sure everything has settled down first
		return 0;

   if(!inited)
   		{
		const char *s = PROTOCOL_VERSION; // "12.34";
		// sscanf(s,"%2.2f", &gProtocolVersionFloat); why does this not work??? 
		gProtocolVersionFloat =  (s[0]-'0')*10;
		gProtocolVersionFloat += (s[1]-'0')*1;
		gProtocolVersionFloat += (s[3]-'0')*0.1F;
		gProtocolVersionFloat += (s[4]-'0')*0.01F;
		dprintf(cCYAN,"gProtVerFloat = %2.2f\n", gProtocolVersionFloat);	
		//send_oasis_cmd(cVersion,0,NULL);  // block with no payload is a request for data
		SET_STOPWATCH(LastRxedCmdWatch);    // init the timeout timer for last time we got a command from external device
		SET_STOPWATCH(LastVerCmdSentWatch); // 
		memset(&gExtBankStates, 0, sizeof(gExtBankStates)); // init this with NV data stuff!!!!
		memset(gOasisSTNchanged, 0, numberof(gOasisSTNchanged)); // flags indicating STN changed in a segment slot in dir[] struct
		gExtBankStates.CurrentBank = gNV.ExtBank;
		//SendNomadVersion();
		inited=1;
		}

	//SetCurrentSerialPort(RS422MAIN);
	SERIAL_FIFO *pRx = &serport[RS422MAIN].rx;
	SERIAL_FIFO *pTx = &serport[RS422MAIN].tx;

	CheckConnections(); // disconnect anything that has not sent a version command in the last 3-4 seconds

	if( CHECK_ALARM(NomadVersionAlarm) )
		{
		SendNomadVersion();
		SET_ALARM(NomadVersionAlarm, 2000);
		}

	if(CHECK_ALARM(NagHandlerAlarm)) // dont need to call this stuff every time....
		{	
		PCnagScreenHandler();
		if(gReqSetting) // for debug only
			{
			RequestPCSetting(gReqSetting);
			gReqSetting=0;
			}

#ifdef ALLOW_RX4_TOUCH
		if(gForceSendAll) // for debug only
			{
			SendAllToTablet(); 
			gForceSendAll=0;
			}
#endif
		if( (gNV.DebugBits&mDEBUGBITS_COMPRINT) ) // && (GMSECS<3000) )  // 0x0008 // debug mix8 commands  // for debug only
			gAllow_comprintf = 6; // gPrintFaderValues = ; // gPrintRxedCmds = 2;
		SET_ALARM(NagHandlerAlarm, 200);
		}

	int32 stime = (GMSECS-gSendAllExitTime);
	if( IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED|FP7_CONNECTED) && (stime>500) ) // MIX8_CONNECTED
		{
		if(IsConnected(PCGUI_CONNECTED))
			OasisTChandler(); // this needs to be accurate

		if( CHECK_ALARM(MeterAlarm) && IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
			{
			//comprintf("Sending Peaks\n");
			if(IsConnected(PCGUI_CONNECTED | OASIS_CONNECTED) )
				{
				send_Oasis_InputPeaks_PC();
				send_Oasis_InputPeaks_MIX8();
				send_Oasis_OutputPeaks();
				send_Oasis_BusPeaks();
				}

			if(connected.OASIS)
				OasisProgramUpdater();
			
			if(CHECK_ALARM(gExtBankStates.NudgeFaderTimer)) 
				{
				gNudgeFadersReq++;
				SET_ALARM(gExtBankStates.NudgeFaderTimer, 2e9); // disable this timer till it gets set next time
				}

			SET_ALARM(MeterAlarm, gMeterSendMS);
			if(stime < 2000 )
				SET_ALARM(MeterAlarm, 300); // if we just send settings to PC then dont clog up the works yet
			}
		else if (CHECK_ALARM(LightAlarm))
			{
			UpdateAriaLEDdata();
			UpdateOasisLEDs();
			OasisSTNupdateHandler(); // dont call this too often. it can slow down the home screen
			//OasisSegInfoMonitor();
			SET_ALARM(LightAlarm, 100);
			}
		else if ( CHECK_ALARM(alarm300) )
			{
			if(gNudgeFadersReq)
				{
				send_oasis_cmd(cFaders,0  ,NULL); // block with no payload is a request for data
				send_oasis_cmd(cTrims,0   ,NULL); // block with no payload is a request for data
				send_oasis_cmd(cOutKnobs,0,NULL); // block with no payload is a request for data
				gNudgeFadersReq=0;
				}
			SET_ALARM(alarm300, 300);
			}
		else if (CHECK_ALARM(alarm1S))
			{
			send_Oasis_HomeScreenStatus(); // SETTING_BATTERY_STATUS
			SET_ALARM(alarm1S, 1000);
			}
		}
	//else  if(FP8connected())	
	//	{
	//	; // send LEDs?  I think the old mix12 handler does this at the moment
	//	}


   	itemsinfifo = get_com_count(); // ItemsInFifo(pRX);
	if(itemsinfifo==0)
		return 1;

   	// Look for RS422 command strings...
	int PrintedSumErrText=0; // only print chksum error text once in this loop to prevent overflow
   	for (k=0; k < 100; k++) // do for up to 100 chars to prevent underrun
	{                  
    	itemsinfifo = get_com_count(); // ItemsInFifo(pRX);
    	if (itemsinfifo < 3) // need 2 bytes to do anything now
			break;

		// ---- check for old Sony cmd ----
		z.magic_AA  = peekcom(0);
	   	z.sum8      = peekcom(1);                      
		if(z.magic_AA != 0xAA) // look for old Sony style command
			{
   			getcom();  // discard the bogus byte
  			gZL.DiscardBytes++;
  			continue; // goto restart;
			}

		// --- check for new AA cmd --- see if it's a good block before we extract it from the fifo
	   	idx=0;
		z.magic_AA  = peekcom(idx++);
	   	if( (z.magic_AA!=0xAA) )
   			{
   			getcom(); // discard the bogus byte
	   		//comprintf("error: no AA found in parse_block()\n");  // discard header byte
  			gZL.DiscardBytes++;
  			continue; // goto restart;
	  		}

   		//z.magic_AA  = 0xAA;
	   	z.sum8      = peekcom(idx++);                      
   		z.blength   = peekcom(idx++);
		//if(gHDRLEN==7) always 6 
	   	//	peekcom(idx++);
	   	z.sourcerx  = peekcom(idx++);
   		z.destrx    = peekcom(idx++);
	   	z.cmd       = peekcom(idx++);


		if(z.blength>=sizeof(z.bytes)) // can we handle this many bytes?
			{ 
			getcom(); // no, discard AA byte and try again
  			gZL.DiscardBytes++;
			continue;
			}
	
	   if( (get_com_count()-idx) < z.blength)  // do we have the whole block?
    	  {
	      z.magic_AA=0;
		  gZL.RetryCount++;
	      return 1; // NO, wait for more chars to show up
    	  }
			
		// peak at ALL the remaining payload bytes of this packet to see if its good
		for(i=0;i<z.blength;i++)
    	  	z.bytes[i] = peekcom(idx++);

		sum=0;   // --- we have the whole block so calculate the chksum and pull it from the FIFO ---
		tmp = getcom(); // discard header AA byte 
		tmp = getcom(); // discard sum8
		for(i=0;i<idx-2;i++) 
      		{
	  		tmp = getcom(); // discard the whole block since we saw it already
      		sum += tmp;
			//    comprintf("%02X ", tmp&0xFF);
      		}

		cmdstr = GetZaxLanCmdString(z.cmd);

		gZL.RawRxCmdCount++; // show that we got a command (might not be valid though)
		sum &= 0xff;
		if(sum != z.sum8)
      		{
			dprintf("SUM:%s\n", cmdstr); 
			PrintedSumErrText++;
			gZL.ChksumErrors++;
			continue; //discard bytes right away to prevent overflow    
			//return 0;
			}


   		//SET_ALARM(CharAlarm, gMix12CharTimeoutMS); // show that we are not waiting for the rest of a bad command to arrive
   		SET_STOPWATCH(LastRxedCmdWatch);             // timestamp last cmd to arrive

		// ------------- we have full block with a good chksum --------------------
		if(z.blength==0)  // if no data block sent then this is a request cmd
			{
			dprintf(cGREEN, "Ignored REQ %s\n", cmdstr);
			return 1;     // Nomad does not deal with requests
			}	

		int AnnoyingCmd = (z.cmd==cVersion)||(z.cmd==cFaders)||(z.cmd==cTrims)||(z.cmd==cOutKnobs);
        static int32 AnCmdCount=0;
		AnCmdCount+=AnnoyingCmd;
			
		if( ((gAllow_comprintf==4) && (!AnnoyingCmd)) ) // 3 = print everything
			{
			uchar *buf = (uchar *)&z;
			for(i=gHDRLEN;i<z.blength+gHDRLEN;i++)
		    	dprintf("%02X ",buf[i]);
		   	dprintf("\n");
			}

		if( (((!AnnoyingCmd)||(GMSECS<2000))&&(AnCmdCount<5)) || (gAllow_comprintf==3) )  // dont keep printing some repeating commands
			{
			if(z.cmd==cVersion)
				dprintf(cYELLOW, "cVer<%s>\n", FixVersionString(&z)); // , z.blength);
			else	
				dprintf(cGREEN, "%s[0x%02X] len=%d\n", cmdstr, (int)z.bytes[0], z.blength);
			}
		else if( (gAllow_comprintf==6) && ((z.cmd==cFaders)||(z.cmd==cTrims)||(z.cmd==cOutKnobs)) ) // good for printing faders and trims
			dprintf(cCYAN, "%s[%d,%d] len=%d\n", cmdstr, (int)z.bytes[0], (int)z.bytes[1], z.blength);

		gZL.GoodRxCmdCount++;
     	tmp = ParseCommandString_Oasis(&z); 

    }//for(100)------------------------------------------------------------------------------------- 
		
	return 1;	
}


short gMP3IsRecording=0;
int UnpackArray(const unsigned char *buf, unsigned len);


// we come here when a full cmd string passes its chksum
extern enum TRANSPORT_MODE DMtpmode;   // current transport mode             
extern int gSendFreqCount;
extern uint Mix12FaderTimeOut;   
extern const char *gSoftwareVersion; //  = "v7.79";  

int ParseCommandString_Oasis(struct ZAXLAN_CMD_STRUCT *z) // ;int bytes, uchar *buf)
{
	int i,j, findex, connected;
	uint16 SetID, cmd = z->cmd; // buf[5];
	char *pstr;
	uchar *p = z->bytes; // &buf[6]; // payload starts here
	float tmpf;
	MESSAGE msg;

	if(gPrintRxedCmds && (z->cmd != cFaders) )
		{
		dprintf("Got %s %02X %02X L=%d\n",GetZaxLanCmdString(cmd), z->bytes[0], z->bytes[1], z->blength);
		}

	//  Version info...
	switch(cmd) {
		case cVersion:  
		static int32 VersionCounter;
		VersionCounter++;
		if(VersionCounter==1)
			UpdateOasisLEDs(); // force a send of LED data to mixer panel

		if(z->blength==0)
			{ // request for version send "NOMAD 12T v7.79 sn001967"
			SendNomadVersion();
			}
		else
			{
			pstr = FixVersionString(z);
			HandleVersionStringConnection(pstr); // (char *)&buf[2]);
			}
		break;

	case cInit:
		//UpdateOasisLEDs();    
		ResendOasisLEDs();
		break;
	
	case cFaders:
	case cID_Fader: // FP7 fader
		process_Oasis_fader(z);    
		break;

	case cID_Key:   // FP7 key
		process_FP7_key(z);      
		break;

	case cTrims:
		process_Oasis_trims(z);    
		break;

	case cOutKnobs:
		process_Oasis_outknobs(z); 
		break;

	case cKey:
		process_Oasis_key(z); 
		break;


	case cEncoder:
		process_Oasis_encoder(z);  
		break;

	case cEvent: // TC button event from GUI
		z->bytes[z->blength]=0; // null terminate the string
		dprintf("cEvent: <%s>\n", z->bytes);
		msg.Type = MSG_ACTION; 
		// process_Oasis_encoder(z);  
		if(strcmp((const char *)z->bytes, "jam.tc")==0)
			{
			OnSettingTCGenJAM_TC(NULL, &msg);
			break;
			}
		if(strcmp((const char *)z->bytes, "jam.ub")==0)
			{
			OnSettingTCGenJAM_UB(NULL, &msg);
			break;
			}
		if(strcmp((const char *)z->bytes, "jam.date")==0)
			{
			OnSettingTCGenJAM_Date(NULL, &msg);
			break;
			}
		if(strcmp((const char *)z->bytes, "jam.time")==0)
			{
			OnSettingTCGenJAM_Time(NULL, &msg);
			break;
			}

		if(strcmp((const char *)z->bytes, "mp3.rec")==0)
			{ // why are we doing this toggle thing here. seems wrong
			if(gMP3IsRecording)
				SendMP3recModeStuff(0);	// stop
			else
				SendMP3recModeStuff(1);	// record
			break;
			}

		if(strcmp((const char *)z->bytes, "mp3.stop")==0) // added in nova v2.39
			{
			SendMP3recModeStuff(0);	// stop
			break;
			}
		break; // case cEvent

	case cSetting: // cMsgHandlerCall:
		if( !PCGUIconnected() ) // && (GMSECS>10000) )
			break;// dont accept commands if we are not allowed to (when demo expires or if not a nomad 12T)
		GotPCsettingCommand();
		UnpackArray(z->bytes, z->blength); 
		j=0; // offset into z->bytes[j] = payload data
		SetID = BytesToU16(&z->bytes[j]); j+=2;
		dprintf(cGREEN, "%s(L=%d):", GetSettingString(SetID), (int)z->blength);
		for(i=0;i<4;i++)       dprintf(cGREEN, " %02x", z->bytes[i]);
		for(;i<z->blength;i++) dprintf(" %02x", z->bytes[i]); dprintf("\n");
		for(i=4;i<z->blength;i++) 
			if(isprint(z->bytes[i])) 
				dprintf("%c", z->bytes[i]); 
		dprintf("\n");
		break;


	case cQRXTX:
		process_QRXTX_RX12(z);
		break;

	case cRXFORMAT: // ignore this stupid cmd form RX12
	case cSUBCH: // ignore this  cmd form RX12
		break;

	case cUSBkeyCode:
		process_USBkeyCode(z);
		break;
	case cUSBkeyCodeFP7:
		process_USBkeyCode_FP7(z);
		break;


	case cHT_ButtonPress: {
		dprintf("Got HT_ButtonPress: ");
		uchar *buf = (uchar *)z;
		for(i=0;i<z->blength+gHDRLEN;i++)
		    dprintf("%02X ",buf[i]);
		dprintf("\n");
		break;
		}

	default:    // unexpected command...    
		if( gAllow_comprintf || (GMSECS<15000) )
			{
			uchar *buf = (uchar *)z;
	   		dprintf(cRED, "Unk cmd %s\n",GetZaxLanCmdString(cmd)); //  = %02X buf[0]=%02X %02X %02X\n",(int)cmd, (int)p[0], (int)p[1], (int)p[2]);
			if(gAllow_comprintf==2)
				for(i=0;i<z->blength+gHDRLEN;i++)
				    comprintf("%02X ",buf[i]);
			comprintf("\n");
			}
	   break;
	}

   return cmd;
}


void SendNomadVersion()
{
	char s[40];
#if 0 // def ALLOW_RX4_TOUCH
	sprintf(s, "%s %s sn%06X","NOMAD NOVA", gSoftwareVersion, gMachineInfo.SerialNumber);   
	send_oasis_cmd(cVersion, strlen(s), (uchar *)s);
#else
	sprintf(s, "%s %s sn%06X",gMachineInfo.FullModelName, gSoftwareVersion, gMachineInfo.SerialNumber);   
	send_oasis_cmd(cVersion, strlen(s), (uchar *)s);
#endif
}
bool gDontSendAnnoyingackets=0;
int   gMaxOasisCmdBytes=0; // keep track of largest packet
static uchar gZLANoutbufPrivate[512+20]; // this global buffer makes debugging easier
void send_oasis_cmd(uint16 cmd, uint16 bytes, uchar *payloadMightBeNull)
{
	int i,j,sum=0;
 	uchar *buf = gZLANoutbufPrivate;
	uchar *payloadsafe = payloadMightBeNull;
	SERIAL_FIFO *pRx = &serport[RS422MAIN].rx; //	SetCurrentSerialPort(portenum);
	SERIAL_FIFO *pTx = &serport[RS422MAIN].tx; //	SetCurrentSerialPort(portenum);

//	if( !IsConnected(PCGUI_CONNECTED) ) // TABLET_CONNECTED|DEVA32_CONNECTED|OASIS_CONNECTED) )
//		return;

	if(bytes > gMaxOasisCmdBytes)
		gMaxOasisCmdBytes = bytes; // keep track of worst case situation
	if(bytes>135)
		dprintf(cRED,"[[[OasisCMDbytes=%d]]]\n",bytes);

	if(bytes<=0) 
		payloadsafe = &gZLANoutbufPrivate[20]; // make sure payload pointer is valid in case we read it

	if(bytes >= (numberof(gZLANoutbufPrivate)-7))
		{
		brkpt(1, "BYTES");
		dprintf(cRED, "Playload too big %d\n",bytes);
		bytes = numberof(gZLANoutbufPrivate)-1; // truncate packet!!!!
		}

	j=0;
	buf[j++]=0xAA;    // 0  hdr
	buf[j++]=0;       // 1  chksum
	buf[j++]=bytes;   // 2  Blength = x bytes of payload data (Blength==0 for requests) IS 256 bytes LONG ENOUGH????
	buf[j++]=bytes>>8;// 3  (now MSbyte of length) src:  // shouldnt this be ff to mean the "master device"?
	buf[j++]=0x00;    // 4  dest: // shouldnt this be FF to mean "send to everybody"?
	buf[j++]=cmd;     // 5  ZaxLan command code (see ZaxLan.h)
	// payload starts here
	for(i=0;i<bytes;i++)
		buf[j++]=payloadsafe[i];

	sum=0; // calulate the chksum of the entire packet 
	for(i=0;i<j-2;i++)
		sum += buf[i+2];
	buf[1] = sum & 0xff;

	int AnnoyingPacket = (cmd==cInPeaksMix8) || (cmd==cInPeaksPC) || (cmd==cBusPeaks) || (cmd==cOutPeaks) || 
						 (cmd==cTimecode)    || (cmd==cVersion)   || (cmd==cFaders)   || (cmd==cTrims) || (cmd==cOutKnobs); // 

	uint16 SetID = 	BytesToU16(&buf[6]);
	if(cmd==cSetting) // dont print periodic home screen status commands too
		AnnoyingPacket = (SetID==SETTING_BATTERY_STATUS) || (SetID==SETTING_DRIVE_TIME_REMAINING) || (SetID==SETTING_DRIVE_TIME_ELAPSED)||(SetID==SETTING_STATUS_TEXT);

//if(gDontSendAnnoyingackets)
//	return;

	for(i=0;i<j;i++)
		PutFifo(pTx, buf[i]); // putcom(buf[i]);

	if(gWaitForTX)
		WaitForTXEmpty(pTx);
		
	if(gPrintSettingHex && (cmd==cSetting) && (!AnnoyingPacket) ) // just print cSetting part of packet
		{for(i=6;i<j;i++) dprintf(" %02x", buf[i]); dprintf("\n");}

	if( (gPrintSettingHex==5) && (cmd==cSetting) && (!AnnoyingPacket) ) // just print cSetting part of packet
		{for(i=6;i<j;i++) dprintf("%c", buf[i]); dprintf("\n");}

   return;
}

void RequestSettingDataFromPC(uint16 SetID)
{
	int j=0; uchar p[10];
	U16toBytes(p, SetID);
	j+=2;
	send_oasis_cmd(cRequest, j, p);
	return;
}

// 16 bit mask (like mLED_BANK3), and on off state 
void cmd_send_CommandString(char *str)
{
	int j=0; uchar p[50];
	strcpy((char *)p, str);
	j = strlen(str); 
	send_oasis_cmd(cCommand, j, p);
	dprintf(cGREEN, "Send cCommand = <%s>\n",str);
	return;
}

// 16 bit mask (like mLED_BANK3), and on off state 
void cmd_send_LED(uint16 ledcode, int state)
{
//dprintf("LED  %04X %d %d\n", ledcode, state, gMSECs);
	int j=0; uchar p[10];
	p[j++] = ledcode>>8;
	p[j++] = ledcode&0xFF;
	p[j++] = state; // this can be brightness too
	send_oasis_cmd(cLED, j, p);
	gZL.LEDcmdCount++;
	return;
}

uint64 GetFP7_UUID(int idex);
void cmd_send_FP7_LED(uint16 ledcode, int state)
{
dprintf("FP7LED  %04X %d %d\n", ledcode, state, gMSECs);
	int j=0; uchar p[20];
	p[j++] = ledcode>>8;
	p[j++] = ledcode&0xFF;
	p[j++] = state; // this can be brightness too
	uint64 u = GetFP7_UUID(0);
	for(int i=0;i<8;i++)
		{
		p[j++] = u&0xFF;
		u >>=8;
		//sprintf((char *)&p[j],"id:%16llX", GetFP7_UUID(0));
		}
	send_oasis_cmd(cID_LED, j, p);
	gZL.LEDcmdCount++;
	return;
}


uchar TrackLEDs[16];
 // this decides the states of the TRK leds (UpdateOasisLEDS sends the data in gLEDs.Channel[i].TRK1,TRK2)
void UpdateAriaLEDdata()
{
	int i,  val, input, inputRaw, isdigital;
	XPOINT_STATE x;
	static ALARM alarm=0;
	if(!CHECK_ALARM(alarm))
		return;
	SET_ALARM(alarm,200);
	
	gLEDs.SCENE=0; // TRK_ENABLES LED
	gLEDs.TAKE=0;  // REC_ENALES LED
	gLEDs.NOTE=0;  // FADER_ASSIGN LED
	int nfaders = GetNumOasisFaders();
	for(i=0;i<nfaders;i++)
		{
		gLEDs.Channel[i].TRK1 = 0;
		gLEDs.Channel[i].TRK2 = 0;
		gLEDs.Channel[i].CH = 0;
		input = inputRaw = GetAudioChannelFromFaderAssign(i+(EXTERNAL_FADER_1)); 	
		if(input < 0)
			continue; // nothing assigned to this fader

		isdigital = 0;
		if(input >= IN_CHAN_DIGITAL_1)
			{
			input -= IN_CHAN_DIGITAL_1;
			isdigital = 1;
			}
		if((input >= IN_CHAN_DIGITAL_1) && gJTAGcableDebugActive)
			{
			dprintf(cRED,"UpLED! in=%d\n",input);
			continue;
			}

		if(connected.ARIA) // ccc   LED tally
			{
			if( (UIPageCurrent() == &UITrackXMix) ) // || (UIPageCurrent() == &UITrack2MixEnables) )	// show which faders are assigned to TRK1 or TRK2
				{ 
				int trk = Nomad.Settings.CurTrackMixPageNum; // (UIPageCurrent() == &UITrack2MixEnables);
				x = GetCrossPoint(DISK_MIXER, INPUT_BUS, input, trk); // check DISK TRACK #1 or #2
				if(!isdigital)	gLEDs.Channel[i].CH = logic(x.A); 		
				else		    gLEDs.Channel[i].CH = logic(x.B); 		
				gLEDs.SCENE=1;  // TRK_ENABLES LED
				}
			else if(UIPageCurrent() == &UIRecordEnables)	 // show which faders are enabled for recording
				{ 
				int trk = FindCardTrackFromFaderNum(i+EXTERNAL_FADER_1);
				if(trk >= 0)
					gLEDs.Channel[i].CH = logic(gNV.fTracksToRecord[trk]); 
				gLEDs.TAKE=1;  // REC_ENALES LED
				}
			else if(UIPageCurrent() == &UIExtFaderAssignList)  // show which faders are assigned to something	
				{
				gLEDs.Channel[i].CH = (input >= 0);
 				gLEDs.NOTE=1;  // FADER_ASSIGN LED
				}
			else if(UIPageCurrent() == &UISetupChanX)	
				{ // show which fader is associated with the input that we are looking at on the Nova
				gLEDs.Channel[i].CH = (inputRaw == Nomad.Setup.Chan);
				}
			}
		else // oasis or mix16
			{
			x = GetCrossPoint(DISK_MIXER, INPUT_BUS, input, 0); // check DISK TRACK #1
			if(!isdigital)	gLEDs.Channel[i].TRK1 = logic(x.A); 		
			else		    gLEDs.Channel[i].TRK1 = logic(x.B); 		
			x = GetCrossPoint(DISK_MIXER, INPUT_BUS, input, 1); // check DISK TRACK #2
			if(!isdigital)	gLEDs.Channel[i].TRK2 = logic(x.A); 		
			else			gLEDs.Channel[i].TRK2 = logic(x.B); 		
			}

		}//for(nfaders)

	return;
}

unsigned PFLGetChanMask(int GetAlt); 
bool     PFLIsActive();
unsigned PFLGetLastChangedTime(); 
int      PFL_GetFaderIsActive(unsigned fnum);

bool gForceInit=0;
// this allows a new Oasis connection to force -s end LED data to a newly booted Oasis or Arria
void ResendOasisLEDs()
{
	gForceInit=1;
}


int gBlinkTime=200;
// BUS/PFL/EQ/CH lights command...	
void UpdateOasisLEDs()
{
	static struct LEDs prevLEDs;
	static short initme=1, prevBrightness = -1, blink=0, bank=-1, BlinkChanged=0;
	static ALARM BlinkAlarm=0;
	bool force = gForceInit;
	if(initme)
		{
		memset(&prevLEDs, 0, sizeof(prevLEDs));
		initme=0;
		}

	if(CHECK_ALARM(BlinkAlarm))
		{
		blink = !blink;
		BlinkChanged=1;
		SET_ALARM(BlinkAlarm, gBlinkTime);
		}

	if(BlinkChanged && (TIMERDIFF(GetOasisConnectTime()) < 2000) )
		{
		force=1; // make sure tally makes it to Aria when connecting
		//dprintf(cGREEN, "\nForced Oasis Tally\n");
		}

	if(connected.FP7)
		{
		for(int k=0;k<7;k++)
			{
			int input = GetAudioChannelFromFaderAssign(k+(FP7_FADER_1)); 	
			if(input < 0)
				continue; // nothing assigned to this fader
	
			gLEDs.FP7_PFL[k] = blink &&  PFLIsActive() && (PFL_GetFaderIsActive(k+FP7_FADER_1) >= 0 );			
			if (gLEDs.FP7_PFL[k] != prevLEDs.FP7_PFL[k]) 
				{
				cmd_send_FP7_LED(mLED_CHBASE_PFL | k, gLEDs.FP7_PFL[k]); 
				prevLEDs.FP7_PFL[k] = gLEDs.FP7_PFL[k];
				}
			}//for(nfaders)
		}//fp7


	if ( (gLEDs.PLAY != prevLEDs.PLAY)  || (force && gLEDs.PLAY) ) {cmd_send_LED(mLED_PLAY, gLEDs.PLAY!=0); prevLEDs.PLAY = gLEDs.PLAY; }
	if ( (gLEDs.REC  != prevLEDs.REC)   || (force && gLEDs.REC)  ) {cmd_send_LED(mLED_REC,  gLEDs.REC!=0);  prevLEDs.REC  = gLEDs.REC;	}
	if ( (gLEDs.STOP != prevLEDs.STOP)  || (force && gLEDs.STOP) ) {cmd_send_LED(mLED_STOP, gLEDs.STOP!=0); prevLEDs.STOP = gLEDs.STOP; }

	if(bank != gNV.ExtBank)
		{
		bank = gNV.ExtBank;
		cmd_send_LED(mLED_BANK1, bank==0);
		cmd_send_LED(mLED_BANK2, bank==1);
		cmd_send_LED(mLED_BANK3, bank==2);
		cmd_send_LED(mLED_BANK4, bank==3);
		cmd_send_LED(mLED_BANK5, bank==4);
		}

	int k = GetHPmemIndex();
	gLEDs.HP1=gLEDs.HP2=gLEDs.HP3=0;
	if(k==0) gLEDs.HP1=1;
	if(k==1) gLEDs.HP2=1;
	if(k==2) gLEDs.HP3=1;

	if ( (gLEDs.HP1 != prevLEDs.HP1) || (force && gLEDs.HP1) ) {cmd_send_LED(mLED_HP1, gLEDs.HP1); prevLEDs.HP1 = gLEDs.HP1;}
	if ( (gLEDs.HP2 != prevLEDs.HP2) || (force && gLEDs.HP2) ) {cmd_send_LED(mLED_HP2, gLEDs.HP2); prevLEDs.HP2 = gLEDs.HP2;}
	if ( (gLEDs.HP3 != prevLEDs.HP3) || (force && gLEDs.HP2) ) {cmd_send_LED(mLED_HP3, gLEDs.HP3); prevLEDs.HP3 = gLEDs.HP3;}
	if ( (gLEDs.SCENE != prevLEDs.SCENE) || (force && gLEDs.SCENE) ) {cmd_send_LED(mLED_SCENE, gLEDs.SCENE); prevLEDs.SCENE = gLEDs.SCENE; }
	if ( (gLEDs.TAKE  != prevLEDs.TAKE)  || (force && gLEDs.TAKE) )  {cmd_send_LED(mLED_TAKE, gLEDs.TAKE);   prevLEDs.TAKE  = gLEDs.TAKE;  }
	if ( (gLEDs.NOTE  != prevLEDs.NOTE)  || (force && gLEDs.NOTE) )  {cmd_send_LED(mLED_NOTE, gLEDs.NOTE);   prevLEDs.NOTE  = gLEDs.NOTE;  }

	gLEDs.SLATE = (gfSlateOn || gfExtSlateOn);
	gLEDs.TALK1 = gfExtTalk1On;
	gLEDs.TALK2 = gfExtTalk2On;

	if ( (gLEDs.SLATE != prevLEDs.SLATE) || (force && gLEDs.SLATE) ) { cmd_send_LED(mLED_SLATE, gLEDs.SLATE); prevLEDs.SLATE = gLEDs.SLATE;}
	if ( (gLEDs.TALK1 != prevLEDs.TALK1) || (force && gLEDs.TALK1) ) { cmd_send_LED(mLED_TALK1, gLEDs.TALK1); prevLEDs.TALK1 = gLEDs.TALK1;}
	if ( (gLEDs.TALK2 != prevLEDs.TALK2) || (force && gLEDs.TALK2) ) { cmd_send_LED(mLED_TALK2, gLEDs.TALK2); prevLEDs.TALK2 = gLEDs.TALK2;}

	// if no channels are active then turn off PFL active state (this does not naturally happen on Nomad when no Oasis is connected)
	//	if(PFLIsActive() && (PFLGetChanMask(0)==0) && (PFLGetChanMask(1)==0) && (PFLGetLastChangedTime()>500) )
	//		PFLSetActiveState(0);

	// THIS JUSTS SENDS DATA FROM gLEDs.Channel[].* it does not decide what that states should be
	int nfaders = GetNumOasisFaders();
	for (k=0; k < nfaders; k++)
		{
		// PFL key always blinks when it is lit
		gLEDs.Channel[k].PFL = blink &&  PFLIsActive() && (PFL_GetFaderIsActive(k+EXTERNAL_FADER_1) >= 0 );

		if (gLEDs.Channel[k].BUS != prevLEDs.Channel[k].BUS) {cmd_send_LED(mLED_CHBASE_BUS | k, gLEDs.Channel[k].BUS); prevLEDs.Channel[k].BUS = gLEDs.Channel[k].BUS;}
		if (gLEDs.Channel[k].PFL != prevLEDs.Channel[k].PFL) {cmd_send_LED(mLED_CHBASE_PFL | k, gLEDs.Channel[k].PFL); prevLEDs.Channel[k].PFL = gLEDs.Channel[k].PFL;}
		if (gLEDs.Channel[k].CH  != prevLEDs.Channel[k].CH)  {cmd_send_LED(mLED_CHBASE_CH  | k, gLEDs.Channel[k].CH);  prevLEDs.Channel[k].CH  = gLEDs.Channel[k].CH;}

/*
//if(k==0) dprintf("OasisLEds %d %d \n",gMSECs,blink);
		if (gLEDs.Channel[k].PFL != prevLEDs.Channel[k].PFL) 
			{
			cmd_send_LED(mLED_CHBASE_PFL | k, gLEDs.Channel[k].PFL); prevLEDs.Channel[k].PFL = gLEDs.Channel[k].PFL;
//			cmd_send_LED(mLED_CHBASE_PFL | k, gLEDs.Channel[k].PFL); prevLEDs.Channel[k].PFL = gLEDs.Channel[k].PFL;
			}
		if (gLEDs.Channel[k].CH  != prevLEDs.Channel[k].CH)  {cmd_send_LED(mLED_CHBASE_CH  | k, gLEDs.Channel[k].CH);  prevLEDs.Channel[k].CH  = gLEDs.Channel[k].CH;}
*/
		if (!connected.ARIA) // these LEDs do not exist on an ARIA
			{
			if(gLEDs.Channel[k].EQ != prevLEDs.Channel[k].EQ)	  {cmd_send_LED(mLED_CHBASE_STAR | k, gLEDs.Channel[k].EQ);  prevLEDs.Channel[k].EQ   = gLEDs.Channel[k].EQ;}
			if (gLEDs.Channel[k].TRK1 != prevLEDs.Channel[k].TRK1){cmd_send_LED(mLED_CHBASE_TK1 | k, gLEDs.Channel[k].TRK1); prevLEDs.Channel[k].TRK1 = gLEDs.Channel[k].TRK1;}
			if (gLEDs.Channel[k].TRK2 != prevLEDs.Channel[k].TRK2){cmd_send_LED(mLED_CHBASE_TK2 | k, gLEDs.Channel[k].TRK2); prevLEDs.Channel[k].TRK2 = gLEDs.Channel[k].TRK2;}
			}//if(!aria)
		}

	if(force) gForceInit=0;
	return;	
}

void send_Oasis_InputPeaks_MIX8(void)
{
	int i,c,count,j=0,lastNonZero=0;
	float f;
	uchar p[32];

	// Meter the inputs mode...
	count=16; // 16;
	for (i=0; i < count; i++) // Mix8 has 8 meters but this has to be 12 for this command p to work 
		{
		f=Peaks.ExtFaderInputsReMap[i].Peak; // (16)
		Peaks.ExtFaderInputsReMap[i].Peak=0;
		p[j] = GetBytePeak(f);
		if(p[j] > 100)
			lastNonZero = j;
		j++;
		}
	// do 17th peak for current input channel
	f=CurrentInputPeak.Peak; // (16)
	CurrentInputPeak.Peak=0;
	p[j] = GetBytePeak(f);
	if(p[j] > 100)
		lastNonZero = j;
	j++;
	
	j = lastNonZero+1;
	if(j<8)
		j=8; // Oasis up to v3.13 assumes 8 meters always comes in!! and will use bad data otherwise

	send_oasis_cmd(cInPeaksMix8,j,p); 
}

void send_Oasis_InputPeaks_PC(void)
{
	int i,c,count,j=0,lastNonZero=0;
	float f;
	uchar p[32];
	if(!PCGUIconnected())
		return;

	// Meter the inputs mode...
	count=6+4+8;//numberof(Peaks.InputLinear); // 16
//count=8; // we need to look at this concept  -  one meter only nees to be shown at a time
	for (i=0; i < count; i++) 
		{
		f=Peaks.InputLinear[i].Peak; // Peaks.InputReMapped[i].Peak; 
		Peaks.InputLinear[i].Peak=0;
		p[j] = GetBytePeak(f);
		if(p[j] > 100)
			lastNonZero = j;
		j++;
		}
	j = lastNonZero+1;
	send_oasis_cmd(cInPeaksPC,j,p); 
}

void send_Oasis_BusPeaks(void)
{
	int i,j=0,count,lastNonZero=1;
	uchar p[42];
			
	count = max(gMachineInfo.MaxRecordChannels,12); /// ???
	for (i=0; i < count; i++) // Mix8 has 8 meters but this has to be 12 for this command p to work 
		{
		p[j] = GetBytePeak(Peaks.Card[i].Peak);
		Peaks.Card[i].Peak=0;
		if(p[j] > 100)
			lastNonZero = j;
		j++;
		}

	send_oasis_cmd(cBusPeaks,lastNonZero+1,p);
}

void send_Oasis_OutputPeaks(void)
{
	int i,j=0,count;
	uchar p[42];
			
	count=6;
	for (i=0; i < count; i++) 
		{   
		p[j++] = GetBytePeak(Peaks.Output[i].Peak); 
		Peaks.Output[i].Peak=0;
		}

	send_oasis_cmd(cOutPeaks,j,p);
}

extern short gPCTClag; // =2;
void OasisTChandler()
{
	int i,j, secs, newmode;
	uint32 tc,ub;
	uchar p[32];
	static int PrevSeconds = -1, PrevReaderSeconds = -1, PrevTPmode = -1;
	static ALARM TCforceAlarm=0, TCforceReaderAlarm=0;
	if(!PCGUIconnected())
		return;
	j=0;
	p[j++] = gNV.FrameRate;
	if(gNV.TimecodeRunMode == TC_RUNMODE_REC)
		p[0] |= 0x80; // set REC RUN bit

	TCDisplayed->GetTimeCodePacked(&p[1], gPCTClag); // HH MM SSS FF LTC  (advance 3 frames to compensate for any delays)
	j += 5;
	secs = p[3];

	// ------only send TC if (change in seconds) or (alarm) or tpmode changes-----
	newmode = GetTransportMode();
	if( (PrevSeconds != secs) || (PrevTPmode != newmode) || CHECK_ALARM(TCforceAlarm) )
		{
		SET_ALARM(TCforceAlarm, 1100);
		
		//p[j++] = (tc>>24)&0xFF; p[1]  hh 
		//p[j++] = (tc>>16)&0xFF; p[2]  mm
		//p[j++] = (tc>>8)&0xFF;  p[3]  ss
		//p[j++] = tc&0xFF;       p[4] frames
    	//p[j++] = ltc;           p[5] ltcbit

		ub = TCDisplayed->GetUserBits();
		p[j++] = (ub>>24)&0xFF;
		p[j++] = (ub>>16)&0xFF;
		p[j++] = (ub>>8)&0xFF;
		p[j++] = ub&0xFF;

		// newmode = GetTransportMode();
		if(newmode == RECORD_MODE)    p[j++]=ZNET_RECORD_MODE;
		else if(newmode == STOP_MODE) p[j++]=ZNET_STOP_MODE;
		else if(newmode == PLAY_MODE) p[j++]=ZNET_PLAY_MODE;
		else p[j++] = UNKNOWN_MODE;

		send_oasis_cmd(cTimecode,j,p);
		#if 0
		j=0; // for debug....
		comprintf("sent Oasis TC: %02X : ",(int)p[j++]);
		for(i=0;i<4;i++)
			comprintf("%d:", (int)p[j++]); // hh mm ss ff
		comprintf("(%d): ", (int)p[j++]); // ltc byte
		for(i=0;i<4;i++)
			comprintf("%02X ", (int)p[j++]); // ubits
		comprintf("\n");
		#endif

		#if 1 // def NOMAD		
		if( (PrevTPmode != newmode) && (gNV.GUI_MP3Recmode==PC_MP3_RECMODE_FOLLOW) )
			SendMP3recModeStuff(newmode == RECORD_MODE); // update PCs MP3 record mode
		#endif

		PrevSeconds = secs;
		PrevTPmode = newmode;
		}

	int activeFrozen  = (GetReader_FrozenFrameTime() < 1000);
	int activeRunning = (GetReader_MovedFrameTime()  < 1000);
	int active = activeFrozen || activeRunning;

	j=0;
	p[j++] = gNV.FrameRate;
	if(activeFrozen)
		p[0] |= 0x80; // set REC RUN bit
	TCReader->GetTimeCodePacked(&p[1], 2); // HH MM SSS FF LTC  (advance 2 frames to compensate for any delays)
	secs = p[3];
	j += 5;
	if( (activeRunning && (PrevReaderSeconds != secs)) || (activeFrozen && CHECK_ALARM(TCforceReaderAlarm)) )
		{
		PrevReaderSeconds = secs;
		//PrevTPmode = newmode;
		SET_ALARM(TCforceReaderAlarm, 1100);
		
		ub = TCReader->GetUserBits();
		p[j++] = (ub>>24)&0xFF;
		p[j++] = (ub>>16)&0xFF;
		p[j++] = (ub>>8)&0xFF;
		p[j++] = ub&0xFF;
		//tmp = GetTransportMode();
		//if(tmp == RECORD_MODE)    p[j++]=ZNET_RECORD_MODE;
		//else if(tmp == STOP_MODE) p[j++]=ZNET_STOP_MODE;
		//else if(tmp == PLAY_MODE) p[j++]=ZNET_PLAY_MODE;
		//else p[j++] = UNKNOWN_MODE;
		p[j++]=0;
		send_oasis_cmd(cTimecodeReader,j,p);
		#if 0
		j=0; // for debug....
		comprintf("sent Oasis Reader TC: fr%02X : ",(int)p[j++]);
		for(i=0;i<4;i++)
			comprintf("%02d:", (int)p[j++]); // hh mm ss ff
		comprintf("(%03d): ", (int)p[j++]); // ltc byte
		for(i=0;i<4;i++)
			comprintf("%02X ", (int)p[j++]); // ubits
		comprintf("\n");
		#endif

		}


	return;
}

void SendGotoPageCommand(char *str)
{
	int j=0;
	uchar p[66];
	comprintf("GotoPage <%s>\n", str);
	for(int i=0;i<strlen(str);i++)
		p[i] = str[j++];
	send_oasis_cmd(cGotoPage, j, p);
	return;
}

short gOasisBacklight=255, gPWMmax=16;
void SendBrightnessCommand(int x) //, int meters, int keys, int tpkeys, int pwm)
{
	int j=0, backlight, meters, keys, tpkeys, pwm;
	pwm=16;
	backlight=1; // always lowest possible setting
	meters=keys=tpkeys=x; // 16 is max brightness at this pwm setting
	if(x>15)
		{
		pwm=7;  // use smaller pwm val for less flicker - sideffect is backlight being twice as bright
		meters=keys=tpkeys = x-15; // 7 is max brightnes at this point
		if(x==29)
			{
			backlight=0; // user can turn off backlight in direct sunlight to save power	
			pwm=1;  // force max brightness with no flicker (backlight cannot use this mode)
			}
		// need to allow 30 to be the default setting with backlight so it has to be normal 
		}
	if(x==0)
		backlight=0;
	uchar p[19];
	p[j++] = backlight; 
	p[j++] = meters; 
	p[j++] = keys; 
	p[j++] = tpkeys;
	p[j++] = pwm;
	dprintf(" \tcBright B=%d, mkt=%d pwm=%d\n", backlight, x, pwm);
	send_oasis_cmd(cBright,j,p);
	return;
}


void RequestInit()
{
	send_oasis_cmd(cInit,    0,NULL); // Tell mixer to send all fader info and reset LEDs to default state.
	send_oasis_cmd(cFaders,  0,NULL); // block with no payload is a request for data
	send_oasis_cmd(cTrims,   0,NULL); // block with no payload is a request for data
	send_oasis_cmd(cOutKnobs,0,NULL); // block with no payload is a request for data
}

int gNudgeFadersReq=0;
void RequestNudgeFaders()
{
	gNudgeFadersReq++;
}
void RequestVersionOasis()
{
	send_oasis_cmd(cVersion,0,NULL); // block with no payload is a request for data
}
void RequestPCSetting(int SetID)
{
	int i,j=0;
	uchar p[12];
	dprintf(cBLUE, "Requesting %s\n", GetSettingString(SetID));
	U16toBytes(&p[j], SetID); j+=2;
	send_oasis_cmd(cSetting,j,p);
	return;
}


int  process_USBkeyCode(struct ZAXLAN_CMD_STRUCT *z)
{
	if(z->blength<2)
		return 0; // error
	int i,codecount = MIN(z->blength/2, 20);
	for(i=0;i<codecount;i++)
		{ // do for all key code words
		uint16 ExtKeyCode;
		ExtKeyCode = z->bytes[0];
		ExtKeyCode <<= 8;
		ExtKeyCode |= z->bytes[1];
		USBKeyboardPortHandler(ExtKeyCode);
		}
	return i;
}

void USBKeyboardPortHandler_FP7(uchar code, uchar modif);
int  process_USBkeyCode_FP7(struct ZAXLAN_CMD_STRUCT *z)
{
	if(z->blength<2)
		return 0; // error

	//dprintf(cGREEN, "FP7 USB KEY: %02X %02X\n", z->bytes[0], z->bytes[1]);
	int i,codecount = MIN(z->blength/2, 20);
	for(i=0;i<codecount;i++)
		{ // do for all key code words
		USBKeyboardPortHandler_FP7(z->bytes[0], z->bytes[1]);
		}
	return i;
}


extern struct USB_KEYBOARD gKBD;
int UI_GetCurrentOutputBus5to8enum(void); 
int UI_SetCurrentOutputBus5to8enum(int val0to3);

int process_FP7_key(struct ZAXLAN_CMD_STRUCT *z) 
{
	uint16 keycode=z->bytes[0], shift=z->bytes[1]&KEY_SHIFT_MASK;
	int i,key, index,channel, digit, fnum; 
	MSG_TYPE MsgPressMode = MSG_KEY_PRESS;

	dprintf("\t\t\tFP7key=%02X\n",(int)keycode); 
	if(z->bytes[1] & KEY_RELEASE_MASK) 
		{
		MsgPressMode = MSG_KEY_RELEASE;
		if ((keycode >= STRIP_KEY_PFL) && (keycode <= (STRIP_KEY_PFL+15)) ) // on nomad this should be limited to 0x57 right?
			{
			digit = (keycode - STRIP_KEY_PFL) & 0xF;
			fnum  = digit + FP7_FADER_1; //
			PFLtoggleChan(digit, true, true);
			}
		}
	return 1;
}

int process_Oasis_key(struct ZAXLAN_CMD_STRUCT *z) 
{
	uint16 keycode=z->bytes[0], shift=z->bytes[1]&KEY_SHIFT_MASK;
	int i,key, index,channel, digit, fnum; 
	MSG_TYPE MsgPressMode = MSG_KEY_PRESS;
	MESSAGE msg;
	msg.Type = MSG_VALUE_SET;

	comprintf("\t\t\tkey=%02X\n",(int)keycode); 
	if(z->bytes[1]&KEY_RELEASE_MASK) 
		MsgPressMode = MSG_KEY_RELEASE;

	static int lastkeycode=0;
	static uint32 KeyPressTime=0;
	bool IsHoldRelease=false;
	if( (MsgPressMode == MSG_KEY_PRESS) && (keycode != 0x00) )
		{
		KeyPressTime = GMSECS; // if not a key release code then time-stamp this key press
		lastkeycode  = keycode;
		}
	else if( TIMERDIFF(KeyPressTime) > 300) // held key release detected
		IsHoldRelease = true;


	// These keys are sent and translated as key presses AND releases to Nomad or Nova
	// PFL buttons are triggered on release only since you can hold them...
	switch(keycode){
		case oaKEY_STAR:    SendKeyMsg(MsgPressMode, KEY_STAR); return 1;
		case oaKEY_ENTER:   SendKeyMsg(MsgPressMode, KEY_MENU); return 1;
		case oaKEY_BACK:    SendKeyMsg(MsgPressMode, KEY_BACK);	PFLClearAll(1);   return 1;
		case oaKEY_PLUS:  
			if(PCGUIconnected()) // SendKeyMsg(MsgPressMode, KEY_F7);   
				{
				if(IsConnected(PCGUI_OASIS_CONNECTED) )
					{
					SendGotoPageCommand("Meters");  // hey! cGotoPage overlaps with cCHANNEL_SHORT!!
					SendGotoPageCommand("MainMenu");
					}
				SendGotoPageCommand("Home"); 
				}
			return 1;

		case oaKEY_SCENE:  
			if(connected.ARIA)
				{
				if(MsgPressMode != MSG_KEY_PRESS)
					return 0; // dont want two mesgs for each press!

				//cycle between TRK1,TRK2 and Home menus
				if(UIPageCurrent() == &UITrackXMix)
					{ // toggle between TRX1 and TRX2
					if     (Nomad.Settings.CurTrackMixPageNum == 0)	Nomad.Settings.CurTrackMixPageNum = 1;
					else if(Nomad.Settings.CurTrackMixPageNum == 1)	GotoPage_ASYNC(&UIHome1);	// press it twice to go back home
					}
				else
					{ // if we are in any other page then goto Track1 on the TrackXMix page
					Nomad.Settings.CurTrackMixPageNum = 0;
					GotoPage_ASYNC(&UITrackXMix); 
					}

				//if     (UIPageCurrent() == &UITrack1MixEnables)	GotoPage_ASYNC(&UITrack2MixEnables); 
				//else if(UIPageCurrent() == &UITrack2MixEnables)	GotoPage_ASYNC(&UIHome1);	// press it twice to go back home
				//else 											GotoPage_ASYNC(&UITrack1MixEnables); 
				return 1;
				}
			if(!PCGUIconnected()) SendKeyMsg(MsgPressMode, KEY_PC_F7);   
			else				  SendGotoPageCommand("SceneTake");
			return 1;

		case oaKEY_TAKE:   	
			if(connected.ARIA)
				{
				if(MsgPressMode != MSG_KEY_PRESS)
					return 0; // dont want two mesgs for each press!
				if(UIPageCurrent() != &UIRecordEnables)	
			 		GotoPage_ASYNC(&UIRecordEnables); 
				else
					GotoPage_ASYNC(&UIHome1);	// press it twice to go back home
				return 1;
				}
			if(!PCGUIconnected()) SendKeyMsg(MsgPressMode, KEY_PC_F8);   
			else				  SendGotoPageCommand("SceneTake");
			return 1;

		case oaKEY_NOTE:   

			if(connected.ARIA)
				{
				if(IsHoldRelease) // hold NOTE key to go BACK on ARIA
				 	{
				 	GotoPage_ASYNC(&UIHome1); //SendKeyMsg(MSG_KEY_PRESS, KEY_HOME);	
				 	//PFLClearAll(1);   
				 	return 1; 
				 	}
				if(MsgPressMode == MSG_KEY_RELEASE)
					{ // ignore presses now
					if(UIPageCurrent() != &UIExtFaderAssignList)	
				 		GotoPage_ASYNC(&UIExtFaderAssignList); //SendKeyMsg(MSG_KEY_PRESS, KEY_HOME);	
					else
						GotoPage_ASYNC(&UIHome1);	// press it twice to go back home
					}
				return 1;
				}
			
			if(!PCGUIconnected()) SendKeyMsg(MSG_KEY_PRESS, KEY_PC_F9);   
			else				  SendGotoPageCommand("Note");
			return 1;

		case oaKEY_REREC: 
			if(shift)  SendKeyMsg(MsgPressMode, KEY_PAGEUP);   
			else       GotoPage_ASYNC(&UIPlayControl);
			return 1; // next home page
		case oaKEY_AUD:   
			if(shift)  SendKeyMsg(MsgPressMode, KEY_PAGEDOWN); 
			else       GotoPage_ASYNC(&UIPlayControl);
			return 1; // prev home page
		case oaKEY_SLATE:
    	    ActivateSlate(MsgPressMode == MSG_KEY_PRESS); 
			if(MsgPressMode == MSG_KEY_PRESS)	gKBD.Mods |=   KEYMODIFIER_SLTCODE; // kludge to keep USB key mods from turing off slate!! see USBkeyboard and tasks.cpp
			else 								gKBD.Mods &= (~KEYMODIFIER_SLTCODE);
			RestoreMainMixes();
			return 1;
		case oaKEY_TALK1:
        	ActivateCom1(MsgPressMode == MSG_KEY_PRESS); 
			if(MsgPressMode == MSG_KEY_PRESS)	gKBD.Mods |=   KEYMODIFIER_COMCODE; // kludge to keep USB key mods from turing off slate!! see USBkeyboard and tasks.cpp
			else 								gKBD.Mods &= (~KEYMODIFIER_COMCODE);
			RestoreMainMixes();
			return 1;
		case oaKEY_TALK2:  // use shift TLK2 as a ---TONE--- key for now
      		gNV.fToneOn = ((MsgPressMode == MSG_KEY_PRESS) && shift); 
      		if(gNV.fToneOn) ToneOn(); 
			else			ToneOff(); 
	  		SetToneMix(gNV.fToneOn);
			return 1;

		//case oaKEY_AUD:  // MIX16 keys...
		case oaKEY_MARK:
		//case oaKEY_REREC:
			GotoPage_ASYNC(&UIPlayControl);
			return 1;

#ifndef RX4
		case oaKEY_AUX:  // move to other bus pages by emulating the bus key
			SendKeyMsg(MsgPressMode, KEY_BUS);
			return 1;
#endif
		default: 
			break;
		}	


		// strip keys start at ch 8 on an Oasis!!
		if(connected.ARIA || (GetNumOasisFaders()>8) )	
			digit = keycode & 0x0f; // StripMask=0x0F; // up to 16 channels
		else
			digit = keycode & 0x07;

		if ((MsgPressMode == MSG_KEY_RELEASE) && InRange(keycode, STRIP_KEY_BUS, STRIP_KEY_BUS+15)  ) // on nomad this should be limited to 0x57 right?
			{ // choose output bus mix pages
			//digit = (keycode - STRIP_KEY_BUS) & StripMask;
			int ch = GetAudioChannelFromFaderAssign(digit+(EXTERNAL_FADER_1)); 	
			if( InRange(ch, 0, MAX_IN_CHAN_ANALOG_DIGITAL-1) )
				Nomad.Setup.Chan = ch; //set nomad's input page if it is reasonable

			if( PCGUIconnected() ) 
				{ //for nova: a1 = 0, d1=16, rx1=24     for PC: a1=0, d1=6
int chpc = ch;
if(ch >= 16) chpc -= 10; // InRange(ch,16,23)) ch -= 10; 
				SendSetting_Byte(SETTING_INPUT_CHANNEL, chpc); // set input number on PC
				SendGotoPageCommand("SourceRouting"); // goto proper Page on PC  // should we send SETTING_INPUT_CONFIGURE too???
				}
			else
				{// no PC so make Nomad goto the appropriate OUTPUT BUS ASSIGN page
				UI_PAGE *page = UIPageCurrent();
				if(page==&UIBusAssign14Analog)	{GotoPage_ASYNC(&UIBusAssign14Digital);	return 1;}
				if(page==&UIBusAssign14Digital)	{GotoPage_ASYNC(&UIBusAssign14RX);		return 1;}
				if(page==&UIBusAssign14RX)		{GotoPage_ASYNC(&UIBusAssign5to8); UI_SetCurrentOutputBus5to8enum(0); return 1;}
				if(page==&UIBusAssign5to8)				
					{
					int ee = UI_GetCurrentOutputBus5to8enum()+1; // choos next output page (5,6,7,8) is enum 0,1,2,3
					if( ee < 4 ) // UI_GetCurrentOutputBus5to8enum()==0) )
						{
						UI_SetCurrentOutputBus5to8enum(ee);  // choose next output page 5,6,7,8
						GotoPage_ASYNC(&UIBusAssign5to8);	 // refresh the page
						}
					else
						GotoPage_ASYNC(&UIBusAssign14Analog);	// we are on out6 so go to next page in list
					return 1;
					}
				// first time we are entering one of these pages...
				if(Nomad.Setup.Chan < IN_CHAN_DIGITAL_RX_1)	{GotoPage_ASYNC(&UIBusAssign14Analog); 	return 1;}
				else										{GotoPage_ASYNC(&UIBusAssign14RX);		return 1;}
				}
			return 1;
			}

		if ((MsgPressMode == MSG_KEY_RELEASE) && InRange(keycode, STRIP_KEY_STAR, STRIP_KEY_STAR+15)  ) // on nomad this should be limited to 0x57 right?
			{ // choose card mix page
			//digit = (keycode - STRIP_KEY_STAR) & StripMask;
			int ch = GetAudioChannelFromFaderAssign(digit+(EXTERNAL_FADER_1)); 	
			if( InRange(ch, 0, MAX_IN_CHAN_ANALOG_DIGITAL-1) )
				Nomad.Setup.Chan = ch; //set nomad's input page if it is reasonable

			// no PC so make Nomad goto the appropriate CARD MIX BUS ASSIGN page
			UI_PAGE *page = UIPageCurrent(); // &UIBusAssign;
			if(page==&UIBusCardMix14Output)	{GotoPage_ASYNC(&UIBusCardMix14);		return 1;}
			if(page==&UIBusCardMix14)		{GotoPage_ASYNC(&UIBusCardMix14Digital);return 1;}
			if(page==&UIBusCardMix14Digital){GotoPage_ASYNC(&UIBusCardMix14RX);		return 1;}
			if(page==&UIBusCardMix14RX)		{GotoPage_ASYNC(&UIBusCardMix14Output);	return 1;}
			// first time we are entering one of these pages...
			if(Nomad.Setup.Chan < IN_CHAN_DIGITAL_RX_1)	{GotoPage_ASYNC(&UIBusCardMix14); 	return 1;}
			else										{GotoPage_ASYNC(&UIBusCardMix14RX);	return 1;}
			return 1; // this is unreachable
			}


		 // on nomad this should be limited to 0x57 right?
		if ( (MsgPressMode == MSG_KEY_RELEASE) &&  InRange(keycode, STRIP_KEY_CH, STRIP_KEY_CH+15) )
			{ 
			//digit = (keycode - STRIP_KEY_CH) & StripMask;
			int ch = GetAudioChannelFromFaderAssign(digit+(EXTERNAL_FADER_1)); 
			UI_PAGE *page = UIPageCurrent(); // &UIBusAssign;
			if(connected.ARIA)
				{
				if(page==&UIExtFaderAssignList)	
					{
					UIControlSetAbsolute_ASYNC(digit, false);
					return 1;
					}
				else if(page==&UIRecordEnables)	
					{
					int trk = FindCardTrackFromFaderNum(digit+EXTERNAL_FADER_1);
					if(trk>=0)
						UIControlSetAbsolute_ASYNC(trk, true); // IsHoldRelease);
					return 1;
					}
				else if( (page==&UITrackXMix) ) //  || (page==&UITrack2MixEnables) )	
					{
					volatile int moveto = -1;
					if(       InRange(ch, IN_CHAN_DIGITAL_RX_1, IN_CHAN_DIGITAL_RX_8) )	moveto = ch - IN_CHAN_DIGITAL_RX_1;
					else if ( InRange(ch, IN_CHAN_ANALOG_1,     IN_CHAN_ANALOG_4)     )	moveto = ch - IN_CHAN_ANALOG_1  + 8;
					else if ( InRange(ch, IN_CHAN_RETURN_1,     IN_CHAN_RETURN_2)     )	moveto = ch - IN_CHAN_RETURN_1  + 8 + 4;
					else if ( InRange(ch, IN_CHAN_DIGITAL_1,    IN_CHAN_DIGITAL_4)    )	moveto = ch - IN_CHAN_DIGITAL_1 + 8 + 4 + 2;
					if(moveto>=0)						
						UIControlSetAbsolute_ASYNC(moveto, true); // IsHoldRelease);
					return 1;
					}
				}//if(connected.ARIA)

			if(page == &UISetupChanX)	
				GotoPage_ASYNC(&UIHome1);	// press CH twice to go back home if in INPUTS pages
			else
				{ // goto INPUTS page and choose the input associated with the current fader
				if( InRange(ch, 0, MAX_IN_CHAN_ANALOG_DIGITAL-1) )
					Nomad.Setup.Chan = ch; //set nomad's input page if it is reasonable
				dprintf("chan=%d ch=%d digit=%d\n", Nomad.Setup.Chan, ch, digit);

int chpc = ch; // for PC only
if(ch >= 16) chpc -= 10; // InRange(ch,16,23)) ch -= 10; 

				SendSetting_Byte(SETTING_INPUT_CHANNEL, chpc); // set input number on PC // should we send SETTING_INPUT_CONFIGURE too???
				SendGotoPageCommand("InputConfigure"); // goto proper Page on PC
				GotoPage_ASYNC(&UISetupChanX);
				}
			return 1;
			}


	// ------------------------ special cases for local action on key-PRESS ONLY ---------------------
	if( (MsgPressMode == MSG_KEY_PRESS) && (keycode != 0x00) )
		{
		//KeyPressTime = GMSECS; // if not a key release code then time-stamp this key press
		//lastkeycode  = keycode;

		if ((keycode >= STRIP_KEY_PFL) && (keycode <= (STRIP_KEY_PFL+15)) ) // on nomad this should be limited to 0x57 right?
			{
			//digit = (keycode - STRIP_KEY_PFL) & StripMask;
			//fnum  = digit+EXTERNAL_FADER_1; // //input = GetAudioChannelFromFaderAssign(digit+(EXTERNAL_FADER_1)); 	
			PFLtoggleChan(digit, true, false);
			return 1; 
			}


		if 		(keycode == oaKEY_REC)		SetTransportMode(RECORD_MODE);
		else if (keycode == oaKEY_PLAY) 	SetTransportMode(PLAY_MODE);
		else if (keycode == oaKEY_STOP)     // KEY_FP8_STOP
			{ 
			if( (GetTransportMode()==STOP_MODE) && (GetNumSegments() > 0) )
			 	; // UIPageGoto(&UIPageFalseStart); this is annoying - maybe do a long key press instead
			else
				SetTransportMode(STOP_MODE);
			}
#ifndef RX4
		else if((keycode == oaKEY_PLUS)  && shift) TransportSetState(TRANSPORT_TONE);      // no release needed for this 
		else if(keycode == oaKEY_REREC)     IncDecAutoPan(1);  // NEXT AUTO PAN
		else if(keycode == oaKEY_AUD)       IncDecAutoPan(-1); // PREV AUTO PAN
#endif
		else if(keycode == oaKEY_HP1)       SetHPmem(0);
		else if(keycode == oaKEY_HP2)       SetHPmem(1);
		else if(keycode == oaKEY_HP3)       SetHPmem(2);
		//else if(keycode==oaKEY_BACK)		SendGotoPageCommand("Home");
		//special case where we want to send a setting to Nomad AND PC
		else if(GetBankKey(keycode) >= 0)
			{ 
			SetNewExtBank( GetBankKey(keycode) );
	        NVIsDirty(DEVAINI); // store the change in gNV.ExtBank
			//OnSetting_ExtBank(NULL, &msg);
			}
		else
			comprintf("UnknownKey = %04X\n", (int)keycode);	//............All other keys...................
		}//KeyPress


	// --- special local action for ker releas ONLY ---
	if (MsgPressMode == MSG_KEY_RELEASE) // (keycode == 0xFF) // ------------ KEY RELEASE (not used any more) -------------------------------------------------------
		{ 
		; // take no action for now......  
		//keycode = lastkeycode;
		//SendKeyMsg(MSG_KEY_RELEASE, keycode); // lastkeycode);
		}

	return 1;
}

int GetBankKey(int keycode) // returns -1 if not a bank key
{
	int bank = -1;
	if(keycode==oaKEY_BANK1) {bank=0;}
	if(keycode==oaKEY_BANK2) {bank=1;}
	if(keycode==oaKEY_BANK3) {bank=2;}
	if(keycode==oaKEY_BANK4) {bank=3;}
	if(keycode==oaKEY_BANK5) {bank=4;}
	return bank;
}

void ExtFaderSetup();

void SetNewExtBank(int bank)
{
	int i,j,input;
	uchar buf[30];

	if(bank>4) bank=4;
	// SetBankLED(bank); // updated in background
	gExtBankStates.CurrentBank = gNV.ExtBank = bank;
	gExtBankStates.BankChangeTime  = GMSECS;
	gExtBankStates.NudgeFaderTimer = GMSECS+EXT_BANK_CHANGE_TIME_MS+300;
	j=0;
	buf[j++] = bank;
	int count = 8;
	if(IsConnected(DEVA32_CONNECTED))
		count=12; 
	for(i=0;i<count;i++)
		{
		input = GetAudioChannelFromFaderAssign(i+(EXTERNAL_FADER_1)); // ADDED -1 IN 6.34
		if(input>=0)
			buf[j++] = gChannelConfig[input].Fader_pos255;
		else
			buf[j++]=0;
		}
	for(;i<16;i++)
		buf[j++]=55; // indicate invalid fader values

	if(IsConnected(OASIS_CONNECTED|DEVA32_CONNECTED))
		{
		send_oasis_cmd(cGotoBank, j, buf);
		dprintf("Send cGotoBank cmd bank+1=%d\n", bank+1);
		}
	ExtFaderSetup();
	return;
}

int process_Oasis_encoder(struct ZAXLAN_CMD_STRUCT *z) 
{              
	uint16 encoderNum = z->bytes[0]; int value=z->bytes[1];
	if (encoderNum != 3) // always encoder number 3 from MIX12 days
          return 1;
     value -= 0x80;
     //value &= 0xFF;    
     ForceEncoderTicks(value);//  gEncoderCount += value; // just force it in there ???
	 return 1;
}

void HandleExtFaderCOMMON(int extfader, int pos255, bool IsFP7); // extfader always goes down to 0 here
void HandleExtTrimKnobCOMMON(int TrimKnobNum, int val255, int trim2030);

int process_Oasis_fader(struct ZAXLAN_CMD_STRUCT *z) 
{
	uint16 fader = z->bytes[0]; uchar value = z->bytes[1];
	int i,j,channel, index, chmap; // delta,
	float tmpf,  pot1, pot2, DB1, DB2;
	//If bank changed the ignore fader positions, but if the bank change was due to 
	//an Oasis being connected then allow the faders to shie thru to make sure they init properly
//	if( ((GMSECS-BankStates.BankChangeTime) < BANK_CHANGE_TIME_MS)  )// ignore moving faders due to bank changes
//		{
//		if( (GMSECS-GetOasisConnectTime()) > BANK_CHANGE_TIME_MS) // Oasis conected a long time ago?
//			return 1; // yes, ignore fader movement
//		}
	if(fader>15)
		return 1; // ERROR????

	if(connected.ARIA)
		{ // allow up to 16 faders if Aria is connected
		if(fader>15) 
			return 0;
		}
	else
		{
		if( GetNumOasisFaders() == 16)
			{ // swap the fader indexes of a deva16 panel - (Oasis 1.52 has fader number in version string)
			if(fader>7) fader -= 8;
			else 		fader += 8;
			}
//		if(fader>7) // cant handle 16 faders yet....  removed this in Nova v4.49
//			return 0;
		}

	bool IsFP7 = (z->cmd == cID_Fader);
	HandleExtFaderCOMMON(fader, value, IsFP7); // connected.FP7);
	//gRTCram.ExtFaderPos[fader] = value; // instantly store fader positions for current bank (only used at boot!)
	return 1;
}


void process_Oasis_trims(uint16 fader, int val255) // struct ZAXLAN_CMD_STRUCT *z) 
{
    int trim2030 = MIN_ANALOG_INPUT_TRIM + (val255*(MAX_ANALOG_INPUT_TRIM-MIN_ANALOG_INPUT_TRIM))/255; // scale -20 to +30
	SetTrimFromExternalFader(fader+(EXTERNAL_FADER_1), val255, trim2030); // may do nothing if appropriate
}
void process_Oasis_trims(struct ZAXLAN_CMD_STRUCT *z) 
{
	process_Oasis_trims(z->bytes[0], z->bytes[1]);
}

// this handles EXT fader commands from all mixing surfaces including FP7, aria, mix12,mix8 and oasis
void HandleExtFaderCOMMON(int extfader, int pos255, bool IsFP7) // extfader always goes down to 0 here
{
	int j,channel, index, chmap; // delta,
	static short InitMe=1;
	static ALARM FP7FaderTimes[NUM_TOTAL_FADERS]; // when last fader packet arrived
	if(InitMe)
		{
		for(int i=0;i<numberof(FP7FaderTimes);i++)
			FP7FaderTimes[i] = 0;
		InitMe=0;
		}

	int SystemFaderNum = extfader + EXTERNAL_FADER_1;	
	if(IsFP7)
		{// ............. round faders ...............
		if(extfader >= 7)
			return; // error!
		SystemFaderNum = extfader + FP7_FADER_1;	
		struct FADER_ASS *p = GetFaderAssPtr(SystemFaderNum); // &FaderAssignList[FaderNum]; // short hand
		static int prev[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
		if(abs(prev[extfader]-pos255) <= 1)
			{
			//dprintf("ign fad[%d]=%d\n",extfader,pos255); 
			return; // ignore minor changes since FP7 spits out too many codes
			}
		prev[extfader] = pos255;
		// detect movement for auto trim on FP8 only
		if( ((GMSECS-FP7FaderTimes[extfader]) < 2000) && (FP7FaderTimes[extfader] != 0) ) // detect more than one fader packet per second so we ignore the 10 second update problem
			ForceSpecialTrimChannel(SystemFaderNum); //  this call just feeds gActiveFaderNum for knob LEDs
		FP7FaderTimes[extfader] = GMSECS;

		//process_Oasis_trims(extfader, pos255); // treat this like a trim knob on Oasis
		if ( (p->type==FADERTYPE_TRIM) || (p->type==FADERTYPE_ZNET) )
			{ // this will look at fader assign and do znet or trim appropriately 
		    int trim2030 = MIN_ANALOG_INPUT_TRIM + (pos255*(MAX_ANALOG_INPUT_TRIM-MIN_ANALOG_INPUT_TRIM))/255; // scale -20 to +30
			SetTrimFromExternalFader(SystemFaderNum, pos255, trim2030); // may do nothing if appropriate
			return;
			}
		// else if(p->type==FADERTYPE_ZNET ) fall thru to oasis handler using IsRoundFader==true
		}//connected.fp7

	// treat as a regular fader
	SetFadersLog(SystemFaderNum, pos255, IsFP7); // IsRoundFader
	#if 0
	// why was this changing FaderTrim[]???
	// we should get rid of FaderTrim[] and replace it with GetFaderTrim2030(int fader){return gChannelConfig[(fader to input)].Trim_Pos255);
	// support for storing trim info when an EXT fader assign has a T in it 6.26
	float temp, pot1, pot2, DB1, DB2;
    temp = pos255;
	temp /= 256.0; // scale from 0 to 1.0
    pot1=0; pot2=1.0; DB1=-26; DB2=+35;
    temp = CalcLineEquation(pot1, pot2, DB1, DB2, temp); // note FaderTrim[] has a HP entry at the beginning!!
	FaderTrim[SystemFaderNum] = clip(temp, -20.0F, 30.0F); // size is MAX_IN_CHAN_SOURCES
	#endif
}

/*
void HandleExtTrimKnobCOMMON(int fader, int val255, int trim2030)
{
	// ..........................Trims Knobs only.................................
	int i = TrimKnobNum; //  - NUM_EXTERNAL_FADERS; 
    //int trim2030 = MIN_ANALOG_INPUT_TRIM + (value*(MAX_ANALOG_INPUT_TRIM-MIN_ANALOG_INPUT_TRIM))/255; // scale -20 to +30
	// what audio ch is assigned to the slide fader below this trim knob?
	SetTrimFromExternalFader(fader+(EXTERNAL_FADER_1), fader, val255, trim2030); // may do nothing if appropriate
	return;
}
*/
uint16 gOasisHpKnobVal=0; // // OutKnob[8] is the external Headphone knob!! HP
int process_Oasis_outknobs(struct ZAXLAN_CMD_STRUCT *z) 
{
	float tmpf, db;
	uint16 fader = z->bytes[0]; 
	int raw255 = z->bytes[1];
	int i,j,val255; // delta,

	if(fader==8)
		gOasisHpKnobVal = raw255; // see ReadFrontADCs() in N_SCI_PORT.cpp for where this is handled

	if(fader>7)
		return 0;  // dont go beyond OUTPUT_FADER_8 

	// ----------- Mix12 OUTPUT KNOBS (fader=24,25,26,27) ---------
	i = fader;
	// 1st update is bogus
	// 0 - -96
	
	float clipval=222;
	val255 = raw255; //  + 30;
	if (val255 > clipval) // 203
		val255 = clipval;

//	val255 = raw255 + 30;
//	if (val255 > 255) // 203
//		val255 = 255;
//	else if (raw255 <= 10)
//		val255 = 0;
//	db = mix12fadertbl[val255] / 12.0F; // maxx out at 0 dB
//	tmpf = dBToLinear(db);


	tmpf = powf(10.0, (-44.0*(clipval - val255)/clipval) / 20.0 );
	if(raw255 < 10) 	//if(tmpf < 0.00001)
		tmpf = 0; // mute completely
	//FadersLog[OUTPUT_FADER_1+i] = tmpf;	
	OutputFadersLog[i] = tmpf;	
	//SetFadersLog(i, tmpf); // these are not yet assignable in fader assign I dont think
	return 1;
}


void SendByteCommand         (int16 cmdcode, int16 group, int16 unit, int32 parm8);
void SendTwo16bitWordsCommand(int16 cmdcode, int16 group, int16 unit, uint16 parm16a, uint16 parm16b);
void ReqestToSendFreq(int freq, int unit);

// I guess this is comming from an RX12
int process_QRXTX_RX12(struct ZAXLAN_CMD_STRUCT *z) 
{
	int i; // ,findex;
	uint16 w16a,w16b, unit, cmd;
	volatile float tmpf;
	uchar *buf = z->bytes;

	// embdded ubcmd from RX12 to be sent directly to NOPT module
	//if( (buf[2+0]!=0xAA) || (buf[2+5]!=0x50) ) 
		//	return -1; // double check some constant bytes

		cmd  = buf[0];
		//group= buf[2];
		unit = buf[2];
		w16a  = buf[3];
		w16a <<= 8;
		w16a |= buf[4];
		w16b  = buf[5];
		w16b <<= 8;
		w16b |= buf[6];

        //findex = unit-1; // map unit code to the list of stored freqs
		if( (cmd==UBCMD_CHANNEL) ) // && IsGoodIndex(findex, numberof(gChannelConfig)) ) // (Jun 24 2025: fixed limitation of unit ha d to be less than 17)
			{
			ReqestToSendFreq(w16a, unit);
			/// update the freq in the INPUTS menu
			for(i=0;i<numberof(gChannelConfig);i++) 
				{
				if(gChannelConfig[i].TxID == unit )
					gChannelConfig[i].ZNetFreq = w16a; 
				}
			//	gSendFreqCount++;
			}
		else if(cmd==UBCMD_GAIN) // #define UBCMD_GAIN (0x81) // comes in as gain in 1/4 dB steps 
			{ // convert to 0 to 255 scaling
	        SendTwo16bitWordsCommand(UBCMD_GAIN, gNV.IFB_group, unit, w16a, 0x5566); //  chaned in Nova_V*351 away from 255/52); // 0-52
			}
		else
			SendByteCommand(cmd, gNV.IFB_group, unit, w16a); // pass thru for unknown commands

		//  chaned in Nova_V*351 (send both workds since in a gain cmd there is a 0x5566 code that tells txer how to scale the gain stting
		//SendTwo16bitWordsCommand(cmd, gNV.IFB_group, unit, w16a, 0x5566); // pass thru for unknown commands

		dprintf(cMAGENTA, "RX12: sent TRX (IFBgroup=%d) unit(%d) w16=%04X,%04X (%d)\n", gNV.IFB_group, unit, (int)w16a, (int)w16b, (int)w16a);
		//if( !RX12connected() )     // gRX12_Ver[0]==0)
		//	RX12connected("RX12-0.00"); // (char *)&buf[2]); // store a fake version string

     	return 1;
}

// send communications error string to PC
void SendErrStringCmd(char *str)
{
	int j=0;
	comprintf("SendErrString,%s>\n", str);
	j = strlen(str);
	send_oasis_cmd(cErrString, j, (uchar *)str);
	return;
}

void SendOasisPopupCmd(char *s0, char *s1, char *s2, char *s3, uint16 timeout)
{
	int i,j=0;
	uchar buf[99];
	if( !PCGUIconnected() )  // TABLET_CONNECTED|DEVA32_CONNECTED|OASIS_CONNECTED) )
		return;

	U16toBytes(&buf[j], timeout); j+=2;
	if(s0){              for(i=0;i<strlen(s0);i++) buf[j++] = s0[i]; }
	if(s1){buf[j++]=' '; for(i=0;i<strlen(s1);i++) buf[j++] = s1[i]; }
	if(s2){buf[j++]=' '; for(i=0;i<strlen(s2);i++) buf[j++] = s2[i]; }
	if(s3){buf[j++]=' '; for(i=0;i<strlen(s3);i++) buf[j++] = s3[i]; }
	buf[j++]=0;
	//for(i=0;i<j;i++) dprintf("%02X ", buf[i]); dprintf("\n");
	send_oasis_cmd(cPopup, j, buf);
}
void SendOasisPopupCmd(char *str, uint16 timeout)
{
	SendOasisPopupCmd(str, NULL,NULL,NULL, timeout);
}

// fill a MATRIX_CELL struct with data from a byte buffer
void BytesToCell(MATRIX_CELL *cell, uchar *p)
{
	int j=0;
	cell->Pointer = NULL; //  whats this for?
	cell->Row    = p[j++]; 
	cell->Col    = p[j++]; 
	cell->Value  = BytesToU32(&p[j]); // uint32
	j+=4;
	cell->Color  = BytesToU16(&p[j]); // uint16
	return;
}
// fill a buffer with cell data
int CellToBytes(uchar *p, MATRIX_CELL *cell)
{
	int j=0;
	p[j++] = cell->Row;
	p[j++] = cell->Col;
	U32toBytes(&p[j], cell->Value); // uint32
	j += 4;
	U16toBytes(&p[j], cell->Color);   // uint16
	j += 2;
	return j;
}



void SendKeyMsg(MSG_TYPE type, uint16 value)
{
	MESSAGE KeyMesg;     
	KeyMesg.Type = type; //  MSG_KEY_PRESS; // assume key press for below
	KeyMesg.Value.Unsigned =  value;
	SendKeypressMessage(&KeyMesg);
}

// choose only one:
#define BURN_OASIS
#define BURN_ARIA
float gNewMixerVersionFloat = 0.0F; // gOASIS_VERSION_FLOAT; 
//float gCurrentVersionFloat = gARIA_VERSION_FLOAT; 
const char *gCurrentMixerName = "Aria?";

#ifdef BURN_OASIS
//	const char *gCurrentMixerName = "Oasis";
	#include  "Fad-325.h"  // 313.h" 
//	float     gOASIS_VERSION_FLOAT =  3.25; // .13
#endif

#ifdef BURN_ARIA
//	const char *gCurrentMixerName = "Aria";
//	float     gARIA_VERSION_FLOAT     = 0.24;
	#include  "ARR-024.h"
#endif

	#define BYTES_PER_BLOCK 128
short gOasisProgramState=0;
void OasisProgramUpdater()
{ 
	int32 i, j;
	uint16 sum16;
	uchar p[BYTES_PER_BLOCK+8];
	static uint32 Addr=0;
	static ALARM alarm=0;
	static bool IsAria=false; // true if we are programminng an ARIA mixer panel
	if(!CHECK_ALARM(alarm))
		return;
	SET_ALARM(alarm, 20);

	// keep track of current mixer version for display in UPDATE_MIXER page
	if(connected.ARIA) 
		{
		gCurrentMixerName    = "Aria";
		gNewMixerVersionFloat = ARIA_VERSION_FLOAT;
		}
	else if(connected.OASIS)
		{
		gCurrentMixerName    = "Oasis";
		gNewMixerVersionFloat = OASIS_VERSION_FLOAT;
		}

	if(gOasisProgramState==0)
		return;

	int items = ItemsInFifo(&serport[RS422MAIN].tx); // pTX);
	if(items > 100) // allow some meter data in here....
		return;

	static int Bootloader_sz = 0;
	static const char *pBootLoaderProgram = BootLoaderProgram_ARIA;
	static uint32 AddrMagic = 0;

	if( (gOasisProgramState>0) && (gOasisProgramState<5) ) // just starting now
		{
		Addr = 0; // reset address index to first 128 byte block
		IsAria = connected.ARIA; // captrure the current mixer panel model one time
		if(IsAria)
			{
			//#ifdef BURN_OASIS
			//gOasisProgramState=0; // dont try to burn ARIA
			//return;
			//#else
			AddrMagic            = ARIA_MAGIC_BURN_ADDR; // 0xFEFF00
			gCurrentMixerName    = "Aria";
			//gCurrentVersionFloat = ARIA_VERSION_FLOAT;
			Bootloader_sz        = sizeof(BootLoaderProgram_ARIA); 
			pBootLoaderProgram   = BootLoaderProgram_ARIA;
			//#endif
			}
		else
			{
			//#ifdef BURN_ARIA
			//gOasisProgramState=0; // dont try to burn OASIS
			//return;
			//#else
			AddrMagic            = OASIS_ADDR_BURN_TMP_SECT; // 0xFEFF00
			gCurrentMixerName    = "Oasis";
			//gCurrentVersionFloat = OASIS_VERSION_FLOAT;
			Bootloader_sz        = sizeof(BootLoaderProgram_OASIS); 
			pBootLoaderProgram   = BootLoaderProgram_OASIS;
			//#endif
			}

		j=0;
		p[j++] = ((AddrMagic >> 16) & 0xFF);  
		p[j++] = ((AddrMagic >>8 )  & 0xFF);
		p[j++] =  (AddrMagic&0xFF);  
		p[j++] = gOasisProgramState-1; // which tmp rom sector to erase
		if(gOasisProgramState==1)
			dprintf("\n\n\n\nSending Program to %s (version %3.2f)...\n", gCurrentMixerName, gNewMixerVersionFloat);

		if( (gOasisProgramState==1) || (gOasisProgramState==2) )
			{ // we were sending too many sector erase cmds so we were erasing the start of ROM!!! I think
			dprintf("Erase TMP sector <%d>\n", gOasisProgramState-1);
			send_oasis_cmd(cSECTBUF, j, p);
			SET_ALARM(alarm, 1000);
			}
		gOasisProgramState++;
		return;
		}

	if(gOasisProgramState==5)
		{
		j=0; sum16=0;//	tmp = Addr;
		p[j++]  = ((Addr>>16)&0xFF);  // send 24bit byte address 
		p[j++]  = ((Addr>>8)&0xFF); 
		p[j++]  =  (Addr&0xFF);  
		for(i=0;i<BYTES_PER_BLOCK;i++)
			{
			sum16 += (uchar) pBootLoaderProgram[Addr];
			p[j++] = pBootLoaderProgram[Addr];
			Addr++;
			if(Addr>=Bootloader_sz) // sizeof(BootLoaderProgram)) // not evenly divisable by anything since its an AIS file!!
				{
				gOasisProgramState++;
				break;
				}
			}
		p[j++] = ((sum16 >> 8)&0xFF); // 16bit chksum
		p[j++] = (sum16 & 0xFF);
		if((Addr%10)==0)
			dprintf("Sending sector %d of %d\n", (Addr/BYTES_PER_BLOCK), 1+Bootloader_sz/BYTES_PER_BLOCK);
		  //dprintf("Sending sector %d of %d (addr=%06X) sum=%04X\n", (Addr/BYTES_PER_BLOCK), 1+sizeof(BootLoaderProgram)/BYTES_PER_BLOCK, Addr, (int)sum16);
		send_oasis_cmd(cSECTBUF, j, p);
		return;
		}

	if(gOasisProgramState==6) // burn program from tmp ROM to Real ROM
		{
		j=0; sum16=0;
		//tmp = Bootloader_sz; // sizeof(BootLoaderProgram);  // if no paload this will apear to be a request for data!!!!
		p[j++] = 0xff & (Bootloader_sz >> 16);
		p[j++] = 0xff & (Bootloader_sz >> 8);
		p[j++] = 0xff & (Bootloader_sz);
		for(i=0;i<Bootloader_sz;i++)
			sum16 += (uchar)pBootLoaderProgram[i];
		//tmp = sum16;
		p[j++] = 0xff & (sum16 >> 8);
		p[j++] = 0xff & (sum16);
		dprintf("Sending Burn ROM request...(sum=%04X)\n", sum16);
		dprintf("\n  Do NOT turn off %s\n  until LED stops blinking!!\n\n", gCurrentMixerName);
		send_oasis_cmd(cPROG_EOF, j, p);
		SET_ALARM(alarm, 20000);
		gOasisProgramState=7; // we are done on our side
		return;
		}

	if(gOasisProgramState==7) // burn program from tmp ROM to Real ROM
		{
		gOasisProgramState=0;
		SET_ALARM(alarm, 20000);
		}

	return;
}

