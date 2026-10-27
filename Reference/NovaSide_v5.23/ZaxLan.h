#pragma once

// ZaxLan command codes                  
#define  cQUICKLEVELS 0 
#define  cERRVCO    1
#define  cSPECTRUM  2
#define  cSUBCH     3 // sub channel block for RXA and RXB
#define  cCHANNEL   4 //  cmd code to change channel ---
#define  cIDCODE    5 //  cmd code to  change idcode
#define  cMODEBYTES 6 //  was LNA  now chanes misc modes - can make rxer auto send quicklevels
#define  cTONE      7 //  cmd code to  change Tone off/on
#define cSINGLEMODE 8 //  cmd code to  change SingleMode off/on
#define cAESMODE    9
#define cQRXPROGRAM   0x0a // triggers QRXprogram mode and provides status to RX12
#define cSCAN_MODE    0x0b
#define cSCAN_RESULTS 0x0c
#define cOUTPUTMODE   0x0d
#define cSOFTVER      0x0e
#define cDEBUGBITS    15 // (for QRX module)    #define cSayWhat15    0x0f

// Nomad to Oasis
#define cInPeaksMix8 0x10
#define cBusPeaks    0x11
#define cOutPeaks    0x12
#define cBright      0x13 // brightness use special LED keycode to change prightness settings
#define cLED         0x14 // turn on or off an LED using 16bit mLED_xxx itmask
#define cInit        0x15 // Reset LEDs to defaults (mostly off except for STOP key?)
#define cTimecode    0x16
#define cTransport       0x17
#define cInPeaksPC       0x18
#define cInPeaksMix8_COMP 0x19
#define cBusPeaks_COMP    0x1A
#define cOutPeaks_COMP    0x1B
#define cInPeaksPC_COMP   0x1C
#define cTimecodeReader   0x1D
#define cDebugBits        0x1E // tells Oasis not to print debug stuff out its RX12 port 
#define cGotoBank         0x1F // includes bank number and up to 16 meter posistions

#define cVersion     0x20
#define cEncoder     0x21
#define cFaders       0x22
#define cTrims        0x23
#define cOutKnobs     0x24
#define cKey          0x25
#define cMsgReturnStr   0x26 // response to cMsgHandlerCall
#define cMsgReturnVal   0x27 // ""        ""
#define cErrString      0x28 // comm error string to be displaysed somewhere on PC
#define cMsgHandlerCall 0x29 // call a msg handler on nomad directly p[0]=HandlerLUTcode, p[1]=msg.Type, p[2] = msg.Value (uint32 or a string)
#define cTimecodeCard   0x2A 
#define cTimecodeGen    0x2B // added in Aug 29-2018
//0x2C
//0x2D
//0x2E
//0x2F                   // ---- TABLET to OASIS commands ----

 #define cSendAll       0x30 // Request from Oasis or RECDSP to send all settings. I tink Oasis only sends the bank selection.
 #define cUSBkeyCode    0x31 //  new for ARIA USB key codes over zaxlan
 #define cUSBkeyCodeFP7 0x32 //  new for FP7 USB key codes over zaxlan

#define cQRXTX         0x50 // 80 embeddded QOPT command sent via QOPT MCASP packet channel (from RS485 port)
#define cENCAPSULATE   0x51 // encapsulated command in the old sony format
//   cmdbuf[j++]=0xAA;    // 0  hdr
//   cmdbuf[j++]=0;       // 1  chksum
//   cmdbuf[j++]=0x05;    // 2  Blength = x bytes of payload data (Blength==0 for requests)
//   cmdbuf[j++]=0x00;    // 3  src: 
//   cmdbuf[j++]=0x00;    // 4  dest:
//   cmdbuf[j++]=cQRXTX;  // 5  0x50; // Encapsulated UBcmd block
//   -------payload starts here--------------
//   cmdbuf[j++]=UBcmdcode;            // 6   UB cmd code
//   cmdbuf[j++]=(GroupUnit>>8)&0xFF;  // 7   group code
//   cmdbuf[j++]=GroupUnit&0xFF;       // 8   unit code
//   cmdbuf[j++]=(w16>>8)&0xFF;        // 9   16bit data msb
//   cmdbuf[j++]= w16&0xFF;            // 10  16bit data lsb

#define cCHANNEL_SHORT 0x52 // HEY!!overlaps with cGotoPage     for IFB200 efficiency  channel16,unitcode8
#define cGotoPage  0x52

 #define cPopup     0x53
 #define cRequest   0x54    // was 0x50
 #define cSetting   0x55    // was 0x51
 #define cDebug     0x56    // was 0x54
 #define cEvent     0x57    // GUI button press for TC jam buttons
 #define cCommand   0x58    // string command sent to GUI such as "mp3.record"

#define cUHFzaxlan  0x59 // Jun 30 2020: encapsulated UHF cmd carrying a UBCMD. See TRX_CFG_BLK in c:\mic\trx900\common\zaxlan.h



#define  ADDR_BURN_TMPROM_REQ  0xFFFFF0 // special addr to tell board to erase its TMP ROM
// we dont need a special magic addr for IFB cuz IFB baords look at the prog file name to determine if it is for them
//#define  IFB_MAGIC_BURN_ADDR      0xFFFFF0 // magic num for profgramming an IFB board
//#define  VTX_MAGIC_BURN_ADDR      0xFEF100 // 24bit!!! magic number for VTX IFB board

#define  OASIS_ADDR_BURN_TMP_SECT 0xFFFFF0 // magic num for profgramming an OASIS or MIX16
#define  ARIA_MAGIC_BURN_ADDR     0xFEFF00 // 24bit!!! magic number addr for ARRIA: erases sectors to start the burning process
#define  cSECTBUF   0x64   // for future programming of the RX12
#define  cPROG_EOF  0x65 

#define  cRXFORMAT  0x66 // for RX12 / QRXplug  [fmtA,fmtB,NUM_FORMATS]  8-1-2016
#define  cSetBaud   0x67 // set baud rate (p[0],p[1] are baud rate and p[2] is bitmask for which port to set  baud rate for (0==none)

#define  cCAPABILITIES 0x68  // NOV 15 2019  allows RX12 to get capabilities from module such as MinFreq and MaxFreq
#define  cGNVDATA   0x69 // allows a PC to read a module's gNV struct (MRX/c6747)
#define  cUNUrx12_9 cGNVDATA
#define  cPICObytes 0x6a // comms between main DSP and an RP2040 slave CPU
#define  cUNUrx12_a cPICObytes // alias for cPICObytes
#define  cUNUrx12_b 0x6b 
#define  cUNUrx12_c 0x6c 
#define  cUNUrx12_d 0x6d 
#define  cUNUrx12_e 0x6e 
				 
// ----------------------  WARNING ---------------------------------------------------------------------------------
// if we need more RX12 commands put them here (less than 0x6f)
// since blength destroys sourcerx field in modern commands
                                                                                                        
// ------------ in oasis any cmd above this line will have a byte-long blength ----------------
// ------------ in oasis any cmd above this line will have a byte-long blength ----------------
// ------------ in oasis any cmd above this line will have a byte-long blength ----------------
// ------------ in oasis any cmd above this line will have a byte-long blength ----------------
#define  IS_BLEN16_CMD(x) (x>=0x6f)
// ------------ any command below this line must have a 16bit blength (sourceRX is MSbyte of length) --------------
// all new commands that are not for Rx12 should proll be below this line


// DEVA24 related commands....
#define  cMetersFrontList     0x6f // FRONTDSP: 1st byte=nMeters, then byte pairs follow(busType,channelNumber)
#define  cPeaksMetersMain     0x70 // for PC GUI - list of meters that are currently displayed on screen (assumes nomad Bus[] scheme?)
#define  cPeaksMetersMainComp 0x71 //   compressor peaks for abover meters
#define  cMetersMainList      0x72 // list of meters on screen (data comes from MetersMain struct) 
#define  cFrontPanelFaders    0x73 // faders from front panel of DEVA24 F1-F12,aux1,aux2,hp 

#define  cStructSetting       0x74 // portions of Nomad struct are sent and a string indicating struct part is prefixed to data
#define  cSoloPFL             0x75 // solo or PFL bit masks (two uint32)   
//#define  cStateValueMsg       0x76 // MESSAGE struct which includes Msg.Type.Value and a Msg.Offset which points into the Nomad struct indicateing that value needs to be updated

#define   cProgramIsAvailable  0x77
#define   cProgramFileDone     0x78
//#define cProgramBlockRequest 0x79
#define   cProgramBlockData    0x7a
#define   cDiskStateLEDs       0x7b // disk read/write status sent from RECDSP to FRONTDSP
//#define cBootUp              0x7c // indicates someone just booted up and is ready for init
#define   cDataBlock           0x7d // data block for sending a file - 1st 4 bytes are the buffer offset - if offset == -1 then the data block is file name + file size
#define   cMsecTimeSync        0x7e // 32 bit GMSECs value from RECDSP for use in conjunction with DSP_CHUNK pulse for accurate GMSECS sync between DSPs which can be used for a reference when sending TC commands (see ZLAN_TC.OptionalRawGMSECS below)

#define   cFaderMotion         0x7f // fader position data with TC sent to RECDSP for recording along with audio (see FaderMotion.cpp)

#define   cVRXSettings         0x80 // fader position data with TC sent to RECDSP for recording along with audio (see FaderMotion.cpp)
#define   cPICO_STATUS         0x81 // RP2040 status packet to c6747 DSP
#define   cProgramLarge        0x82 // large-format programming command that uses the 12-bit ZaxLan payload length field
/* 
cFaderMotion is not a fader motion packet (see FaderMotion.h) but rather a way to send fader motion packets to RECDSP:
payload for cFaderMotion is:
	(uint32)GMSECS   - timestamp from INPUTDSP of when this was valid
	uint16 FaderMotionPairs  (inputnum<<2 : faderval_dB*4) packed into a 16 bit word
	note NumberOfPairs is implied by hdr->len 
*/

// Deva24PC GUI wants ALL peaks at this point!!
//  WE ARE NOT USING THESE YET AND MAY NEVER NEED THEM IF WE DONT ADD ANY MORE SCREENS TO DEVA24PC
/*
#define   cPeaksMeters_PCTracks		 0x80  
#define   cPeaksMeters_PCTracksComp	 0x81
#define   cPeaksMeters_PCInputs		 0x82
#define   cPeaksMeters_PCInputsComp	 0x83
#define   cPeaksMeters_PCOutputs	 0x84
#define   cPeaksMeters_PCOutputsComp 0x85
*/
//#define  OASIS_CONNECTED     1	
//#define  FP8_CONNECTED       2	
//#define  MIX8_CONNECTED      4 	
//#define  RX12_CONNECTED      8	
//#define  PCGUI_CONNECTED        0x10
//#define  PCGUI_TOUCH_CONNECTED  0x20
//#define  PCGUI_OASIS_CONNECTED  0x40
#define   cConnectedStat    0x86 // connection status bits (uint16) used for PCGUI_CONNECTED (see remote.h)

#define   cMix16ChannelMap  0x87 // for DEVA24PC only at the moment

// HowyTerm opcodes, matched to QRX400/common/ZaxLanCmdCodes.h and howy_term/ZaxLanCmdCodes.h.
#define   cHT_PlaceButton 0xB0 // target creates a button on howyterm
#define   cHT_EditButton  0xB1 // change an existing button's color, state, or label
#define   cHT_ButtonPress 0xB2 // howyterm button pressed (howyterm -> target)
#define   cHT_Button      cHT_ButtonPress // old Nova name; this is a press, not the 0xB1 edit
#define   cHT_EditState   0xB3 // edit color and state string: id, color, stateStr
#define   cHT_DefineTable 0xB4 // tableId, rowCount, then label and format pairs
#define   cHT_UpdateTable 0xB5 // tableId, then raw values per row format
#define   cHT_Cmd         0xB6 // wrapper: bytes[0] is an inner HT opcode or HTSC_* extension



#define CMD_HAS_UUID(x) (x>=(unsigned)0xE0))
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These new commands have an 8 byte UUID at the end of every packet indicating its source or destination
// they are for the FP7 or a new Oasis or Aria could use them too.
// WARNING z.sourcerx must be zero since all command codes greater than 0x6f have a 16bit blength (see IS_BLEN16_CMD(x) (x>=0x6f))

/*****   cID_Fader is a new version of a fader command. Sent by an FP7 or a new Aria. 
 do not send any command faster than 15 times per second
 zbytes[]: a,b,c,d,iiiiiiii  
 where 
 a is the fader number 
 b is the fader value (0-255)
 c is the second fader number (optional)
 d is the second fader value  (optional) there can be several fader values sent in this way. 
 blength is used to determine how many fader values are in a packet
 iiiiiiii is the 8 byte UUID from the FP7's ROM chip
 if the command contains only an ID code then it is a request for data that triggers all fader values to be sent
*/
#define cID_Fader   0xE0 

/*****  cID_Key is a new key command   zbytes[]: a,b,iiiiiiii  
 where  a = key code, b = key value
 iiiiiiii is which slave device (FP7) should accept this data
*/
#define cID_Key     0xE1 

/****  cID_LED  turn on or off an LED using 16bit mLED_xxx bitmask    zbytes[]: ab,cd,iiiiiiii 
 where ab is a 16 bit LED mask, cd is a second LED bit mask
 iiiiiiii is which slave devive (FP7) should accept this LED data
*/
#define cID_LED     0xE2 



/********************************************
  to open a screen on the GUI from Oasis.
  Data payload is a string specifying the target page, from the list
  below. Note that the string is not nul-terminated.

Player
CommandMonitor
DiskUtilsFormat
Script
MyDeva
SetupMenu
InputNames
PresetNotes
DiskUtils
MirrorDrive
Cue
SceneTake
InputConfigure
NumberEditor
TextEditor
HeadphoneMix
MirrorFormat
DanteRouter
MainMenu
Memory
TimecodeRunMode
TimecodeEditor
RecordChannels
WirelessRx
Meters
PlaybackRouter
OperatingMode
InputEq
Timecode
FaderAssign
UserbitsEditor
DiskFolders
TrackMix
Home
OutputMix
Note
*************************/


void InitZaxLan();
//int  LookForZaxLanBlock(void);
//void SendUBZaxLanCmd(int UBcmdcode, int GroupUnit, uint16 w16);
//void SendUBZaxLanCmd(int UBcmdcode, int GroupUnit, uint16 w0, uint16 w1);
void SendUBZaxLanCmd(int UBcmdcode, int GroupUnit, uint16 *w, int16 words); // , int16 ParmRequest);  
void UpdateQRXprogStruct(); // copy our globals into a nice structure for sending to RX12

//void Send_Freq(int rxnum, int32 FreqA, int32 FreqB );
void SendUBZaxLanCmd(int UBcmdcode, int GroupUnit, uint16 w16);  

//#define MIN_ZLAN_PACKET_LEN  6 // smallest possible packet length in bytes (payload=0)
//#define MAX_ZLAN_BLENGTH  200  // max payload size (typically 5 when encapsulating a UBCMD block)
//#define MAX_ZLAN_BLENGTH   (512+16) // allow a 512 byte sector to be transmitted to RX12 DSP

#define ZLAN_HDRLEN          6 // lbytes in header that do not include payload
#define MIN_ZLAN_PACKET_LEN  6 // smallest possible packet length in bytes (payload=0)
#define MAX_ZLAN_BLENGTH  8000 // 254  // max payload size (typically 5 when encapsulating a UBCMD block)
//#define MAX_ZLAN_BLENGTH   (512+16) // allow a 512 byte sector to be transmitted to RX12 DSP
#define MIN_ZLAN_PACKET_LEN  6 // smallest possible packet length in bytes (payload=0)

//#define NUMBER_OF_RXSTAT 8 // see zaxlan.h
//#define MASTER_RXNUM (NUMBER_OF_RXSTAT-1) // last valid index
//extern short gZlanAllowPrintCrap;


struct ZAXLAN_CMD_STRUCT {
   uchar magic_AA;
   uchar sum8;
   uchar blength;   
   uchar sourcerx;  
   uchar destrx;    
   uchar cmd;   
   uchar bytes[MAX_ZLAN_BLENGTH];
};

struct ZAXLAN_CMD_STRUCT_16 {
   uchar magic_AA;
   uchar sum8;
   uchar blength;   
   uchar sourcerx;  
   uchar destrx;    
   uchar cmd;   
   uchar bytes[16];// this is for alocating the struct on the stack
};

//int SendZaxLanBlock(ZAXLAN_CMD_STRUCT *z);
//int SendZaxLanBlock(ZAXLAN_CMD_STRUCT *z, uchar *buf);
//int SendZaxLanBlock(WIRELESSRX *rx, ZAXLAN_CMD_STRUCT *z);

void Send_RXformat(int rxnum, uchar fmtA, uchar fmtB);


struct ZLAN_STATE{ // from QRX
	int16 debug; // replaces SERIAL_IS_ZLANDEBUG
	int ForceTXdata;
	int DiscardBytes;
	int ChksumErrors;
	int SentCount;
	int StuffCount;
	int RetryCount;
	int RawRxCmdCount, GoodRxCmdCount, RxBytes;
	int GoodSonyCmdCount;
	int LEDcmdCount;
	uint32 TimeoutAlarm;
};
extern struct ZLAN_STATE gZL;

#define ZLAN_TC_FROZEN_MASK     0x80     
#define ZLAN_TC_VALID_MASK      0x40
// this is the byte format of cTimecode command
typedef struct {
	uchar  FrameRate; 			// 0  MSbit is IsSTATIONARY flag
	uchar  hh,mm,ss,ff,ltc; 	// 1 2 3 4 5 
	uchar  ub[4];           	// 6 7 8 9   MSB is in ub[0]
	uchar  ZTPmode;         	// 10
	uchar  OptionalRawGMSECS[4]; // 11 12 13 14  optional raw timestamp. RECDSP puts its gExternalGMSECS in here (motorola endian)
	uchar  OptionalSPM[4];       // 15 16 17 18  optional SamplesPastMidnight. (motorola endian) if 96khz srate this is spm/2
	uchar  OptionalSampleRateD4[2]; // 19 20 optional Srate divided by 4 to fit itnot a uint16 (motorola endian) 
}ZLAN_TC;

struct QOPT_STAT_STRUCT {
//	uint32 TagError;
//	uint32 MissingMagic;
//	uint32 TagTooBig;
//	uint32 NotEnufData;
	int32  QoptBoardIsRunning;
//	int32  Sum32ErrorCount;
//	int32  Sum32_GOOD_Count;
//	int32  TxStompCount;
	uint16 ScreenOver;
//	uint16 LcdUpdateReq;
//	int32  LcdBlockCount;
//    uint16 *LCD_Bitmap;
};
extern struct QOPT_STAT_STRUCT gQOPTstat;

// bit field box properties
struct QUICKLEVELS_BITS
{
     unsigned int RFpeakA   : 8;//0
     unsigned int RFpeakB   : 8;//1
     		  int Audio0    : 8;//2
     unsigned int RXAformat : 4;//3
     unsigned int RXAantenna: 1;//3
     unsigned int RXBantenna: 1;//3
     unsigned int RXAlocked : 1;//3
     unsigned int RXBlocked : 1;//3

     		  int Audio1    : 8;//4
     unsigned int RXBformat : 4;//5
     unsigned int SingleMode: 1;//5
     unsigned int unu1_3    : 3;//5
     unsigned int TXAbatt   : 4;//6
     unsigned int TXBbatt   : 4;//6
	          int Audio2    : 8;//7

              int Audio3    : 8;//8
     unsigned int ErrA      : 8;//9
     unsigned int ErrB      : 8;//10
     unsigned int HardErr   : 8;//11

	unsigned int RXAfreq    : 16;
	unsigned int RXBfreq    : 16;

	unsigned int RXAtpmode  : 2;
	unsigned int RXBtpmode  : 2;
    unsigned int unu2_12    : 12;
};
extern struct QUICKLEVELS_BITS gQuick;

#ifndef RX4 // this is in uiRX12.cpp for some reason
// this struct contains all of the info that comes over on the TRX's sub channel data stream
// TC wil have to be fiddlede with to make it accurate during serial communication
struct SUBCH_INFO_BITS
{
	uint BatteryDisplayCode:4; 
	uint SubChUP    :1; //(14) map from old txer hardware ; HCS~  = PB14 = up  
	uint SubChDOWN  :1; //(13) map from old txer hardware ; HREQ~ = PB13 = down     switch input bit in x:<<M_HDR
	uint SubChFUNC  :1; //(12) map from old txer hardware : HDS   = PB12 = function switch input bit in x:<<M_HDR
	uint SubChREC   :1; //   new key for HandHeld channel switching feature (v5-73 Jan 10 2009)            
	uint Raw12bitBatterySample:8; // bits 7-0 (LSbyte)

	uint TPmode  :4;   // bits 15-12
	uint B0_Ver  :2;   // bits 11-10 version of this block 0 = old 1=addition of Version and Format fields
	uint B0_Unused :1;   // bits 9
	uint Delay   :5;   // bits 8-4
	uint Format  :3;   // bits 3-1
	uint SecurityOn:1; // bit  0

// V1 adds GainSetting
//struct SUBCH_BLK2  { // new TimeCode block

	// word[1][2] = TC (32khz)
	uint TC;
	// word[3][4] = UserBits
	uint UB;
	// word[5] = Status Bits
	uint B2_Version    : 3;  // V1
	uint FrameRate     : 3;
	uint HasBeenJammed : 1;
	uint IsStationary  : 1;
	uint B2_Unused     : 2;
	uint GainSetting   : 6;
};
extern struct SUBCH_INFO_BITS gSubChInfo[2];
#endif

//#define  cCHANNEL   4 //  cmd code to change channel ---
struct CHANNEL_BITS
{
    int32 chA; // in case we have to support 25khz spacing
    int32 chB; // 
};

//#define  cIDCODE    5 //  cmd code to  change idcode
struct IDCODE_BITS
{
    uint32 IDcodeAB;
};

struct TONE_BITS
{
	char ToneOnMode;  
};

#if 0 // def RX12
#define MAX_SCAN_CHANNELS 384
struct CHSCAN {
   int16 state;
   int16 chmin;
   int16 chmax;
   int16 index;
   int16 increment; // channel increment in 100khz steps
   uchar tScan[MAX_SCAN_CHANNELS+1];
//   uchar tQOS[MAX_SCAN_CHANNELS+1];
};
#endif

#define SCAN_MODE_CANCEL  0
#define SCAN_MODE_START   1 
#define SCAN_MODE_REQDATA 2
#define SCAN_MODE_START_RXAONLY 3
#define SCAN_MODE_START_RXBONLY 4
#define SCAN_MODE_SCANNING_MASK 0x10 // this bit is used to indicate we are scanning
// this is for starting or stopping a scan and for adjusting start and stop freq
struct CHSCAN_MODE_BITS
{
	int16 start; // 0=cancel  1=start  2=request scan data starting at index (alawys up to 20 points)
	// 3=start (use RXA only)
	// 4=start (use RXB only)
 	int16 increment; //  was  automatically determined by QRXplug scan machine .... char increment; // how many DecaHz are between each scan point
	int16 index; // where to get the data that we want
	int16 chmin; // start freq in decahz
	int16 chmax; // end freq in decahz
	uchar ave,dly,UsePostFilterPeaks;
};
struct CHSCAN_RESULTS_BITS
{
   char  Mstate;
   char  Mcount; // how many valid scan points we are returning 
   int16 Mchmin;
   int16 Mchmax;
   int16 Mindex;     // where the scan machine is currently at 
   int16 Mincrement; // channel increment in 100khz steps
   int16 Mave;
   int16 Mdly;
   int16 Mpfilt;
   int16 ThisStartIndex; // where these data points start in the real tScan array
   uchar ScanResults[20];
};


#define MRX_SIZEOF_RXFORMAT_RESULTS_BITS 18
struct RXFORMAT_RESULTS_BITS // as of v2.34C MRX reports the size of this struct as 18 bytes
{
	uchar  fmtA;
	uchar  fmtB;
	uchar  FormatMax; // FORMAT_MAX; // let RX12 know what this QRX can do
	uchar  gMachineInfo_ver; // changed in 1.26 !!

	uint16 gMachineInfo_MinFreqMHzA;
	uint16 gMachineInfo_MaxFreqMHzA;
	uint16 gMachineInfo_MinFreqMHzB;
	uint16 gMachineInfo_MaxFreqMHzB;

	// the following were added in MRX-233C
	uchar  AnalogSwitch; // 
	uchar  SwapOutputs;  // actually AESswap MODE
	uchar  RouterMode;   // 
	uchar  QRXprogState; // 
	// added in MRX v2.34C
	uint16 CenterFreqMHZ;  // PC endian format  
};

// most of the machine info struct from a QRX plug
struct QRX_MINFO
{
	uchar  Mver;    // version code of the QRX's MachineInfo struct
	uchar  ThisVer; // ""    ""  of this struct

	uint16 HardwareRevision;
	int16  SerialNumberDec; // represented as DECIMAL in RX12!!!!
	uint16 ModelNumber;          // raw model number code in ROM (see enums above)
	uint16 CrossA;//V2 decaHz
	uint16 CrossB;//V2 decaHz
	int16  FluxTweakVal; // move flux capacitor up or down as per Ronin's request (effects both filters equally)

	uchar  HasFluxCapacitor; // V3 set to 1 if varactor filter board is installed (requires 0x62 PLD)
	uchar  PLD_Rev;       // PLD rev code
	uchar  PLD_ID;        // PLD ID code 
	uchar  Options;	
	
	uint16 SROMdeviceID; 
	uint16 PROMdeviceID; 

	char software[6]; // software version string
};



struct ZAXLAN_DEBUG_STRUCT {
//	short AllowZaxLan;
	int32 DiscardBytes;
	int32 RetryCount;
	int32 GoodCmdCount, RawCmdCount;
	int32 ChksumErrors;
	int32 SentCount;
	int32 ComCount;
	int32 OverFlowCount;
	//char  SoftwareVersion[8];
};

// I had to put this in uiRx12.cpp!!!
//typedef struct 
//{ // put ms * 4 in here so an entry of 4 sends a cmd every 20ms
//	UInt8 ver; // indicates our modebytes capabilities
//	UInt8 AutoSend_cQUICKLEVELS;
//	UInt8 AutoSend_cSUBCH;
//	UInt8 AutoSend_cQRXPROGRAM;
//}MODE_BYTES;
//extern MODE_BYTES ModeBytes;
//void Send_ModeBytes(int rxnum);
//void Parse_ModeBytes(struct ZAXLAN_CMD_STRUCT *z);
int  ModeBytesMS(int rawcode);
int  MS_to_ModeBytes(int ms); 

char *GetZaxLanCmdString(unsigned cmd); //  char *ZlanCmdStringLUT[] = {

void UpdateScanMinMax(int FreqCenter);

void Send_QRXTX_cmd(short UBcmdcode, uint16 GroupUnit, uint16 w16);

int SendIFBcommand(            int UBcmdcode, int group, int unit, uint16 w0, uint16 w1=0);
int SendIFBcommand_MODULE_ONLY(int UBcmdcode, int group, int unit, uint16 w0, uint16 w1=0);
int SendIFBcommand_w64(        int UBcmdcode, int group, int unit, uint16 w0, uint16 w1, uint16 w2, uint16 w3, bool AllPorts=1);

extern int64 gExternalGMSECS; // GMSECs from INPUTDSP  (only good for 32 bits!)

int64 GetExternalGMSECS();


void   InitZaxLan();
uint16 GetBlength(struct ZAXLAN_CMD_STRUCT *z);
void   StoreBlength(struct ZAXLAN_CMD_STRUCT *z, uint16 len);

void sendzlan(struct ZAXLAN_CMD_STRUCT *z); 
void sendzlanRequest(uchar cmd); // req for a cmd to be sent to us
void sendzlanRequest(uchar cmd, SPORTENUM portenum);  // req for a cmd to be sent to us
void sendzlan(uchar cmd, uint16 len, uchar *payload); 
void sendzlan(uchar cmd, uint16 len, uchar *payload, SPORTENUM portenum); 
int  MakeZlanHeader(uchar *buf, uint16 len);
void sendzlan(struct ZAXLAN_CMD_STRUCT *z, uint16 len);
void sendzlan(struct ZAXLAN_CMD_STRUCT *z, uint16 len, SPORTENUM portenum);

uint32 BytesToU32(const uchar *p); 
uint16 BytesToU16(const uchar *p);
int    U32toBytes(uchar *p, uint32 x);
int    U16toBytes(uchar *p, uint16 x);

//void Send_Freq(int w, int rxnum, int32 FreqA, int32 FreqB);
void SendVersion(SPORTENUM port);



void SendIFBsettingZlan(int SettingEnum, int len, void *vpayload); // send a cSetting command to IFB module
// use cSetting command to set these values and then store them into Fixed parms
// nova needs to ask for these on boot up too!
#define CSETTING_VHF_MOD_GAIN          0 // Fixed.Mod_Gain); 
#define CSETTING_VHF_MOD_IQ_GAIN_RATIO 1 // Fixed.Mod_IQRatio); 
#define CSETTING_VHF_MOD_IQ_PHASE_IMB  2 // Fixed.Mod_Imballance); 
#define CSETTING_VHF_MOD_ICAL          3 // Fixed.Ical
#define CSETTING_VHF_MOD_QCAL          4 // Fixed.Qcal
#define CSETTING_VHF_DEBUG_CODE        5 // gDebugCode
#define CSETTING_VHF_DIAMOND           6 // Fixed.diamond
#define CSETTING_VHF_SYNTHPWR          7 // Fixed.SynthOutputLevel 
#define CSETTING_VHF_IQDELAY           8 // Fixed.IQDelayMode
#define CSETTING_VHF_TUNEOFFSET        9  // Fixed.TuneOffset
#define CSETTING_VHF_PORTMATCH         10 // Fixed.PortMatch
#define CSETTING_VHF_DEEMPH            11 // Fixed.IFB_DeEmph
#define CSETTING_VHF_POWERCAL          12 // Fixed.IFB_?
//#define CSETTING_VHF_ENABLE          13 // RamParms.VHF_TxEnable
#define CSETTING_VHF_USE_COMPANDER     14
#define CSETTING_VHF_POWERCAL3X        15 // for setting individual power cals Fixed.TXpwr_ZHD[3] thru Fixed.TXpwr_FM[3]

#define CSETTING_MOD_STRUCT            99 // send entire gModSettings struct

#define CSETTING_VHF_STORE_FIXED 123   // tell IFB board to store its current fixed parms into ROM
#define CSETTING_VHF_SENDALL     0x7f  // send all settings in one packet to Nova

struct MOD_SETTINGS 
{
	char Gain;        // 
	uchar IQRatio;    // 0-255
	uchar Imballance; // 0-255
	char Ical;
	char Qcal;
	char Diamond;
	char SynthOutputLevel;
	char IQdelay;
	char TuneOffset;// 8
	char PortMatch; // 9
	char DeEmph;    //10
	uchar PowerCal; //11   0-255
	uint16 TXpwr_ZHD[3];
	uint16 TXpwr_FM[3];
};
extern struct MOD_SETTINGS gModSettings;
extern char gSendVHF_NVsave;  // tell IFB board to save fixed parms in its own rom
extern char gVHF_DebugCode;




