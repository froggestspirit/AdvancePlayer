
bool running;
bool minit(int songID, int sRate, bool inf);
bool loadSong(int id);
unsigned long read_long_from(unsigned long addr);
float *mloop();
bool stop();
float sampleRate;
unsigned int musicSize;
unsigned int songLoc;
unsigned int songOffset;
unsigned char numTracks;
unsigned int tablePointer;
unsigned int numSongs;
unsigned int tempFilePos;
unsigned char keyIsSplit[0x80];
unsigned int keySplitPointer;
unsigned char lastCommand[16];
unsigned char lastNote[16];
unsigned char lastVel[16];
unsigned char tempNote;
unsigned char tempVel;

bool delayHit;
bool seqFrame;
int maxLoops;//number of times to loop the song before fading out
unsigned char timesLooped[16];
signed short fadeVol;
unsigned char loopi;
bool infLoop;
bool paused=false;

unsigned char temp[4];
unsigned char tempKey[8];
unsigned long temp32;
unsigned char varLength;
unsigned short headerSize;
unsigned long infoBlock;
unsigned long fatArray;
unsigned long seqArray;
unsigned long bankArray;
unsigned long wavArray;
unsigned short seqID;
unsigned short bankID;
unsigned short wavID[4];

int seqIndex;
unsigned long seqPointer;
unsigned long bankPointer;
unsigned long wavPointer[4];
unsigned long seqSize;
unsigned long dataOffset;
unsigned long chPointer[16];
unsigned short usedTracks;
unsigned short seqTempo;
unsigned short tempoFill;
unsigned char seqVol;

unsigned char repeated;//1 if the command was repeated
unsigned char slotChannel[16];//sample playing in this slot belongs to what channel
unsigned char slotFree[16];//is this slot free?
unsigned char curSlot;
unsigned long slotMidiFreq[16];
unsigned char slotNote[16];
unsigned char slotNoteVel[16];
unsigned long slotNoteLen[16];
unsigned long slotSamplePointer[16];
unsigned long slotKeyPointer[16];
unsigned char slotInstType[16];
unsigned long chDelay[16];
unsigned char chInstrument[16];
unsigned char chPan[16];
unsigned char chPanL[16];
unsigned char chPanR[16];
unsigned char chVol[16];
signed char chTranspose[16];
signed short chPitchBendCur[16];
signed char chPitchBend[16];
unsigned char chPitchBendRange[16];
unsigned short chSweepPitch[16];
unsigned char chPriority[16];
unsigned char chPolyMono[16];
unsigned char chTie[16];
unsigned char chPort[16];
unsigned char chPortOn[16];
unsigned char chPortTime[16];
unsigned short chModDelay[16];
unsigned char chModDepth[16];
unsigned char chModSpeed[16];
unsigned char chModType[16];
unsigned char chModRange[16];
unsigned char chAttack[16];
unsigned char chDecay[16];
unsigned char chSustain[16];
unsigned char chRelease[16];
unsigned char chLoopTimes[16];//times left to loop
unsigned long chLoopOffset[16];//offset to return to from loop end marker
unsigned long chReturnOffset[16];//offset to return to from the call
bool chInCall[16];//track hit a call command and did not hit return yet
bool validInst[128];

bool slotWavNibble[16];//what nibble is the 4bit wave on?
float slotAttack[16];
float slotDecay[16];
float slotSustain[16];
float slotRelease[16];
unsigned char slotPan[16];
unsigned char slotPanL[16];
unsigned char slotPanR[16];
unsigned char slotADSRVol[16];
unsigned char slotADSRState[16];

unsigned long keyPointer[16];
unsigned long instPointer[16];

float volModL[16];
float volModR[16];
float mixer[2];//pre mixer
float soundOut[2];//final output
bool chActive[16];//is the channel on
float slotPitch[16];
unsigned long slotPitchFill[16];
unsigned short slotSampleID[16];//(channel instrument<<7)+note
unsigned long samplePitchFill[16];
float samplePos[16];//current position in the sample

float samplePitch[16];
unsigned long sampleOffset[16];//offset of the current sample
float sampleEnd[16];//the end of the sample
float sampleLoop[16];//loop point
float sampleLoopLength[16];//length of loop point to end
unsigned char sampleFormat[16];//format of the sample
bool sampleLoops[16];//does the sample loop

signed short soundChannel4Bit;
bool sampleDone[16];//did the non-looping sample finish
bool sampleNibbleLow;
unsigned char tempNibble;
signed char sampleStepIndex;
signed short sampleDiff;
signed short samplePredictor;
signed short sampleStep;
float sampleOutput[16];
float curKeyRoot[16];

float lastSample[2];

signed char PSG[0x40];

unsigned char gb_hram [0x40];
signed short soundBuffer [2];
float soundChannelPos [4];
signed short soundChannel4Bit;
const signed short *PU1Table;
const signed short *PU2Table;
const unsigned char *PU4Table;
unsigned short PU4TableLen;

unsigned short gb_ch1Freq;
unsigned char gb_ch1SweepCounter;
unsigned char gb_ch1SweepCounterI;
bool gb_ch1SweepDir;
unsigned char gb_ch1SweepShift;
unsigned char gb_ch1Vol;
unsigned char gb_ch2Vol;
unsigned char gb_ch3Vol;
unsigned char gb_ch4Vol;
unsigned char gb_ch1VolI;
unsigned char gb_ch2VolI;
unsigned char gb_ch3VolI;
unsigned char gb_ch4VolI;
unsigned char gb_ch1Len;
unsigned char gb_ch2Len;
unsigned char gb_ch3Len;
unsigned char gb_ch4Len;
unsigned char gb_ch1LenI;
unsigned char gb_ch2LenI;
unsigned char gb_ch3LenI;
unsigned char gb_ch4LenI;
bool gb_ch1LenOn;
bool gb_ch2LenOn;
bool gb_ch3LenOn;
bool gb_ch4LenOn;
unsigned char gb_ch1EnvCounter;
unsigned char gb_ch2EnvCounter;
unsigned char gb_ch4EnvCounter;
unsigned char gb_ch1EnvCounterI;
unsigned char gb_ch2EnvCounterI;
unsigned char gb_ch4EnvCounterI;
bool gb_ch1EnvDir;
bool gb_ch2EnvDir;
bool gb_ch4EnvDir;
unsigned short gb_WAVRAM[32];