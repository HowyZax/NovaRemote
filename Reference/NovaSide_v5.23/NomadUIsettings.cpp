#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "TypeDefs.h"
#include "deva.h"
#include "devatypes.h"
#include "transport.h"
#include "Globals.h"
#include "Hardware.h"
#include "Hardware_FRONT.h"
#include "DRIVES.h"

#include "AudioMix.h"
#include "Effects.h"
#include "NVData.h"
#include "NVData.h"

#include "AudioPeak.h"
#include "ZaxNet.h"
#include "DiskManager.h"
#include "AES.h" // for SetSampleRate()
#include "Timecode.h"
#include "FAT.h"
#include "nvdataMIRROR.h"
#include "Mirror.h"

#include "LCD_QRX.h"
#include "N_SCIport.h"
#include "DiskAPI.h"
#include "Clock.h"
#include "AudioIO.h"
#include "MiscTools.h"
#include "FactoryCodes.h"
#include "MP3common.h"
#include "debug.h" // must be last to insure assert is defined properly!!!
//#include "AutoMixer.h"
#include "Remote.h"
#include "SharedRam.h"
#include "NomadUIsettings.h"
#include "RealTimeclock.h" // for FOLDER
#include "RemoteOasis.h"
#include "protocol.h" // for oasis serial settings codes

//#include "SerialPorts.h"
//#include "ZaxLan.h"

#include "NOMADincludes.h"
#include "..\UI\SOURCE\auto\message.h"
#include "..\UI\SOURCE\auto\uiTables.hpp_" // for detecting which screen we are in

#define EXPORT 

#define GET_IT  TRUE
#define SET_IT  FALSE

int zsnprintf(char *buffer, size_t count, const char *format, ... );

#define EXT_ZNET_START 11 // where the FP8 ZNET unit codes start

extern TimeCode *TCGenerator;
extern TimeCode *TCReader;
extern TimeCode *TCDisk;
extern double   FrameRateTbl[];
extern TimeCode *TCGenSelected;
extern TimeCode *TCDisplayed;

short gBankChangeFreezeFadersmooth=0; // dont change trims or fader positions when changing banks for a second
//int gUI_BusTrackOffset=0; // for tellijng card msgHandlers which tracks they are looking at

//extern struct NV_MIRROR_SETTINGS gMirrorDrive[4];  // #2 is the only one used in Deva...  Internal HD drive, USB, CF, SDCARD
//extern struct MIRRORCMDS gMIRcmds;    // global flags to trigger FW operations
//extern struct MIRROR_STATUS mstat;    // MIRROR status
//extern struct MIRROR_CONFIG mcfg;     // MIRROR setup

extern ushort DMcurRecSeg;         // current segment being recorded
extern ushort DMcurFolder;         // current folder
extern ushort DMgotoseg;
extern short  gPreRecordBufferIsDirty;

extern const char *gSoftwareVersion;
extern struct PRESETS gPresetCache;
MESSAGE gDummyUImsg;

float SetNVTrimDelta(unsigned  channel, float deltaDB); // from AudioIO.cpp
float GetNVTrim(unsigned  channel);
void  SetNVTrim(unsigned  channel, float trim);

void SendTrackNamesToUI();
void SendNotePresetsToUI();
void SendTrackPresetsToUI();

void InitBusGainSettings();

void ForceIFBfreqIsolated(int x); // use this wizely (check IFB mode)?
// do not use this for frequency changes
//int UI_SendIFBparm32(int *p32);     // this waits for the command to be ready to be sent (max of 20ms)
int UI_GetRawTrackIsEnabled(int i); //  the absolute truth about which tracks are enabled for recording
int UI_HomeScreenIsCompact();       // from UIhome.cpp. tells NomadUIsettings.cpp how many status lines we have room for

void SendSetting_2D_indexed_CR(uint16 SetID, MATRIX_CELL *cell, uchar dim1, uchar dim2); 
void SendSetting_Array2Drow(uint16 SetID, uchar row, uchar bytestosend, uchar *ary, uchar dim1, uchar dim2);
void UIWirelessInit();
int  UI_GetBusGainSetting_EnumToGain(int enumval);
int  UI_GetBusGainSetting_GainToEnum(int gain); // return an enum
int  ConvertNomadXpointToCellValue(XPOINT_STATE x, MixerType type, InputBus bus, int iInput, int iOutput);

/////////////////////////////////  this came from nomad.h //////////////////////
// nomad.h is automatically created by makeui.bat and contains fake
// placeholders for anything that was added to the lua file
// Merely adding getset = 'UI_PreRecordTime', added some magical stuff
// and it added fake calls to UI_PreRecordtime() in nomad.cpp so WIN32 version can run
// with its own fake calls to UI_PreRecordtime()


/****************************************************************************/
/* DATA */
/****************************************************************************/

NOMAD_STATE Nomad;
//POPUP gPopUp;

#define GET_STATE(ptr)  NOMAD_STATE *ptr = &Nomad


/****************************************************************************/
/* CODE */
/****************************************************************************/
void UI_init_INI_stuff(); // also called by RESTORE PRESET
void UpdateNomadStruct(void);

void NomadClear(void) // called at top of main() only (BEFORE NV INIT)
{
	memset(&Nomad, 0, sizeof(Nomad)); // RX4:  who is suppose to init this!>!?!?!
	UIWirelessInit();

}
EXPORT void NomadInit(void)
{
    int i;
    GET_STATE(p);
#ifndef SIMULATE
//	memset(&Nomad, 0, sizeof(Nomad)); // RX4:  who is suppose to init this!>!?!?!
//	memset(p, 0, sizeof(NOMAD_STATE));
	
	memset(&MetersMain, 0, sizeof(MetersMain)); // RX4:  who is suppose to init this!>!?!?!
	memset(&gDummyUImsg, 0, sizeof(gDummyUImsg));
    NomadSetTrackCount(gMachineInfo.MaxRecordChannels);

    zsnprintf(p->About.Model,  10, "%s",   gMachineInfo.FullModelName);// "NOMAD");
	if(gMachineInfo.SoftwareOptions & SOPT_NOVA_TOUCH)
		strcat(p->About.Model, "(T)");

    zsnprintf(p->About.Serial,           9, "%05X", gMachineInfo.SerialNumber); // "01000");
    zsnprintf(p->About.Version.Software, 9, "%s", gSoftwareVersion);            // "v0.82");
    zsnprintf(p->About.Version.Dsp,      7, "%X", gMachineInfo.DSPboardRevision);
    //zsnprintf(p->About.Version.TemperatureString,    7, "%X", gMachineInfo.AudioPLDrevision);
	GetRTC_Temperature(Nomad.About.Version.TemperatureString, gNV.UseCelcius);	// for about screen  --    { 'TEMPERATURE',    setting='about.temperature' }, -- int 
    zsnprintf(p->About.Version.Main,     7, "%X", gMachineInfo.MainPLDrevision); 

	for (i = 0; i < numberof(p->Setup.Channel); i++) 
        p->Setup.Channel[i].ZNetFreq_BOO = GetDefault_ZNetFreq(i); // 5400;
	UpdateNomadStruct();
    Nomad.Metadata.CurSeg = 1; // which seg to display or edit on metadata page 

	UI_init_INI_stuff();
	SetRecordChannels(gNV.fTracksToRecord); // update Nomad struct

	Nomad.Settings.ButtonHoldTimeMS  = gNV.ButtonHoldTimeMS;
	Nomad.Settings.SlatePageFlipMode = gNV.SlatePageFlipMode;

	Nomad.Settings.Allow_HOMEPAGE_ENG    = gNV.Allow_HOMEPAGE_ENG   ;
	Nomad.Settings.Allow_HOMEPAGE_O6     = gNV.Allow_HOMEPAGE_O6    ;  
	Nomad.Settings.Allow_HOMEPAGE_CARD   = gNV.Allow_HOMEPAGE_CARD  ;  
	Nomad.Settings.Allow_HOMEPAGE_O2CARD = gNV.Allow_HOMEPAGE_O2CARD; 
	Nomad.Settings.Allow_HOMEPAGE_I4P4   = gNV.Allow_HOMEPAGE_I4P4  ; 
	Nomad.Settings.Allow_HOMEPAGE_O4R2   = gNV.Allow_HOMEPAGE_O4R2  ; 
	Nomad.Settings.Allow_HOMEPAGE_D12    = gNV.Allow_HOMEPAGE_D12   ; 

	Nomad.Settings.UseOldTrackMixPage    = gNV.UseOldTrackMixPage;
	Nomad.About.IsNova2                  = gIsNova2;
	Nomad.Settings.TxBattInMeter         = gNV.TxBattInMeter;
	Nomad.Settings.TxGainInMeter         = gNV.TxGainInMeter;
	// NVCHANGE For duplicate settings in gNV and nomad.*  
	// if Nomad state needs updating from gNV do it here...!!!!!!!!!!!!!!!!!!!!!


#else  // ------------ SIMULATE -----------------------the following is for the simulator only!!!
    
    NomadSetTrackCount(12); // assume NOMAD 12 for now

    strcpy(p->About.Model, "Nova1");
	if(gIsNova2)
	    strcpy(p->About.Model, "Nova2");
//	if(gMachineInfo.SoftwareOptions & SOPT_NOVA_TOUCH)
		strcat(p->About.Model, "(T)");
		
    // setup nomad 12 feature set as default
    Nomad.Settings.HasFeatureZaxnet =1;
    Nomad.Settings.HasFeatureAES    =1;
    Nomad.Settings.HasFeatureUSBport=Nomad.Settings.HasFeatureMP3=1;
    Nomad.Settings.HasFeatureMIX8   =1;

    strcpy(p->About.Serial, "01234");
    strcpy(p->About.Version.Software, "v4.50");
    strcpy(p->About.Version.Dsp,      "B");
    strcpy(p->About.Version.Audio,    "--");
    strcpy(p->About.Version.Main,     "57");
    strcpy(p->About.Version.IFB,      "0.33");

    p->Metadata.CurSeg = 3;

    strcpy(p->Metadata.Scene, "123A");
    strcpy(p->Metadata.Take, "100");
    strcpy(p->Metadata.Note, "I LOVE NOVA"); 

		i=0;	// NOTE presets [20]
		strcpy(p->Setup.NotePresets[i++].Name,"Aircraft"); 
		strcpy(p->Setup.NotePresets[i++].Name,"Car");
		strcpy(p->Setup.NotePresets[i++].Name,"Circle Take");
		strcpy(p->Setup.NotePresets[i++].Name,"Ambiance");
		strcpy(p->Setup.NotePresets[i++].Name,"Tone -20.000");
		strcpy(p->Setup.NotePresets[i++].Name,"Wild Track");
		strcpy(p->Setup.NotePresets[i++].Name,"MOS");
		strcpy(p->Setup.NotePresets[i++].Name,"No Good");
		strcpy(p->Setup.NotePresets[i++].Name,"Good");
		strcpy(p->Setup.NotePresets[i++].Name,"Room Tone");
		strcpy(p->Setup.NotePresets[i++].Name,"Wild Lines");
		strcpy(p->Setup.NotePresets[i++].Name,"PFX");
		strcpy(p->Setup.NotePresets[i++].Name,"Mis-Slated");
		strcpy(p->Setup.NotePresets[i++].Name,"No Slate");
		strcpy(p->Setup.NotePresets[i++].Name,"Tails Sticks");
		strcpy(p->Setup.NotePresets[i++].Name,"Fan");
		strcpy(p->Setup.NotePresets[i++].Name,"Interview");
		strcpy(p->Setup.NotePresets[i++].Name,"OTF");
		strcpy(p->Setup.NotePresets[i++].Name,"VO"); // WARNING we are alomost out of slots! that was slot 19!!!

		i=0;// 	Track Names [20]
		strcpy(p->Setup.TrackPresets[i++].Name,"Mix");
		strcpy(p->Setup.TrackPresets[i++].Name,"Mix-L");
		strcpy(p->Setup.TrackPresets[i++].Name,"Mix-R");
		strcpy(p->Setup.TrackPresets[i++].Name,"Boom");
		strcpy(p->Setup.TrackPresets[i++].Name,"Mid");
		strcpy(p->Setup.TrackPresets[i++].Name,"Side");
		strcpy(p->Setup.TrackPresets[i++].Name,"Lav");
		strcpy(p->Setup.TrackPresets[i++].Name,"Plant");
		strcpy(p->Setup.TrackPresets[i++].Name,"Car");
		strcpy(p->Setup.TrackPresets[i++].Name,"Music");
		strcpy(p->Setup.TrackPresets[i++].Name,"Playback");

    strcpy(p->Metadata.StatusLine1,"Mount FAT32");            // Status Line 1");
    strcpy(p->Metadata.StatusLine2,"16.0GB (6MB free)");      // Status Line 2");
    strcpy(p->Metadata.StatusLine3,"Mirroring is OFF");       // Status Line 3");
    
    p->Settings.HomePageVariantBank = 0;
    p->Settings.HomePageVariant[0] = 3;
    p->Settings.HomePageVariant[1] = 2;
    p->Transport.Shifted = 1;

boo    for (i=0; i < numberof(p->Setup.Channel); i++) {
       	p->Setup.Channel[i].Trim = 0; // i*2;          // just for display only....
        p->Setup.Channel[i].Freq = GetDefault_ZNetFreq(i); // 5400;
    }

    for(i=0;i < numberof(p->Mirror); i++) {
        p->Mirror[i].StartSeg = 1;
        p->Mirror[i].EndSeg   = 999;
    }

    for(i=0;i<4;i++)
       Nomad.Settings.fTracksToRecord[i]=true; // turn on 4 tracks

    Nomad.Settings.VfadCycleCount = 4; //  ALL  (valid values are 0-4) (0 = none)
    
    // make some fake segs and folders
    UI_SetCurrentCardFolder(1); // folder 0 is invalid
    for(i=0;i<numberof(Nomad.Folder);i++)
       {
       Nomad.Folder[i].TotalSegs = 0; 
       Nomad.Folder[i].SizeLBA   = 0;
       sprintf(Nomad.Folder[i].Name, "Z%03d", i);
       }          
    Nomad.Folder[1].TotalSegs = 5; 
    Nomad.Folder[1].SizeLBA   = 432332;
    Nomad.Folder[2].TotalSegs = 3; 
    Nomad.Folder[2].SizeLBA   = 32332;
    for(i=0;i<numberof(Nomad.HphMon.Mem);i++)
        {
        Nomad.HphMon.Mem[i].index = i;
        sprintf(Nomad.HphMon.Mem[i].Name, "HP %d", i+1);
        }

    UI_UpdatePlaybackInfo();
    UI_ClearPopUp();
    Nomad.Settings.ISOattenMask   = 3;   // simualtes gNV.ISOattenMask
    Nomad.Settings.NeverClipShiftDB = 6; // simualtes gNV.NeverClipShift
    Nomad.Settings.SampleRateHZ = 48000; // ugghh
boo	Nomad.Settings.IsNova2      = 0;     // nnn
#endif //SIMULATE

//    UIWirelessInit(); // thjis can no longer happen here since this cgets called AFTER NV init
    return;
}

void UI_init_INI_stuff() // also called by RESTORE PRESET
{
   int i;
   SendTrackNamesToUI();
   SendNotePresetsToUI();
   SendTrackPresetsToUI();

   InitBusGainSettings();
	for(i=0;i<numberof(Nomad.HphMon.Mem);i++)// this will always put a good string in there
		{
		if(Nomad.HphMon.Mem[i].Name[0] == 0)
		   sprintf(Nomad.HphMon.Mem[i].Name, "HP %d", i+1);
	   	}
	return;
}

extern  const char *pFrameRateStrTbl[]; //  = {"23.98","24","25","29.97NDF","29.97DF","30NDF","30DF"};
extern  int    gMCPrestoreCount;
// this gets called every 100ms to update the Nomad struct for misc variables that need to be sent to the UI
void UpdateNomadStruct(void)
{
	static int PrevPreRecordtime = -1;
	static int PrevFrameRate = -1;
    GET_STATE(p);

    Nomad.Settings.HasFeatureZaxnet  = gMachineInfo.AllowIFB;
    Nomad.Settings.HasFeatureAES     = gMachineInfo.AllowAES;
    Nomad.Settings.HasFeatureUSBport_BOO = gMachineInfo.AllowUSB;
    Nomad.Settings.HasFeatureMP3_BOO     = gMachineInfo.AllowMP3;
	Nomad.Settings.HasFeatureMIX8    = gMachineInfo.AllowMIX8; // only for old PLD handling
    Nomad.Settings.IsLiteModel       = (gMachineInfo.ModelNumber == MODEL_NOMADLITE);
	Nomad.Settings.SampleRateHZ      = gNV.InternalSampleRate; // make sure LUA knows what sample rate this is

    if(PrevFrameRate != gNV.FrameRate)
		{  
        GetFrameRateString(p->Metadata.TimecodeRate, gNV.FrameRate);
		PrevFrameRate = gNV.FrameRate;
		}

	if(PrevPreRecordtime != gNV.PreRecordTime)
		{
		sprintf(p->Metadata.Pre, "%d", (int)gNV.PreRecordTime);
		PrevPreRecordtime = gNV.PreRecordTime;
		}

    UI_GetStatusString(p->Metadata.StatusLine1, STATUS_STR_LINE1, 29);
    UI_GetStatusString(p->Metadata.StatusLine2, STATUS_STR_LINE2, 29);
    UI_GetStatusString(p->Metadata.StatusLine3, STATUS_STR_LINE3, 29);
	Nomad.Transport.Shifted = gNV.TransportShifted;

	Nomad.Settings.Allow_HOMEPAGE_ENG    = gNV.Allow_HOMEPAGE_ENG   ;    // 182
	Nomad.Settings.Allow_HOMEPAGE_O6     = gNV.Allow_HOMEPAGE_O6    ;     // 183
	Nomad.Settings.Allow_HOMEPAGE_CARD   = gNV.Allow_HOMEPAGE_CARD  ;   // 184
	Nomad.Settings.Allow_HOMEPAGE_O2CARD = gNV.Allow_HOMEPAGE_O2CARD;   // 185
	Nomad.Settings.Allow_HOMEPAGE_I4P4   = gNV.Allow_HOMEPAGE_I4P4  ;   // 186
	Nomad.Settings.Allow_HOMEPAGE_O4R2   = gNV.Allow_HOMEPAGE_O4R2  ;   // 187
	Nomad.Settings.Allow_HOMEPAGE_D12    = gNV.Allow_HOMEPAGE_D12   ;    // 188

	Nomad.Settings.UseOldTrackMixPage    = gNV.UseOldTrackMixPage;
	Nomad.Settings.TxBattInMeter  = gNV.TxBattInMeter;
	Nomad.Settings.TxGainInMeter  = gNV.TxGainInMeter;

	//NVCHANGE ------- put duplicate GNV and Nomad state updates here? -------------

	#ifdef RX4
	gMachineInfo.SoftwareOptions |= SOPT_NOMAD_TOUCH;
	#endif

//    strcpy(Nomad.About.Version.IFB, gIFBsoftwareVersion);  // keep ABOUT screen up to date
	#ifdef NOMAD
    Nomad.About.KeyFix = gMCPrestoreCount; // keep track of MCP chip fix errors
	#endif
	Nomad.ZNet.TxEnable = gNV.IFB_mode;
    return;
}

int UI_Get_MuteFrame1TCtest()
{
	return Nomad.Settings.MuteFrame1TCtest;
}

#ifndef WIN32
int zsnprintf(char *buffer, size_t count, const char *format, ... )
{
	va_list arglist;
	va_start(arglist,format);

	vsprintf(buffer,format,arglist);
    buffer[count-1] = '\0';
	return strlen(buffer); // count;
}
#endif


EXPORT int UI_IsSlatePageActive(void) // called by NOMAD code to see if slate page is running
{
   GET_STATE(p);
   int tmp = p->Settings.SlatePageIsActive;
//   p->Settings.SlatePageIsActive=0; // reset back to zero when called!!!
   return tmp;
}

EXPORT void UI_SlatePageDetect(int x) // called by UI code to indicate the slate page is active (called when a TCGEN msg is recevied)
{          
   GET_STATE(p);
   p->Settings.SlatePageIsActive=x;
   return;
}

   // set something to freeze display for 2 seconds...  
int gSlateFreezeReq=0;
EXPORT void UI_SetSlateFreezeRequest(int TriggerRequest)
{                                                  
     gSlateFreezeReq = TriggerRequest;
}
EXPORT int UI_GetSlateFreezeRequest()
{                                                  
     return gSlateFreezeReq;
}

               
MATRIX_CELL *GetCellPtr(MESSAGE *msg)
{
	if( (MsgType(msg) == MSG_MATRIX_CELL_GET)  || 
	    (MsgType(msg) == MSG_MATRIX_CELL_SET)  || 
	    (MsgType(msg) == MSG_MATRIX_CELL_MENU) || 
	    (MsgType(msg) == MSG_MATRIX_CURSOR) ||
        (MsgType(msg) == MSG_MATRIX_CELL_ROW_TITLE) ||
        (MsgType(msg) == MSG_MATRIX_CELL_COL_TITLE)) {
		return (MATRIX_CELL*)MsgPtr(msg);
    }
//	brkpt(MsgType(msg),"GetCellPtr");
	return (MATRIX_CELL*)&gDummyUImsg; // return some valid poirter that will not be used anyway
}
EXPORT MATRIX_CELL *MsgMatrixCell(MESSAGE *msg)
{
    return GetCellPtr(msg);
}

                          
EXPORT int Get_UIHomePageVariant()
{
   return gNV.StartScreen; // p->Settings.HomePageVariantBank;
}

EXPORT void Save_UIHomePageVariant(unsigned x)
{
	gNV.StartScreen = x; 
	gRTCram.StartScreen555x = 0x5550 | (0x7 & gNV.StartScreen);
   //NVIsDirty(DEVAINI); // dont save this every time a screen is changed ! It happensimmediately.
}

#ifdef NOMAD
EXPORT int Get_UIHomePageVariantBank()
{
   GET_STATE(p);
   int index = Get_UIHomePageVariantBank(); // p->Settings.HomePageVariantBank;
   return gNV.HomePageVariant[index];
}
EXPORT void Set_UIHomePageVariantBank(int bank)
{
   GET_STATE(p);
   int index = Get_UIHomePageVariantBank(); // p->Settings.HomePageVariantBank;
   assert(index < numberof(gNV.HomePageVariant));
   if(x>MAX_HOMEPAGE_VARIANTS) gNV.HomePageVariant[index] = MAX_HOMEPAGE_VARIANTS;
   else                        gNV.HomePageVariant[index] = x;
   NVIsDirty(DEVAINI);
}
#endif
                           

EXPORT UInt8 NomadTrackCount(void)
{
    return Nomad.About.NTracks;
}
/* Set track count (for simulator) */
EXPORT void NomadSetTrackCount(UInt8 ntracks)
{
    assert(ntracks > 0 && ntracks <= MAX_DISK_CHANNELS); // 16); 12);
    assert(ntracks <= numberof(Nomad.Setup.Tracks));
    Nomad.About.NTracks = ntracks;
}

int16 gTrackNameChange=0; // for debug
void TrackNameChangeMonitor() // WE NEED TO SEND NV NAMES TO NOMAD ON BOOT UP!
{
   int i,j;
   //return;///////////
   int count = min(numberof(gNV.MeterLabel), numberof(Nomad.Setup.Tracks)); 
   for(i=0;i<count;i++)
      {
      if(strcmp(gNV.MeterLabel[i], Nomad.Setup.Tracks[i].Name))
	     {
	     //strcpy(gNV.MeterLabel[i], Nomad.Setup.Tracks[i].Name);	  // WHY DOESNT THIS COPY THE STRINGS??
		 for(j=0;j<MAX_METER_LABEL_STR_SIZE;j++)
		    {
		    gNV.MeterLabel[i][j] = Nomad.Setup.Tracks[i].Name[j];
			if(gNV.MeterLabel[i][j]==0)
			   break;
			}
		 gNV.MeterLabel[i][MAX_METER_LABEL_STR_SIZE]=0; // just in case
//		 if(i<12) SendSetting_Array1D(SETTING_TRACK_NAME, Nomad.Setup.Tracks[i].Name, 12, i);
		 if(i<MAX_DISK_CHANNELS) SendSetting_Array1D(SETTING_TRACK_NAME, Nomad.Setup.Tracks[i].Name, MAX_DISK_CHANNELS, i);
         NVIsDirty(DEVAINI);
		 gTrackNameChange++;
		 }
	  }
}

void SendTrackNamesToUI()
{
   int i,j;
   int count = min(numberof(gNV.MeterLabel), numberof(Nomad.Setup.Tracks)); 
   for(i=0;i<count;i++)
     strcpy(Nomad.Setup.Tracks[i].Name, gNV.MeterLabel[i]);
}


short gTrackPresetChange=0; // for debug
void TrackPresetChangeMonitor() // WE NEED TO SEND NV NAMES TO NOMAD ON BOOT UP!
{
   int i,j,k;
   for(i=0;i<numberof(gNV.TrackPresets);i++)//(20)
      {
      if(strcmp(gNV.TrackPresets[i], Nomad.Setup.TrackPresets[i].Name))
	     {
	     //strcpy(gNV.MeterLabel[i], Nomad.Setup.TrackPresets[i].Name);	  // WHY DOESNT THIS COPY THE STRINGS??
		 for(j=0;j<numberof(gNV.TrackPresets[i]);j++)
		    {
		    gNV.TrackPresets[i][j] = Nomad.Setup.TrackPresets[i].Name[j];//  } TrackPresets[MAX_NOTE_PRESETS];
			if(gNV.TrackPresets[i][j]==0)
			   break;
			}
		 k = numberof(gNV.TrackPresets[0]); // k = max string length
		 gNV.TrackPresets[i][k] = 0; // just in case
		 if(i<16)
		 	SendSetting_Array1D(SETTING_NAMES_PRESET_NAME, Nomad.Setup.TrackPresets[i].Name, 16, i); // 20 presets
         NVIsDirty(DEVAINI);
		 gTrackPresetChange++;
		 }
	  }
}
void SendTrackPresetsToUI()
{
    int i,j;
   for(i=0;i<numberof(Nomad.Setup.TrackPresets);i++)
     strcpy(Nomad.Setup.TrackPresets[i].Name, gNV.TrackPresets[i]);

}

short gNotePresetChange=0; // for debug
void NotePresetChangeMonitor() // WE NEED TO SEND NV NAMES TO NOMAD ON BOOT UP!
{
   int i,j,k;
   for(i=0;i<numberof(gNV.Notes);i++)//(20)
      {
      if(strcmp(gNV.Notes[i], Nomad.Setup.NotePresets[i].Name))
	     { // we found a string difference
		 for(j=0;j<numberof(gNV.Notes[i]);j++)
		    {
		    gNV.Notes[i][j] = Nomad.Setup.NotePresets[i].Name[j];//  } NotePresets[MAX_NOTE_PRESETS];
			if(gNV.Notes[i][j]==0)
			   break;
			}
		if(i<10)
			SendSetting_Array1D(SETTING_NOTE_PRESET, gNV.Notes[i], 10, i);

         NVIsDirty(DEVAINI);
		 gNotePresetChange++;
		 }
	  }
}
void SendNotePresetsToUI()
{
    int i,j;
   for(i=0;i<numberof(Nomad.Setup.NotePresets);i++)
     strcpy(Nomad.Setup.NotePresets[i].Name, gNV.Notes[i]);

}

unsigned int PackedTCtoBCD(unsigned int PackedTC);


int ByteToBCD(int byte)
{
   byte &= 0xff;
   return ((byte/10)<<4) | (byte%10);
}

 
unsigned int PackedTCtoBCD(unsigned int PackedTC)
{
   volatile unsigned int hh,mm,ss,ff, bcd;
   ff = ByteToBCD(PackedTC&0xFF);
   PackedTC >>= 8;
   ss = ByteToBCD(PackedTC&0xFF);
   PackedTC >>= 8;
   mm = ByteToBCD(PackedTC&0xFF);
   PackedTC >>= 8;
   hh = ByteToBCD(PackedTC&0xFF);
   bcd = (hh<<24) | (mm<<16) | (ss<<8) | ff;   
   return bcd;
}

int BCDtoInt(int bcd)
{
   int i,tmp,multiplier=1;
   tmp=0;
   for(i=0;i<8;i++)
      {
	  tmp += (bcd & 0xF)*multiplier;
	  multiplier *= 10;
	  bcd >>= 4;
	  }
   return tmp;
}


unsigned int UI_GetTimecodePacked(TimeCode *p, int FrameInc) // how many frmes to inc by (for display sync delay)
{
   volatile uint32 TCbcd, TC32;
   TC32  = p->GetTimeCodePacked(FrameInc);
   TCbcd = PackedTCtoBCD(TC32);
   return TCbcd;
}

unsigned int UI_GetTimecodePacked_Disk(int FrameInc)
{
   return UI_GetTimecodePacked(TCDisk, FrameInc);
}


void UI_Timecode_BCDtoHHMMSS(int *hh, int *mm, int *ss, int *ff, uint32 TCbcd)
{
    *ff = BCDtoInt(TCbcd&0xFF);	TCbcd>>=8;
    *ss = BCDtoInt(TCbcd&0xFF);	TCbcd>>=8;
    *mm = BCDtoInt(TCbcd&0xFF);	TCbcd>>=8;
    *hh = BCDtoInt(TCbcd&0xFF);	
}

extern int16 gEnableDebugScreen;
extern short gCDS_TABLE_DEBUG; // set this to create a CDS gain table (not for CDScal[] calibration)

MsgHandler(OnSettingTCGenTimecode)
{
    static TC timecode;
	int hh,mm,ss,ff;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = UI_GetTimecodePacked(TCGenerator, 0); // timecode++;
            return MSG_DONE;
        case MSG_VALUE_SET:
		    UI_Timecode_BCDtoHHMMSS(&hh, &mm, &ss, &ff, (uint32)msg->Value.Unsigned);
			switch(DecodeFactoryTimecode((uint32)msg->Value.Unsigned)) // entry values are private (MiscTools.cpp)
				{
				case FACT_ENTRY_SECRET:
					Nomad.Settings.SecretFactoryMode=1;	// this allows us to burn the machine info struct!!
				    PopUp("setup",1);
				    break;
				case FACT_ENTRY_USER1:				  // users cannot change machine info here
					Nomad.Settings.UserFactoryMode=1; // this is a less serious factory mode for tuning VCO etc
				    PopUp("user fact mode 1",2);
				    break;
				case FACT_ENTRY_USER7:
					Nomad.Settings.UserFactoryMode=7; 
				    PopUp("user fact mode 7",2);
				    break;
				case FACT_ENTRY_USER17:
					Nomad.Settings.UserFactoryMode=17; 
				    PopUp("user fact mode 17",2);
				    break;
				case FACT_ENTRY_CDS_DEBUG:
	  				gCDS_TABLE_DEBUG=1;  // make sure we are in cal mode....
				    PopUp("CDS CELL DEBUG",2);
				    break;
				default: break;
				}

            TCGenerator->SetTimeCode(hh,mm,ss,ff);
            return MSG_DONE;

        default: break;
    }
    return 0;
}


MsgHandler(OnSettingTCGenUserbits)
{
    static TC timecode;
	int hh,mm,ss,ff;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = TCGenerator->GetUserBits();
            return MSG_DONE;
        case MSG_VALUE_SET:
		    TCGenerator->SetUserBits1((uint32)msg->Value.Unsigned);
			if(Nomad.Settings.SecretFactoryMode)
				{
				uint32 arg = 0;
				switch(DecodeFactoryUserbits((uint32)msg->Value.Unsigned, &arg)) // values are private (MiscTools.cpp)
					{
					case FACT_UB_SERIAL:
						{
					    gMachineInfo.SerialNumber = arg;	
						char str[20];
						sprintf(str, "%06X",arg);
						PopUp(str,1);
					    BurnMachineInfoStruct_REQ();
					    break;
					    }
					case FACT_UB_MODEL_RX4:
						gMachineInfo.ModelNumber = MODEL_RX4;
						BurnMachineInfoStruct_REQ();
						break;
					case FACT_UB_BURN:	// just burn the struct (for saving CDScal[])
					    BurnMachineInfoStruct_REQ();
						break;
					default: break;
					}
			    }
            return MSG_DONE;
        default: break;
    }
    return 0;
}

bool IsDemoUnit()
{
	if(gMachineInfo.DemoUnit)
		return 1; 
//	if(!gMachineInfo.NotDemoUnit)
//		return 1;
	return 0;
}
MsgHandler(OnSettingUpgradeCode)
{
    static unsigned mem=0, OneChanceOnly=0;
	int result;
	char str[20];
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = mem; // UI_GetTimecodePacked(TCGenerator, 0); // timecode++;
            return MSG_DONE;
        case MSG_VALUE_SET:
			OneChanceOnly++;
			//sprintf(str, "%06X",msg->Value.Unsigned); // convert to a hex string 
            result = DecodeOptions(msg->Value.Unsigned, (unsigned) gMachineInfo.SerialNumber); // decrypt SOPT code

			if(OneChanceOnly>1)
				{				
				//PartialDebugScreen(7, true);
				dprintf("\n ERROR - RESTART NOW\n");
				PopUp("ERROR - RESTART NOW",1000);
				return MSG_DONE;
				}

			// Valid code?
			if ( (result > 0) && (OneChanceOnly == 1))
				{// Yes...
				//PartialDebugScreen(7, true);
				if     (result == 3)  { // make this NORMAL unit
					gMachineInfo.DemoUnit=0; 		// set to non zero to make this a DEMO unit
					gMachineInfo.NotDemoUnit=1;		// and set to zero to make this a NORMAL unit 
					}// gMachineInfo.ModelNumber = MODEL_RX4; gMachineInfo.SoftwareOptions &= (~SOPT_NOMAD_TOUCH); } 
				else if(result == 4)  { // make this a DEMO UNIT
					gMachineInfo.DemoUnit=1; 		// set to non zero to make this a DEMO unit
					gMachineInfo.NotDemoUnit=0;		// and set to zero to make this a NORMAL unit 
					}
				//else if(result == 6)  {gMachineInfo.ModelNumber = MODEL_NOMAD6;   gMachineInfo.SoftwareOptions &= (~SOPT_NOMAD_TOUCH); } 
				//else if(result == 8)  {gMachineInfo.ModelNumber = MODEL_NOMAD8;   gMachineInfo.SoftwareOptions &= (~SOPT_NOMAD_TOUCH); } 
				//else if(result == 10) {gMachineInfo.ModelNumber = MODEL_NOMAD10;  gMachineInfo.SoftwareOptions &= (~SOPT_NOMAD_TOUCH); } 
				//else if(result == 12) {gMachineInfo.ModelNumber = MODEL_NOMAD12;  gMachineInfo.SoftwareOptions &= (~SOPT_NOMAD_TOUCH); } 
				//else if(result == 32) gMachineInfo.SoftwareOptions |= SOPT_NOMAD_D32;   // deva32 software
				else if(result == 51) 
					{
					gMachineInfo.SoftwareOptions |= (SOPT_NOMAD_TOUCH | SOPT_NOVA_TOUCH); 
					}
				//else if(result == 64) gMachineInfo.SoftwareOptions |= SOPT_NOMAD_TOUCH; 
				else 
					return MSG_DONE; // invalid product code!!!
				BurnMachineInfoStruct_REQ();
 				}
            return MSG_DONE;

        default: break;
    }
    return 0;
}

// see N_Clock.cpp
//unsigned UI_GetDate_BCD(); 
//void     UI_SetDate_BCD(unsigned bcd);
//unsigned UI_GetTime_BCD(); 
//void     UI_SetTime_BCD(unsigned bcd);

MsgHandler(OnSettingTCGenJAM_Time)
{
    uint32 tmp32, hh,mm,ss,ff=0;
    time_t ltime;   struct tm *t; 
	DevaTime(&ltime);
    t = localtime(&ltime);

    switch (MsgType(msg)) {
        case MSG_ACTION:		   
			ff = 0; // we dont have a pulse detector for the DATE RTC. so TC will be off by up to 1 second
			TCGenerator->SetTimeCode(t->tm_hour, t->tm_min, t->tm_sec, ff);
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingTCGenJAM_Date)//jamdate immediately
{
    uint32 UB32;
    switch (MsgType(msg)) {
        case MSG_ACTION:		   
			UB32 = UI_GetDate_BCD();
			TCGenerator->SetUserBits1(UB32);
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingTCGenJAM_TC)
{
    uint32 UB32;
    switch (MsgType(msg)) {
        case MSG_ACTION:
		  if( GetTransportMode() == RECORD_MODE)
			{
			PopUp_RecordConflict(); 
			return MSG_DONE;
			}
          TCGenerator->RawJam(TCReader, TRUE, TRUE);
		  gPreRecordBufferIsDirty=1; // PreRecordIsDirty();  gNV.TCSetTime = GMSECS;
          return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingTCGenJAM_UB)
{
    unsigned a,b,c,d; // UB32;
    switch (MsgType(msg)) {
        case MSG_ACTION:
		  if( GetTransportMode() == RECORD_MODE)
			{
			PopUp_RecordConflict(); 
			return MSG_DONE;
			}
		TCReader->GetUserBits(&a,&b,&c,&d); //  UserBits1, unsigned UserBits2, unsigned UserBits3, unsigned UserBits4)
		TCGenerator->SetUserBits(a,b,c,d);
		gPreRecordBufferIsDirty=1; 
          return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingTCstyleUSA)
{
    uint32 UB32;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned = gNV.fDateUSA;   return MSG_DONE;
        case MSG_VALUE_SET:	gNV.fDateUSA = msg->Value.Unsigned;   NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

	
MsgHandler(OnSettingTCCardTimecode){return 0;}	
MsgHandler(OnSettingTCCardUserbits){return 0;}	


MsgHandler(OnSettingTCReaderTimecode) // read only
{
    static TC timecode;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:		   
			timecode =  UI_GetTimecodePacked(TCReader, 0);
            msg->Value.Unsigned = timecode;
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingTCReaderUserbits) // read only
{
    static TC timecode;
	int hh,mm,ss,ff;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = TCReader->GetUserBits();
            return MSG_DONE;
        default: break;
    }
    return 0;
}
/*
MsgHandler(OnSetting_Date)// {return 0;}
{
    static TC timecode;
	int hh,mm,ss,ff;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = UI_GetDate_BCD(); 
            return MSG_DONE;
        case MSG_VALUE_SET:
            UI_SetDate_BCD(msg->Value.Unsigned);
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_Time)// {return 0;}
{
    static TC timecode;
	int hh,mm,ss,ff;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = UI_GetTime_BCD(); 
            return MSG_DONE;
        case MSG_VALUE_SET:
            UI_SetTime_BCD(msg->Value.Unsigned);
            return MSG_DONE;
        default: break;
    }
    return 0;
}
*/

MsgHandler(OnSettingDate_Year) 
{
   time_t ltime;
   DevaTime(&ltime);
   struct tm *t = localtime(&ltime);
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = t->tm_year+1900;    return MSG_DONE;
        case MSG_VALUE_SET:   t->tm_year = msg->Value.Unsigned-1900;	UI_SetDate(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}
// menu = {'1 Jan','2 Feb','3 Mar','4 Apr','5 May','6 June','7 July','8 Aug','9 Sept','10 Oct','11 Nov','12 Dec' }, 
MsgHandler(OnSettingDate_Month) 
{
	time_t ltime;
	DevaTime(&ltime);
	struct tm *t = localtime(&ltime);
    switch (MsgType(msg)) {                        // 0 based now
        case MSG_VALUE_GET:  msg->Value.Unsigned = t->tm_mon+0;    return MSG_DONE;
        case MSG_VALUE_SET:  t->tm_mon = msg->Value.Unsigned-0;	   UI_SetDate(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingDate_Day) 
{
   time_t ltime;
   DevaTime(&ltime);
   struct tm *t = localtime(&ltime);//   struct tm *t = localtime(&DevaSeconds);

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = t->tm_mday;    return MSG_DONE;
        case MSG_VALUE_SET:  t->tm_mday = msg->Value.Unsigned;	  UI_SetDate(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingTime_Hours)  
{
	time_t ltime;
    DevaTime(&ltime);
	struct tm *t = localtime(&ltime);//   struct tm *t = localtime(&DevaSeconds);

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = t->tm_hour;    return MSG_DONE;
        case MSG_VALUE_SET:  t->tm_hour = msg->Value.Unsigned;	 UI_SetTime(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingTime_Minutes)
{
    time_t ltime;
	DevaTime(&ltime);
	struct tm *t = localtime(&ltime);//   struct tm *t = localtime(&DevaSeconds);

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = t->tm_min;   return MSG_DONE;
        case MSG_VALUE_SET:  t->tm_min = msg->Value.Unsigned;	UI_SetTime(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingTime_Seconds)
{
   time_t ltime;
	DevaTime(&ltime);
	struct tm *t = localtime(&ltime);//   struct tm *t = localtime(&DevaSeconds);

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = t->tm_sec;   return MSG_DONE;
        case MSG_VALUE_SET:  t->tm_sec = msg->Value.Unsigned;	UI_SetTime(t);  return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingTCmode)    // menu = { 'FREE-RUN', 'REC-RUN', 'CJAM', 'CJAM TC', 'CJAM UB' },
{ 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.TimecodeRunMode;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.TimecodeRunMode = msg->Value.Unsigned;
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_RUNMODE, msg->Value.Unsigned); // send enum value to PC
            NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
   return 0; 
}

int SendIFB_FrameRate(int FrameRateEnum);

MsgHandler(OnSettingTCrate)   // menu = { '23.98', '24', '25', '29.97', '29.97DF', '30', '30DF' },
{ 
    int IsDropFrame=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = gNV.FrameRate;  return MSG_DONE;
        case MSG_VALUE_SET:
		    gNV.FrameRate = msg->Value.Unsigned;
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_FRAMERATE, msg->Value.Unsigned); // send enum value to PC
			UpdateFrameRates(gNV.FrameRate);
			// wrong context!  SendIFB_FrameRate(gNV.FrameRate);
            NVIsDirty(DEVAINI);
        return MSG_DONE;
        default: break;
     }
	return 0;
}

MsgHandler(OnSettingTCincUB)     // menu = { 'OFF', 'ON'  },
{ 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:    msg->Value.Unsigned = gNV.fIncUserBits;    return MSG_DONE;
        case MSG_VALUE_SET:
		    gNV.fIncUserBits = msg->Value.Unsigned;
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_INCUSERBITS, msg->Value.Unsigned); // send enum value to PC
            NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
   return 0; 
}

MsgHandler(OnSettingTCoutSelector)     // menu = { 'GENERATOR', 'CARD'} 
{ 
	int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.TimeCodeOut;   return MSG_DONE;
        case MSG_VALUE_SET:
			mem = msg->Value.Unsigned;
			if(mem > 1) mem=0; // cycle if illegal
		    gNV.TimeCodeOut = mem;
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_OUT, mem); // send enum value to PC

			if (gNV.TimeCodeOut == TC_TIMEOUT_GEN)	
				TCGenSelected = TCGenerator;
			else
				TCGenSelected = TCDisk;
            NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
   return 0; 
}
				
/*
enum {
	TC_DISPLAY_GEN, // 0
	TC_DISPLAY_DISK, // 1
	TC_DISPLAY_GEN_STOP, // not supported?
	TC_DISPLAY_ENUMSIZE
};
*/
MsgHandler(OnSettingTCdisplaySelector) // menu = { 'GENERATOR', 'CARD'} -- , 'GEN-STOP' }
{ 
	int mem, forceit=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = gNV.TimeCodeDisplayed;   return MSG_DONE;
        case MSG_VALUE_SET:
			 mem = msg->Value.Unsigned;
			#ifdef NOMAD
		     if(mem == TC_DISPLAY_GEN_STOP) // not supported so cycle around to beginning
				{
				mem = TC_DISPLAY_GEN;
				forceit=1; // send msg back to PC to force it away from this selection
				}
			//if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_DISPLAYED, mem); // , forceit); // send enum value to PC
			#endif
		    gNV.TimeCodeDisplayed = mem;
			gTransportStatusChange++; // allow TCdisplayed to change
            NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
   return 0; 
}



MsgHandler(OnSettingMirrorMode) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = GUI_GetMirrorMode(gNV.MirrorDrive);  return MSG_DONE;
        case MSG_VALUE_SET:
            if(CountTracksToMirror(gNV.MirrorDrive)==0)
				PopUp_Generic("Warning: There are", "no mirror tracks", "selected.  See", "RECORD ENABLES page.", 4, 5000); 
			else if(!gfat[DRIVE_CF]->MediaInserted)
				PopUp("No mirror card", 1000); 
			else if(!gfat[DRIVE_CF]->Mounted)
				PopUp("Mirror card not\nMounted", 2000); 
			else 
				{
	            SetMirrorMode(gNV.MirrorDrive, msg->Value.Unsigned); 
				gRTCram.MirrorMode[gNV.MirrorDrive] = msg->Value.Unsigned; // save boot up mirror mode

				if(gMirrorDrive[gNV.MirrorDrive].FileNameMode == 2)	PopUp("Warning\nThe file name mode\n<1_T2> is not\nrecomended", 3000); 
				if(gMirrorDrive[gNV.MirrorDrive].FileNameMode == 3)	PopUp("Warning\nThe file name mode\n<1T2> is not\nrecomended", 3000); 
				}	
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingMirrorDrive) 
{
#ifndef RX4 // there are no mirror drive choices since thgere is only one drive
	int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = gNV.MirrorDrive;
            return MSG_DONE;
        case MSG_VALUE_SET:
			mem = msg->Value.Signed;
			if(mem <= 0)       mem = DRIVE_USB;
			if(mem > DRIVE_CF) mem = DRIVE_CF; // dont allow SD_DRIVE yet         USB_DRIVE and CF_DRIVE only
			#ifdef USE_FATFS // USB not working yet in FATFS build  
			mem = DRIVE_CF; 
			#endif
            if( IsMirroring() ) 
				PopUp_MirrorConflict();
            else
            	{
            	GUI_SetMirrorDrive(mem); // gNV.MirrorDrive = mem;
				NVIsDirty(DEVAINI);      
				}
            return MSG_DONE;
        default: break;
    }
#endif
    return 0;
}

/**
 * Message handler to configure custom column color in a UI_MATRIX control
 */
MsgHandler(OnMatrixCustomColorDemo)
{
    switch (MsgType(msg))
    {
        case MSG_CONFIGURE:
            int column;
            for (column = 4; column < 8; column++) {
                UIMatrixColumnColorSet((UI_MATRIX*)context, column, COLOR_RED);
            }
            break;      /* Allow further processing of this message */
        default: break;
    }
    return 0;
}

// use this for a standard matrix that just has analog inputs
MsgHandler(OnMatrixCustomColor_StdAnalog)
{
    switch (MsgType(msg))
    {
        case MSG_CONFIGURE:
            int column;
            for (column = 4; column < 8; column++) {
                if(!GetIOcfgChannelActiveStatus(column+IN_CHAN_ANALOG_1)) // check analog channel exsistance
                    UIMatrixColumnColorSet((UI_MATRIX*)context, column, COLOR_RED);
            }
            break;      /* Allow further processing of this message */
        default: break;
    }
    return 0;
}

// use this for a standard matrix that just has digital inputs
MsgHandler(OnMatrixCustomColor_StdDigital)
{
    switch (MsgType(msg))
    {
        case MSG_CONFIGURE:
        {
            int column;
            for (column = 0; column < 8; column++) 
                if(!GetIOcfgChannelActiveStatus(column+IN_CHAN_DIGITAL_1)) // check channel exsistance nova1/nova2
                    UIMatrixColumnColorSet((UI_MATRIX*)context, column, COLOR_RED);
        }
            break;      /* Allow further processing of this message */
        default: break;
    }
    return 0;
}


// use this for the Aud Bus Assign screen   R3 R4 S C T
MsgHandler(OnMatrixCustomColor_AUX_BUS_ASSIGN)
{
    switch (MsgType(msg))
    {
        case MSG_CONFIGURE:
            ;// if(!GetIOcfgChannelActiveStatus(IN_CHAN_RETURN_3)) UIMatrixColumnColorSet((UI_MATRIX*)context, 0, COLOR_RED);
            // if(!GetIOcfgChannelActiveStatus(IN_CHAN_RETURN_4)) UIMatrixColumnColorSet((UI_MATRIX*)context, 1, COLOR_RED);
            break;      /* Allow further processing of this message */
        default: break;
    }
    return 0;
}

// use this for the HP, HP2 and TAPE/MONO out matrix screens (RET1,2,3,4 are on the 3rd row)
MsgHandler(OnMatrixCustomColor_HP_STYLE)
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET:
            assert(cell);
            /* Following overrides the default cell color for individual cells */
            if (cell->Row == 2) // is this the RETURN row?
               if(!GetIOcfgChannelActiveStatus(cell->Col+IN_CHAN_RETURN_1)) // check RETURN's channel exsistance
                    cell->Color = COLOR_RED; // RED means channel is disabled in IOconfig menu                		
        default: break;
    }
    return 0;
}         

// use this for the CARD 9-12 screen (0ut1234 Ret1234)
MsgHandler(OnMatrixCustomColor_O4R4)
{
    switch (MsgType(msg))  {
        case MSG_CONFIGURE:
            int column;
            for (column = 4; column < 8; column++)
                if(!GetIOcfgChannelActiveStatus((column-4)+IN_CHAN_RETURN_1)) // check analog channel exsistance
                    UIMatrixColumnColorSet((UI_MATRIX*)context, column, COLOR_RED);
            //break;      /* Allow further processing of this message */
        default: break;
    }
    return 0;
}

int CountRecordEnables()
{
	int i,count=0;
   	for(i=0;i<MAX_DISK_CHANNELS;i++)
		count += gNV.fTracksToRecord[i];
	return count;
}


// this is for both ui.lua and Oasis app
// I changed this and ui.lua to match PCnova GUI in V4.00A
#define GUI_SRATE_48000 0
#define GUI_SRATE_48048 1
#define GUI_SRATE_88200 2
#define GUI_SRATE_96000 3
// I changed this table to match PCnova GUI in V4.00A
int SampleRate_LUT[]  = {48000, 48048, 88200, 96000}; // this table must match PC GUI!!!
//int SampleRate_LUT[]= {44100, 47952, 48000, 48048}; // , 88200, 96000};
//int SampleRate_LUT[]= {44100, 47952, 48000, 48048, 88200, 96000, 96096, 192000};
MsgHandler(OnSettingSampleRate)
{
    int i;
	static ALARM alarm=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
			msg->Value.Unsigned = 0;
    		for(i=0;i<numberof(SampleRate_LUT);i++)
       			if(SampleRate_LUT[i] == gNV.InternalSampleRate)
	      			msg->Value.Unsigned = i;
			return MSG_DONE;
        case MSG_VALUE_SET:   
			if(!CHECK_ALARM(alarm))
	        	return MSG_DONE;//hdelayMS(900);  // let previous setting take hold first
   			if( msg->Value.Unsigned >= numberof(SampleRate_LUT) ) // make sure its in the table
      			return MSG_DONE;
//            if( (msg->Value.Unsigned!=GUI_SRATE_48000) && (gNV.IFB_mode!=IFBMODE_OFF) )
//				{
//				PopUp_Generic("Please turn", "off IFB", "", "", 4, 2000);
//				SendSetting_Byte(SETTING_SAMPLE_RATE, GUI_SRATE_48000); // change enum value back to PC
//				return MSG_DONE;
//				}		 

            if(msg->Value.Unsigned>GUI_SRATE_96000) // 96k is as high as we can go on Nova
				return MSG_DONE;    

            if(msg->Value.Signed<GUI_SRATE_48000) 
				return MSG_DONE;    /// ********************** 47952 not supported **************  v2.46

			// skip 88200 since I dont think AES or ADCs are running at the correct rate - MCLK would have to change I think to make this work....
            if( (msg->Value.Unsigned == GUI_SRATE_88200) && (gNV.InternalSampleRate<88200) ) msg->Value.Unsigned++;// = GUI_SRATE_96000;
            if( (msg->Value.Unsigned == GUI_SRATE_88200) && (gNV.InternalSampleRate>88200) ) msg->Value.Unsigned--;// = GUI_SRATE_48048;

			// limit record tracks when going over 48048
			if( (msg->Value.Unsigned>GUI_SRATE_48048) && (CountRecordEnables() > MAX_TRACKS_AT_96KHZ) ) // limit SRATE if too many tracks are enabled
				{
				PopUp_Generic("Reduce", "Record", "Tracks", "", 4, 2000);
				SendSetting_Byte(SETTING_SAMPLE_RATE, GUI_SRATE_48048); // change enum value back to PC
				return MSG_DONE;
				}		 
			else 
				PopUp("(Reboot)",1);

			gNV.InternalSampleRate = SampleRate_LUT[msg->Value.Unsigned];
			SetSampleRate(gNV.InternalSampleRate); // this sets gSampleRate which is monitored by a task
			dprintf("SetSrate = %d\n", gNV.InternalSampleRate);

			if(context != gPCcontext)
				SendSetting_Byte(SETTING_SAMPLE_RATE, msg->Value.Unsigned); // send enum value to PC
			NVIsDirty(DEVAINI);      
			SET_ALARM(alarm, 500); // dont change srate too quickly
        	return MSG_DONE;
        default: break;		       
    }
    return 0;
}
				

MsgHandler(OnSettingRecordPreroll)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.PreRecordTime; return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Unsigned > 10)
				return MSG_DONE;
			gNV.PreRecordTime = msg->Value.Unsigned; 
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_RECORD_PREROLL, msg->Value.Unsigned); // send enum value to PC
          	gPreRecordBufferIsDirty=1; //  flush pre-record buffer
        	NVIsDirty(DEVAINI);
        	return MSG_DONE;
        default: break;		       
    }
    return 0;
}


									  
#ifndef WIN32  
// ---------- nomad hardware only --------------
//#define gBootTimeReference 0 // gMSECs staryts at 0 so no need to do anthing extra to make that happen
ALARM Timer(void)  /* return the DSP's 1ms system timer   used by uitimer.cpp */
{
    return gMSECs; // dont use GMSECS here since it may be defined as Timer();
}
int TimerIsAfter(UInt32 t)
{
    return (Timer() > t); // ((Timer() - t) > 0);hmmm...
}
SInt32 TimerDiff(ALARM t1, ALARM t2)
{
    return ((SALARM) ((SALARM)t2 - (SALARM)t1));
}
#else // ---------- win32 simulation ---------
ALARM gBootTimeReference=0; // Nomad assumes gMSECs is 0 at boot so this will fake out the win32 system to do the wsame thing
void   InitNomadWin32Timer() { gBootTimeReference = Timer(); } // find out when we started the program so we can make GMSECS start at 0 ms
void   SET_ALARM     		(ALARM &StaticTimerVar, unsigned msecs) { StaticTimerVar = GMSECS + msecs;}
void   KILL_ALARM			(ALARM &StaticTimerVar) { StaticTimerVar = ALARM_INFINTY;}
bool   CHECK_ALARM			(ALARM StaticTimerVar)  { return (GMSECS > StaticTimerVar);} // Checks the alarm state...   Returns TRUE when the alarm has expired, FALSE if not.

SALARM CHECK_ALARM_TIMELEFT (ALARM StaticTimerVar)  { return StaticTimerVar - GMSECS;} // see how much time is left on the clock (negative if it will never trigger)
void   SET_STOPWATCH        (ALARM &StaticTimerVar) { StaticTimerVar = GMSECS;} // marks a timestamp

SALARM CHECK_STOPWATCH      (ALARM StaticTimerVar)  { return GMSECS - StaticTimerVar;} // returns elapsed time since SET_STOPWATCH was last called

#endif

// I remove this from TransportSetState()
EXPORT void ToneModeSetState(int state) // -1 is toggle mode  this is called by the key detector in ui.cpp!!!
{
	if(state == -1) // LUA_TONE_MODE_TOGGLE) // toggle tone on and off
		gNV.fToneOn = !gNV.fToneOn;
	else 
		gNV.fToneOn = state;
      ToneOnOff(gNV.fToneOn);
	  SetToneMix(gNV.fToneOn);
  	  SendSetting_Byte(SETTING_TONE_ENABLE, gNV.fToneOn); // send enum value to PC
}

// -------------------- HEY this transport thing is LUA specific and does not line up with our nornal transport enum!!------------------------
//#include "Transport.h"
//#define LUA_STOP_MODE   0	// this is from the lua code only! NOT from NOMAD CODE!
//#define LUA_PLAY_MODE   1
//#define LUA_RECORD_MODE 2
//#define LUA_REW_MODE    3
//#define LUA_FF_MODE     4
//#define LUA_TONE_MODE_TOGGLE   5 // toggle tone
					  
EXPORT void UI_TransportSetState(UInt8 state)// this is called by the key detector in ui.cpp!!!
{
   if(state == LUA_TRANSPORT_STOP)
      {
      if( (GetTransportMode()==STOP_MODE) && (GetNumSegments() > 0) && ((UIPageCurrent() == &UIHome1) || (UIPageCurrent() == &UIHome2))  )
      	 {
		 UIPageGoto(&UIPageFalseStart); 
		 return;
		 }
      else
         SetTransportMode(STOP_MODE);
	  }
   if(state == LUA_TRANSPORT_PLAY)SetTransportMode(PLAY_MODE);

   	if(state == LUA_TRANSPORT_REC) 
   		{
		if(Timer()<4000)
			PopUp("Wait...",1); // dont allow record right after booting
		else
	   		SetTransportMode(RECORD_MODE);
		}

   if(state == LUA_REWIND_MODE)   DMgotoseg = folder[DMcurFolder].CurPlaySeg-1; // SetTransportMode(REWIND_MODE);
   if(state == LUA_FFWD_MODE)     DMgotoseg = folder[DMcurFolder].CurPlaySeg+1; // SetTransportMode(FFWD_MODE);
   return;
}

EXPORT UInt8 UI_TransportState(void)
{
   enum TRANSPORT_MODE mode = GetTransportMode();
   if(mode == PLAY_MODE)   return LUA_TRANSPORT_PLAY;
   if(mode == RECORD_MODE) return LUA_TRANSPORT_REC;
   return LUA_TRANSPORT_STOP;
}

MsgHandler(OnSettingTransportState)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = UI_TransportState();  return MSG_DONE;
        case MSG_VALUE_SET:  UI_TransportSetState(msg->Value.Unsigned);  return MSG_DONE;
        default: break;
    }
    return 0;
}


/*  I had to move thise to NomadSettings.h for RemoteOasis.cpp
#define MCELL_OFF    0
#define MCELL_ON     1
#define MCELL_ON_INV 2
#define MCELL_PF	 3
#define MCELL_PF_INV 4

enum { // codes to allow the following routine to know how to interpret the page's data
	   MENU_CARD_MIX14_A,   MENU_CARD_MIX14_D,   MENU_CARD_MIX14_SCT,   
       MENU_CARD_MIX58_A,   MENU_CARD_MIX58_D,   MENU_CARD_MIX58_SCT,   
       MENU_CARD_MIX912_A,  MENU_CARD_MIX912_D,  MENU_CARD_MIX912_SCT,   

	   MENU_OUT_MIX14_A,   MENU_OUT_MIX14_D,   MENU_OUT_MIX14_SCT,   
       MENU_OUT_MIX58_A,   MENU_OUT_MIX58_D,   MENU_OUT_MIX58_SCT,   
       MENU_OUT_MIX912_A,  MENU_OUT_MIX912_D,  MENU_OUT_MIX912_SCT,   
};
*/
int UI_GetCurrentOutputBus5to8enum(); // see uibus.cpp this returns  0 or 1

// this returns cell->Value
void GetSetTranslatedMixCrosspoint(MATRIX_CELL *cell, int MenuType, bool Get)	// struct {UInt8 Row; UInt8 Col; UInt32 Value; }
{
   bool CellOnOffOnly=false; // force choices to only on or off    (InputBusType==ST_BUS) || (InputBusType==OUTPUT_BUS) )
   int OutputBusNum       = cell->Row;
   int InputBusNum        = cell->Col; // assume inputs line up perfectly for now
   MixerType MixerBusType = DISK_MIXER;
   InputBus  InputBusType = INPUT_BUS; // ANALOG_BUS   ST_BUS   DITIGAL_BUS   PF_DIGITAL_BUS
   XPOINT_STATE x; // bit field... see AudioMix.h

	switch (MenuType) {

	case MENU_OUT_MIX_A:   
		MixerBusType = OUTPUT_MIXER;  
    case MENU_CARD_MIX_A:  
		//OutputBusNum += gUI_BusTrackOffset; // this UI var chooses tracks 1-4, 5-8 or 9-12
		InputBusType = ANALOG_BUS;
		break; 

	case MENU_OUT_MIX_D:   
		MixerBusType = OUTPUT_MIXER; 
	case MENU_CARD_MIX_D:  	
		//OutputBusNum += gUI_BusTrackOffset;
		InputBusType = DIGITAL_BUS; 
		break;

	case MENU_CARD_MIX_O14:  	
		//OutputBusNum += gUI_BusTrackOffset;
		InputBusType = OUTPUT_BUS; 
		CellOnOffOnly=true; // force choices to only on or off 
		break;

	case MENU_OUT_MIX_SCT: 
	 	MixerBusType = OUTPUT_MIXER;
	case MENU_CARD_MIX_SCT:  
	case MENU_TRACKX_MIX_SCT:  
		if(MenuType==MENU_TRACKX_MIX_SCT)
			OutputBusNum = Nomad.Settings.CurTrackMixPageNum;
		if(cell->Col == 0) InputBusNum=ST_SLATE_OFFSET; // special SLATE case 
		if(cell->Col == 1) InputBusNum=ST_TALK1_OFFSET; // special COM case
//		if(cell->Col == 2) InputBusNum=ST_TALK2_OFFSET; // what is this for?
		if(cell->Col == 2) InputBusNum=ST_TONE_OFFSET;  // special TONE case
		//OutputBusNum += gUI_BusTrackOffset;
		InputBusType = ST_BUS; 
   		CellOnOffOnly=true; // force choices to only on or off    (InputBusType==ST_BUS) || (InputBusType==OUTPUT_BUS) )
		break;  


	case MENU_OUT_MIX56: // this menu has a completely different matrix layout!! [trk1-6] [trk7-12] [out1-4] [R1,R2]
		MixerBusType = OUTPUT_MIXER;  
		InputBusType = DISK_BUS;    
	//  if(cell->Row==0)  InputBusNum += 0; // track-6
		if(cell->Row==1)  InputBusNum += 6; // track 7-12
		if(cell->Row==2)  InputBusType = OUTPUT_BUS;    // hey can we route outpus to outputs?
		if(cell->Row==3) {InputBusType = ANALOG_BUS; InputBusNum += MAX_PROXY_INPUTS;}   // just two returns
		OutputBusNum = 4 + UI_GetCurrentOutputBus5to8enum(); // out 5,6,7,8
   		CellOnOffOnly=true; // force choices to only on or off  
		break;


	case MENU_TRACKX_MIX_A: // this menu is a lot like the auto mixer assign page
		//OutputBusNum = (MenuType==MENU_TRACK2_MIX_A); // choose dest track 
		OutputBusNum = Nomad.Settings.CurTrackMixPageNum;
		MixerBusType = DISK_MIXER;  
		InputBusType = ANALOG_BUS;    
		if(cell->Row==0) {InputBusType = DIGITAL_BUS; InputBusNum += MAX_AES_INPUTS;}   // RX inputs
		if(cell->Row==1) {InputBusType = ANALOG_BUS;  InputBusNum += 0;}   // 4 ANALOG inputs
		if(cell->Row==2) {InputBusType = ANALOG_BUS;  InputBusNum += PF_RETURN_BUS_INDEX;}   // 2 RETURN inputs
		if(cell->Row==3) {InputBusType = DIGITAL_BUS; InputBusNum += 0;}   // 4or8 digital inputs (not used anymore see case MENU_TRACKX_MIX_D)
   		//CellOnOffOnly=true; // force choices to only on or off  
		break;

	case MENU_TRACKX_MIX_D: // new digital row so we can turn these red when needed
		OutputBusNum = Nomad.Settings.CurTrackMixPageNum;
		MixerBusType = DISK_MIXER;  
		InputBusType = DIGITAL_BUS;    
		break;

	case MENU_TRACKX_MIX_OUTPUTS: // this menu is a lot like the auto mixer assign page
		//OutputBusNum = (MenuType==MENU_TRACK2_MIX_OUTPUTS);  // choose dest track 1 or 2
		OutputBusNum = Nomad.Settings.CurTrackMixPageNum;
		MixerBusType = DISK_MIXER;  
		InputBusType = OUTPUT_BUS;    
   		CellOnOffOnly=true; // force choices to only on or off  
		break;


	case MENU_OUT_MIX_RX: 
	 	MixerBusType = OUTPUT_MIXER;

	case MENU_CARD_MIX_RX:
		//OutputBusNum += gUI_BusTrackOffset;
		InputBusType = DIGITAL_BUS; 
		InputBusNum  += MAX_AES_INPUTS; //was '4' there are 4 regular digital inputs and then 8 RXer inputs
		break;

//	case MENU_CARD_MIX_TRACKS:
//		InputBusType = DISKMIX_BUS; // we now allow tracks as inputs on outs 1-4
//		break;

	default:
		brkpt(MenuType, "Unknown Menu type\n");
		cell->Value=0;
		return;
	}

	// get current xpoint so we can set it to a new value (if(!Get))
	x = GetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum);

// DO NOT LIE!!!	// dont allow <<turning on>> a crosspoint that does not exist on this product but display it none the less if it is on!!
//	if(!Get && (InputBusType == DIGITAL_BUS) && !GetIOcfgChannelActiveStatus(InputBusNum+IN_CHAN_DIGITAL_1)) // check channel exsistance
//		cell->Value = 0;
//	else
		cell->Value = ConvertNomadXpointToCellValue(x, MixerBusType, InputBusType, InputBusNum, OutputBusNum);  

/*
		static ALARM alarm=0;
		if( CHECK_ALARM(alarm) && (cell->Col==0) && (cell->Row==0) )
			{
			dprintf(cCYAN, "MixerBusType=%d, InputBusType=%d, InputBusNum=%d, OutputBusNum=%d, CellValue = %d\n",
				(int)MixerBusType, (int)InputBusType, (int)InputBusNum, (int)OutputBusNum,  cell->Value);
			SET_ALARM(alarm,300);
			}
*/		
    if(Get) 
 	   return; // return cell->Value;
	/////////////////////// SET CROSSPOINT////////////////////////////////////////////////////////////////////////

	// cycle thru the proper choices based on what the current xpoint is set to
	int cXPT_INVERT=0;   
	if(Nomad.Settings.AllowInvertedCrosspointsState) // set mode only...
    	cXPT_INVERT = XPT_INVERT; //  invert crosspoint
	bool IsPreFader = false; 
	int ActionMask = XPT_OFF;
    if(CellOnOffOnly) //  (InputBusType==ST_BUS) || (InputBusType==OUTPUT_BUS) )
		{
		if(cell->Value==MCELL_OFF)    /* _X  */  { ActionMask = XPT_ON;  };
		if(cell->Value==MCELL_ON)     /* _OFF*/  { ActionMask = XPT_OFF; };
		}
	else if(cXPT_INVERT) // is invert allowed?
		{
		if(cell->Value==MCELL_OFF)    /* _X    */{ ActionMask = XPT_ON;              } // InputBusType = ANALOG_BUS;}  
		if(cell->Value==MCELL_ON)     /* _Xinv */{ ActionMask = XPT_ON|cXPT_INVERT;  } // InputBusType = ANALOG_BUS;}
		if(cell->Value==MCELL_ON_INV) /* _P    */{ ActionMask = XPT_ON;              IsPreFader = true;} // InputBusType = PF_ANALOG_BUS;}   
		if(cell->Value==MCELL_PF)     /* _Pinv */{ ActionMask = XPT_ON|cXPT_INVERT;  IsPreFader = true;} // InputBusType = PF_ANALOG_BUS;}
		if(cell->Value==MCELL_PF_INV) /* _OFF  */{ ActionMask = XPT_OFF; } // this forces both analog busses OFF
		}
	else                //  #define MCELL_OFF 0   MCELL_ON 1  MCELL_ON_INV 2   MCELL_PF 3  MCELL_PF_INV 4
		{
		if(cell->Value==MCELL_OFF)    /* _X  */  { ActionMask = XPT_ON;    }; // InputBusType = ANALOG_BUS;    }  
		if(cell->Value==MCELL_ON)     /* _P  */  { ActionMask = XPT_ON;    IsPreFader = true;} // InputBusType = PF_ANALOG_BUS; }
		if(cell->Value==MCELL_PF)     /* _OFF*/  { ActionMask = XPT_OFF; }	// this forces both analog busses OFF
		}

	if(IsPreFader)
		{
		if(InputBusType == ANALOG_BUS)  InputBusType = PF_ANALOG_BUS;
		if(InputBusType == DIGITAL_BUS) InputBusType = PF_DIGITAL_BUS;
		}

	   if(ActionMask == XPT_OFF)
	   		{ // ANALOG BUS and PreFader BUS should never be on at the same time for a given cross point
			if(InputBusType == ANALOG_BUS)
				{
				SetCrossPoint(MixerBusType, ANALOG_BUS,    InputBusNum, OutputBusNum, XPT_OFF);
				SetCrossPoint(MixerBusType, PF_ANALOG_BUS, InputBusNum, OutputBusNum, XPT_OFF);
				}
			else if(InputBusType == DIGITAL_BUS)
				{
				SetCrossPoint(MixerBusType, DIGITAL_BUS,    InputBusNum, OutputBusNum, XPT_OFF);
				SetCrossPoint(MixerBusType, PF_DIGITAL_BUS, InputBusNum, OutputBusNum, XPT_OFF);
				}
			else  // if(InputBusType == ST_BUS)
				SetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum, XPT_OFF);
		     }
		else
			SetCrossPoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum, ActionMask);

		UpdatePCwithNomadCrosspoint(MixerBusType, InputBusType, InputBusNum, OutputBusNum);

	dprintf(cGREEN, "MixerBusType=%d, InputBusType=%d, InputBusNum=%d, OutputBusNum=%d, CellValue = %d ActionMask=%d\n",
				(int)MixerBusType, (int)InputBusType, (int)InputBusNum, (int)OutputBusNum,  cell->Value, ActionMask);

Finish:
	    NVIsDirty(MIXINI);  
     	if(MixerBusType==DISK_MIXER)   {SavePreset(DISKMIX_PRESET,   gNV.DiskMixPresetNum);   fDiskMixDirty=1; }  // calls UpdateDiskShifters();
		if(MixerBusType==OUTPUT_MIXER) {SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum); fOutputMixDirty=1;}

  return; //  cell->Value;
}



EXPORT bool UI_GetNV_AllowInvertedXpoints()
{
	return gNV.AllowInvertedXpoints; // if zero, let star key be "back"
}
MsgHandler(OnSetting_AllowInvertedXpoints)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AllowInvertedXpoints; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.AllowInvertedXpoints = msg->Value.Signed; return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingAllowInvertedXpointsSTATE)
{
    switch (MsgType(msg)) {
        case MSG_KEY_PRESS:
			if(!UI_GetNV_AllowInvertedXpoints())
				return 0; // let STAR key be BACK key
            if(MsgKey(msg) == KEY_STAR)
               {                          
               SetInvertedCrossPointsMode(-1); // set or toggle   
               return MSG_DONE;
               }
        default: break;
    }
    return 0;
}

EXPORT void SetInvertedCrossPointsMode(int state)
{
	if(!UI_GetNV_AllowInvertedXpoints())
		return;
    GET_STATE(p);
    if(state<0)	p->Settings.AllowInvertedCrosspointsState ^= 1;
    else       	p->Settings.AllowInvertedCrosspointsState = state;
	
	if(p->Settings.AllowInvertedCrosspointsState)
		PopUp_GenericNoPC("(Allow Invert)",      "", "", "", 4, 500);
	else
		PopUp_GenericNoPC("(No Invert)",      "", "", "", 4, 500);
}




/*** CardMix14 matrix data ****************************************************************************   
            0    1     2    3     4
  menu = { ' ', 'X', '/X', 'P', '/P' },  
 */	   
MsgHandler(OnSetting_CardMix14_Analog)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_A, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_A, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardMix14_Digital)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_D, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_D, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardMix14_SCT)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_SCT, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_SCT, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardMix14_O14)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_O14, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_O14, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_CardMix14_RX)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_RX, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_CARD_MIX_RX, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSetting_OutMix14_Analog)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_A, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_A, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutMix14_Digital)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_D, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_D, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutMix14_SCT)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_SCT, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_SCT, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutMix14_RX)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_RX, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_RX, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
/*MsgHandler(OnSetting_OutMix14_Tracks)
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_TRACKS, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_OUT_MIX_TRACKS, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
*/
MsgHandler(OnSetting_BusAssign_Track_1){ int idex=0; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = Nomad.Out1to4MixInputTrack[idex]; return MSG_DONE;
        case MSG_VALUE_SET:   Nomad.Out1to4MixInputTrack[idex] = msg->Value.Unsigned; 
//		if(context != gPCcontext)	UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, InputBusType, iInput, idex+4);
		    NVIsDirty(MIXINI);  
            return MSG_DONE;
    }
    return 0;
}
MsgHandler(OnSetting_BusAssign_Track_2){ int idex=1; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = Nomad.Out1to4MixInputTrack[idex]; return MSG_DONE;
        case MSG_VALUE_SET:   Nomad.Out1to4MixInputTrack[idex] = msg->Value.Unsigned; 
//		if(context != gPCcontext)	UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, InputBusType, iInput, idex+4);
		    NVIsDirty(MIXINI);  
            return MSG_DONE;
    }
    return 0;
}
MsgHandler(OnSetting_BusAssign_Track_3){ int idex=2; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = Nomad.Out1to4MixInputTrack[idex]; return MSG_DONE;
        case MSG_VALUE_SET:   Nomad.Out1to4MixInputTrack[idex] = msg->Value.Unsigned; 
//		if(context != gPCcontext)	UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, InputBusType, iInput, idex+4);
		    NVIsDirty(MIXINI);  
            return MSG_DONE;
    }
    return 0;
}
MsgHandler(OnSetting_BusAssign_Track_4){ int idex=3; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   msg->Value.Unsigned = Nomad.Out1to4MixInputTrack[idex]; return MSG_DONE;
        case MSG_VALUE_SET:   Nomad.Out1to4MixInputTrack[idex] = msg->Value.Unsigned; 
//		if(context != gPCcontext)	UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, InputBusType, iInput, idex+4);
		    NVIsDirty(MIXINI);  
            return MSG_DONE;
    }
    return 0;
}


int UI_GetCurrentOutputBus5to8enum(); // see uibus.cpp this returns  0 or 1
// struct { UInt8 Card[12], Return[2], Out[4]; } 
// struct {  OUTMIX out[4];  } Out5to8Mix; // special mixer for out 5 and 6 only
MsgHandler(OnSetting_OutMix5to8) // 555
{
    int idex = UI_GetCurrentOutputBus5to8enum();
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
            if(cell->Row==0) cell->Value = Nomad.Out5to8Mix.out[idex].Card[cell->Col];
            if(cell->Row==1) cell->Value = Nomad.Out5to8Mix.out[idex].Card[cell->Col+8];
            if(cell->Row==2) cell->Value = Nomad.Out5to8Mix.out[idex].Out[cell->Col];
            if(cell->Row==3) cell->Value = Nomad.Out5to8Mix.out[idex].Return[cell->Col];
            if(cell->Row==4) 
            	{
            	if(cell->Col==0) cell->Value = Nomad.Out5to8Mix.out[idex].Slate;
            	if(cell->Col==1) cell->Value = Nomad.Out5to8Mix.out[idex].Com;
            	if(cell->Col==2) cell->Value = Nomad.Out5to8Mix.out[idex].Tone;
				}
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: { // data FROM UI
			if( (!Nomad.Settings.AllowInvertedCrosspointsState) && (cell->Value == 2) )
				cell->Value = 0; // ignore inverted possability
			InputBus InputBusType = DISK_BUS;
			int iInput = cell->Col;
            if(cell->Row==0) Nomad.Out5to8Mix.out[idex].Card[cell->Col]   = cell->Value;
            if(cell->Row==1) Nomad.Out5to8Mix.out[idex].Card[cell->Col+8] = cell->Value, iInput+=8;
            if(cell->Row==2) Nomad.Out5to8Mix.out[idex].Out[cell->Col]    = cell->Value, InputBusType = OUTPUT_BUS;
            if(cell->Row==3) Nomad.Out5to8Mix.out[idex].Return[cell->Col] = cell->Value, InputBusType = RETURN_BUS;
            if(cell->Row==4) 
            	{
				InputBusType = ST_BUS;
            	if(cell->Col==0) Nomad.Out5to8Mix.out[idex].Slate = cell->Value, iInput = ST_SLATE_OFFSET;
            	if(cell->Col==1) Nomad.Out5to8Mix.out[idex].Com   = cell->Value, iInput = ST_TALK1_OFFSET;	
            	if(cell->Col==2) Nomad.Out5to8Mix.out[idex].Tone  = cell->Value, iInput = ST_TONE_OFFSET;
				}
			if(context != gPCcontext)
				UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, InputBusType, iInput, idex+4);
		    NVIsDirty(MIXINI);  
            return MSG_DONE;
			}
        default: break;
    }
    return 0;
}


void UI_Set48V(int ch, int OffOnToggle, void *context);



extern short gCDS_TABLE_DEBUG; // set this to create a CDS gain calibration table

/**
 * Setting message handler for pseudo setting that accesses trim setting 
 * for currently selected Setup.Chan.
 *
 * @see OnPageSetupChanX
 */
static char TrimText[50];
MsgHandler(OnSettingSetupChanTrim)	// ttt
{
	static short mem[MAX_IN_CHAN_ANALOG+MAX_IN_CHAN_DIGITAL];
	int ch, trim, delta;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_DIGITAL) { msg->Value.Signed = 0; return 0;  }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Signed = mem[ch] = GetNVTrim(ch); // Nomad.Setup.Channel[Nomad.Setup.Chan].Trim;
            return MSG_DONE;
        case MSG_VALUE_SET:
			trim =  GetNVTrim(ch); 
            mem[ch] =  msg->Value.Signed; // for debug
			delta   =  msg->Value.Signed - trim;
//			if(gCDS_TABLE_DEBUG)
//			   {
//			   if( (trim>300) ) delta  *=  10; // increase increment since we have to go up to 4000
//			   if( ((trim>200)&&(trim<1000)) || (trim>3000) ) delta  *=  10; // increase increment since we have to go up to 4000
//			   }

//			SetNVTrimDelta(ch, delta); // create a delta value
// I'm afraid to chanmge this to absolute Trim in nova v3.02 - seems to work but why risk it
 SetNVTrim(ch, msg->Value.Signed);

			//if(context != gPCcontext)this is done in OasisSTNupdateHandler
			//	SendSetting_1D_indexed(SETTING_INPUT_TRIM_LEVEL, gChannelConfig[ch].Trim, ConvertNovaChanToPC(ch), 18); 
			NVIsDirty(AICINI); //was NVIsDirty(DEVAINI);
            return MSG_DONE;
        case MSG_EDITOR_GET:
            /* Return pointer to the editor for this setting type. Use the simple editor
             * that doesn't allow numeric entry, only encoder. */
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;

        case MSG_TEXT_GET:
			sprintf(TrimText, "%d", msg->Value.Signed);
			// Same bottom stop SetTrim_50X mutes on (MIN_ANALOG_INPUT_TRIM).
			if(gNV.TrimsCanMute && (msg->Value.Signed <= (int)MIN_ANALOG_INPUT_TRIM))
				strcpy(TrimText, "MUTE");
			break;

        default: break;
    }
    return 0;
}
void UpdateTrim_HMM(int nChannel);
void InitCDSbigTable();
void ForceTrimToMIN(unsigned  channel);
//    name = 'setup.chan.CDScal',  range = {0,200},   units = 'clicks', 
//    data = 'Nomad.Setup.Channel[Nomad.Setup.Chan].CDScal',
//    msghandler = 'OnSettingSetupChanCDScal', 
MsgHandler(OnSettingSetupChanCDScal)  // secret factory CDS cell calibration mode
{
	static short mem[MAX_IN_CHAN_ANALOG+10];
	int ch, trim, delta;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=6) {  msg->Value.Signed = 0;  return 0; }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Signed = mem[ch] = gMachineInfo.CDScal[ch]; // GetCDScalSetting(ch); 
            return MSG_DONE;
        case MSG_VALUE_SET:
			trim =  GetNVTrim(ch); 
            mem[ch] =  msg->Value.Signed; // for debug
			gMachineInfo.CDScal[ch] =  msg->Value.Signed;
			InitCDSbigTable();  // recreate all CDS tables with new click val from CDScal[]
			ForceTrimToMIN(ch); // makes sure trim is set to absolute zero (-20) for calibration mode!!!
            //UpdateTrim(ch);     // make the change noticeable to Put CDS CHUNK
            return MSG_DONE;
        case MSG_EDITOR_GET:
            /* Return pointer to the editor for this setting type. Use the simple editor
             * that doesn't allow numeric entry, only encoder. */
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;
        default: break;
    }
    return 0;
}

/**
 * Setting message handler for pseudo setting that accesses trim setting 
 * for currently selected Setup.Chan.
 *
 * @see OnPageZNetChanX
 */

void SendIFBIncDecCommand(int inc, int unit);

extern short gZnetIncDecGain_unit; // =0;
extern short gZnetIncDecGain_val; // =0;
/*
MsgHandler(OnSettingZNetChanTrim)
{
    static int mem=0;
	static ALARM alarm=0;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    switch (MsgType(msg)) {

        case MSG_VALUE_GET:
			if( (gChannelConfig[Nomad.Setup.Chan].Mix8TrimMode == TRIMMODE_ZAXNET) && (MIX8connected() || OASISconnected()) ) // gNV.fMix12Enabled) // was 2!!!
               {// display RAW trim value based on Mix8 Trim Knob position
               msg->Value.Signed = Nomad.Setup.Channel[ch].ZNetTrimDB; // (52*GetZaxNetTrim255(Nomad.Setup.Chan))/255;
 	           return MSG_DONE;
			   }

		    if(CHECK_ALARM(alarm)) 
		    	mem=0; // reset to zero every 500ms
            msg->Value.Signed = mem; // (255.0/35.0) * ZTrims[Nomad.Setup.Chan].trim; // convert from 255 max to 35 max for
            return MSG_DONE;

        case MSG_VALUE_SET:
		    if(!CHECK_ALARM(alarm)) // dont set this too often
            	return MSG_DONE;

			if( (gChannelConfig[Nomad.Setup.Chan].Mix8TrimMode == TRIMMODE_ZAXNET) && (MIX8connected() || OASISconnected()) ) // gNV.fMix12Enabled) // was 2!!!
            	return MSG_DONE; // dont allow user input here if mix8 is connected

            if(msg->Value.Signed > 0) 
            	{
            	mem = 1; //   Nomad.Setup.Channel[Nomad.Setup.Chan].Trim = msg->Value.Signed;
				SendIFBIncDecCommand(mem, Nomad.Setup.Chan+1);
				gZnetIncDecGain_unit=Nomad.Setup.Chan+1;
				gZnetIncDecGain_val =1;
				}
            if(msg->Value.Signed < 0) 
            	{
            	mem = -1; //   Nomad.Setup.Channel[Nomad.Setup.Chan].Trim = msg->Value.Signed;
				SendIFBIncDecCommand(mem, Nomad.Setup.Chan+1);
				gZnetIncDecGain_unit= Nomad.Setup.Chan+1;
				gZnetIncDecGain_val = -1;
				}
		    //hdelayMS(400);
		    SET_ALARM(alarm, 200);
            return MSG_DONE;

        case MSG_EDITOR_GET:
            // Return pointer to the editor for this setting type. Use the simple editor
            // that doesn't allow numeric entry, only encoder. 
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;            
        default: break;
    }
    return 0;
}
*/

//Setting { name = 'setup.chan.ZfreqInput',  range = {0,1000,0.1},msghandler = 'OnSetting_InputZfreq', }
MsgHandler(OnSetting_InputZtrim) // znet trim for InputSetup page (new)
{
	int ch = Nomad.Setup.Chan;
	int txid = gChannelConfig[ch].TxID;
//    static int mem=0;
	static ALARM alarm=0;
    switch (MsgType(msg)) {

        case MSG_VALUE_GET:
			msg->Value.Signed = clip(gChannelConfig[ch].ZNetTrimDB, 0.0F, 52.0F);
		    //if(CHECK_ALARM(alarm)) 
		    //	mem=0; // reset to zero every 500ms
            //msg->Value.Signed = mem;  // just display -1, 0, +1 to inc end dec the trim
            return MSG_DONE;

        case MSG_VALUE_SET:
			gChannelConfig[ch].ZNetTrimDB = msg->Value.Signed;
			NVIsDirty(AICINI); //was NVIsDirty(DEVAINI);
			StoreAudioChannelPosition_RTCRAM(ch);
		    /*
		    if(!CHECK_ALARM(alarm)) // dont set this too often
            	return MSG_DONE;
            mem = clip(msg->Value.Signed, -1, 1);
			if(mem!=0)
				{
				SendIFBIncDecCommand(mem,txid);
				gZnetIncDecGain_unit = txid;
				gZnetIncDecGain_val  = mem;
				}
		    SET_ALARM(alarm, 200);
            */
            return MSG_DONE;

        case MSG_EDITOR_GET:
            /* Return pointer to the editor for this setting type. Use the simple editor
             * that doesn't allow numeric entry, only encoder. */
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;            
        default: break;
    }
    return 0;
}



MsgHandler(OnSetting_InputTXID)
{
    int ch=Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned =  gChannelConfig[ch].TxID;  return MSG_DONE;
        case MSG_VALUE_SET:  
			gChannelConfig[ch].TxID = clip(msg->Value.Unsigned, 0, NUM_ZNET_CHAN-1);

			if(context != gPCcontext)
	  	  		SendSetting_1D_indexed(SETTING_INPUT_ZAXNET_ID, ConvertNovaChanToPC(ch), gChannelConfig[ch].TxID, 18);
	        NVIsDirty(AICINI); 
 	        return MSG_DONE;
        default: break;
    }
    return 0;
}

void SetInputZnetFreq_NV(int idex, int f)
{
	if(gChannelConfig[idex].TxID)
		ReqestToSendFreq(f, gChannelConfig[idex].TxID);
	gChannelConfig[idex].ZNetFreq = clip(f, 4600, 9990);
	NVIsDirty(AICINI);  
}
//Setting { name = 'setup.chan.ZfreqInput',  range = {0,1000,0.1},     msghandler = 'OnSetting_InputZfreq', }
MsgHandler(OnSetting_InputZfreq) // sends channel change command to a TRX (via input setup menu instead of zaxnet menu)
{
    int ch = clip(Nomad.Setup.Chan, 0, numberof(gChannelConfig)-1);
	//int txidIndex =  gChannelConfig[ch].TxID - 1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Signed = gChannelConfig[ch].ZNetFreq; // gNV.IFB_TxFreq[txidIndex];
            return MSG_DONE;

        case MSG_VALUE_SET:	 // IFB_TRX_ChannelHandler() will notice this change and send it (slowly)
			SetInputZnetFreq_NV(ch, msg->Value.Signed);
			//gChannelConfig[ch].ZNetFreq = clip(msg->Value.Signed, 4700, 9900);
			//NVIsDirty(AICINI);  
        	//UI_SendIFBparm32(&gNV.IFB_mode); 
            return MSG_DONE;

        case MSG_EDITOR_GET:
            /* Return pointer to the editor for this setting type. Use the simple editor
             * that doesn't allow numeric entry, only encoder. */
//            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            msg->Value.Pointer = &UIEditorNum.Editor;
            return MSG_DONE;            
        default: break;
    }
    return 0;
}

uchar ZNET_GetLowPowerMode(uchar audioch);
void  ZNET_LowPowerModeChanged(uchar val, uchar audioch);
MsgHandler(OnSetting_ZnetLowPower)
{
    int ch = clip(Nomad.Setup.Chan, 0, numberof(gChannelConfig)-1);
	//int txidIndex =  gChannelConfig[ch].TxID - 1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Signed = ZNET_GetLowPowerMode(ch);
            return MSG_DONE;

        case MSG_VALUE_SET:	 // ZNET_LowPowerMode_Handler() will notice this change and send it (slowly)
			{
			int newval = msg->Value.Signed;
            int prev = ZNET_GetLowPowerMode(ch);
				/*
			if(newval==REMOTE_POWER_LOW1_WAKE)
				{
				if(newval>prev) // we are incrementing
					newval++;   // skip to LOW2 mode setting since LOW1 does not do anything
				else if(newval<prev)
					newval--; // skip to NOMRAL mode (0)
				}
				*/
			ZNET_LowPowerModeChanged(ch, newval);
            return MSG_DONE;
			}
        case MSG_EDITOR_GET:
            //Return pointer to the editor for this setting type. Use the simple editor that doesn't allow numeric entry, only encoder.
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;            
        default: break;
    }
    return 0;
}


#ifdef WIN32
int GetChunkUsage(int ResetPeakDecay){return 1;} // fixme
#endif
int GetCPUusagePercent();
/**
 * Read-only setting message handler to return CPU usage
 * 
 */
MsgHandler(OnSettingCpuUsage)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = GetCPUusagePercent(); // 20+GetChunkUsage(0);   // Report percent with peak decay (add 20% for good measure)
            return MSG_DONE;
        default:
            break;
    }
    return 0;
}

int GetActualIFBmode();
int GetActualIFBmodeChangeTime(); // for UI
extern int32 gRecordEnterTime; // set int Transport.cpp
extern char gstrFWline1[], gstrFWline2[], gstrFWline3[],gstrWarning[];// display lines kludge
extern int  gDiskSecsLeft;
extern int    gMCPrestoreCount;
extern uint32 gMCPrestoreTime;
extern uint32 gUSBkbdFoundTime;
extern short  gKeyboardExists;

EXPORT int UI_GetStatusString(char *str, unsigned StringNumber, unsigned MaxStringLength)
{
    char tmpstr[40],  TLODstr[40], *p=gstrFWline1;
	static char strBattery[20]; 
    int i, tmp, Percent, IsExternal, IsAlmostDead, IsReallyDead;
    float Volts;
    uint32 hours, minutes, seconds;
    static int16 StrCycle=0; 
    static ALARM alarm=0; // , NewSegAlarm=0;
    tmpstr[0]=0;  TLODstr[0]=0;
    p = tmpstr;

   if(StringNumber==STATUS_STR_LINE1) 
      p = gstrFWline1;
   if(StringNumber==STATUS_STR_LINE2) p = gstrFWline2;
   if(StringNumber==STATUS_STR_LINE3) p = gstrFWline3; // assume basic mirror string for now (overridden later)
   if(StringNumber==STATUS_STR_WARN)  p = gstrWarning;
   if(StringNumber==STATUS_STR_BAT) // not used anymore.... 
   	  {
      UI_GetBatteryPowerInfo(&Percent, &Volts, &IsExternal, &IsAlmostDead,  &IsReallyDead, strBattery);
   	  p = strBattery;
      }
                                            
	if(StringNumber==STATUS_STR_CUR_SEG) 
   		{  
      	if(GetTransportMode()==RECORD_MODE)  zsnprintf(p, 10, "%03d", (int)DMcurRecSeg);
      	else                                 zsnprintf(p, 10, "%03d", (int)folder[DMcurFolder].CurPlaySeg);   
        }
        	  
    if(StringNumber==STATUS_STR_CUR_FOLDER)   zsnprintf(p, 10, "%s",   folder[DMcurFolder].Name);
    if(StringNumber==STATUS_STR_CUR_SEGS)     zsnprintf(p, 10, "%03d", (int)folder[DMcurFolder].TotalSegs);
                                                                          

    if(StringNumber==STATUS_STR_LINE3)
    	{
	    if(CHECK_ALARM(alarm)) 
    	  {
	      if(++StrCycle>3) // was 2) 7.42
	      	StrCycle=0; // cycle thru what string is on LINE3
		  SET_ALARM(alarm, 2000);
		  }

        if(StrCycle==1) 
			{
			if(GetTransportMode()==RECORD_MODE)  
				StrCycle=2; //  get rid of this when recording
			GetHHMMSSstring(TLODstr, gDiskSecsLeft, "rec time ");
			p = TLODstr; // forceably toggle rec time in place of line3 string (this may get forceably overwritten below)
			}

        if(StrCycle==2) // print seg time if recording   "Seg006 (1:02:00)"
			{
			if(GetTransportMode()==RECORD_MODE)
				{
				tmp = GetElapsedRecTime();
				GetHHMMSSstring(tmpstr, tmp, "");
				sprintf(TLODstr, "Seg %d (%s)", DMcurRecSeg, tmpstr+1); // "Seg006 (1:02:00)"
				p = TLODstr; // forceably toggle SEGtime in place of line3 string (this may get forceably overwritten below)
				}
			else
				StrCycle++; // =0;
			}

        if(StrCycle==3) 
			{
			int seg = 1+GetNumSegments(); // get current seg number
			//if(GetTransportMode()==RECORD_MODE)  
			//	seg++;

			sprintf(tmpstr, "S:%s T:%s", dir[seg].Scene, dir[seg].Take);
			char segstr[9];
			segstr[0]=0;
			if(strlen(tmpstr)<12)
				{
				//if(GetTransportMode()!=RECORD_MODE)  
				//	sprintf(segstr, "(Next)", seg); // we are displaying a seg that does not exist yet
				//else
					sprintf(segstr, " (%02d)", seg);
				}
			if(strlen(tmpstr)>=17) // still too long? then strip off first two chars
				sprintf(tmpstr, "%s T:%s", dir[seg].Scene, dir[seg].Take);

			strcat(tmpstr, segstr);
			p = tmpstr; 
			}

	//if(gJTAGcableDebugActive)
	//	{
	//	GetHHMMSSstring(TLODstr, gDiskSecsLeft, "rec time ");
	//	p = TLODstr; 
	//	}


//   		// print the new seg number for a while after a RECORD press
//   		if( ((GMSECS-gRecordEnterTime)<3000)  && (StringNumber==STATUS_STR_LINE3) )
//   			{
//   			if(gRecordEnterTime==0) sprintf(tmpstr, "Last Seg = %03d", folder[DMcurFolder].TotalSegs); // displayed on boot up
//			else                    sprintf(tmpstr, "New Seg %03d", folder[DMcurFolder].TotalSegs+1); // disp after record key press
//			p = tmpstr;
//			}

		#ifdef NOMAD
        // force an indication of any IFB mode change
		if( (GMSECS - GetActualIFBmodeChangeTime()) < 1200)
			{
			strcpy(tmpstr, "IFB MODE = ");
			if( GetActualIFBmode() == IFBMODE_OFF) 	strcat(tmpstr, "OFF");
			if( GetActualIFBmode() == IFBMODE_TX) 	strcat(tmpstr, "TX");
			if( GetActualIFBmode() == IFBMODE_RX) 	strcat(tmpstr, "RX");
		    p = tmpstr;
			}

		if( ((GMSECS - gMCPrestoreTime) < 3000) && (GMSECS>4000) ) // print MCP restore error message
			{
			sprintf(tmpstr, "KEY FIX (%d)", gMCPrestoreCount);
		    p = tmpstr;
			}
		#endif

		if(gKBD.exists)
			{
			if((GMSECS - gKBD.FoundTime) < 3000) 
				{
				sprintf(tmpstr, "Found Keyboard");
			    p = tmpstr;
				}
			else if( (GMSECS - gKBD.LockedChangedTime) < 2000)
				{
				if(gKBD.locked)	sprintf(tmpstr, "Keyboard LOCKED");
				else		    sprintf(tmpstr, "Keyboard UNLOCKED");
			    p = tmpstr;
				}
			}
						

   		if(strlen(p)==0) // if LINE3 is not filled, then always print TLOD
      		{
	  		SET_ALARM(alarm, 10); // if LINE3 becomes filled, jump to it quickly
      		StrCycle=1;
			p = TLODstr;
	  		}

		}//if(STR_LINE3)

	// tack on ZlastUnit code for 1 second when Znet cmd is sent
	if( (StringNumber==STATUS_STR_LINE3) && (GetLastZnetUnit() > 0) && ((GMSECS-GetLastZnetUnitSentTime()) < 1000) )
		{
		char znetstr[31], pstr[21];
		memcpy(pstr, p, sizeof(pstr));
		pstr[13]=0; // truncate string for case of  "rec time = 12:34:56" 
		zsnprintf(znetstr, 30, "%s z%d", pstr, GetLastZnetUnit());
		p = znetstr;
		}

	i=0;
	int len = clip(strlen(p), 0, (MaxStringLength-1) );
   	strcpy(str, p); // copy the string into the working string area
	str[len]=0; // make sure termiator is in there at all times
    return len; // strlen(p);
}

void Deva_Main_InitGlobalVariables(void)
{	      				  				
//    strcpy(gstrBattery,"INT");
    gstrFWline1[0]=0;
    gstrFWline2[0]=0;
    gstrFWline3[0]=0;
    gstrWarning[0]=0;    
}
     

#ifdef WIN32
EXPORT void UI_SetCurrentCardFolder(int fnum) 
{
   if( GetTransportMode() == STOP_MODE)
      if(DMgotoFolder==0)
         DMgotoFolder = fnum; 
}
#endif
/*
EXPORT void UI_GetCardFolderInfo(char *strFname, int *SizeKB, int *FileCount, int *CurrentFolder, int fnum)
{
   float tmpf;
   zsnprintf(strFname, 10,  "%s", folder[fnum].Name);
   
   tmpf = folder[fnum].SizeLBA;
   tmpf *= 512;
   tmpf /= 1000;
   *SizeKB    = tmpf; // 210+fnum;
   *FileCount = folder[fnum].TotalSegs; // 21+fnum;
   *CurrentFolder = DMcurFolder;
}
*/
EXPORT int  UI_GetCurrentMirrorDrive()
{
	return gNV.MirrorDrive;
}            

EXPORT int  UI_GetFolderCount(int drive)
{
     assert(drive==0);
     return GUI_MAX_FOLDER; //was 99, MAX_FOLDER is 125  all folders always exist even if they are empty
}            

//int CurFolder[4]={1,1,1,1};
EXPORT int  UI_GetCurrentFolder(int drive)
{
	if(drive==0)
       return DMcurFolder;
	//else //if(drive==CF_DRIVE)
	return GUI_GetCurrentMirrorFolderIndex(drive);
    //assert(0); //error
	//return 1;
}            


void UI_SetCurrentFolder(int drive, int fnum)
{
	int mirdrive=UI_GetCurrentMirrorDrive();

	if(drive==DRIVE_INTHD)
		{
		if( GetTransportMode() != STOP_MODE) {PopUp_RecordConflict(); return; }
   	    else // if( GetTransportMode() == STOP_MODE)
			{
			if(gNV.MirFolderFollow)
				{ // change both mirror folder and primary folder in this mode
				if( IsMirroring() )  { PopUp_MirrorConflict();  return; } // mirror mode must be off
		    	SetRawMirrorFolder(mirdrive, fnum);
				NVIsDirty(MIRRORINI);
				}
			UI_GotoPrimaryFolder(fnum); //  	DMgotoFolder      = fnum; 
    	   	//zsnprintf(gNV.CurrentFolder, 20, "%s", folder[fnum].Name); // retarded
	       	gNV.CurFolderNum = fnum;            // we are not storing this in NV anymore (as of 7.39)
			gRTCram.FOLDERMAGIC_5678=0x5678;    // indicates Current folder is valid (0x5678)                     
			gRTCram.FOLDER = gNV.CurFolderNum;  // Current folder                      
			SendSetting_String(SETTING_STORE_FOLDER, folder[fnum].Name);
    	   	NVIsDirty(DEVAINI);
	 		}
		}
	
	if(drive!=DRIVE_INTHD) // just change the mirror folder
	    {
	    SetRawMirrorFolder(drive, fnum);
		NVIsDirty(MIRRORINI);
		}
	
   return;
}

      
EXPORT void UI_GetFolderInfo(int drive, int fnum, char *FolderName, int *SizeMB, int *FilesInFolder)
{
   float tmpf;
   if(fnum<=0) 
   	  fnum = UI_GetCurrentFolder(drive);

   zsnprintf(FolderName, 10,  "%s", folder[fnum].Name);

   tmpf = folder[fnum].SizeLBA;
   tmpf *= 512;
   tmpf /= 1000000;
   *SizeMB    = tmpf; // 210+fnum;
   *FilesInFolder = folder[fnum].TotalSegs; // 21+fnum;
//   *CurrentFolder = DMcurFolder;
//   //fnum++;
//   sprintf(FolderName, "X%03d", fnum);
//   *SizeMB = fnum + 200;
//   *FilesInFolder = fnum + 20;
}


/**
 * Message handler for read-only psuedo-setting that reports timecode clock offset
 */
MsgHandler(OnSettingTimecodeTuneOffset)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: 
            zsnprintf(MsgText(msg), MAX_UI_TEXT, ".01 FPH");
            return MSG_DONE;
        default: break;
    }
    return 0;
}

/**
 * Message handler to report text (or text attributes) for read-only ReaderTC
 * text label
 */
MsgHandler(OnTextReaderTC)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET:
            strcpy(MsgText(msg), "READER TC");  
            return MSG_DONE;
        default: break;
    }
    return 0;
}


// convert a simple matrix into a linear value 
//(for example RECORD ENABLES page cannot fit all channels in a single row)
// enter matrix X dimention, and cell coordinates ... returns int
int MatrixToLinear(int ncols, MATRIX_CELL *cell)
{
    //assert(cell->Col < ncols);
    return  cell->Row * ncols + cell->Col;
}

int FindLastTrackEnablesIndex()
{
	int last = 0;
	for(int i=0;i<numberof(gNV.fTracksToRecord);i++)
		if(gNV.fTracksToRecord[i])
			last = i;
	return last;
}
      
/**
 * Setting message handler to access PRIMARY RECORD ENABLES
 */
MsgHandler(OnSettingRecEnables) //fTracksToRecord
{
//    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    uint AllowIt=0, channel;
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET:{ // data TO UI
		    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
//			if((gNV.InternalSampleRate>48048) && (cell->Row==1) )
//	            cell->Color = COLOR_RED; // make all 4 cells red in this case
   			if(channel<MAX_DISK_CHANNELS)
   				cell->Value = gNV.fTracksToRecord[channel];
            return MSG_DONE;
			}
        case MSG_MATRIX_CELL_SET:{ // data FROM UI
		    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
			channel = MatrixToLinear(8, cell); // ncols 

            AllowIt=1;
            if( (gNV.InternalSampleRate>48048) && (CountRecordEnables() > MAX_TRACKS_AT_96KHZ) ) // limit SRATE if too many tracks are enabled 
			   AllowIt = 0;

			if(channel >= gMachineInfo.MaxRecordChannels)
			   AllowIt = 0;

			if(AllowIt)
    			{
				if( GetTransportMode() == RECORD_MODE)
					{
					PopUp_RecordConflict();
					SendSetting_Array1D(SETTING_RECORD_TRACKS, gNV.fTracksToRecord, 32, numberof(gNV.fTracksToRecord));
					//SendSettingRecEnables(&gNV.fTracksToRecord[0]); // force PC back to existing setting
					}
				else
    				{
    				gNV.fTracksToRecord[channel] = Nomad.Settings.fTracksToRecord[channel] = cell->Value;
					SetRecordChannels(&gNV.fTracksToRecord[0]);  // imposes ALL proper restrictions including samplerate of 96khz
					if(context != gPCcontext)
						SendSetting_Array1D(SETTING_RECORD_TRACKS, gNV.fTracksToRecord, 16, numberof(gNV.fTracksToRecord)); // 16);
					NVIsDirty(DEVAINI);
					}
				}
            return MSG_DONE;
			}
//        case MSG_CONFIGURE:
//            if( (gNV.InternalSampleRate>48048) && (channel>=8) )
//            	{
//                 UIMatrixColumnColorSet((UI_MATRIX*)context, 0, COLOR_RED);
//                 UIMatrixColumnColorSet((UI_MATRIX*)context, 1, COLOR_RED);
//                 UIMatrixColumnColorSet((UI_MATRIX*)context, 2, COLOR_RED);
//                 UIMatrixColumnColorSet((UI_MATRIX*)context, 3, COLOR_RED);
//            }
//            break;      /* Allow further processing of this message */

        default: break;
    }
    return 0;
}

/**
 * Setting message handler to access TRACKS TO MIRROR
 */
MsgHandler(OnSettingTracksToMirror)
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    uint channel;
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
			if(channel<16) // gMachineInfo.MaxRecordChannels)
    			cell->Value = logic( gMirrorDrive[DRIVE_CF].MirEnables & (1<<channel) ); //   gmir[gNV.MirrorDrive].cfg.fTracksToMirror[channel];
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
			if( IsMirroring() ) // GUI_GetMirrorMode(gNV.MirrorDrive))
		   		PopUp_MirrorConflict(); // cant change these now!
			else
				{
				channel = MatrixToLinear(8, cell); // ncols 
				if(channel<16) 
					{ // we need this cuz its non volatile....
				  	if(cell->Value) 	gMirrorDrive[DRIVE_CF].MirEnables |=  (1 << channel);
				  	else				gMirrorDrive[DRIVE_CF].MirEnables &= ~(1 << channel);
					SetMirrorConfiguration(DRIVE_CF, &gMirrorDrive[DRIVE_CF]); // this fills gmir[drive].cfg.*  with NV info
			      	NVIsDirty(MIRRORINI);
				  	}
				}
            return MSG_DONE;
        default: break;
    }
    return 0;	
}

MsgHandler(OnMatrix_DisplayRecEnablesTrackName)
{
    static int prevRow = 0, prevCol = 0, idex = 0;
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: 
            {
			UI_MATRIX *p = (UI_MATRIX*)context;
		    if ( (p->ColIndex != prevCol) || (p->RowIndex  != prevRow) ) //   if( MatrixCursorMoved(p, prevCol, prevRow) )
                {
                prevCol = p->ColIndex;
                prevRow = p->RowIndex;
                idex = p->ColIndex + p->RowIndex*8;
                }
			}
            return 0; // MSG_DONE;

        case MSG_DRAW:
			{
            if(UIPageCurrent()->ControlIndex != 1) // only draw if we are drawing the PRI matrix
                return 0;
 		    UI_MATRIX *p = (UI_MATRIX*)context;
            static int prev_idex = 0;
		    char str[50];
            if(  (idex <= numberof(Nomad.Setup.Tracks) )) // (prev_idex!=idex) &&
                {
		        sprintf(str, "%s", Nomad.Setup.Tracks[idex].Name);
                CanvasString(&LcdCanvas, 2, LCD_HEIGHT-15, str); 
                prev_idex = idex;
                }
		    return 0;
            }
        default: break;
    }
    return 0;
}                    


/**	  AUX device is NOT SUPPORTED YET SO JUST IGNORE ATTEMPTS TO CHECK THESE BOXES
 * Setting message handler to access TRACKS TO MIRROR for AUX device
 */
MsgHandler(OnSettingAuxTracksToMirror)
{
    //static UInt8 mem[2][8]; 
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    uint channel;
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
            channel = MatrixToLinear(8, cell); // ncols 
//          MatrixCellGet(&mem[0][0], numberof(mem), numberof(mem[0]), (MATRIX_CELL*)MsgPtr(msg));
//			cell->Value=0;
//			if(channel<16) // gMachineInfo.MaxRecordChannels)
    			cell->Value = 0; // mcfg.fTracksToMirror[channel];
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
            channel = MatrixToLinear(8, cell); // ncols 
//          MatrixCellSet(&mem[0][0], numberof(mem), numberof(mem[0]), (MATRIX_CELL*)MsgPtr(msg));
//			if(channel<16) // gMachineInfo.MaxRecordChannels)
//    			mcfg.fTracksToMirror[channel] = cell->Value; 
//			NVIsDirty(MIRRORINI);
            return MSG_DONE;
        default: break;
    }
    return 0;	
}

//#define FMASK_FADER   (1<<0) //0x01
//#define FMASK_TRIM    (1<<1) //0x02
//#define FMASK_ZAXNET  (1<<2) //0x04
//#define FMASK_DIGITAL (1<<3) //0x08
//#define FMASK_ANALOG  (1<<4) //0x10
//#define FMASK_VIRTUAL (1<<5) //0x20

int Nomad_GetFaderAssign(unsigned iInput, unsigned iFader) // do not call with a digital input index!
{
    int i;//,result=0,a,d; // FTYPE_IS_NOTHING;
	int bank = GetFaderBankIndex(iFader);
	struct FADER_ASS *p = &FaderAssList[bank][iFader]; // short hand
	if(  (iFader >= numberof(FaderAssList)) )  // (iInput>=numberof(gAuxFaderConfig)) ||
		{
		brkpt(1, "GFAauxErr");
		return FADERTYPE_DISABLED;
		}

	if(p->type==FADERTYPE_DISABLED)
		return FADERTYPE_DISABLED;

	//result = a = d = 0;
	for(i=0;i<numberof(p->ch);i++) // search the input list for this particular input channel
		{
		if(p->ch[i] == iInput)
			return p->type;
		}
	return FADERTYPE_DISABLED;
}
#define NOMADGUI_FADER_OFF 0
#define NOMADGUI_FADER_A   1
#if 0
#define NOMADGUI_FADER_D   2
#define NOMADGUI_FADER_T   3
#define NOMADGUI_FADER_Z   4
#endif
// Nomad GUI: OFF,A,D,T,Z
int GetFaderEnum_ForNomadGUI(int input, int fnum) // this input will never be >= IN_CHAN_DIGITAL!!
{  
	struct FADER_ASS *p = GetFaderAssPtr(fnum); // &FaderAssList[bank][fnum]; // short hand
	if (SearchFaderForInput(fnum, input)) 
		return NOMADGUI_FADER_A;

	return NOMADGUI_FADER_OFF; // fader does not contain this input number
}

/*
void RequestNudgeFaders(); 
//enum FADER_CYCLE_MODE {CYCLE_A,CYCLE_D,CYCLE_T,CYCLE_Z}; // what crosspoints we can choose...  press start to change mode in INT FADER ASS PAGE
// this now only handles INTernal fader assigns page - this is the only page that llows multi trim and multi-fader
//int HandleFaderAssignPages(int PageID, MESSAGE *msg) // , int MsgType, void *MsgPtr);
MsgHandler(OnFaderAssignPage) //    msghandler = { 'OnFaderAssignPage'},  -- handles custom drawing and user interaction
{
	if(MsgType(msg) == MSG_PAGE_ENTER)
		{
		; // PopUp_GenericNoPC("Press * to change", "crosspoint type", "", "", 4, 500);
		}
	return 0;
}
*/		
MsgHandler(OnMatrixFaderAssign)
{
//    static int Fcycle=CYCLE_A; // AllowTrim=0;
    //static float TrimRecallTable[12];
    MATRIX_CELL *cell;      // = (MATRIX_CELL*)MsgPtr(msg); this causes MAXX sys_ABORT 
    volatile int gui,tmode, i, HasZnet=Nomad.Settings.HasFeatureZaxnet;
    volatile int FaderNum, InputNum; //    int FaderNum = cell->Row, InputNum = cell->Col; 
 	static int initme=1;

	if(initme && (MsgType(msg) == MSG_KEY_RELEASE) )
		{ //  MSG_PAGE_ENTER doies not work here so this is a kludge to tell the user how to do it
		//PopUp_GenericNoPC("Press * to change", "crosspoint type", "", "", 4, 1000);
		initme=0;
		}

    switch (MsgType(msg)) {
        //case MSG_PAGE_ENTER:            
            //for(i=0;i<10;i++)
            //   TrimRecallTable[i] = GetNVTrim(i); // store all the current trims
			//PopUp_GenericNoPC("Press * to change", "crosspoint type", "", "", 4, 1000);
           // break;
        case MSG_MATRIX_CELL_GET: // data TO UI
		    cell = (MATRIX_CELL*)MsgPtr(msg);  
			InputNum = cell->Col; 
			FaderNum = cell->Row;
			if(UIPageCurrent() == &UIFaderAssignDigital) InputNum += IN_CHAN_DIGITAL_1; 
			if(UIPageCurrent() == &UIFaderAssignRX)      InputNum += IN_CHAN_DIGITAL_RX_1; 
			cell->Value = logic( GetFaderEnum_ForNomadGUI(InputNum, FaderNum) );
            return MSG_DONE;

        case MSG_MATRIX_CELL_SET: // data FROM UI
		    cell = (MATRIX_CELL*)MsgPtr(msg);  
			InputNum = cell->Col; 
			FaderNum = cell->Row; 
			if(UIPageCurrent() == &UIFaderAssignDigital) InputNum += IN_CHAN_DIGITAL_1; 
			if(UIPageCurrent() == &UIFaderAssignRX)      InputNum += IN_CHAN_DIGITAL_RX_1; 
            tmode =  Nomad_GetFaderAssign(InputNum, FaderNum); // returns p->ftype
            Nomad_SetFaderAssign(!tmode, InputNum, FaderNum);
			NVIsDirty(MIXINI); // this is not needed for some reason but should trigger the NV save pixel
            return MSG_DONE;
#ifdef NOMAD
        case MSG_KEY_PRESS:
			if(MsgKey(msg) == KEY_STAR)
            	{             
				int cmax=3;
				if(!HasZnet)
					cmax=2;
               	if(++Fcycle > cmax)
               		Fcycle = 0; // cycle thru 0,1,2,3 (A,D,T,Z)

                if(Fcycle==0)   PopUp_GenericNoPC("(Analog)",      "", "", "", 4, 500);
                if(Fcycle==1)   PopUp_GenericNoPC("(Digital)",     "", "", "", 4, 500);
                if(Fcycle==2)   PopUp_GenericNoPC("(Input Trim)",  "", "", "", 4, 500);
                if(Fcycle==3)   PopUp_GenericNoPC("(Zaxnet Trim)", "", "", "", 4, 500);
               	return MSG_DONE;
               	}
#endif
        default: break;
    }
    return 0;
}                    
// there <were> 3 fader assign pages, Internal 1-8, External 1-4, External 5-8
								 //    0    1    2    3    4
/*
MsgHandler(OnMatrixFaderAssign)  // { ' ', 'A', 'D', 'T', 'Z' },
{
	return HandleFaderAssignPages(0, msg);
}
MsgHandler(OnMatrixExtFaderAssign1)  // { ' ', 'A', 'D', 'T', 'Z' },
{
	return HandleFaderAssignPages(1, msg);
}
MsgHandler(OnMatrixExtFaderAssign2)  // { ' ', 'A', 'D', 'T', 'Z' },
{
	return HandleFaderAssignPages(2, msg);
}
*/

// new plan.....
//if a fader moves it can come on as well as its hard coded trim knob (if applicable)
//if a fader moves and it is not assigned then LEDs do not change

int ActiveFaderToLEDnumBOO(int ActiveFaderNum)
{
;
// if fader is not assigned then no auto-trim is active = LED = OFF
// if 
  return 0;
}

int CalcLEDtrimStateBOO(int ActiveAudioChannelTrim, unsigned  x)
{
	int i,tmode, LEDstate=0;
	
//	if( (gNV.AutoTrim==0) || (x>5) )
      return 0;
}

 
MsgHandler(OnSettingCFmirrorStartSeg)  
{
    int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
               msg->Value.Unsigned = GUI_GetStartSegment(gNV.MirrorDrive);
              return MSG_DONE;
        case MSG_VALUE_SET:   
              mem = msg->Value.Unsigned; 
              if(mem > 999) mem = 1; // allow wrap around for nomad
              if(mem < 1)   mem = 999;
              GUI_SetStartSegment(gNV.MirrorDrive, mem);
			  gMirrorDrive[gNV.MirrorDrive].StartSeg = GUI_GetStartSegment(gNV.MirrorDrive);
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSettingCFmirrorEndSeg)  
{
    int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
               msg->Value.Unsigned = GUI_GetEndSegment(gNV.MirrorDrive);
              return MSG_DONE;
        case MSG_VALUE_SET:   
              mem = msg->Value.Unsigned; 
              if(mem > 999) mem = 1; // allow wrap around for nomad
              if(mem < 1)   mem = 999;
              GUI_SetEndSegment(gNV.MirrorDrive, mem);
//			  gMirrorDrive[gNV.MirrorDrive].EndSeg = GUI_GetEndSegment(gNV.MirrorDrive);
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}

void MirrorTypeChanged(int drive); // triger a re-read of the log file in mirror manager
// assumes this ordering matches the deva mirror.h definitions!
//{ 'BWAV MONO', 'BWAV POLY', 'MP3 32KB', 'MP3 64KB', 'MP3 128KB'  },
MsgHandler(OnSettingCFmirrorFileType)
{
    int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
              msg->Value.Unsigned =  GUI_GetMirrorFileType(gNV.MirrorDrive);
              return MSG_DONE;
        case MSG_VALUE_SET:
				mem = msg->Value.Unsigned;
				if( (mem==2) && !gMachineInfo.AllowMP3) 
					mem=1; // do not allow MP3 selection!!
              GUI_SetMirrorFileType(gNV.MirrorDrive, mem);
			  gMirrorDrive[gNV.MirrorDrive].FileType = GUI_GetMirrorFileType(gNV.MirrorDrive);
			  MirrorTypeChanged(gNV.MirrorDrive); // triger a re-read of the log file in mirror manager
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSettingCFmirrorResolution)
{
    //static int mem;
    int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
               msg->Value.Unsigned =  GUI_GetMirrorFileResolution(gNV.MirrorDrive);
              return MSG_DONE;
        case MSG_VALUE_SET:   
              mem = msg->Value.Unsigned;
              if(mem >= INVALID_RES) mem = RES_24BITS; // 0=24, 1=16, 2=32 float
			  gMirrorDrive[gNV.MirrorDrive].FileResolution = mem;
              GUI_SetMirrorFileResolution(gNV.MirrorDrive, mem);
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}
MsgHandler(OnSettingCFmirrorFileNameMode)
{
    //static int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
               msg->Value.Unsigned =  GUI_GetMirrorFileNameMode(gNV.MirrorDrive);
              return MSG_DONE;
        case MSG_VALUE_SET:   
			  gMirrorDrive[gNV.MirrorDrive].FileNameMode = msg->Value.Unsigned;
              GUI_SetMirrorFileNameMode(gNV.MirrorDrive, msg->Value.Unsigned);
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSettingCFmirrorTCoffset)
{
//  static int mem;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:   
               msg->Value.Unsigned = GUI_GetMirrorTCoffset(gNV.MirrorDrive); 
              return MSG_DONE;
        case MSG_VALUE_SET:   
              GUI_SetMirrorTCoffset(gNV.MirrorDrive, msg->Value.Unsigned);
              gMirrorDrive[gNV.MirrorDrive].TCSkew = GUI_GetMirrorTCoffset(gNV.MirrorDrive); 
              NVIsDirty(MIRRORINI); 
              return MSG_DONE;
        default: break;		       
    }
    return 0;
}




void DisplayStatusLines()
{
    CANVAS *c = UICanvas();
    int x = LCD_WIDTH/2, y, ystep = CanvasFontHeight(c);
    char tmpstr[40];

    y = LCD_HEIGHT-ystep*4;
    CanvasLine(c, 5-1,  y, LCD_WIDTH-5,  y, COLOR_BLACK);  
    y += 6;                     

    UI_GetStatusString(tmpstr, STATUS_STR_LINE1, 25); // "MOUNTED FAT32 VOLUME");
    y=CanvasStringCenter(c, x, y, tmpstr); 
                
    UI_GetStatusString(tmpstr, STATUS_STR_LINE2, 25); // "7.0GB FREE 1.0GB USED");
    y=CanvasStringCenter(c, x, y, tmpstr); 
                
    UI_GetStatusString(tmpstr, STATUS_STR_LINE3, 25); // "MIRRORING IS OFF");
    y=CanvasStringCenter(c, x, y, tmpstr); 
    return;
}

MsgHandler(OnSettingCFmirrorFormatRequest)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                //char tmpstr[40];
                
                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to Format"); 
                    y=CanvasStringCenter(c, x, y, "Mirror Card"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else
                    {
                    y+=ystep*2;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }                         
               
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            return 0; // break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if( (MsgKey(msg) == KEY_STAR) && (StarKeyHasBeenPressed==0) )
                {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
                StarKeyHasBeenPressed++;    // this must happen first (re-enterant) 
                if(StarKeyHasBeenPressed==1)  // 2.79B  - allow only one key press
			     	{
					//if(GUI_GetMirrorMode(gNV.MirrorDrive)==MIR_MODE_OFF)
                  		UI_CFMirrorFormatREQUEST(gNV.MirrorDrive);   
					//else
					//	PopUp_MirrorConflict();	
					 }
               return MSG_DONE;
               }
            }
            break;   
        }
    return 0;
}

void NVSaveAllNow();
MsgHandler(OnSettingFactoryDefaultsRequest)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            // Maybe clear status on entry to this page??
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                
                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to Restore"); 
                    y=CanvasStringCenter(c, x, y, "Factory Defaults"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else if(StarKeyHasBeenPressed<5)
                    {
                    y=CanvasStringCenter(c, x, y, "Please wait..."); 
                    }
                else  //if(StarKeyHasBeenPressed==5) 
                    {
//                  y+=ystep*2;
                    y=CanvasStringCenter(c, x, y, "Default Settings"); 
                    y=CanvasStringCenter(c, x, y, "have been restored."); y+=6;
                    y=CanvasStringCenter(c, x, y, "Restart now"); 
                    }                         
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            return 0; // break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {                    // ONLY DO THIS ONE TIME...
            if( (MsgKey(msg) == KEY_STAR) && (StarKeyHasBeenPressed==0) )
                {                          
		        if( GetTransportMode() != STOP_MODE)  { PopUp_RecordConflict(); return MSG_DONE; }
				if( IsMirroring() ) { PopUp_MirrorConflict(); return MSG_DONE; }

                StarKeyHasBeenPressed++; // this must happen first (re-enterant)    
				NVSetFactoryDefaults();
				//gRTCram.FOLDERMAGIC_5678=0x5678;    // indicates Current folder is valid (0x5678)                     
				//gRTCram.FOLDER = 1; // gNV.CurFolderNum;  // Current folder                      
				// Clear the flash system...
				SFFSInit(TRUE);
				// Force a save to the flash NOW...
				NVSaveAllNow(); // save all files NOW
				ToneInit();
				ToneOff();
				SetRefLevel(gNV.iRefLevel); // cRefLevels[gNV.iRefLevel]);
				InitPeakDetect();
				SetRecordChannels(&gNV.fTracksToRecord[0]);  // &gNV.fTracksToRecord[0])
				SetSampleRate(gNV.InternalSampleRate);
				SetAESReference(gNV.SampleRateRef);
				InitMixer();
				StarKeyHasBeenPressed=5;
               return MSG_DONE;
               }
            }
            break;   
        }
    return 0;
}

MsgHandler(OnSettingClearFaderAssignsRequest)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                
                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to Clear"); 
                    y=CanvasStringCenter(c, x, y, "All Fader Assigns"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else
                    {
//                  y+=ystep*2;
                    y=CanvasStringCenter(c, x, y, "Fader Assignments"); 
                    y=CanvasStringCenter(c, x, y, "have been cleared."); 
//                    y=CanvasStringCenter(c, x, y, "Restart now"); 
                    }                         
                DisplayStatusLines();                
                return 0; // MSG_DONE;
            }
        //    break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {                    // ONLY DO THIS ONE TIME...
            if( (MsgKey(msg) == KEY_STAR) && (StarKeyHasBeenPressed==0) )
                {                          
                StarKeyHasBeenPressed++;  // this must happen first (re-enterant)   
                ClearAllFaderAssigns();
				for(int i=0;i<numberof(gChannelConfig);i++)
					{
					gChannelConfig[i].FaderLog     = 0.0; // 0 is negative infinity dB
					gChannelConfig[i].Fader_pos255 = 0; 
					}

			    NVIsDirty(MIXINI); // 2.92
                return MSG_DONE;
                }
            }
            break;   
        }
    return 0;
}

MsgHandler(OnSettingDeleteLastSeg)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2; // 40;
                int y = 35; // 100;
                int ystep = CanvasFontHeight(c);
                char tmpstr[40], strSegs[10], strFolder[20], strCurSeg[10];

               UI_GetStatusString(strCurSeg, STATUS_STR_CUR_SEG,    5);
               UI_GetStatusString(strSegs,   STATUS_STR_CUR_SEGS,   5);
               UI_GetStatusString(strFolder, STATUS_STR_CUR_FOLDER, 12);
                
               if(StarKeyHasBeenPressed==0)
                    { 
                    y = CanvasStringCenter(c, x, y, "Press * to Delete "); 
                    sprintf(tmpstr,                  "Segment %s", strSegs); 
                    y = CanvasStringCenter(c, x, y,  tmpstr); 
                    sprintf(tmpstr,                  "in Folder %s", strFolder);
                    y = CanvasStringCenter(c, x, y,  tmpstr); 
                    y = CanvasStringCenter(c, x, y, "                  ");  
                    y = CanvasStringCenter(c, x, y, "Press BACK to Exit");  
                    }
                else
                    {
                    y += ystep*2;
                    CanvasStringCenter(c, x, y, "Press BACK to Exit");  y += ystep;
                    }                                        
               
               y = LCD_HEIGHT-ystep*3;     
               CanvasLine(c,   0, y, LCD_WIDTH,  y, COLOR_BLACK);  
               y+=6;
               y = CanvasStringCenter(c, x, y, "Last Segment in");
               sprintf(tmpstr,                 "Folder %s is %s", strFolder, strSegs); 
               y = CanvasStringCenter(c, x, y, tmpstr); 
               //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */

        case MSG_KEY_PRESS:
            {                    // ONLY DO THIS ONE TIME...
            if( (MsgKey(msg) == KEY_STAR)) //  && (StarKeyHasBeenPressed==0) )
				{                              
		       	if( GetTransportMode() == RECORD_MODE)
               		{
			   		PopUp_RecordConflict(); 
               		return MSG_DONE;
               		}
             	StarKeyHasBeenPressed++;     // this must happen first (re-enterant)
             	if(StarKeyHasBeenPressed==1)
                	UI_DeleteLastSeg(); // prevent the ability to repeatedly delete segs 2.79B
             	return MSG_DONE;
               	}
            }
            break;   

        }
    return 0;
}
extern short gRelaxedSectors;
MsgHandler(OnSettingCFprimaryFormatRequest)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                //char tmpstr[40];
                
                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to Format"); 
                    y=CanvasStringCenter(c, x, y, "Primary Card"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else if(StarKeyHasBeenPressed==1)
                    {
                    y=CanvasStringCenter(c, x, y, "This is the PRIMARY CF"); 
                    y=CanvasStringCenter(c, x, y, "card you are about to erase"); 
                    y=CanvasStringCenter(c, x, y, "ARE YOU SURE?"); 
                    y+=3;
                    y=CanvasStringCenter(c, x, y, "Press * to Continue"); 
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else 
                    { // just let the debug screen do all the work...
                    y=28;
                    y=CanvasStringCenter(c, x, y, "This will take about");

					uint32 GB = 1+(disk[0].CapacityLBA/2)/1000000; // 16gb=2.5m  32GB=4.9m  128GB=16.3m
					char tmpstr[30];
					sprintf(tmpstr, "%d minutes (%dGB)", 1+GB/10, GB); // div by 8 if using DSM when formatting FATFS, div by 10 if using raw read write calls
                  	y=CanvasStringCenter(c, x, y, tmpstr); // "about 2 minutes");

                    }                         
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if( (MsgKey(msg) == KEY_STAR) || (MsgKey(msg) == KEY_FRONT_PFL1) )
                {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
		     	if( GetTransportMode() == RECORD_MODE){PopUp_RecordConflict();  return MSG_DONE; }
                StarKeyHasBeenPressed++; // this must happen first (re-enterant)    
                if(StarKeyHasBeenPressed==2)
				  	{
				  	//PartialDebugScreen(11, true); // show the bottom half of the 1967 screen
					if(MsgKey(msg) == KEY_FRONT_PFL1) UI_CFPrimaryFormatREQUEST(99); // partial format
					else					          UI_CFPrimaryFormatREQUEST(1);  // complete format
				  	}
                return MSG_DONE;
                }
            }
            break;   

        case MSG_PAGE_EXIT:
			PartialDebugScreen(-1, false); // restore normal UI screen update 
            break; // return 0;
        }
    return 0;
}


extern float gNewMixerVersionFloat; //  3.12;
MsgHandler(OnSetting_BurnOasisRequest)
{
    static int StarKeyHasBeenPressed=0;
	char tmpstr[50];

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_EXIT:
			StarKeyHasBeenPressed=0;
			gOasisProgramState=0;
			PartialDebugScreen(-1,false); // restore normal UI screen update 
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
             
				if(!connected.ARIA && !connected.OASIS) // OASISconnected())
                    {
			        y=CanvasStringCenter(c, x, y, "Mixer hardware"); 
			        y=CanvasStringCenter(c, x, y, "not connected..."); 
			        y+=ystep;
			        y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
					return 0;
                    }
                
                if(gOasisProgramState==0)
                    {
			        y=CanvasStringCenter(c, x, y, "Press * to update"); 
					sprintf(tmpstr, "Mixer firmware to %3.2f",gNewMixerVersionFloat); 
			        y=CanvasStringCenter(c, x, y, tmpstr); 
			        y+=ystep;
			        y=CanvasStringCenter(c, x, y, "Current version"); 
					float tmpf = GetVersionFloat(OASIS_CONNECTED);
					sprintf(tmpstr, "is %2.2f", tmpf);
			        y=CanvasStringCenter(c, x, y, tmpstr); 
			        y+=ystep;
			        y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else
                    {
      			  	y=CanvasStringCenter(c, x, y, "Wait for LED"); 
      			  	y=CanvasStringCenter(c, x, y, "to stop blinking"); 
      			  	y=CanvasStringCenter(c, x, y, "to prevent dammage..."); 
                    }                         
               
                //DisplayStatusLines();                
                return 0; // MSG_DONE;
            }
        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_FRONT_PFL1) 
				{
				PartialDebugScreen(-1,false); // restore normal UI screen update 
				}

            if(MsgKey(msg) == KEY_HP) 
				{
				gOasisProgramState=0;
			 //	PartialDebugScreen(-1,false); // restore normal UI screen update 
             //   return MSG_DONE;
				}

            if( (MsgKey(msg) == KEY_STAR) && (StarKeyHasBeenPressed==0) )
                {                          
            	if( gOasisProgramState==0 ) 
					gOasisProgramState=1;     // start burning process
                StarKeyHasBeenPressed++;      // this must happen first (re-enterant) 
                //if(StarKeyHasBeenPressed==1)  // allow only one key press
				//	PartialDebugScreen(7, false);
               return MSG_DONE;
               }
            }
            break;   
        }
    return 0;
}

void UI_EraseCurPrimaryFolder();

MsgHandler(OnSettingEraseCurFolder)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            StarKeyHasBeenPressed=0; // Maybe clear status on entry to this page??
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                char tmpstr[40];
                
                if( (DMtpmode==STOP_MODE) && (GUI_GetMirrorMode(gNV.MirrorDrive)==0) )  
                    {
                   	y=CanvasStringCenter(c, x, y, "Press * to erase"); 
                   	y=CanvasStringCenter(c, x, y, "folder"); 
 				    zsnprintf(tmpstr, 30, "%s (%d segs)", folder[DMcurFolder].Name,  (int)folder[DMcurFolder].TotalSegs);
	               	y=CanvasStringCenter(c, x, y, tmpstr); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else // if(StarKeyHasBeenPressed==1)
                    {
                    y=CanvasStringCenter(c, x, y, "Mirroring must be OFF"); 
                    y=CanvasStringCenter(c, x, y, "and unit must be in"); 
                    y=CanvasStringCenter(c, x, y, "STOP mode to proceed."); 
					StarKeyHasBeenPressed=0; // force no key press
                    }                         
               
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
            	{                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
		     	if( GetTransportMode() == RECORD_MODE) { PopUp_RecordConflict();  return MSG_DONE; }
               	if( (DMtpmode==STOP_MODE) && (GUI_GetMirrorMode(gNV.MirrorDrive)==0) ) 
               	  	{
               	  	StarKeyHasBeenPressed++; // this must happen first (re-enterant)    
				  	if(StarKeyHasBeenPressed==1)
				     	UI_EraseCurPrimaryFolder();
				  	}
               	return MSG_DONE;
               	}//if(KEY_STAR)
            }//case KEY_PRESS
            break;   
        }
    return 0;
}


MsgHandler(OnSetting_StoreToMirrorCF_Request)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
            // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                //char tmpstr[40];

                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to store"); 
                    y=CanvasStringCenter(c, x, y, "settings to"); 
                    y=CanvasStringCenter(c, x, y, "Mirror CF Card"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else 
                    { // just let the debug screen do all the work...
                    y=28;
//                  y=CanvasStringCenter(c, x, y, "This will take");
//                  y=CanvasStringCenter(c, x, y, "about 6 minutes");
                    }                         
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
               {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
               	StarKeyHasBeenPressed++;  // this must happen first (re-enterant)   
               	if(StarKeyHasBeenPressed==1)
				  	{
				  	//PartialDebugScreen(11, true); // show the bottom half of the 1967 screen
                	//gMIRcmds.ExportINIfiles=1; // trigger the transfer   
				  	GUI_TriggerExportINIfiles(gNV.MirrorDrive);
				  	}
               	return MSG_DONE;
               	}
            }
            break;   

        case MSG_PAGE_EXIT:
			PartialDebugScreen(-1, false); // restore normal UI screen update 
            return 0;

        }
    return 0;
}

MsgHandler(OnSetting_RecallFromMirrorCF_Request)
{
    static int StarKeyHasBeenPressed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER: // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
                //char tmpstr[40];

                if(StarKeyHasBeenPressed==0)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to recall"); 
                    y=CanvasStringCenter(c, x, y, "settings from"); 
                    y=CanvasStringCenter(c, x, y, "Mirror CF Card"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else 
                    { // just let the debug screen do all the work...
                    y=28;
                    //y=CanvasStringCenter(c, x, y, "This will take");
                    //y=CanvasStringCenter(c, x, y, "about 6 minutes");
                    }                         
                DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
                {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
		    	if( GetTransportMode() == RECORD_MODE){ PopUp_RecordConflict();  return MSG_DONE; }
                StarKeyHasBeenPressed++;  // this must happen first (re-enterant)   
                if(StarKeyHasBeenPressed==1)
				   {
				   //PartialDebugScreen(11, true);    // show the bottom half of the 1967 screen
                 //gMIRcmds.ImportINIfiles=1; // trigger the transfer   
				   GUI_TriggerImportINIfiles(gNV.MirrorDrive);
				   }
               return MSG_DONE;
               }
            }
            break;   

        case MSG_PAGE_EXIT:
			PartialDebugScreen(-1, false); // restore normal UI screen update 
            return 0;

        }
    return 0;
}

void BurnBootLoader_ASYNC(int mode);
bool BootLoaderExists();
char *BurnBootloader_StatLine(int linenum);

MsgHandler(OnSetting_BurnBootLoader_Request)
{
    static int StarKeyHasBeenPressed=0, Displayed=0;

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER: // Maybe clear status on entry to this page??
			StarKeyHasBeenPressed=0;
			Displayed=0;
            break;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
				if(Displayed==FALSE)
					{
				    //PartialDebugScreen(6,true);  // show the bottom half of the 1967 screen
                    //BurnBootLoader_ASYNC(0); // display status only;
                    Displayed++;
					}
                
				if( !BootLoaderExists() )
					{
                    y=CanvasStringCenter(c, x, y, "NO BOOTLOADER FOUND"); 
					}
				else if(StarKeyHasBeenPressed==0) // 10)
                    {
                    y=CanvasStringCenter(c, x, y, "Press * to burn"); 
                    y=CanvasStringCenter(c, x, y, "Bootloader to ROM"); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
                    }
                else 
                    { // just let the debug screen do all the work...
#ifndef RX4
                    //y = 28;
					int yy = LCD_HEIGHT-ystep; // LCD_HIGHT - 10; // start at bottom of the screen
					char str[150];
					int idex = GetLogFileLine(str, -1); // get last line
					for(int i=0;i<12;i++)
						{
						if(idex < 0)
							break;
						CanvasString(c, 1, yy, str );
						yy -= ystep;
					    idex = GetLogFileLine(str, idex); // get prev line
						}
					// y = CanvasStringCenter(c, x, y, BurnBootloader_StatLine(1) ); // "about 6 minutes");
#endif
                    }                         
                //DisplayStatusLines();                
                //return 0; // MSG_DONE;
            }
            break;  /* Allow further processing */
        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
                {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
		    	if( GetTransportMode() == RECORD_MODE){PopUp_RecordConflict();  return MSG_DONE; }
                StarKeyHasBeenPressed++;  // this must happen first (re-enterant)   
                if(StarKeyHasBeenPressed==1)
				  	{
				  	//PartialDebugScreen(11, true);    // show the bottom half of the 1967 screen
					dprintf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); // clear screen 
                  	BurnBootLoader_ASYNC(1); // do it
				  	}
                return MSG_DONE;
                }
            }
            break;   

        case MSG_PAGE_EXIT:
			PartialDebugScreen(-1, false); // restore normal UI screen update 
            return 0;

        }
    return 0;
}


char *GetUSBdebugString(int strnum);
int   GetUSBdebugInt(int num);

MsgHandler(OnSetting_USBstatusPage)
{
	return 0;
}


// * Message handler for PageConfirm to draw status strings - we are not using this feature any more
MsgHandler(OnPageConfirmStatus)
{
    return 0;
}


MsgHandler(OnSettingPanMode12)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gNV.PanMode[0];  return MSG_DONE;
        case MSG_VALUE_SET: 
        	gNV.PanMode[0] = msg->Value.Unsigned;  
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_OUTPUTMIX_PAN_FOLLOW, GetPanModeForSendingToPC() );
	        NVIsDirty(DEVAINI);  
	        return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSettingPanMode34)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gNV.PanMode[1];   return MSG_DONE;
        case MSG_VALUE_SET: 
        	gNV.PanMode[1] = msg->Value.Unsigned; 
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_OUTPUTMIX_PAN_FOLLOW, GetPanModeForSendingToPC() );
	        NVIsDirty(DEVAINI);  
	        return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSettingPanMode56)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gNV.PanMode[2];   return MSG_DONE;
        case MSG_VALUE_SET: 
        	gNV.PanMode[2] = msg->Value.Unsigned;   
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_OUTPUTMIX_PAN_FOLLOW, GetPanModeForSendingToPC() );
        	NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;		       
    }
    return 0;
}

MsgHandler(OnSetting_PreventEmptyPan) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.PreventEmptyPan; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.PreventEmptyPan = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}			

static const char *gAutoPanColTitles[] = { "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "A1", "A2", "A3", "A4", "R1", "R2" };
//static const char *gAutoPanColTitles[] = { "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "A1", "A2", "A3", "A4"}; // remove R1,R2 since they are duplicates of A1,A2 for some rzason
static const char gPanCharLUT[4] = {' ', 'L', 'C', 'R'};

InputBus ConvertColToAutopanInputBus(int col)
{
	if(gAutoPanColTitles[col][0] == 'X')	return DIGITAL_BUS;
	if(gAutoPanColTitles[col][0] == 'D')	return DIGITAL_BUS;
	if(gAutoPanColTitles[col][0] == 'R')	return RETURN_BUS;
//	if(gAutoPanColTitles[col][0] == 'A')
		return ANALOG_BUS;
}
int ConvertColToAutopanInputBase(int col)
{
	if( IsBadIndex(col, numberof(gAutoPanColTitles)) )
		return 0;
	//InputBus bus = ConvertColToAutopanInputBus(col);
	int InputNum = (gAutoPanColTitles[col][1] - '0' - 1);
	if(gAutoPanColTitles[col][0] == 'X') // RXer input?
		InputNum += MAX_AES_INPUTS; // was 4 // offset of RXers from start of digital inputs	
	return InputNum;		
}

static ALARM gAutoPanTextAlarm=0;
static char gAutoPanText[16];
// call this when auto pan changes something so it shows up in HP text in uihome.cpp
void SetAutoPanText(int col, uint PanType)
{
	char tmpstr[10], c='A';
	memset(gAutoPanText, 0, sizeof(gAutoPanText)); // just in case...

	if(gAutoPanColTitles[col][0]=='X') // previx "X" with an RX to make it an RXer
		sprintf(tmpstr, "R%s=%c", gAutoPanColTitles[col], gPanCharLUT[PanType]); // "RX4=L"
	else
		sprintf(tmpstr, "%s=%c", gAutoPanColTitles[col], gPanCharLUT[PanType]); // "A4=L"
	strcpy(gAutoPanText, tmpstr);
	SET_ALARM(gAutoPanTextAlarm, 1000); // display the text for 1 second
	return;
} 
// if auto pan just changed jam the text instead of HP text
int GetAutoPanText(char *str) // called in uihome.cpp
{
	if( (GMSECS < 3000) || CHECK_ALARM(gAutoPanTextAlarm) )
		return 0;
	strcpy(str, gAutoPanText);
    return 1;     
} 
int AutoPanEnabled() 
{	
	return gNV.AutoPan;
}

//#define PAN_TYPE_OFF    0
//#define PAN_TYPE_LEFT   1
//#define PAN_TYPE_RIGHT  2
//#define PAN_TYPE_CENTER 3

//#define XPT_OFF 1
//#define XPT_ON  2

// look at output mix 12, 34, or 56 (OutputMixPair=0,1,2) and determine what pan assigns were selected
// return a pan type so UI knows what to display in the box
int Observe_PanAssign(unsigned col, int OutputMixPair) 
{
	unsigned  i=0; // , actionL, actionR;  // assert(InputBusNum<8);	
	XPOINT_STATE x, y; // bit field... see AudioMix.h
   
	//   this is removing things according to jack
	if(IsBadIndex(col, numberof(gNV.PanMatrix)) ) return 0; // if( InputBusNum > numberof(gNV.PanMatrix) ) return 0;
	int input    = ConvertColToAutopanInputBase(col);
	InputBus bus = ConvertColToAutopanInputBus(col);

	i = OutputMixPair*2;

	if(bus == DIGITAL_BUS)
		{
		x = GetCrossPoint(OUTPUT_MIXER, bus, input, i+0); // Left
		y = GetCrossPoint(OUTPUT_MIXER, bus, input, i+1); // Right
            
	    if( (x.B == DIGITAL) && (y.B == DIGITAL) ) return PAN_TYPE_CENTER;
    	if  (x.B == DIGITAL)  return PAN_TYPE_LEFT;
		if  (y.B == DIGITAL)  return PAN_TYPE_RIGHT;
		}
	else
		{ // analog or return bus...
		x = GetCrossPoint(OUTPUT_MIXER, bus, input, i+0); // Left
		y = GetCrossPoint(OUTPUT_MIXER, bus, input, i+1); // Right
            
	    if( (x.A == ANALOG) && (y.A == ANALOG) ) return PAN_TYPE_CENTER;
    	if  (x.A == ANALOG)  return PAN_TYPE_LEFT;
		if  (y.A == ANALOG)  return PAN_TYPE_RIGHT;
		}	

	return PAN_TYPE_OFF;
}     		
// PWR key can call this as well as the ENG home screen
static void AdjustPanSetting(int col, int PanType)
{
	assert(col < numberof(gNV.PanMatrix));
	if( (gNV.PreventEmptyPan) && (PanType==0) )
		{
		if(gNV.PanMatrix[col]==1) PanType=3; // wrap backwards past zero
		else                      PanType=1; // skip zero when going forward
		}
	gNV.PanMatrix[col] = PanType; // only used locally now for wrap direction detection
	SetAutoPanText(col, PanType);
    Nomad_UpdatePanAssign(col); // int inputnum, InputBus bus);
	NVIsDirty(MIXINI);  
	return;
}

// don't forget to call NVIsDirty(DEVAINI) and  NVIsDirty(MIXINI)
void Nomad_UpdatePanAssign(unsigned col) // unsigned input, InputBus bus) // DO NOT run this at boot up! pan assigns are suggestions only
{
   unsigned  i, actionL, actionR;  // assert(InputBusNum<8);	
   if(IsBadIndex(col, numberof(gNV.PanMatrix)) ) return; //  InputBusNum > numberof(gNV.PanMatrix)) return;
   int input    = ConvertColToAutopanInputBase(col);
   InputBus bus = ConvertColToAutopanInputBus(col);

	gNV.PanMode[0]=1; // as of V2.20 Pan Mode 12 is always on
	for(i=0;i<MAX_OUTPUTS;i+=2) // do for all 6 pannable output busses
		{
		if(gNV.PanMode[i/2]) // is this output pair pannable?
      		{ // Yes
			actionL = actionR = XPT_OFF; // (1)
			if( (gNV.PanMatrix[col]==PAN_TYPE_LEFT)  || (gNV.PanMatrix[col]==PAN_TYPE_CENTER) ) 
			   actionL = XPT_ON; // (2)
			if( (gNV.PanMatrix[col]==PAN_TYPE_RIGHT) || (gNV.PanMatrix[col]==PAN_TYPE_CENTER) ) 
			   actionR = XPT_ON; // (2)
      		SetCrossPoint              (OUTPUT_MIXER, bus, input, i+0, actionL, TRUE);
			UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, bus, input, i+0);
      		SetCrossPoint              (OUTPUT_MIXER, bus, input, i+1, actionR, TRUE);
			UpdatePCwithNomadCrosspoint(OUTPUT_MIXER, bus, input, i+1);
		    SavePreset(OUTPUTMIX_PRESET, gNV.OutputMixPresetNum); 
		    fOutputMixDirty=1;
			}
		}
	return;
}     		

short GetLastActiveFaderNum();
short GetLastActiveFaderAudioChan();
short ConvertAudioChanToMatrixCol(unsigned input)
{
	for(int col=0;col<numberof(gAutoPanColTitles);col++)
		{
		int result   = ConvertColToAutopanInputBase(col);
		//InputBus bus = ConvertColToAutopanInputBus(col);
				
		if(gAutoPanColTitles[col][0] == 'X')	result += IN_CHAN_DIGITAL_1; // IN_CHAN_DIGITAL_RX_1;
		if(gAutoPanColTitles[col][0] == 'D')	result += IN_CHAN_DIGITAL_1;
		if(gAutoPanColTitles[col][0] == 'R')	result += IN_CHAN_RETURN_1;
		if(result == input)
			return col; // found match!!
		}
	return -1; // audio chan not found in matrix list
}

bool UIMatrixMoveTo(UI_MATRIX *p, int row, int col);

void HandleMatrixCursorAutoPanMovement(UI_MATRIX *p)
{
	static int prevAutoTrimChan = -1;
	int chan = GetLastActiveFaderAudioChan();
	if( (chan>=0) && (chan != prevAutoTrimChan) )
		{
		int col = ConvertAudioChanToMatrixCol(chan);
		if(col >= 0)
			{
		    //UIMatrixMoveTo(p, 0, col); // m->Target.Row, m->Target.Col);
	        p->ColIndex = col;
    	    UI_UIsetupRefreshRequest();
			}
		prevAutoTrimChan = chan;
		}
}

short gPanDsplayOffset=0;
MsgHandler(OnSettingPanMatrix)  // menu = { ' ', 'L', 'R', 'C' },
{

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET:{ 
			UI_MATRIX *mm = (UI_MATRIX*)context;
			HandleMatrixCursorAutoPanMovement((UI_MATRIX*)context);
		    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
			//unsigned InputNum = cell->Col;
			unsigned PanType  = cell->Value;
			assert(cell->Col < numberof(gNV.PanMatrix)); // 8
//			cell->Value = gNV.PanMatrix[InputNum];
            cell->Value = Observe_PanAssign(cell->Col, 0); // look at output 12 mix to see what pan assign is there
			if( ((cell->Value!=Observe_PanAssign(cell->Col, 1))&&gNV.PanMode[1]) || 
			    ((cell->Value!=Observe_PanAssign(cell->Col, 2))&&gNV.PanMode[2])  )
       			UIMatrixColumnColorSet((UI_MATRIX*)context, cell->Col, COLOR_RED); // show if outputs 34 or 56 are not in sync with out12
			else 
       			UIMatrixColumnColorSet((UI_MATRIX*)context, cell->Col, COLOR_BLACK); // all is ok with this input setting
            return MSG_DONE;        
		}

        case MSG_MATRIX_CELL_SET:{ // data FROM UI
		    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
			//unsigned InputNum = cell->Col;
			unsigned PanType  = cell->Value;
			AdjustPanSetting(cell->Col, PanType); // PWR key can call this too now
            return MSG_DONE;
		}

        case MSG_MATRIX_CELL_COL_TITLE: {
            MATRIX_CELL *cell = MsgMatrixCell(msg);
//            static const char *colTitles[] = { "X1", "X2", "X3", "X4", "D1", "D2", "D3", "D4", "A1", "A2", "A3", "A4", "R1", "R2" };
            assert(cell->Pointer);
            if(cell->Col < numberof(gAutoPanColTitles)) {
                snprintf((char*)cell->Pointer, MAX_UI_TEXT,  "%s", gAutoPanColTitles[cell->Col]);
            }
        }
            return MSG_DONE;

        case MSG_FOCUS_QUERY: // see if we have proper focus
			msg->Value.Unsigned = Nomad.Settings.HomeMatrixActive;
            return MSG_DONE;
        default: break;
    }
    return 0;
}

#ifdef ALLOW_AUTOPAN  // WHAT IS AUTO PAN AND DOES RX4 HAVE IT?
//Setting { name = 'auto.pan', menu = {'OFF', 'ON'}, msghandler = 'OnSetting_AutoPan_Enable' }
MsgHandler(OnSetting_AutoPan_Enable) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.AutoPan;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.AutoPan  = msg->Value.Signed; NVIsDirty(DEVAINI);return MSG_DONE;
        default: break;
    }
    return 0;
}
void IncDecAutoPan(int inc) // called by PWR key press
{
    int i, AudioChannel, IsDigital, FaderNum;
    int PanType; //matrix looks like this on MAXX:  A1 A2 A3 A4 D1 D2 D3 D4
	char tmpstr[18];

	if( (gNV.AutoPan==0) || PFLIsActive() ) // allow autopan?
		return;

    bool valid = GetActiveAutoTrim(&AudioChannel, &FaderNum, &IsDigital); 
	if(!valid)
		return;

	if(AudioChannel<0) // make sure we have a valid channel number even if knob is not asigned
	   AudioChannel = FaderNum;

	if(IsDigital) 
		AudioChannel += MAX_IN_CHAN_ANALOG; // (was 6 to move to proper place on ENG home screen matrix)

	if(IsBadIndex(AudioChannel, sizeof(gNV.PanMatrix) ))  /// ERROR FIX THIS!!!!! (INDIG_6 has a wacky value sometimes)
		return; // do nothing I guess

	PanType = gNV.PanMatrix[AudioChannel];
	if(inc>0) PanType++;
	else      PanType--;
	if(PanType<0) PanType=3;
	if(PanType>3) PanType=0;
	AdjustPanSetting(AudioChannel, PanType); // this will wrap the pantype 

//RequestENGscreen(1000);
//	#if 0
//	for(i=0;i<8;i++)
//		{
//		tmpstr[i*2+0] = ' ';
//		tmpstr[i*2+1] = PanCharLUT[ gNV.PanMatrix[i] ];
//		}
//	tmpstr[i*2] = 0;
//	PopUp_Generic(" Auto Pan", "A A A A A A D D D D", "1 2 3 4 5 6 1 2 3 4", tmpstr, 4, 1200); 
//	#endif
	return;
}
#endif // NOMAD



MsgHandler(OnSetting_TA5OUT1)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[0].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS1, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_TA5OUT2)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[1].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS2, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

int Is10dBAttenuated(int gain)
{
	return ( (int)gain == -10);
}
MsgHandler(OnSetting_TA5OUT1A_NOVA2)  // menu ={ 'MIC', 'LINE',   'MIC-10dB', 'LINE-10dB' },
{
    float *iGain = &gOutputChanConfig[0].Gain; // use this as our secondary output copy gain in nova
    switch (MsgType(msg)) {
		case MSG_VALUE_GET: { 			
			msg->Value.Signed=0; // assume MIC level
			if((int)(*iGain) > -30)
				msg->Value.Signed = 1; // assume LINE level
			if(Is10dBAttenuated(gOutputChanConfig[0].GainTAfive)) // look at main output's attenuation for this
				msg->Value.Signed += 2; // skip to attenuated settings
        	return MSG_DONE;
			}

        case MSG_VALUE_SET: {  // there are only two settings really, mic or line. The -10dB part comes from OnSetting_TA5OUT1
			static int prev = 0;
			volatile int LineLevel = !Is10dBAttenuated(gOutputChanConfig[0].GainTAfive); // look at main output's attenuation for this
			// only two choices now
			prev = !prev;
			LineLevel  = prev; // just toggle the setting blindly
			dprintf("msg->Value.Signed = %d, prev=%d line=%d\n",msg->Value.Signed,prev,LineLevel);
			if(LineLevel)	UI_SetBusGainSetting(eBUS1A, 1, iGain, ODBCINI); // choose   0dB 
			else  			UI_SetBusGainSetting(eBUS1A, 0, iGain, ODBCINI); // choose -35dB  
        	return MSG_DONE;
			}
/*
		case MSG_ENCODER:
			{
			int LineLevel = (msg->Value.Signed>0); // turning knob to the right makes it line level
			if(LineLevel)	UI_SetBusGainSetting(eBUS1A,   0, iGain, ODBCINI); // choose   0dB 
			else  			UI_SetBusGainSetting(eBUS1A, -35, iGain, ODBCINI); // choose -35dB  
        	return MSG_DONE;
			}
*/
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TA5OUT1A_NOVA1)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[0].Gain; // use this as our secondary output copy gain in RX4
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS1A, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TA5OUT2A_NOVA2)  // menu ={ 'MIC', 'LINE',   'MIC-10dB', 'LINE-10dB' },
{
    float *iGain = &gOutputChanConfig[1].Gain; // use this as our secondary output copy gain in nova
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: { 
			msg->Value.Signed=0; // assume MIC level
			if((int)(*iGain) > -30)
				msg->Value.Signed = 1; // assume LINE level
			if(Is10dBAttenuated(gOutputChanConfig[1].GainTAfive)) // look at main output's attenuation for this
				msg->Value.Signed += 2; // skip to attenuated settings
        	return MSG_DONE;
			}
        case MSG_VALUE_SET: {  // there are only two settings really, mic or line. The -10dB part comes from OnSetting_TA5OUT1
			static int prev = 0;
			volatile int LineLevel = !Is10dBAttenuated(gOutputChanConfig[0].GainTAfive); // look at main output's attenuation for this
			// only two choices now
			prev = !prev;
			LineLevel  = prev; // just toggle the setting blindly
			dprintf("msg->Value.Signed = %d, prev=%d line=%d\n",msg->Value.Signed,prev,LineLevel);
			if(LineLevel)	UI_SetBusGainSetting(eBUS2A, 1, iGain, ODBCINI); // choose   0dB 
			else  			UI_SetBusGainSetting(eBUS2A, 0, iGain, ODBCINI); // choose -35dB  
        	return MSG_DONE;
			}
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TA5OUT2A_NOVA1)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[1].Gain;  // use this as our secondary output copy gain in RX4
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS2A, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TA5OUT3)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[2].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS3, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_TA5OUT4)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[3].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS4, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;							  
    }
    return 0;
}
MsgHandler(OnSetting_TA5OUT5)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[4].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS5, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;							  
    }
    return 0;
}
MsgHandler(OnSetting_TA5OUT6)  // menu = { '-35dBu', '-10dBu', '0dBu' }
{
    float *iGain = &gOutputChanConfig[5].GainTAfive;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = UI_GetBusGainSetting_GainToEnum(*iGain);  return MSG_DONE;
        case MSG_VALUE_SET:  UI_SetBusGainSetting(eBUS6, msg->Value.Signed, iGain, ODBCINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}



float gIFBTxGainLinear = 1.0;
void SetIFBTXGain(float g) // gNV.IFB_TxGain)
{
	gIFBTxGainLinear = powf(10,g/20.0F);	
}
MsgHandler(OnSetting_IFB_TxGain)  // special TAPEout gain adjustment to prevent IFB from clipping  range={0,40}, units='dBu' },
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.IFB_TxGain;  return MSG_DONE;
        case MSG_VALUE_SET:  
			gNV.IFB_TxGain = msg->Value.Signed;  
        	SetIFBTXGain(gNV.IFB_TxGain); 
	        return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_RET1)  // range={-40,8}, units='dBu', },
{
    float *iGain = &gChannelConfig[IN_CHAN_RETURN_1].Trim;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = *iGain;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	UI_SetBusGainSetting(eRET1, msg->Value.Signed, iGain, AICINI);   
			//if(context != gPCcontext) this is done in OasisSTNupdateHandler
			//	SendSetting_1D_indexed(SETTING_INPUT_TRIM_LEVEL, *iGain, IN_CHAN_RETURN_1,18); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_RET2)  // range={-40,8}, units='dBu', },
{
    float *iGain = &gChannelConfig[IN_CHAN_RETURN_2].Trim;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = *iGain;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	UI_SetBusGainSetting(eRET2, msg->Value.Signed, iGain, AICINI);   
			//if(context != gPCcontext)this is done in OasisSTNupdateHandler
			//	SendSetting_1D_indexed(SETTING_INPUT_TRIM_LEVEL, *iGain, IN_CHAN_RETURN_2,18); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

//slate gain
MsgHandler(OnSetting_INTSLATE)  // range={-40,8}, units='dB', },
{
	float g = 	gNV.IntSlateGain; // convert to float pointer
    float *iGain = &g;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = *iGain;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	UI_SetBusGainSetting(eINTSLATE, msg->Value.Signed, iGain, DEVAINI);   
			gNV.IntSlateGain = g; // store it
        	return MSG_DONE;
        case MSG_EDITOR_GET:
            /* Return pointer to the editor for this setting type. Use the simple editor
             * that doesn't allow numeric entry, only encoder. */
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_EXTSLATE)  // WE NEED A SEPARATE CASE FOR THIS SETTING
{
	float g = 	gNV.ExtSlateGain; // convert to float pointer
    float *iGain = &g;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = *iGain;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	UI_SetBusGainSetting(eINTSLATE, msg->Value.Signed, iGain, DEVAINI);   
			gNV.ExtSlateGain = g; // store it
        	return MSG_DONE;
        case MSG_EDITOR_GET:
            // Return pointer to the editor for this setting type. Use the simple editor
            // that doesn't allow numeric entry, only encoder. 
            msg->Value.Pointer = &UIEditorNumSimple.Editor;
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_ToneFreq) 
{                       
	float inc=0;
    static unsigned mem = round(gNV.TestToneFreq); 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = mem;  return MSG_DONE;
        case MSG_VALUE_SET:  
           if     (msg->Value.Unsigned == mem+1) inc =  10.0; // if(msg->Value.Unsigned>  mem *= 1.1;
           else if(msg->Value.Unsigned == mem-1) inc = -10.0;
           else if(msg->Value.Unsigned == mem+10) inc = 100.0; // holding knob down and rotating
           else if(msg->Value.Unsigned == mem-10) inc = -100.0;
		   else    mem = msg->Value.Unsigned/10; // numeric entry mode
		   
		   if(mem>=900)  inc *=10; // 100Hz  // maintain nice round numbers
		   if(mem>=2000) inc *=2;  // 200Hz 
		   mem += inc;
		   if(mem<20)    mem=20; // clip
		   if(mem>20000) mem=20000;
		   gNV.TestToneFreq = mem;
		   SetToneFreq(gNV.TestToneFreq);   // this sets gNV.TestToneFreq
           NVIsDirty(DEVAINI);
           return MSG_DONE;

        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_ToneLevel) 
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.iRefLevel;  return MSG_DONE;
        case MSG_VALUE_SET:  
           gNV.iRefLevel = msg->Value.Signed;  
           SetRefLevel(gNV.iRefLevel);
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TONE_LEVEL, msg->Value.Signed);
           NVIsDirty(DEVAINI); 
           return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingSetupChanMicLine)
{
    int ch=clip(Nomad.Setup.Chan, 0, numberof(Nomad.Setup.Channel));
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG){  msg->Value.Unsigned=0; return MSG_DONE; } // force to LINE mode (for returns)

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = (0 == gChannelConfig[ch].fMicLevel); // inverse
            return MSG_DONE;
        case MSG_VALUE_SET:				
        	gChannelConfig[ch].fMicLevel = (0 == msg->Value.Unsigned); // inverse
		    //SetMicLevel(ch, gChannelConfig[ch].fMicLevel);
			// this call handles all conflicts and popups....
	        UI_Set48V(ch, gChannelConfig[ch].f48V, context);
			//if( (!gChannelConfig[ch].fMicLevel) && gChannelConfig[ch].f48V)
			//	{
			//    gChannelConfig[ch].f48V = 0; // DO NOT ALLOW LINE AND 48V AT THE SAME TIME so turn off 48V
			//	PopUp_Generic("48V not allowed", "in LINE level", "", "", 4, 1000);
		    //    UI_Set48V(ch, gChannelConfig[ch].f48V, context);
			//    }
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_LEVEL, ConvertNovaChanToPC(ch), (0==gChannelConfig[ch].fMicLevel), 18); // dont re-send if just set
            NVIsDirty(AICINI);
            return MSG_DONE;
        default: break;
    }
    return 0;
}

void UI_Set48V(int ch, int OffOnToggle, void *context)
{
    if(ch>LAST_CDSCELL_INPUT) // IN_CHAN_ANALOG_6) 
    	return;

	if(OffOnToggle==2) // toggle mode?
		OffOnToggle = !gChannelConfig[ch].f48V;

	if(OffOnToggle && (gChannelConfig[ch].fMicLevel==0) ) // 48V AND LINE level?
		{// DO NOT ALLOW LINE AND 48V AT THE SAME TIME
		PopUp_Generic("48V not allowed", "in LINE level", "", "", 4, 1500);
		OffOnToggle=0;
		//gChannelConfig[ch].f48V=0;
		//if(context != gPCcontext)
		//	SendSetting_1D_indexed(SETTING_INPUT_POWER48V, ConvertNovaChanToPC(ch), gChannelConfig[ch].f48V, 18);//, true); // forceit make sure all cases are sent back
	    //NVIsDirty(AICINI);
		//return;
		}

	gChannelConfig[ch].f48V = OffOnToggle;
	SendSetting_1D_indexed(SETTING_INPUT_POWER48V, ConvertNovaChanToPC(ch), gChannelConfig[ch].f48V, 18);//, true); // forceit make sure all cases are sent back
    NVIsDirty(AICINI);
	return;
}

MsgHandler(OnSettingSetup48V)
{
    int ch=Nomad.Setup.Chan;
    //if(ch>IN_CHAN_ANALOG_6) { msg->Value.Unsigned = 0; return MSG_DONE;}

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = gChannelConfig[ch].f48V;
            return MSG_DONE;
        case MSG_VALUE_SET:		
			UI_Set48V(ch, msg->Value.Unsigned, context); // this has Line Level protection checking in it too
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSetupMix8Trim)
{
    int ch=Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = logic(gChannelConfig[ch].Mix8TrimMode);  return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Unsigned) gChannelConfig[ch].Mix8TrimMode = TRIMMODE_ZAXNET; // 2 is greg's magic number
			else					gChannelConfig[ch].Mix8TrimMode = 0; // 1=mix8 = never used
			if(context != gPCcontext)
	  	  		SendSetting_1D_indexed(SETTING_INPUT_TRIM_MODE, ConvertNovaChanToPC(ch), msg->Value.Unsigned, 18);
	        NVIsDirty(AICINI); 
 	        return MSG_DONE;
        default: break;
    }
    return 0;
}


// hardware HPF and digital HPF seting On OFF only (also controls channels higher than 6 too but DSP HPF only in this case
MsgHandler(OnSettingSetupHPF)
{
    int ch=Nomad.Setup.Chan;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if(ch>=MAX_IN_CHAN_ANALOG){  msg->Value.Unsigned=0; return MSG_DONE; }// force to OFF (for returns)

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = gChannelConfig[ch].fHPF; // BOOL
            return MSG_DONE;
        case MSG_VALUE_SET:		
			gChannelConfig[ch].fHPF = msg->Value.Unsigned; // BOOL
		    //Set_HPF(ch, gChannelConfig[ch].fHPF); handled by HPF_UpdateMonitor() now.....
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_HPF_ENABLE, ConvertNovaChanToPC(ch), msg->Value.Unsigned,18); 
            NVIsDirty(AICINI);
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingSetupChan_DigHPF_Freq) // DSP based HPF freq setting - not OnOff 
{
	int ch, freq, delta; // note HPF is not part of the normal EQ system
	ch = Nomad.Setup.Chan;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG) {  msg->Value.Signed = 0;  return MSG_DONE;  }
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
			msg->Value.Signed = round(gChannelConfig[ch].HighPassFreq);
    	    return MSG_DONE;
        case MSG_VALUE_SET:
			if(context == gPCcontext)
	           	freq = msg->Value.Signed; // PC GUI is calling us !!!!! 
			else
				{
				freq = gChannelConfig[ch].HighPassFreq; // int now   round(gChannelConfig[ch].HighPassFreq);
				delta = msg->Value.Signed - freq; 
				if(abs(delta)==1) freq  = freq + delta*10; // inc by 10Hz when knob is rotated
				else freq = (msg->Value.Signed/10)*10; // numeric entry mode...
           		} 
   	        if(freq > 230) freq = 230;
       	    if(freq < 40)  freq = 40;
       		gChannelConfig[ch].HighPassFreq = (uchar)freq;
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_HPF_FREQ, ConvertNovaChanToPC(ch), (uchar)freq,18); 
			NVIsDirty(AICINI);	// HPFInUseUpdate() is called periodically to handle these changes
            return MSG_DONE;
        default: break;
    }
    return 0;
}

//extern int gMaxDelaymsecs;   // =40;
//extern int gMaxDelaySamples; // #define MAX_DELAY_SAMPLES 8192  (170ms at 48k)

MsgHandler(OnSettingSetupDelay_OnOff)
{
    int ch=Nomad.Setup.Chan; // , MaxDelay = gMaxDelaymsecs;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if( (Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG_DIGITAL) && (MsgType(msg)==MSG_VALUE_GET) || (MsgType(msg)==MSG_VALUE_SET) )
    //	{  
    //	msg->Value.Unsigned=0; 
    //	return MSG_DONE; 
    //	}

    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gChannelConfig[ch].DelayOnOff;  return MSG_DONE;
        case MSG_VALUE_SET:	
        	gChannelConfig[ch].DelayOnOff = msg->Value.Unsigned; 
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_DELAY_ENABLE, ConvertNovaChanToPC(ch), msg->Value.Unsigned, 18); 
        	NVIsDirty(AICINI);  
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

// this calulation is also used to init the PC on startup 
// both the PC and LUA interface use p1ms whereas Nova internally uses samples
float ConvertDelay_Point1ms_To_Samples(uint32 point1ms)
{
	float tmpsamples = (point1ms * gNV.InternalSampleRate);
	tmpsamples /= 10000.0F;
	tmpsamples = round(tmpsamples);
	if(tmpsamples >= MAX_INPUT_DELAY_SAMPLES)
		return MAX_INPUT_DELAY_SAMPLES;
	return tmpsamples;
}
int32 ConvertDelay_Samples_To_Point1ms(uint32 samples)
{
	return round((float)(samples*10000) / (float)gNV.InternalSampleRate);
}
MsgHandler(OnSettingSetupDelay_MS)
{
    int ch=Nomad.Setup.Chan; // , MaxDelay = gMaxDelaymsecs;
    float tmpsamples, tmpf;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned = ConvertDelay_Samples_To_Point1ms(gChannelConfig[ch].DelaySamples); // round((float)(gChannelConfig[ch].DelaySamples*10000) / (float)gNV.InternalSampleRate);
            return MSG_DONE;
        case MSG_VALUE_SET:		
			//gChannelConfig[IN_CHAN_ANALOG+0].DelayMode = DELAY_IN_SAMPLES; // (1st channel determines delay mode)
			//gChannelConfig[IN_CHAN_DIGITAL+0].DelayMode= DELAY_IN_SAMPLES; // DELAY_IN_MSECS
			//tmpsamples = (msg->Value.Unsigned * gNV.InternalSampleRate);
			//tmpsamples /= 10000.0F;
			//tmpsamples = round(tmpsamples);
			//if(tmpsamples >= MAX_INPUT_DELAY_SAMPLES) // gMaxDelaySamples)
			//	gChannelConfig[ch].DelaySamples = MAX_INPUT_DELAY_SAMPLES; // gMaxDelaySamples;
			//else
			//	gChannelConfig[ch].DelaySamples = tmpsamples;
			gChannelConfig[ch].DelaySamples = ConvertDelay_Point1ms_To_Samples(msg->Value.Unsigned);
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_DELAY_TIME, ConvertNovaChanToPC(ch), msg->Value.Unsigned, 18); 
            NVIsDirty(AICINI);
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSetupPhase)
{
    int ch=Nomad.Setup.Chan;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG){  msg->Value.Unsigned=0; return MSG_DONE; }

    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gChannelConfig[ch].phase;  return MSG_DONE;
        case MSG_VALUE_SET:	
        	gChannelConfig[ch].phase = msg->Value.Unsigned; 
			if(context != gPCcontext)
				SendSetting_1D_indexed(SETTING_INPUT_PHASE, ConvertNovaChanToPC(ch), msg->Value.Unsigned,18); 
        	NVIsDirty(AICINI);  
        	return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSetting_DebugBits)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned =  gNV.DebugBits;    return MSG_DONE;
        case MSG_VALUE_SET:	gNV.DebugBits = msg->Value.Unsigned; NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingTransportShifted)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
            msg->Value.Unsigned     = gNV.TransportShifted;
            Nomad.Transport.Shifted = gNV.TransportShifted;
            return MSG_DONE;
        case MSG_VALUE_SET:		
      	    Nomad.Transport.Shifted = msg->Value.Unsigned;
		    gNV.TransportShifted    = msg->Value.Unsigned;
			NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingCharger)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:    msg->Value.Unsigned = gNV.Charger;    return MSG_DONE;
        case MSG_VALUE_SET:	   gNV.Charger = msg->Value.Unsigned; NVIsDirty(DEVAINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingAutoTrim)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned =  gNV.AutoTrim;  return MSG_DONE;
        case MSG_VALUE_SET:	gNV.AutoTrim = msg->Value.Unsigned;  NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

int EQisRunning(int ch)
{
	int i, count=0;;

	for(i=0;i<numberof(gChannelConfig[ch].EQ);i++)
		if(gChannelConfig[ch].EQ[i].type)
			count++;
	return count;
}

float gDefaultNotch_Q = 9.9;
void Nomad_SetNotchFilter(int NotchNumber, int mChannel, int freq, float Q, float dBgain, int OnOff)
{
	int i,tmp, band = NotchNumber;

    if(OnOff) gChannelConfig[mChannel].EQ[band].type = EQ_TYPE_NOTCH;
	else      gChannelConfig[mChannel].EQ[band].type = EQ_TYPE_OFF;

	gChannelConfig[mChannel].EQ[band].frequency = freq;
	gChannelConfig[mChannel].EQ[band].Q         = Q;
	gChannelConfig[mChannel].EQ[band].dBgain    = 0;

	if( EQisRunning(mChannel) ) //  (gChannelConfig[mChannel].EQ[0].type) || (gChannelConfig[mChannel].EQ[1].type) )
	    gChannelConfig[mChannel].fEQInLine = TRUE;	// somebody is running
	else	  	
	  	gChannelConfig[mChannel].fEQInLine = FALSE; // both notches are off so kill ALL EQs all together

	UpdateEQBypass(mChannel);
	UpdateEQCoefs(mChannel, gChannelConfig[mChannel].EQ);
	NVIsDirty(AICINI);
}

int Nomad_GetNotchFilter(int NotchNumber, int mChannel, int *freq, float *Q, float *dBgain)
{
	int i,tmp, band = NotchNumber;

	*freq   = gChannelConfig[mChannel].EQ[band].frequency;
	*Q      = gChannelConfig[mChannel].EQ[band].Q;
	*dBgain = gChannelConfig[mChannel].EQ[band].dBgain;

	if(gChannelConfig[mChannel].EQ[band].type) 
	   return TRUE;
  
    return FALSE;
}

/**
 * Setting message handler for pseudo setting that accesses NOTCH setting 
 * for currently selected Setup.Chan.
 *
 * @see OnPageSetupChanX
 */
int UI_GetNotchOnOffState(int NotchNum) // for ui.lua
{
	int OnOff, ch, trim, delta; // note notch1 resides in band = [0]
	int freq; float Q, dBgain;
	MESSAGE msg;
	msg.Type = MSG_VALUE_GET;
	return Nomad_GetNotchFilter(NotchNum, Nomad.Setup.Chan, &freq, &Q, &dBgain);
}
MsgHandler(OnSettingSetupChanNotch1OnOff)	
{
	int OnOff, ch, trim, delta, band=0; // note notch1 resides in band = [0]
	int freq; float Q, dBgain;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG) { msg->Value.Signed = 0; return MSG_DONE; }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
			msg->Value.Signed = OnOff;
    	    return MSG_DONE;
        case MSG_VALUE_SET:
			MATRIX_CELL cell;
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
            Nomad_SetNotchFilter(band, ch,  freq,  Q,  dBgain, msg->Value.Signed);
			cell.Col=0; cell.Row=ConvertNovaChanToPC(ch), cell.Value=msg->Value.Signed;
			if(context != gPCcontext)
				SendSetting_2D_indexed_CR(SETTING_INPUT_NOTCH_ENABLE, &cell, 22,2); 
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSetupChanNotch2OnOff)  
{
	int OnOff, ch, trim, delta, band=1; // note notch2 resides in band = [1]
	int freq; float Q, dBgain;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG)  { msg->Value.Signed = 0;  return MSG_DONE; }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
			msg->Value.Signed = OnOff;
    	    return MSG_DONE;
        case MSG_VALUE_SET:
			MATRIX_CELL cell;
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
            Nomad_SetNotchFilter(band, ch,  freq,  Q,  dBgain, msg->Value.Signed);
			cell.Col=1; cell.Row=ConvertNovaChanToPC(ch), cell.Value=msg->Value.Signed;
			if(context != gPCcontext)
				SendSetting_2D_indexed_CR(SETTING_INPUT_NOTCH_ENABLE, &cell, 22,2); 
            return MSG_DONE;
        default: break;
    }
    return 0;
}

// modify a value that must be kindof logarithmic steps  
// allows 2 thresolds and 2 multipliers
int EncoderReScale(int oldval, int newval,  int T1, int M1,  int T2, int M2)
{
	volatile int inc=0, tmp;

	if     (newval == oldval+1)  inc =  1; 
	else if(newval == oldval-1)  inc = -1;
	else if(newval == oldval+10) inc =  10; // holding knob down and rotating
	else if(newval == oldval-10) inc = -10;
	else    return newval; // numeric entry mode - return value un-scathed
		   
	if     (oldval>=T2) inc *= M2; //  scale more
	else if(oldval>=T1) inc *= M1; //  scale a bit
	
	oldval = oldval - (oldval%inc); // force round numbers all the time - did not work

	tmp = oldval + inc;
   	return  tmp; 
}

extern int16 gEncoderCount;
EXPORT void ForceEncoderTicks(int delta)
{
	gEncoderCount+=delta;
}


MsgHandler(OnSettingSetupChanNotch1Freq) // 500=2,20  1000 = 5,50
{
	int OnOff, ch, trim, delta, band=0; // note notch1 resides in band = [0]
	int freq; float Q, dBgain;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG) {  msg->Value.Signed = 0;  return MSG_DONE;  }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
    		msg->Value.Signed = freq;
    	    return MSG_DONE;
        case MSG_VALUE_SET:
			MATRIX_CELL cell;
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
			freq = EncoderReScale(round(freq), msg->Value.Signed,  500, 2,  1000, 5);
            Nomad_SetNotchFilter(band, ch, freq, Q, dBgain, OnOff);
			cell.Col=0; cell.Row=ConvertNovaChanToPC(ch), cell.Value=(uchar)freq;
			if(context != gPCcontext)
				SendSetting_2D_indexed_CR(SETTING_INPUT_NOTCH_FREQ, &cell, 22,2); 
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSetupChanNotch2Freq)  
{
	int OnOff, ch, trim, delta, band=1; // note notch1 resides in band = [1]
	int freq; float Q, dBgain;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
	//if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG) {  msg->Value.Signed = 0;  return MSG_DONE;  }
	ch = Nomad.Setup.Chan;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
			msg->Value.Signed = freq;
    	    return MSG_DONE;
        case MSG_VALUE_SET:
			MATRIX_CELL cell;
    		OnOff = Nomad_GetNotchFilter(band, ch, &freq, &Q, &dBgain);
			freq = EncoderReScale(round(freq), msg->Value.Signed,  500, 2,  1000, 5);
            Nomad_SetNotchFilter(band, ch, freq, Q, dBgain, OnOff);
			cell.Col=1; cell.Row=ch, cell.Value=(uchar)freq;
			if(context != gPCcontext)
				SendSetting_2D_indexed_CR(SETTING_INPUT_NOTCH_FREQ, &cell, 18,2); 
            return MSG_DONE;
        default: break;
    }
    return 0;
}



MsgHandler(OnSettingTCopMode)  // { 'NORMAL', 'AUTOLOAD' },
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	if(gNV.OpMode == OPMODE_AUTOLOAD) msg->Value.Unsigned = 1;
        	else 					          msg->Value.Unsigned = 0;
        	return MSG_DONE;
        case MSG_VALUE_SET:  
        	if(msg->Value.Unsigned) gNV.OpMode = OPMODE_AUTOLOAD;  
        	else 					gNV.OpMode = OPMODE_NORMAL;  
           	NVIsDirty(DEVAINI); 
           	return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingRemoteRoll)   // { 'OFF', 'POSITIVE', 'NEGATIVE' },		void GPi1_Handler(void); //gNV.GPi1Mode   // off, rising edge, falling edge
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.GPi1Mode;	return MSG_DONE;
        case MSG_VALUE_SET:  
/*  only a nomad limiation
			if( AnExternalMixerIsConnected() ) // gNV.fMix12Enabled) // GPI will conflict with MIX8
				{
				PopUp_Generic("Turn off Mixer", "first.", "", "", 4, 7000);
				gNV.GPi1Mode=0;
            	return MSG_DONE;
				}
*/
        	gNV.GPi1Mode = msg->Value.Unsigned;  	
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSlateExternal)// { 'INT', 'EXT' },	   Slate source normal/camera
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.SlateSource;	return MSG_DONE;
        case MSG_VALUE_SET:  gNV.SlateSource = msg->Value.Unsigned;	NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingWriteSoundReport)
{                       
    switch (MsgType(msg)) {
        case MSG_ACTION: {
			int result = GenerateSoundReportFileRequest(gNV.MirrorDrive); // we can write sound report while mirroring here
			if(result < 0)
				PopUp("Mirror card not\nready");
       		return MSG_DONE;
			}
        default: break;
    }
    return 0;
}


MsgHandler(OnSetting_FACT_VCO_TUNE) 
{                       
    switch (MsgType(msg)) {	 // this gets updated by VCODAChandler() in N_HardwareOpt.cpp
        case MSG_VALUE_GET:  msg->Value.Signed    = gMachineInfo.VCOtune; return MSG_DONE;
        case MSG_VALUE_SET:  gMachineInfo.VCOtune = msg->Value.Signed;    return MSG_DONE;
        default: break;
    }
    return 0;
}

int SetRTCtune_factory(int chip, char val);
MsgHandler(OnSetting_FACT_RTC0_TUNE) // this gets updated by RTCtuneHandler() in RealTimeClock.cpp
{                       
    switch (MsgType(msg)) {	  
        case MSG_VALUE_GET:  msg->Value.Signed = gMachineInfo.RTC0tune; return MSG_DONE;
        case MSG_VALUE_SET:  
        	gMachineInfo.RTC0tune = msg->Value.Signed; 
	        SetRTCtune_factory(0, gMachineInfo.RTC0tune);
			return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_FACT_RTC1_TUNE) // this gets updated by RTCtuneHandler() in RealTimeClock.cpp
{                       
    switch (MsgType(msg)) {	  
        case MSG_VALUE_GET:  msg->Value.Signed = gMachineInfo.RTC1tune; return MSG_DONE;
        case MSG_VALUE_SET:  
        	gMachineInfo.RTC1tune = msg->Value.Signed; 
	        SetRTCtune_factory(1, gMachineInfo.RTC1tune);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

			   
MsgHandler(OnSetting_LCD_Brightness) 
{     
	int val;                  
    switch (MsgType(msg)) {	  // this gets updated by VCODAChandler() in N_HardwareOpt.cpp
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.Brightness;    return MSG_DONE;
        case MSG_VALUE_SET:  
        	gNV.Brightness    = msg->Value.Signed;    
            NVIsDirty(DEVAINI);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_LED_Brightness) // from 0 to 30 - controls front panel LEDs 
{    
    switch (MsgType(msg)) {	  // this gets updated by VCODAChandler() in N_HardwareOpt.cpp
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.LEDtransportBrightness;    return MSG_DONE;
        case MSG_VALUE_SET:  
			msg->Value.Signed = clip(msg->Value.Signed, 0, 11);
        	gNV.LEDtransportBrightness = gNV.LEDtrimBrightness = msg->Value.Signed;    
			if(context != gPCcontext)
				{
				int val = msg->Value.Signed;// * (255/30);
				//SendSetting_Byte(SETTING_CONTROLLER_BACKLIGHT, msg->Value.Unsigned);
				SendBrightnessCommand(msg->Value.Signed); // gOasisBacklight, val, val, val, gPWMmax);
				}
            NVIsDirty(DEVAINI);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}


static void UI_UpdateLimiters_NV() // dont call this anywhere else cuz it triggers NVdata!
{
	UpdateCompressorSettings_IN(  &gCompSet_IN);
	UpdateCompressorSettings_OUT( &gCompSet_OUT);
	UpdateCompressorSettings_DISK(&gCompSet_DISK);
    NVIsDirty(AICINI);
    NVIsDirty(ODBCINI);
    return;	
}

//name = 'setup.chan.CompOn', menu ={ 'OFF', 'ON', 'GroupA','GroupB','GroupC'}, 
MsgHandler(OnSetting_InputLimiter_enable)
{
    int ch=Nomad.Setup.Chan;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG){  msg->Value.Unsigned=0; return MSG_DONE; }

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	  
			if(gChannelConfig[ch].fCompressor==0) 
        		msg->Value.Unsigned = 0; // off
        	else 
        		msg->Value.Unsigned = gChannelConfig[ch].LinkGroup + 1; 
        	return MSG_DONE;
        case MSG_VALUE_SET:   
        	gChannelConfig[ch].fCompressor = logic(msg->Value.Unsigned); // withhout logic() this will set it to OFF when setting it to 2!  
			if(msg->Value.Unsigned >= 1)
				{
				if(ch >= (MAX_PROXY_INPUTS+MAX_RETURN_INPUTS)) // apparently we cannot group digtial inputs (see Effects.cpp::CompPeakDetect_Grouped())
					msg->Value.Unsigned = 1; // limit to "ON"
				gChannelConfig[ch].LinkGroup = msg->Value.Unsigned - 1; // 	0,1,2,3 for Notlinked,ON,GroupA,GroupB,GroupC
				}		
			//gChannelConfig[ch].LinkGroup = clip(gChannelConfig[ch].LinkGroup, 0, MAX_LINKGROUP);
			if(gChannelConfig[ch].LinkGroup > MAX_LINKGROUP) 
				gChannelConfig[ch].LinkGroup = MAX_LINKGROUP; // just in case 
			
			if(context != gPCcontext)
				{
				//SendSetting_Byte(SETTING_INPUT_COMPRESS_ENABLE, gChannelConfig[ch].fCompressor); // send BOOL value to PC
				//SendSetting_Byte(SETTING_INPUT_COMPRESS_LINK,   gChannelConfig[ch].LinkGroup);   // send enum value to PC none,a,b,c
				SendSetting_1D_indexed(SETTING_INPUT_COMPRESS_ENABLE, ConvertNovaChanToPC(ch), gChannelConfig[ch].fCompressor, 18); 
				SendSetting_1D_indexed(SETTING_INPUT_COMPRESS_LINK,   ConvertNovaChanToPC(ch), gChannelConfig[ch].LinkGroup,   18); 
				}
        	NVIsDirty(AICINI);  
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
/*
MsgHandler(OnSetting_InputLimiter_link)
{
    int ch=Nomad.Setup.Chan;
    assert(Nomad.Setup.Chan < numberof(Nomad.Setup.Channel));
    //if(Nomad.Setup.Chan>=MAX_IN_CHAN_ANALOG){  msg->Value.Unsigned=0; return MSG_DONE; }

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	
        	msg->Value.Unsigned = gChannelConfig[ch].LinkGroup; // +1; 
        	return MSG_DONE;
        case MSG_VALUE_SET:   
			//	gChannelConfig[ch].LinkGroup = msg->Value.Unsigned; //  - 1; // 	0,1,2,3 for Notlinked,ON,GroupA,GroupB,GroupC
			//if(gChannelConfig[ch].LinkGroup > MAX_LINKGROUP) 
			gChannelConfig[ch].LinkGroup = clip(msg->Value.Unsigned, 0, MAX_LINKGROUP; // just in case 
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_INPUT_COMPRESS_LINK,   gChannelConfig[ch].LinkGroup);   // send enum value to PC none,a,b,c
        	NVIsDirty(AICINI);  
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
*/
MsgHandler(OnSetting_InputLimiter_attack)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Signed = round(1000.0*gCompSet_IN.attack);  return MSG_DONE;
        case MSG_VALUE_SET: 
			if(msg->Value.Signed<=1) msg->Value.Signed = 1;
			gCompSet_IN.attack = 0.001 * (float)msg->Value.Signed; 
			UI_UpdateLimiters_NV();	
			if(context != gPCcontext)
				{
				uchar ary[3]; ary[0] = msg->Value.Unsigned;
				SendSetting_Array1D(SETTING_INPUT_COMPRESS_ATTACK, ary, 3, 1); // just send frist val (array represents three different compressor types)
				}
			return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_InputLimiter_decay) 
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(1000.0*gCompSet_IN.decay);  return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed<=5) msg->Value.Signed = 5;
        	gCompSet_IN.decay = 0.001 * (float)msg->Value.Signed; 
        	UI_UpdateLimiters_NV();
			if(context != gPCcontext)
				{
				uchar ary[3]; ary[0] = msg->Value.Unsigned;
				SendSetting_Array1D(SETTING_INPUT_COMPRESS_DECAY, ary, 3, 1); // just send frist val (array represents three different compressor types)
				}
			return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_InputLimiter_thresh)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_IN.threshold);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_IN.threshold = msg->Value.Signed; 
        	UI_UpdateLimiters_NV();	
			if(context != gPCcontext)
				{
				uchar ary[3]; ary[0] = msg->Value.Signed;  // always nevative
				SendSetting_Array1D(SETTING_INPUT_COMPRESS_THRESH, ary, 3, 1); // just send frist val (array represents three different compressor types)
				}
			return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_InputLimiter_ratio) 
{                       
    //float base = 1.0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round((gCompSet_IN.ratio)*10.0);  return MSG_DONE;
        case MSG_VALUE_SET:  
        	gCompSet_IN.ratio = 0.1*(float)msg->Value.Signed; 
        	UI_UpdateLimiters_NV(); 
			if(context != gPCcontext)
				{
				uchar ary[3]; ary[0] = msg->Value.Unsigned + 1;
				SendSetting_Array1D(SETTING_INPUT_COMPRESS_RATIO, ary, 3, 1); // just send frist val (array represents three different compressor types)
				}
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_InputLimiter_gain)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_IN.dbgain);  return MSG_DONE;
        case MSG_VALUE_SET:  
        	gCompSet_IN.dbgain = msg->Value.Signed; 
        	UI_UpdateLimiters_NV();	
			if(context != gPCcontext)
				{
				uchar ary[3]; ary[0] = msg->Value.Signed;
				SendSetting_Array1D(SETTING_INPUT_COMPRESS_GAIN, ary, 3, 1); // just send frist val (array represents three different compressor types)
				}
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////  gOutputChanConfig
MsgHandler(OnSetting_MatrixOutputLimiter_enable) 
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    unsigned ch;
	//    assert(ch < numberof(gOutputChanConfig)); this might not be a matrix msg! this is prolly done elsewhere too!!!  FIXME find them!!!!

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET:	  
        	ch = MatrixToLinear(8, cell);  
        	cell->Value = gOutputChanConfig[ch].fCompressor;  return MSG_DONE;
        case MSG_MATRIX_CELL_SET:   
			ch = MatrixToLinear(8, cell);
		    assert(ch < numberof(gOutputChanConfig)); 
		    if(ch >= numberof(gOutputChanConfig)){  cell->Value=0; return MSG_DONE; }
			if(context != gPCcontext)
				SendSetting_2D_indexed_CR(SETTING_OUTPUTMIX_COMP, cell, 1,MAX_OUTPUTS); 
        	gOutputChanConfig[ch].fCompressor = cell->Value;  NVIsDirty(ODBCINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_OutputLimiter_attack)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Signed = round(1000.0*gCompSet_OUT.attack);  return MSG_DONE;
        case MSG_VALUE_SET: 
			if(msg->Value.Signed<=1) msg->Value.Signed = 1;
			gCompSet_OUT.attack = 0.001 * (float)msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutputLimiter_decay) 
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(1000.0*gCompSet_OUT.decay);  return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed<=5) msg->Value.Signed = 5;
        	gCompSet_OUT.decay = 0.001 * (float)msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutputLimiter_thresh)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_OUT.threshold);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_OUT.threshold = msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutputLimiter_ratio) 
{                       
    //float base = 1.0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round((gCompSet_OUT.ratio)*10.0);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_OUT.ratio = 0.1*(float)msg->Value.Signed; UI_UpdateLimiters_NV(); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutputLimiter_gain)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_OUT.dbgain);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_OUT.dbgain = msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////  gDiskChanConfig
MsgHandler(OnSetting_MatrixCardLimiter_enable) 
{
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    unsigned ch;

    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET:  
        	ch = MatrixToLinear(8, cell);
        	cell->Value = gDiskChanConfig[ch].fCompressor;  return MSG_DONE;
        case MSG_MATRIX_CELL_SET:  
		    ch = MatrixToLinear(8, cell);
		    assert(ch < numberof(gDiskChanConfig));
		    if(ch >= numberof(gDiskChanConfig)){  cell->Value=0; return MSG_DONE; }
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TRACKMIX_COMP, cell->Value); // send enum value to PC
        	gDiskChanConfig[ch].fCompressor = cell->Value;  NVIsDirty(ODBCINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_CardLimiter_attack)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Signed = round(1000.0*gCompSet_DISK.attack);  return MSG_DONE;
        case MSG_VALUE_SET: 
			if(msg->Value.Signed<=0) msg->Value.Signed = 1;
			gCompSet_DISK.attack = 0.001 * (float)msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardLimiter_decay) 
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(1000.0*gCompSet_DISK.decay);  return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed<=0) msg->Value.Signed = 5;
        	gCompSet_DISK.decay = 0.001 * (float)msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardLimiter_thresh)
{                       
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_DISK.threshold);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_DISK.threshold = msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardLimiter_ratio) 
{                       
    //float base = 1.0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round((gCompSet_DISK.ratio)*10.0);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_DISK.ratio = 0.1*(float)msg->Value.Signed; UI_UpdateLimiters_NV(); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_CardLimiter_gain)  
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = round(gCompSet_DISK.dbgain);  return MSG_DONE;
        case MSG_VALUE_SET:  gCompSet_DISK.dbgain = msg->Value.Signed; UI_UpdateLimiters_NV();	return MSG_DONE;
        default: break;
    }
    return 0;
}



#ifdef NOMAD
MsgHandler(OnSetting_IntBattWarnOnOff) // NOT USED ON NOVA!
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.LowBatteryWarnInt; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.LowBatteryWarnInt = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             
MsgHandler(OnSetting_IntBattWarn) //int threshold    NOT USED ON NOVA!
{
	int min=4.9*10 , max=9*10;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	msg->Value.Signed = round(gNV.LowBatteryThreshInt*10.0);  
        	return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed < min) msg->Value.Signed = min;
			if(msg->Value.Signed > max) msg->Value.Signed = max;
        	gNV.LowBatteryThreshInt = msg->Value.Signed/10.0;  
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}						             
#endif

MsgHandler(OnSetting_ExtBattWarnOnOff) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.LowBatteryWarnExt; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.LowBatteryWarnExt = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             
MsgHandler(OnSetting_ExtBattWarn)//ext threshold
{
   int min=8*10 , max=16*10;
   switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	msg->Value.Signed = round(gNV.LowBatteryThreshExt*10.0);  
        	return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed < min) msg->Value.Signed = min;
			if(msg->Value.Signed > max) msg->Value.Signed = max;
        	gNV.LowBatteryThreshExt = msg->Value.Signed/10.0;  
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}						             


MsgHandler(OnPage_UIPageBattWarnEXT) // pop up page
{
//    UI_MSGBOX *p = &UI_MsgBox;
    switch (MsgType(msg)) {
//        case MSG_PAGE_ENTER:
//            /* Dim the contents of existing screen contents on entry to page.
//             * Some of this content will remain on screen when the message
//             * box is active */
//            CanvasMistify(UICanvas());
//            return MSG_DONE;
        case MSG_PAGE_DRAW:
            {
                /* Note that the page is not automatically erased 
                 * (UI_PAGE_FLAG_NO_ERASE is set via ui.lua) */
                 
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2; // 40;
                int y = 55; // 100;
                
                const IMAGE_RGB *img = &ImgWarningTriangle1;
//                const int lineHeight = CanvasFontHeight(c);
//                const int textHeight = lineHeight * p->NLines;
//                const int ymargin = 10;
//                const int hbox = max(textHeight, lineHeight*2) + ymargin*2;
//                const int wbox = ImageWidth(img);
//                const int xtop = (LCD_WIDTH-ImageWidth(img))/2;
//                const int ytop = (LCD_HEIGHT- (hbox + ImageHeight(img)))/2;
//                int x = xtop;
//                int y = ytop;

//                /* Draw title bar image */
                CanvasImageRGB(c, 2, 22, img);

                /* Draw title text */
               // CanvasStringEx(c, x+2, y+3, ALIGN_CENTER, FONT_LARGE, COLOR_OFF, "LOW EXTERNAL BATTERY"); // p->Title);
                y = 25 + ImageHeight(img);
 
                y = CanvasStringCenter(c, x, y, "LOW EXTERNAL"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "BATTERY VOLTAGE"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "Press BACK to continue"); 
               
//                CanvasFillRect(c, x, y, x+wbox-1, y+hbox-1, COLOR_MSGBOX_BACKGROUND);
//                CanvasBox(c, x, y, x+wbox-1, y+hbox-1, COLOR_ON);

                /* Draw text lines */

//                x += 6;
//                y += (hbox - textHeight)/2;
//                for (int i = 0; i < p->NLines; i++) {
//                    CanvasString(c, x, y, p->Lines[i]);
//                    y += lineHeight;
//                }
            }
            return 0; // MSG_DONE;
//        case MSG_KEY_PRESS:
//            /* Report keycode of key event, then exit the message box */
//            p->KeyCode = MsgKey(msg);
//            UIPageExit();
//            return MSG_DONE;
//        case MSG_KEY_RELEASE:
//            return MSG_DONE;
//        case MSG_TIMER_10MS:
//            /* On timeout, exit with keycode KEY_NONE */
//            if (TimeoutHasExpired(&p->Timeout)) {
//                p->KeyCode = KEY_NONE;
//                UIPageExit();
//            }
//            return MSG_DONE;
        default:
            break;
    }
    return 0;
}

MsgHandler(OnPage_UIPageBattWarnINT) // pop up page
{
    switch (MsgType(msg)) {
        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                const IMAGE_RGB *img = &ImgWarningTriangle1;
                int x = LCD_WIDTH/2; // 40;
                int y = 45; // 100;
                
                CanvasImageRGB(c, 2, 22, img);
                y = 25 + ImageHeight(img);
 
                y = CanvasStringCenter(c, x, y, "LOW INTERNAL"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "BATTERY VOLTAGE"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "Press BACK to continue"); 
            }
            return 0; // MSG_DONE;
        default:
            break;
    }
    return 0;
}


MsgHandler(OnPage_UIPageTLODwarn) // pop up page
{
    switch (MsgType(msg)) {
        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                const IMAGE_RGB *img = &ImgWarningTriangle1;
                int x = LCD_WIDTH/2; // 40;
                int y = 45; // 100;
                
                CanvasImageRGB(c, 2, 22, img);
                y = 25 + ImageHeight(img);
 
                y = CanvasStringCenter(c, x, y, "LESS THAN 5 MINUTES"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "LEFT ON CARD"); 
				y += 6;
                y = CanvasStringCenter(c, x, y, "Press BACK to continue"); 
            }
            return 0; // MSG_DONE;
        default:
            break;
    }
    return 0;
}

MsgHandler(OnPage_UIPageFalseStart)   // pop up	  false start
{
    static int state=0, seg = GetNumSegments();
    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            state=0;  
            return 0;  // note gNV.fNoFalseStartDelete exists but is not used or set anywhere
        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
				char str[40];
                int x = LCD_WIDTH/2, y = 25;
 				if(state==0)	
					{
					seg = GetNumSegments();
					sprintf(str, "(Segment %d)", GetNumSegments());
	                y = CanvasStringCenter(c, x, y, str); 	y += 10;
	                y = CanvasStringCenter(c, x, y, "HOLD Menu to MARK"); 	y += 10;
    	            y = CanvasStringCenter(c, x, y, "PRESS SLT to DELETE"); y += 10;
        	        y = CanvasStringCenter(c, x, y, "Press BACK to Cancel"); 
					y += 10;
					sprintf(str, "Scene: %s",dir[seg].Scene); y = CanvasString(c, 5, y, str);
					sprintf(str, "Take: %s",dir[seg].Take);  y = CanvasString(c, 5, y, str);
					sprintf(str, "Note: %s",dir[seg].Note);  y = CanvasString(c, 5, y, str);
					}
 				else if(state==1)	
					{
					sprintf(str, "Seg %d has been MARKED", seg); y = CanvasStringCenter(c, x, y, str);
					y += 10;
        	        y = CanvasStringCenter(c, x, y, "Press BACK"); 
					y += 12;
					sprintf(str, "Scene: %s",dir[seg].Scene); y = CanvasString(c, 5, y, str);
					sprintf(str, "Take: %s",dir[seg].Take);  y = CanvasString(c, 5, y, str);
					sprintf(str, "Note: %s",dir[seg].Note);  y = CanvasString(c, 5, y, str);
					}
				else // if(state==2)	
					{
					y+=20;
					sprintf(str, "Seg %d has been DELETED", seg);
	                y = CanvasStringCenter(c, x, y, str); 	y += 12;
        	        y = CanvasStringCenter(c, x, y, "Press BACK"); 
					}	   

            }
            return 0; // MSG_DONE;
        case MSG_KEY_RELEASE: // regular SLT key (was menu)
            if( (MsgKey(msg) == KEY_SLATE) && (state==0) )
				{
				//int mode, mdrive = gNV.MirrorDrive;
				int fnum   = GetCurrentFolder();
                //if(SegIsBeingMirrored(fnum, seg))
				//	{
				//	mode = GUI_GetMirrorMode(mdrive);
				//	SetMirrorMode(mdrive, MIR_MODE_OFF);
				//	sleep(7000); // wait for abort to take place
				//	SetMirrorMode(mdrive, mode);
				//	}

                seg = UI_DeleteLastSeg(); // search for false start to see how mirror handles this condition
//                sleep(500);
//                SegWasDeleted(fnum, seg); // called by False Start to tell mirror to go back one seg if it has already finished mirroring this seg
				state = 2;
				}
            return MSG_DONE;

        case MSG_KEY_LONG: // LONG menu key
            if( (MsgKey(msg) == KEY_MENU) && (state==0) )
				{
				seg = UI_MarkCurrentSegment();
				state = 1;
				}
            return MSG_DONE;

		}
    return 0;
}

MsgHandler(OnPage_UIPageMirrorConflict) // pop up
{
    int x = LCD_WIDTH/2, y = 45;
    switch (MsgType(msg)) {
        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
//              const IMAGE_RGB *img = &ImgWarningTriangle1;
//              CanvasImageRGB(c, 0, 20, img);
//              y = 25 + ImageHeight(img);
                y = CanvasStringCenter(c, x, y, "Please turn mirroring"); y += 6;
                y = CanvasStringCenter(c, x, y, "OFF before performing"); y += 6;
                y = CanvasStringCenter(c, x, y, "this operation.");       y += 6;
            }
        default:
            break;
    }
    return 0;
}

MsgHandler(OnPage_UIPageRecordConflict)  // pop up
{
    int x = LCD_WIDTH/2, y = 45;
    switch (MsgType(msg)) {
        case MSG_PAGE_DRAW:
            {
//          CANVAS *c = UICanvas();
            y = CanvasStringCenter(UICanvas(), x, y, "Connot perform this"); 		y += 6;
            y = CanvasStringCenter(UICanvas(), x, y, "operation while recording."); y += 6;
            }
        default:
            break;
    }
    return 0;
}


int ReadINIfileFromPresetROM(int PresetNum, int FileNumber, char *buf, int maxbytes); // read one INI file from preset struct in ROM
int WriteRomPreset_ASYNC(int PresetNum); // , char *name);
//int WriteRomPreset(int PresetNum, char *name);  // write all INI files to a preset struct in ROM
int ImportINIfiles(uint PresetNum, int DiskNum);

//int gExportError = -1;
int gExportState = 0;
short Get_ASYNC_ExportError(); // {return gExportError;}
void  Set_ASYNC_ExportError(short val); // {gExportError=val;}

void UI_DrawStorePage(int PresetNum)
{
    CANVAS *c = UICanvas();
    int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
    char tmpstr[40];
                
	if(gExportState==0)
    	{
        y=CanvasStringCenter(c, x, y, "Press * to Store"); 
		sprintf(tmpstr, "Preset #%d", PresetNum);  y=CanvasStringCenter(c, x, y, tmpstr); 
        y+=ystep;
        y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
        }
	else if(Get_ASYNC_ExportError() < 0) // (gExportError<0)
		{
        y=CanvasStringCenter(c, x, y, "Please wait..."); 
        }
	else   
        if( Get_ASYNC_ExportError() == 0 ) //  gExportError==0)
        	{
	        y=CanvasStringCenter(c, x, y, "Settings have been"); 
			sprintf(tmpstr, "stored to Preset #%d", PresetNum);  y=CanvasStringCenter(c, x, y, tmpstr); 
        	}                         
		else
			{
			sprintf(tmpstr, "Error in Store (%d)", Get_ASYNC_ExportError() );  y=CanvasStringCenter(c, x, y, tmpstr); 
			}
	return;
}

void UI_ExportStarkKeyHandler(int PresetNum)
{                          
    char namestr[40];
	gExportState++;     // this may re-enter itself so be careful
	if(gExportState==1)
		{
		gExportState++;  // this must happen first (re-enterant)
		//sprintf(namestr, "This is preset %d",PresetNum);
		//gExportError = WriteRomPreset(PresetNum-1, namestr);
		WriteRomPreset_ASYNC(PresetNum-1); // , namestr);
		gExportState++;
		}
}

MsgHandler(OnSettingStoreNV1 )
{
    int PresetNum=1;

    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gExportState = 0;
			Set_ASYNC_ExportError(-1); // gExportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawStorePage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if( (MsgKey(msg) == KEY_STAR) )
                {                          
				UI_ExportStarkKeyHandler(PresetNum);
				return MSG_DONE;
				}
            break;   
        }
    return 0;
}
MsgHandler(OnSettingStoreNV2 )
{
    int PresetNum=2;

    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gExportState = 0;
			Set_ASYNC_ExportError(-1); // gExportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawStorePage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if( (MsgKey(msg) == KEY_STAR) )
                {                          
				UI_ExportStarkKeyHandler(PresetNum);
				return MSG_DONE;
				}
            break;   
        }
    return 0;
}
MsgHandler(OnSettingStoreNV3 )
{
    int PresetNum=3;

    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gExportState = 0;
			Set_ASYNC_ExportError(-1); // gExportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawStorePage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if( (MsgKey(msg) == KEY_STAR) )
                {                          
				UI_ExportStarkKeyHandler(PresetNum);
				return MSG_DONE;
				}
            break;   
        }
    return 0;
}

//int gImportError = -1;
int gImportState = 0;

short Get_ASYNC_ImportError(); // {return gImporttError;}
void  Set_ASYNC_ImportError(short val); // {gImporttError=val;}
void ReadRomPreset_ASYNC(int PresetNum);

int UI_DrawRecallPage(int PresetNum)
{
    CANVAS *c = UICanvas();
    int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
    char tmpstr[40];
                
	if(gImportState==0)
    	{
        y=CanvasStringCenter(c, x, y, "Press * to recall"); 
		sprintf(tmpstr, "Preset #%d", PresetNum);  y=CanvasStringCenter(c, x, y, tmpstr); 
        y+=ystep;
        y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
        }
	else if( Get_ASYNC_ImportError() < 0)  // gImportError < 0) // import ahs not finished yet...
		{
        y=CanvasStringCenter(c, x, y, "Please wait..."); 
        }
	else  //if(state==5) // import is finished...
        {
		if(Get_ASYNC_ImportError() == 0 ) // gImportError==0)
			{
	        y=CanvasStringCenter(c, x, y, "Settings have been"); 
			sprintf(tmpstr, "recalled from Preset #%d", PresetNum);  y=CanvasStringCenter(c, x, y, tmpstr); 
    	    y+=ystep;
        	y=CanvasStringCenter(c, x, y, "*** RESTART NOW ***"); 
	        }                         
		else
			{
			sprintf(tmpstr, "Error In Recall (%d)", Get_ASYNC_ImportError() );  y=CanvasStringCenter(c, x, y, tmpstr); 
			}
		}
	return 0;
}



void UI_ImportStarkKeyHandler(int PresetNum)
{                          
	gImportState++;     // this may re-enter itself so be careful
	if(gImportState==1)
		{
		gImportState++;  // this must happen first (re-enterant)
        //gImportError = ImportINIfiles(PresetNum-1, 0);
		ReadRomPreset_ASYNC(PresetNum); 
		gImportState++;
		}
}

MsgHandler(OnSettingRecallNV1)
{
    int PresetNum=1;
    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gImportState = 0;
			Set_ASYNC_ImportError(-1); // gImportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawRecallPage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if(MsgKey(msg) == KEY_STAR)
				{
				UI_ImportStarkKeyHandler(PresetNum);
                return MSG_DONE;
				}
            break;   
        }
    return 0;
}

MsgHandler(OnSettingRecallNV2)
{
    int PresetNum=2;
    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gImportState = 0;
			Set_ASYNC_ImportError(-1); // gImportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawRecallPage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if(MsgKey(msg) == KEY_STAR)
				{
				UI_ImportStarkKeyHandler(PresetNum);
                return MSG_DONE;
				}
            break;   
        }
    return 0;
}

MsgHandler(OnSettingRecallNV3) 
{
    int PresetNum=3;
    switch (MsgType(msg)) {
        case MSG_PAGE_ENTER:
            gImportState = 0;
			Set_ASYNC_ImportError(-1); // gImportError = -1;
            break;
        case MSG_PAGE_DRAW:
            UI_DrawRecallPage(PresetNum);
			return 0; // MSG_DONE;
            //break;
        case MSG_KEY_PRESS:
            if(MsgKey(msg) == KEY_STAR)
				{
				UI_ImportStarkKeyHandler(PresetNum);
                return MSG_DONE;
				}
            break;   
        }
    return 0;
}

extern int gSTNchanged; // writes a dir entry to disks (for STN)
void ToggleSTN_MP3(int seg) // 4.57  called in metadata page via STAR KEY (uimetadata.cpp)
{
	dir[seg].MP3noMirror = !dir[seg].MP3noMirror;
	UI_SetCurrentMetaDataSeg(seg); // properly refresh MP3 state display (MP3 will not turn on if MP3 file does not exist)
	dir[seg].fChanged=99;
	gSTNchanged++;
	return;
}

// this gets called on PAGE_ENTER also
void UI_SetCurrentMetaDataSeg(int x)
{
	if(x < 0) x = folder[DMcurFolder].TotalSegs+1; // default to NEXT seg on screen ENTER     folder[DMcurFolder].CurPlaySeg;
	if(x < 1)									x = 1;
	if(x > (folder[DMcurFolder].TotalSegs+1))	x = (folder[DMcurFolder].TotalSegs+1);
	Nomad.Metadata.CurSeg = x;
	strcpy(Nomad.Metadata.Scene, dir[x].Scene); // display current string
	strcpy(Nomad.Metadata.Take,  dir[x].Take);  // display current string
	strcpy(Nomad.Metadata.Note,  dir[x].Note);  // display current string

	Nomad.Metadata.MP3on = 0;
#if defined(ALLOW_MP3) && defined(NOMAD)
	if( (gMP3dir[dir[x].MP3index].FileID != dir[x].FileID) || (!gMP3dir[dir[x].MP3index].Used) || dir[x].MP3noMirror )
		Nomad.Metadata.MP3on = 0;
	else
		Nomad.Metadata.MP3on = 1;
#endif

	return;
}

MsgHandler(OnSettingMetaDataCurSeg)
{
    int seg;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	
        	msg->Value.Signed = Nomad.Metadata.CurSeg; 
        	return MSG_DONE;
        case MSG_VALUE_SET:
			seg = msg->Value.Signed;
			if(seg == 0) seg = -1; // wrap to NEXT seg
			if(seg > (folder[DMcurFolder].TotalSegs+1) ) seg = 1; // wrap to 1st seg
			UI_SetCurrentMetaDataSeg(seg);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}						             
 
MsgHandler(OnSettingMetadataScene)
{
//    static char s[40];
    switch (MsgType(msg)) {
        case MSG_TEXT_GET:	
        	strcpy(MsgText(msg), dir[Nomad.Metadata.CurSeg].Scene); 
        	return MSG_DONE;
        case MSG_TEXT_SET:
			 SetScene(Nomad.Metadata.CurSeg,  MsgText(msg));
        	 return 0; //MSG_DONE;
        default: break;
    }
    return 0;
}						             


MsgHandler(OnSettingMetadataTake)
{
//    static char s[40];
    switch (MsgType(msg)) {
        case MSG_TEXT_GET:	
        	strcpy(MsgText(msg), dir[Nomad.Metadata.CurSeg].Take); 
        	return MSG_DONE;
        case MSG_TEXT_SET:
			SetTake(Nomad.Metadata.CurSeg, MsgText(msg));
        	return 0; //MSG_DONE;
        default: break;
    }
    return 0;
}						             


MsgHandler(OnSettingMetadataNote)
{
//    static char s[40];
    switch (MsgType(msg)) {
        case MSG_TEXT_GET:	
        	strcpy(MsgText(msg), dir[Nomad.Metadata.CurSeg].Note); 
        	return MSG_DONE;
        case MSG_TEXT_SET:
			SetNote(Nomad.Metadata.CurSeg, MsgText(msg));
        	return 0; //MSG_DONE;
        default: break;
    }
    return 0;
}						             

int UI_NameFolder(char *pName, uint fnum);
MsgHandler(OnSetting_NamePrimaryFolder)
{
     unsigned fnum = GetCurrentFolder(); 
//     static char s[40];
//     assert((fnum >= 1) && (fnum <= 99)); // numberof(Folder)) );
     if( (fnum==0) || (fnum>99) )  return 0;

     switch (MsgType(msg)) {
        case MSG_TEXT_MAXLEN_GET:	
        	msg->Value.Signed = MAX_FOLDER_NAME_SIZE; // 8 
        	return MSG_DONE;

        case MSG_TEXT_GET:	
        	strcpy(MsgText(msg), folder[fnum].Name); 
        	return MSG_DONE;

        case MSG_TEXT_SET:
        	UI_NameFolder(MsgText(msg), fnum);
			return 0; // MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSetting_HPbeepLoudness)
{
	ALARM alarm=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepGain; return MSG_DONE;
        case MSG_VALUE_SET:	
        	gNV.HPbeepGain = msg->Value.Signed; 
			if(CHECK_ALARM(alarm))
				{
				CauseHPbeeps(3,100); // beep periodically so they can hear what they are adjusting
				SET_ALARM(alarm,700);
				}			  	
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}			
MsgHandler(OnSetting_HPbeepBatteryOn) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepBatteryOn; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.HPbeepBatteryOn = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}			
MsgHandler(OnSetting_HPbeepDiskOn) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepDiskOn; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.HPbeepDiskOn = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}			
MsgHandler(OnSetting_HPbeepRECSTOP)	
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepRECSTOP; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.HPbeepRECSTOP = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}			
//MsgHandler(OnSetting_HPbeepUNU)   	
//{
//    switch (MsgType(msg)) {
//        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepUNU; return MSG_DONE;
//        case MSG_VALUE_SET:	gNV.HPbeepUNU = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
//        default: break;
//    }
//    return 0;
//}			
			             
MsgHandler(OnSetting_HPbeepGain)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.HPbeepGain; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.HPbeepGain = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             



MsgHandler(OnSetting_SlowDisplaySpeed)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed   = gNV.SlowDisplaySpeed; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.SlowDisplaySpeed = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}			

EXPORT void UI_UpdatePlaybackInfo(void)
{
   int seg = UI_GetCurrentPlaySeg();  // IN WIN32 this is getting called before dir[seg] is inited!
   int ss,mm, ssTotal,mmTotal;
   char *p;

   //seg = Nomad.PlayBack.GotoPlaySegRequest;
   Nomad.PlayBack.ActualCurrentPlaySeg = seg;
//   Nomad.PlayBack.ScrubRequest = 0;
   Nomad.PlayBack.Percent = UI_GetPlaybackPercent();
   Nomad.PlayBack.NTracks = dir[seg].ChannelCount;  
   p = TCDisk->GetTimeCodeStr();
   strcpy(Nomad.PlayBack.TCstr, p); // "%02d:%02d:%02d:%02d", TCval>>24, TCval>>16, TCval>>8, TCval&0xFF);
   strcpy(Nomad.PlayBack.Scene, dir[seg].Scene);
   strcpy(Nomad.PlayBack.Take,  dir[seg].Take);
   strcpy(Nomad.PlayBack.Note,  dir[seg].Note);

   UI_GetPlayTime(&Nomad.PlayBack.mm, &Nomad.PlayBack.ss, &Nomad.PlayBack.mmTotal, &Nomad.PlayBack.ssTotal); 
   return;     
}     

MsgHandler(OnSettingGotoPlaySegRequest)
{
    static int PrevSeg=0;
    int seg = UI_GetCurrentPlaySeg();
	if(seg != PrevSeg)
	   PrevSeg = Nomad.PlayBack.GotoPlaySegRequest = seg; // tick to next seg if a change was detected during playback

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:		   
			msg->Value.Unsigned = Nomad.PlayBack.ActualCurrentPlaySeg; // Nomad.PlayBack.GotoPlaySegRequest;
            return MSG_DONE;
        case MSG_VALUE_SET:		   
			if( msg->Value.Unsigned > GetNumSegments() )
				Nomad.PlayBack.GotoPlaySegRequest = GetNumSegments();
			else if(msg->Value.Unsigned<1)
				Nomad.PlayBack.GotoPlaySegRequest = 1;
			else 
				Nomad.PlayBack.GotoPlaySegRequest = msg->Value.Unsigned; // valid seg number req
			Nomad.PlayBack.PlaySegMarker = 0; // we dont care about markers here...
            return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingScrubRequest)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:		   
			msg->Value.Unsigned = Nomad.PlayBack.ScrubRequest;
            return MSG_DONE;
        case MSG_VALUE_SET:
			Nomad.PlayBack.ScrubRequest = msg->Value.Signed;
            return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSettingMirrorRemember)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gNV.RememberMirrorMode;  return MSG_DONE;
        case MSG_VALUE_SET: gNV.RememberMirrorMode = msg->Value.Signed;    NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}
 
 

int GetCurrentRecordSeg()
{
   if(GetTransportMode() == RECORD_MODE)  return folder[DMcurFolder].LastSeg+1;
   else                  				  return folder[DMcurFolder].LastSeg+0;
}

#ifdef ALLOW_AUDITION
int CountDropoutMarkers(int seg) // count markers in a segment
{
    int i;
    struct DIR *d = &dir[seg];

    for(i=0;i<(numberof(d->MarkedList));i++)
		{
		if(d->MarkedList[i]==0)
	  		break;
		}
	return i;
}
int CountDropoutMarkersInFolder() // count all the markers in the current folder
{
    int seg=1, MarkersInFolder=0;
    for(;;) // do for all segs in current folder
		{
		if(dir[seg++].Used==0) // no more segs?
		   break;
		MarkersInFolder += CountDropoutMarkers(seg);
		}
	return MarkersInFolder;

}

#define MARK_DROPOUT_REWIND_SECS 3

// this is 0 based
// if you put -1 in here it will give you spm of start of segment (the "zeroth marker")
unsigned long CalcMarkedSPM(int seg, int AbsIndex)
{
    struct DIR *d = &dir[seg];
    int64  spm, offset, SamplesPerDay;
	static int lastseg=0, lastindex=0;

    SamplesPerDay = (unsigned long)3600UL * 24UL * (unsigned long) d->SampleRate;

    if(AbsIndex<0) // -1 means "beginning of segment"
      offset = 0;
    else
      offset = d->MarkedList[AbsIndex]; // this is the current TC stamp in quarter second increments (implied Srate of "4")

    offset *= (d->SampleRate/2); //  half second resolution
    spm = offset + d->TimeCode; // add offset to start of segment

    if(spm >= SamplesPerDay) // this cannot happen!!
   	  spm -= SamplesPerDay; // wrap

	if( (lastseg!=seg) || (lastindex!=AbsIndex) )
    	dprintf("Calc Marker[%d] Seg:%d %ds\n", AbsIndex, seg, (int)(offset/(d->SampleRate/4)));
    lastseg=seg; lastindex=AbsIndex;
    return spm;
}
//extern int gRunningClusterCount;// keeps track of how many clusters were actually sent to the disk
extern int gSTNchanged;         // writes any modified dir5 entries to disk (when in STOP mode)

//max file size is 7.7 hours. half second clicks gives us 65536/2/3600 = 9.1 hours
//so just use an offset in half second clicks.....
int MarkDropOut()
{
	int i, seg = GetCurrentRecordSeg();
	volatile int64 spm, tcdisk, beg, off, SamplesPerDay;
	//float tmpf;
    struct DIR *d = &dir[seg];

	if(GetTransportMode() != RECORD_MODE)
	   return 0;

	i = CountDropoutMarkers(seg); // find first empty slot

	if(i >= numberof(d->MarkedList))
		return -1; // no more markers left  

	SamplesPerDay = (int64)(3600 * 24) * (int64)d->SampleRate;

    off = *TCDisk;
	off = off - d->TimeCode; 

	// why does this math not work??? - because we cannot step thru this stuff for some reason.....
	tcdisk = *TCDisk;
	beg    = d->TimeCode; 

	off = tcdisk - d->TimeCode; // this does not work
	off = (int64)tcdisk - (int64)beg;         // this does not work

    // spmOffset -= d->SampleRate*MARK_DROPOUT_REWIND_SECS; // rewind a few seconds to get back to the real dropout
	if(off >= SamplesPerDay) off -= SamplesPerDay; // wrap
    off /= (d->SampleRate/2);    // convert to half second units of time
	off -= MARK_DROPOUT_REWIND_SECS*2;
	if(off<=0) off=1; // dont go before the start of the seg
	if(off>0xFFFF) off = 0xFFFF; // max out at 16bits

	dprintf("Marker:%d Seg:%d %dsecs\n", i, seg, (int)off/2); 

	d->MarkedList[i] = off;  // mark it with current TC stamp (but not zero!)
	if(d->MarkedList[i]==0)
	   d->MarkedList[i] = 1; // force it to be non-zero (beacues zero means unused)
	d->fChanged=99; gSTNchanged++; // write this seg's entry to disk
	return 1;
}

// this allows us to pretend like there is one big list of markers in the folder
// it returns the seg number and absolute marker index of the marker implied by BigIndex
// segResult == 0 if it fails
void BigIndex_to_AbsIndex(int *segResult, int *AbsMarkerIndex, int BigIndex)
{
	int i, seg=1, count=0;
	*segResult=0;
	*AbsMarkerIndex=0;
	for(;;)
		{
		if(dir[seg].Used==0) // last seg in folder?
			return;	// marker not found!
	    for(i=0;i<(numberof(dir[seg].MarkedList));i++) // look thru all the markers in this seg
			{
			if(dir[seg].MarkedList[i]==0)
	  			break; // no more markers in this segment
			count++;
			if(count == BigIndex)
			   {
			   *segResult      = seg;
			   *AbsMarkerIndex = i;
			   return;
			   }
			}//for(numberof)
		seg++;
		}//for(;;)
//	return; // should never come here
}

// 0 based (enter with actual marker index)  -1 == beginnig of segment
// note: index -1 means beginning of the segment! 
void GetMarked_TCstring(char *str, int seg, int AbsIndex)
{
   //int i;
   int64 spm;
   struct DIR *d = &dir[seg];

   if( (d->Used==0) || (AbsIndex >= (int)numberof(d->MarkedList)) ) // without (int) this rejects -1
	   {
	   strcpy(str, "----");
	   return;
	   }
    
    spm = CalcMarkedSPM(seg, AbsIndex);
    GetTimeCodeString(str, spm, d->SampleRate, d->FrameRate, ':');
    return;
}


MsgHandler(OnPage_ZaxnetMarkDropout)
{
    int seg, index;
    static int Marked=0;
    char tmpstr[40], TCstring[20];

    switch (MsgType(msg)) {

        case MSG_PAGE_ENTER:
			Marked = 0;
			if(GetTransportMode()==RECORD_MODE)  
				Marked = MarkDropOut();
            return 0;

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
                int x = LCD_WIDTH/2, y = 35, ystep = CanvasFontHeight(c);
               	if(Marked==0)
					{
                    y=CanvasStringCenter(c, x, y, "Must be in record"); 
                    y=CanvasStringCenter(c, x, y, "to mark a drop out."); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
					}
				else if(Marked < 0)
					{
                    y=CanvasStringCenter(c, x, y, "ERROR"); 
                    y=CanvasStringCenter(c, x, y, "No markers available"); 
                    y=CanvasStringCenter(c, x, y, "in this segment."); 
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 

					}
				else // if(Marked > 0)
					{
					seg   = UI_GetCurrentRecSeg(); // this is the only place this is called... lets try to get rid of futuresegs
                    index = CountDropoutMarkers(seg)-1; // count markers in a segment
					GetMarked_TCstring(TCstring, seg, index);
                    y=CanvasStringCenter(c, x, y, "Location has been marked."); y+=ystep;
					sprintf(tmpstr, "Segment %d", seg);     y=CanvasStringCenter(c, x, y, tmpstr); y+=ystep/2;
					sprintf(tmpstr, "TC %s", TCstring);     y=CanvasStringCenter(c, x, y, tmpstr); y+=ystep/2;
					sprintf(tmpstr, "Marker #%d", index+1); y=CanvasStringCenter(c, x, y, tmpstr); y+=ystep/2;
                    y+=ystep;
                    y=CanvasStringCenter(c, x, y, "Press BACK to Exit"); 
					}
            }
            break;  /* Allow further processing */
        }
    return 0;
}

// list all marked dropouts in the current folder
MsgHandler(OnPage_ZaxnetAudition)
{
    int i,seg,absIndex; // //UI_LIST *p = (UI_LIST *)context;
    SETTING *setting = (SETTING*)context;   /* This setting object */
    char tmpstr[30];
	static unsigned index=0, MarkersInFolder = 0;

    switch (MsgType(msg)) {
        case MSG_LIST_COUNT_GET:
            MarkersInFolder = CountDropoutMarkersInFolder();
			msg->Value.Unsigned = MarkersInFolder;
            return MSG_DONE;

        case MSG_LIST_ITEM_GET:
            {
            MSG_PTR(LIST_ITEM, item);
            assert(item);
            assert(item->Text);
            assert(item->Index < MarkersInFolder);
			BigIndex_to_AbsIndex(&seg, &absIndex, item->Index); // this will always return something pretty
            GetMarked_TCstring(tmpstr, seg, absIndex);
            zsnprintf(item->Text, MAX_UI_TEXT, "Seg %03d: %s", seg, tmpstr);
            return MSG_DONE;
			}
        case MSG_LIST_ITEM_ACTION:
            // playback the marked portion of the segment
            //index = msg->Value.Unsigned;
            //assert(index < MarkersInFolder); 
			//pstr = Nomad_Get_HP_Name_StringPtr( index);
	        //UITextEditActivate(setting);        //UITextEditStatic(pstr,  6); // sizeof(namestr));
            return MSG_DONE;

        case MSG_TEXT_MAXLEN_GET:	
        	msg->Value.Signed = 26+1; 
        	return MSG_DONE;

        case MSG_TEXT_GET:	
//          assert(index < HPcount); // 
        	sprintf(MsgText(msg), "%d:", index+1);
        	return MSG_DONE;

        case MSG_TEXT_SET:
			//pstr = Nomad_Get_HP_Name_StringPtr(index);
          	//strcpy(pstr, MsgText(msg));
			//NVIsDirty(MIXINI);
        	return MSG_DONE;

        default: break;
    }
    return 0;
}

extern TimeCode *TCRemotePlayBack; 
extern TimeCode *TCDisk; 
//void Update_TCRemotePlayBack(struct DIR *dd); // GotoSegment() passes &dir[DMgotoseg]
int CountDropoutMarkers(int seg); // count markers in a segment

MsgHandler(OnSettingGotoPlayMarkerRequest)// { return 0; }  // Znet audition
{
    static int InitMe=1; 
    int tmpseg, marker, curseg = UI_GetCurrentPlaySeg();
	//if(curseg != PrevSeg)
	//   PrevSeg = Nomad.PlayBack.GotoPlaySegRequest = curseg; // tick to next seg if a change was detected during playback
	if( InitMe && (Nomad.PlayBack.GotoPlaySegRequest==0) )
		{ // fix uninitialize values for the first time we enter this screen
		Nomad.PlayBack.GotoPlaySegRequest = GetNumSegments();
	    Nomad.PlayBack.PlaySegMarker	  = 0;	
		InitMe=0;
		}

    switch (MsgType(msg)) {

        case MSG_VALUE_GET:		   
			msg->Value.Unsigned = Nomad.PlayBack.GotoPlaySegRequest * 100 + Nomad.PlayBack.PlaySegMarker;
            return MSG_DONE;

        case MSG_VALUE_SET:
            tmpseg = msg->Value.Unsigned / 100;
			marker = msg->Value.Unsigned % 100;
			if(marker==99)
				{
			    marker = CountDropoutMarkers(tmpseg);
				//if(marker<0) marker=0; // HEY our marker number should be index+1 so the zeroth marker == the start of the play seg!!!
				}
			if(marker > CountDropoutMarkers(tmpseg)) // count markers in a segment
			   {
			   marker=0;
			   tmpseg++;
			   }

			if( tmpseg > GetNumSegments() )
				{
				tmpseg = GetNumSegments();
				marker = 0;
				}
			else if(tmpseg < 1)
				{
				tmpseg = GetNumSegments();
				marker = 0;
				}
			Nomad.PlayBack.GotoPlaySegRequest = tmpseg; // valid seg number req
			Nomad.PlayBack.PlaySegMarker      = marker;
            return MSG_DONE;

        default: break;
    }//switch(msg)
    return 0;
}



MsgHandler(OnPage_ZaxnetReRecord) {return 0;}
MsgHandler(OnPage_ZaxnetRecover)  {return 0;} 

int  ZnetTxModePopUpCheck() // see UImetadata.cpp for Audtion page configure call in  MsgHandler(OnUI_ZnetAudition)
{
	char tmpstr[20];
	int mode = GetActualIFBmode();
	tmpstr[0]=0;
	if( mode == IFBMODE_TX)
	   return mode;
	strcpy(tmpstr, "OFF.");
	if( mode == IFBMODE_RX) 	strcpy(tmpstr, "in RX mode.");
	PopUp_Generic("Warning: IFB is", tmpstr, "", "", 4, 5000);
	return mode;
}

#else  // (not) ALLOW_AUDITION
          
// see zaxnet.cpp
//int   ZnetUI_TriggerAudition(){return 1;} // long spmOffset){return 1;}
//int   ZnetUI_StopAudition()   {return 1;}
//int   ZnetUI_TriggerReRecord(){return 1;}
//int UI_GetPlaybackPercent(unsigned long  spm, int seg) { return 0;}

void  GetMarked_TCstring(char *str, int seg, int absIndex){  strcpy(str, "00:00:00:00"); }
unsigned long  CalcMarkedSPM(int seg, int MarkedIndex)   { return 0; }
int  ZnetTxModePopUpCheck(){return 0;}

#endif // ALLOW_AUDITION



void GetMBGBstringKB(int maxlen, char *str, int64 kbytes); // from mirror.cpp

MsgHandler(OnSetting_CFprimaryFreeSpaceStr)
{
    volatile float x;
	char tmpstr[30];
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: 
			x = CalcRemainingRecordableLBAs(1);
			x *= 512;
			x /= 1024;
			GetMBGBstringKB(5, tmpstr, x); // FreeSpaceKB); // feed this space in kB
            zsnprintf(MsgText(msg), MAX_UI_TEXT, "%s FREE", tmpstr); // "13GB FREE");
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_CFmirrorFreeSpaceStr)
{
    int x;
	char tmpstr[30];
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: 
			x = CalcMirrorFreeSpaceKB(gNV.MirrorDrive, tmpstr);
            if(x>0) zsnprintf(MsgText(msg), MAX_UI_TEXT, "%s FREE", tmpstr); // "13GB FREE");
            else    zsnprintf(MsgText(msg), MAX_UI_TEXT, "----"); // no card
            return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_MaxFileSize)
{
   
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:         
            msg->Value.Signed = gNV.MaxFileSize;
            return MSG_DONE;

        case MSG_VALUE_SET:
            gNV.MaxFileSize = msg->Value.Signed;
            if(gNV.MaxFileSize<2) gNV.MaxFileSize = 2;
			 NVIsDirty(DEVAINI); 
            return MSG_DONE;

        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_PFLrouting)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.PFLroutingMode;   return MSG_DONE;
        case MSG_VALUE_SET:  gNV.PFLroutingMode = msg->Value.Signed; NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_Dynamic48V)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.Dynamic48V;    return MSG_DONE;
        case MSG_VALUE_SET:  gNV.Dynamic48V = msg->Value.Signed;	 NVIsDirty(DEVAINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_MirFolderFollow)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.MirFolderFollow;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.MirFolderFollow = msg->Value.Signed;	NVIsDirty(DEVAINI);  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_PreventPunch)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.PreventPunch;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.PreventPunch = msg->Value.Signed;	NVIsDirty(DEVAINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_VfadCycleCount)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.VfadCycleCount;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.VfadCycleCount = msg->Value.Signed;	NVIsDirty(DEVAINI);   return MSG_DONE;
        default: break;
    }
    return 0;
}

EXPORT int UI_GetNeverClipShiftDB()	// for UImeter.cpp
{
	return gNV.NeverClipShiftDB;
}


MsgHandler(OnSetting_NeverClipShiftDB) // sets gain (including OFF setting) 0,6,12,18,24 dB   see OnSetting_ISOatten
{
    int delta;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.NeverClipShiftDB;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	delta = msg->Value.Signed - gNV.NeverClipShiftDB;
        	if(delta>0) gNV.NeverClipShiftDB += 6; //  = msg->Value.Signed;  
        	if(delta<0) gNV.NeverClipShiftDB -= 6; //  = msg->Value.Signed;  
			if(gNV.NeverClipShiftDB<0) gNV.NeverClipShiftDB = 0;
			gNV.NeverClipShiftDB = gNV.NeverClipShiftDB - (gNV.NeverClipShiftDB%6); // force to 6 db steps
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_NEVERCLIP_ATTENUATION, gNV.NeverClipShiftDB/6);
		    UpdateDiskShifters();
        	NVIsDirty(DEVAINI);   
        	return MSG_DONE;
        default: break;
    }
    return 0;
}



EXPORT int UI_GetIsoAttenMask()	// for UImeter.cpp
{
	return gNV.ISOattenMask;
}
MsgHandler(OnSetting_ISOatten)// sets channel mask
{
    int i;
	uchar ary[32];
    MATRIX_CELL *cell = GetCellPtr(msg); // (MATRIX_CELL*)MsgPtr(msg);
    volatile uint32 channel;
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: // data TO UI
			channel = MatrixToLinear(8, cell); // ncols 
			cell->Value=0;
			if(channel<16) // gMachineInfo.MaxRecordChannels)
    			cell->Value = 0 != (gNV.ISOattenMask&(1<<channel));
            return MSG_DONE;
        case MSG_MATRIX_CELL_SET: // data FROM UI
			channel = MatrixToLinear(8, cell); // ncols 
		    if( GetTransportMode() == RECORD_MODE)
				{
		   		PopUp_RecordConflict(); // cant change these now!
				return MSG_DONE;
				}
			if(channel<16) 
			  { // we need this cuz its non vovlatile....
              if(cell->Value) gNV.ISOattenMask |= (1<<channel);
			  else			  gNV.ISOattenMask &= (~(1<<channel));
			  UpdateDiskShifters();
			  if(context != gPCcontext)
				{
				for(i=0;i<16;i++)
					ary[i] = (0 != ((1<<i)&gNV.ISOattenMask));
				SendSetting_Array2Drow(SETTING_TRACKMIX_NEVERCLIP, 0, i, ary, 8,12);   // send contents of gNV.ISOattenMask
				}
		      NVIsDirty(DEVAINI);
			  }
            return MSG_DONE;
        default: break;
    }
    return 0;	
}


MsgHandler(OnSetting_ExtraInputGain) // 0 means none, 1 = +10dB (effects ANALOG inputs 1-6 only)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.ExtraInputGain;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.ExtraInputGain = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_ExtraDigitalInputGain) // 0 means none, 1 = +10dB (effects DIGITAL inputs 1-4 only)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.ExtraDigitalGain;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.ExtraDigitalGain = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_ExtraReturnGain) // 0 means none, 1 = +10dB (effects RETURN 1,2 )
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.ExtraReturnGain;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.ExtraReturnGain = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

int UI_GetMP3supportSetting()
{
     return  gNV.MP3mode;
}


MsgHandler(OnSetting_MP3support) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.MP3mode;  return MSG_DONE;
        case MSG_VALUE_SET:  
			//gNV.MP3mode = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
    	    gNV.MP3mode = 0;  // I also disabled it in tasks.cpp (search for fixme)
			PopUp("MP3 is disabled\nin this version",1);
	        NVIsDirty(DEVAINI); 
    	    return MSG_DONE; // dont allow mp3 for now since it interferes with recording

        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_MP3mix) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.MP3mix;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.MP3mix = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

// menu = {'NORMAL', 'FAST1', 'FAST2', 'FAST3 !', 'FAST4 !!'},
MsgHandler(OnSetting_DSPspeed)	// this was 0,1,2
{
	int delta;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.DSPspeed;  return MSG_DONE;
        case MSG_VALUE_SET:  
			delta = msg->Value.Signed - gNV.DSPspeed;
			if(delta > 0)	gNV.DSPspeed = 2; // dont use 1 cuz it was the default for a while before this feature existed
			if(delta < 0)	gNV.DSPspeed = 0;
			//if(msg->Value.Signed==0)	gNV.DSPspeed = msg->Value.Signed;  
			//else						gNV.DSPspeed = 2; // fast is an extra 2*24 MHz
			//gNV.DSPspeed = clip(gNV.DSPspeed, 0, 2);
        	NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_AllowLogging)	// enables dprintf debug messages to screen (like Jtag enable)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.AllowLogging;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.AllowLogging = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

void GUI_TriggerWriteDebugLogFile(int drive);
MsgHandler(OnSetting_Page1967Screen) // display the 1967 screen
{
    switch (MsgType(msg)) {
	        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
                {                          
            	if( IsMirroring() ) {PopUp_MirrorConflict(); return MSG_DONE; }
		    	if( GetTransportMode() == RECORD_MODE){PopUp_RecordConflict();  return MSG_DONE; }
                GUI_TriggerWriteDebugLogFile(DRIVE_CF);
                return MSG_DONE;
                }
            }
            break;   

        case MSG_PAGE_DRAW:
            {
                CANVAS *c = UICanvas();
				int ystep = CanvasFontHeight(c);
           		int yy = LCD_HEIGHT-ystep; // LCD_HIGHT - 10; // start at bottom of the screen
				char str[150];
				int idex = GetLogFileLine(str, -1); // get last line
				for(int i=0;i<12;i++)
					{
					if(idex < 0)
						break;
					CanvasString(c, 1, yy, str );
					yy -= ystep;
				    idex = GetLogFileLine(str, idex); // get prev line
					}
				return 0; //  Allow further processing 
			}
		default:
			break;
        }
    return 0;
}

void ActivateTone(int state)
{
	if(gNV.fToneOn==state)
		return; // dont jitter when turning on repeatedly in CDScal mode
	gNV.fToneOn=state;	
	if(state)  ToneOn(); 
	else 	   ToneOff(); 	
	SetToneMix(gNV.fToneOn);
}

extern int gTrimCalChannels; // see N_HARDWAREOPT.CPP
MsgHandler(OnSetting_TrimCalChannels)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gTrimCalChannels;  return MSG_DONE;
        case MSG_VALUE_SET:  gTrimCalChannels  = msg->Value.Signed; return MSG_DONE;
        default: break;
    }
    return 0;
}

void GotoPage_ASYNC(UI_PAGE *p); // in tasks.cpp
MsgHandler(OnSetting_TrimCalGo)
{	
	static int mem=0;

    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	msg->Value.Signed = mem;  	
        	return MSG_DONE;
        case MSG_VALUE_SET:  
        	mem = msg->Value.Signed;	
			if( (mem==4) && gTrimCalChannels )
				{
        		gCDS_TABLE_DEBUG = 1; // trigger the calibration
				Save_UIHomePageVariant(HOMEPAGE_I4P4);
				//gNV.HomePageVariant[Nomad.Settings.HomePageVariantBank] = HOMEPAGE_I4P4; // 4
				GotoPage_ASYNC(&UIHome1); 
				PopUp("calibrating...",1);
				}
        	return MSG_DONE;
/*
		case MSG_KEY_PRESS: // why cant I see any key presses here???
			if( (MsgKey(msg) == KEY_COM) && (mem==4) && gTrimCalChannels )
				{
        		gCDS_TABLE_DEBUG = 1; // trigger the calibration
				mem = 0; // show that we are doing something now
				return MSG_DONE;
				}
			break; // allow further processing of keys
*/
        default: break;
    }

    return 0;
}
  	
short gForceSlateMic=0;
short gForceComMic=0;
void RestoreMainMixes(); // for SLATE button
MsgHandler(OnSetting_ForceSlate)// THIS ONE HAS NO MEMORY AND IS FOR TRADE SHOWS!!!
{
	int mem=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gForceSlateMic;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	gfSlateOn      = msg->Value.Signed; 
			gForceSlateMic = msg->Value.Signed; // =1;
   			RestoreMainMixes();
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_BootJamDate)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.BootJamDate;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	gNV.BootJamDate  = msg->Value.Signed; 
			if(context != gPCcontext)
				SendSetting_Byte(SETTING_TIMECODE_AUTOJAM, msg->Value.Signed );
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}

EXPORT int GetSetExtFad(int ExtFaderBase, MESSAGE *msg, void *context); //

//TrimRecallTable[i] = GetNVTrim(i); // store all the current trims
float TrimRecallTable[MAX_INPUT_SOURCES];
MsgHandler(OnExtFaderAssignPage) //    msghandler = { 'OnFaderAssignPage'},  -- handles custom drawing and user interaction
{
	int i;
	if(MsgType(msg) == MSG_PAGE_ENTER)
		{
		for(i=0;i<MAX_INPUT_SOURCES;i++)
    		TrimRecallTable[i] = GetNVTrim(i); // store all the current trims (from gChannelConfig[channel].Trim)
		}
	return 0;
}

MsgHandler(OnSetting_ExtFad1) {	return GetSetExtFad(1-1, msg, context); }
MsgHandler(OnSetting_ExtFad2) {	return GetSetExtFad(2-1, msg, context); }
MsgHandler(OnSetting_ExtFad3) {	return GetSetExtFad(3-1, msg, context); }
MsgHandler(OnSetting_ExtFad4) {	return GetSetExtFad(4-1, msg, context); }
MsgHandler(OnSetting_ExtFad5) {	return GetSetExtFad(5-1, msg, context); }
MsgHandler(OnSetting_ExtFad6) {	return GetSetExtFad(6-1, msg, context); }
MsgHandler(OnSetting_ExtFad7) {	return GetSetExtFad(7-1, msg, context); }
MsgHandler(OnSetting_ExtFad8) {	return GetSetExtFad(8-1, msg, context); }

MsgHandler(OnSetting_ExtFad9)  {	return GetSetExtFad(9-1, msg, context); }
MsgHandler(OnSetting_ExtFad10) {	return GetSetExtFad(10-1, msg, context); }
MsgHandler(OnSetting_ExtFad11) {	return GetSetExtFad(11-1, msg, context); }
MsgHandler(OnSetting_ExtFad12) {	return GetSetExtFad(12-1, msg, context); }
MsgHandler(OnSetting_ExtFad13) {	return GetSetExtFad(13-1, msg, context); }
MsgHandler(OnSetting_ExtFad14) {	return GetSetExtFad(14-1, msg, context); }
MsgHandler(OnSetting_ExtFad15) {	return GetSetExtFad(15-1, msg, context); }
MsgHandler(OnSetting_ExtFad16) {	return GetSetExtFad(16-1, msg, context); }


//---- these are Nova's GUI enmus that ui.lua uses as fader assigns in the EXT fader page
//                     0   'OFF',
#define A_TRIM_START   1                //         'A1+Trim', 'A2+Trim', 'A3+Trim', 'A4+Trim', 'R1+Trim', 'R2+Trim',
#define D_TRIM_START  (A_TRIM_START+6)  // 7  //   'D1+Trim', 'D2+Trim', 'D3+Trim', 'D4+Trim', 'D5+Trim', 'D6+Trim', 'D7+Trim', 'D8+Trim',
#define RX_TRIM_START (D_TRIM_START+8)  //  (11+4) 'RX1+Trim','RX2+Trim','RX3+Trim','RX4+Trim','RX5+Trim','RX6+Trim','RX7+Trim','RX8+Trim',
					   
#define A_ZNET_START  (RX_TRIM_START+8) // (19+4)   //   'A1+ZNET', 'A2+ZNET', 'A3+ZNET', 'A4+ZNET', 'R1+ZNET', 'R2+ZNET',
#define D_ZNET_START  (A_ZNET_START+6)  // (25+4)   //   'D1+ZNET', 'D2+ZNET', 'D3+ZNET', 'D4+ZNET', 'D5+ZNET', 'D6+ZNET', 'D7+ZNET', 'D8+ZNET',
#define RX_ZNET_START (D_ZNET_START+8)  // (29+4+4) //   'RX1+ZNET','RX2+ZNET','RX3+ZNET','RX4+ZNET','RX5+ZNET','RX6+ZNET','RX7+ZNET','RX8+ZNET',

#define ERR_START     (RX_ZNET_START+8)

int CalcInputFromUIExtFaderAssEnum(int uiEnum) // returns channel index and -1 if OFF
{
	if(uiEnum >= A_ZNET_START)
		uiEnum -= (A_ZNET_START-A_TRIM_START); // get it down to lower half

	if(uiEnum >= RX_TRIM_START)		return uiEnum - RX_TRIM_START + IN_CHAN_DIGITAL_RX_1;
	if(uiEnum >=  D_TRIM_START)		return uiEnum -  D_TRIM_START + IN_CHAN_DIGITAL_1;
	return uiEnum - A_TRIM_START;
}
/*
#define A_START    1 // 1   'A1','A2','A3','A4','R1','R2',  (6)
#define D_START    7 // 7   'D1','D2','D3','D4',            (4)
#define RX_START  11 //     'RX1','RX2','RX3','RX4','RX5','RX6','RX7','RX8', (8)
#define TA_START  19 // 19  'TA1','TA2','TA3','TA4','TR1','TR2', (6)
#define TD_START  25 // 29  'TD1','TD2','TD3','TD4',             (4)
#define TRX_START 29 //     'Trx1','Trx2','Trx3','Trx4','Trx5','Trx6','Trx7','Trx8', (8)
#define Z_START   37 // 37  'Z1','Z2','Z3','Z4','Z5','Z6','Z7','Z8','Z9','Z10', 'Z11','Z12','Z13','Z14','Z15','Z16','Z17','Z18','Z19','Z20',
#define ERR_START 57 // 57  
*/


//msg->Value.Unsigned is a long list from Nomad GUI as displayed above
EXPORT int GetSetExtFad(int ExtFaderBase, MESSAGE *msg, void *context) //
{
	volatile int in,tmode, faderbase = ExtFaderBase; // this is just 0-7 
	volatile int FaderNum = faderbase + EXTERNAL_FADER_1;	    
	if(FaderNum > EXTERNAL_FADER_16) // EXTERNAL_FADER_8)
		return 0;
	if(FaderNum < EXTERNAL_FADER_1)
		return 0;
	struct FADER_ASS *p = GetFaderAssPtr(FaderNum); // &FaderAssignList[FaderNum]; // short hand

    switch  (MsgType(msg)) 
    { 
//        case MSG_TEXT_GET:  {
//        	zsnprintf(MsgText(msg), MAX_UI_TEXT, "F%d %d %d", ExtFaderBase, p->type, p->ch[0]);
//        	return MSG_DONE; 
//        	}
        case MSG_VALUE_GET: 
        	{
			msg->Value.Unsigned = 0;
			if(p->type!=FADERTYPE_DISABLED) 
				{
				int ch=p->ch[0],  offset = 0;
				if( IsGoodIndex(ch, numberof(gChannelConfig)) )
					{
					//if(gChannelConfig[ch].Mix8TrimMode==TRIMMODE_ZAXNET ) 
					if(p->type == FADERTYPE_ZNET) 
						offset = A_ZNET_START - A_TRIM_START; // offset into znet trim mode LUT 
					}					
				if(     InRange(ch, IN_CHAN_ANALOG_1,   IN_CHAN_RETURN_2)) 		msg->Value.Unsigned = A_TRIM_START  + offset + ch; // return A1-R2
				else if(InRange(ch, IN_CHAN_DIGITAL_1,  IN_CHAN_DIGITAL_8)) 	msg->Value.Unsigned = D_TRIM_START  + offset + ch-IN_CHAN_DIGITAL_1;    // return D1-D4
				else if(InRange(ch, IN_CHAN_DIGITAL_RX_1,IN_CHAN_DIGITAL_RX_8))	msg->Value.Unsigned = RX_TRIM_START + offset + ch-IN_CHAN_DIGITAL_RX_1; // return RX1-RX8
				else msg->Value.Unsigned = 0; //  ERROR!
				}
			/*
			if(p->type == FADERTYPE_TRIM)
				{ // should this ever happen???
				msg->Value.Unsigned = 0; //  ERROR!
				}
			if(p->type == FADERTYPE_ZNET)
				{
				msg->Value.Unsigned = 0; // Z_START + p->ch[0]-1; // return Z1-Z20 . not an index so minus 1
				}
        	*/
        	return MSG_DONE; 
        	}

        case MSG_VALUE_SET: 
       		{
			// PCGUI needs to have its enums updated to handle this flexability - or we could limit us to the single fader old-style
			// AudioMix.cpp::Nomad_SetFaderAssign(int TypeMask, int iInput, int iFader) allows digital to be specified by (in>=10) or FMASK_DIGITAL

			//int ftype = GetExtFad(ExtFaderBase); // this is the current fader asignment. It is the trim that we are going to potentially destroy
			//MESSAGE mm;//	void *context = NULL; 
			//mm.Type=MSG_VALUE_GET;
			//GetSetExtFad(ExtFaderBase, &mm, NULL); // returns UI enum that is not the same as the PC now
			//int ftype = mm.Value.Unsigned;
			if(msg->Value.Unsigned >= ERR_START)
				return MSG_DONE; // clip at edges

			msg->Value.Unsigned = clip(msg->Value.Unsigned, 0, ERR_START);
			int ch = clip(CalcInputFromUIExtFaderAssEnum(msg->Value.Unsigned), 0, numberof(gChannelConfig));
			dprintf("ch = %d ValUns=%d\n", ch, msg->Value.Unsigned);

			if( (p->type!=FADERTYPE_DISABLED) && !IsNova2() ) 
				{ // limit what channels used can choose from (skip Dig 5-8)
				int CurCh = p->ch[0]; // cet current channel
				if( (CurCh == IN_CHAN_DIGITAL_4) && (ch == IN_CHAN_DIGITAL_5) )
					ch = IN_CHAN_DIGITAL_RX_1; // skip up to RX1
				if( (CurCh == IN_CHAN_DIGITAL_RX_1) && (ch == IN_CHAN_DIGITAL_8) )
					ch = IN_CHAN_DIGITAL_4;    // skip down to 4
				}

			if(msg->Value.Unsigned>=RX_ZNET_START)		Nomad_SetFaderAssign(FADERTYPE_ZNET, ch, FaderNum);    // Z RX1-8
			else if(msg->Value.Unsigned>=D_ZNET_START)	Nomad_SetFaderAssign(FADERTYPE_ZNET, ch, FaderNum);	   // Z D1-8
			else if(msg->Value.Unsigned>=A_ZNET_START)	Nomad_SetFaderAssign(FADERTYPE_ZNET, ch, FaderNum);	   // Z A1-6
			else if(msg->Value.Unsigned>=RX_TRIM_START)	Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);   // T RX1-8
			else if(msg->Value.Unsigned>=D_TRIM_START)	Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);   // T D1-8
			else if(msg->Value.Unsigned>=A_TRIM_START) 	Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);   // T A1-6
        	else 										Nomad_SetFaderAssign(FADERTYPE_DISABLED, ch, FaderNum);
   			SetNVTrim(ch, TrimRecallTable[ch]); // restore the current trim setting that we just stepped on

			// if we are setting to trim mode, restore the trim that we just previously destroyed
			//volatile int inputnum = ftype - A_TRIM_START;
			//if( InRange(msg->Value.Unsigned, A_TRIM_START, RX_TRIM_START+8) ) //  (inputnum>=IN_CHAN_ANALOG_1) && (inputnum<=IN_CHAN_DIGITAL_RX_1) )
    		//	SetNVTrim(ch, TrimRecallTable[ch]); // restore the current trim setting that we just stepped on

			NVIsDirty(MIXINI); // if we turn something off we still need to save it!
			NVIsDirty(AICINI); // might as well save fader positions in gChannelConfig now too right?
        	return MSG_DONE; 
        	} 
        default: break;
    }
    return 0;

}



EXPORT UI_PAGE *GetF1KeyShortcut()
{
	if(gNV.FP8keyF1==F1_KEY_UIMirrorCF)      return &UIMirrorCF;     
	if(gNV.FP8keyF1==F1_KEY_UIPrimaryCF)     return &UIPrimaryCF;
	if(gNV.FP8keyF1==F1_KEY_UITrackNames)    return &UITrackNames;
	if(gNV.FP8keyF1==F1_KEY_UIRecordEnables) return &UIRecordEnables;
	if(gNV.FP8keyF1==F1_KEY_UIMetaData)      return &UIMetaDataDisplay;
	return &UIMirrorCF;
}

MsgHandler(OnSetting_F1_Key) //gNV.MasterOutputmode
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.FP8keyF1;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.FP8keyF1 = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_ZaxNetMode) // 7.04 gNV.ZaxNetMode    this is TC output style zaxnet!!
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	msg->Value.Signed = gNV.ZaxNetMode;  
        	SetAllowZaxNet_TCTX(gNV.ZaxNetMode); 
        	return MSG_DONE;

        case MSG_VALUE_SET:  
        	gNV.ZaxNetMode = msg->Value.Signed;  
        	SetAllowZaxNet_TCTX(gNV.ZaxNetMode); 
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE;
        default: break;
    }
    return 0;
}


MsgHandler(OnSetting_HPphaseChange)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.HPphaseChange;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.HPphaseChange  = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_HPextraGain)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.HPextraGain;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.HPextraGain  = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}



MsgHandler(OnSetting_AllowMultiTrim)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.AllowMultiTrim;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.AllowMultiTrim  = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

/*
void ChangeExtFaderMixPreset(int bank)
{
//	bank = clip(bank, 0, NUM_FADER_BANKS-1);
//	int i;
    //LoadPreset(FADERASSIGN_PRESET,     bank); // .FaderMixPresetNum);
	//LoadPreset(FADERASSIGN_AUX_PRESET, bank); // .FaderMixPresetNum);
	FaderSetup(); // bank);    // setup and install data into FaderEnabled[] stored in FaderAssignMatrix[][] and fill mix12ChannelMap[]
//	fFaderAssignDirty=1; // force a call to RebuildFaderAssign() which updates the actual fader coefs
	return;
}
*/

MsgHandler(OnSetting_ExtBank) // OASIS ONLY!   THIS DOES NOT GET CALLED WHEN HITTING THE FRONT_BANK KEY!!!
{
	int bank;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gExtBankStates.CurrentBank;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	SetNewExtBank(msg->Value.Unsigned); 
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; // store the cahnge in  gNV.ExtBank
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_IntBank) // NOVA FRONT BANK! 
{
	int bank;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gIntBankStates.CurrentBank;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	SetNewInternalBank(msg->Value.Unsigned); 
	        NVIsDirty(DEVAINI); 
    	    return MSG_DONE; // store the cahnge in  gNV.FrontBank
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_MuteFrame1TCtest)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = Nomad.Settings.MuteFrame1TCtest;  return MSG_DONE;
        case MSG_VALUE_SET:  Nomad.Settings.MuteFrame1TCtest = msg->Value.Signed;  return MSG_DONE;
        default: break;
    }
    return 0;
}

// allows us to adjust value in greater steps than LUA wants to. typically inc1=10 and inc2=100
int GUIdelta(int msgval, int gnval, int inc1, int inc2)
{
	int delta =  msgval - gnval;
    if(delta ==  1) gnval += inc1;
	else if(delta == -1) gnval -= inc1;
    else if(delta ==  10) 
    	{
    	gnval += inc2; // we need >= in case we overshoot lua's limit!
		gnval -= (gnval%inc2); // this prevents us from overshooting LUAs limit
		}
	else if(delta == -10) 
		{
		gnval -= inc2;
		gnval -= (gnval%inc2);
		}

    else // if( (delta >=  10) || (delta >=  10) )
    	gnval = msgval; // some one typed in a number maunally ... I hope
	return gnval;
}	

 
MsgHandler(OnSetting_SlatePageBeepDelay)
{
	int delta;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gNV.SlatePageBeepDelay;  return MSG_DONE;
        case MSG_VALUE_SET:  
			gNV.SlatePageBeepDelay = GUIdelta(msg->Value.Signed, gNV.SlatePageBeepDelay, 10 ,100);
	        NVIsDirty(DEVAINI); //  NVIsDirty(AICINI);   NVIsDirty(MIXINI);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
/*
MsgHandler(OnSetting_FrontBank)
{
	int delta;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.FrontBank;  return MSG_DONE;
        case MSG_VALUE_SET:  
        	gNV.FrontBank = clip(msg->Value.Unsigned, 0, 3);   
        	NVIsDirty(DEVAINI);
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
*/
MsgHandler(OnSetting_UseCelcius)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.UseCelcius;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.UseCelcius = msg->Value.Unsigned;   NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_FanSpeedRecord)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Unsigned = gNV.FanSpeedRecord;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.FanSpeedRecord = msg->Value.Unsigned;   NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}


int GetMode_ZaxNetTXSlaved()
{
	return gNV.ZaxNetTXSlaved;
}


int ConvertNomadXpointToCellValue(XPOINT_STATE x, MixerType type, InputBus bus, int iInput, int iOutput)
{
	int v = MCELL_OFF; 
	if(bus == DIGITAL_BUS)
		{
		if(x.B==DIGITAL)    v = MCELL_ON; // UI = 'X'  (DIGITAL = 4)
		if(x.B==PF_DIGITAL) v = MCELL_PF; // UI = 'P'  (PF_DIGITAL = 3)
		if(x.BInvrt)        v++;          // inc to UI's inverted state
		}
	else
		{
		if(x.A == ANALOG)   		v = MCELL_ON; // UI = 'X'  (DIGITAL = 4)
		if(x.A == PF_ANALOG)		v = MCELL_PF; // UI = 'P'  (PF_DIGITAL = 3)
		if(x.AInvrt)       			v++;          // inc to UI's inverted state
		if(x.A && (bus==ST_BUS))    v = MCELL_ON; // Slate Tone and TC are only on or off!!
		if(x.A && (bus==OUTPUT_BUS))v = MCELL_ON; // outs are only on or off!!
		}
	return v;		
}

//         rowscroller = 'bus.track.offset',
/*
MsgHandler(OnSetting_BusTrackOffset) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed  = gUI_BusTrackOffset;  return MSG_DONE;
        case MSG_VALUE_SET:  gUI_BusTrackOffset = msg->Value.Signed;   return 0; // MSG_DONE;
        default: break;
    }
    return 0;
}
*/


// this one is from RX12
/*
MsgHandler(OnSetting_FACT_STORE_MACHINE_INFO_RX12) 
{
	static int mem=0;
	static ALARM alarm=0;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  
        	if(mem && CHECK_ALARM(alarm-100))
				PopUp_Generic("BURNING", "MACHINE", "INFO", "", 4, 1000);
			if(mem && CHECK_ALARM(alarm))
				{
				mem=0;
				BurnMachineInfoStruct_REQ();
				}
        	return MSG_DONE;
        case MSG_VALUE_SET:  
        	mem =  msg->Value.Signed;
			if(mem)
				{
				PopUp_Generic("BURNING", "MACHINE", "INFO", "", 4, 1000);
				SET_ALARM(alarm, 1000);
				}
        	return MSG_DONE;
        default: break;
    }
    return 0;
}
*/
// this one is from nomad
MsgHandler(OnSetting_FACT_STORE_MACHINE_INFO) 
{                       
	static int mem=0;
    switch (MsgType(msg)) {	 
        case MSG_VALUE_GET:  msg->Value.Signed = mem; return MSG_DONE;
        case MSG_VALUE_SET:  
			if(msg->Value.Signed>0)
				{
				//PartialDebugScreen(6, true);
				BurnMachineInfoStruct_REQ();
				}
			msg->Value.Signed=0;
	        return MSG_DONE;
        default: break;
    }
    return 0;
}
/*
MsgHandler(OnSetting_AES42_12_powerOnOff) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AES42_12_powerOn; return MSG_DONE;
        case MSG_VALUE_SET:	
        	gNV.AES42_12_powerOn  = msg->Value.Signed; // AES42_Power_Handler() will handle this change for us magically
			NVIsDirty(DEVAINI); 
			return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSetting_AES42_34_powerOnOff) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AES42_34_powerOn; return MSG_DONE;
        case MSG_VALUE_SET:	
        	gNV.AES42_34_powerOn  = msg->Value.Signed; // AES42_Power_Handler() will handle this change for us magically
			NVIsDirty(DEVAINI); 
			return MSG_DONE;
        default: break;
    }
    return 0;
}						             
*/
// kludgy way to deal with input CHANX menu not updating its text
MsgHandler(OnSetting_AES42_1234_powerOnOff) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	
        	if( (Nomad.Setup.Chan==IN_CHAN_DIGITAL_1) || (Nomad.Setup.Chan==IN_CHAN_DIGITAL_2) )
	        	msg->Value.Signed = gNV.AES42_12_powerOn;
        	else if( (Nomad.Setup.Chan==IN_CHAN_DIGITAL_3) || (Nomad.Setup.Chan==IN_CHAN_DIGITAL_4) )
    	    	msg->Value.Signed = gNV.AES42_34_powerOn;
			else 
				msg->Value.Signed = 0; // invalid channel
			 return MSG_DONE;
        case MSG_VALUE_SET:	
        	if( (Nomad.Setup.Chan==IN_CHAN_DIGITAL_1) || (Nomad.Setup.Chan==IN_CHAN_DIGITAL_2) )
	        	gNV.AES42_12_powerOn = msg->Value.Signed; // AES42_Power_Handler() will handle this change for us magically
        	else if( (Nomad.Setup.Chan==IN_CHAN_DIGITAL_3) || (Nomad.Setup.Chan==IN_CHAN_DIGITAL_4) )
	        	gNV.AES42_34_powerOn = msg->Value.Signed; // AES42_Power_Handler() will handle this change for us magically
			NVIsDirty(DEVAINI); 
			return MSG_DONE;
        default: break;
    }
    return 0;
}						             

/*
MsgHandler(OnSetting_AES42mod) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AES42mode; return MSG_DONE;
        case MSG_VALUE_SET:	
        	gNV.AES42mode  = msg->Value.Signed; 
   			Set_AES42_MOD_state(gNV.AES42mode); // AES42 modulation pin
			NVIsDirty(DEVAINI); 
			return MSG_DONE;
        default: break;
    }
    return 0;
}						             
*/

MsgHandler(OnSetting_ADC_12_powerOn) 
{// ProxyADCPowerDown_Handler() will handle this change for us magically
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed  = gNV.ADC_12_powerOn; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.ADC_12_powerOn = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSetting_ADC_34_powerOn) 
{// ProxyADCPowerDown_Handler() will handle this change for us magically
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed  = gNV.ADC_34_powerOn; return MSG_DONE;
        case MSG_VALUE_SET:	gNV.ADC_34_powerOn = msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             



void UI_SetTrimKeyActiveState(int state)
{
	gIntBankStates.ZnetButtonActive = 0;
	if(state<0)
		gIntBankStates.TrimButtonActive = !gIntBankStates.TrimButtonActive;
	else if(state)
		gIntBankStates.TrimButtonActive=1;
	else
		gIntBankStates.TrimButtonActive=0;
}

void UI_SetZnetKeyActiveState(int state)
{
	gIntBankStates.TrimButtonActive = 0;
	if(state<0)
		gIntBankStates.ZnetButtonActive = !gIntBankStates.ZnetButtonActive;
	else if(state)
		gIntBankStates.ZnetButtonActive=1;
	else
		gIntBankStates.ZnetButtonActive=0;
}

MsgHandler(OnSetting_OutType){return 0;}

int UI_GetButtonHoldTimeMS(){return Nomad.Settings.ButtonHoldTimeMS;}
MsgHandler(OnSetting_ButtonHoldTimeMS)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed  = gNV.ButtonHoldTimeMS; return MSG_DONE;
        case MSG_VALUE_SET:	
			gNV.ButtonHoldTimeMS = GUIdelta(msg->Value.Signed, gNV.ButtonHoldTimeMS, 100 , 200);
        	Nomad.Settings.ButtonHoldTimeMS = gNV.ButtonHoldTimeMS; //  = msg->Value.Signed;  
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

EXPORT void UI_Set_SlatePageFlipMode(int val)
{
	if(val < 0)
		gNV.SlatePageFlipMode = !gNV.SlatePageFlipMode;
	else 
		gNV.SlatePageFlipMode = val;
	Nomad.Settings.SlatePageFlipMode = gNV.SlatePageFlipMode;	
}
MsgHandler(OnSetting_SlatePageFlipMode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.SlatePageFlipMode; return MSG_DONE;
        case MSG_VALUE_SET:	UI_Set_SlatePageFlipMode(msg->Value.Signed);     	return MSG_DONE; 
        default: break;
    }
    return 0;
}


void UI_SaveFrontBankSetting()
{
	NVIsDirty(DEVAINI); // store the change in  gNV.FrontBank if we are really running
}


int SetTestMode(int mode);
MsgHandler(OnSetting_TestMode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = Nomad.Settings.TestMode;  return MSG_DONE;
        case MSG_VALUE_SET:
			Nomad.Settings.TestMode = msg->Value.Signed;  
			SetTestMode(Nomad.Settings.TestMode);
        	PartialDebugScreen(2);
			return MSG_DONE;
 	       	//break;
			
		default: 
			break;
    	}
    return 0;
}

#define OUT_ZERO_DB 2
#define OUT_N10_DB  1
#define OUT_N35_DB  0
extern short gCDS_TABLE_DEBUG;

void SetAllBusGains_Test(int gainval)
{
	UI_SetBusGainSetting(eBUS1A,gainval, &gOutputChanConfig[0].Gain,       ODBCINI);
	UI_SetBusGainSetting(eBUS2A,gainval, &gOutputChanConfig[1].Gain,       ODBCINI);

	UI_SetBusGainSetting(eBUS1, gainval, &gOutputChanConfig[0].GainTAfive, ODBCINI);
	UI_SetBusGainSetting(eBUS2, gainval, &gOutputChanConfig[1].GainTAfive, ODBCINI);
	UI_SetBusGainSetting(eBUS3, gainval, &gOutputChanConfig[2].GainTAfive, ODBCINI);
	UI_SetBusGainSetting(eBUS4, gainval, &gOutputChanConfig[3].GainTAfive, ODBCINI);
	UI_SetBusGainSetting(eBUS5, gainval, &gOutputChanConfig[4].GainTAfive, ODBCINI);
	UI_SetBusGainSetting(eBUS6, gainval, &gOutputChanConfig[5].GainTAfive, ODBCINI);
} 
int SetTestMode(int mode)
{
	int i;
	static int prev=0;
	char tmpstr1[30], tmpstr2[30], tmpstr3[30];    //float *iGain = &gOutputChanConfig[0].Gain;
	tmpstr1[0]=0;
	tmpstr2[0]=0;
	tmpstr3[0]=0;

	if(mode!=prev)
		dprintf("\n\n %d ",mode);

		switch (mode) {
			case 0: // do nothing
				break; 
			case 1: // default base settings
				sprintf(tmpstr1, "OUTS=0dB");
				sprintf(tmpstr2, "MIC, HPF=OFF, 48V=OFF");
				SetAllBusGains_Test(OUT_ZERO_DB);
	        	for(i=0;i<4;i++)
					{
					gChannelConfig[i].fHPF=0;
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
	  			ActivateTone(0);
				break;
			case 2: 
				sprintf(tmpstr1,"TONE, OUTS=0dB ");
				SetAllBusGains_Test(OUT_ZERO_DB);
			    SetToneFreq(1000); 
	  			ActivateTone(1);
				break;
			case 3: 
				sprintf(tmpstr1,"TONE, OUTS -10dB ");
				SetAllBusGains_Test(OUT_N10_DB);
	  			ActivateTone(1);
				break;
			case 4: 
				sprintf(tmpstr1,"TONE, OUTS -35dB");
				SetAllBusGains_Test(OUT_N35_DB);
	  			ActivateTone(1);
				break;
			case 5: 
				sprintf(tmpstr1, "INPUTS=LINE");
				sprintf(tmpstr2, "OUTS 0dB");
				SetAllBusGains_Test(OUT_ZERO_DB);
	        	for(i=0;i<4;i++)
	        		gChannelConfig[i].fMicLevel = 0;
	  			ActivateTone(1);
				break;
			case 6: 
				sprintf(tmpstr1,"TONE 30Hz");
			    SetToneFreq(30); 
	  			ActivateTone(1);
				break;
			case 7: 
				sprintf(tmpstr1,"HP FITERS = ON");
	        	for(i=0;i<4;i++)
					gChannelConfig[i].fHPF=1;
				break;
			case 8: 
				sprintf(tmpstr1,"HP FITERS = OFF");
	        	for(i=0;i<4;i++)
					gChannelConfig[i].fHPF=0;
				break;
			case 9: 
				sprintf(tmpstr1,"TONE = 1K");
			    SetToneFreq(1000); 
	  			ActivateTone(1);
				break;
			case 10: 
				sprintf(tmpstr1,"INPUTS=MIC LOUD");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				break;
			case 11: 
				sprintf(tmpstr1,"INPUTS=MIC QUIET");
				sprintf(tmpstr2,"----48V IS NEXT----");
				SetAllBusGains_Test(OUT_N35_DB);
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				break;
			case 12: 
				sprintf(tmpstr1,"INPUTS=MIC <48Vch1=ON>");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				gChannelConfig[0].f48V = TRUE;
				break;
			case 13: 
				sprintf(tmpstr1,"INPUTS=MIC <48Vch2=ON>");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				gChannelConfig[1].f48V = TRUE;
				break;
			case 14: 
				sprintf(tmpstr1,"INPUTS=MIC <48Vch3=ON>");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				gChannelConfig[2].f48V = TRUE;
				break;
			case 15: 
				sprintf(tmpstr1,"INPUTS=MIC <48Vch4=ON>");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				gChannelConfig[3].f48V = TRUE;
				break;
			case 16: 
				sprintf(tmpstr1,"ALL PHANT = OFF");
	        	for(i=0;i<4;i++)
					{
	        		gChannelConfig[i].fMicLevel = 1;
					gChannelConfig[i].f48V = FALSE;
					}
				break;

			case 20: 
				sprintf(tmpstr1,"TRIM CAL, OUTS -10dB");
				SetAllBusGains_Test(OUT_N10_DB);
				gCDS_TABLE_DEBUG=1;
				break;


 	       default:
				sprintf(tmpstr1,"--- INVALID TEST MODE ---");
 	       		break;
		}

	if(mode != prev)
		{
		prev = mode;
		sprintf(tmpstr3, "mode %d\n%s\n%s\n", mode, tmpstr1, tmpstr2);
		//dprintf(tmpstr3);
		PopUp(tmpstr3,99);
		KillNVsave(); // dont save any of these changes!
		}

	return mode;
}

void DeleteTrailingChars(char *ss, char c)
{
   short i,len;
   
   if(ss[0]==0) 
      return;
      
   len=strlen(ss);   
   
   for(i=(len-1);i>0;i--)
      {
      if(ss[i] != c) break;  // we found a char that should be deleted
      ss[i]=0;   // remove it
      }
   return;
}


MsgHandler(OnSetting_WalkieEnable)// Setting { name = 'WalkieEnable',   menu = {'OFF', 'ON'}, 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.WalkieEnable; return MSG_DONE;
        case MSG_VALUE_SET: gNV.WalkieEnable = 	msg->Value.Signed;
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_WalkieVoltsON)  // Setting { name = 'WalkieVoltsON',  range= {0.1, 3.0, 0.1}, units='V', msghandler = 'OnSetting_WalkieVoltsON' }
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = 10 * gNV.WalkieVoltsON; return MSG_DONE;
        case MSG_VALUE_SET: gNV.WalkieVoltsON = (float)msg->Value.Signed*0.1;
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_WalkieVoltsOFF) // Setting { name = 'WalkieVoltsOFF', range= {0.1, 3.0, 0.1}, units='V', msghandler = 'OnSetting_WalkieVoltsOFF' }
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = 10 * gNV.WalkieVoltsOFF; return MSG_DONE;
        case MSG_VALUE_SET: gNV.WalkieVoltsOFF = (float)msg->Value.Signed * 0.1;
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'fact.batttery.cal',   range={-1000, 1000},  msghandler = 'OnSetting_FACT_BATT_CAL' }
#ifdef SIMULATE
void BurnMachineInfoStruct_REQ(){PopUp("Burned Machine Info",2);}
MsgHandler(OnSetting_FACT_BATT_CAL) {return 0;}
#else
MsgHandler(OnSetting_FACT_BATT_CAL) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:  msg->Value.Signed = gMachineInfo.BattCal;	return MSG_DONE;
        case MSG_VALUE_SET:  gMachineInfo.BattCal = msg->Value.Signed;  return MSG_DONE;
        default: break;
        }
    return 0;
}
#endif
MsgHandler(OnUIBatteryCal) 
{
    switch (MsgType(msg)) {

        case MSG_PAGE_DRAW:
            {
            CANVAS *c = UICanvas();
            int Percent,IsExternal,IsAlmostDead,IsReallyDead; 
            float Volts;
            char tmpstr[40]; 
            UI_GetBatteryPowerInfo(&Percent, &Volts, &IsExternal, &IsAlmostDead, &IsReallyDead, tmpstr);

            int x = LCD_WIDTH/2 - 45, y = 60;
            BatteryDraw(c, x, y, Percent, IsAlmostDead, tmpstr);

            y += 30; x = LCD_WIDTH/2;
            sprintf(tmpstr, "%2.2f VOLTS",Volts);
            y = CanvasStringCenter(c, x, y, tmpstr);
            y+=5;
            y = CanvasStringCenter(c, x, y, "(Press * to save)");
            break;  /* Allow further processing */
            }

        case MSG_KEY_PRESS:
            {
            if(MsgKey(msg) == KEY_STAR) 
               {                          
               BurnMachineInfoStruct_REQ(); // this will create a popup 
               return MSG_DONE;
               }
            break;   
            }

        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_AllowExtraShortcutMenus) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AllowExtraShortcutMenus; return MSG_DONE;
        case MSG_VALUE_SET: gNV.AllowExtraShortcutMenus = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

EXPORT bool UI_AllowExtraShortcutMenus()
{
	return gNV.AllowExtraShortcutMenus;	
}

//Setting { name = 'allow.home.page.HOMEPAGE_ENG'    , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_ENG' ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_ENG) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_ENG; uchar *g = &gNV.Allow_HOMEPAGE_ENG; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *n = *g = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_O6'     , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_O6'  ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_O6) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_O6; uchar *g = &gNV.Allow_HOMEPAGE_O6; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_CARD'   , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_CARD' ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_CARD) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_CARD; uchar *g = &gNV.Allow_HOMEPAGE_CARD; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_O2CARD' , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_O2CARD' }
MsgHandler(OnSetting_Allow_HOMEPAGE_O2CARD) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_O2CARD; uchar *g = &gNV.Allow_HOMEPAGE_O2CARD; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_I4P4'   , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_I4P4' ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_I4P4) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_I4P4; uchar *g = &gNV.Allow_HOMEPAGE_I4P4;  
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_O4R2'   , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_O4R2' ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_O4R2) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_O4R2; uchar *g = &gNV.Allow_HOMEPAGE_O4R2; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

//Setting { name = 'allow.home.page.HOMEPAGE_D12'    , menu = {'OFF', 'ON'}, msghandler = 'OnSetting_Allow_HOMEPAGE_D12'  ' }
MsgHandler(OnSetting_Allow_HOMEPAGE_D12) {	uchar *n = &Nomad.Settings.Allow_HOMEPAGE_D12; uchar *g = &gNV.Allow_HOMEPAGE_D12; 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = *g; return MSG_DONE;    
        case MSG_VALUE_SET: *g = *n = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_Bank0Color)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.Bank0Color; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.Bank0Color = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_IFBEncryptionMode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.IFB_EncryptMode; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.IFB_EncryptMode = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}


int GetFkeyHoldTimeMS() // 2500;  for uicontrols.cpp
{
	return gNV.FkeyHintTime;
}
MsgHandler(OnSetting_FkeyHintHoldTime)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.FkeyHintTime; return MSG_DONE;    
        case MSG_VALUE_SET: {
			int delta = (gNV.FkeyHintTime - msg->Value.Signed);
			gNV.FkeyHintTime = GUIdelta(msg->Value.Signed, gNV.FkeyHintTime, 100 ,500);
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
			}
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_BankKeysMode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned = gNV.BankKeysMode; return MSG_DONE;    
        case MSG_VALUE_SET: {
        	gNV.BankKeysMode = msg->Value.Unsigned;
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
			}
        default: break;
    }
    return 0;
}


int UI_GetBankKeysMode()
{
	return gNV.BankKeysMode;
}


MsgHandler(OnSetting_SinglePFLmode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned = gNV.SinglePFLmode; return MSG_DONE;    
        case MSG_VALUE_SET: {
        	gNV.SinglePFLmode = msg->Value.Unsigned;
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
			}
        default: break;
    }
    return 0;
}

EXPORT int UI_GetCurIntBankNum()
{
	return gIntBankStates.CurrentBank;
}


int UI_GetTransportMode()
{
	return GetTransportMode(); // enums are the same for the UI space
//  GET_STATE(p);
//  return p->Transport.State;
}

MsgHandler(OnSetting_PFLpostFader)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned = gNV.PFLpostFader; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.PFLpostFader = msg->Value.Unsigned; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}


/* not needed any more - was not even hooked up in the first place!!
MsgHandler(OnSetting_PlayToOutputs) //was gNV.MasterOutputmode
{
    switch (MsgType(msg)) { // NOTE THIS IS INVERTED LOGIC WRT UI.LUA
        case MSG_VALUE_GET:  msg->Value.Signed = !gNV.MuteOutputPlayback;  return MSG_DONE;
        case MSG_VALUE_SET:  gNV.MuteOutputPlayback = !msg->Value.Signed;  NVIsDirty(DEVAINI); return MSG_DONE;
        default: break;
    }
    return 0;
}
*/

static int Set_OutputMuteMode(MESSAGE *msg, int outNum)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned = gNV.OutputMuteMode[outNum]; return MSG_DONE;    
        case MSG_VALUE_SET: 
			gNV.OutputMuteMode[outNum] = msg->Value.Unsigned;
			fOutputMixDirty=1; // make OutputMixer() re-build its Mute array
			NVIsDirty(ODBCINI);
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_OutputMuteMode1){	return Set_OutputMuteMode(msg,1-1); }
MsgHandler(OnSetting_OutputMuteMode2){	return Set_OutputMuteMode(msg,2-1); }
MsgHandler(OnSetting_OutputMuteMode3){	return Set_OutputMuteMode(msg,3-1); }
MsgHandler(OnSetting_OutputMuteMode4){	return Set_OutputMuteMode(msg,4-1); }
MsgHandler(OnSetting_OutputMuteMode5){	return Set_OutputMuteMode(msg,5-1); }
MsgHandler(OnSetting_OutputMuteMode6){	return Set_OutputMuteMode(msg,6-1); }
MsgHandler(OnSetting_OutputMuteMode7){	return Set_OutputMuteMode(msg,7-1); }
MsgHandler(OnSetting_OutputMuteMode8){	return Set_OutputMuteMode(msg,8-1); }

MsgHandler(OnSetting_AllowExtKeyboard)
{ 
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Unsigned  = gNV.AllowExtKeyboard; return MSG_DONE;    
        case MSG_VALUE_SET: 
   			gNV.AllowExtKeyboard = msg->Value.Unsigned; // updated aysnchronously by Keyboard5Vmode_Handler()
   			NVIsDirty(DEVAINI);
			return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_DisableRxerPowerCycle) // allow user to turn off QRX212 power cycle mode when no rxers are isntalled
{
    switch (MsgType(msg)) { 
        case MSG_VALUE_GET:  msg->Value.Signed =  gNV.DisableRxerPowerCycle;  return MSG_DONE;
        case MSG_VALUE_SET:  
            gNV.DisableRxerPowerCycle =  msg->Value.Signed;   
   			NVIsDirty(DEVAINI);
            return MSG_DONE;
        default: break;
    }
    return 0;
}


uint32 GetTrack12MixInputMask(int Row, int col)
{
	uint32 mask=1;
	if(Row==0)	return (mask<<col); // ---RXer inputs---
	mask <<= 8;
	if(Row==1)	return (mask<<col); // ---analog inputs---
	mask <<= 8;
	if(Row==2)	return (mask<<col);  // ---return inputs---
	mask <<= 8;
	if(Row==3)	return (mask<<col);  // ---digital inputs---
	brkpt(Row,"Trk12mixMask");
	return 0; // ERROR
}

MsgHandler(OnSetting_TrackXMixMatrixInputs) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_A, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_A, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TrackXMixMatrixDigitalInputs) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_D, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_D, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TrackXMixMatrixOutputs) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_OUTPUTS, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_OUTPUTS, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TrackXMixMatrixSCT) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_SCT, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACKX_MIX_SCT, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}


bool GetRawFkeyState(int fkeyIndex);
MsgHandler(OnUITrackXMixEncoder)
{
    switch (MsgType(msg)) {

        case MSG_MATRIX_CURSOR:            
//		case MSG_ENCODER:
			{
			if(GetRawFkeyState(0))
				{ // detect F6 key being held to scroll track numbers using encoder
				int direction = msg->Value.Signed ? 1 : -1;
				Nomad.Settings.CurTrackMixPageNum = clip(Nomad.Settings.CurTrackMixPageNum+direction, 0, MAX_DISK_CHANNELS-1);
                UIRefresh();
				return MSG_DONE; // show we used this encoder message
				}
			break;
			}
/*      case MSG_PAGE_DRAW:
            {
            CANVAS *c = UICanvas();
            CanvasLine(c, 000,135, LCD_WIDTH-2,135, COLOR_GRAY);  
            }
        case MSG_KEY_PRESS:
        	if(MsgKey(msg) == KEY_FKEY3) {              // '>>':
				UIPageGoto(&UITrack2MixEnables);
		        return MSG_DONE;
				}
			break;
*/
        }
    return 0;
}

/*
MsgHandler(OnSetting_Track2MixMatrixInputs) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_A, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_A, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_Track2MixMatrixOutputs) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_OUTPUTS, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_OUTPUTS, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_Track2MixMatrixSCT) 
{
    switch (MsgType(msg)) {
        case MSG_MATRIX_CELL_GET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_SCT, GET_IT); return MSG_DONE;
        case MSG_MATRIX_CELL_SET: GetSetTranslatedMixCrosspoint((MATRIX_CELL*)MsgPtr(msg), MENU_TRACK2_MIX_SCT, SET_IT); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnUITrack2MixEnables)
{
    switch (MsgType(msg)) {
        case MSG_PAGE_DRAW: {
            CANVAS *c = UICanvas();
            CanvasLine(c, 000,135, LCD_WIDTH-2,135, COLOR_GRAY);  
			break;
            }
        case MSG_KEY_PRESS:
        	if(MsgKey(msg) == KEY_FKEY2) {              // '<<':
				UIPageGoto(&UITrack1MixEnables);
		        return MSG_DONE;
				}
			break;
        }
    return 0;
}
*/
				

MsgHandler(OnSettingSoundReport_Production)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.Production);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.Production,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSettingSoundReport_Studio)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.Studio);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.Studio,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSettingSoundReport_Director)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.Director);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.Director,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSettingSoundReport_SoundMixer)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.SoundMixer);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.SoundMixer,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSettingSoundReport_Tel)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.Tel);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.Tel,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSettingSoundReport_Email)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.Email);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.Email,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             

MsgHandler(OnSettingSoundReport_ProductionUnit)
{
    switch (MsgType(msg)) {
        case MSG_TEXT_GET: strcpy(MsgText(msg), Nomad.SoundReport.ProductionUnit);  return MSG_DONE;
        case MSG_TEXT_SET: strcpy(Nomad.SoundReport.ProductionUnit,  MsgText(msg)); NVIsDirty(MIRRORINI); return MSG_DONE;
        default: break;
    }
    return 0;
}						             


MsgHandler(OnSetting_ShowDisabledTrackMeters)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.ShowDisabledTrackMeters; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.ShowDisabledTrackMeters = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_UseOldTrackMixPage) 
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	
        	msg->Value.Signed = Nomad.Settings.UseOldTrackMixPage; 
        	return MSG_DONE;    
        case MSG_VALUE_SET: 
        	Nomad.Settings.UseOldTrackMixPage = msg->Value.Signed; 
        	gNV.UseOldTrackMixPage            = msg->Value.Signed; 
        	NVIsDirty(DEVAINI); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_ReRecStrategy)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.ReRecStrategy; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.ReRecStrategy = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}


EXPORT int GetNumAESInputs()
{
	if(gIsNova2)
		return 8;
	else
		return 4;
}

EXPORT bool IsNova2()
{
	return gIsNova2; // Nomad.Settings.IsNova2;
}

EXPORT bool GetIOcfgChannelActiveStatus(int ch)
{
	if( !IsNova2() && InRange(ch, IN_CHAN_DIGITAL_5, IN_CHAN_DIGITAL_8) )
		return 0; // dig 4-6 is not supported on Nova1
	return 1;
}

// convert GUI mapping to Nova (need to skip Nova's extra dummy channels)
uint16 ConvertPCchanToNova(uint16 chan)
{
	if(chan >= 6)
		chan += 10; // convert GUI mapping to Nova (skip Nova's dummy channels)
	if(chan>=MAX_INPUT_SOURCES) 
		chan=0; // OUCH!
	//Nomad.Setup.Chan = msg.Value.Unsigned;  
	return chan;	
}

uint16 ConvertNovaChanToPC(uint16 chan)
{
	if(chan >= 16)
		chan -= 10; // convert GUI mapping to Nova (skip Nova's dummy channels)
	if(chan>=MAX_INPUT_SOURCES) 
		chan=0; // OUCH!
	//Nomad.Setup.Chan = msg.Value.Unsigned;  
	return chan;	
}

MsgHandler(OnSetting_AlwaysListenToReturns)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.AlwaysListenToReturns; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.AlwaysListenToReturns = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}


void SetOutputDelayUI(int ch, uint16 val)
{	// in deva.h: #define MAX_OUTPUT_DELAY_SAMPLES 32768 // 32k/48 = 682ms
	gOutputChanConfig[ch].delay = clip(val, 0, MAX_OUTPUT_DELAY_SAMPLES/48); 
	//	if(context != gPCcontext)
	//		SendSetting_1D_indexed(SETTING_OUTPUT_DELAY, ConvertNovaChanToPC(ch), msg->Value.Unsigned, 18); 
	NVIsDirty(ODBCINI);  
}
MsgHandler(OnSettingOutputDelay_1){ int ch=1-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_2){ int ch=2-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_3){ int ch=3-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_4){ int ch=4-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_5){ int ch=5-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_6){ int ch=6-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_7){ int ch=7-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}
MsgHandler(OnSettingOutputDelay_8){ int ch=8-1;
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gOutputChanConfig[ch].delay;  return MSG_DONE;
        case MSG_VALUE_SET:	SetOutputDelayUI(ch, msg->Value.Unsigned);       	return MSG_DONE;
        default: break;
    }
    return 0;
}



  
 //WARNING 
 //To make th FP7 have its own fader assign table is a big pain
 //You will need to look everywhere GetFaderAssPtr() is called and also do a GetFaderAssPtr_FP7() somehow

int GetSetExtFad_FP7(int ExtFaderBase, MESSAGE *msg, void *context);
MsgHandler(OnSetting_ExtFad_FP7_1) {	return GetSetExtFad_FP7(1-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_2) {	return GetSetExtFad_FP7(2-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_3) {	return GetSetExtFad_FP7(3-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_4) {	return GetSetExtFad_FP7(4-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_5) {	return GetSetExtFad_FP7(5-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_6) {	return GetSetExtFad_FP7(6-1, msg, context); }
MsgHandler(OnSetting_ExtFad_FP7_7) {	return GetSetExtFad_FP7(7-1, msg, context); }


//---- these are Nova's GUI enmus that ui.lua uses as fader assigns in the EXT fader page
//                     0   'OFF',
#define FP7_A_FADER_START   1                    //         'A1+Trim', 'A2+Trim', 'A3+Trim', 'A4+Trim', 'R1+Trim', 'R2+Trim',
#define FP7_D_FADER_START  (FP7_A_FADER_START+6) // 7  //   'D1+Trim', 'D2+Trim', 'D3+Trim', 'D4+Trim', 'D5+Trim', 'D6+Trim', 'D7+Trim', 'D8+Trim',
#define FP7_RX_FADER_START (FP7_D_FADER_START+8) //  (11+4) 'RX1+Trim','RX2+Trim','RX3+Trim','RX4+Trim','RX5+Trim','RX6+Trim','RX7+Trim','RX8+Trim',
					   
#define FP7_A_TRIM_START  (FP7_RX_FADER_START+8)// 'A1+Trim', 'A2+Trim', 'A3+Trim', 'A4+Trim', 'R1+Trim', 'R2+Trim',
#define FP7_D_TRIM_START  (FP7_A_TRIM_START+6)  // 'D1+Trim', 'D2+Trim', 'D3+Trim', 'D4+Trim', 'D5+Trim', 'D6+Trim', 'D7+Trim', 'D8+Trim',
#define FP7_RX_TRIM_START (FP7_D_TRIM_START+8)  // 'RX1+Trim','RX2+Trim','RX3+Trim','RX4+Trim','RX5+Trim','RX6+Trim','RX7+Trim','RX8+Trim',
					   
#define FP7_A_ZNET_START  (FP7_RX_TRIM_START+8) // (19+4)   //   'A1+ZNET', 'A2+ZNET', 'A3+ZNET', 'A4+ZNET', 'R1+ZNET', 'R2+ZNET',
#define FP7_D_ZNET_START  (FP7_A_ZNET_START+6)  // (25+4)   //   'D1+ZNET', 'D2+ZNET', 'D3+ZNET', 'D4+ZNET', 'D5+ZNET', 'D6+ZNET', 'D7+ZNET', 'D8+ZNET',
#define FP7_RX_ZNET_START (FP7_D_ZNET_START+8)  // (29+4+4) //   'RX1+ZNET','RX2+ZNET','RX3+ZNET','RX4+ZNET','RX5+ZNET','RX6+ZNET','RX7+ZNET','RX8+ZNET',
#define FP7_ERR_START     (FP7_RX_ZNET_START+8)

int CalcInputFromUIExtFaderAssEnum_FP7(int uiEnum) // returns channel index and -1 if OFF
{
	if(uiEnum == 0)
		return -1;

	// shift enum down to the FADER group (1-15)
	if(      InRange(uiEnum, FP7_A_TRIM_START,  FP7_A_ZNET_START-1) ) uiEnum -= (FP7_A_TRIM_START-FP7_A_FADER_START); // get it down to lower group
	else if( InRange(uiEnum, FP7_A_ZNET_START,  FP7_ERR_START-1   ) ) uiEnum -= (FP7_A_ZNET_START-FP7_A_FADER_START); // get it down to lower group

	if(uiEnum >= FP7_RX_FADER_START)		return uiEnum - FP7_RX_FADER_START + IN_CHAN_DIGITAL_RX_1;
	if(uiEnum >=  FP7_D_FADER_START)		return uiEnum -  FP7_D_FADER_START + IN_CHAN_DIGITAL_1;
	return uiEnum - FP7_A_FADER_START;
}

// msg->Value.Unsigned is a long list from Nomad GUI
EXPORT int GetSetExtFad_FP7(int ExtFaderBase, MESSAGE *msg, void *context) //
{
	volatile int in,tmode, faderbase = ExtFaderBase; // this is just 0-7 
	volatile int FaderNum = faderbase + FP7_FADER_1;	    
	if( !InRange(FaderNum, FP7_FADER_1, FP7_FADER_14) )	
		return 0;
	if( !InRange(ExtFaderBase, 0, NUM_FP7_KNOBS-1) )
		return 0;
	struct FADER_ASS *p = GetFaderAssPtr(FaderNum); // short hand

    switch  (MsgType(msg)) 
    { 
//        case MSG_TEXT_GET:  {
//        	zsnprintf(MsgText(msg), MAX_UI_TEXT, "F%d %d %d", ExtFaderBase, p->type, p->ch[0]);
//        	return MSG_DONE; 
//        	}
        case MSG_VALUE_GET: 
        	{
			msg->Value.Unsigned = 0;
			if(p->type != FADERTYPE_DISABLED) 
				{
				int ch = p->ch[0],  offset = 0;
				if( IsGoodIndex(ch, numberof(gChannelConfig)) )
					{
					if      (p->type == FADERTYPE_ZNET)		offset = FP7_A_ZNET_START - 1;  // offset into ZNET trim mode LUT 
					else if (p->type == FADERTYPE_TRIM) 	offset = FP7_A_TRIM_START - 1;  // offset into TRIM trim mode LUT 
					else if (p->type == FADERTYPE_FADER) 	offset = FP7_A_FADER_START - 1; // offset into FADER mode LUT 
					}					
				if(     InRange(ch, IN_CHAN_ANALOG_1,   IN_CHAN_RETURN_2)) 		msg->Value.Unsigned = FP7_A_FADER_START  + offset + ch; // return A1-R2
				else if(InRange(ch, IN_CHAN_DIGITAL_1,  IN_CHAN_DIGITAL_8)) 	msg->Value.Unsigned = FP7_D_FADER_START  + offset + ch-IN_CHAN_DIGITAL_1;    // return D1-D4
				else if(InRange(ch, IN_CHAN_DIGITAL_RX_1,IN_CHAN_DIGITAL_RX_8))	msg->Value.Unsigned = FP7_RX_FADER_START + offset + ch-IN_CHAN_DIGITAL_RX_1; // return RX1-RX8
				else msg->Value.Unsigned = 0; //  ERROR!
				}
        	return MSG_DONE; 
        	}

        case MSG_VALUE_SET: 
       		{
			// PCGUI needs to have its enums updated to handle this flexability - or we could limit us to the single fader old-style
			if(msg->Value.Unsigned >= FP7_ERR_START)
				return MSG_DONE; // clip at edges

			msg->Value.Unsigned = clip(msg->Value.Unsigned, 0, FP7_ERR_START);
			int ch = clip(CalcInputFromUIExtFaderAssEnum_FP7(msg->Value.Unsigned), 0, numberof(gChannelConfig));
			dprintf("FP7: ch=%d ValUns=%d\n", ch, msg->Value.Unsigned);

			if( (p->type!=FADERTYPE_DISABLED) && !IsNova2() ) 
				{ // limit what channels used can choose from (skip Dig 5-8)
				int CurCh = p->ch[0]; // cet current channel
				if( (CurCh == IN_CHAN_DIGITAL_4) && (ch == IN_CHAN_DIGITAL_5) )
					ch = IN_CHAN_DIGITAL_RX_1; // skip up to RX1
				if( (CurCh == IN_CHAN_DIGITAL_RX_1) && (ch == IN_CHAN_DIGITAL_8) )
					ch = IN_CHAN_DIGITAL_4;    // skip down to 4
				}
			// start searching the highest enums first
			if     (msg->Value.Unsigned>=FP7_RX_ZNET_START)	Nomad_SetFaderAssign(FADERTYPE_ZNET,  ch, FaderNum);  // Z RX1-8
			else if(msg->Value.Unsigned>=FP7_D_ZNET_START)	Nomad_SetFaderAssign(FADERTYPE_ZNET,  ch, FaderNum);  // Z D1-8
			else if(msg->Value.Unsigned>=FP7_A_ZNET_START)	Nomad_SetFaderAssign(FADERTYPE_ZNET,  ch, FaderNum);  // Z A1-6
			else if(msg->Value.Unsigned>=FP7_RX_TRIM_START)	Nomad_SetFaderAssign(FADERTYPE_TRIM,  ch, FaderNum);  // T RX1-8
			else if(msg->Value.Unsigned>=FP7_D_TRIM_START)	Nomad_SetFaderAssign(FADERTYPE_TRIM,  ch, FaderNum);  // T D1-8
			else if(msg->Value.Unsigned>=FP7_A_TRIM_START) 	Nomad_SetFaderAssign(FADERTYPE_TRIM,  ch, FaderNum);  // T A1-6
			else if(msg->Value.Unsigned>=FP7_RX_FADER_START)Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);  // F RX1-8
			else if(msg->Value.Unsigned>=FP7_D_FADER_START)	Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);  // F D1-8
			else if(msg->Value.Unsigned>=FP7_A_FADER_START) Nomad_SetFaderAssign(FADERTYPE_FADER, ch, FaderNum);  // F A1-6
        	else 											Nomad_SetFaderAssign(FADERTYPE_DISABLED, ch, FaderNum);

   			SetNVTrim(ch, TrimRecallTable[ch]); // restore the current trim setting that we just stepped on

			// if we are setting to trim mode, restore the trim that we just previously destroyed
			//volatile int inputnum = ftype - A_TRIM_START;
			//if( InRange(msg->Value.Unsigned, A_TRIM_START, RX_TRIM_START+8) ) //  (inputnum>=IN_CHAN_ANALOG_1) && (inputnum<=IN_CHAN_DIGITAL_RX_1) )
    		//	SetNVTrim(ch, TrimRecallTable[ch]); // restore the current trim setting that we just stepped on

			NVIsDirty(MIXINI); // if we turn something off we still need to save it!
			NVIsDirty(AICINI); // might as well save fader positions in gChannelConfig now too right?
        	return MSG_DONE; 
        	} 
        default: break;
    }
    return 0;

}

MsgHandler(OnSetting_HasHiRezPotChip)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET: msg->Value.Unsigned = gMachineInfo.HasHiRezPotChip;  return MSG_DONE;
        case MSG_VALUE_SET:	gMachineInfo.HasHiRezPotChip = msg->Value.Unsigned;  return MSG_DONE;
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_ColorBlindMode)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.ColorBlindMode; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.ColorBlindMode = msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_IncTakeOnSceneChange) // this setting is inverted in GUI
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = !gNV.DontIncTakeOnSceneChange; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.DontIncTakeOnSceneChange = !msg->Value.Signed; NVIsDirty(DEVAINI); return MSG_DONE; 
        default: break;
    }
    return 0;
}
MsgHandler(OnSetting_FlipPowerAB) // this is a more permenant factory setting
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gMachineInfo.FlipPowerAB; return MSG_DONE;    
        case MSG_VALUE_SET: 
        	gMachineInfo.FlipPowerAB = msg->Value.Signed; 
        	BurnMachineInfoStruct_REQ(); 
        	return MSG_DONE; 
        default: break;
    }
    return 0;
}

int GetChannelConfig_TXID(int InChannelIndex)
{
	if(IsGoodIndex(InChannelIndex, numberof(gChannelConfig)))
		return  gChannelConfig[InChannelIndex].TxID;
	else 
		return 0; // error
}

MsgHandler(OnSetting_SDcardBoardInstalled)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gMachineInfo.SDcardBoardInstalled; return MSG_DONE;    
        case MSG_VALUE_SET: gMachineInfo.SDcardBoardInstalled = msg->Value.Signed; return MSG_DONE; 
        default: break;
    }
    return 0;
}

bool Get_GNV_PreventJapanFreqs() { return gNV.PreventJapanFreqs; }
MsgHandler(OnSetting_PreventJapanFreqs)  // Setting { name = 'PreventJapanFreqs', menu = {'OFF', 'ON'},  msghandler = 'OnSetting_PreventJapanFreqs' }
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.PreventJapanFreqs; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.PreventJapanFreqs = msg->Value.Signed; NVIsDirty(DEVAINI);  return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_TrimsCanMute)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.TrimsCanMute; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.TrimsCanMute = msg->Value.Signed; NVIsDirty(DEVAINI);  return MSG_DONE; 
        default: break;
    }
    return 0;
}

MsgHandler(OnSetting_MixTrackSeparator)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.MixTrackSeparator; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.MixTrackSeparator = msg->Value.Signed; NVIsDirty(DEVAINI);  return MSG_DONE; 
        default: break;
    }
    return 0;
}


MsgHandler(OnSetting_SendFrontFaders)
{
    switch (MsgType(msg)) {
        case MSG_VALUE_GET:	msg->Value.Signed = gNV.SendFrontFaders; return MSG_DONE;    
        case MSG_VALUE_SET: gNV.SendFrontFaders = msg->Value.Signed; NVIsDirty(DEVAINI);  return MSG_DONE; 
        default: break;
    }
    return 0;
}
