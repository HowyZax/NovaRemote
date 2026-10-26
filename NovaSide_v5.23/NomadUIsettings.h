#ifndef UInt8
typedef unsigned char  UInt8;
#endif
 
void  NomadInit(void);
UInt8 NomadTrackCount(void);
void  NomadSetTrackCount(UInt8 ntracks);
void  UI_UIsetupRefreshRequest();

//UInt8 PFLGetChanMask(void);
//void PFLToggleChan(UInt8 chan);
//void PFLToggle(void);
//void PFLSetActive(bool state);
//bool PFLIsActive(void);
//void PFLSetChan(UInt8 chan, bool On);
//void PFLClearAll(int ext);
//int PFLGetBusListItem_TrackNum(unsigned idex);

void TransportSetState(UInt8 state);
UInt8 TransportState(void);
void HPHMemIncDec(bool inc, bool skipEmpty, void *context);
void HPHMemIncDec(bool inc,bool skipEmpty);
int  HPHMemIndex(void);
//void SetHPmem(int index);
void SetHPmem(int index, void *context=NULL);
int  GetHPmemIndex();
void SetHPmemIndex(int val);
//HPHMEM *HPHMEMCurrent(void);

void UI_PreRecordTime(int x);
int UI_PreRecordTime();
void UI_PreRecordTimeChanged();
void MetersConfigure(int style);
void MetersConfigureAutoTrim(int chan);
void MetersConfigureChanX(int chan);

void SendTrackNamesToUI();


int  Get_UIHomePageVariant();   // these are located in uihome.cpp so I can make UIHomePageVariant non-volatile
void Set_UIHomePageVariant(unsigned x); // these prolly should be implemented in a different way to be consistant
void UpdateNomadStruct(void);


extern const unsigned IOConfigTbl[];

void PopUp_Generic(char *s0, char *s1, char *s2, char *s3, int spacing, int timeoutMS);
void PopUp_Generic(char *s0, char *s1, char *s2, char *s3, int spacing, int timeoutMS, void *context);

void DetectLatchedUpInputChannel();

void UI_SetCurrentMetaDataSeg(int x);

int UI_GetNomadSetupChan();
int UI_GetNomadMaxMeters();
void RefreshAllLiteDiskChans();

void UI_ToggleMP3on();
int UI_GetMP3String(char *text);
int GetBusIndexForPeakDetect(int meternum);

int UI_GetNomad_N_Meters();

void TrackPresetChangeMonitor(); // WE NEED TO SEND NV NAMES TO NOMAD ON BOOT UP!
void SendTrackPresetsToUI();
void NotePresetChangeMonitor(); // WE NEED TO SEND NV NAMES TO NOMAD ON BOOT UP!
void SendNotePresetsToUI();

int SetExtFad(int FaderNum, int RawUserInputNum, void *context); // for PCGUI calls

int  FindPlaybackHPpreset();
void LoadHPpreset(int index); // (not sticky!)   gNV.HeadphoneMixPresetNum
void TryToLoadPlayPreset();

#define HP_LEFT  0 // for setting crosspoints 
#define HP_RIGHT 1

// ' ', 'A', 'D', 'T', 'Z' from ui.lua
#define UI_FADER_NONE    0
#define UI_FADER_ANALOG  1
#define UI_FADER_DIGITAL 2
#define UI_FADER_TRIM    3
#define UI_FADER_ZAXNET  4

// common order for UI cell contents
#define MCELL_OFF    0
#define MCELL_ON     1
#define MCELL_ON_INV 2
#define MCELL_PF	 3
#define MCELL_PF_INV 4

enum { // codes to allow the following routine to know how to interpret the page's data
		MENU_CARD_MIX_A,   MENU_CARD_MIX_D,  MENU_CARD_MIX_SCT, MENU_CARD_MIX_O14,  MENU_CARD_MIX_RX,
		MENU_OUT_MIX_A,    MENU_OUT_MIX_D,   MENU_OUT_MIX_SCT,  MENU_OUT_MIX56, 	 MENU_OUT_MIX_RX, 
		MENU_TRACKX_MIX_A, MENU_TRACKX_MIX_D, MENU_TRACKX_MIX_OUTPUTS, MENU_TRACKX_MIX_SCT,
		MENU_CARD_MIX_TRACKS // new
};

/*
// this is for both ui.lua and Oasis app
#define GUI_SRATE_44100 0
#define GUI_SRATE_47952 1
#define GUI_SRATE_48000 2
#define GUI_SRATE_48048 3
#define GUI_SRATE_88200 4
#define GUI_SRATE_96000 5
*/

extern void *gPCcontext; //  = (void *)0x2; // make sure this is a unique, invalid pointer value (points to Nullporinter test memoory area)               
#define GET_STATE(ptr)  NOMAD_STATE *ptr = &Nomad

#define POP_DEFAULT 5000  // default popup timeout
#define POP_FOREVER 60000 // 16bit limit on zlane cPopup cmd!!
void PopUp_Generic(const char *s0, const char *s1, const char *s2, const char *s3, int spacing, int timeoutMS);
void PopUp_Generic(const char *s0, const char *s1, const char *s2, const char *s3, int spacing, int timeoutMS, void *context);
void PopUp(const char *s);
void PopUp(const char *s, int timeoutMS);

void PopUp_MirrorConflict();
void PopUp_RecordConflict();
void PopUp_Generic(char *s0, char *s1, char *s2, char *s3, int spacing, int timeoutMS);
void PopUp_GenericNoPC(char *s0, char *s1, char *s2, char *s3, int spacing, int timeoutMS);

void PlaybackHPpresetHandler();
bool IsDemoUnit();

void TryToLoad_HP_BankPreset(int bankIndex);

uint16 ConvertPChpEnumToHPnibble(uint16 PCenum);

// for PC/LUA to Nova conversion
float ConvertDelay_Point1ms_To_Samples(uint32 point1ms); 
int32 ConvertDelay_Samples_To_Point1ms(uint32 samples);

// convert PCGUI input mapping to Nova (need to skip Nova's extra dummy channels)
uint16 ConvertPCchanToNova(uint16 chan);
uint16 ConvertNovaChanToPC(uint16 chan);



