//SDAT Player 1.0 FroggestSpirit


void readTemp(int bytes){
	for(int ini=0; ini<bytes; ini++){
		temp[ini]=sdat[filePos++];
	}
}

#include "lookuptables.h"

void NDSbuttonloop(){
	infLoop^=1;
}
void NDSbuttonPause(){
	paused^=1;
	seqFrame=true;
}
void NDSbuttonNext(){
	running = false;
}
void NDSbuttonShuffle(){
	running = false;
}

bool NDS_loadSong(int id){//loads a song
    int idi=id;
	songLoc=NDS_getAddress((idi*8)+4);
	filePos=songLoc;
	songOffset=NDS_getAddress(filePos);
	printf("Song Location:%X\n",songLoc);
	printf("Song Offset:%X\n",songOffset);	
	readTemp(4);
	numTracks=(temp[1]<<8);
	numTracks+=temp[0];
    	printf("Number of Tracks:%X\n",numTracks);
	tablePointer=NDS_getAddress(songLoc+8);
    	printf("Table at:%X\n",tablePointer);
	return true;
}

unsigned long NDS_getAddress(unsigned long addr){//returns the 4-byte address at addr
	filePos=addr;
	readTemp(4);
	unsigned long tempLong;
	tempLong=(temp[3]<<8);
	tempLong=((tempLong+temp[2])<<8);
	tempLong=((tempLong+temp[1])<<8);
	tempLong=(tempLong+temp[0]);
	return tempLong;
}

bool NDS_begin(int songID, int sRate)
{
    running=false;
	sampleRate=sRate;
    filePos=0;
	// Set the output values properly
	delayHit=false;
	tempoFill=0;
	usedTracks=0xFFFF;
	sseqTempo=120;
	sseqVol=0x7F;
	fadeVol=0;
	maxLoops=2;
	mixer[0]=0;
	mixer[1]=0;
	soundOut[0]=0;
	soundOut[1]=0;
	
	sdatSize=sizeof(sdat);
	printf("File Size:%d\n",sdatSize);
	for(int i=0; i<16; i++){
		PSG[0x00+i] = i<2 ? 0x3F : -0x3F;
		PSG[0x10+i] = i<4 ? 0x3F : -0x3F;
		PSG[0x20+i] = i<8 ? 0x3F : -0x3F;
		PSG[0x30+i] = i<12 ? 0x3F : -0x3F;
	}

	//Read SDAT header
    filePos=0x00;
	readTemp(4);
	totalSongs=numSongs=temp[0]+(temp[1]<<8)+(temp[2]<<16)+(temp[3]<<24);
    printf("Number of Songs: %X\n",numSongs);
    
	for (int i=0; i<16; i++) {
		timesLooped[i]=maxLoops;
		chPointer[i]=0;
		slotChannel[i]=0;
		slotFree[i]=0;
		slotNote[i]=0;
		slotNoteVel[i]=0;
		slotNoteLen[i]=0;
		chDelay[i]=0;
		chInstrument[i]=0;
		chPan[i]=0x40;
		chPanL[i]=0;
		chPanR[i]=0;
		chVol[i]=0x7F;
		chTranspose[i]=0;
		chPitchBendCur[i]=0;
		chPitchBend[i]=0;
		chPitchBendRange[i]=2;
		chSweepPitch[i]=0;
		chPriority[i]=0;
		chPolyMono[i]=0;
		chTie[i]=0;
		chPort[i]=0;
		chPortOn[i]=0;
		chPortTime[i]=0;
		chModDelay[i]=0;
		chModDepth[i]=0;
		chModSpeed[i]=0;
		chModType[i]=0;
		chModRange[i]=0;
		chAttack[i]=0;
		chDecay[i]=0;
		chSustain[i]=0;
		chRelease[i]=0;
		chLoopTimes[i]=0;
		chLoopOffset[i]=0;
		chReturnOffset[i]=0;
		chInCall[i]=false;
	
		chActive[i]=false;
		slotSamplePointer[i]=0;
		tempNibble=0;
		slotMidiFreq[i]=0;
		slotPitch[i]=0;
		slotPitchFill[i]=0;
		samplePitchFill[i]=0;
		chInstrument[i]=0;
        sampleDone[i]=true;
	}

	sseqIndex=songID;
	NDS_loadSong(sseqIndex);
	//read sseq header
	filePos=songLoc+0x0c;
	for(int i=0; i<numTracks; i++){
		chPointer[i]=NDS_getAddress(filePos)-songOffset;
		chActive[i]=true;
		timesLooped[i]=0;
		printf("Track Pointer %X: %X\n",i,chPointer[i]);
	}
    //read sbnk header
	filePos=tablePointer;
	int numKeySplits=0;
	int numAllSplits=0;
	for(int i=0; i<128; i++){
		keyPointer[i<<7]=filePos;
		readTemp(4);
		keyIsSplit[i]=0;
		switch(temp[0]){
			case 0x00:
			case 0x08:
			case 0x03:
			case 0x0B://Sample/Wav
				instPointer[i<<7]=NDS_getAddress(filePos);
				filePos+=4;
			break;
			case 0x01:
			case 0x02:
			case 0x04:
			case 0x09:
			case 0x0A:
			case 0x0C://PSG
				filePos+=8;
			break;
			case 0x40://Key Split
				keyIsSplit[i]=1;
				numKeySplits++;
				temp32=NDS_getAddress(filePos);
				keySplitPointer=NDS_getAddress(filePos);
				printf("Key Split Keys at:%X Table at:%X\n",keySplitPointer,temp32);
				if(keySplitPointer<sdatSize && temp32<sdatSize){
					tempFilePos=filePos;				
					filePos=keySplitPointer;
					for(int ii=0; ii<128; ii++){
						//get sub instruments
						temp[0]=sdat[filePos++];
						keyPointer[(i<<7)+ii]=temp32+(temp[0]*12);
					}
					for(int ii=0; ii<128; ii++){
						instPointer[(i<<7)+ii]=NDS_getAddress(keyPointer[(i<<7)+ii]+4);
					}
					filePos=tempFilePos;
				}
			break;
			case 0x80://Every Key Split
				keyIsSplit[i]=1;
				numAllSplits++;
				temp32=NDS_getAddress(filePos);
				printf("Drum Split at:%X\n",temp32);
				if(temp32<sdatSize){
					tempFilePos=filePos;
					for(int ii=0; ii<128; ii++){
						//get sub instruments
						keyPointer[(i<<7)+ii]=temp32+(ii*12);
						instPointer[(i<<7)+ii]=NDS_getAddress(keyPointer[(i<<7)+ii]+4);
					}
					filePos=tempFilePos;
				}
				filePos+=4;
			break;
		}
	}
	printf("Number of Key Splits %X\n",numKeySplits);
	printf("Number of Drum Splits %X\n",numAllSplits);
	running = true;
	return true;
}

bool NDS_stop()
{
	running = false;
	return true;
}

char * NDS_loop(){
	seqFrame=false;
	static int c = 0;
//sample processing
	if (!running) return 0; // Nothing to do here!
    if(paused) return 0;
    c+=96;
	if(c>=sampleRate){//player update
        c-=sampleRate;
        seqFrame=true;
        if(fadeVol<0){
            fadeVol--;
            if(fadeVol<=-723) running = false;
            if (!running) return 0;
        }
        tempoFill+=sseqTempo;
        if(tempoFill>=240){
            tempoFill-=240;
            for(int i=0; i<16; i++){
                if(chActive[i]){
                    if(chDelay[i]==0){
                        delayHit=false;
                        while(!delayHit){
                            filePos=chPointer[i];
                            if(sdat[filePos]<0x80){
								temp[0]=lastCommand[i];
								repeated=1;
							}else{
								temp[0]=sdat[filePos++];
								repeated=0;
							}
                            //printf("Command:%X at:%X\n",temp[0],filePos);
                            switch(temp[0]){
                                case 0xB1: //End of Track
                                    chActive[i]=false;
                                    delayHit=true;
                                    timesLooped[i]=maxLoops;
                                    loopi=0;
                                    while(timesLooped[loopi]>=maxLoops && loopi<16) loopi++;
                                    if(loopi==16) fadeVol=-722;
                                break;
                                case 0xB2: //Jump
                                    chPointer[i]=NDS_getAddress(filePos)-songOffset;
                                    if(timesLooped[i]<maxLoops)timesLooped[i]++;
                                    loopi=0;
                                    while(timesLooped[loopi]>=maxLoops && loopi<16) loopi++;
                                    if(loopi==16 && fadeVol==0 && !infLoop) fadeVol--;
                                break;
                                case 0xB3: //Call
                                    chReturnOffset[i]=chPointer[i]+5;
                                    chInCall[i]=true;
                                    chPointer[i]=NDS_getAddress(filePos)-songOffset;
                                break;
                                case 0xB4: //Return
                                    if(chInCall[i]){
                                        chPointer[i]=chReturnOffset[i];
                                        chInCall[i]=false;
                                    }else{
                                        chPointer[i]++;
                                    }
                                break;
                                case 0xBA: //Priority
                                    temp[0]=sdat[filePos++];
                                    
                                    chPointer[i]+=2;
                                break;
                                case 0xBB: //Tempo
                                    temp[0]=sdat[filePos++];
                                    sseqTempo=(temp[0]<<1);
                                    chPointer[i]+=2;
                                break;
                                case 0xBC: //Transpose
                                    temp[0]=sdat[filePos++];
                                    chTranspose[i]=temp[0];
                                    chPointer[i]+=2;
                                break;
                                case 0xBD: //Instrument Change
                                    temp[0]=sdat[filePos++];
                                    if(chInstrument[i]!=(temp[0]&0x7F)){
                                        chInstrument[i]=temp[0]&0x7F;
                                    }
                                    chPointer[i]+=2;
                                    lastCommand[i]=0xBD;
                                break;
                                case 0xBE: //Volume
                                    temp[0]=sdat[filePos++];
                                    chVol[i]=NDS_Cnv_dB(temp[0]);
                                    chPointer[i]+=2;
                                    lastCommand[i]=0xBE;
                                break;
                                case 0xBF: //Pan
                                    temp[0]=sdat[filePos++];
                                    chPan[i]=temp[0];
                                    chPanL[i]=PAN_TABLE[temp[0]];
                                    chPanR[i]=PAN_TABLE[127-temp[0]];
                                    chPointer[i]+=2;
                                    lastCommand[i]=0xBF;
                                break;
                                case 0xC0: //Pitch Bend
                                    temp[0]=sdat[filePos++];
                                    chPitchBend[i]=(temp[0]);
                                    chPointer[i]+=2;
                                    chPitchBendCur[i]=((chPitchBend[i]-0x40)*(chPitchBendRange[i]<<1));
                                    //if(chPitchBend[i]>0x40) chPitchBendCur[i]=((chPitchBend[i]-0x40)*chPitchBendRange[i]);
                                    //if(chPitchBend[i]<0x40) chPitchBendCur[i]=(chPitchBend[i]*chPitchBendRange[i]);
                                    lastCommand[i]=0xC0;
                                break;
                                case 0xC1: //Pitch Bend Range
                                    temp[0]=sdat[filePos++];
                                    chPitchBendRange[i]=temp[0];
                                    chPointer[i]+=2;
                                    chPitchBendCur[i]=((chPitchBend[i]-0x40)*(chPitchBendRange[i]<<1));
                                    //if(chPitchBend[i]>128) chPitchBendCur[i]=((chPitchBend[i]-256)*chPitchBendRange[i]);
                                    //if(chPitchBend[i]<=128) chPitchBendCur[i]=(chPitchBend[i]*chPitchBendRange[i]);
                                    lastCommand[i]=0xC1;
                                break;
                                case 0xC2: //LFO Speed
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=2;
                                break;
                                case 0xC3: //LFO delay
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=2;
                                break;
                                case 0xC4: //LFO depth
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=2;
                                    lastCommand[i]=0xC4;
                                break;
                                case 0xC5: //LFO type
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=2;
                                break;
                                case 0xC8: //detune
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=2;
                                    lastCommand[i]=0xC8;
                                break;
                                case 0xCD: //psudo echo
                                    temp[0]=sdat[filePos++];

                                    chPointer[i]+=3;
                                    lastCommand[i]=0xCD;
                                break;
                                case 0xCE: //Note Off
									//printf("OFF:%X\n",i);
									if(sdat[filePos]<0x80){//note argument provided?
										temp[0]=sdat[filePos++];
										chPointer[i]++;
									}else{
										temp[0]=lastNote[i];
									}
									lastNote[i]=temp[0];
									tempNote=temp[0];
									if(sdat[filePos]<0x80){//velocity argument provided?
										temp[0]=sdat[filePos++];
										chPointer[i]++;
									}else{
										temp[0]=lastVel[i];
									}
									lastVel[i]=temp[0];
									tempVel=temp[0];
									
									for(int slot=0; slot<16; slot++){
										if(slotChannel[slot]==i){//find slots used by this channel
											if(slotNote[slot]==tempNote){
												slotFree[slot]=0;
												slotADSRState[slot]=3;
												slotADSRVol[slot]=slotSustain[slot];
											}
										}
									}
                                    lastCommand[i]=0xCE;
                                    chPointer[i]++;
                                break;
                                default:
                                    if(temp[0]>=0xCF){//Note on
										lastCommand[i]=temp[0];
										//printf("ON:%X Command:%X\n",i,temp[0]);
                                        curSlot=0xFF;
                                        for(int slot=0; slot<16; slot++){
                                            if(slotFree[slot]==0){//check for free slots
                                                curSlot=slot;
                                                slot=16;
                                            }
                                        }
                                        if(curSlot==0xFF){
                                            for(int slot=0; slot<16; slot++){
                                                if(slotFree[slot]==1){//if free slots are unavailable, check for slot in release state
                                                    curSlot=slot;
                                                    slot=16;
                                                }
                                            }
                                        }
                                        if(curSlot==0xFF){
                                            for(int slot=0; slot<16; slot++){
                                                if(slotFree[slot]==2){//if free slots are unavailable, pick a slot
                                                    curSlot=slot;
                                                    slot=16;
                                                }
                                            }
                                        }
                                        if(curSlot<0xFF){
											curSlot=i;//remove once track priority is implemented
                                            slotChannel[curSlot]=i;
                                            slotFree[curSlot]=2;
                                            slotNoteLen[curSlot]=0;
                                            if(temp[0]>=0xD0){
												slotNoteLen[curSlot]=(DELTA_TIME[temp[0]-0xD0]);
											}
                                            if(sdat[filePos]<0x80){//note argument provided?
												temp[0]=sdat[filePos++];
												chPointer[i]++;
											}else{
												temp[0]=lastNote[i];
											}
                                            lastNote[i]=temp[0];
                                            slotNote[curSlot]=temp[0];
                                            slotMidiFreq[curSlot]=((slotNote[curSlot]+chTranspose[i])<<7);
                                            if(sdat[filePos]<0x80){//velocity argument provided?
												temp[0]=sdat[filePos++];
												chPointer[i]++;
											}else{
												temp[0]=lastVel[i];
											}
											lastVel[i]=temp[0];
                                            slotNoteVel[curSlot]=NDS_Cnv_dB(temp[0]);
                                            if(slotNoteLen[curSlot]>0){//Optional delta-time
												if(sdat[filePos]<0x80){
													slotNoteLen[curSlot]+=sdat[filePos++];
													chPointer[i]++;
												}
											}
											if(keyIsSplit[chInstrument[i]]==1){
												slotSamplePointer[curSlot]=instPointer[(chInstrument[i]<<7)+slotNote[curSlot]];
												slotInstType[curSlot]=sdat[keyPointer[(chInstrument[i]<<7)+slotNote[curSlot]]];
												slotKeyPointer[curSlot]=keyPointer[(chInstrument[i]<<7)+slotNote[curSlot]];
											}else{
												slotSamplePointer[curSlot]=instPointer[chInstrument[i]<<7];
												slotInstType[curSlot]=sdat[keyPointer[(chInstrument[i]<<7)]];
												slotKeyPointer[curSlot]=keyPointer[(chInstrument[i]<<7)];
											}
											if(slotInstType[curSlot]==0 || slotInstType[curSlot]==8){//Sample
												if(slotInstType[curSlot]==0){
													slotPitchFill[curSlot]=slotPitch[curSlot]=FREQ_TABLE[60<<7];
												}else if(slotInstType[curSlot]==8){
													slotPitch[curSlot]=FREQ_TABLE[slotMidiFreq[curSlot]];
													slotPitchFill[curSlot]=0;
												}
												sampleDone[curSlot]=false;
												samplePos[curSlot]=0;
												sampleOutput[curSlot]=0;
												slotADSRState[curSlot]=0;
												slotADSRVol[curSlot]=-92544;
												slotPanL[curSlot]=max(chPanL[i]+PAN_TABLE[slotPan[curSlot]],-723);
												slotPanR[curSlot]=max(chPanR[i]+PAN_TABLE[127-slotPan[curSlot]],-723);
												samplePitchFill[curSlot]=sampleRate;
												if(sdat[slotSamplePointer[curSlot]+3]==0x40){
													sampleLoops[curSlot]=1;
													sampleLoop[curSlot]=NDS_getAddress(slotSamplePointer[curSlot]+8);
												}else{
													sampleLoops[curSlot]=0;
													sampleLoop[curSlot]=0;
												}
												samplePitch[curSlot]=NDS_getAddress(slotSamplePointer[curSlot]+4);
												samplePitch[curSlot]/=1024;
												sampleEnd[curSlot]=NDS_getAddress(slotSamplePointer[curSlot]+12);
												slotSamplePointer[curSlot]+=16;
											}else if((slotInstType[curSlot]&7)<3){//PSG
												slotPitchFill[curSlot]=slotPitch[curSlot]=FREQ_TABLE[72<<7];
												sampleDone[curSlot]=false;
												samplePos[curSlot]=0;
												sampleOutput[curSlot]=0;
												slotADSRState[curSlot]=0;
												slotADSRVol[curSlot]=-92544;
												slotPanL[curSlot]=max(chPanL[i]+PAN_TABLE[slotPan[curSlot]],-723);
												slotPanR[curSlot]=max(chPanR[i]+PAN_TABLE[127-slotPan[curSlot]],-723);
												samplePitchFill[curSlot]=sampleRate;
												sampleLoops[curSlot]=1;
												sampleLoop[curSlot]=0;
												samplePitch[curSlot]=0x20B7;
												sampleEnd[curSlot]=16;
												slotSamplePointer[curSlot]=sdat[slotKeyPointer[curSlot]+4]*0x10;
											}else if((slotInstType[curSlot]&7)==3){//Wav
												slotPitchFill[curSlot]=slotPitch[curSlot]=FREQ_TABLE[72<<7];
												sampleDone[curSlot]=false;
												samplePos[curSlot]=0;
												sampleOutput[curSlot]=0;
												slotADSRState[curSlot]=0;
												slotADSRVol[curSlot]=-92544;
												slotPanL[curSlot]=max(chPanL[i]+PAN_TABLE[slotPan[curSlot]],-723);
												slotPanR[curSlot]=max(chPanR[i]+PAN_TABLE[127-slotPan[curSlot]],-723);
												samplePitchFill[curSlot]=sampleRate;
												sampleLoops[curSlot]=1;
												sampleLoop[curSlot]=0;
												samplePitch[curSlot]=0x20B7;
												sampleEnd[curSlot]=16;
												slotWavNibble[curSlot]=0;
											}else{
												sampleDone[curSlot]=true;
											}

                                        }else{
                                            temp[1]=0;
                                            if(temp[0]>=0xD0) temp[1]=(DELTA_TIME[temp[0]-0xD0]);
                                            if(sdat[filePos]<0x80){//note argument provided?
												lastNote[i]=sdat[filePos++];
												chPointer[i]++;
											}
                                            if(sdat[filePos]<0x80){//velocity argument provided?
												lastVel[i]=sdat[filePos++];
												chPointer[i]++;
											}
                                            if(temp[1]>0){//Optional delta-time
												if(sdat[filePos]<0x80){
													temp[1]+=sdat[filePos++];
													chPointer[i]++;
												}
											}
                                        }
                                        chPointer[i]++;
                                    }else if(temp[0]>=0x80 && temp[0]<=0xB0){//Delta-Time
                                        if(temp[0]>0x80) chDelay[i]+=DELTA_TIME[temp[0]-0x81];
                                        delayHit=true;
                                        chPointer[i]++;
                                    }else{
                                        printf("Illegal Instruction 0x%X at: %X\n",temp[0],filePos-1);
                                        running=false;
                                        delayHit=true;
                                    }
                                break;
                            }
                            chPointer[i]-=repeated;
                        }
                    }
                    if(chDelay[i]>0) chDelay[i]-=1;//change this
                }
            }
            for(int i=0; i<16; i++){                
				if(slotNoteLen[i]>0){
                    slotNoteLen[i]-=1;//change this
                    if(slotNoteLen[i]==0){
						if(slotADSRState[i]<3){
							slotFree[i]=0;
							sampleDone[i]=true;
							slotADSRState[i]=3;
							slotADSRVol[i]=slotSustain[i];
						}
					}
                }
            }
        }
        /*for(int i=0; i<16; i++){
            
            switch(slotADSRState[i]){
                case 0:
                    slotADSRVol[i]=((slotADSRVol[i]*slotAttack[i])/255);
                    if(slotADSRVol[i]<0) break; 
                    slotADSRState[i]++;
                break;
                case 1:
                    slotADSRVol[i]-=slotDecay[i];
                    if(slotADSRVol[i]>slotSustain[i]) break;
                    slotADSRState[i]++;
                case 2:
                    slotADSRVol[i]=slotSustain[i];
                break;
                case 3:
                    slotADSRVol[i]-=slotRelease[i];
                    if(slotADSRVol[i]>-92544) break;
                    slotADSRVol[i]=-92544;
                    slotADSRState[i]++;
                    sampleDone[i]=true;
                    slotFree[i]=0;
                break;
            }
            volModL[i]=NDS_Cnv_Vol(fadeVol+slotNoteVel[i]+chVol[slotChannel[i]]+slotPanL[i]);
            volModR[i]=NDS_Cnv_Vol(fadeVol+slotNoteVel[i]+chVol[slotChannel[i]]+slotPanR[i]);
            //volModL[i]=NDS_Cnv_Vol(fadeVol+slotNoteVel[i]+(slotADSRVol[i]>>7)+chVol[slotChannel[i]]+slotPanL[i]);
            //volModR[i]=NDS_Cnv_Vol(fadeVol+slotNoteVel[i]+(slotADSRVol[i]>>7)+chVol[slotChannel[i]]+slotPanR[i]);
        }*/
    }
    mixer[0]=0;
    mixer[1]=0;
    for(int i=0; i<16; i++){//process each slot
        if(slotFree[i]!=0 && !sampleDone[i]){
            signed long tune=slotMidiFreq[i]+chPitchBendCur[slotChannel[i]];
            if(tune<0) tune=0;
            if(tune>=(128*128)){
                printf("Freq to high:%X\n",tune);
                tune=(128*128)-1;
            }
            tune=FREQ_TABLE[tune];
            slotPitchFill[i]+=tune;
            while(slotPitchFill[i]>=slotPitch[i]){
                slotPitchFill[i]-=slotPitch[i];
                samplePitchFill[i]+=samplePitch[i];
                while(samplePitchFill[i]>=sampleRate){
                    samplePitchFill[i]-=sampleRate;
                    if((slotInstType[i]&7)==3){//Wav
						temp[0]=sdat[samplePos[i]+slotSamplePointer[i]];
						if(slotWavNibble[i]==0){
							samplePos[i]--;
							slotWavNibble[i]=1;
							temp[0]&=0xF0;
						}else{
							slotWavNibble[i]=0;
							temp[0]<<=4;
						}
						sampleOutput[i]=temp[0]-0x80;
					}else if(slotSamplePointer[i]<=0x40){//PSG
						sampleOutput[i]=PSG[samplePos[i]+slotSamplePointer[i]];
					}else{//sample
						if(sdat[samplePos[i]+slotSamplePointer[i]]>=0x80){
							sampleOutput[i]=(sdat[samplePos[i]+slotSamplePointer[i]])-0x100;
						}else{
							sampleOutput[i]=sdat[samplePos[i]+slotSamplePointer[i]];
						}
					}
                    samplePos[i]++;
                    if(samplePos[i]>=sampleEnd[i]){
                        if(sampleLoops[i]==1){
                            samplePos[i]=sampleLoop[i];
                        }else{
                            sampleDone[i]=true;
                            slotFree[i]=0;
                        }
                    }

                }
            }			
            mixer[0]+=(sampleOutput[i]*0x1000);//*volModL[i]);//left
            mixer[1]+=(sampleOutput[i]*0x1000);//*volModR[i]);//right
        }
    }
    soundOut[0]=((mixer[0]) & 0xFF);
    soundOut[1]=((mixer[0]>>8) & 0xFF);
    soundOut[2]=((mixer[0]>>16) & 0xFF);
    soundOut[3]=((mixer[1]) & 0xFF);
    soundOut[4]=((mixer[1]>>8) & 0xFF);
    soundOut[5]=((mixer[1]>>16) & 0xFF);
	return soundOut;
}
