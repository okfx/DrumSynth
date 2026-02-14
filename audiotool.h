#ifndef AUDIOTOOL_H
#define AUDIOTOOL_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       d3W2;           //xy=289.4167785644531,1315.0000224113464
AudioSynthWaveform       d3W4;           //xy=289.4167785644531,1354.0000224113464
AudioSynthWaveform       d3606W1;        //xy=377.4167785644531,1075.0000224113464
AudioSynthWaveform       d3606W2;        //xy=377.4167785644531,1107.0000224113464
AudioSynthWaveform       d3606W3;        //xy=377.4167785644531,1137.0000224113464
AudioSynthWaveform       d3606W4;        //xy=377.4167785644531,1167.0000224113464
AudioSynthWaveform       d3606W5;        //xy=377.4167785644531,1198.0000224113464
AudioSynthWaveform       d3606W6;        //xy=377.4167785644531,1229.0000224113464
AudioSynthWaveformModulated d3W3;           //xy=412.4167785644531,1360.0000224113464
AudioSynthWaveformModulated d3W1;           //xy=415.4167785644531,1321.0000224113464
AudioEffectEnvelope      d1PitchEnv;     //xy=502.4167785644531,162.00002241134644
AudioSynthWaveformDc     d1DC;           //xy=503.4167785644531,121.00002241134644
AudioSynthNoiseWhite     clapNoise2;     //xy=507.4167785644531,998.0000224113464
AudioMixer4              d3606Mixer1;    //xy=547.4167785644531,1122.0000224113464
AudioMixer4              d3606Mixer2;    //xy=547.4167785644531,1184.0000224113464
AudioMixer4              d3Mixer1;       //xy=553.4167785644531,1367.0000224113464
AudioMixer4              d3Mixer2;       //xy=553.4167785644531,1431.0000224113464
AudioSynthNoiseWhite     clapNoise1;     //xy=573.4167785644531,807.0000224113464
AudioSynthWaveformModulated d2b;            //xy=634.4167785644531,382.00002241134644
AudioEffectEnvelope      clap2AmpEnv;    //xy=665.4167785644531,997.0000224113464
AudioSynthWaveformModulated d1b;            //xy=715.4167785644531,147.00002241134644
AudioSynthWaveformModulated d1;             //xy=716.4167785644531,108.00002241134644
AudioSynthWaveformModulated d1c;            //xy=716.4167785644531,182.00002241134644
AudioSynthWaveformModulated d1d;            //xy=716.4167785644531,218.00002241134644
AudioMixer4              d3606MasterMixer; //xy=726.4167785644531,1157.0000224113464
AudioEffectEnvelope      clap1AmpEnv;    //xy=731.4167785644531,807.0000224113464
AudioMixer4              d3MasterMixer;  //xy=736.4167785644531,1415.0000224113464
AudioEffectEnvelope      d2bAmpEnv;      //xy=785.4167785644531,382.00002241134644
AudioFilterStateVariable clap2Filter;    //xy=832.4167785644531,1005.0000224113464
AudioSynthNoiseWhite     d2Noise;        //xy=860.4167785644531,540.0000224113464
AudioFilterStateVariable clap1Filter;    //xy=890.4167785644531,814.0000224113464
AudioFilterStateVariable d3606HPF;       //xy=895.4167785644531,1158.0000224113464
AudioFilterStateVariable d3BPF;          //xy=907.4167785644531,1416.0000224113464
AudioMixer4              d1OscMixer;     //xy=915.4167785644531,163.00002241134644
AudioSynthWaveformModulated d2;             //xy=943.4167785644531,389.00002241134644
AudioEffectEnvelope      d2NoiseEnvelope; //xy=1029.4167785644531,540.0000224113464
AudioFilterStateVariable d3606BPF;       //xy=1048.4167785644531,1177.0000224113464
AudioEffectDelay         clapDelay1;     //xy=1052.4167785644531,814.0000224113464
AudioEffectDelay         clapDelay2;     //xy=1053.4167785644531,936.0000224113464
AudioSynthSimpleDrum     drum3;          //xy=1058.4167785644531,1552.0000224113464
AudioEffectEnvelope      d1AmpEnv;       //xy=1067.4167785644531,164.00002241134644
AudioFilterStateVariable d3Filter;       //xy=1064.4167785644531,1320.0000224113464
AudioEffectEnvelope      d2AmpEnv;       //xy=1101.4167785644531,389.00002241134644
AudioEffectEnvelope      d3AmpEnv;       //xy=1112.4167785644531,1414.0000224113464
AudioSynthSimpleDrum     d2Attack;       //xy=1182.4167785644531,645.0000224113464
AudioFilterStateVariable d1LowPass;      //xy=1222.4167785644531,171.00002241134644
AudioEffectEnvelope      d3606AmpEnv;    //xy=1220.4167785644531,1177.0000224113464
AudioFilterStateVariable d2NoiseFilter;  //xy=1255.4167785644531,540.0000224113464
AudioSynthSimpleDrum     drum2;          //xy=1275.4167785644531,485.00002241134644
AudioMixer4              clapMixer1;     //xy=1277.4167785644531,788.0000224113464
AudioMixer4              clapMixer2;     //xy=1281.4167785644531,910.0000224113464
AudioFilterStateVariable d2AttackFilter; //xy=1329.4167785644531,652.0000224113464
AudioSynthSimpleDrum     drum1;          //xy=1397.4167785644531,196.00002241134644
AudioSynthWaveformDc     d1DCwf;         //xy=1431.4167785644531,291.00002241134644
AudioMixer4              d3WfMixer;      //xy=1428.4167785644531,1472.0000224113464
AudioSynthWaveformDc     d3DCwf;         //xy=1429.4167785644531,1630.0000224113464
AudioAmplifier           clapAmp;        //xy=1471.4167785644531,786.0000224113464
AudioFilterStateVariable clapMasterFilter; //xy=1481.4167785644531,857.0000224113464
AudioMixer4              d2Mixer;        //xy=1512.4167785644531,492.00002241134644
AudioAmplifier           drum1Amp;       //xy=1568.4167785644531,196.00002241134644
AudioSynthWaveformSine   d2WfSine;       //xy=1571.4167785644531,943.0000224113464
AudioEffectEnvelope      clapMasterEnv;  //xy=1588.4167785644531,634.0000224113464
AudioEffectWaveFolder    d1Wavefolder;   //xy=1595.4167785644531,285.00002241134644
AudioFilterStateVariable d2Filter;       //xy=1604.4167785644531,589.0000224113464
AudioEffectWaveFolder    d3Wavefolder;   //xy=1656.4167785644531,1479.0000224113464
AudioEffectFreeverb      d2Verb;         //xy=1684.4167785644531,425.00002241134644
AudioAmplifier           d2WfAmp;        //xy=1716.4167785644531,943.0000224113464
AudioMixer4              d1Mixer;        //xy=1759.4167785644531,205.00002241134644
AudioMixer4              snareClapMixer; //xy=1845.4167785644531,640.0000224113464
AudioEffectWaveFolder    d2Wavefolder;   //xy=1850.4167785644531,868.0000224113464
AudioMixer4              d3Mixer;        //xy=1882.4167785644531,1283.0000224113464
AudioMixer4              d2MasterMixer;  //xy=1886.4167785644531,431.00002241134644
AudioFilterStateVariable d1Filter;       //xy=1919.4167785644531,212.00002241134644
AudioFilterStateVariable d2WfLowpass;    //xy=1929.4167785644531,794.0000224113464
AudioFilterBiquad        d1EQ;           //xy=2082.416778564453,223.00002241134644
AudioMixer4              drumMixer;      //xy=2095.416778564453,436.00002241134644
AudioFilterLadder        d3MasterFilter; //xy=2109.416778564453,1161.0000224113464
AudioSynthWaveform       masterWfWaveform; //xy=2230.416778564453,589.0000224113464
AudioAmplifier           d1Amp;          //xy=2233.416778564453,224.00002241134644
AudioEffectWaveFolder    masterWf;       //xy=2237.416778564453,521.0000224113464
AudioMixer4              masterMixer;    //xy=2495.416778564453,448.00002241134644
AudioAmplifier           delayAmp;       //xy=2495.416778564453,551.0000224113464
AudioEffectDelay         masterDelay;    //xy=2496.416778564453,907.0000224113464
AudioFilterStateVariable delayFilter;    //xy=2497.416778564453,750.0000224113464
AudioMixer4              delayMixer;     //xy=2498.416778564453,643.0000224113464
AudioAnalyzePeak         masterPeak;     //xy=2704.0832443237305,448.00003242492676
AudioRecordQueue         scopeQueue;     //xy=2704,490
AudioFilterStateVariable masterHiPass;   //xy=2780.416778564453,549.0000224113464
AudioFilterLadder        masterLowPass;  //xy=2782.416778564453,611.0000224113464
AudioFilterStateVariable masterBandPass; //xy=2790.416778564453,676.0000224113464
AudioAmplifier           masterAmp;      //xy=2968.416778564453,676.0000224113464
AudioFilterBiquad        finalFilter;    //xy=3118.416778564453,675.0000224113464
AudioOutputI2S           i2s1;           //xy=3266.416778564453,675.0000224113464
AudioConnection          patchCord1(d3W2, 0, d3W1, 0);
AudioConnection          patchCord2(d3W4, 0, d3W3, 0);
AudioConnection          patchCord3(d3606W1, 0, d3606Mixer1, 0);
AudioConnection          patchCord4(d3606W2, 0, d3606Mixer1, 1);
AudioConnection          patchCord5(d3606W3, 0, d3606Mixer1, 2);
AudioConnection          patchCord6(d3606W4, 0, d3606Mixer1, 3);
AudioConnection          patchCord7(d3606W5, 0, d3606Mixer2, 0);
AudioConnection          patchCord8(d3606W6, 0, d3606Mixer2, 1);
AudioConnection          patchCord9(d3W3, 0, d3Mixer1, 1);
AudioConnection          patchCord10(d3W1, 0, d3Mixer1, 0);
AudioConnection          patchCord11(d1PitchEnv, 0, d1, 0);
AudioConnection          patchCord12(d1PitchEnv, 0, d1b, 0);
AudioConnection          patchCord13(d1PitchEnv, 0, d1c, 0);
AudioConnection          patchCord14(d1PitchEnv, 0, d1d, 0);
AudioConnection          patchCord15(d1DC, d1PitchEnv);
AudioConnection          patchCord16(clapNoise2, clap2AmpEnv);
AudioConnection          patchCord17(d3606Mixer1, 0, d3606MasterMixer, 0);
AudioConnection          patchCord18(d3606Mixer2, 0, d3606MasterMixer, 1);
AudioConnection          patchCord19(d3Mixer1, 0, d3MasterMixer, 0);
AudioConnection          patchCord20(d3Mixer2, 0, d3MasterMixer, 1);
AudioConnection          patchCord21(clapNoise1, clap1AmpEnv);
AudioConnection          patchCord22(d2b, d2bAmpEnv);
AudioConnection          patchCord23(clap2AmpEnv, 0, clap2Filter, 0);
AudioConnection          patchCord24(d1b, 0, d1OscMixer, 1);
AudioConnection          patchCord25(d1, 0, d1OscMixer, 0);
AudioConnection          patchCord26(d1c, 0, d1OscMixer, 2);
AudioConnection          patchCord27(d1d, 0, d1OscMixer, 3);
AudioConnection          patchCord28(d3606MasterMixer, 0, d3606HPF, 0);
AudioConnection          patchCord29(clap1AmpEnv, 0, clap1Filter, 0);
AudioConnection          patchCord30(d3MasterMixer, 0, d3BPF, 0);
AudioConnection          patchCord31(d2bAmpEnv, 0, d2, 0);
AudioConnection          patchCord32(clap2Filter, 1, clapDelay2, 0);
AudioConnection          patchCord33(d2Noise, d2NoiseEnvelope);
AudioConnection          patchCord34(clap1Filter, 1, clapDelay1, 0);
AudioConnection          patchCord35(d3606HPF, 2, d3606BPF, 0);
AudioConnection          patchCord36(d3BPF, 1, d3Filter, 0);
AudioConnection          patchCord37(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord38(d2, d2AmpEnv);
AudioConnection          patchCord39(d2NoiseEnvelope, 0, d2NoiseFilter, 0);
AudioConnection          patchCord40(d2NoiseEnvelope, 0, d2NoiseFilter, 1);
AudioConnection          patchCord41(d3606BPF, 1, d3606AmpEnv, 0);
AudioConnection          patchCord42(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord43(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord44(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord45(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord46(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord47(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord48(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord49(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord50(drum3, 0, d3WfMixer, 2);
AudioConnection          patchCord51(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord52(d3Filter, 2, d3AmpEnv, 0);
AudioConnection          patchCord53(d2AmpEnv, 0, d2Mixer, 0);
AudioConnection          patchCord54(d3AmpEnv, 0, d3WfMixer, 1);
AudioConnection          patchCord55(d2Attack, 0, d2AttackFilter, 0);
AudioConnection          patchCord56(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord57(d1LowPass, 0, d1Mixer, 0);
AudioConnection          patchCord58(d3606AmpEnv, 0, d3WfMixer, 0);
AudioConnection          patchCord59(d2NoiseFilter, 2, d2Mixer, 2);
AudioConnection          patchCord60(drum2, 0, d2Mixer, 1);
AudioConnection          patchCord61(clapMixer1, 0, clapMasterFilter, 0);
AudioConnection          patchCord62(clapMixer2, 0, clapMasterFilter, 0);
AudioConnection          patchCord63(d2AttackFilter, 1, d2Mixer, 3);
AudioConnection          patchCord64(drum1, drum1Amp);
AudioConnection          patchCord65(d1DCwf, 0, d1Wavefolder, 1);
AudioConnection          patchCord66(d3WfMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord67(d3WfMixer, 0, d3Mixer, 0);
AudioConnection          patchCord68(d3DCwf, 0, d3Wavefolder, 1);
AudioConnection          patchCord69(clapAmp, clapMasterEnv);
AudioConnection          patchCord70(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord71(d2Mixer, 0, d2Filter, 0);
AudioConnection          patchCord72(drum1Amp, 0, d1Mixer, 1);
AudioConnection          patchCord73(d2WfSine, d2WfAmp);
AudioConnection          patchCord74(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord75(d1Wavefolder, 0, d1Mixer, 3);
AudioConnection          patchCord76(d2Filter, 2, snareClapMixer, 0);
AudioConnection          patchCord77(d3Wavefolder, 0, d3Mixer, 1);
AudioConnection          patchCord78(d2Verb, 0, d2MasterMixer, 1);
AudioConnection          patchCord79(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord80(d1Mixer, 0, d1Filter, 0);
AudioConnection          patchCord81(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord82(snareClapMixer, d2Verb);
AudioConnection          patchCord83(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord84(snareClapMixer, 0, delayMixer, 1);
AudioConnection          patchCord85(d2Wavefolder, 0, d2WfLowpass, 0);
AudioConnection          patchCord86(d3Mixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord87(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord88(d1Filter, 2, d1EQ, 0);
AudioConnection          patchCord89(d2WfLowpass, 0, d2MasterMixer, 2);
AudioConnection          patchCord90(d1EQ, d1Amp);
AudioConnection          patchCord91(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord92(drumMixer, 0, masterWf, 0);
AudioConnection          patchCord93(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord94(d3MasterFilter, 0, delayMixer, 2);
AudioConnection          patchCord95(masterWfWaveform, 0, masterWf, 1);
AudioConnection          patchCord96(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord97(d1Amp, 0, delayMixer, 0);
AudioConnection          patchCord98(masterWf, 0, masterMixer, 1);
AudioConnection          patchCord99(masterMixer, 0, masterHiPass, 0);
AudioConnection          patchCord100(masterMixer, masterPeak);
AudioConnection          patchCord112(masterMixer, 0, scopeQueue, 0);
AudioConnection          patchCord101(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord102(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord103(delayFilter, 0, delayMixer, 3);
AudioConnection          patchCord104(delayMixer, masterDelay);
AudioConnection          patchCord105(delayMixer, delayAmp);
AudioConnection          patchCord106(masterHiPass, 2, masterLowPass, 0);
AudioConnection          patchCord107(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord108(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord109(masterAmp, finalFilter);
AudioConnection          patchCord110(finalFilter, 0, i2s1, 0);
AudioConnection          patchCord111(finalFilter, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=1445.4167785644531,105.00002241134644
// GUItool: end automatically generated code

#endif
