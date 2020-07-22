//AdvancePlay 0.2 FroggestSpirit

u_int8_t read_byte(){
    return music[filePos++];
}

u_int16_t read_short(){
    return music[filePos++] + (music[filePos++] << 8);
}

u_int32_t read_long(){
    return music[filePos++] + (music[filePos++] << 8) + (music[filePos++] << 16) + (music[filePos++] << 24);
}

#include "lookuptables.h"
#include "lfsr.h"

bool loadSong(int id){ //loads a song
    int idi = id;
    songLoc = read_long_from((idi * 8) + 4);
    filePos = songLoc;
    songOffset = read_long();
    printf("Song Location:%X\n", songLoc);
    printf("Song Offset:%X\n", songOffset);    
    numTracks = read_long() & 0xFFFF;
    printf("Number of Tracks:%X\n", numTracks);
    tablePointer = read_long_from(songLoc + 8);
    printf("Table at:%X\n", tablePointer);
    return true;
}

unsigned long read_long_from(unsigned long addr){ //returns the 4 -byte address at addr
    filePos = addr;
    return read_long();
}

bool init(int songID, int sRate, bool inf){
    running = false;
    sampleRate = sRate;
    filePos = 0;
    //Set the output values properly
    delayHit = false;
    tempoFill = 0;
    usedTracks = 0xFFFF;
    sseqTempo = 120;
    sseqVol = 0x7F;
    fadeVol = 0;
    maxLoops = 2;
    infLoop = inf;
    mixer[0] = 0;
    mixer[1] = 0;
    soundOut[0] = 0;
    soundOut[1] = 0;
    
    musicSize = sizeof(music);
    printf("File Size:%d\n", musicSize);
    for(int i = 0; i < 16; i++){
        PSG[0x00 + i] = i < 2 ? 0xBF : 0x40;
        PSG[0x10 + i] = i < 4 ? 0xBF : 0x40;
        PSG[0x20 + i] = i < 8 ? 0xBF : 0x40;
        PSG[0x30 + i] = i < 12 ? 0xBF : 0x40;
    }

    //Read container header
    filePos = 0x00;
    totalSongs = numSongs = read_long();
    printf("Number of Songs: %X\n", numSongs);
    
    for (int i = 0; i < 16; i++) {
        timesLooped[i] = maxLoops;
        chPointer[i] = 0;
        slotChannel[i] = 0;
        slotFree[i] = 0;
        slotNote[i] = 0;
        slotNoteVel[i] = 0;
        slotNoteLen[i] = 0;
        chDelay[i] = 0;
        chInstrument[i] = 0;
        chPan[i] = 0x40;
        chPanL[i] = 0x40;
        chPanR[i] = 0x40;
        chVol[i] = 0x7F;
        chTranspose[i] = 0;
        chPitchBendCur[i] = 0;
        chPitchBend[i] = 0x40;
        chPitchBendRange[i] = 2;
        chSweepPitch[i] = 0;
        chPriority[i] = 0;
        chPolyMono[i] = 0;
        chTie[i] = 0;
        chPort[i] = 0;
        chPortOn[i] = 0;
        chPortTime[i] = 0;
        chModDelay[i] = 0;
        chModDepth[i] = 0;
        chModSpeed[i] = 0;
        chModType[i] = 0;
        chModRange[i] = 0;
        chAttack[i] = 0;
        chDecay[i] = 0;
        chSustain[i] = 0;
        chRelease[i] = 0;
        chLoopTimes[i] = 0;
        chLoopOffset[i] = 0;
        chReturnOffset[i] = 0;
        chInCall[i] = false;
    
        chActive[i] = false;
        slotSamplePointer[i] = 0;
        tempNibble = 0;
        slotMidiFreq[i] = 0;
        slotPitch[i] = 0;
        slotPitchFill[i] = 0;
        samplePitchFill[i] = 0;
        chInstrument[i] = 0;
        sampleDone[i] = true;
    }

    sseqIndex = songID;
    loadSong(sseqIndex);
    //read sseq header
    filePos = songLoc + 0x0c;
    for(int i = 0; i < numTracks; i++){
        chPointer[i] = read_long() - songOffset;
        chActive[i] = true;
        timesLooped[i] = 0;
        printf("Track Pointer %X: %X\n", i, chPointer[i]);
    }
    //read sbnk header
    filePos = tablePointer;
    int numKeySplits = 0;
    int numAllSplits = 0;
    for(int i = 0; i < 128; i++){
        keyPointer[i << 7] = filePos;
        keyIsSplit[i] = 0;
        switch(read_long() & 0xFF){
            case 0x00:
            case 0x08:
            case 0x03:
            case 0x0B: //Sample / Wav
                instPointer[i << 7] = read_long();
                filePos += 4;
            break;
            case 0x01:
            case 0x02:
            case 0x04:
            case 0x09:
            case 0x0A:
            case 0x0C: //PSG
                filePos += 8;
            break;
            case 0x40: //Key Split
                keyIsSplit[i] = 1;
                numKeySplits++;
                temp32 = read_long();
                keySplitPointer = read_long();
                printf("Key Split Keys at:%X Table at:%X\n", keySplitPointer, temp32);
                if(keySplitPointer < musicSize && temp32 < musicSize){
                    tempFilePos = filePos;                
                    filePos = keySplitPointer;
                    for(int ii = 0; ii < 128; ii++){//get sub instruments
                        keyPointer[(i << 7) + ii] = temp32 + (read_byte() * 12);
                    }
                    for(int ii = 0; ii < 128; ii++){
                        filePos = keyPointer[(i << 7) + ii] + 4;
                        instPointer[(i << 7) + ii] = read_long();
                    }
                    filePos = tempFilePos;
                }
            break;
            case 0x80: //Every Key Split
                keyIsSplit[i] = 1;
                numAllSplits++;
                temp32 = read_long();
                printf("Drum Split at:%X\n", temp32);
                if(temp32 < musicSize){
                    tempFilePos = filePos;
                    for(int ii = 0; ii < 128; ii++){
                        //get sub instruments
                        keyPointer[(i << 7) + ii] = temp32 + (ii * 12);
                        filePos = keyPointer[(i << 7) + ii] + 4;
                        instPointer[(i << 7) + ii] = read_long();
                    }
                    filePos = tempFilePos;
                }
                filePos += 4;
            break;
        }
    }
    printf("Number of Key Splits %X\n", numKeySplits);
    printf("Number of Drum Splits %X\n", numAllSplits);
    running = true;
    return true;
}

bool stop(){
    running = false;
    return true;
}

float *loop(){
    seqFrame = false;
    static int c = 0;
 //sample processing
    if (!running) return 0; //Nothing to do here!
    if(paused) return 0;
    c += 60;
    if(c >= sampleRate){ //player update
        c -= sampleRate;
        seqFrame = true;
        if(fadeVol < 0){
            fadeVol--;
            if(fadeVol <= - 723) running = false;
            if (!running) return 0;
        }
        tempoFill += sseqTempo;
        while(tempoFill >= 150){
            tempoFill -= 150;
            for(int i = 0; i < 16; i++){
                if(chActive[i]){
                    if(chDelay[i] == 0){
                        delayHit = false;
                        while(!delayHit){
                            filePos = chPointer[i];
                            if(music[filePos] < 0x80){
                                temp[0] = lastCommand[i];
                                repeated = 1;
                            }else{
                                temp[0] = read_byte();
                                repeated = 0;
                            }
                            //printf("Command:%X at:%X\n", temp[0], filePos);
                            switch(temp[0]){
                                case 0xB1: //End of Track
                                    chActive[i] = false;
                                    delayHit = true;
                                    timesLooped[i] = maxLoops;
                                    loopi = 0;
                                    while(timesLooped[loopi] >= maxLoops && loopi < 16) loopi++;
                                    if(loopi == 16) fadeVol = - 722;
                                break;
                                case 0xB2: //Jump
                                    chPointer[i] = read_long() - songOffset;
                                    if(timesLooped[i] < maxLoops)timesLooped[i]++;
                                    loopi = 0;
                                    while(timesLooped[loopi] >= maxLoops && loopi < 16) loopi++;
                                    if(loopi == 16 && fadeVol == 0 && !infLoop) fadeVol--;
                                break;
                                case 0xB3: //Call
                                    chReturnOffset[i] = chPointer[i] + 5;
                                    chInCall[i] = true;
                                    chPointer[i] = read_long() - songOffset;
                                break;
                                case 0xB4: //Return
                                    if(chInCall[i]){
                                        chPointer[i] = chReturnOffset[i];
                                        chInCall[i] = false;
                                    }else{
                                        chPointer[i]++;
                                    }
                                break;
                                case 0xBA: //Priority
                                    read_byte();
                                    
                                    chPointer[i] += 2;
                                break;
                                case 0xBB: //Tempo
                                    sseqTempo = (read_byte() << 1);
                                    chPointer[i] += 2;
                                break;
                                case 0xBC: //Transpose
                                    chTranspose[i] = read_byte();
                                    chPointer[i] += 2;
                                break;
                                case 0xBD: //Instrument Change
                                    chInstrument[i] = read_byte() & 0x7F;
                                    chPointer[i] += 2;
                                    lastCommand[i] = 0xBD;
                                break;
                                case 0xBE: //Volume
                                    chVol[i] = read_byte();
                                    chPointer[i] += 2;
                                    lastCommand[i] = 0xBE;
                                break;
                                case 0xBF: //Pan
                                    chPan[i] = read_byte();
                                    chPanL[i] = chPan[i];
                                    chPanR[i] = 127 - chPan[i];
                                    chPointer[i] += 2;
                                    lastCommand[i] = 0xBF;
                                break;
                                case 0xC0: //Pitch Bend
                                    chPitchBend[i] = read_byte();
                                    chPointer[i] += 2;
                                    chPitchBendCur[i] = ((chPitchBend[i] - 0x40) * (chPitchBendRange[i] << 1));
                                    
                                    //if(chPitchBend[i] > 0x40) chPitchBendCur[i] = ((chPitchBend[i] - 0x40) * chPitchBendRange[i]);
                                    //if(chPitchBend[i] < 0x40) chPitchBendCur[i] = (chPitchBend[i] * chPitchBendRange[i]);
                                    lastCommand[i] = 0xC0;
                                break;
                                case 0xC1: //Pitch Bend Range
                                    chPitchBendRange[i] = read_byte();
                                    chPointer[i] += 2;
                                    chPitchBendCur[i] = ((chPitchBend[i] - 0x40) * (chPitchBendRange[i] << 1));
                                    //if(chPitchBend[i] > 128) chPitchBendCur[i] = ((chPitchBend[i] - 256) * chPitchBendRange[i]);
                                    //if(chPitchBend[i] <= 128) chPitchBendCur[i] = (chPitchBend[i] * chPitchBendRange[i]);
                                    lastCommand[i] = 0xC1;
                                break;
                                case 0xC2: //LFO Speed
                                    chPointer[i] += 2;
                                break;
                                case 0xC3: //LFO delay
                                    chPointer[i] += 2;
                                break;
                                case 0xC4: //LFO depth
                                    chPointer[i] += 2;
                                    lastCommand[i] = 0xC4;
                                break;
                                case 0xC5: //LFO type
                                    chPointer[i] += 2;
                                break;
                                case 0xC8: //detune
                                    chPointer[i] += 2;
                                    lastCommand[i] = 0xC8;
                                break;
                                case 0xCD: //psudo echo
                                    chPointer[i] += 3;
                                    lastCommand[i] = 0xCD;
                                break;
                                case 0xCE: //Note Off
                                    //printf("OFF:%X\n", i);
                                    if(music[filePos] < 0x80){ //note argument provided?
                                        temp[0] = read_byte();
                                        chPointer[i]++;
                                    }else{
                                        temp[0] = lastNote[i];
                                    }
                                    lastNote[i] = temp[0];
                                    tempNote = temp[0];
                                    if(music[filePos] < 0x80){ //velocity argument provided?
                                        temp[0] = read_byte();
                                        chPointer[i]++;
                                    }else{
                                        temp[0] = lastVel[i];
                                    }
                                    lastVel[i] = temp[0];
                                    tempVel = temp[0];
                                    
                                    for(int slot = 0; slot < 16; slot++){
                                        if(slotChannel[slot] == i){ //find slots used by this channel
                                            if(slotNote[slot] == tempNote){
                                                slotFree[slot] = 0;
                                                slotADSRState[slot] = 3;
                                                slotADSRVol[slot] = slotSustain[slot];
                                            }
                                        }
                                    }
                                    lastCommand[i] = 0xCE;
                                    chPointer[i]++;
                                break;
                                default:
                                    if(temp[0] >= 0xCF){ //Note on
                                        lastCommand[i] = temp[0];
                                        //printf("ON:%X Command:%X\n", i, temp[0]);
                                        curSlot = 0xFF;
                                        for(int slot = 0; slot < 16; slot++){
                                            if(slotFree[slot] == 0){ //check for free slots
                                                curSlot = slot;
                                                slot = 16;
                                            }
                                        }
                                        if(curSlot == 0xFF){
                                            for(int slot = 0; slot < 16; slot++){
                                                if(slotFree[slot] == 1){ //if free slots are unavailable, check for slot in release state
                                                    curSlot = slot;
                                                    slot = 16;
                                                }
                                            }
                                        }
                                        if(curSlot == 0xFF){
                                            for(int slot = 0; slot < 16; slot++){
                                                if(slotFree[slot] == 2){ //if free slots are unavailable, pick a slot
                                                    curSlot = slot;
                                                    slot = 16;
                                                }
                                            }
                                        }
                                        if(curSlot < 0xFF){
                                            //curSlot = i; //remove once track priority is implemented
                                            slotChannel[curSlot] = i;
                                            slotFree[curSlot] = 2;
                                            slotNoteLen[curSlot] = 0;
                                            if(temp[0] >= 0xD0){
                                                slotNoteLen[curSlot] = (DELTA_TIME[temp[0] - 0xD0]);
                                            }
                                            if(music[filePos] < 0x80){ //note argument provided?
                                                temp[0] = read_byte();
                                                chPointer[i]++;
                                            }else{
                                                temp[0] = lastNote[i];
                                            }
                                            lastNote[i] = temp[0];
                                            slotNote[curSlot] = temp[0];
                                            slotMidiFreq[curSlot] = ((slotNote[curSlot] + chTranspose[i]) << 7);
                                            if(music[filePos] < 0x80){ //velocity argument provided?
                                                temp[0] = read_byte();
                                                chPointer[i]++;
                                            }else{
                                                temp[0] = lastVel[i];
                                            }
                                            lastVel[i] = temp[0];
                                            slotNoteVel[curSlot] = temp[0];
                                            if(slotNoteLen[curSlot] > 0){ //Optional delta - time
                                                if(music[filePos] < 0x80){
                                                    slotNoteLen[curSlot] += read_byte();
                                                    chPointer[i]++;
                                                }
                                            }
                                            if(keyIsSplit[chInstrument[i]] == 1){
                                                slotSamplePointer[curSlot] = instPointer[(chInstrument[i] << 7) + slotNote[curSlot]];
                                                slotInstType[curSlot] = music[keyPointer[(chInstrument[i] << 7) + slotNote[curSlot]]];
                                                slotKeyPointer[curSlot] = keyPointer[(chInstrument[i] << 7) + slotNote[curSlot]];
                                            }else{
                                                slotSamplePointer[curSlot] = instPointer[chInstrument[i] << 7];
                                                slotInstType[curSlot] = music[keyPointer[(chInstrument[i] << 7)]];
                                                slotKeyPointer[curSlot] = keyPointer[(chInstrument[i] << 7)];
                                            }
                                            //if(chPitchBendCur[i] != 0) printf("%X Pitch:%X\n", i, chPitchBendCur[i]);
                                            if(slotInstType[curSlot] == 0 || slotInstType[curSlot] == 8){ //Sample
                                                if(slotInstType[curSlot] == 0){
                                                    slotPitchFill[curSlot] = slotPitch[curSlot] = FREQ_TABLE[music[slotKeyPointer[curSlot] + 1] << 7];
                                                }else{
                                                    slotPitch[curSlot] = FREQ_TABLE[slotMidiFreq[curSlot]];
                                                    slotPitchFill[curSlot] = 0;
                                                }
                                                sampleDone[curSlot] = false;
                                                samplePos[curSlot] = 0;
                                                sampleOutput[curSlot] = 0;
                                                slotADSRState[curSlot] = 0;
                                                slotADSRVol[curSlot] = 0;
                                                slotAttack[curSlot] = music[slotKeyPointer[curSlot] + 8];
                                                slotDecay[curSlot] = music[slotKeyPointer[curSlot] + 9];
                                                slotSustain[curSlot] = music[slotKeyPointer[curSlot] + 10];
                                                slotRelease[curSlot] = music[slotKeyPointer[curSlot] + 11];
                                                slotPanL[curSlot] = chPanL[i];
                                                slotPanR[curSlot] = chPanR[i];
                                                samplePitchFill[curSlot] = sampleRate;
                                                if(music[slotSamplePointer[curSlot] + 3] == 0x40){
                                                    sampleLoops[curSlot] = 1;
                                                    sampleLoop[curSlot] = float(read_long_from(slotSamplePointer[curSlot] + 8));
                                                }else{
                                                    sampleLoops[curSlot] = 0;
                                                    sampleLoop[curSlot] = 0.0f;
                                                }
                                                samplePitch[curSlot] = read_long_from(slotSamplePointer[curSlot] + 4);
                                                samplePitch[curSlot] /= 1024;
                                                sampleEnd[curSlot] = float(read_long_from(slotSamplePointer[curSlot] + 12));
                                                sampleLoopLength[curSlot] = sampleEnd[curSlot] - sampleLoop[curSlot];
                                                slotSamplePointer[curSlot] += 16;
                                            }else if((slotInstType[curSlot] & 7) < 3){ //PSG
                                                slotPitchFill[curSlot] = slotPitch[curSlot] = FREQ_TABLE[(music[slotKeyPointer[curSlot] + 1] + 12) << 7];
                                                sampleDone[curSlot] = false;
                                                samplePos[curSlot] = 0;
                                                sampleOutput[curSlot] = 0;
                                                slotADSRState[curSlot] = 0;
                                                slotADSRVol[curSlot] = 0;
                                                slotAttack[curSlot] = (7 - music[slotKeyPointer[curSlot] + 8]) * 0x24;
                                                if(slotAttack[curSlot] == 0) slotAttack[curSlot] = 1;
                                                slotDecay[curSlot] = (7 - music[slotKeyPointer[curSlot] + 9]) * 0x24;
                                                if(slotDecay[curSlot] == 0) slotDecay[curSlot] = 1;
                                                slotSustain[curSlot] = (music[slotKeyPointer[curSlot] + 10]) * 0x11;
                                                slotRelease[curSlot] = (7 - music[slotKeyPointer[curSlot] + 11]) * 0x24;
                                                if(slotRelease[curSlot] == 0) slotRelease[curSlot] = 1;
                                                slotPanL[curSlot] = chPanL[i];
                                                slotPanR[curSlot] = chPanR[i];
                                                samplePitchFill[curSlot] = sampleRate;
                                                sampleLoops[curSlot] = 1;
                                                sampleLoop[curSlot] = 0.0f;
                                                samplePitch[curSlot] = 0x20B7;
                                                sampleEnd[curSlot] = 16.0f;
                                                sampleLoopLength[curSlot] = 16.0f;
                                                slotSamplePointer[curSlot] = music[slotKeyPointer[curSlot] + 4] * 0x10;
                                            }else if((slotInstType[curSlot] & 7) == 3){ //Wav
                                                slotPitchFill[curSlot] = slotPitch[curSlot] = FREQ_TABLE[(music[slotKeyPointer[curSlot] + 1] + 12) << 7];
                                                sampleDone[curSlot] = false;
                                                samplePos[curSlot] = 0;
                                                sampleOutput[curSlot] = 0;
                                                slotADSRState[curSlot] = 0;
                                                slotADSRVol[curSlot] = 0;
                                                slotAttack[curSlot] = (7 - music[slotKeyPointer[curSlot] + 8]) * 0x24;
                                                if(slotAttack[curSlot] == 0) slotAttack[curSlot] = 1;
                                                slotDecay[curSlot] = (7 - music[slotKeyPointer[curSlot] + 9]) * 0x24;
                                                if(slotDecay[curSlot] == 0) slotDecay[curSlot] = 1;
                                                slotSustain[curSlot] = (music[slotKeyPointer[curSlot] + 10]) * 0x11;
                                                slotRelease[curSlot] = (7 - music[slotKeyPointer[curSlot] + 11]) * 0x24;
                                                if(slotRelease[curSlot] == 0) slotRelease[curSlot] = 1;
                                                slotPanL[curSlot] = chPanL[i];
                                                slotPanR[curSlot] = chPanR[i];
                                                samplePitchFill[curSlot] = sampleRate;
                                                sampleLoops[curSlot] = 1;
                                                sampleLoop[curSlot] = 0.0f;
                                                samplePitch[curSlot] = 0x20B7;
                                                sampleEnd[curSlot] = 32.0f;
                                                sampleLoopLength[curSlot] = 32.0f;
                                                slotWavNibble[curSlot] = 0;
                                            }else if((slotInstType[curSlot] & 7) == 4){ //Noise
                                                slotPitchFill[curSlot] = slotPitch[curSlot] = 1; //FREQ_TABLE[(music[slotKeyPointer[curSlot] + 1] + 12) << 7];
                                                sampleDone[curSlot] = false;
                                                samplePos[curSlot] = 0;
                                                sampleOutput[curSlot] = 0;
                                                slotADSRState[curSlot] = 0;
                                                slotADSRVol[curSlot] = 0;
                                                slotAttack[curSlot] = (7 - music[slotKeyPointer[curSlot] + 8]) * 0x24;
                                                if(slotAttack[curSlot] == 0) slotAttack[curSlot] = 1;
                                                slotDecay[curSlot] = (7 - music[slotKeyPointer[curSlot] + 9]) * 0x24;
                                                if(slotDecay[curSlot] == 0) slotDecay[curSlot] = 1;
                                                slotSustain[curSlot] = (music[slotKeyPointer[curSlot] + 10]) * 0x11;
                                                slotRelease[curSlot] = (7 - music[slotKeyPointer[curSlot] + 11]) * 0x24;
                                                if(slotRelease[curSlot] == 0) slotRelease[curSlot] = 1;
                                                slotPanL[curSlot] = chPanL[i];
                                                slotPanR[curSlot] = chPanR[i];
                                                samplePitchFill[curSlot] = sampleRate;
                                                sampleLoops[curSlot] = 1;
                                                sampleLoop[curSlot] = 0.0f;
                                                samplePitch[curSlot] = 0x6E0;
                                                slotSamplePointer[curSlot] = music[slotKeyPointer[curSlot] + 4];
                                                if(slotSamplePointer[curSlot]) sampleEnd[curSlot] = 0x7F;
                                                if(!slotSamplePointer[curSlot]) sampleEnd[curSlot] = 0x7FFF;
                                                sampleLoopLength[curSlot] = sampleEnd[curSlot];
                                            }else{
                                                sampleDone[curSlot] = true;
                                                //printf("%X Invalid Inst Type?:%X\n", i, slotInstType[curSlot]);
                                            }

                                        }else{
                                            temp[1] = 0;
                                            if(temp[0] >= 0xD0) temp[1] = (DELTA_TIME[temp[0] - 0xD0]);
                                            if(music[filePos] < 0x80){ //note argument provided?
                                                lastNote[i] = read_byte();
                                                chPointer[i]++;
                                            }
                                            if(music[filePos] < 0x80){ //velocity argument provided?
                                                lastVel[i] = read_byte();
                                                chPointer[i]++;
                                            }
                                            if(temp[1] > 0){ //Optional delta - time
                                                if(music[filePos] < 0x80){
                                                    temp[1] += read_byte();
                                                    chPointer[i]++;
                                                }
                                            }
                                        }
                                        chPointer[i]++;
                                    }else if(temp[0] >= 0x80 && temp[0] <= 0xB0){ //Delta - Time
                                        if(temp[0] > 0x80) chDelay[i] += DELTA_TIME[temp[0] - 0x81];
                                        delayHit = true;
                                        chPointer[i]++;
                                    }else{
                                        printf("Illegal Instruction 0x%X at: %X\n", temp[0], filePos - 1);
                                        running = false;
                                        delayHit = true;
                                    }
                                break;
                            }
                            chPointer[i] -= repeated;
                        }
                    }
                    if(chDelay[i] > 0) chDelay[i] -= 1; //change this
                }
            }
            for(int i = 0; i < 16; i++){
                if(slotNoteLen[i] > 0){
                    slotNoteLen[i] -= 1; //change this
                    if(slotNoteLen[i] == 0){
                                slotFree[i] = 0;
                                slotADSRState[i] = 3;
                                slotADSRVol[i] = slotSustain[i];
                        }
                }
            }
        }
        for(int i = 0; i < 16; i++){
            
            switch(slotADSRState[i]){
                case 0:
                    slotADSRVol[i] += slotAttack[i];
                    if(slotADSRVol[i] < 0xFF) break; 
                    slotADSRState[i]++;
                    slotADSRVol[i] = 0xFF;
                break;
                case 1:
                    slotADSRVol[i] -= slotDecay[i];
                    if(slotADSRVol[i] > slotSustain[i]) break;
                    slotADSRState[i]++;
                    slotADSRVol[i] = slotSustain[i];
                case 2:
                    slotADSRVol[i] = slotSustain[i];
                break;
                case 3:
                    slotADSRVol[i] -= slotRelease[i];
                    if(slotADSRVol[i] > 0) break;
                    slotADSRVol[i] = 0;
                    slotADSRState[i]++;
                    sampleDone[i] = true;
                    slotFree[i] = 0;
                break;
            }
            volModL[i] = (slotNoteVel[i] / 127.0f) * (chVol[slotChannel[i]] / 127.0f) * (slotADSRVol[i] / 255.0f);
            volModR[i] = volModL[i] * (slotPanR[i] / 127.0f);
            volModL[i] *= (slotPanL[i] / 127.0f);
        }
    }
    mixer[0] = 0;
    mixer[1] = 0;
    for(int i = 0; i < 16; i++){ //process each slot
        if(slotFree[i] != 0 && !sampleDone[i]){
            signed long tune = slotMidiFreq[i] + chPitchBendCur[slotChannel[i]];
            if(tune < 0) tune = 0;
            if(tune >= (128 * 128)){
                printf("Freq to high:%X\n", tune);
                tune = (128 * 128) - 1;
            }
            tune = FREQ_TABLE[tune];
                if((slotInstType[i] & 7) == 3){ //Wav
                    temp[0] = music[int((samplePos[i] / 2) + slotSamplePointer[i])];
                    if(fmod((samplePos[i] / 2), 1) < 0.5f){
                            temp[0] &= 0xF0;
                    }else{
                            temp[0] <<= 4;
                    }
                    
                    sampleOutput[i] = (temp[0] - 0x80) / 1024.0f;
                }else if((slotInstType[i] & 7) == 4){ //PSG Noise
                        soundChannel4Bit = int(samplePos[i]) & 7;
                        if(slotSamplePointer[i]) sampleOutput[i] = ((((lfsr7[int(samplePos[i] / 8)] << soundChannel4Bit) & 0x80) << 1) - 0x80) / 1024.0f;
                        if(!slotSamplePointer[i]) sampleOutput[i] = ((((lfsr15[int(samplePos[i] / 8)] << soundChannel4Bit) & 0x80) << 1) - 0x80) / 1024.0f;
                }else if(slotSamplePointer[i] <= 0x40){ //PSG
                        sampleOutput[i] = (PSG[int(samplePos[i] + slotSamplePointer[i])] - 0x80) / 1024.0f;
                }else{ //sample
                        sampleOutput[i] = music[int(samplePos[i] + slotSamplePointer[i])];
                        if(sampleOutput[i] >= 0x80) sampleOutput[i] -= 0x100;
                        sampleOutput[i] = (sampleOutput[i]) / 512.0f;
                }
                samplePos[i] += ((samplePitch[i] * (tune / slotPitch[i])) / sampleRate);
            if(samplePos[i] >= sampleEnd[i]){
                if(sampleLoops[i]){
                    samplePos[i] = sampleLoop[i] + fmod((samplePos[i] - sampleLoop[i]), sampleLoopLength[i]);
                }else{
                    sampleDone[i] = true;
                    slotFree[i] = 0;
                }
            }            
            mixer[0] += (sampleOutput[i] * volModL[i]); //left
            mixer[1] += (sampleOutput[i] * volModR[i]); //right
        }
    }
    soundOut[0] = mixer[0];
    soundOut[1] = mixer[1];
    return soundOut;
}
