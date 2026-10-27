// zaxlan cmd string strings

const char *ZlanCmdStringLUT[] = {
 "cQUICKLEVELS", //   0 
 "cERRVCO", //        1
 "cSPECTRUM", //      2
 "cSUBCH", //         3 // sub channel block for RXA and RXB
 "cCHANNEL", //       4 //  cmd code to change channel ---
 "cIDCODE", //        5 //  cmd code to  change idcode
 "cLNA", //           6 //  cmd code to  change LNA  off/on
 "cTONE", //          7 //  cmd code to  change Tone off/on
 "cSINGLEMODE", //    8 //  cmd code to  change SingleMode off/on
 "cAESMODE", //       9
 "cQRXPROGRAM", //    0x0a // triggers QRXprogram mode and provides status to RX12
 "cSCAN_MODE", //     0x0b
 "cSCAN_RESULTS", //  0x0c
 "cOUTPUTMODE", //    0x0d
 "cSOFTVER", //       0x0e
 "cSayWhat15", //     0x0f
 "cInPeaksMix8", //  0x10
 "cBusPeaks", //     0x11
 "cOutPeaks", //     0x12
 "cBright", //       0x13 // use special LED keycode to change prightness settings
 "cLED", //          0x14 // turn on or off an LED using 16bit mLED_xxx itmask
 "cInit",      //    0x15
 "cTimecode",  //    0x16
 "cTransport", //    0x17
 "cInPeaksPC", //    0x18
 "cInPeaksMix8_COMP", // 0x19
 "cBusPeaks_COMP", // 0x1A
 "cOutPeaks_COMP", // 0x1B
 "cInPeaksPC_COMP", // 0x1C
 "cTimecodeReader", // 0x1D
 "x1e", // 
 "x1f", // 
 "cVersion", //      0x20
 "cEncoder", //      0x21
 "cFader", //        0x22
 "cTrim",  //        0x23
 "oknob", //         0x24 // cOutKnob
 "cKey",      //     0x25
 "cMsgReturnStr",  //0x26 
 "cMsgReturnVal",  //0x27 
 "cErrString",     //0x28 
 "cMsgHandlerCall",//0x29 
 "cTimecodeCard",  //0x2A
 "x2B",		   // 0x2B
 "x2C",		   // 0x2C
 "x2D",		   // 0x2D
 "x2E",		   // 0x2E
 "x2F",        // 0x2F 
 "cSendAll",   // 0x30
 "cUSBkeyCode",// 0x31
 "cUSBkeyCodeFP7",  // 0x32
 "x33",		//0x33
 "x34",		//0x34
 "x35",		//0x35
 "x36",		//0x36
 "x37",		//0x37
 "x38",		//0x38
 "x39",		//0x39
 "x3a",		//0x3a
 "x3b",		//0x3b
 "x3c",		//0x3c
 "x3d",		//0x3d
 "x3e",		//0x3e
 "x3f",		//0x3f
 "x40",		//0x40
 "x41",		//0x41
 "x42",		//0x42
 "x43",		//0x43
 "x44",		//0x44
 "x45",		//0x45
 "x46",		//0x46
 "x47",		//0x47
 "x48",		//0x48
 "x49",		//0x49
 "x4a",		//0x4a
 "x4b",		//0x4b
 "x4c",		//0x4c
 "x4d",		//0x4d
 "x4e",		//0x4e
 "x4f",		//0x4f
 "cQRXTX",		//  0x50
 "cENCAPSE",	//  0x51
 "cCH_SHORT",	//was cGotoPage  0x52   also cCHANNEL_SHORT 0x52 // for IFB200 efficiency  channel16,unitcode8
 "cPopup",		//0x53
 "cRequest",	//0x54  
 "cSetting",	//0x55  
 "cDebug",		//0x56  
 "cEvent",		//0x57  
 "cCommand",   //0x58
 "x59",		//0x59
 "x5a",		//0x5a
 "x5b",		//0x5b
 "x5c",		//0x5c
 "x5d",		//0x5d
 "x5e",		//0x5e
 "x5f",		//0x5f
 "x60",		//0x60
 "x61",		//0x61
 "x62",		//0x62
 "x63",		//0x63
 "cSECTBUF ",	//0x64  
 "cPROG_EOF",	//0x65 
 "cRXformat",	//0x66   cRXFORMAT   for RX12 / QRXplug  [fmtA,fmtB,NUM_FORMATS] 8-1-2016
 "x67",		//0x67
 "x68",		//0x68
 "x69",		//0x69
 "x6a",		//0x6a
 "x6b",		//0x6b
 "x6c",		//0x6c
 "x6d",		//0x6d
 "x6e",		//0x6e
 "cMetersFrontList",	//0x6f
 "cPeaksMetersMain",     //0x70 for FRONTDSP - this requires a list of meters that are currently displayed on screen
 "cPeaksMetersMainComp", //0x71
 "cMetersMainList",      //0x72  list of meters on screen (data comes from metersMain struct)
 "cFrontFdr",	 //0x73 cFrontPanelFaders
 "cStructSetting",		 //0x74
 "cSoloPFL",		//0x75
 "x76",		//0x76
 "cProgramIsAvailable", //  0x77
 "cProgramFileDone",    //  0x78
 "x79", // "cProgramBlockRequest", //  0x79
 "cProgramBlockData",    //         0x7a
 "cDiskStateLEDs",       //         0x7b   disk read/write status sent from RECDSP to FRONTDSP
 "0x7c",        // "cBootUp",       0x7c
 "cDataBlock",  // 0x7d
 "x7e",	    	// 0x7e
};

#include "TypeDefs.h"
//#include "SerialPorts.h" // fro zaxlan.h SPORTENUM enum
//#include "Zaxlan.h"
const char *CSTRUCTstringLUT[] = {
 "About"       ,// 0, 
 "Info"		   ,// 1,
 "PFL"		   ,// 2,
 "Transport"   ,// 3,
 "TimeCode"	   ,// 4,
 "AirMail"	   ,// 5,
 "Mirror"	   ,// 6,
 "HphMon"	   ,// 7,
 "Setup"	   ,// 8,
 "ZNet"		   ,// 9,
 "Metadata"	   ,// 10,	
 "Folder"	   ,// 11,
 "Settings"	   ,// 12,
 "PlayBack"	   ,// 13,
 "TrackMix"	   ,// 14,
 "OutputMix"   ,// 15,
 "ISOTracks"   ,// 16,
 "InputCtl"    ,// 17,
 "FaderAssign" ,// 18,
 "Record"      ,// 19,
 "OutputCtrl"  ,// 20,
 "AutoMix"     ,// 21,
 "SoundReport" ,// 22 
 "SegInfo"     ,  // 23 - this is not part of nomad ... SegInfoStruxt[256]
 "Memory"        ,// 24
 "ExtFaderAssign",// 25
 "RTCfaderPos"   ,// 26 // for storing and recovering fader postions from RTC RAM on RECDSP
};


#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h> // for abs()
#include <string.h> // for memset()
#define numberof(x)     ((int)(sizeof(x)/sizeof((x)[0])))

#include "SerialPorts.h"
#include "ZaxLan.h"
//I think the compiler gave us a few extra chars to align the next variable to 32 bit boundary so we may be OK on 3.13
char ZlanErrString[26]; // was 10 !!!! only needs to hold  "cXXX(0xXX)" which is 11 chars
char *GetZaxLanCmdString(unsigned cmd)
{
	if(cmd == cID_Fader)
		{
		strcpy(ZlanErrString, "cID_Fader");
		return ZlanErrString;
		}
	if(cmd == cID_Key)
		{
		strcpy(ZlanErrString, "cID_Key");
		return ZlanErrString;
		}
	if(cmd == cID_LED)
		{
		strcpy(ZlanErrString, "cID_LED");
		return ZlanErrString;
		}


	if(cmd >= numberof(ZlanCmdStringLUT))
		{
		sprintf(ZlanErrString, "(cUNKcmd=0x%02X)",(int)cmd);
		}
	else if(ZlanCmdStringLUT[cmd][0]=='x')
		sprintf(ZlanErrString, "(cUNKcmd=0x%02X)",(int)cmd);
	else
		return (char *)ZlanCmdStringLUT[cmd]; // success
	return ZlanErrString;
}

char *GetCSTRUCTstring(unsigned cmd)
{
	if(cmd >= numberof(CSTRUCTstringLUT))
		{
		sprintf(ZlanErrString, "(CSTR?=%d)",(int)cmd);
		return ZlanErrString;
		}
	return (char *)CSTRUCTstringLUT[cmd]; // success
}



const char *UBCMDstringLUT_0_5[] = {
	"zero",        // 0
	"SCENE",       // 1
	"TAKE",        // 2 // see Transciever.c
	"REEL",        // 3
	"SEGNUM",      // 4
	"SET_ERX_DLY", // 5 // was UBCMD_FILENAME 5
};

const char *UBCMDstringLUT_0x80[] = {

	"0x80",//          (0x80) 
	"GAIN",//          (0x80+1) // gain in 1/4 dB steps (gain from 0 to 63dB max raw code = 255)
	"CHANNEL",//       (0x80+2) // channel code in DecaHz  5555 => 555.5 MHz or enumerated EU channel 1-14
	"RAMPARMS16",//    (0x80+3) // adjust a RamParms item directly
	"unuRamParms32",// (0x80+4) // adjust a RamParms item directly
	"TRANSPORT",//     (0x80+5) // change transport mode
	"CHASE",//         (0x80+6) // playback chase (w/ TC offset and UBits)
	"IFBMIX",//        (0x80+7) // IFB MIX gain setting via MIX8 and IFB00 combo
	"POWERROLL",//     (0x80+8) // NEW: power roll bit from Deva
	"GAIN_INCDEC",//   (0x80+9) // inc or dec gain - added for QOPT - Jan 10 2011 
	"ERASE_ROM",//   (0x80+10) // erase ROM in preparation for program data - also
	"PROGRAM2",//    (0x80+11) // IFB transmits a program file  to ERX using this command (2 bytes in packet)
	"PROGRAM6",//    (0x80+12) // IFB transmits a program file  to ERX using this command (6 bytes in packet)
	"END_PROG",//    (0x80+13) // end of program file - if chksum is ok then trigger a burn of the REAL ROM area on ERX
	"NEW_SECTOR",//  (0x80+14) // start of new sector with sector number -QRX only
	"PROG_FILL32",// (0x80+15) // fill a sector with the same 32 bit word (in words16[0] and words16[1]) (a UBCMD_PROGRAM2 will follow)
	"unuSTBY_TC",//    (0x90) // write directly into PARMS area (16bit word)
	"SET_IFB_MODE",//  (0x91) // TX or RX mode
	"SET_IFB_FREQ",//  (0x92) // freq in MHz
	"SET_TXPWR",//     (0x93) // 
	"SET_TC",//        (0x94) // 
	"unuSET_UB",//      (0x95) // 
	"SET_TC_RATE",//    (0x96) // 
	"SET_TC_JMODE",//   (0x97) // TCJAM_AUTOJAM=2 TCJAM_AUTOLOAD=3
	"SET_REMOTE_PWR",// 0x98 // 
	"SET_IDCODE",//     0x99 // 
	"SET_DBGMODE",//    0x9A // test tone = 1
	"G_IDECold",//      0x9B // inc or dec gain te old way (using TC block)
	"SET_LOWPOWER",//   0x9C // hmm problems with this concept... attempt for allowing one TXer at a time to go into low power mode
	"unu_SET_EDLY",//   0x9D // SET ERX's Audio Delay setting remotely (units are milliseconds here)
	"UNU1_NOPT",//      0x9E // unused: nopt v 0.38 can send this
	"SET_TXNAME",//      0x9F // unused: nopt v 0.38 can send this
	

	// see zaxNetUHF.h these send structs out that are designed to fit in the UHF subch block area
	// we can use cQRXTX to send these over serial ports via zaxlan
	"TRX_CFG",		//  0xA0 
	"URX_CFG",		//  0xA1 
	"SYS_CFG_A",	//  0xA2
	"SYS_CFG_B",	//  0xA3
	"URX_PRESET",	//  0xA4
	"CL_FREQ_CHANGING",// 0xA5
	"UNU_A6",
	"UNU_A7",
	"UNU_A8",
	"UNU_A9",
	"UNU_AA",
	"UNU_AB",
	"UNU_AC",
	"UNU_AD",
	"UNU_AE",
	"UNU_AF",

	// back to regular 2.4GHZ zaxnet commnads now...
	"SETFWD_IDCODE",// 0xB0 // CL:388D: Jun 27 2023  set and forward ecryption code (two 16 bit words, 12 bits each)
	"SET_LOC_GRP",  // 0xB1 // CL:388E: Jun 30 2023  set local RamParms.group code on camrea link
	"SET_VHFchKHZ", // 0xB2 // 32bit freq for VTX board
	"SET_VHFMODE",  // 0xB3 // modulation mode for VTX board OFF,FM,ZHF06
	"SET_VHFtxPWR", // 0xB4 // VHF TX power setting in mW
};

char UBCMDstrTmp[20];
const char *GetUBCMDstring(unsigned cmd)
{
	if(cmd < numberof(UBCMDstringLUT_0_5))
		return (const char *)UBCMDstringLUT_0_5[cmd];

	unsigned c2 = cmd-0x80;
	if(c2 < numberof(UBCMDstringLUT_0x80) )
		return (const char *)UBCMDstringLUT_0x80[c2];

	sprintf(UBCMDstrTmp,"UCMD_0x%02X", cmd);
	return (const char *)UBCMDstrTmp;
}


