#define ALLOW_RX4_TOUCH

#include "stdio.h"
#include "math.h"
#include "string.h"     // for memcpy 
#include "stdlib.h"     //  for abs

#include "deva.h"
#include "devatypes.h"				   
#include "globals.h"
#include "TypeDefs.h"
#include "AudioMix.h"
#include "Effects.h"
#include "nvdata.h"

#include "remote.h"
#include "transport.h" 
#include "RS232.h"     
#include "debug.h"   
#include "PS2KeyBoard.h" 
#include "hardware.h" 
#include "Mix12.h"
#include "AudioPeak.h"
#include <time.h>
#include "RealTimeClock.h"

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

#include <std.h>
#include <mbx.h>
#include <tsk.h>
#include "MailBoxes.h" // needed for transport mbx

#define LONG_KEY_MS 500

//#include "ZlanCmdStringLUT.h"
#include "RemoteOasis.h" // for sending STN to OASIS
#include "protocol.h"    // for oasis serial settings codes
#include "PC_GUI_OFFSETS.h"

enum {LEFT,RIGHT};

extern TimeCode *TCDisplayed;
extern TimeCode *TCReader;
extern TimeCode *TCGenerator;
//extern int16 gTrackNameChange; // for debug


struct PCGUI_STATE {
	char nTrackMeters, nOutputMeters, nInputMeters;
	char TrackMetersVisible, OutputMetersVisible, InputMetersVisible;
	char InputMetersNum; // which input meter is visible
}gPCGUIstate; // indicates what we need to send to PCGUI program (Nomad Touch or Oasis software)


extern struct BANK_STATES BankStates;

extern int gUSER_GUI_event;
extern int gAllow_comprintf;
extern int gfCommandScr;
extern struct PRESETS gPresetCache;

void SendMP3recModeStuff(int RecModeState);
void auto_send_server_Oasis();
void send_oasis_cmd(uint16 cmd, uint16 bytes, uchar *payloadMightBeNull);
void UpdateOasisLEDs();
int  CallHandlerViaEnum(int HandlerEnum, void *context, MESSAGE *msg);
void RequestSettingDataFromPC(uint16 SetID);
int  GetBankKey(int keycode); // returns -1 if not a bank key
void SetNewBank(int bank);
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

void SendSetting_Array1D(uint16 SetID, uchar *ary, uchar dimX, int ArraySize);
void SendSetting_Array1D(uint16 SetID, uint32 *ary, uchar dimX, int ArraySize);
void SendSetting_Array1D(uint16 SetID, char **StringArray, uchar dimX,  int ArraySize);
void SendSetting_Array1D(uint16 SetID, char *str, uchar dimX,  int StrIndex);

void SendOasisSegEntry(int seg, char *scene, char *take, char *note);

void RequestVersion(); // this is a mix12 command but it will encapsulate when needed
void RequestInit();
void RequestPCSetting(int SetID);
void RequestVersion_SonyStyle();
    
int process_Oasis_version(uchar *str);
int process_Oasis_fader(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_trims(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_outknobs(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_encoder(struct ZAXLAN_CMD_STRUCT *z);
int process_Oasis_key(struct ZAXLAN_CMD_STRUCT *z); // 
int process_Oasis_MsgHandlerCall(struct ZAXLAN_CMD_STRUCT *z); 
int process_Oasis_Setting(struct ZAXLAN_CMD_STRUCT *z);
int process_QRXTX_RX12(struct ZAXLAN_CMD_STRUCT *z); 

void   SetCrosspointFromPC(MixerType MixerBusType, MATRIX_CELL *cell);
void   SetHeadPhoneCrosspointFromPC(uchar preset, InputBus InputBusType, uchar channel, uchar PCenum);
uint16 ConvertPChpEnumToHPnibble(uint16 PCenum);
uint16 ConvertHPnibbleToPCenum(uint16 nibs);

extern struct ZLAN_STATE gZL;  // mostly for status and debug stuff

int  ParseCommandString_Oasis(struct ZAXLAN_CMD_STRUCT *z); 
void SendKeypressMessage(MESSAGE *KeyMesg); // from tasks.cpp
void SendKeyMsg(MSG_TYPE type, uint16 value); // same thing but simpler

void   cmd_send_LED(uint16 ledcode, int state);
void   cmd_send_CommandSt3ring(char *str);
void   SendOasisMP3RecordFile(struct DIR *dd, char *FolderName, int seg, int FileNameMode);
void   SendOasisMP3RecordMetadata(struct DIR *dd, char *FolderName, int seg, int FileNameMode);
void   UpdateOasisLEDs();
void   UpdateTrackLEDdata();
uint32 BytesToU32(const uchar *p);
uint16 BytesToU16(const uchar *p);
int    U32toBytes(uchar *p, uint32 x);
int    U16toBytes(uchar *p, uint16 x);
void   BytesToCell(MATRIX_CELL *cell, uchar *p);
int    CellToBytes(uchar *p, MATRIX_CELL *cell);
void   Mix12Handler();
int    GetHPmatrix(int HPisRight, int IsCardBus, MixerType HPmixertype, MATRIX_CELL *cell);

float CalcLineEquation(float f1, float f2, float p1, float p2, float channelMHz);
void  OasisSTNupdateHandler();
char *FixVersionString(struct ZAXLAN_CMD_STRUCT *z);
void  SendSetting_Array2Drow(uint16 SetID, uchar row, uchar bytestosend, uchar *ary, uchar dim1, uchar dim2);

extern enum  TRANSPORT_MODE DMtpmode;   // current transport mode             
extern int gFoundMix12;
extern int gfMix12SyncReq;

extern uint32 gSendAllExitTime;
extern int   gMeterSendMS;
extern short gPrintSettingHex;     // print hex output of cSetting cmds that Nomad sends to PC
extern int   gHPrefreshRequest;    // force HP mem page to refresh when we change HP preset via PC
extern short gMetersConfigRequest; // home page update meter info
extern short gReqSetting, gForceSendAll;
extern short gWaitForTX,  gQuietMode; // dont print debug stuff (during PC init)
extern float gProtocolVersionFloat;   // empty version code

extern char gOasisSTNchanged[255]; // flags indicating STN changed in a segment slot in dir[] struct
int    FindLastTrackEnablesIndex();
void   cmd_send_CommandString(char *str);
void   PrintBuf(uchar *buf, int count);

extern short gMP3IsRecording;
int UnpackArray(const unsigned char *buf, unsigned len);
#define mMIX_ASSIGNED  0x01
#define mMIX_POSTFADE  0x02
#define mMIX_PHASE     0x04

/*
typedef struct {
    MSG_TYPE  Type;
    union {
        SInt32  Signed;
        UInt32  Unsigned;
        void   *Pointer;
        char   *Text;
    } Value;
}MESSAGE;
*/
#define DFORMAT_STRING   0
#define DFORMAT_8BIT     1  
#define DFORMAT_16BIT    2
#define DFORMAT_32BIT    4
#define DFORMAT_KEYPAIRS 5 

char *dFormatString_LUT[] = {
"STRING(0)",
"8BIT(1)",
"16BIT(2)",
"UnKnownFmt(3)",
"32BIT(4)",
"KEYPAIRS(5)", 
"UnKnownFmt(6)",
};


enum {
    UNPACK_FULL_INDEX,
    UNPACK_SHORT_INDEX,
};
static void show(const char *msg, unsigned nidx, unsigned char *idx, uchar *arr, const char *eol)
{
    unsigned i;
    dprintf(cYELLOW,"%s", msg);
    for (i = 0; i < nidx; i++) // print cell index to be changed
        dprintf(cYELLOW, "[%02d]", idx[i]);

	// print array dimensions
	if(nidx==1) dprintf(cBLUE, " dim=%d ", (int)arr[0]);
	if(nidx==2) dprintf(cBLUE, " (%dx%d) ", (int)arr[0], (int)arr[1]);
	if(nidx==3) dprintf(cBLUE, " (%dx%dx%d) ", (int)arr[0], (int)arr[1], (int)arr[2]);
    // display how to fill cell->zdim,col,row values when sending back data
    //dprintf(cMAGENTA, " ZCR(%d,%d,%d) ", (int)idx[0], (int)idx[1], (int)idx[2]);
	//dprintf("%s", eol);
}

int ApplySetting(uint16 SetID, uchar nidx, uchar *idx, uchar *arr, char *eol, MATRIX_CELL *cell);

int UnpackArray(const unsigned char *buf, unsigned len)
{
#define CHECK(cond, msg) if (!(cond)) { dprintf(cRED, msg); goto Fail; }
#define CHECK_BUFFER() CHECK(p<=buf+len, "Buffer overrun while unpacking")
    unsigned char narr = 0; unsigned char arr[3];   // Optional array dimensions
    unsigned char nidx = 0; unsigned char idx[3];   // Optional index where array will be cleared/modified
    unsigned i, j;
    unsigned state = UNPACK_FULL_INDEX;
    const unsigned char *p = buf;
    unsigned char fmt;
    unsigned short id;
    int clear = 1;
	MATRIX_CELL cell;
	memset(&cell, 0, sizeof(cell) );

    dprintf("-----unpack %d bytes:"  , len);

    id = *p++;
    id |= (*p++) << 8;  // ID
    fmt = *p++;         // FMT

    dprintf("id=0x%02X fmt=%s ", id, dFormatString_LUT[fmt]);
    CHECK(fmt <= 5, "Unsupported format");

    // For arrays, get details about the source array
    narr = *p++; CHECK(narr <= sizeof(arr), "Array dimensions too large");
    for (i = 0; i < narr; i++) {
        arr[i] = *p++;
    }
    CHECK_BUFFER();
    // For arrays, get index that specifies which part of the array is being modified
	memset(idx,0,sizeof(idx));
    if (narr) {
        nidx = *p++; CHECK(nidx <= sizeof(idx), "Index dimensions too large");
        for (i = 0; i < nidx; i++) {
            idx[i] = *p++;
        }
    }
    if (narr != nidx) {
        // TODO: send message to clear setting
        show("CLR", nidx, idx, arr, "\n");
        clear = 0;   
	     //cell.Zdim  = idx[0]; //  page number ????
	     //cell.Col   = idx[1]; //  0 based index
	     //cell.Row   = idx[2]; //  0 based index
	     if(fmt==0)
	     	cell.Pointer = (void *)""; // return pointer to empty string
	     else
	     	cell.Value=0; // return zero for clear
	     ApplySetting(id, nidx, idx, arr, "\n", &cell);
          }

    // Read (index, value) pairs from rest of packet
    while (p < buf+len) {
        CHECK_BUFFER();
        // Get index (if required)
        if (state == UNPACK_FULL_INDEX) {
            // Index in header provides nidx bytes. Unpack the remaining
            // narr-nidx bytes to make a full index
            for (j = nidx; j < narr; j++) {
                idx[j] = *p++;
            }
            state = UNPACK_SHORT_INDEX;
        }
        else if (state == UNPACK_SHORT_INDEX) {
            // Unpack just the final byte of index
            idx[narr-1] = *p++;
            if (idx[narr-1] == 0xFF) {
                // Go back to get a full index
                state = UNPACK_FULL_INDEX;
                continue;
            }
        }

        // Get value
        // TODO: send message to set value at specified full index
        show("SET", narr, idx, arr, " ");
        clear = 0;
		//cell.Zdim  = idx[0]; //  page number ????
		//cell.Col   = idx[1]; //  0 based index
		//cell.Row   = idx[2]; //  0 based index

        if (fmt >= 1 && fmt <= 4) {         // Number 1-4 bytes
            uint32 value = 0;
			// this appears to be the wrong endian assumption
            //for (j = 0; j < fmt; j++) {
            //    value <<= 8;
            //    value |= *p++;
            //}
            if(fmt==1) {value = *p; 							dprintf("0x%01X ", value);} // dont use fmt*2 trick here!
            if(fmt==2) {value = BytesToU16((const uchar *)p); 	dprintf("0x%02X ", value);}
            if(fmt==4) {value = BytesToU32((const uchar *)p); 	dprintf("0x%04X ", value);}
			p += fmt;
		    cell.Value = value;
  	     	ApplySetting(id, nidx, idx, arr, " ", &cell);
        }
        else if (fmt == 0) {                // String
            dprintf("\"%s\"\n", p);
		   	cell.Pointer = (void *)p; 
	     	ApplySetting(id, nidx, idx, arr, " ", &cell);
            for ( ;*p; p++) ;
            p++;
        }
        else if (fmt == 5) {                // Key,Value pairs
			char *m[12], jj=0;
			dprintf("\n"); // scene=xx take=tt note=nn
            while (*p) {
                dprintf("    %s=", p);  // not sure how to decode this
                m[jj++]=(char *)p; for ( ;*p; p++) ; p++;
                dprintf("%s\n", p);
                m[jj++]=(char *)p; for ( ;*p; p++) ; p++;
				if(jj>=10)
					break;
            }
			m[jj++]=NULL;
			m[jj++]=NULL;
			cell.Pointer = (void *)m; // list of strings
  	     	ApplySetting(id, nidx, idx, arr, " ", &cell);
            p++;
            dprintf("\n");
        }
        if (narr == 0) break;
    }
    if (clear) { /* Deferred clear required if narr=nidx and no data received */
        show("CLR", nidx, idx, arr, "\n");
	     //cell.Zdim  = idx[0]; //  page number ????
	     //cell.Col   = idx[1]; //  0 based index
	     //cell.Row   = idx[2]; //  0 based index
	     cell.Value=0;
	     ApplySetting(id, nidx, idx, arr, "\n", &cell);
    }
    //dprintf("-----done-----\n");
    return 0;

Fail:    
    dprintf(cRED, "-------Bad packet-------");
#undef CHECK_OVERFLOW
#undef CHECK
    return -1;
}


// send a single indexed value in a 1D array
void SendSetting_1D_indexed(uint16 SetID, uchar index, uchar val, uchar dimX)
{
	int i,j=0;
	uchar p[20];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	dprintf(cBLUE, "Sending_1D_indexed %s [%d] val=%d\n", GetSettingString(SetID), (int)index, (int)val);

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 1;     // 1D array 
	p[j++] = dimX;  // array dimentions

	p[j++] = 1;     // 1D index to follow 
	// send index,val
	p[j++] = index;
	if(val)
		p[j++] = val; // only send non-zero values

	send_oasis_cmd(cSetting,j,p);
	return;
}
void SendSetting_Array1D(uint16 SetID, uchar *ary, uchar dimX, int ArraySize)
{
	int i,j=0;
	uchar p[120];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	
		{
		dprintf(cBLUE, "Sending %s ", GetSettingString(SetID));
		for(i=0;i<ArraySize;i++)
			dprintf("%d ", (int) ary[i]);
		dprintf("\n");			
		}

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 1;    // 1D array 
	p[j++] = dimX; // array dimentions
	p[j++] = 0;    //  clear entire array 
	for(i=0;i<ArraySize;i++) // send each non-zero value
		{
		if(ary[i])
			{
			p[j++] = i;
			p[j++] = ary[i];
			}
		}

	send_oasis_cmd(cSetting,j,p);
	return;
}
void SendSetting_Array1D(uint16 SetID, uint32 *ary, uchar dimX, int ArraySize)
{
	int i,j=0;
	uchar p[120];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	
		{
		dprintf(cBLUE, "Sending %s ", GetSettingString(SetID));
		for(i=0;i<ArraySize;i++)
			dprintf("%d ", (int) ary[i]);
		dprintf("\n");			
		}

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_32BIT;
	p[j++] = 1;    // 1D array 
	p[j++] = dimX; // array dimentions
	p[j++] = 0;    //  clear entire array 
	for(i=0;i<ArraySize;i++) // send each non-zero value
		{
		if(ary[i])
			{
			p[j++] = i;
			p[j++] = 0xff&(ary[i]);    // since this is a color I dont know which way is which
			p[j++] = 0xff&(ary[i]>>8);
			p[j++] = 0xff&(ary[i]>>16);
			p[j++] = 0xff&(ary[i]>>24);
			}
		}

	send_oasis_cmd(cSetting,j,p);
	return;
}


// string array overload
void SendSetting_Array1D(uint16 SetID, char **StringArray, uchar dimX,  int ArraySize)
{
	int i,j=0;
	uchar p[200];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s\n", GetSettingString(SetID));

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_STRING;
	p[j++] = 1;    // 1D array 
	p[j++] = dimX; // array dimentions
	p[j++] = 0;    //  clear entire array   but how do I NOT do this?
	for(i=0;i<ArraySize;i++) // do for each string
		{
		int len = strlen(StringArray[i]);
		if(len) // does this string exist?
			{
			p[j++] = i; // yes, send it's index first
			for(int x=0;x<=len;x++) // include null terminator
				{
				p[j++] = StringArray[i][x];
				//if(j>=sizeof(p)) { brkpt("StrFullErr"), return;} // just leave if our buffer is too small
				}
			}
		}

	send_oasis_cmd(cSetting,j,p);
	return;
}
// single string with index overload
void SendSetting_Array1D(uint16 SetID, char *str, uchar dimX,  int StrIndex)
{
	int i,j=0;
	uchar p[200];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s[%d] <%s>\n", GetSettingString(SetID), StrIndex, str);

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_STRING;
	p[j++] = 1;    // 1D array 
	p[j++] = dimX; // array dimentions
	p[j++] = 1;    		// 1 dim byte to follow?
	p[j++] = StrIndex;  // send it's index first
	for(int x=0;x<=strlen(str);x++) // include null terminator
		p[j++] = str[x];

	send_oasis_cmd(cSetting,j,p);
	return;
}


// fill Row,Col,Value and provide overall dimentions of array in dm1,dim2
void SendSetting_2D_indexed_CR(uint16 SetID, MATRIX_CELL *cell, uchar dim1, uchar dim2)
{
	int i,j=0;
	uchar p[99];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;

	if(!gQuietMode)	dprintf(cBLUE, "Sending_2DLCR %s R=%d C=%d val=%d\n", GetSettingString(SetID), cell->Row, cell->Col, (int)cell->Value);

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 2;  	// 2D array 
	p[j++] = dim1;  // array dimentions
	p[j++] = dim2;  // array dimentions

	p[j++] = 2;  // 2D index to follow 

	// send index,val
	p[j++] = cell->Row;
	p[j++] = cell->Col;
	if(cell->Value)
		p[j++] = cell->Value; // only send non-zero values

//	PrintBuf(p,j); // 
	send_oasis_cmd(cSetting,j,p);
	return;
}

// fill Col,Pointer and provide overall dimentions of array in dim1,dim2 (sends entire row based on dim2
void SendSetting_Array2Drow(uint16 SetID, uchar row, uchar bytestosend, uchar *ary, uchar dim1, uchar dim2)
{
	int i,j=0;
	uchar p[120];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;

	if(!gQuietMode)	
		{
		dprintf(cBLUE, "Sending_Array2Drow %s row#%d d1=%d d2=%d: ", GetSettingString(SetID), (int)row,(int)dim1,(int)dim2);
		for(i=0;i<bytestosend;i++)
			dprintf("%d ", (int) ary[i]);
		if(bytestosend==0)
			dprintf(cYELLOW, "CLEAR ARRAY");
		dprintf("\n");			
		}

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 2;  	// 2D array 
	p[j++] = dim1;  // array dimentions
	p[j++] = dim2;  // array dimentions
	if(bytestosend==0)
		{
		p[j++] = 0;     //  clear entire array 
		send_oasis_cmd(cSetting,j,p);
		return;
		}

	p[j++] = 1;   //  1 dimentional index to follow which means we want to clear a row of data
	p[j++] = row; //  clear the row we are about to fill

	for(i=0;i<bytestosend;i++) // send each non-zero value in the entire row
		{
		if(ary[i])
			{
			// send index,val
			//p[j++] = row;
			p[j++] = i; // only send a single index into the 2d array!!!
			p[j++] = ary[i]; // only send non-zero values
			}
		}

	send_oasis_cmd(cSetting,j,p);
	return;
}

// Assumes first index is zero (oreset 0 for trackmix)
// fill Col,Pointer and provide overall dimentions of array in dim1,dim2 (sends entire row based on dim2
void SendSetting_Array3Drow(uint16 SetID, uchar row, uchar bytestosend, uchar *ary, uchar dim1, uchar dim2, uchar dim3)
{
	int i,j=0;
	uchar p[120];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;

	if(!gQuietMode)	
		{
 		dprintf(cBLUE, "Sending_Array3Drow %s row#%d d1=%d d2=%d d3=%d: ", GetSettingString(SetID), (int)row,(int)dim1,(int)dim2, (int)dim3);
		for(i=0;i<bytestosend;i++)
			dprintf("%d ", (int) ary[i]);
		if(bytestosend==0)
			dprintf(cYELLOW, "CLEAR ARRAY");
		dprintf("\n");			
		}

	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 3;  	// 3D array 
	p[j++] = dim1;  // array dimentions
	p[j++] = dim2;  // array dimentions
	p[j++] = dim3;  // array dimentions
	if(bytestosend==0)
		{
		p[j++] = 0;     //  clear entire array 
		send_oasis_cmd(cSetting,j,p);
		return;
		}

	p[j++] = 2;   //  2 dimentional index to follow which means we want to clear a row of data
	p[j++] = 0;   //  always assume preset #0
	p[j++] = row; //  clear the row we are about to fill

	for(i=0;i<bytestosend;i++) // send each non-zero value in the entire row
		{
		if(ary[i])
			{
			// send index,val
			//p[j++] = row;
			//p[j++] = 0; // preset #0
			p[j++] = i; // column number
			p[j++] = ary[i]; // only send non-zero values into the desired row
			}
		}

	send_oasis_cmd(cSetting,j,p);
	return;
}

// fill Zdim,Row,Col,Value and provide overall dimentions of array in dm1,dim2
void SendSetting_3D_indexed(uint16 SetID, MATRIX_CELL *cell, uchar dim1, uchar dim2, uchar dim3)
{
	int i,j=0;
	uchar p[99];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;

	if(!gQuietMode)	dprintf(cBLUE, "Sending_3D %s\n", GetSettingString(SetID));
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 3;  	// 3D array 
	p[j++] = dim1;  // array dimentions
	p[j++] = dim2;  // array dimentions
	p[j++] = dim3;  // array dimentions

	p[j++] = 3;  // 3D index to follow 

	// send index,val
	p[j++] = cell->Zdim;
	p[j++] = cell->Row;
	p[j++] = cell->Col;
	if(cell->Value)
		p[j++] = cell->Value; // only send non-zero values
//for(i=0;i<j;i++) // 
//	dprintf("%02x ",p[i]);
//dprintf("\n");
	send_oasis_cmd(cSetting,j,p);
	return;
}

void PrintBuf(uchar *buf, int count)
{
	for(int i=0;i<count;i++) // 
		dprintf("%02x ", buf[i]);
	dprintf("\n");
}

void SendSetting_String(uint16 SetID, char *str)
{
	int i,j=0;
	uchar p[200];
	if(SetID!=SETTING_STATUS_TEXT) //  allow status text no matter what so we know why GUI is not working
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s <%s>\n", GetSettingString(SetID), str);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_STRING;
	p[j++] = 0;    // non-array 
	for(int x=0;x<=strlen(str);x++) // include null terminator
		p[j++] = str[x];

	send_oasis_cmd(cSetting,j,p);
	return;
}
short gDontSendStatusLines=0;
extern uint32 gPCdisconnectCount, gPCGUI_ConnectTime;
extern char gstrFWline1[], gstrFWline2[], gstrFWline3[],gstrWarning[]; // , gstrBattery[]; // display lines kludge
void send_Oasis_HomeScreenStatus() // SETTING_BATTERY_STATUS
{
	int  i, tmp, Percent, IsExternal, IsAlmostDead, IsReallyDead, len;
	float Volts;
	uchar ary[20], FoundDiff;
	static uchar prev_ary[20], inited=0;
	char str[99], tmpstr[30];
	if( (!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED)) || gDontSendStatusLines )
		return;

	if(inited==0)
		{
		memset(prev_ary,0xff,sizeof(prev_ary));
		inited=1;
		}
	
	// look for changes in IOconfig setting for Red input boxes on PC
#ifdef NOMAD
	len = numberof(ary);
	memset(ary,0,sizeof(ary));
	ary[10] = ary[11] = !IOConfigIsActive(IO_CONFIG_AES_1_2);
	ary[12] = ary[13] = !IOConfigIsActive(IO_CONFIG_AES_3_4);
	ary[14] = ary[15] = !IOConfigIsActive(IO_CONFIG_AES_5_6);
	ary[16] = ary[17] = !IOConfigIsActive(IO_CONFIG_AES_7_8);
	ary[6]  = ary[7]  = !IOConfigIsActive(IO_CONFIG_RET_1_2);
	ary[8]  = ary[9]  = !IOConfigIsActive(IO_CONFIG_RET_3_4);
	ary[4]  = ary[5]  = !IOConfigIsActive(IO_CONFIG_ALG_5_6);
	FoundDiff=0;
	for(i=0;i<len;i++)
		{
		if(ary[i]!=prev_ary[i])
			FoundDiff++;
		prev_ary[i] = ary[i];
		}
	if(FoundDiff) // send them all
		SendSetting_Array1D(SETTING_INPUT_WARNING, ary, 32, len);
#endif

gQuietMode=1;
	#if 0 // def RX4
	Percent=50; Volts=11, IsExternal=1; IsAlmostDead=0; IsReallyDead=0; strcpy(str, "EXT");
	#else
	UI_GetBatteryPowerInfo(&Percent, &Volts, &IsExternal, &IsAlmostDead,  &IsReallyDead, str);
	#endif

	if(str[0]=='I') 
		str[0]='B'; // I and 1 look so similar on PC so 7.0V looks like 17.0V
	SendSetting_String(SETTING_BATTERY_STATUS, str);

	GetHHMMSSstring(str, gDiskSecsLeft, "");
	SendSetting_String(SETTING_DRIVE_TIME_REMAINING, str);

	tmp = GetElapsedRecTime();
	GetHHMMSSstring(str, tmp, "");
	SendSetting_String(SETTING_DRIVE_TIME_ELAPSED, str);

	strcpy(str, " [");
	strcat(str, gstrFWline1);	strcat(str, " :: ");
	strcat(str, gstrFWline2);	strcat(str, " :: ");
	strcat(str, gstrFWline3);
	strcat(str, "]");
	//sprintf(tmpstr, " PCdisCount=%d ", gPCdisconnectCount); strcat(str, tmpstr);
	//sprintf(str, ":: %s :: %s :: %s", gstrFWline1,gstrFWline2,gstrFWline3);
	if( (GMSECS-gPCGUI_ConnectTime)<4000)
		SendSetting_String(SETTING_STATUS_TEXT, "------ Found Nova ------");
	else
		SendSetting_String(SETTING_STATUS_TEXT, str);
gQuietMode=0;
	return;
}


void SendSetting_Byte(uint16 SetID, uchar val) // this is translated as signed in PC!!!!!!!!!! (so ExtMeterDecay cant go over 127)
{
	int i,j=0;
	uchar p[30];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode) dprintf(cBLUE, "Sending %s (%d)\n", GetSettingString(SetID), (int)val);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_8BIT;
	p[j++] = 0; // no array dims
	p[j++] = val;

	send_oasis_cmd(cSetting,j,p);
	return;
}

void SendSetting_int16(uint16 SetID, int16 val)
{
	int i,j=0;
	uchar p[30];
	if(!IsConnected(OASIS_CONNECTED|PCGUI_CONNECTED|DEVA32_CONNECTED) )
		return;
	if(!gQuietMode) dprintf(cBLUE, "Sending %s (%d)\n", GetSettingString(SetID), (int)val);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_16BIT;
	p[j++] = 0; // no array dims
	//p[j++] = 0xff&(val); 
	//p[j++] = 0xff&(val>>8);
	j += U16toBytes(&p[j], val);
	send_oasis_cmd(cSetting,j,p);
	return;
}



void SendOasisSegEntry(int seg, char *scene, char *take, char *note)
{
	int i,j=0, len, SetID=SETTING_METADATA_HISTORY;
	uchar p[200];
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s (seg %d)\n", GetSettingString(SetID), seg);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_KEYPAIRS; //05
	p[j++] = 1;    // 1D-array 

	p[j++] = 10;    // array dimention

	if(seg == -1) // clear entire array if seg is -1
		{ 
		p[j++] = 0;
		send_oasis_cmd(cSetting,j,p);
		return;
		}

	p[j++] = 1;     // 1 index to follow
	p[j++] = seg-1; // zero based  seg number index

	len = sprintf((char *)&p[j], "take");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", take);	j += (len+1);

	len = sprintf((char *)&p[j], "scene");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", scene);	j += (len+1);

	len = sprintf((char *)&p[j], "note");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", note);	j += (len+1);
	//p[j++] = 0;     // double terminator
	send_oasis_cmd(cSetting,j,p);
	return;
}
#if 1
void SendMP3recModeStuff(int RecModeState)	
{
	if(RecModeState)
		{
		int seg = UI_GetCurrentRecSeg(), mode =  gmir[gNV.MirrorDrive].cfg.FileNameMode;
		char *fname = folder[GetCurrentFolder()].Name;
		SendOasisMP3RecordFile(&dir[seg], fname, seg, mode);
		SendOasisMP3RecordMetadata(&dir[seg], fname, seg, mode);
		cmd_send_CommandString("mp3.record");
		gMP3IsRecording=1;
		}
	else
		{
		cmd_send_CommandString("mp3.stop");
		gMP3IsRecording=0;
		}
	return;
}
void SendOasisMP3RecordFile(struct DIR *dd, char *FolderName, int seg, int FileNameMode)
{
	int i,j=0, len, SetID=SETTING_MP3_RECORD_FILE;
	uchar p[200];
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s (seg %d)\n", GetSettingString(SetID), seg);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_KEYPAIRS; // 05
	p[j++] = 0;    // not an array 

  len = sprintf((char *)&p[j], "foldername\1%s\1segnum\1%03d\1scene\1%s\1take\1%s\1mode\1%d\1",
                                FolderName, seg, dd->Scene, dd->Take, FileNameMode);
  for (uchar *s = &p[j]; *s; s++) { if (*s == '\1') *s = 0; }
  j+=len;
/*
	len = sprintf((char *)&p[j], "foldername");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", FolderName);	j += (len+1);

	len = sprintf((char *)&p[j], "segnum");			j += (len+1);
	len = sprintf((char *)&p[j], "%d", seg);		j += (len+1);

	len = sprintf((char *)&p[j], "scene");			j += (len+1);
	len = sprintf((char *)&p[j], "%s", dd->Scene);	j += (len+1);
	len = sprintf((char *)&p[j], "take");			j += (len+1);
	len = sprintf((char *)&p[j], "%s", dd->Take);	j += (len+1);

	len = sprintf((char *)&p[j], "mode");				j += (len+1);
	len = sprintf((char *)&p[j], "%d", FileNameMode);	j += (len+1);
*/
	send_oasis_cmd(cSetting,j,p);
	return;
}
extern  const char *pFrameRateStrTbl[]; //  = {"23.98","24","25","29.97NDF","29.97DF","30NDF","30DF"};
void SendOasisMP3RecordMetadata(struct DIR *dd, char *FolderName, int seg, int FileNameMode)
{
	int i,j=0, len, SetID=SETTING_MP3_RECORD_METADATA;
	uchar p[200];
	if(!gQuietMode)	dprintf(cBLUE, "Sending %s (seg %d)\n", GetSettingString(SetID), seg);
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_KEYPAIRS; //05
	p[j++] = 0;    // not an array 

	len = sprintf((char *)&p[j], "foldername");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", FolderName);	j += (len+1);

	len = sprintf((char *)&p[j], "segnum");		j += (len+1);
	len = sprintf((char *)&p[j], "%d", seg);	j += (len+1);

	len = sprintf((char *)&p[j], "scene");			j += (len+1);
	len = sprintf((char *)&p[j], "%s", dd->Scene);	j += (len+1);
	len = sprintf((char *)&p[j], "take");			j += (len+1);
	len = sprintf((char *)&p[j], "%s", dd->Take);	j += (len+1);
	len = sprintf((char *)&p[j], "note");			j += (len+1);
	len = sprintf((char *)&p[j], "%s", dd->Note);	j += (len+1);

	len = sprintf((char *)&p[j], "tc");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", TCGenerator->GetTimeCodeStr());	j += (len+1);
	len = sprintf((char *)&p[j], "ub");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", TCGenerator->GetUserBitsStr());	j += (len+1);

	len = sprintf((char *)&p[j], "fr");		j += (len+1);
	char tmpstr[10];
	GetFrameRateString(tmpstr, dir[seg].FrameRate);
	len = sprintf((char *)&p[j], "%s", tmpstr); j += (len+1);

	len = sprintf((char *)&p[j], "mode");				j += (len+1);
	len = sprintf((char *)&p[j], "%d", FileNameMode);	j += (len+1);
	p[j++]=0;
	send_oasis_cmd(cSetting,j,p);
	return;
}
#endif

#if 0
void SendClearMetadataHistoryCommand()
{
//0x3A 0x00    metadata.history
//0x05             type=kv
//0x01 0x0A    array size 1D [10]
//0x00             empty index (clears the entire array)
//                     ... no data
	int i,j=0, len, SetID=SETTING_METADATA_HISTORY;
	uchar p[30];
	if(!gQuietMode)	dprintf(cGREEN, "Sending Clear History\n");
	U16toBytes(&p[j], SetID); j+=2;
	p[j++] = DFORMAT_KEYPAIRS; //05
	p[j++] = 1;    // 1D-array 

	p[j++] = 10;    // ??
	p[j++] = 1;     // 1 index to follow
	p[j++] = seg-1; // seg number index

	len = sprintf((char *)&p[j], "take");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", take);	j += (len+1);

	len = sprintf((char *)&p[j], "scene");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", scene);	j += (len+1);

	len = sprintf((char *)&p[j], "note");		j += (len+1);
	len = sprintf((char *)&p[j], "%s", note);	j += (len+1);
	//p[j++] = 0;     // double terminator
	send_oasis_cmd(cSetting,j,p);
	return;
}
#endif

void OasisSTNchanged(int seg) 
{
	if( (seg<=0) || (seg>=numberof(gOasisSTNchanged)) )
		return;
	gOasisSTNchanged[seg]=1;
	gOasisSTNchanged[0]=1; // zeroth entry is a global change flag so we dont have to search thru the array all the time
	//.OasisChanged = folder[DMcurFolder].OasisChanged=1;
}

void SetCrosspointFromPC(MixerType MixerBusType, MATRIX_CELL *cell)
{
	int OutputBusNum= cell->Col;   		// trackmix num 0,1
	int InputBusNum = cell->Row;   		// input num  (0x10=digital)
	int xptType 	= cell->Value&0xFF; // 
	int ActionMask	= XPT_OFF;
	int iInput      = cell->Row;
   	InputBus        InputBusType = PF_ANALOG_BUS;
	if(xptType & mMIX_ASSIGNED) ActionMask   = XPT_ON;
	if(xptType & mMIX_PHASE)	ActionMask  |= XPT_INVERT;
	
	if(MixerBusType==OUTPUT_MIXER) 
		{ 
		if( InRange(OutputBusNum, 0,4-1) ) //  output 1-4 mixes 
			{ 
			if( InRange(iInput, 0,5) )   // AA|AA|RR   analog and return inputs
				{
				if(xptType & mMIX_POSTFADE)	InputBusType = ANALOG_BUS;
				else						InputBusType = PF_ANALOG_BUS;
				InputBusNum  = iInput;  // (iInput-4);
				}
			else if( InRange(iInput, 6,13) ) // DDDD|DDDD // AES inputs
				{
				if(xptType & mMIX_POSTFADE)	InputBusType = DIGITAL_BUS;
				else						InputBusType = PF_DIGITAL_BUS;
				InputBusNum  = iInput-6; 
				}
			else if( InRange(iInput, 14,21) ) // XXXX|XXXX // RXer inputs
				{
				if(xptType & mMIX_POSTFADE)	InputBusType = DIGITAL_BUS;
				else						InputBusType = PF_DIGITAL_BUS;
				InputBusNum  = iInput-14 + MAX_AES_INPUTS; // PF_RX_BUS_INDEX; 
				}
			//else if(InRange(iInput, 22,25) )  // "OOOO" outputs 1-4
			//	{
			//	InputBusType = OUTPUT_BUS;
			//	InputBusNum  = (iInput-22);
			//	}
			}
		else if( InRange(OutputBusNum, 4,MAX_OUTPUTS-1) ) // out 5,6 mixes (and 7,8?)
			{
			if(InRange(iInput, 0x1e,0x2d)  )  // HEY THIS MIGHT CHANGE TO 18 or 20 IN THE FUTURE!!!
				{
				InputBusType = DISK_BUS;    // HEY why does this index on the GUUI start at 1e?
	 			InputBusNum  = (iInput-0x1e);
				}
			else if(InRange(iInput, 4,5) )
				{
				InputBusType = RETURN_BUS;
				InputBusNum  = (iInput-4);
				}
			else if(InRange(iInput, 0x16,0x19) )
				{
				InputBusType = OUTPUT_BUS;
				InputBusNum  = (iInput-0x16);
				ActionMask = xptType ? XPT_ON : XPT_OFF;
				}
			}
		else
			{
			brkpt(cell->Row, "OUTbusSet");
			return;
			}
		goto fin;
		}//if(MixerBusType==OUTPUT_MIXER) 

	// there are 4 output bus buttons after the digital buttons on the DiskMix page (ON/OFF only)
	if( (MixerBusType==DISK_MIXER) ) // && ((InputBusType==PF_DIGITAL_BUS)||(InputBusType==DIGITAL_BUS)) && (InputBusNum>=8) )
		{ 
		// this is identical to outout 1-4 mix pages...
		if( InRange(iInput, 0,5) )   // AA|AA|RR   analog and return inputs
			{
			if(xptType & mMIX_POSTFADE)	InputBusType = ANALOG_BUS;
			else						InputBusType = PF_ANALOG_BUS;
			InputBusNum  = iInput;  // (iInput-4);
			}
		else if( InRange(iInput, 6,13) ) // DDDD|DDDD // AES inputs
			{
			if(xptType & mMIX_POSTFADE)	InputBusType = DIGITAL_BUS;
			else						InputBusType = PF_DIGITAL_BUS;
			InputBusNum  = iInput-6; 
			}
		else if( InRange(iInput, 14,21) ) // XXXX|XXXX // RXer inputs
			{
			if(xptType & mMIX_POSTFADE)	InputBusType = DIGITAL_BUS;
			else						InputBusType = PF_DIGITAL_BUS;
			InputBusNum  = iInput-14 + MAX_AES_INPUTS; // PF_RX_BUS_INDEX; 
			}
		else if(InRange(iInput, 0x16,0x19) )  // "OOOO" outputs 1-4
			{
			InputBusType = OUTPUT_BUS;
			InputBusNum  = (iInput-0x16);
			ActionMask = xptType ? XPT_ON : XPT_OFF;
			}
		else
			{
			brkpt(cell->Row, "DSKbusSet");
			return;
			}
		}

    // SetCrossPoint() is not handling this automatically it looks like....
	if(ActionMask == XPT_OFF)
		{ // ANALOG BUS and PreFader BUS should never be on at the same time for a given cross point
		if( (InputBusType==PF_DIGITAL_BUS) || (InputBusType==DIGITAL_BUS) )
			{ // clear all digital
			SetCrossPoint(MixerBusType, DIGITAL_BUS,    InputBusNum, OutputBusNum, XPT_OFF);
			SetCrossPoint(MixerBusType, PF_DIGITAL_BUS, InputBusNum, OutputBusNum, XPT_OFF);
			}
		else if( (InputBusType==PF_ANALOG_BUS) || (InputBusType==ANALOG_BUS) )
			{ // clear all analog
			SetCrossPoint(MixerBusType, ANALOG_BUS,    InputBusNum, OutputBusNum, XPT_OFF);
			SetCrossPoint(MixerBusType, PF_ANALOG_BUS, InputBusNum, OutputBusNum, XPT_OFF);
			}
		}

fin:
	SetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum, ActionMask);
	dprintf("SetCrosspoint: %s: %s, InBusNum=%d, OutBusNum=%d, ActMask=%s\n",GetMixerBusTypeString(MixerBusType), GetBusTypeString(InputBusType), InputBusNum, OutputBusNum, GetActionMaskString(ActionMask) );
	NVIsDirty(MIXINI);  
/*
	SavePreset(HEADPHONE2MIX_PRESET,    0); 
    SavePreset(ADV_HEADPHONE2MIX_PRESET,0); 
    SavePreset(TAPEMONOMIX_PRESET,      0); 
    SavePreset(ADV_TAPEMONOMIX_PRESET,  0); 
*/
	return;
} 

void SetTapeMonoCrosspointFromOasis(MixerType MixerBusType, int IsRight, InputBus InputBusType, MATRIX_CELL *cell)
{
	int channel = cell->Col;   	
	int BusEnum = cell->Row;
	int XpointAction = XPT_OFF;
    if(cell->Value == 1) XpointAction = XPT_ON;
   	if(cell->Value == 2) XpointAction = XPT_ON|XPT_INVERT;

	if(InputBusType==PF_ANALOG_BUS)
		channel += MIX_SOURCE_PF_RETURN;

   	SetCrossPoint(MixerBusType, InputBusType, channel, IsRight, XpointAction);
   	SavePreset(TAPEMONOMIX_PRESET,      0); 
   	SavePreset(ADV_TAPEMONOMIX_PRESET,  0); 
    NVIsDirty(MIXINI);
	return;
}

void SetHeadPhoneCrosspointFromPC(uchar preset, InputBus InputBusType, uchar channel, uchar PCenum) // MATRIX_CELL *cell)
{
	if( IsBadIndex(preset, numberof(Nomad.HphMon.Mem) ) ){brkpt(preset, "HPpresetBig");return;}
	HPHMEM *h = &Nomad.HphMon.Mem[preset]; 
	uchar nibbles = ConvertPChpEnumToHPnibble(PCenum);
	if(InputBusType==PF_ANALOG_BUS)
		{
		if( IsBadIndex(channel, numberof(h->Return)) )	{brkpt(channel, "HPcardBig");	return;	}
		h->Return[channel] = nibbles;
		}
	if(InputBusType==DISK_BUS)
		{
		if( IsBadIndex(channel, numberof(h->Card)) ) { brkpt(channel, "HPcardBig");return;}
		h->Card[channel] = nibbles;
		}
	if(InputBusType==OUTPUT_BUS)
		{
		if( IsBadIndex(channel, numberof(h->Out)) )	 { brkpt(channel, "HPoutBig");return;}
		// dont allow inverted phases here!
		h->Out[channel] = nibbles & (~UI_HP_LPHASE) & (~UI_HP_RPHASE); 
		}
	NVIsDirty(MIXINI);
	return;
}
  
// convert a corsspoint from GetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum) 
// to an enum used in the PC protocol
uchar ConvertXPTtoPCtrackmixVal(XPOINT_STATE *x, int IsDigital, int IsOutput)
{
	int v=0; // assume the xpt is off for now

	if(IsOutput)
		return logic(x->A);

	if(!IsDigital)
		{
		if(x->A==ANALOG)    v = mMIX_ASSIGNED | mMIX_POSTFADE; 
		if(x->A==PF_ANALOG) v = mMIX_ASSIGNED; 
		if(x->AInvrt)       v |= mMIX_PHASE; 
		}
	else
		{
		if(x->B==DIGITAL)    v = mMIX_ASSIGNED | mMIX_POSTFADE; 
		if(x->B==PF_DIGITAL) v = mMIX_ASSIGNED; 
		if(x->BInvrt)        v |= mMIX_PHASE; 
		}
	return v;
}

uchar ConvertMixMatrixtoPCheadphoneVal(MIX_MATRIX *m1, MIX_MATRIX *m2)
{
	int v1,v2;
	v1 = m1->enable; 
	if(m1->invert && v1)  v1++; 
	v2 = m2->enable; 
	if(m2->invert && v2)  v2++; 

	if( (v1==0) && (v2==0) ) return 0; //case 0: break;
	if( (v1==1) && (v2==0) ) return 1; //case 1: ActionMask[0] = XPT_ON;   												break;
	if( (v1==0) && (v2==1) ) return 2; //case 2: 									ActionMask[1] = XPT_ON;   			break;
	if( (v1==1) && (v2==1) ) return 3; //case 3: ActionMask[0] = XPT_ON;            ActionMask[1] = XPT_ON;  			break;
	if( (v1==2) && (v2==0) ) return 4; //case 4: ActionMask[0] = XPT_ON|XPT_INVERT;                                    	break;
	if( (v1==0) && (v2==2) ) return 5; //case 5:                                    ActionMask[1] = XPT_ON|XPT_INVERT; 	break;
	if( (v1==2) && (v2==1) ) return 6; //case 6: ActionMask[0] = XPT_ON|XPT_INVERT; ActionMask[1] = XPT_ON; 			break;
	if( (v1==1) && (v2==2) ) return 7; //case 7: ActionMask[0] = XPT_ON;            ActionMask[1] = XPT_ON|XPT_INVERT; 	break;
	if( (v1==2) && (v2==2) ) return 8; //case 8: ActionMask[0] = XPT_ON|XPT_INVERT; ActionMask[1] = XPT_ON|XPT_INVERT; 	break;
	return 0; // error
}



// send any scene take note changes to PC
void OasisSTNupdateHandler()
{
	volatile short i;
	short  fnum = disk[0].CurrentFolder;
	static short prevfnum = -1, PrevSeg = -1, initme=1;
	static char HpName[NHPHMEM][7];   // copy of  Headphone monitor routing memories from nomad.cpp
	static uchar HpEnable[NHPHMEM];
	static char prev_SCENE_CURRENT[DIR_SCENE_SZ+2], prev_TAKE_CURRENT[DIR_TAKE_SZ+2], prev_NOTE_CURRENT[DIR_NOTE_SZ+2];
	static char prev_SCENE_NEXT[DIR_SCENE_SZ+2],    prev_TAKE_NEXT[DIR_TAKE_SZ+2],    prev_NOTE_NEXT[DIR_NOTE_SZ+2];
	static float prevTrim[22];
	static int prevConState=0;

	if(initme)
		{
		for(i=0;i<numberof(prevTrim);i++)
			prevTrim[i] = -99;
		prev_SCENE_CURRENT[0]=11; // make sure this does not boot up as 9 somehow
		initme=0;
		}

	if( !PCGUIconnected() )
		{
		prevConState=0;
		return;
		}

	// init/destroy these when we come back from a lost connection
	if( (prevConState==0) && (prev_SCENE_CURRENT[0] != 9) )  
		{
		memset(HpName,   0, sizeof(HpName));
		memset(HpEnable, 0, sizeof(HpEnable));
		memset(prev_SCENE_CURRENT, 0, sizeof(prev_SCENE_CURRENT));
		memset(prev_TAKE_CURRENT,  0, sizeof(prev_TAKE_CURRENT));
		memset(prev_NOTE_CURRENT,  0, sizeof(prev_NOTE_CURRENT));
		memset(prev_SCENE_NEXT,    0, sizeof(prev_SCENE_NEXT));
		memset(prev_TAKE_NEXT,     0, sizeof(prev_TAKE_NEXT));
		memset(prev_NOTE_NEXT,     0, sizeof(prev_NOTE_NEXT));
		prev_SCENE_CURRENT[0]=9; // make sure thee are invalid strings
		prev_TAKE_CURRENT[0]=9;
		prev_NOTE_CURRENT[0]=9;
		prev_SCENE_NEXT[0]=9;
		prev_TAKE_NEXT[0]=9;
		prev_NOTE_NEXT[0]=9;	
		prevfnum = PrevSeg = -1;
		}

	prevConState=1;

	if(prevfnum != fnum)
		{
		int tot = MAX(folder[fnum].TotalSegs+2, folder[prevfnum].TotalSegs+2); // make sure we clear everytihng out!!
		SendOasisSegEntry(-1, "","",""); // clear the entire array
		for(i=1;i<tot;i++)
			gOasisSTNchanged[i] = 1; 
		gOasisSTNchanged[0] = 1;     
		SendSetting_String(SETTING_STORE_FOLDER,  folder[fnum].Name);
      		// send command to clear entire metdata array  
		}

	SERIAL_FIFO *pTx = &serport[RS422MAIN].tx;
	int prev = gQuietMode; gQuietMode=1;	
	// send any changes in any metadata
	int titems = ItemsInFifo(pTx);
	if( (titems < 190) && gOasisSTNchanged[0])  // 380 chars is 100ms worth of clogging at 38400 baud
		{
		int count = UI_CalcNextSegNum()-1; // ONLY do up to next seg // get the seg number that is considered the "NEXT SEG" at the moment
		for(i=count;i>0;i--) // do it backwards since it takes a few seconds and the last ones are the most important
			{
			if(gOasisSTNchanged[i])
				{
				gQuietMode=1; // dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
				SendOasisSegEntry(i, dir[i].Scene, dir[i].Take, dir[i].Note); // sends SetID = SETTING_METADATA_HISTORY
				gQuietMode=0; // dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
				gOasisSTNchanged[i]=0;
				titems = ItemsInFifo(pTx);
				if(titems > 190) // get_tx_items() > 190)
					goto fin; // break; // dont clog the TX buffer
				}
			}
		if(i==1)
			gOasisSTNchanged[0]=0; // we made it thru the whole folder			
		}

	// ----------send any changes in home screen segment info-------------
	int seg;
	if(GetTransportMode() == PLAY_MODE)	seg = UI_GetCurrentPlaySeg();
	else								seg = UI_GetCurrentRecSeg();
	if( (PrevSeg != seg) || (prevfnum != fnum) )
		{ 
		PrevSeg = seg;
		char str[30];
		sprintf(str, "%03d", (int)seg);
		SendSetting_String(SETTING_STORE_CURRENT,  str); // "current segment" on home screen

		sprintf(str, "%03d", (int)UI_GetCurrentRecSeg());
		SendSetting_String(SETTING_STORE_SEGMENT, str); // "total segs" on home screen
		}

	gQuietMode=1; // dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);

	if(strcmp(dir[seg].Scene,   prev_SCENE_CURRENT)) {SendSetting_String(SETTING_SCENE_CURRENT, dir[seg].Scene);  strcpy(prev_SCENE_CURRENT, dir[seg].Scene); }
	if(strcmp(dir[seg].Take,    prev_TAKE_CURRENT))  {SendSetting_String(SETTING_TAKE_CURRENT,  dir[seg].Take);	  strcpy(prev_TAKE_CURRENT , dir[seg].Take);  }
	if(strcmp(dir[seg].Note,    prev_NOTE_CURRENT))  {SendSetting_String(SETTING_NOTE_CURRENT,  dir[seg].Note);	  strcpy(prev_NOTE_CURRENT , dir[seg].Note);  }
 	if(strcmp(dir[seg+1].Scene, prev_SCENE_NEXT))	 {SendSetting_String(SETTING_SCENE_NEXT,    dir[seg+1].Scene);strcpy(prev_SCENE_NEXT,    dir[seg+1].Scene); }
	if(strcmp(dir[seg+1].Take,  prev_TAKE_NEXT))	 {SendSetting_String(SETTING_TAKE_NEXT,     dir[seg+1].Take); strcpy(prev_TAKE_NEXT ,    dir[seg+1].Take);  }
	if(strcmp(dir[seg+1].Note,  prev_NOTE_NEXT))	 {SendSetting_String(SETTING_NOTE_NEXT,     dir[seg+1].Note); strcpy(prev_NOTE_NEXT ,    dir[seg+1].Note);  }

	char *s; // check for changes in HP names
	for(i=0;i<12;i++) // numberof(gPresetCache.cHeadphonePresetName);i++)
		{
		s = Nomad_Get_HP_Name_StringPtr(i);
		int mismatch = strcmp(HpName[i], s);
		if(mismatch)
			{
			SendSetting_Array1D(SETTING_HPMIX_NAMES, s,12,i); // this needs to be a 0-based index!!
			strcpy(HpName[i], s);
			}
		}

	// update trims here since trims can be updated in so many places in nomad
	for(i=0;i<NUM_PC_INPUTS;i++) // 22
		{
		float curTrim = gChannelConfig[ConvertPCchanToNova(i)].Trim;
		if(prevTrim[i] != curTrim)
			{
			prevTrim[i] = curTrim;	
			SendSetting_1D_indexed(SETTING_INPUT_TRIM_LEVEL, i, prevTrim[i], NUM_PC_INPUTS); // convert to 1-based index
			}
		}
	gQuietMode=prev;


fin:
	prevfnum = fnum;
	return;
}

int GetPanModeForSendingToPC()
{
	int m1,m2,result;
	MESSAGE msg;
	void *context = NULL; // setting to NULL will allow these messages to be sent to PC
	msg.Type=MSG_VALUE_GET;
	OnSettingPanMode34(context, &msg);
	m1 = msg.Value.Unsigned;
	OnSettingPanMode56(context, &msg);
	m2 = msg.Value.Unsigned;  	
	result=PC_PAN_MODE_OFF;
	if((m1==1) && (m2==0)) result=PC_PAN_MODE_34  ; // 35
	if((m1==0) && (m2==1)) result=PC_PAN_MODE_56  ; // 56
	if((m1==1) && (m2==1)) result=PC_PAN_MODE_3456; // 3456
	return result;
}


MsgHandler(OnSettingToneEnables);
MsgHandler(OnSettingSlateEnables);

extern ALARM gPCGUI_SendAllToTabletTime;  // when SendalltoTablet() is called we need to extend our time out since it can take over 5 seconds to get another version string from PC
void SendAllToTablet() // aaa sss
{
	int i,j=0, result, count, dim, track;
	uint32 tmp32, ary32[64];
	XPOINT_STATE x; // ,x1,x2;
	MIX_MATRIX m1,m2;
	MIX_MATRIX *pm1,*pm2;
	uchar ary[64];
	char str[MAX_UI_TEXT+2];
	MATRIX_CELL cell;
	MESSAGE msg;
	void *context = NULL; // setting to NULL will allow these messages to be sent to PC
	//static int didit=0;
	//if( !IsConnected(PCGUI_CONNECTED))  // TABLET_CONNECTED|DEVA32_CONNECTED|OASIS_CONNECTED) )
	//	return;
	memset(&cell, 0, sizeof(cell));
	memset(&msg,  0, sizeof(msg));
	SERIAL_FIFO *pTx = &serport[RS422MAIN].tx;
	int64 startTime = gMSECs;
	int StartOutIndex = pTx->out;

	gPCGUI_SendAllToTabletTime = GMSECS;
	char timestr[20]; GetmsecsString(GMSECS, timestr);
	dprintf(cGREEN, "---- SendAllToTablet() ----- start %dms [%s]\n" , (int)(gMSECs-startTime), timestr);

	SendSetting_String(SETTING_STATUS_TEXT, "Loading 0%");
	RequestPCSetting(SETTING_HOME_METERS_COUNT);
	//RequestPCSetting(SETTING_INPUT_CHANNEL);

WaitForTXEmpty(pTx);
dprintf(cGREEN, "---- SendAllToTablet() ----- Elapsed1 %dms\n" , (int)(gMSECs-startTime));

	// assume this is not an array for now...
    msg.Type = MSG_VALUE_GET; 

	OnSettingSampleRate(context, &msg);		SendSetting_Byte(SETTING_SAMPLE_RATE,          msg.Value.Unsigned);
	OnSettingRecordPreroll(context, &msg);	SendSetting_Byte(SETTING_RECORD_PREROLL,       msg.Value.Unsigned);
	OnSettingTCrate(context, &msg);			SendSetting_Byte(SETTING_TIMECODE_FRAMERATE,   msg.Value.Unsigned);
	OnSettingTCincUB(context, &msg);		SendSetting_Byte(SETTING_TIMECODE_INCUSERBITS, msg.Value.Unsigned);
	gQuietMode=1; dprintf(cMAGENTA,"QUIET MODE IS ON...\n");

	OnSettingTCmode(context, &msg); 			SendSetting_Byte(SETTING_TIMECODE_RUNMODE	, msg.Value.Unsigned);
	OnSetting_BootJamDate(context, &msg);		SendSetting_Byte(SETTING_TIMECODE_AUTOJAM	, msg.Value.Unsigned); // Auto Date Jam
	OnSettingTCoutSelector(context, &msg); 		SendSetting_Byte(SETTING_TIMECODE_OUT		, !msg.Value.Unsigned);
	OnSettingTCdisplaySelector(context, &msg);	SendSetting_Byte(SETTING_TIMECODE_DISPLAYED	, !msg.Value.Unsigned);
	OnSetting_LED_Brightness(context, &msg);	SendSetting_Byte(SETTING_CONTROLLER_BACKLIGHT,msg.Value.Unsigned);
	OnSetting_ToneLevel(context, &msg);			SendSetting_Byte(SETTING_TONE_LEVEL		    , (char)msg.Value.Signed);
    SendSetting_Byte(SETTING_TONE_ENABLE, gNV.fToneOn); // send enum value to PC
	SendSetting_Byte(SETTING_OUTPUTMIX_PAN_FOLLOW, GetPanModeForSendingToPC() ); // THIS GETS A LITTLE COMPLICATED
	SendSetting_Byte(SETTING_METERS_COUNT,       clip(gNV.GUI_MetersCount,     6, 16));

	//gNV.GUI_HomeMetersCount = FindLastTrackEnablesIndex()+1;

// no longer send home meter count since it is chosen on the GUI. This could be misleading!
//	SendSetting_Byte(SETTING_HOME_METERS_COUNT,  clip(gNV.GUI_HomeMetersCount, 8, 16));

	SendSetting_Byte(SETTING_MP3RECORD_MODE, gNV.GUI_MP3Recmode); 

	SendSetting_Byte(SETTING_INPUT_CHANNEL,   ConvertNovaChanToPC(Nomad.Setup.Chan)); // old
	SendSetting_Byte(SETTING_INPUT_CONFIGURE, ConvertNovaChanToPC(Nomad.Setup.Chan)); // new for 0.85 - whats the difference?

	if(GetVersionFloat(OASIS_CONNECTED) >= 1.51F)
		SendBrightnessCommand(gNV.LEDtrimBrightness); 

	GotPCsettingCommand(); // reset PC's timeout in case we take too much time here

	SendSetting_String(SETTING_STATUS_TEXT, "Loading 10%");

	for(i=0;i<MAX_DISK_CHANNELS;i++)
		ary[i] = (0 != ((1<<i)&gNV.ISOattenMask));
	SendSetting_Array2Drow(SETTING_TRACKMIX_NEVERCLIP, 0, i, ary, 8,MAX_DISK_CHANNELS); // 12);   // send contents of gNV.ISOattenMask
	SendSetting_Byte(SETTING_NEVERCLIP_ATTENUATION, gNV.NeverClipShiftDB/6);

	SendSetting_Byte(SETTING_TRACKMIX_PRESET , 0 ); // force preset to zero all the time
	SendSetting_Array1D(SETTING_RECORD_TRACKS, gNV.fTracksToRecord, 32, 16);

	count = MAX(numberof(gNV.Notes), 10); // GUI has 10 note slots
   	for(i=0;i<count;i++)
		SendSetting_Array1D(SETTING_NOTE_PRESET, gNV.Notes[i], 10,  i);

	// do I need this here anymore???
	SendSetting_String(SETTING_STORE_FOLDER, folder[GetCurrentFolder()].Name);


GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
WaitForTXEmpty(100,pTx);
//sleep(10); // let the PC catch up
SendSetting_String(SETTING_STATUS_TEXT, "Loading 40%");
dprintf(cGREEN, "Elapsed2 %dms\n" , (int)(gMSECs-startTime));

	// -------------- headphone stuff ------------
	SendSetting_Byte(SETTING_HPMIX_PRESET, HPHMemIndex()); // 
#if  1 // ndef RX4
	int numPresets = 12; // numberof(Nomad.HphMon.Mem);
	//SETTING_HPMIX_NAMES   // this is handled in STNhandler    Text{12}         hpmix.names
	SendSetting_Byte(SETTING_HPMIX_PHASE, 0); // turn off Invert Phase button when re-starting
	int preset=0;
	SendSetting_Array2Drow(SETTING_HPMIX_TRACKS,  preset, 0,  ary, numPresets, 20); // clear entire array
	SendSetting_Array2Drow(SETTING_HPMIX_RETURNS, preset, 0,  ary, numPresets,  2); // clear entire array
	SendSetting_Array2Drow(SETTING_HPMIX_OUTPUTS, preset, 0,  ary, numPresets,  8); // clear entire array
	// send all 12 HP preset pages to PC
	int LastNonZeroEntry;
	for(preset=0;preset<numPresets;preset++)
		{
		HPHMEM *h = &Nomad.HphMon.Mem[preset];

		LastNonZeroEntry = -1;
		for(i=0;i<MAX_DISK_CHANNELS;i++)
			{
			ary[i] = ConvertHPnibbleToPCenum(h->Card[i]);		//gPrintSettingHex=1;
			if(ary[i]) LastNonZeroEntry = i;
			}
		if(LastNonZeroEntry>=0)
			SendSetting_Array2Drow(SETTING_HPMIX_TRACKS, preset, i, ary, numPresets,12);// Menu{12,20}	//gPrintSettingHex=0;

		LastNonZeroEntry = -1;
		for(i=0;i<2;i++)
			{
			ary[i] = ConvertHPnibbleToPCenum(h->Return[i]);
			if(ary[i]) LastNonZeroEntry = i;
			}
		if(LastNonZeroEntry>=0)
			SendSetting_Array2Drow(SETTING_HPMIX_RETURNS, preset, i, ary, numPresets,2);// Menu{12,2}	

		LastNonZeroEntry = -1;
		for(i=0;i<6;i++)
			{
			ary[i] = ConvertHPnibbleToPCenum(h->Out[i]);
			if(ary[i]) LastNonZeroEntry = i;
			}
		if(LastNonZeroEntry>=0)
			SendSetting_Array2Drow(SETTING_HPMIX_OUTPUTS, preset, i, ary, numPresets,8);// Menu{12,8}	
		}// for all presets

	for(i=0;i<numPresets;i++)
		ary[i] = Nomad.HphMon.Mem[i].enable;
	SendSetting_Array1D(SETTING_HPMIX_ENABLE, ary, 12, 12); // Menu{12}

#endif

GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
WaitForTXEmpty(100,pTx);
sleep(10); // let the PC catch up
SendSetting_String(SETTING_STATUS_TEXT, "Loading 50%");
SendNomadVersion(); // dont let PC think we disconnected.....
dprintf(cGREEN, "Elapsed3 %dms\n" , (int)(gMSECs-startTime));

	// OnSetting_ToneLevel(context, &msg);			SendSetting_Byte(SETTING_TONE_LEVEL		    , msg.Value.Unsigned);

	// these dont matter...
	//SendSetting_Byte(SETTING_TRACKMIX_FADER, 0); // turn off button when re-starting
	//SendSetting_Byte(SETTING_TRACKMIX_PHASE, 0); // turn off Invert Phase button when re-starting
	//SETTING_TRACKMIX_ISOMATRIX // I think this is for oasis only?

	for(i=0;i<MAX_DISK_CHANNELS;i++) // this was crashing PC.................................
		ary[i] = gDiskChanConfig[i].fCompressor; 
	SendSetting_Array1D(SETTING_TRACKMIX_COMP, ary, MAX_DISK_CHANNELS, MAX_DISK_CHANNELS);

    msg.Type = MSG_VALUE_GET; 
	msg.Value.Pointer = &cell;
	for(i=0;i<MAX_DISK_CHANNELS;i++)
		{
		cell.Col = i;
		OnSettingToneEnables(context, &msg);
		ary[i] = cell.Value;
		}
	SendSetting_Array1D(SETTING_TRACKMIX_TONE, ary, MAX_DISK_CHANNELS, MAX_DISK_CHANNELS);

	for(i=0;i<MAX_DISK_CHANNELS;i++)
		{
		cell.Col = i;
		OnSettingSlateEnables(context, &msg);
		ary[i] = cell.Value;
		}
	SendSetting_Array1D(SETTING_TRACKMIX_SLATE, ary, MAX_DISK_CHANNELS, MAX_DISK_CHANNELS);

GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
WaitForTXEmpty(pTx);
SendSetting_String(SETTING_STATUS_TEXT, "Loading 60%");
dprintf(cGREEN, "Elapsed4 %dms\n" , (int)(gMSECs-startTime));

//#if 1 // ndef RX4
	// ------------------------------ send track mix array -----------------------------------ttt
	memset(ary,0,sizeof(ary));

	int cc = ItemsInFifo(pTx);
 	SendSetting_Array3Drow(SETTING_TRACKMIX_MATRIX, 0, 0,  ary, 8,12,22); // (8,12,22) clear entire array?????
	cc = ItemsInFifo(pTx)-cc;
	dprintf("(track mix clear took %d chars)\n",cc); // 14 chars
	//	sleep(5000);

	int inputnum=0;
	//int bytestosend=MAX_DISK_CHANNELS;
	for(track=0;track<MAX_DISK_CHANNELS;track++)
		{
		for(i=0;i<6;i++) // analog 1-4 RET 1-2
			{
			x = GetCrossPoint(DISK_MIXER, INPUT_BUS, i, track);
			ary[i] = ConvertXPTtoPCtrackmixVal(&x, false, false);
			}
		inputnum=0;
		for(i=PC_AES_START;i<PC_AES_START+16;i++) // AES 1-8 and RX 1-8
			{
			x = GetCrossPoint(DISK_MIXER, INPUT_BUS, inputnum, track);
			ary[i] = ConvertXPTtoPCtrackmixVal(&x, true, false);
			inputnum++;
			}
		inputnum=0;
		for(i=PC_OUTPUT_START;i<PC_OUTPUT_START+4;i++) // output 1-4
			{
			x = GetCrossPoint(DISK_MIXER, OUTPUT_BUS, inputnum, track);
			ary[i] = ConvertXPTtoPCtrackmixVal(&x, false, true);
			inputnum++;
			}
		SendSetting_Array3Drow(SETTING_TRACKMIX_MATRIX, track, i, ary, 8,MAX_DISK_CHANNELS,22); 
		}//for(all 16 tracks)

	for(i=0;i<MAX_DISK_CHANNELS;i++)
		ary[i] = gDiskChanConfig[i].fCompressor;
	SendSetting_Array2Drow(SETTING_TRACKMIX_COMP, 0, i, ary, 1,MAX_DISK_CHANNELS); 

	for(i=0;i<MAX_DISK_CHANNELS;i++)
		{
		x = GetCrossPoint(DISK_MIXER, ST_BUS, ST_SLATE_OFFSET, i);
		ary[i] = logic(x.A); 
		}
	SendSetting_Array2Drow(SETTING_TRACKMIX_SLATE, 0, i, ary, 1,MAX_DISK_CHANNELS); 

	for(i=0;i<MAX_DISK_CHANNELS;i++)
		{
		x = GetCrossPoint(DISK_MIXER, ST_BUS, ST_TONE_OFFSET, i);
		ary[i] = logic(x.A); 
		}
	SendSetting_Array2Drow(SETTING_TRACKMIX_TONE, 0, i, ary, 1,MAX_DISK_CHANNELS); 

GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
SendSetting_String(SETTING_STATUS_TEXT, "Loading 70%");
WaitForTXEmpty(100,pTx);
dprintf(cGREEN, "Elapsed7 %dms\n" , (int)(gMSECs-startTime));
	int charcount = ItemsInFifo(pTx); // find out how many chars we sent to PC for the output matrix
	// --------------------------------- send OutputMix array -------------------------------ooo
	// Nova Remote's inputs look like this:  [0]Ana1-4, [4]RET1-2, [6]Dig1-8, [14]RX1-8 
	memset(ary,0,sizeof(ary));
	SendSetting_Array3Drow(SETTING_OUTPUTMIX_MATRIX, 0, 0,  ary, 1,8,50); // (1,6,18) clear entire array?????
	int out;
	for(out=0;out<4;out++) // just do outs 1-4
		{
		int j=0, IsDigital=0;
		for(i=0;i<6;i++) // analog 1-4 RET 1-2
			{
			x = GetCrossPoint(OUTPUT_MIXER, INPUT_BUS, i, out);
			ary[j++] = ConvertXPTtoPCtrackmixVal(&x, IsDigital, false);
			}
	    IsDigital=1;
		for(i=0;i<MAX_AES_INPUTS;i++) // ;i<6+4;i++) // digital 1-8
			{
			x = GetCrossPoint(OUTPUT_MIXER, INPUT_BUS, i, out);
			ary[j++] = ConvertXPTtoPCtrackmixVal(&x, IsDigital, false);
			}
		//j+=4; // skip over 4 extra digital inputs on PC side
		IsDigital=1;
		for(i=0;i<MAX_OUTPUTS;i++) //  rx 1-8
			{
			x = GetCrossPoint(OUTPUT_MIXER, INPUT_BUS, i+MAX_AES_INPUTS, out);
			ary[j++] = ConvertXPTtoPCtrackmixVal(&x, IsDigital, false);
			}
		SendSetting_Array3Drow(SETTING_OUTPUTMIX_MATRIX, out, j, ary, 1,8,50); 
		}//for(outputs 1-4)

	// this is strange. Through trial and error I found that tracks start at ary[30] PC_TRACK_START
	memset(ary,0,sizeof(ary));
	for(out=4;out<MAX_OUTPUTS;out++) // ----- just do outs 5,6,7,8 ------
		{
		i=0;
		for(j=0;j<2;j++) // Ret1-2
			{
			x = GetCrossPoint(OUTPUT_MIXER, RETURN_BUS, j, out);
			ary[j+4] = x.A ? mMIX_ASSIGNED : 0;    //   PC_RETURN_START
			}
		for(j=0;j<4;j++) // outs 1-4 as sources
			{
			x = GetCrossPoint(OUTPUT_MIXER, OUTPUT_BUS, j, out);
			ary[j+PC_OUTPUT_START] = x.A ? mMIX_ASSIGNED : 0; // (22)
			}
		for(j=0;j<16;j++) // tracks 1-16
			{
			x = GetCrossPoint(OUTPUT_MIXER, DISK_BUS, j, out);
			ary[j+PC_TRACK_START] = x.A ? mMIX_ASSIGNED : 0;  // (30)
			}
		i = j+PC_TRACK_START;//30; // last entry in ary[] (i is bytestosend)
		SendSetting_Array3Drow(SETTING_OUTPUTMIX_MATRIX, out, i, ary, 1,8,50); 
		}//for(outputs 1-4)

	memset(ary,0,sizeof(ary));
	for(i=0;i<MAX_OUTPUTS;i++)
		ary[i] = gOutputChanConfig[i].fCompressor;
	SendSetting_Array2Drow(SETTING_OUTPUTMIX_COMP, 0, i, ary, 1,MAX_OUTPUTS); 

	for(i=0;i<MAX_OUTPUTS;i++)
		{
		x = GetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_SLATE_OFFSET, i);
		ary[i] = logic(x.A);
		}
	SendSetting_Array2Drow(SETTING_OUTPUTMIX_SLATE, 0, i, ary, 1,MAX_OUTPUTS); 

	for(i=0;i<MAX_OUTPUTS;i++)
		{
		x = GetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TONE_OFFSET, i);
		ary[i] = logic(x.A);
		}
	SendSetting_Array2Drow(SETTING_OUTPUTMIX_TONE, 0, i, ary, 1,MAX_OUTPUTS); 

	for(i=0;i<MAX_OUTPUTS;i++)
		{
		x = GetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TALK1_OFFSET, i);
		ary[i] = logic(x.A);
		}
	SendSetting_Array2Drow(SETTING_OUTPUTMIX_COM1, 0, i, ary, 1,MAX_OUTPUTS); 
	// print how many chars we sent to PC for the output matrix
	int delta = ItemsInFifo(pTx)-charcount;
	dprintf(cCYAN, "%d chars sent for OUTPUTMIX stuff (%dms worth)\n", delta, (delta * 1000) / 3840 );

GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
//sleep(10); // let the PC catch up
SendSetting_String(SETTING_STATUS_TEXT, "Loading 80%");
//WaitForTXEmpty(100,pTx);
dprintf(cGREEN, "Elapsed8 %dms\n" , (int)(gMSECs-startTime));

#if 1 // ndef RX4


#ifdef USE_PC_BANKS
//gQuietMode=0; dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
	SendSetting_Byte(SETTING_MIXER_BANK, gNV.ExtBank);                 //  ----- send fader assigns -----
	int bank=0, EXTfadercount = 8, prevbank = gNV.ExtBank;
	for(bank=0;bank<3;bank++)
		{
		gNV.ExtBank = bank;
    	LoadPreset(FADERASSIGN_PRESET,     bank); 
		//LoadPreset(FADERASSIGN_AUX_PRESET, bank); 
		for(i=0;i<EXTfadercount;i++)
			{
			ary[i] = GetExtFad(i); // gExtFaderBanks[bank][i].InputNum; 
			}
		SendSetting_Array2Drow(SETTING_MIXER_SOURCE, bank, i, ary, 5,16); // 5 banks 15 faders
		}
	gNV.ExtBank = prevbank;
   	LoadPreset(FADERASSIGN_PRESET,     gNV.ExtBank); 
	//LoadPreset(FADERASSIGN_AUX_PRESET, gNV.Bank); 
//gQuietMode=1; dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
#endif
	
	const int numInputs = NUM_PC_INPUTS; //  (MAX_PROXY_INPUTS + MAX_RETURN_INPUTS + MAX_AES_INPUTS + MAX_RX_INPUTS)
	for(i=0;i<MAX_PROXY_INPUTS;i++) // (4)
		ary[i] = (0==gChannelConfig[ConvertPCchanToNova(i)].fMicLevel); // MIC/LINE level
	SendSetting_Array1D(SETTING_INPUT_LEVEL, ary, numInputs ,i);

	for(i=0;i<MAX_PROXY_INPUTS;i++)	
		ary[i] = gChannelConfig[ConvertPCchanToNova(i)].f48V;  // 48V 
	SendSetting_Array1D(SETTING_INPUT_POWER48V, ary, numInputs ,i);

	// send Delay On Off 
	for(i=0;i<numInputs;i++)	
		ary[i] = gChannelConfig[ConvertPCchanToNova(i)].DelayOnOff; 					
	SendSetting_Array1D(SETTING_INPUT_DELAY_ENABLE, ary, numInputs, i);

	for(i=0;i<numInputs;i++) // Delay Time  PC represents it in 0.1ms incremnts same as LUA
		ary[i] = ConvertDelay_Samples_To_Point1ms(gChannelConfig[ConvertPCchanToNova(i)].DelaySamples); 
	SendSetting_Array1D(SETTING_INPUT_DELAY_TIME, ary, numInputs ,i); 

	
	// WE HAVE A LOT OF DIGITAL STUFF MISSING HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	for(i=0;i<numInputs;i++) 	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].phase; 
		SendSetting_Array1D(SETTING_INPUT_PHASE, ary, numInputs ,i);

	for(i=0;i<numInputs;i++) 	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].fHPF;  // HPfilter On Off 
		SendSetting_Array1D(SETTING_INPUT_HPF_ENABLE, ary, numInputs ,i);

	for(i=0;i<numInputs;i++)	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].HighPassFreq;   // HPfreq 
		SendSetting_Array1D(SETTING_INPUT_HPF_FREQ, ary, numInputs ,i);

	for(i=0;i<numInputs;i++)	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].TxID;   
		SendSetting_Array1D(SETTING_INPUT_ZAXNET_ID, ary, numInputs ,i);

	for(i=0;i<numInputs;i++)	ary[i] = logic(gChannelConfig[ConvertPCchanToNova(i)].Mix8TrimMode);   
		SendSetting_Array1D(SETTING_INPUT_TRIM_MODE, ary, numInputs ,i);

	//fix LUA so we have sepeate on off stuff 
	for(i=0;i<numInputs;i++)	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].fCompressor;   // input compressor enable (not link group)
		SendSetting_Array1D(SETTING_INPUT_COMPRESS_ENABLE, ary, numInputs ,i);

	for(i=0;i<numInputs;i++)	ary[i] = gChannelConfig[ConvertPCchanToNova(i)].LinkGroup;   //  link group
		SendSetting_Array1D(SETTING_INPUT_COMPRESS_LINK, ary, numInputs ,i);

//gQuietMode=0; dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
	ary[0] = round(1000.0*gCompSet_IN.attack);
	SendSetting_Array1D(SETTING_INPUT_COMPRESS_ATTACK, ary, 3, 1); // just send frist val (array represents three different compressor types)
	ary[0] = round(1000.0*gCompSet_IN.decay);
	SendSetting_Array1D(SETTING_INPUT_COMPRESS_DECAY, ary, 3, 1);  
	ary[0] = round(gCompSet_IN.ratio*10.0);
	SendSetting_Array1D(SETTING_INPUT_COMPRESS_RATIO, ary, 3, 1);  
	ary[0] = round(gCompSet_IN.dbgain);
	SendSetting_Array1D(SETTING_INPUT_COMPRESS_GAIN, ary, 3, 1);   
	ary[0] = round(gCompSet_IN.threshold);
	SendSetting_Array1D(SETTING_INPUT_COMPRESS_THRESH, ary, 3, 1); 

SendSetting_String(SETTING_STATUS_TEXT, "Loading 90%");
//gQuietMode=0; dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
//david made these arrays wierd.
//these arrays have 18 (or is it 22 now?) presets and each has 2 entries: 1 for each notch filter !!
	SendSetting_Array2Drow(SETTING_INPUT_NOTCH_ENABLE, 0, 0, ary, numInputs,2); // clear entire array
	SendSetting_Array2Drow(SETTING_INPUT_NOTCH_FREQ,   0, 0, ary, numInputs,2); // clear entire array
	// notch1,2 enable
	for(i=0;i<numInputs;i++) // these array dimentions look backwards to me.....
		{
		ary[0] = logic(gChannelConfig[ConvertPCchanToNova(i)].EQ[0].type); // notch0 on/off
		ary[1] = logic(gChannelConfig[ConvertPCchanToNova(i)].EQ[1].type); // notch1 on/off
		SendSetting_Array2Drow(SETTING_INPUT_NOTCH_ENABLE, i, 2, ary, numInputs,2);   //SendSetting_Array1D(SETTING_INPUT_NOTCH_ENABLE, ary, 18,i);	
		}
	// notch1,2 freq
	for(i=0;i<numInputs;i++) // these array dimentions look backwards to me.....
		{
		ary[0] = (gChannelConfig[ConvertPCchanToNova(i)].EQ[0].frequency); // notch0
		ary[1] = (gChannelConfig[ConvertPCchanToNova(i)].EQ[1].frequency); // notch1
		SendSetting_Array2Drow(SETTING_INPUT_NOTCH_FREQ, i, 2, ary, numInputs,2); 
		}
#endif
	
	// preset names
	for(i=0;i<16;i++) // why does protocol.h say this is [16]?
		SendSetting_Array1D(SETTING_NAMES_PRESET_NAME, Nomad.Setup.TrackPresets[i].Name, 16, i); // 20 presets

/*
	for(i=0;i<16;i++)	
		ary32[i] = gNV.TrackPresetsColor_BOO[i];
	SendSetting_Array1D(SETTING_NAMES_PRESET_COLOR, ary32, 16,i);
*/
	// actual track names
	for(i=0;i<MAX_DISK_CHANNELS;i++)
		SendSetting_Array1D(SETTING_TRACK_NAME, Nomad.Setup.Tracks[i].Name, MAX_DISK_CHANNELS, i);

	for(i=0;i<numberof(gNV.ColorOfMeter_BOO);i++)	
		ary32[i] = gNV.ColorOfMeter_BOO[i];
	SendSetting_Array1D(SETTING_TRACK_COLOR, ary32, numberof(gNV.ColorOfMeter_BOO),i);



	SendSetting_int16(SETTING_METERS_METER_DECAY, 255); // 1000/10); // send some better default meter decay settings
	SendSetting_Byte(SETTING_METERS_METER_HOLD,  7);   // 70/10);   // 

GotPCsettingCommand(); // reset PC's timeout in case we take too much time here
SendSetting_String(SETTING_STATUS_TEXT, "Loading 100%");
dprintf(cGREEN, "Elapsed %dms - ItemsInFifo() = %d (%dms worth)\n" , (int)(gMSECs-startTime), ItemsInFifo(pTx), (ItemsInFifo(pTx)*10)/38);
//WaitForTXEmpty(100,pTx);
int charsSent = pTx->out - StartOutIndex;
if(charsSent < 0) charsSent += pTx->len; // wrap
int ee = (int)(gMSECs-startTime);
int charsPerSecond = (charsSent*1000)/ee;
dprintf(cGREEN, "Elapsed %dms  charsSent = %d (%dms worth)\n" ,ee , charsSent, (charsSent * 1000) / 3840);
dprintf(cGREEN, "  effective datarate = %d bits per second.\n" ,  charsPerSecond * 10);
 

gQuietMode=0; dprintf(cMAGENTA,"gQuietMode=%d...\n", gQuietMode);
	Nomad.Setup.Chan=0; // dont leave it at 10
	gSendAllExitTime = GMSECS; // let timeout handler know we need some extra time
	dprintf(cGREEN, "exiting...\n");
	return;
}//aaa




int GetHPmatrix(int HPisRight, int IsCardBus, MixerType HPmixertype, MATRIX_CELL *cell)
{
	XPOINT_STATE x;
//	MixerType HPmixertype=HEADPHONE_MIXER;
    int BusEnum = cell->Row; 
    int channel = cell->Col;
    //int XpointType = cell->Value;
    HPisRight = (HPisRight != 0); // force it to be boolean

//	if(IsHP2)
//	  {	  // this is for the HP2 page in the BUS menu system
//	  HPmixertype   =HEADPHONE2_MIXER;
//	  }

   if(IsCardBus)
      {
	  if(BusEnum==1) // channels 9-12 are on the second row in the HP CARD TRACKS pages
	     channel += 8;
      x = GetCrossPoint(HPmixertype, DISK_BUS, channel, HPisRight);
	  goto Done;
      }

    if((BusEnum==0)) // &&(channel<8)) // CARD BUS
        x = GetCrossPoint(HPmixertype, DISK_BUS, channel, HPisRight);

    if((BusEnum==1)&&(channel<6)) // OUT 
    	x = GetCrossPoint(HPmixertype, OUTPUT_BUS, channel, HPisRight);

    if((BusEnum==2)&&(channel<4)) // RETURNS
    	x = GetCrossPoint(HPmixertype, PF_ANALOG_BUS, channel+6, HPisRight); // was ANALOG_BUS

Done:
	cell->Value = (x.A==SET); // translate for UI
	if(x.AInvrt && cell->Value)  cell->Value++; // inverted

//   if((BusEnum==1)&&(channel<8)) // BUS - assume CARD bus for now
//	  cell->Value = HeadphoneMixMatrix[HPisRight][channel+5].enable; //  = XpointType; // BUs is just assigned as the CARD bus for now
//   if((BusEnum==2)&&(channel<5)) // RETURNS
 //	  cell->Value = HeadphoneMixMatrix[HPisRight][channel+0].enable; //  = XpointType; // BUs is just assigned as the CARD bus for now
   return cell->Value; // this routie can be used two ways now ...
}

int UI_NumRemoteTrackMeters()
{
	int num=0, x;
	if( IsConnected(OASIS_CONNECTED) )
		num=2;
	x = gPCGUIstate.nTrackMeters * gPCGUIstate.TrackMetersVisible;
	if( IsConnected(PCGUI_CONNECTED) & (x>num) ) 
		num = x;
	return num;
}

int UI_NumRemoteOutputMeters()
{
	int num=0, x;

	x = gPCGUIstate.nOutputMeters * gPCGUIstate.OutputMetersVisible;
	if( IsConnected(PCGUI_CONNECTED) & (x>num) ) 
		num = x;
	return num;
}

int UI_NumRemoteInputMeters()
{
	int num=0, x;

	if( IsConnected(OASIS_CONNECTED|MIX8_CONNECTED) )
		num =  GetNumOasisFaders(); // was 8

	x = gPCGUIstate.nInputMeters * gPCGUIstate.InputMetersVisible;
	if( IsConnected(PCGUI_CONNECTED) & (x>num) ) 
		num = x;
	return num;
}

int MatrixToLinear(int ncols, MATRIX_CELL *cell);
MATRIX_CELL *GetCellPtr(MESSAGE *msg);
// new tone enables matrix (just like RecEnables) 
MsgHandler(OnSettingToneEnables) 
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    XPOINT_STATE x; // bit field... see AudioMix.h
    int channel, action;

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
			x = GetCrossPoint(DISK_MIXER, ST_BUS, ST_TONE_OFFSET,  channel);
			cell->Value = MCELL_OFF; // assume its off for now
			if(x.A)
				cell->Value = MCELL_ON; // Slate Tone and TC are only on or off!!
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
			channel = MatrixToLinear(8, cell); // ncols 
			//	SendSetting_1D_indexed(SETTING_TRACKMIX_TONE, channel, cell->Value, 32);
		    if(cell->Value==0) action = XPT_OFF;
			else			   action = XPT_ON;
		    SetCrossPoint(DISK_MIXER, ST_BUS, ST_TONE_OFFSET, channel, action, TRUE);
			if(context != gPCcontext)
				UpdatePCwithNomadCrosspoint(DISK_MIXER, ST_BUS, ST_TONE_OFFSET, channel);
//			Update_ToneCardAssign();
			SavePreset(DISKMIX_PRESET,   gNV.DiskMixPresetNum);   
			NVIsDirty(MIXINI); // v7.89
			fDiskMixDirty=1;
            return MSG_DONE;
        default: break;
    }
    return 0;
}
// new slate enables matrix (just like RecEnables) 
MsgHandler(OnSettingSlateEnables) 
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);  
    XPOINT_STATE x; // bit field... see AudioMix.h
    int channel, action;

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
			x = GetCrossPoint(DISK_MIXER, ST_BUS, ST_SLATE_OFFSET,  channel);
			cell->Value = MCELL_OFF; // assume its off for now
			if(x.A)
				cell->Value = MCELL_ON; // Slate Tone and TC are only on or off!!
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
			channel = MatrixToLinear(8, cell); // ncols 
		    if(cell->Value==0) action = XPT_OFF;
			else			   action = XPT_ON;
		    SetCrossPoint(DISK_MIXER, ST_BUS, ST_SLATE_OFFSET, channel, action, TRUE);
			if(context != gPCcontext)
				UpdatePCwithNomadCrosspoint(DISK_MIXER, ST_BUS, ST_SLATE_OFFSET, channel);
				//SendSetting_1D_indexed(SETTING_TRACKMIX_SLATE, channel, cell->Value, 12);
			SavePreset(DISKMIX_PRESET,   gNV.DiskMixPresetNum);   
			fDiskMixDirty=1;
			NVIsDirty(MIXINI); // v7.89
            return MSG_DONE;
        default: break;
    }
    return 0;
}
// new COM enables matrix (just like RecEnables) 
MsgHandler(OnSettingComEnables) 
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);  
    XPOINT_STATE x; // bit field... see AudioMix.h
    int channel, action;

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
			x = GetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TALK1_OFFSET,  channel);
			cell->Value = MCELL_OFF; // assume its off for now
			if(x.A)
				cell->Value = MCELL_ON; // Slate Tone and TC are only on or off!!
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
			channel = MatrixToLinear(8, cell); // ncols 
		    if(cell->Value==0) action = XPT_OFF;
			else			   action = XPT_ON;
		    SetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TALK1_OFFSET, channel, action, TRUE);
			if(context != gPCcontext)
				UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, ST_BUS, ST_TALK1_OFFSET, channel);
				//SendSetting_1D_indexed(SETTING_OUTPUTMIX_COM1, channel, cell->Value, 6);
			SavePreset(OUTPUTMIX_PRESET,   gNV.OutputMixPresetNum);   
			fOutputMixDirty=1;
			NVIsDirty(MIXINI); // v7.89
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_InputLimiter_gain);  


int UnpackArray(const uchar *buf, uint32 len);

const short ISOattenTable[]={0, 6,12,18,24,25}; // menu list from PC
void SwapRowCol(MATRIX_CELL *cell)
{
	uint32 tmp = cell->Row;
	cell->Row = cell->Col;
	cell->Col = tmp;
}
	
#define UI_TONE_MODE   5 // toggle tone (this enum is for TransportSetState() / lua code only)!
void *gPCcontext = (void *)0x2; // make sure this is a unique, invalid pointer value (points to Nullporinter test memoory area)

int ApplySetting(uint16 SetID, uchar nidx, uchar *idx, uchar *arr, char *eol, MATRIX_CELL *cell)
{
	int i,j=0, result, action=XPT_OFF;
	uint32 tmp32, m1,m2;
	char str[MAX_UI_TEXT+2];
	MESSAGE msg;
	void *context = gPCcontext; // previously not used, but now I am using this to tell who called the msghandler now

    cell->Zdim  = idx[0]; //  page number ????
    cell->Col   = idx[1]; //  0 based index
	cell->Row   = idx[2]; //  0 based index
	if(nidx==1)
		{ // 1D arrays look at Col in NomadUIsettings.cpp
		cell->Col = cell->Zdim;
		cell->Zdim=0;
 	    dprintf(cMAGENTA, " C=%d ", (int)cell->Col);
		}
	else
    	dprintf(cMAGENTA, " ZCR(%d,%d,%d) ", (int)idx[0], (int)idx[1], (int)idx[2]);
	dprintf("%s", eol); 

	// assume this is not an array for now...
	msg.Value.Unsigned = cell->Value;
    msg.Type = MSG_VALUE_SET; // (Arrays use MSG_MATRIX_CELL_SET)

	action = XPT_OFF;
	if(msg.Value.Unsigned) 
		action = XPT_ON;

	//gCalledByPacket=1;
	switch(SetID){
#if defined(ALLOW_RX4_TOUCH) //  1 // def NOMAD
        case SETTING_SAMPLE_RATE   : 		OnSettingSampleRate(context, &msg);    	break; //  1 Menu         sample.rate - menu:44100:47952:48000:48048:88200:96000
        case SETTING_RECORD_PREROLL: 		OnSettingRecordPreroll(context, &msg); 	break; //  2 Number       record.preroll - range{0,10}
		case SETTING_TIMECODE_FRAMERATE:	OnSettingTCrate(context, &msg); 		break; // 77 Menu         timecode.framerate - menu:23.98:24:25:29.97NDF:29.97DF:30NDF:30DF
		case SETTING_TIMECODE_INCUSERBITS:	OnSettingTCincUB(context, &msg); 		break; // 78 On/Off 
		case SETTING_TIMECODE_RUNMODE:		OnSettingTCmode(context, &msg); 		break; // 79 Menu  timecode.runmode - menu: FreeRun : RecRun : CJAM ALL : CJAM-TC : CJAM-UB
		case SETTING_TIMECODE_AUTOJAM:		OnSetting_BootJamDate(context, &msg); 	break; // Auto Date Jam
		case SETTING_CONTROLLER_BACKLIGHT:	OnSetting_LED_Brightness(context, &msg);break; //
		case SETTING_MIXER_BANK:			OnSetting_ExtBank(context, &msg); 		break;
		case SETTING_METERS_COUNT:		gNV.GUI_MetersCount     = clip(msg.Value.Unsigned,6,16);NVIsDirty(DEVAINI); break; //

		//gNV.GUI_HomeMetersCount = FindLastTrackEnablesIndex()+1;
		case SETTING_HOME_METERS_COUNT: gNV.GUI_HomeMetersCount = clip(msg.Value.Unsigned,8,16);NVIsDirty(DEVAINI); break; //

		case SETTING_MP3RECORD_MODE:	gNV.GUI_MP3Recmode = msg.Value.Unsigned; 	            NVIsDirty(DEVAINI); break; //

		case SETTING_INPUT_CHANNEL:		
		case SETTING_INPUT_CONFIGURE:	
			Nomad.Setup.Chan = ConvertPCchanToNova(msg.Value.Unsigned); // convert GUI mapping to Nova (skip Nova's dummy channels)
			UI_UIsetupRefreshRequest(); 
			break;

		case SETTING_TIMECODE_GENERATOR_TC:	TCGenerator->SetTimeCode((char *)cell->Pointer); break; // manual jam
		case SETTING_TIMECODE_GENERATOR_UB:	TCGenerator->SetUserBits((char *)cell->Pointer); break; // manual jam

		case SETTING_TIMECODE_OUT:		
			msg.Value.Unsigned = !msg.Value.Unsigned;
			OnSettingTCoutSelector(context, &msg); 	
			break;// 75 Menu  timecode.out - menu:Disk:Generator
		case SETTING_TIMECODE_DISPLAYED:
			msg.Value.Unsigned = !msg.Value.Unsigned;
			OnSettingTCdisplaySelector(context, &msg);
			break;//76Menu  timecode.displayed - menu:Disk:Gen Stop:Generator

		case SETTING_TONE_LEVEL:	
			msg.Value.Signed = (char)msg.Value.Signed; // convert to signed char
			OnSetting_ToneLevel(context, &msg);		
			break;// in dB range = (-90 to 0)
		case SETTING_TONE_ENABLE:	
			gNV.fToneOn = msg.Value.Unsigned; 
			ToneOnOff(gNV.fToneOn); 
			SetToneMix(gNV.fToneOn);
			break;

		// these are displayed on the home screen of PC GUI
/*  this should never happen now....
		case SETTING_SCENE_CURRENT:		
			tmp32 = UI_CalcNextSegNum();
			SetScene(tmp32,  (char *)cell->Pointer);
			break;
		case SETTING_TAKE_CURRENT:		
			tmp32 = UI_CalcNextSegNum();
			SetTake(tmp32,  (char *)cell->Pointer);
			break;
		case SETTING_NOTE_CURRENT:		
			tmp32 = UI_CalcNextSegNum();
			SetNote(tmp32,  (char *)cell->Pointer);
			break;
*/
		case SETTING_SCENE_NEXT:		
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			tmp32 = UI_CalcNextSegNum();
			SetScene(tmp32,  (char *)cell->Pointer);
			SetTake(tmp32,   "1"); // reset take to 1
			break;
		case SETTING_TAKE_NEXT:		
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			tmp32 = UI_CalcNextSegNum();
			SetTake(tmp32,  (char *)cell->Pointer);
			break;
		case SETTING_NOTE_NEXT:			
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			tmp32 = UI_CalcNextSegNum();
			SetNote(tmp32,  (char *)cell->Pointer);
			break;

		case SETTING_OUTPUTMIX_PAN_FOLLOW:            //  32 // Menu             outputmix.pan.follow - menu:OFF:34:3456
			if(msg.Value.Unsigned==PC_PAN_MODE_OFF ) 	{m1=0; m2=0;}
			if(msg.Value.Unsigned==PC_PAN_MODE_34  ) 	{m1=1; m2=0;}
			if(msg.Value.Unsigned==PC_PAN_MODE_56  ) 	{m1=0; m2=1;}
			if(msg.Value.Unsigned==PC_PAN_MODE_3456) 	{m1=1; m2=1;}
			msg.Value.Unsigned = m1;	
			OnSettingPanMode34(context, &msg);
			msg.Value.Unsigned = m2;  	
			OnSettingPanMode56(context, &msg);
			break;

        case SETTING_TRACKMIX_NEVERCLIP:  //16 Menu{8,8}  trackmix.neverclip - menu:0dB:-6dB:-12dB:-18dB:-24dB
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
			// cell->Value = ISOattenTable[cell->Value];  leave it as bool value = on or off
        	OnSetting_ISOatten(context, &msg); // set/clr the bit in gNV.ISOattenMask 
        	break;

		case SETTING_NEVERCLIP_ATTENUATION:
			gNV.NeverClipShiftDB = msg.Value.Signed*6; // ISOattenTable[msg.Value.Signed]; // 0, 6, 12 etc  must be 6dB steps!!
			UpdateDiskShifters();
        	NVIsDirty(DEVAINI);   
			break;

		case SETTING_RECORD_TRACKS: 	//   3 // Menu{32}         record.tracks - menu::X
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
          	OnSettingRecEnables(context, &msg);    
			MetersConfigure(Get_UIHomePageVariant()); // refresh home page meters
          	break; 

		case SETTING_TRACKMIX_COMP:		// 13 // Menu{8,8}        trackmix.comp - menu:OUT:IN
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
			OnSetting_MatrixCardLimiter_enable(context, &msg);    
			break;

		case SETTING_TRACKMIX_TONE:		// 14 // Menu{8,8}        trackmix.tone - menu::X
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
			OnSettingToneEnables(context, &msg);
			break;

		case SETTING_TRACKMIX_SLATE:		// 15 // Menu{8,8}        trackmix.slate - menu::X
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
			OnSettingSlateEnables(context, &msg);
			break;

		case SETTING_TRACKMIX_MATRIX:	//  11 // Number{8,8,32}   trackmix.matrix - range{0,2047}
		  	SetCrosspointFromPC(DISK_MIXER, cell);
		    SavePreset(DISKMIX_PRESET, gNV.DiskMixPresetNum);   
    		fDiskMixDirty=1; // calls UpdateDiskShifters();
		  	break; 

		case SETTING_OUTPUTMIX_MATRIX:	//  22 // Number{8,8,32}   outputmix.matrix - range{0,2047}
		  	SetCrosspointFromPC(OUTPUT_MIXER, cell);
   			SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum);   
   			fOutputMixDirty=1; 
		  	break;

		case SETTING_OUTPUTMIX_COMP:	
		    msg.Type = MSG_MATRIX_CELL_SET;
			msg.Value.Pointer = (void *) cell;
			OnSetting_MatrixOutputLimiter_enable(context, &msg);    
			break;

		case SETTING_OUTPUTMIX_SLATE:		// 15 // Menu{8,8}        trackmix.slate - menu::X
		    SetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_SLATE_OFFSET, cell->Col, action);
			SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum); 
			fOutputMixDirty=1;
			break;

		case SETTING_OUTPUTMIX_TONE:	
		    SetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TONE_OFFSET, cell->Col, action);
			SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum); 
			fOutputMixDirty=1;
			break;

		case SETTING_OUTPUTMIX_COM1:	
		    SetCrossPoint(OUTPUT_MIXER, ST_BUS, ST_TALK1_OFFSET, cell->Col, action);
			SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum); 
			fOutputMixDirty=1;
			break;
		
		case SETTING_INPUT_LEVEL:
			Nomad.Setup.Chan = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupChanMicLine(context, &msg); 
			break;

		case SETTING_INPUT_POWER48V:
			Nomad.Setup.Chan = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetup48V(context, &msg); 
			break;


		case SETTING_INPUT_DELAY_ENABLE:
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupDelay_OnOff(context, &msg); 
			break;

		case SETTING_INPUT_DELAY_TIME: // GUI clicks in 0.1 ms incremernts  so 1 = 0,1ms and 10 = 1ms
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupDelay_MS(context, &msg); // always stored in samples!! but displayed in fractional ms on GUI
			break;

		case SETTING_INPUT_HPF_ENABLE:
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupHPF(context, &msg); 
			break;

		case SETTING_INPUT_HPF_FREQ:
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;//>>8;
			OnSettingSetupChan_DigHPF_Freq(context, &msg); 
			break;

		case SETTING_INPUT_PHASE: 
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupPhase(context, &msg); 
			break;

		case SETTING_INPUT_COMPRESS_ENABLE: // Menu{32}         input.compress.enable - menu:OFF:ON
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSetting_InputLimiter_enable(context, &msg); // NOMAD CAN DO 0,1,2,3 for Notlinked,ON,GroupA,GroupB,GroupC 
			break;
		case SETTING_INPUT_COMPRESS_LINK:   // Menu{32}         input.compress.enable - menu:OFF:ON
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSetting_InputLimiter_enable(context, &msg); // NOMAD CAN DO 0,1,2,3 for Notlinked,ON,GroupA,GroupB,GroupC 
			break;

		case SETTING_INPUT_COMPRESS_ATTACK: // not an array!!!!  input.compress.attack - range{0,100}
			OnSetting_InputLimiter_attack(context, &msg); 
			break;

		case SETTING_INPUT_COMPRESS_DECAY:  // Number{32}       input.compress.decay - range{0,100}
			OnSetting_InputLimiter_decay(context, &msg); 
			break;

		case SETTING_INPUT_COMPRESS_THRESH: // Number{32}       input.compress.thresh - range{-20,0}
			msg.Value.Signed    = (char)cell->Value; // always negative!!!
			OnSetting_InputLimiter_thresh(context, &msg); 
			break;

		case SETTING_INPUT_COMPRESS_RATIO: // Number{32}       input.compress.ratio - range{1.0,20.0,0.1}
			OnSetting_InputLimiter_ratio(context, &msg); 
			break;

		case SETTING_INPUT_COMPRESS_GAIN: // Number{32}       input.compress.gain - range{-12,12}
			OnSetting_InputLimiter_gain(context, &msg); 
			break;

		case SETTING_INPUT_ZAXNET_ID: // Number{18}
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSetting_InputTXID(context, &msg); 
			break;
		case SETTING_INPUT_TRIM_MODE: // Number{18}
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Unsigned  = cell->Value;
			OnSettingSetupMix8Trim(context, &msg); 
			break;
		case SETTING_INPUT_TRIM_LEVEL: // Number{18}
			Nomad.Setup.Chan    = cell->Col;	msg.Value.Signed = (char)cell->Value; // CAN BE NEGATIVE
			OnSettingSetupChanTrim(context, &msg); 
			break;

		case SETTING_INPUT_NOTCH_ENABLE: //// Menu{18,2}
			Nomad.Setup.Chan    = cell->Zdim; //	msg.Value.Unsigned  = cell->Value;
			if(cell->Col==0)	OnSettingSetupChanNotch1OnOff(context, &msg); 
			else				OnSettingSetupChanNotch2OnOff(context, &msg); 
			break;

		case SETTING_INPUT_NOTCH_FREQ: //// Menu{18,2}
			Nomad.Setup.Chan    = cell->Zdim; //	msg.Value.Unsigned  = cell->Value;
			if(cell->Col==0)	OnSettingSetupChanNotch1Freq(context, &msg); 
			else				OnSettingSetupChanNotch2Freq(context, &msg); 
			break;

		case SETTING_NOTE_PRESET: // Text{10}   
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			if(cell->Col >= numberof(Nomad.Setup.NotePresets))
				break; // should never be possble
			strcpy(Nomad.Setup.NotePresets[cell->Col].Name,  (char *)cell->Pointer); // NotePresetChangeMonitor() will detect this and save this nopte into gNV.Notes
			break;

/*
		// ---- TapeMono mix stuff -------
		case SETTING_TAPEMIX_TRACKS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 0, DISK_BUS, cell);    	break; 
		case SETTING_TAPEMIX_OUTPUTS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 0, OUTPUT_BUS, cell);  	break; 
		case SETTING_TAPEMIX_RETURNS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 0, PF_ANALOG_BUS, cell);	break; 
		case SETTING_MONOMIX_TRACKS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 1, DISK_BUS, cell);    	break; 
		case SETTING_MONOMIX_OUTPUTS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 1, OUTPUT_BUS, cell);  	break; 
		case SETTING_MONOMIX_RETURNS:	SetTapeMonoCrosspointFromOasis(TAPEMONO_MIXER, 1, PF_ANALOG_BUS, cell);	break; 

		// ---- HP2 mix stuff -------
		case SETTING_HPMIX2_TRACKS:		SetHeadPhoneCrosspointFromOasis(HEADPHONE2_MIXER, DISK_BUS, cell);     	break; 
		case SETTING_HPMIX2_OUTPUTS:	SetHeadPhoneCrosspointFromOasis(HEADPHONE2_MIXER, OUTPUT_BUS, cell);     break;        			
        case SETTING_HPMIX2_RETURNS:	SetHeadPhoneCrosspointFromOasis(HEADPHONE2_MIXER, PF_ANALOG_BUS, cell);  break; 
*/

		// ---- headphone page stuff -------
		case SETTING_HPMIX_PRESET: 	
			if(cell->Value<12)	
				{
				if( (cell->Value==11) && (GetHPmemIndex() == cell->Value) )
					{
					cell->Value=0; // kludge to force GUI to cycle round to 1st HP preset 
					SendSetting_Byte(SETTING_HPMIX_PRESET, cell->Value); 
					}
				SetHPmem(cell->Value, gPCcontext);	
				gHPrefreshRequest=1;
				} 
			break;
		case SETTING_HPMIX_NAMES:
			{
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			char *pstr = Nomad_Get_HP_Name_StringPtr(cell->Col); // this has error checking in it
          	strcpy(pstr, (const char *)cell->Pointer);
			NVIsDirty(MIXINI);
			break;
			}

		case SETTING_HPMIX_ENABLE: 	if(cell->Col<numberof(Nomad.HphMon.Mem)){ Nomad.HphMon.Mem[cell->Col].enable = cell->Value; NVIsDirty(MIXINI);	} break;
		case SETTING_HPMIX_TRACKS:	SetHeadPhoneCrosspointFromPC(cell->Zdim, DISK_BUS,      cell->Col, cell->Value); break; 
		case SETTING_HPMIX_OUTPUTS: SetHeadPhoneCrosspointFromPC(cell->Zdim, OUTPUT_BUS,    cell->Col, cell->Value); break; 
		case SETTING_HPMIX_RETURNS: SetHeadPhoneCrosspointFromPC(cell->Zdim, PF_ANALOG_BUS, cell->Col, cell->Value); break; 
//		case SETTING_HPMIX_ZNET_SELECT:	if(cell->Col < 12)	{gPresetCache.HeadphoneRxNum[cell->Col] = cell->Value;  NVIsDirty(MIXINI); } break; // store  gPresetCache.HeadphoneRxNum in HP MIX PRESET section

#ifndef RX4
		// -------- fader assign -------
		case SETTING_MIXER_SOURCE:
			{
			int bank = clip(cell->Zdim, 0, NUM_FADER_PRESETS-1), FaderNum = cell->Col, RawUserInputNum = cell->Value, prevbank=0;
			prevbank = gNV.Bank;
			//if(RawUserInputNum < 20)
			//	gExtFaderBanks[bank][FaderNum].InputNum = RawUserInputNum;
			if(gNV.Bank==bank)
				SetExtFad(FaderNum, RawUserInputNum, gPCcontext);
			else
				{// gPresetCache.FaderAssignMatrix[NUM_FADER_PRESETS][NUM_TOTAL_FADERS][MAX_IN_CHAN_SOURCES];
				gNV.Bank = bank; // temporarily load in a different preset
				//gPresetCache.AuxFaderConfig[bank][RawUserInputNum].iFaderTrim  = -1; // we dont support this yet on ext faders
				//gPresetCache.AuxFaderConfig[bank][RawUserInputNum].iZaxNetTrim = -1;
		    	LoadPreset(FADERASSIGN_PRESET,     bank); 
				//LoadPreset(FADERASSIGN_AUX_PRESET, bank); 
				SetExtFad(FaderNum, RawUserInputNum, gPCcontext); // this saves the preset using gNV.Bank
				gNV.Bank = prevbank; // get our real current bank # back
		    	LoadPreset(FADERASSIGN_PRESET,     gNV.Bank); 
				//LoadPreset(FADERASSIGN_AUX_PRESET, gNV.Bank); 
				}
   	        NVIsDirty(MIXINI); // save the preset to NV
			break;
			}

//SETTING_FADERS_MODE
//SETTING_FADERS_ZAXNET_ID
#endif


		case SETTING_METADATA_HISTORY:
			{ // key pair strings
			if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
			char **m = (char **)cell->Pointer;
			if(!m[0]){ dprintf(cRED, "ERROR Got Null m[0] from PC\n");	break; }
			dprintf("got:[%d] %s=<%s> %s=<%s> %s=<%s>\n", cell->Col, m[0],m[1],m[2],m[3],m[4],m[5]);
			if(m[0][0]=='n')SetNote( cell->Col+1, m[1]);
			if(m[2][0]=='n')SetNote( cell->Col+1, m[3]);
			if(m[4][0]=='n')SetNote( cell->Col+1, m[5]);
			if(m[0][0]=='s')SetScene( cell->Col+1, m[1]);
			if(m[2][0]=='s')SetScene( cell->Col+1, m[3]);
			if(m[4][0]=='s')SetScene( cell->Col+1, m[5]);
			if(m[0][0]=='t')SetTake( cell->Col+1, m[1]);
			if(m[2][0]=='t')SetTake( cell->Col+1, m[3]);
			if(m[4][0]=='t')SetTake( cell->Col+1, m[5]);
			break;
  	     	} 

		case SETTING_TRACK_NAME:
			if(cell->Col < numberof(Nomad.Setup.Tracks))
				{
				if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
				int sz =  sizeof(gNV.MeterLabel[0]); //21
				char tmpstr[20]; memcpy(tmpstr, cell->Pointer, 20);
				tmpstr[19]=0;
				dprintf("sz=%d, len=%d\n", sz, strlen(tmpstr) );
				strncpy(Nomad.Setup.Tracks[cell->Col].Name, tmpstr, sizeof(Nomad.Setup.Tracks[0].Name));
				strncpy(gNV.MeterLabel[cell->Col],          tmpstr, sizeof(gNV.MeterLabel[0]));
				gMetersConfigRequest=1;
		        NVIsDirty(DEVAINI);
				}
			break;

		case SETTING_TRACK_COLOR:
			if(cell->Col < numberof(gNV.ColorOfMeter_BOO) )
				{
				gNV.ColorOfMeter_BOO[cell->Col] = cell->Value; // 24bit color code
				gMetersConfigRequest=1;
		        NVIsDirty(DEVAINI);
				}
			break;

		case SETTING_NAMES_PRESET_NAME: // track preset names
			if(cell->Col < numberof(gNV.TrackPresets) )
				{
				if(!cell->Pointer){ dprintf(cRED, "ERROR Got Null Pointer from PC\n");	break; }
				strncpy(Nomad.Setup.TrackPresets[cell->Col].Name, (char *)cell->Pointer, sizeof(Nomad.Setup.TrackPresets[0].Name));
				strncpy(gNV.TrackPresets[cell->Col],              (char *)cell->Pointer, sizeof(gNV.TrackPresets[0]));
				gMetersConfigRequest=1;
		        NVIsDirty(DEVAINI);
				}
			break;
		case SETTING_NAMES_PRESET_COLOR:
			if(cell->Col < numberof(gNV.TrackPresetsColor_BOO) )
				{
				gNV.TrackPresetsColor_BOO[cell->Col] = cell->Value; // 24bit color code
				gMetersConfigRequest=1;
		        NVIsDirty(DEVAINI);
				}
			break;
#endif // NOMAD

		  default:
			dprintf(cRED, "No Handler for SetID#%d <%s>\n",SetID,GetSettingString(SetID));
			break;
		}

	//gCalledByPacket=0;
    return 1;
}
	

void UpdatePCwithNomadCrosspoint(MixerType MixerBusType, InputBus InputBusType, int InputBusNum, int OutputBusNum)
{
#if defined(NOMAD) || defined(ALLOW_RX4_TOUCH)
   	XPOINT_STATE x; // bit field... see AudioMix.h
   	MATRIX_CELL cell;
	cell.Value=0;

	if(InputBusType == PF_RETURN_BUS) { InputBusNum += MIX_SOURCE_RETURN_OFFSET; InputBusType = PF_ANALOG_BUS; }
	if(InputBusType == RETURN_BUS)    { InputBusNum += MIX_SOURCE_RETURN_OFFSET; InputBusType = ANALOG_BUS; }

	x = GetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum);

    if( (MixerBusType==OUTPUT_MIXER) && (InputBusType==ST_BUS) )
       	{
		cell.Value = logic(x.A); 	cell.Row=0; cell.Col=OutputBusNum;
		if(InputBusNum==ST_SLATE_OFFSET)SendSetting_2D_indexed_CR(SETTING_OUTPUTMIX_SLATE,&cell, 1, MAX_OUTPUTS);
		if(InputBusNum==ST_TONE_OFFSET)	SendSetting_2D_indexed_CR(SETTING_OUTPUTMIX_TONE, &cell, 1, MAX_OUTPUTS);
		if(InputBusNum==ST_TALK1_OFFSET)SendSetting_2D_indexed_CR(SETTING_OUTPUTMIX_COM1, &cell, 1, MAX_OUTPUTS);
	   	goto Finish;
	   	}
    if( (MixerBusType==OUTPUT_MIXER) && ((InputBusType==ANALOG_BUS) || (InputBusType==PF_ANALOG_BUS)) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim  = 0;
		cell.Col   = InputBusNum;  // analog(5)
		cell.Row   = OutputBusNum; // out(3)
		if(x.A==ANALOG)    cell.Value = mMIX_ASSIGNED|mMIX_POSTFADE;  
		if(x.A==PF_ANALOG) cell.Value = mMIX_ASSIGNED;
		if(x.AInvrt)       cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_OUTPUTMIX_MATRIX, &cell, 1,8,46); // 50);
	   	goto Finish;
		}
    else if( (MixerBusType==OUTPUT_MIXER) && ((InputBusType==DIGITAL_BUS) || (InputBusType==PF_DIGITAL_BUS)) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim  = 0;
		//if(InputBusNum>3)
		//	InputBusNum+=4; // if RX bus then skip over the extra 4 digital inputs on the GUI side
		cell.Col   = InputBusNum+6;  // digital(5)
		cell.Row   = OutputBusNum;   // out(3)
		if(x.B==DIGITAL)    cell.Value = mMIX_ASSIGNED|mMIX_POSTFADE;  
		if(x.B==PF_DIGITAL) cell.Value = mMIX_ASSIGNED;
		if(x.BInvrt)        cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_OUTPUTMIX_MATRIX, &cell, 1,8,46); // 50);
	   	goto Finish;
		}
    else if( (MixerBusType==OUTPUT_MIXER) && (InputBusType==DISK_BUS) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim  = 0;
		cell.Col   = InputBusNum+30; // disk() starts at 30 in GUI
		cell.Row   = OutputBusNum;   // out(3)
		if(x.A)      cell.Value = mMIX_ASSIGNED;
		if(x.AInvrt) cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_OUTPUTMIX_MATRIX, &cell, 1,8,46); // 50);
	   	goto Finish;
		}
    else if( (MixerBusType==OUTPUT_MIXER) && (InputBusType==OUTPUT_BUS) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim  = 0;
		cell.Col   = InputBusNum+0x16; // out() starts at 0x16 in GUI
		cell.Row   = OutputBusNum;   // out(3)
		if(x.A)      cell.Value = mMIX_ASSIGNED;  // always prefader!?!?!?!??!
		if(x.AInvrt) cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_OUTPUTMIX_MATRIX, &cell, 1,8,50);
	   	goto Finish;
		}
    else if( (MixerBusType==DISK_MIXER) && (InputBusType==ST_BUS) )
       	{
		cell.Value = logic(x.A); 	cell.Row=0; cell.Col=OutputBusNum;
		if(InputBusNum==ST_SLATE_OFFSET)SendSetting_2D_indexed_CR(SETTING_TRACKMIX_SLATE,&cell, 1, 12);
		if(InputBusNum==ST_TONE_OFFSET)	SendSetting_2D_indexed_CR(SETTING_TRACKMIX_TONE, &cell, 1, 12);
		//if(InputBusNum==ST_TALK1_OFFSET)SendSetting_2D_indexed_CR(SETTING_TRACKMIX_COM1, &cell, 1, 12);
	   	goto Finish;
	   	}
    else if( (MixerBusType==DISK_MIXER) && ((InputBusType==ANALOG_BUS) || (InputBusType==PF_ANALOG_BUS)) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim = 0;
		cell.Col = InputBusNum;  // analog(5)
		cell.Row = OutputBusNum; // out(3)
		if(x.A==ANALOG)    cell.Value = mMIX_ASSIGNED|mMIX_POSTFADE;  
		if(x.A==PF_ANALOG) cell.Value = mMIX_ASSIGNED;
		if(x.AInvrt)       cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_TRACKMIX_MATRIX, &cell,8,12,22);
	   	goto Finish;
		}
    else if( (MixerBusType==DISK_MIXER) && ((InputBusType==DIGITAL_BUS) || (InputBusType==PF_DIGITAL_BUS)) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim = 0;
		cell.Col = InputBusNum+10;  // digital
		cell.Row = OutputBusNum;    // out
		if(x.B==DIGITAL)    cell.Value = mMIX_ASSIGNED|mMIX_POSTFADE;  
		if(x.B==PF_DIGITAL) cell.Value = mMIX_ASSIGNED;
		if(x.BInvrt)        cell.Value |= mMIX_PHASE; 
		SendSetting_3D_indexed(SETTING_TRACKMIX_MATRIX, &cell, 8,12,22);
	   	goto Finish;
		}
    else if( (MixerBusType==DISK_MIXER) && (InputBusType==OUTPUT_BUS) )
		{
		cell.Value = 0; // assume its off for now
		cell.Zdim = 0;
		cell.Col = InputBusNum+22; // (0x16)       was 6+4+8;  // skip past digital inputs  6+4+8=18
		cell.Row = OutputBusNum;   // out(3)
		if(x.A) cell.Value = mMIX_ASSIGNED|mMIX_POSTFADE;  
		SendSetting_3D_indexed(SETTING_TRACKMIX_MATRIX, &cell,8,12,22);
	   	goto Finish;
		}

	dprintf(cRED, "UpdatePCwithNomadCrosspoint() did nothing: %s: %s, InputBusNum=%d OutputBusNum=%d \n", GetMixerBusTypeString(MixerBusType), GetBusTypeString(InputBusType), (int)InputBusNum, (int)OutputBusNum);
Finish:
#endif
	return;
}

void Dump_RS422_TX_buffer()
{
	SetColor(cGREEN);
	dprintf("(Dump-TXbuf)\n");
}

