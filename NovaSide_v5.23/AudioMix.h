#pragma once

#define SINGLE_FADER_MULT  // define this to get rid of wasteful fader chunk multip;ly (use single scaler instead) v8.82

//enum {LEFT,RIGHT};
#define MAX_ANALOG_INPUT_TRIM	(30.0)
#define MIN_ANALOG_INPUT_TRIM  (-20.0)

#define NUM_FADER_BANKS      5
//#define NUM_VIRTUAL_FADERS 0 // 4
#define NUM_INTERNAL_FADERS  5 
#define NUM_OASIS_FADERS     16
#define NUM_FP7_KNOBS        14
#define NUM_TOTAL_FADERS     (NUM_INTERNAL_FADERS+NUM_OASIS_FADERS+NUM_FP7_KNOBS) // howy added one for headphone cuz it looks like other people have the headphone included too

#define INTERNAL_FADER_1   0 // front panel knobs
#define INTERNAL_FADER_2   1 
#define INTERNAL_FADER_3   2 
#define INTERNAL_FADER_4   3 
#define INTERNAL_FADER_5   4 

#define	EXTERNAL_FADER_1  5   // Oasis faders
#define EXTERNAL_FADER_2  6  
#define EXTERNAL_FADER_3  7  
#define EXTERNAL_FADER_4  8  
#define EXTERNAL_FADER_5  9  
#define EXTERNAL_FADER_6  10  
#define EXTERNAL_FADER_7  11  
#define EXTERNAL_FADER_8  12  
#define	EXTERNAL_FADER_9  13
#define	EXTERNAL_FADER_10 14 
#define	EXTERNAL_FADER_11 15  
#define	EXTERNAL_FADER_12 16 
#define	EXTERNAL_FADER_13 17 
#define	EXTERNAL_FADER_14 18 
#define	EXTERNAL_FADER_15 19 
#define	EXTERNAL_FADER_16 20

#define	FP7_FADER_1  21 // 29  // (OUTPUT_FADER_8+1 ) // FP7 knobs (assume two FP7s can be connected for 14 faders
#define FP7_FADER_2  22 // 30  // (OUTPUT_FADER_8+2 )
#define FP7_FADER_3  23 // 31  // (OUTPUT_FADER_8+3 )
#define FP7_FADER_4  24 // 32  // (OUTPUT_FADER_8+4 )
#define FP7_FADER_5  25 // 33  // (OUTPUT_FADER_8+5 )
#define FP7_FADER_6  26 // 34  // (OUTPUT_FADER_8+6 )
#define FP7_FADER_7  27 // 35  // (OUTPUT_FADER_8+7 )
#define FP7_FADER_8  28 // 36  // (OUTPUT_FADER_8+8 )
#define	FP7_FADER_9  29 // 37  // (OUTPUT_FADER_8+9 )
#define	FP7_FADER_10 30 // 38  // (OUTPUT_FADER_8+10)
#define	FP7_FADER_11 31 // 39  // (OUTPUT_FADER_8+11)
#define	FP7_FADER_12 32 // 40  // (OUTPUT_FADER_8+12)
#define	FP7_FADER_13 33 // 41  // (OUTPUT_FADER_8+13)
#define	FP7_FADER_14 34 // 42  // (OUTPUT_FADER_8+14)
//#define FP7_FADER_15 //(OUTPUT_FADER_8+15)
//#define FP7_FADER_16 //(OUTPUT_FADER_8+16)

#define MAX_NUM_OUTPUT_KNOBS  8
/*
#define OUTPUT_FADER_1  0   // 21 //    // Oasis output knobs
#define OUTPUT_FADER_2  1   // 22 //  
#define OUTPUT_FADER_3  2   // 23 //  
#define OUTPUT_FADER_4  3   // 24 //  
#define OUTPUT_FADER_5  4   // 25 //   // these are for the Oasis and Deva32!!!!!!
#define OUTPUT_FADER_6  5   // 26 //  
#define OUTPUT_FADER_7  6   // 27 // 
#define OUTPUT_FADER_8  7   // 28 // 
*/
/***********************************************************
*                                                          *
* PRIMARY AUDIO BUS LAYOUT                                 *
* -------------------------------------------------------  *
*                                                          *
* Pre-Fader Analog inputs (1-8)                            *
* Pre-Fader Digital inputs (1-8)                           *
* Analog Input (1-8)                                       *
* Digital Input (1-8)                                      *
* Slate input                                              *
* Tone                                                     *
* Talk1                                                    *
* Talk2                                                    *
* Camera input                                             *
* Disk mix bus (1-16)                                      *
* Output mix bus (1-10)                                    *
* Headphone mix left/right                                 *
* Play camera                                              *
* Play bus (1-16)                                          *
* Play out bus (1-10)                                      *
* Analog inputs (1-8)                                      *
* Digital inputs (1-8)                                     *
* Mute                                                     *
*                                                          *
***********************************************************/

// Bus indexes (used to abstractly specify a bus)

#define BUS_PF_ANALOG  0x0000
#define BUS_PF_DIGITAL 0x0100
#define BUS_ANALOG     0x0200
#define BUS_DIGITAL    0x0300
#define BUS_DISK_MIX   0x0400
#define BUS_OUTPUT_MIX 0x0500
#define BUS_PLAY       0x0600
#define BUS_ANALOG_IN  0x0700
#define BUS_ANALOG_OUT 0x0800
#define BUS_MUTE       0x0900

#define BUS_MASK  0x0FF00

//#define MAX_PROXY_INPUTS  6
//#define MAX_RETURN_INPUTS 4

// THESE NEED TO BE FIXED!
#define MAX_OUTPUTS			 8  // 6  increased to 8 for Nova2
#define MAX_DISK_CHANNELS    16 // 12  THERE ARE A FEW "16"s that are max array assumptions in the code so if we go over 16 we have to fix some loops

#define MAX_PROXY_INPUTS     4  // 12
#define MAX_RETURN_INPUTS    2
#define MAX_DUMMYA_INPUTS    (6+4)  // added 4 in Nova2:  how much to get us to 12 analog inputs to match didgtal inputs due to Greg's a vs D overlap bus scheme
#define MAX_AES_INPUTS       8
#define MAX_RX_INPUTS        8
#define MAX_DIGITAL_INPUTS   (MAX_AES_INPUTS     + MAX_RX_INPUTS) // (12+4) // added 4 in Nova2  // 4+4 AESinputs, 8 RXers,
#define MAX_ANALOG_INPUTS    (MAX_PROXY_INPUTS   + MAX_RETURN_INPUTS+MAX_DUMMYA_INPUTS)   // 12 
#define MAX_INPUTS           (MAX_DIGITAL_INPUTS + MAX_ANALOG_INPUTS) // not including slate,talks and tone
//#define MAX_UI_OUTPUTS     MAX_OUTPUTS    

typedef struct {
 SAMPLE PF_ANALOG[MAX_PROXY_INPUTS][CHUNK]; // (0                    + 0                ) //  0 
 SAMPLE PF_RETURN[MAX_RETURN_INPUTS][CHUNK];// (PF_ANALOG_BUS_INDEX  + MAX_PROXY_INPUTS ) // 12 was  6
 SAMPLE PF_DUMMYA[MAX_DUMMYA_INPUTS][CHUNK];// 6+4 dummy inputs to make sure analog lines up with digital inputs
 SAMPLE PF_AES[MAX_AES_INPUTS][CHUNK];      // (PF_RETURN_BUS_INDEX  + MAX_RETURN_INPUTS) // 16 was 10
 SAMPLE PF_RX[MAX_RX_INPUTS][CHUNK];        // (PF_RETURN_BUS_INDEX  + MAX_RETURN_INPUTS) // 16 was 10
 
 SAMPLE ANALOG[MAX_PROXY_INPUTS][CHUNK];    // (PF_DIGITAL_BUS_INDEX + MAX_DIGITAL_INPUTS) // 40 was 20  (post fader analog and returns)
 SAMPLE RETURN[MAX_RETURN_INPUTS][CHUNK];   // (ANALOG_BUS_INDEX     + MAX_PROXY_INPUTS  ) // 26
 SAMPLE DUMMYA[MAX_DUMMYA_INPUTS][CHUNK];   // 
 SAMPLE AES[MAX_AES_INPUTS][CHUNK];     	// 
 SAMPLE RX[MAX_RX_INPUTS][CHUNK];   	// 
 
 SAMPLE SLATE[1][CHUNK];                      // (DIGITAL_BUS_INDEX    + MAX_DIGITAL_INPUTS) //  was 40 
 SAMPLE TALK1[1][CHUNK];      // stupid but disk mix uses these mix sources
 SAMPLE TALK2[1][CHUNK];      // stupid but disk mix uses these mix sources
 SAMPLE TONE[1][CHUNK];                       // (SLATE_INDEX+1) // 
 
 SAMPLE OUTPUTMIX[MAX_OUTPUTS][CHUNK];                  // (TALK2_INDEX+1) 
// SAMPLE HEADPHONE2[2][CHUNK];                 // (OUTPUTMIX_BUS_INDEX + 6)   //****ALIAS*** 
// SAMPLE TAPEMONO[2][CHUNK];                   // (OUTPUTMIX_BUS_INDEX + 8)   //****ALIAS*** 
 SAMPLE HEADPHONE[2][CHUNK];                  // (OUTPUTMIX_BUS_INDEX + (6+4)) // 
 SAMPLE DISKMIX[MAX_DISK_CHANNELS][CHUNK];    // (PROXY_IN_BUS_INDEX + MAX_PROXY_INPUTS) 
// SAMPLE PROXY[MAX_PROXY_INPUTS][CHUNK];       // (HEADPHONE_BUS_INDEX+2)   // should this be 10 for future expansion?   
 SAMPLE PLAY[MAX_DISK_CHANNELS][CHUNK];       // (DISKMIX_BUS_INDEX  + MAX_DISK_CHANNELS) 

 SAMPLE ANALOG_IN[MAX_PROXY_INPUTS+MAX_RETURN_INPUTS][CHUNK];  // was MAX_ANALOG_INPUTS which includes DUMMY_INPUTS   
 SAMPLE AES_IN[MAX_AES_INPUTS][CHUNK];     
 SAMPLE RX_IN[MAX_RX_INPUTS][CHUNK];     
 SAMPLE PROXY[MAX_PROXY_INPUTS][CHUNK];     
 SAMPLE IFB_RX[1][CHUNK];
 SAMPLE AUTOMIX[1][CHUNK];
 SAMPLE RAW_SLATE_INT[CHUNK]; // for accurate meters
 SAMPLE RAW_SLATE_EXT[CHUNK];
 SAMPLE MUTE[1][CHUNK];   
} BUS_STRUCT;
extern BUS_STRUCT *bus;
  


// Bus related defines...
// Indexes into the Bus structure in AudioBufs.cpp
// We should really build these indexes using the respective 
// # defs for the bus width i.e. MAX_INPUT_CHANNELS, MAX_DISK_CHANNELS, etc.
// However this would really expand the # defs.
//
//   ------------ NOMAD BUSSES ----------------------
//                            ---- INPUT BUSES ---- MIX BUSES - MISC    
#define PF_ANALOG_BUS_INDEX   (0)
#define PF_RETURN_BUS_INDEX   (PF_ANALOG_BUS_INDEX  + MAX_PROXY_INPUTS)   // 
#define PF_DUMMYA_BUS_INDEX   (PF_RETURN_BUS_INDEX  + MAX_RETURN_INPUTS)  // 
#define PF_AES_BUS_INDEX      (PF_DUMMYA_BUS_INDEX  + MAX_DUMMYA_INPUTS)  // 
#define PF_RX_BUS_INDEX       (PF_AES_BUS_INDEX     + MAX_AES_INPUTS)    // (alias)

#define ANALOG_BUS_INDEX      (PF_RX_BUS_INDEX      + MAX_RX_INPUTS) // 
#define RETURN_BUS_INDEX	  (ANALOG_BUS_INDEX     + MAX_PROXY_INPUTS)   // 
#define DUMMYA_BUS_INDEX	  (RETURN_BUS_INDEX     + MAX_RETURN_INPUTS)  
#define AES_BUS_INDEX         (DUMMYA_BUS_INDEX     + MAX_DUMMYA_INPUTS)  // 
#define RX_BUS_INDEX          (AES_BUS_INDEX        + MAX_AES_INPUTS) //was+4) // (alias) 4+4 regular digital inputs and then come the RXer digital inputs 

#define SLATE_INDEX           (RX_BUS_INDEX    + MAX_RX_INPUTS) // (8)
#define TALK1_INDEX 		  (SLATE_INDEX   + 1) 
#define TALK2_INDEX 		  (TALK1_INDEX   + 1) 
#define TONE_INDEX            (TALK2_INDEX   + 1)  
							             
#define OUTPUTMIX_BUS_INDEX   (TONE_INDEX+1) // (CAMERA_INDEX+5)
//#define TAPEMONO_BUS_INDEX    (OUTPUTMIX_BUS_INDEX+(MAX_OUTPUTS-2)) // +6) //HEY! this is a pseudonym for outputs 5 and 6
//#define HEADPHONE2_BUS_INDEX  (0+10+10+10+10+1+1+1+1+5+ 16+8 ) // was after DiskMix
#define HEADPHONE_BUS_INDEX   (OUTPUTMIX_BUS_INDEX+MAX_OUTPUTS) // was after DiskMix

#define DISKMIX_BUS_INDEX     (HEADPHONE_BUS_INDEX+2) 
#define PLAY_BUS_INDEX        (DISKMIX_BUS_INDEX+MAX_DISK_CHANNELS) 

#define ANALOG_IN_BUS_INDEX   (PLAY_BUS_INDEX      + MAX_DISK_CHANNELS)  // 
#define AES_IN_BUS_INDEX      (ANALOG_IN_BUS_INDEX + MAX_PROXY_INPUTS+MAX_RETURN_INPUTS)  // includes returns!! 
#define RX_IN_BUS_INDEX       (AES_IN_BUS_INDEX    + MAX_AES_INPUTS)     // includes returns!! 
#define PROXY_IN_BUS_INDEX    (RX_IN_BUS_INDEX     + MAX_RX_INPUTS)      //
#define IFB_RX_INDEX          (PROXY_IN_BUS_INDEX  + MAX_PROXY_INPUTS)   //
#define AUTOMIX_INDEX         (IFB_RX_INDEX        + 1)
#define RAW_SLATE_INT_INDEX   (AUTOMIX_INDEX       + 1) 
#define RAW_SLATE_EXT_INDEX   (RAW_SLATE_INT_INDEX + 1) 
#define MUTE_INDEX            (RAW_SLATE_EXT_INDEX + 1) 
#define BUS_SIZE              (MUTE_INDEX          + 1) 


// prefader analog, prefader digital, analog, digital, slate, tone, talk1, talk2
//#define MAX_MIX_SOURCES  	  ((10+10+10+10)+1+1+1+1) 
#define MIX_SOURCES_PRE_AD	  MAX_INPUTS // (10+10)
#define MIX_SOURCES_POST_AD   MAX_INPUTS // (10+10)

#define MIX_SOURCE_PF_ANALOG  PF_ANALOG_BUS_INDEX  //
#define MIX_SOURCE_PF_RETURN  PF_RETURN_BUS_INDEX  //
#define MIX_SOURCE_PF_AES     PF_AES_BUS_INDEX     //
#define MIX_SOURCE_PF_RX      PF_RX_BUS_INDEX      //  was 4) // (alias)

#define MIX_SOURCE_ANALOG     ANALOG_BUS_INDEX     //
#define MIX_SOURCE_RETURN     RETURN_BUS_INDEX	   //
#define MIX_SOURCE_AES        AES_BUS_INDEX        //
#define MIX_SOURCE_RX         RX_BUS_INDEX         // +MAX_AES_INPUTS) //  was 4) //  (alias)

#define MIX_SOURCE_SLATE      SLATE_INDEX          //
#define MIX_SOURCE_TALK1      TALK1_INDEX 		   //
#define MIX_SOURCE_TALK2      TALK2_INDEX 		   //
#define MIX_SOURCE_TONE       TONE_INDEX           //
#define MAX_MIX_SOURCES      (MIX_SOURCE_TONE+1) // +2 becasue we are adding COM I think - there is something funny going on here... 
// the following is for DiskMix only
#define MIX_SOURCE_OUTPUTS   (MAX_MIX_SOURCES) // hmmmm scary  

// these are special offsets into ST_BUS for old disk mixer
#define ST_SLATE_OFFSET  0 
#define ST_TALK1_OFFSET  1 //(MIX_SOURCE_TALK1 - MIX_SOURCE_SLATE) // 1
#define ST_TALK2_OFFSET  2 //(MIX_SOURCE_TALK2 - MIX_SOURCE_SLATE) // 2
#define ST_TONE_OFFSET   3 //(MIX_SOURCE_TONE  - MIX_SOURCE_SLATE) // 3 


// the following are for the regular HP mixes (including non-ADV instances of HP2,TAPE,MONO mix)
//#define  HP_MIXSOURCE_CAMERA  0   // this is not for ADV HP mixer only for HPmixer which starts at CAMERA bus
#define  HP_MIXSOURCE_OUTPUT   (0)  // was (OUTPUTMIX_BUS_INDEX - CAMERA_INDEX)   // 5
#define  HP_MIXSOURCE_RETURN   (HP_MIXSOURCE_OUTPUT + MAX_OUTPUTS)  // getting rid of ADVhpMix in RX4...
#define  HP_MIXSOURCE_DISK     (HP_MIXSOURCE_RETURN + MAX_RETURN_INPUTS) 
#define  HP_MIX_SOURCES        (HP_MIXSOURCE_DISK   + MAX_DISK_CHANNELS) 

#define MIX_SOURCE_ANALOG_OFFSET     (0)
#define MIX_SOURCE_RETURN_OFFSET	 (MIX_SOURCE_ANALOG_OFFSET + MAX_PROXY_INPUTS) // 10
#define MIX_SOURCE_AES_OFFSET	 	 (MIX_SOURCE_ANALOG_OFFSET + MAX_ANALOG_INPUTS) 
#define MIX_SOURCE_RX_OFFSET		 (MIX_SOURCE_AES_OFFSET    + MAX_AES_INPUTS) 
#define MIX_SOURCE_POST_FADER_OFFSET (MIX_SOURCE_RX_OFFSET     + MAX_RX_INPUTS) // 20
// Analog 1-10  Digital 1-10
#define MAX_INPUT_SOURCES	 (MAX_ANALOG_INPUTS+MAX_DIGITAL_INPUTS) // ANALOG and DIGITAL bus only. Does not include outputs or legacy camera busses


// Defines for gChannelConfig array...

// ANALOG, DIGITAL, SLATE CAMERA // NOMAD NOMAD NOMAD NOMAD NOMAD NOMAD NOMAD NOMAD NOMAD 
#define MAX_IN_CHAN_SOURCES        (TALK2_INDEX+1) // (MAX_INPUT_SOURCES+2) // added Slate,talk1  (10+10+1+1)    //	float DigitalGain[MAX_IN_CHAN_SOURCES];    
#define MAX_IN_CHAN_ANALOG_DIGITAL (MAX_ANALOG_INPUTS+MAX_DIGITAL_INPUTS) // 20
#define MAX_IN_CHAN_ANALOG   MAX_ANALOG_INPUTS  // 10
#define MAX_IN_CHAN_DIGITAL  MAX_DIGITAL_INPUTS // 10

// --- this is for gChannelConfig[] ---
#define IN_CHAN_ANALOG_1	 0
#define IN_CHAN_ANALOG_2	 1
#define IN_CHAN_ANALOG_3	 2
#define IN_CHAN_ANALOG_4	 3
#define LAST_CDSCELL_INPUT   IN_CHAN_ANALOG_4
#define IN_CHAN_RETURN_1     4
#define IN_CHAN_RETURN_2     5
//  6+4 analog dummy channels so analog inputs is the same as digital inputs for legacy math
#define IN_CHAN_ANADUMMY_1 	 6 
#define IN_CHAN_ANADUMMY_2	 7
#define IN_CHAN_ANADUMMY_3	 8
#define IN_CHAN_ANADUMMY_4   9
#define IN_CHAN_ANADUMMY_5   10
#define IN_CHAN_ANADUMMY_6   11

#define IN_CHAN_ANADUMMY_7   12 // added these in NOVA2
#define IN_CHAN_ANADUMMY_8   13
#define IN_CHAN_ANADUMMY_9   14
#define IN_CHAN_ANADUMMY_10  15

#define IN_CHAN_DIGITAL     16 // was 12 in nova1
#define IN_CHAN_DIGITAL_1	(IN_CHAN_DIGITAL+0)
#define IN_CHAN_DIGITAL_2	(IN_CHAN_DIGITAL+1)
#define IN_CHAN_DIGITAL_3	(IN_CHAN_DIGITAL+2)
#define IN_CHAN_DIGITAL_4	(IN_CHAN_DIGITAL+3)
#define IN_CHAN_DIGITAL_5	(IN_CHAN_DIGITAL+4)
#define IN_CHAN_DIGITAL_6	(IN_CHAN_DIGITAL+5)
#define IN_CHAN_DIGITAL_7	(IN_CHAN_DIGITAL+6)
#define IN_CHAN_DIGITAL_8	(IN_CHAN_DIGITAL+7)

#define IN_CHAN_DIGITAL_RX  24 // (IN_CHAN_DIGITAL_8+1) //(24)
#define IN_CHAN_DIGITAL_RX_1 (IN_CHAN_DIGITAL_RX+0)
#define IN_CHAN_DIGITAL_RX_2 (IN_CHAN_DIGITAL_RX+1)
#define IN_CHAN_DIGITAL_RX_3 (IN_CHAN_DIGITAL_RX+2)
#define IN_CHAN_DIGITAL_RX_4 (IN_CHAN_DIGITAL_RX+3)
#define IN_CHAN_DIGITAL_RX_5 (IN_CHAN_DIGITAL_RX+4)
#define IN_CHAN_DIGITAL_RX_6 (IN_CHAN_DIGITAL_RX+5)
#define IN_CHAN_DIGITAL_RX_7 (IN_CHAN_DIGITAL_RX+6)
#define IN_CHAN_DIGITAL_RX_8 (IN_CHAN_DIGITAL_RX+7)
//#define IN_CHAN_DIGITAL_9	20
//#define IN_CHAN_DIGITAL_10	21
//#define IN_CHAN_DIGITAL_11	22
//#define IN_CHAN_DIGITAL_12	23 // Nova1 has (4+8) digital inputs  nova2 has 8+8 digital inputs
#define LAST_DIGITAL_INPUT  IN_CHAN_DIGITAL_RX_8

#define IN_CHAN_SLATE       (LAST_DIGITAL_INPUT+1) // 32 was 24 // (IN_CHAN_DIGITAL + MAX_DIGITAL_INPUTS) // 21 
#define IN_CHAN_TALK1       (LAST_DIGITAL_INPUT+2) // 33 was 25 
#define IN_CHAN_TALK2       (LAST_DIGITAL_INPUT+3) // 34 was 26 
#define IN_CHAN_IFB_RX      (LAST_DIGITAL_INPUT+4) // 35 was 27
//#define NUM_IN_CHAN_SOURCES 28 // note these all dont really line up with bus indexes


/* used in  AudioMix.h    MIX_MATRIX, structure used to setup mix... */
struct MIX_MATRIX
{
	uchar enable,invert;
//	unsigned int enable : 1;	// channel is on or off flag
//	unsigned int invert : 1;	// channel is phase inverted flag
//	unsigned int vfader : 1;    // part of the special soundfield mix / automix (dynamic/static gain value)
};


void UpdateHeadphoneMix(int nHeadphone = 0, int nChannel = -1,   int fUpdateNow = TRUE, int fAdvanced = FALSE);
void UpdateDiskMix     (int nChannel = -1,  int fUpdateNow = TRUE);
void UpdateOutputMix   (int nChannel = -1,  int fUpdateNow = TRUE);
void ClearAllFaderAssigns(int saveit=1);
void RestoreMainMixes();
int  CameraReturnOn(void);
void ClearDiskMixer();
void ChangeExtFaderMixPreset(int preset);
void FaderSetup(); 
int  GetCardTrackISOchannel(int track);
int  FindCardTrackFromFaderNum(int fnum);
void ExtFaderSetup(); // now called on boot and when changing banks

//void SetFadersLog(int fader, float logval);
//void SetFadersLog(int fader, float logval, int pos255);
void SetFadersLog(int fader, int pos255, bool IsRound);

void StoreAudioChannelPositions_RTCRAM();
void StoreAudioChannelPosition_RTCRAM(unsigned AudioChannel);

void UpdateVirtualFader(int vnum);
 // call this whenever gNV.Faders[vnum] changes

//void RefreshMix12map(); // this has to look at bank 0!

void InitMixer(void);
void ToneInit (void);
void ToneOnOff(int state);
void ToneOn   (void);
void ToneOff  (void);
void SetToneMix(bool On);
void SetRefLevel(double level);
void SetToneFreq(double freq);
int  CardTrackCanHaveAutoTrim(int dch);
int  SearchFaderForDuplicate(int fnum, int ftype, char *str1, char *str2);
int	 SearchFaderForInput(int fnum, int input);

//#define NUM_EQ_CHANNELS   16
#define NUM_EQ_SECTIONS	   5

#define EQ_TYPE_OFF        0

#define EQ_TYPE_LOW_SHELF  1
#define EQ_TYPE_HIGH_SHELF 2
#define EQ_TYPE_PEAKING    3
#define	EQ_TYPE_NOTCH      4
#define EQ_TYPE_HIGHPASS   5

#define EQ_TYPE_BPF		   6
#define EQ_TYPE_LPF		   7	


int  GetOutputRouting(int nOutput, TRANSPORT_MODE mode, int & bus, int & nBusIndex);
void SetOutputRouting(int nOutput, TRANSPORT_MODE mode, int bus, int nBusIndex);

// For Set / Get cross point functions only ...
enum MixerType {HEADPHONE_MIXER, 
                DISK_MIXER, 
                OUTPUT_MIXER, 
                ADV_HEADPHONE_MIXER,
                HEADPHONE2_MIXER, 
                ADV_HEADPHONE2_MIXER, 
                TAPEMONO_MIXER, 
                ADV_TAPEMONO_MIXER };

enum InputBus {PF_ANALOG_BUS, 
			   PF_RETURN_BUS, // did not exist prior to RX4
               PF_DIGITAL_BUS, 
//               PF_AES_BUS, 
//               PF_RX_BUS, 
			   ANALOG_BUS, 
			   RETURN_BUS, // was called CAMERA_BUS,
			   DIGITAL_BUS, 
//			   AES_BUS,
//			   RX_BUS,
			   DISK_BUS, 
			   OUTPUT_BUS, 
			   ST_BUS,
			   INPUT_BUS };


void SetCrossPoint(MixerType type, InputBus bus, int iInput, int iOutput, int action, int update = 0);
void CycleCrossPoint(MixerType type, InputBus bus, int iInput, int iOutput, int update = 0);
#define XPT_OFF		1
#define XPT_ON		2
//#define XPT_TOGGLE	4 // no longer use this cuz it requires memory of phase setting
#define XPT_INVERT	8 
struct XPOINT_STATE
{
	uchar A, AInvrt; // analog
	uchar B, BInvrt; // digital
};
enum XPOINT_VALUE { CLEAR = 0, PF_ANALOG = 1, ANALOG = 2, PF_DIGITAL = 3, DIGITAL = 4, SET = 5 };
XPOINT_STATE GetCrossPoint(MixerType type, InputBus bus, int iInput, int iOutput);

// this is for gChannelConfig[].TrimMode
#define TRIMMODE_NORMAL 0
#define TRIMMODE_MIX8   1 // this is never used!!!
#define TRIMMODE_ZAXNET 2

void Nomad_SetFaderAssign(unsigned TypeMask, unsigned InputNum, unsigned FaderNum);
int  Nomad_GetFaderAssign(unsigned InputNum, unsigned FaderNum);
void Nomad_UpdatePanAssign(unsigned InputBusNum);
int  Observe_PanAssign(unsigned InputBusNum, int OutputMixPair);
//int  Nomad_CheckPanAssign (unsigned InputBusNum);
void ClearAllMixStructs();	// clears everything
void UpdateDiskShifters();

void AddInputToFaderChList(struct FADER_ASS *p, int input);
int  DeleteInputFromFaderChList(struct FADER_ASS *p, int input);
int  GetAudioChannelFromBank0FaderAssign(int RawFaderNum);
int  GetFaderNumFromAudioChannel(int chan);
int  StripDigitalPartOfInput(int input); // convert digital index to analog base index
void CleanUpFaderChList(struct FADER_ASS *p);  // just sorts p->ch[] as uchars

char *GetBusTypeString(InputBus InputBusType);
char *GetMixerBusTypeString(MixerType MixerBusType);
char *GetActionMaskString(int  ActionMask);

int GetAudioChannelFromFaderAssign(int fader);

#define PAN_TYPE_OFF    0 // this matches UI    
#define PAN_TYPE_LEFT   1
#define PAN_TYPE_CENTER 2  // v2.28: changed ordering to grey code transition
#define PAN_TYPE_RIGHT  3

////////////////////////// Stuff For AudioBufs.cpp ////////////////////////////////////////////////

extern int fDiskMixDirty;
extern int fOutputMixDirty;
extern int fFaderAssignDirty;

#define SIZEOF_FadersLog  NUM_TOTAL_FADERS // (1+8+4+12+4)
// 1  +		// Headphone
// 8  +		// Deva hardware faders 1-8
// 4  +		// Virtual faders 1-4
// 12 +		// External faders 1-12
// 4		// External output gain
// 4 		// External output gain 5-8 for new Oasis and Deva32

//#define FADER_FILLER_NUM 2 // padd the end just in case....		(8+4+12+4)=28
#define SIZEOF_Fader  (NUM_TOTAL_FADERS) //(33) // Faders (N,L,R) chunk   // These are where the fader values are used 
// FadersLog[] has to line up with mix12 fader and knob codes	(Remote.cpp)

#define SIZEOF_DigitalGain       (MAX_IN_CHAN_SOURCES)
#define SIZEOF_FaderTrim         (MAX_IN_CHAN_SOURCES) //(22) was(16) but SLATE is 22!!!!!!
#define SIZEOF_Mix12Trim         (MAX_IN_CHAN_SOURCES) //(22) was(16) 
#define SIZEOF_DelayLine_A		 (MAX_IN_CHAN_ANALOG_DIGITAL)
#define SIZEOF_DelayLine_B       (MAX_INPUT_DELAY_SAMPLES) 
#define SIZEOF_OutputDelayLine_A (MAX_OUTPUTS)
#define SIZEOF_OutputDelayLine_B (MAX_OUTPUT_DELAY_SAMPLES)
#define SIZEOF_ProxyDelayLine_A   MAX_PROXY_INPUTS
#define SIZEOF_ProxyDelayLine_B	 (CHUNK*2) // only need 2 chunks worth

//#define MAX_PROXY_DELAY_SAMPLES SIZEOF_ProxyDelayLine_B
//#define PROXY_DELAY_SAMPLES 23 // ADC delay 39-16=23    AK5358a = proxy ADC (filter delay=16) AK4621 = primary high quality ADC (filter delay=39)

extern SAMPLE Bus[BUS_SIZE][CHUNKSIZE];   
extern SAMPLE Tone[CHUNKSIZE]; 
extern float  FadersLog[SIZEOF_FadersLog];  // (NUM_TOTAL_FADERS) (33)
extern float  OutputFadersLog[MAX_NUM_OUTPUT_KNOBS]; 

#ifdef SINGLE_FADER_MULT 
//extern SAMPLE InputFaderLogSmooth[MAX_IN_CHAN_SOURCES];// NUM_TOTAL_FADERS // These are where the fader values wind up
#else
extern SAMPLE Fader[SIZEOF_Fader][CHUNKSIZE]; // (NUM_TOTAL_FADERS) (33) 
#endif

extern float  DigitalGain      [SIZEOF_DigitalGain];      // (maxInsources)(22)
extern float  DigitalGainSmooth[SIZEOF_DigitalGain];// (maxInsources)(22)
extern float  DigitalGainPhase [SIZEOF_DigitalGain]; // (maxInSources)(22) +1 or -1
extern float  FaderTrim        [SIZEOF_FaderTrim]; // maxInSources (22)    
extern float  Mix12Trim        [SIZEOF_Mix12Trim]; // MaxInSources (22)    

extern SAMPLE DelayLine      [SIZEOF_DelayLine_A]      [SIZEOF_DelayLine_B]; // MAX_IN_CHAN_ANALOG_DIGITAL][MAX_DELAY_SAMPLES];	// Delay lines
extern SAMPLE OutputDelayLine[SIZEOF_OutputDelayLine_A][SIZEOF_OutputDelayLine_B]; // [MAX_OUTPUTS][MAX_DELAY_SAMPLES];
extern SAMPLE ProxyDelayLine [SIZEOF_ProxyDelayLine_A] [SIZEOF_ProxyDelayLine_B];	// fixed Delay lines for proxy inputs


#define mHP1  0 // these three are indexes into HeadphoneMixEnables[][][] 
#define mHP2  1
#define mTAPE 2

#define SIZEOF_HeadphoneMixMatrix_A	   3  // for hp1, hp2 , TapeMono
#define SIZEOF_HeadphoneMixMatrix_B	   2  // one pair each
#define SIZEOF_HeadphoneMixMatrix_C	  (MAX_DISK_CHANNELS + MAX_OUTPUTS + MAX_RETURN_INPUTS) //  how many headphone mix sources there are I am trying to get rid of AdvHpMixer by adding returns here - 
//#define SIZEOF_HeadphoneMixMatrix_C	  (MAX_DISK_CHANNELS + MAX_OUTPUTS) // +MAX_CAMERA_INPUTS  // how many headphone mix sources there are

/*
#define SIZEOF_AdvHeadphoneMixMatrix_A 3 // for hp1, hp2 , TapeMono
#define SIZEOF_AdvHeadphoneMixMatrix_B 2
#define SIZEOF_AdvHeadphoneMixMatrix_C (2*(MAX_IN_CHAN_ANALOG+MAX_IN_CHAN_DIGITAL))
*/

#define SIZEOF_DiskMixMatrix_A	    MAX_DISK_CHANNELS
#define SIZEOF_DiskMixMatrix_B     (MAX_MIX_SOURCES+MAX_OUTPUTS)	//  (added output in 3.15D)

#define SIZEOF_OutputMixMatrix_A   MAX_OUTPUTS
#define SIZEOF_OutputMixMatrix_B   MAX_MIX_SOURCES

extern struct MIX_MATRIX HeadphoneMixMatrix   [SIZEOF_HeadphoneMixMatrix_A][   SIZEOF_HeadphoneMixMatrix_B][   SIZEOF_HeadphoneMixMatrix_C];
//extern struct MIX_MATRIX AdvHeadphoneMixMatrix[SIZEOF_AdvHeadphoneMixMatrix_A][SIZEOF_AdvHeadphoneMixMatrix_B][SIZEOF_AdvHeadphoneMixMatrix_C];

extern struct MIX_MATRIX DiskMixMatrix    [SIZEOF_DiskMixMatrix_A][  SIZEOF_DiskMixMatrix_B];
extern struct MIX_MATRIX OutputMixMatrix  [SIZEOF_OutputMixMatrix_A][SIZEOF_OutputMixMatrix_B];

//#define FTYPE_IS_FADER  0 //(mTrimArmed == 0) ("Fader");
//#define FTYPE_IS_TRIM   1 //(mTrimArmed == 1) ("Trim");
//#define FTYPE_IS_ZAXNET 2 //(mTrimArmed == 2) ("ZaxNet|Trim");

// these are being replaced by IS_FADER, IS_TRIM, IS_ZNET enums
/*
#define FMASK_FADER   (1<<0) // 1
#define FMASK_TRIM    (1<<1) // 2
#define FMASK_ZAXNET  (1<<2) // 4
#define FMASK_DIGITAL (1<<3) // 8
#define FMASK_ANALOG  (1<<4) // 16
#define FMASK_VIRTUAL (1<<5) // 32
*/

// use these enums to determine what kind of fader this is
//#define SIZEOF_FaderAssignMatrix_A  NUM_TOTAL_FADERS
#define SIZEOF_FaderList  6 // max number of multiplle inputs assigned to one fader 
#define FADERTYPE_DISABLED 0
#define FADERTYPE_FADER    1  
#define FADERTYPE_ZNET     2
#define FADERTYPE_TRIM     3
// due to multiple inputs per fader we need to be able to clea rout all assignments to a fader using this 
#define FADERTYPE_CLEARALL 4

#define EXT_BANK_CHANGE_TIME_MS 1300  // how long it takes Oaiss to change banks
#define INT_BANK_CHANGE_TIME_MS  300  // how long it takes front panel faders to change banks
struct BANK_STATES{
	short CurrentBank,TrimButtonActive,ZnetButtonActive;
	uint32 BankChangeTime; // indicates last time bank was changed
	uint32 NudgeFaderTimer; // triggers a little after the faders stop moving to insure we get all the faders sent to fadersLog
//	uchar  BankPos[5][16]; // memory for 5 fader banks from Nomad or from demo data
};
extern struct BANK_STATES gIntBankStates;
extern struct BANK_STATES gExtBankStates;
int GetFaderBankIndex(int faderNum);
struct FADER_ASS *GetFaderAssPtr(int fnum);
int SetNewInternalBank(int bank); // (-1) means toggle to next bank

struct FADER_ASS // new faderAssign matrix that combines gAuxFaderConfig into gFaderAssignMatrix
{ 
	char type; // fader type enum  
	char ch[SIZEOF_FaderList]; //[6] short list of audio channels (not a znet TXID! if znet then gChannelConfig[chan].TXID determines znet ID
	// audio ch of -1 means empty channel list slot
};
//extern struct FADER_ASS FaderAssignList[SIZEOF_FaderAssignMatrix_A]; // [SIZEOF_FaderAssignMatrix_B];
extern struct FADER_ASS FaderAssList[NUM_FADER_BANKS][NUM_TOTAL_FADERS]; // RX4: replaced gPresetCache with this one thing that has all banks and all faders in it


#define SIZEOF_gChannelConfig    MAX_INPUTS // MAX_IN_CHAN_SOURCES // was SIZEOF_Fader // 22  was 20 (SLATE is 22!) [MAX_INPUT_SOURCES];    // one for each input channel
#define SIZEOF_gDiskChanConfig   MAX_DISK_CHANNELS // 12
#define SIZEOF_gOutputChanConfig MAX_OUTPUTS

extern struct CHANNEL_CONFIGURATION        gChannelConfig   [SIZEOF_gChannelConfig];  //   Analog + Digital + Returns (no slate)
extern struct DISK_CHANNEL_CONFIGURATION   gDiskChanConfig  [SIZEOF_gDiskChanConfig];
extern struct OUTPUT_CHANNEL_CONFIGURATION gOutputChanConfig[SIZEOF_gOutputChanConfig];



#define PF_DIGITAL_BUS_INDEX  		PF_AES_BUS_INDEX 
#define DIGITAL_BUS_INDEX     		AES_BUS_INDEX 
#define MIX_SOURCE_PF_DIGITAL 		PF_AES_BUS_INDEX     //
#define MIX_SOURCE_DIGITAL        	MIX_SOURCE_AES        //
#define MIX_SOURCE_DIGITAL_OFFSET 	MIX_SOURCE_AES_OFFSET

#define NUM_PC_INPUTS 22 // = (MAX_PROXY_INPUTS + MAX_RETURN_INPUTS + MAX_AES_INPUTS + MAX_RX_INPUTS)

