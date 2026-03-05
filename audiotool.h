#ifndef AUDIOTOOL_H
#define AUDIOTOOL_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
// GUItool: begin automatically generated code
AudioSynthWaveformDc     d1PitchDC;           //xy=100,100
AudioSynthWaveformModulated d2Osc;             //xy=100,350
AudioSynthNoiseWhite     d2Noise;        //xy=100,400
AudioSynthNoiseWhite     clapNoise1;     //xy=100,630
AudioSynthNoiseWhite     clapNoise2;     //xy=100,700
AudioSynthWaveform       d3FmMod1;           //xy=100,830
AudioSynthWaveform       d3FmMod2;           //xy=100,880
AudioSynthWaveform       d3606Osc1;        //xy=100,930
AudioSynthWaveform       d3606Osc2;        //xy=100,980
AudioSynthWaveform       d3606Osc3;        //xy=100,1030
AudioEffectEnvelope      d1PitchEnv;     //xy=300,130
AudioEffectEnvelope      d2AmpEnv;       //xy=300,350
AudioEffectEnvelope      d2NoiseEnv; //xy=300,400
AudioSynthSimpleDrum     d2ClickTransient;       //xy=300,460
AudioEffectEnvelope      clapAmpEnv1;    //xy=300,630
AudioEffectEnvelope      clapAmpEnv2;    //xy=300,700
AudioSynthWaveformModulated d3FmCarrier1;           //xy=300,830
AudioSynthWaveformModulated d3FmCarrier2;           //xy=300,880
AudioSynthWaveform       d3606Osc4;        //xy=300,930
AudioSynthWaveform       d3606Osc5;        //xy=300,980
AudioSynthWaveform       d3606Osc6;        //xy=300,1030
AudioSynthWaveformModulated d1OscSine;             //xy=500,90
AudioSynthWaveformModulated d1OscSaw;            //xy=500,140
AudioSynthWaveformModulated d1OscSquare;            //xy=500,190
AudioSynthSimpleDrum     d2Body;         //xy=500,350
AudioFilterStateVariable d2NoiseFilter;  //xy=500,400
AudioFilterStateVariable d2ClickFilter; //xy=500,460
AudioFilterStateVariable clapFilter1;    //xy=500,630
AudioFilterStateVariable clapFilter2;    //xy=500,700
AudioMixer4              d3FmCarrierMixer;       //xy=500,850
AudioMixer4              d3606OscMixer1;    //xy=500,950
AudioMixer4              d3606OscMixer2;    //xy=500,1030
AudioMixer4              d1OscMixer;     //xy=700,140
AudioMixer4              d2VoiceMixer;        //xy=700,390
AudioEffectDelay         clapDelay1;     //xy=700,630
AudioEffectDelay         clapDelay2;     //xy=700,700
AudioMixer4              d3FmBusMixer;  //xy=700,850
AudioMixer4              d3606BusMixer; //xy=700,990
AudioEffectEnvelope      d1AmpEnv;       //xy=900,140
AudioFilterStateVariable d2VoiceHighPass;       //xy=900,390
AudioMixer4              clapMixer1;     //xy=900,630
AudioMixer4              clapMixer2;     //xy=900,700
AudioFilterStateVariable d3FmBandPass;          //xy=900,850
AudioFilterStateVariable d3606HighPass;       //xy=900,990
AudioFilterStateVariable d1LowPass;      //xy=1100,140
AudioSynthWaveformDc     d1WfDrive;         //xy=1100,230
AudioMixer4              clapBusMixer; //xy=1100,660
AudioFilterStateVariable d3FmHighPass;       //xy=1100,850
AudioFilterStateVariable d3606BandPass;       //xy=1100,990
AudioSynthSimpleDrum     d1Snap;          //xy=1300,80
AudioEffectWaveFolder    d1Wavefolder;   //xy=1300,210
AudioFilterStateVariable clapMasterFilter; //xy=1300,660
AudioEffectEnvelope      d3FmAmpEnv;       //xy=1300,850
AudioEffectEnvelope      d3606AmpEnv;    //xy=1300,990
AudioSynthSimpleDrum     d3Perc;          //xy=1300,1100
AudioAmplifier           d1SnapAmp;       //xy=1500,80
AudioMixer4              d1VoiceMixer;        //xy=1500,160
AudioSynthWaveformSine   d2WfSineOsc;       //xy=1500,480
AudioAmplifier           clapAmp;        //xy=1500,660
AudioMixer4              d3VoiceMixer;      //xy=1500,1100
AudioSynthWaveform       d3WfOsc;       //xy=1500,1190
AudioFilterStateVariable d1HighPass;       //xy=1700,160
AudioMixer4              snareClapMixer; //xy=1700,390
AudioAmplifier           d2WfAmp;        //xy=1700,480
AudioEffectEnvelope      clapMasterEnv;  //xy=1700,660
AudioEffectWaveFolder    d3Wavefolder;   //xy=1700,1140
AudioFilterBiquad        d1EQ;           //xy=1900,160
AudioEffectFreeverb      d2Reverb;         //xy=1900,330
AudioEffectWaveFolder    d2Wavefolder;   //xy=1900,480
AudioMixer4              d3MasterMixer;        //xy=1900,1130
AudioAmplifier           d1Amp;          //xy=2100,160
AudioMixer4              d2MasterMixer;  //xy=2100,380
AudioFilterStateVariable d2WfLowPass;    //xy=2100,480
AudioFilterLadder        d3MasterFilter; //xy=2100,1130
AudioSynthWaveform       masterWfOscSine;         //xy=2100,1360
AudioSynthWaveform       masterWfOscSaw;          //xy=2100,1410
AudioMixer4              drumMixer;      //xy=2300,1310
AudioMixer4              masterWfInputMixer;        //xy=2300,1390
AudioEffectWaveFolder    masterWavefolder;       //xy=2500,1340
AudioMixer4              masterMixer;    //xy=2700,1340
AudioMixer4              delaySendMixer;     //xy=2700,1480
AudioRecordQueue         scopeQueue;     //xy=2900,1280
AudioFilterStateVariable masterHighPass;   //xy=2900,1340
AudioAmplifier           delayAmp;       //xy=2900,1430
AudioEffectDelay         masterDelay;    //xy=2900,1540
AudioFilterLadder        masterLowPass;  //xy=3100,1340
AudioFilterStateVariable delayFilter;    //xy=3100,1540
AudioFilterStateVariable masterBandPass; //xy=3300,1340
AudioAmplifier           masterAmp;      //xy=3500,1340
AudioFilterBiquad        finalFilter;    //xy=3700,1340
AudioAmplifier           finalAmp;       //xy=3900,1340
AudioOutputI2S           i2s1;           //xy=4100,1340
AudioConnection          patchCord1(d3FmMod1, 0, d3FmCarrier1, 0);
AudioConnection          patchCord2(d3FmMod2, 0, d3FmCarrier2, 0);
AudioConnection          patchCord3(d3606Osc1, 0, d3606OscMixer1, 0);
AudioConnection          patchCord4(d3606Osc2, 0, d3606OscMixer1, 1);
AudioConnection          patchCord5(d3606Osc3, 0, d3606OscMixer1, 2);
AudioConnection          patchCord6(d3606Osc4, 0, d3606OscMixer1, 3);
AudioConnection          patchCord7(d3606Osc5, 0, d3606OscMixer2, 0);
AudioConnection          patchCord8(d3606Osc6, 0, d3606OscMixer2, 1);
AudioConnection          patchCord9(d3FmCarrier2, 0, d3FmCarrierMixer, 1);
AudioConnection          patchCord10(d3FmCarrier1, 0, d3FmCarrierMixer, 0);
AudioConnection          patchCord11(d1PitchEnv, 0, d1OscSine, 0);
AudioConnection          patchCord12(d1PitchEnv, 0, d1OscSaw, 0);
AudioConnection          patchCord13(d1PitchEnv, 0, d1OscSquare, 0);
AudioConnection          patchCord14(d1PitchDC, d1PitchEnv);
AudioConnection          patchCord15(clapNoise2, clapAmpEnv2);
AudioConnection          patchCord16(d3606OscMixer1, 0, d3606BusMixer, 0);
AudioConnection          patchCord17(d3606OscMixer2, 0, d3606BusMixer, 1);
AudioConnection          patchCord18(d3FmCarrierMixer, 0, d3FmBusMixer, 0);
AudioConnection          patchCord19(clapNoise1, clapAmpEnv1);
AudioConnection          patchCord20(clapAmpEnv2, 0, clapFilter2, 0);
AudioConnection          patchCord21(d1OscSaw, 0, d1OscMixer, 1);
AudioConnection          patchCord22(d1OscSine, 0, d1OscMixer, 0);
AudioConnection          patchCord23(d1OscSquare, 0, d1OscMixer, 2);
AudioConnection          patchCord24(d3606BusMixer, 0, d3606HighPass, 0);
AudioConnection          patchCord25(clapAmpEnv1, 0, clapFilter1, 0);
AudioConnection          patchCord26(d3FmBusMixer, 0, d3FmBandPass, 0);
AudioConnection          patchCord27(clapFilter2, 1, clapDelay2, 0);
AudioConnection          patchCord28(d2Noise, d2NoiseEnv);
AudioConnection          patchCord29(clapFilter1, 1, clapDelay1, 0);
AudioConnection          patchCord30(d3606HighPass, 2, d3606BandPass, 0);
AudioConnection          patchCord31(d3FmBandPass, 1, d3FmHighPass, 0);
AudioConnection          patchCord32(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord33(d2Osc, d2AmpEnv);
AudioConnection          patchCord34(d2NoiseEnv, 0, d2NoiseFilter, 0);
AudioConnection          patchCord35(d2NoiseEnv, 0, d2NoiseFilter, 1);
AudioConnection          patchCord36(d3606BandPass, 1, d3606AmpEnv, 0);
AudioConnection          patchCord37(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord38(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord39(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord40(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord41(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord42(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord43(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord44(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord45(d3Perc, 0, d3VoiceMixer, 2);
AudioConnection          patchCord46(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord47(d1AmpEnv, 0, masterWfInputMixer, 2);
AudioConnection          patchCord48(d3FmHighPass, 2, d3FmAmpEnv, 0);
AudioConnection          patchCord49(d2AmpEnv, 0, d2VoiceMixer, 0);
AudioConnection          patchCord50(d2AmpEnv, 0, masterWfInputMixer, 3);
AudioConnection          patchCord51(d3FmAmpEnv, 0, d3VoiceMixer, 1);
AudioConnection          patchCord52(d2ClickTransient, 0, d2ClickFilter, 0);
AudioConnection          patchCord53(clapMixer1, 0, clapBusMixer, 0);
AudioConnection          patchCord54(clapMixer2, 0, clapBusMixer, 1);
AudioConnection          patchCord55(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord56(d1LowPass, 0, d1VoiceMixer, 0);
AudioConnection          patchCord57(d3606AmpEnv, 0, d3VoiceMixer, 0);
AudioConnection          patchCord58(d2NoiseFilter, 2, d2VoiceMixer, 2);
AudioConnection          patchCord59(d2Body, 0, d2VoiceMixer, 1);
AudioConnection          patchCord60(d2ClickFilter, 1, d2VoiceMixer, 3);
AudioConnection          patchCord61(clapBusMixer, 0, clapMasterFilter, 0);
AudioConnection          patchCord62(d1Snap, d1SnapAmp);
AudioConnection          patchCord63(d1WfDrive, 0, d1Wavefolder, 1);
AudioConnection          patchCord64(d3VoiceMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord65(d3VoiceMixer, 0, d3MasterMixer, 0);
AudioConnection          patchCord66(d3WfOsc, 0, d3Wavefolder, 1);
AudioConnection          patchCord67(d2VoiceMixer, 0, d2VoiceHighPass, 0);
AudioConnection          patchCord68(clapAmp, clapMasterEnv);
AudioConnection          patchCord69(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord70(d1SnapAmp, 0, d1VoiceMixer, 1);
AudioConnection          patchCord71(d2WfSineOsc, d2WfAmp);
AudioConnection          patchCord72(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord73(d1Wavefolder, 0, d1VoiceMixer, 3);
AudioConnection          patchCord74(d2VoiceHighPass, 2, snareClapMixer, 0);
AudioConnection          patchCord75(d3Wavefolder, 0, d3MasterMixer, 1);
AudioConnection          patchCord76(d2Reverb, 0, d2MasterMixer, 1);
AudioConnection          patchCord77(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord78(d1VoiceMixer, 0, d1HighPass, 0);
AudioConnection          patchCord79(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord80(snareClapMixer, d2Reverb);
AudioConnection          patchCord81(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord82(snareClapMixer, 0, delaySendMixer, 1);
AudioConnection          patchCord83(d2Wavefolder, 0, d2WfLowPass, 0);
AudioConnection          patchCord84(d3MasterMixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord85(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord86(d1HighPass, 2, d1EQ, 0);
AudioConnection          patchCord87(d2WfLowPass, 0, d2MasterMixer, 2);
AudioConnection          patchCord88(d1EQ, d1Amp);
AudioConnection          patchCord89(masterWfOscSine, 0, masterWfInputMixer, 0);
AudioConnection          patchCord90(masterWfOscSaw, 0, masterWfInputMixer, 1);
AudioConnection          patchCord91(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord92(drumMixer, 0, masterWavefolder, 0);
AudioConnection          patchCord93(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord94(d3MasterFilter, 0, delaySendMixer, 2);
AudioConnection          patchCord95(masterWfInputMixer, 0, masterWavefolder, 1);
AudioConnection          patchCord96(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord97(d1Amp, 0, delaySendMixer, 0);
AudioConnection          patchCord98(masterWavefolder, 0, masterMixer, 1);
AudioConnection          patchCord99(masterMixer, 0, masterHighPass, 0);
AudioConnection          patchCord100(masterMixer, scopeQueue);
AudioConnection          patchCord101(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord102(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord103(delayFilter, 0, delaySendMixer, 3);
AudioConnection          patchCord104(delaySendMixer, masterDelay);
AudioConnection          patchCord105(delayFilter, 0, delayAmp, 0);
AudioConnection          patchCord106(masterHighPass, 2, masterLowPass, 0);
AudioConnection          patchCord107(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord108(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord109(masterAmp, finalFilter);
AudioConnection          patchCord110(finalFilter, finalAmp);
AudioConnection          patchCord111(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord112(finalAmp, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=4100,1430
// GUItool: end automatically generated code

#endif // AUDIOTOOL_H
