#ifndef AUDIOTOOL_H
#define AUDIOTOOL_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
// GUItool: begin automatically generated code
AudioSynthWaveformDc     d1PitchDC;      //xy=255,170
AudioSynthWaveformModulated d2Osc;          //xy=255,420
AudioSynthNoiseWhite     d2Noise;        //xy=255,470
AudioSynthNoiseWhite     clapNoise1;     //xy=255,700
AudioSynthNoiseWhite     clapNoise2;     //xy=255,770
AudioSynthWaveform       d3FmMod1;       //xy=255,900
AudioSynthWaveform       d3FmMod2;       //xy=255,950
AudioSynthWaveform       d3606Osc1;      //xy=255,1000
AudioSynthWaveform       d3606Osc2;      //xy=255,1050
AudioSynthWaveform       d3606Osc3;      //xy=255,1100
AudioEffectEnvelope      d1PitchEnv;     //xy=455,200
AudioEffectEnvelope      d2AmpEnv;       //xy=455,420
AudioEffectEnvelope      d2NoiseEnv;     //xy=455,470
AudioSynthSimpleDrum     d2ClickTransient; //xy=455,530
AudioEffectEnvelope      clapAmpEnv1;    //xy=455,700
AudioEffectEnvelope      clapAmpEnv2;    //xy=455,770
AudioSynthWaveformModulated d3FmCarrier1;   //xy=455,900
AudioSynthWaveformModulated d3FmCarrier2;   //xy=455,950
AudioSynthWaveform       d3606Osc4;      //xy=455,1000
AudioSynthWaveform       d3606Osc5;      //xy=455,1050
AudioSynthWaveform       d3606Osc6;      //xy=455,1100
AudioSynthWaveformModulated d1OscSine;      //xy=655,160
AudioSynthWaveformModulated d1OscSaw;       //xy=655,210
AudioSynthWaveformModulated d1OscSquare;    //xy=655,260
AudioSynthSimpleDrum     d2Body;         //xy=655,420
AudioFilterStateVariable d2NoiseFilter;  //xy=655,470
AudioFilterStateVariable d2ClickFilter;  //xy=655,530
AudioFilterStateVariable clapFilter1;    //xy=655,700
AudioFilterStateVariable clapFilter2;    //xy=655,770
AudioMixer4              d3FmCarrierMixer; //xy=655,920
AudioMixer4              d3606OscMixer1; //xy=655,1020
AudioMixer4              d3606OscMixer2; //xy=655,1100
AudioMixer4              d1OscMixer;     //xy=855,210
AudioMixer4              d2VoiceMixer;   //xy=855,460
AudioEffectDelay         clapDelay1;     //xy=855,700
AudioEffectDelay         clapDelay2;     //xy=855,770
AudioMixer4              d3FmBusMixer;   //xy=855,920
AudioMixer4              d3606BusMixer;  //xy=855,1060
AudioEffectEnvelope      d1AmpEnv;       //xy=1055,210
AudioFilterStateVariable d2VoiceHighPass; //xy=1055,460
AudioMixer4              clapMixer1;     //xy=1055,700
AudioMixer4              clapMixer2;     //xy=1055,770
AudioFilterStateVariable d3FmBandPass;   //xy=1055,920
AudioFilterStateVariable d3606HighPass;  //xy=1055,1060
AudioFilterStateVariable d1LowPass;      //xy=1255,210
AudioSynthWaveformDc     d1WfDrive;      //xy=1255,300
AudioMixer4              clapBusMixer;   //xy=1255,730
AudioFilterStateVariable d3FmHighPass;   //xy=1255,920
AudioFilterStateVariable d3606BandPass;  //xy=1255,1060
AudioSynthSimpleDrum     d1Snap;         //xy=1455,150
AudioEffectWaveFolder    d1Wavefolder;   //xy=1455,280
AudioFilterStateVariable clapMasterFilter; //xy=1455,730
AudioEffectEnvelope      d3FmAmpEnv;     //xy=1455,920
AudioEffectEnvelope      d3606AmpEnv;    //xy=1455,1060
AudioSynthSimpleDrum     d3Perc;         //xy=1455,1170
AudioAmplifier           d1SnapAmp;      //xy=1655,150
AudioMixer4              d1VoiceMixer;   //xy=1655,230
AudioSynthWaveformSine   d2WfSineOsc;    //xy=1655,550
AudioAmplifier           clapAmp;        //xy=1655,730
AudioMixer4              d3VoiceMixer;   //xy=1655,1170
AudioSynthWaveform       d3WfOsc;        //xy=1655,1260
AudioFilterStateVariable d1HighPass;     //xy=1855,230
AudioMixer4              snareClapMixer; //xy=1855,460
AudioAmplifier           d2WfAmp;        //xy=1855,550
AudioEffectEnvelope      clapMasterEnv;  //xy=1855,730
AudioEffectWaveFolder    d3Wavefolder;   //xy=1855,1210
AudioFilterBiquad        d1EQ;           //xy=2055,230
AudioEffectFreeverb      d2Reverb;       //xy=2055,400
AudioEffectWaveFolder    d2Wavefolder;   //xy=2055,550
AudioMixer4              d3MasterMixer;  //xy=2055,1200
AudioMixer4              masterWfInputMixer; //xy=2077,1515
AudioSynthWaveform       masterWfOscSine; //xy=2167,1314
AudioSynthWaveform       masterWfOscSaw; //xy=2167,1350
AudioAmplifier           d1Amp;          //xy=2255,230
AudioMixer4              d2MasterMixer;  //xy=2255,450
AudioFilterStateVariable d2WfLowPass;    //xy=2255,550
AudioFilterLadder        d3MasterFilter; //xy=2255,1200
AudioEffectDelay         wfDelay;        //xy=2262,1535
AudioMixer4              drumMixer;      //xy=2455,1380
AudioAmplifier           wfDelayAmp;     //xy=2550,1805
AudioEffectWaveFolder    masterWavefolder; //xy=2568,1519
AudioMixer4              masterMixer;    //xy=2855,1410
AudioMixer4              delaySendMixer; //xy=2855,1550
AudioRecordQueue         scopeQueue;     //xy=3055,1350
AudioFilterStateVariable masterHighPass; //xy=3055,1410
AudioAmplifier           delayAmp;       //xy=3055,1500
AudioEffectDelay         masterDelay;    //xy=3055,1610
AudioFilterLadder        masterLowPass;  //xy=3255,1410
AudioFilterStateVariable delayFilter;    //xy=3255,1610
AudioFilterStateVariable masterBandPass; //xy=3455,1410
AudioAmplifier           masterAmp;      //xy=3655,1410
AudioFilterBiquad        finalFilter;    //xy=3855,1410
AudioAmplifier           finalAmp;       //xy=4055,1410
AudioAmplifier           usbTrim;           //xy=4177,1341
AudioOutputI2S           i2s1;           //xy=4255,1410
AudioOutputUSB           usb1;           //xy=4301,1340
AudioConnection          patchCord1(d1PitchDC, d1PitchEnv);
AudioConnection          patchCord2(d2Osc, d2AmpEnv);
AudioConnection          patchCord3(d2Noise, d2NoiseEnv);
AudioConnection          patchCord4(clapNoise1, clapAmpEnv1);
AudioConnection          patchCord5(clapNoise2, clapAmpEnv2);
AudioConnection          patchCord6(d3FmMod1, 0, d3FmCarrier1, 0);
AudioConnection          patchCord7(d3FmMod2, 0, d3FmCarrier2, 0);
AudioConnection          patchCord8(d3606Osc1, 0, d3606OscMixer1, 0);
AudioConnection          patchCord9(d3606Osc2, 0, d3606OscMixer1, 1);
AudioConnection          patchCord10(d3606Osc3, 0, d3606OscMixer1, 2);
AudioConnection          patchCord11(d1PitchEnv, 0, d1OscSine, 0);
AudioConnection          patchCord12(d1PitchEnv, 0, d1OscSaw, 0);
AudioConnection          patchCord13(d1PitchEnv, 0, d1OscSquare, 0);
AudioConnection          patchCord14(d2AmpEnv, 0, d2VoiceMixer, 0);
AudioConnection          patchCord15(d2NoiseEnv, 0, d2NoiseFilter, 0);
AudioConnection          patchCord16(d2ClickTransient, 0, d2ClickFilter, 0);
AudioConnection          patchCord17(clapAmpEnv1, 0, clapFilter1, 0);
AudioConnection          patchCord18(clapAmpEnv2, 0, clapFilter2, 0);
AudioConnection          patchCord19(d3FmCarrier1, 0, d3FmCarrierMixer, 0);
AudioConnection          patchCord20(d3FmCarrier2, 0, d3FmCarrierMixer, 1);
AudioConnection          patchCord21(d3606Osc4, 0, d3606OscMixer1, 3);
AudioConnection          patchCord22(d3606Osc5, 0, d3606OscMixer2, 0);
AudioConnection          patchCord23(d3606Osc6, 0, d3606OscMixer2, 1);
AudioConnection          patchCord24(d1OscSine, 0, d1OscMixer, 0);
AudioConnection          patchCord25(d1OscSaw, 0, d1OscMixer, 1);
AudioConnection          patchCord26(d1OscSquare, 0, d1OscMixer, 2);
AudioConnection          patchCord27(d2Body, 0, d2VoiceMixer, 1);
AudioConnection          patchCord28(d2NoiseFilter, 2, d2VoiceMixer, 2);
AudioConnection          patchCord29(d2ClickFilter, 1, d2VoiceMixer, 3);
AudioConnection          patchCord30(clapFilter1, 1, clapDelay1, 0);
AudioConnection          patchCord31(clapFilter2, 1, clapDelay2, 0);
AudioConnection          patchCord32(d3FmCarrierMixer, 0, d3FmBusMixer, 0);
AudioConnection          patchCord33(d3606OscMixer1, 0, d3606BusMixer, 0);
AudioConnection          patchCord34(d3606OscMixer2, 0, d3606BusMixer, 1);
AudioConnection          patchCord35(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord36(d2VoiceMixer, 0, d2VoiceHighPass, 0);
AudioConnection          patchCord37(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord38(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord39(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord40(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord41(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord42(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord43(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord44(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord45(d3FmBusMixer, 0, d3FmBandPass, 0);
AudioConnection          patchCord46(d3606BusMixer, 0, d3606HighPass, 0);
AudioConnection          patchCord47(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord48(d2VoiceHighPass, 2, snareClapMixer, 0);
AudioConnection          patchCord49(clapMixer1, 0, clapBusMixer, 0);
AudioConnection          patchCord50(clapMixer2, 0, clapBusMixer, 1);
AudioConnection          patchCord51(d3FmBandPass, 1, d3FmHighPass, 0);
AudioConnection          patchCord52(d3606HighPass, 2, d3606BandPass, 0);
AudioConnection          patchCord53(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord54(d1LowPass, 0, d1VoiceMixer, 0);
AudioConnection          patchCord55(d1WfDrive, 0, d1Wavefolder, 1);
AudioConnection          patchCord56(clapBusMixer, 0, clapMasterFilter, 0);
AudioConnection          patchCord57(d3FmHighPass, 2, d3FmAmpEnv, 0);
AudioConnection          patchCord58(d3606BandPass, 1, d3606AmpEnv, 0);
AudioConnection          patchCord59(d1Snap, d1SnapAmp);
AudioConnection          patchCord60(d1Wavefolder, 0, d1VoiceMixer, 3);
AudioConnection          patchCord61(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord62(d3FmAmpEnv, 0, d3VoiceMixer, 1);
AudioConnection          patchCord63(d3606AmpEnv, 0, d3VoiceMixer, 0);
AudioConnection          patchCord64(d3Perc, 0, d3VoiceMixer, 2);
AudioConnection          patchCord65(d1SnapAmp, 0, d1VoiceMixer, 1);
AudioConnection          patchCord66(d1VoiceMixer, 0, d1HighPass, 0);
AudioConnection          patchCord67(d2WfSineOsc, d2WfAmp);
AudioConnection          patchCord68(clapAmp, clapMasterEnv);
AudioConnection          patchCord69(d3VoiceMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord70(d3VoiceMixer, 0, d3MasterMixer, 0);
AudioConnection          patchCord71(d3WfOsc, 0, d3Wavefolder, 1);
AudioConnection          patchCord72(d1HighPass, 2, d1EQ, 0);
AudioConnection          patchCord73(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord74(snareClapMixer, d2Reverb);
AudioConnection          patchCord75(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord76(snareClapMixer, 0, delaySendMixer, 1);
AudioConnection          patchCord77(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord78(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord79(d3Wavefolder, 0, d3MasterMixer, 1);
AudioConnection          patchCord80(d1EQ, d1Amp);
AudioConnection          patchCord81(d2Reverb, 0, d2MasterMixer, 1);
AudioConnection          patchCord82(d2Wavefolder, 0, d2WfLowPass, 0);
AudioConnection          patchCord83(d3MasterMixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord84(masterWfOscSine, 0, masterWfInputMixer, 0);
AudioConnection          patchCord85(masterWfOscSaw, 0, masterWfInputMixer, 1);
AudioConnection          patchCord86(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord87(d1Amp, 0, delaySendMixer, 0);
AudioConnection          patchCord88(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord89(d2WfLowPass, 0, d2MasterMixer, 2);
AudioConnection          patchCord90(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord91(d3MasterFilter, 0, delaySendMixer, 2);
AudioConnection          patchCord92(wfDelay, 0, wfDelayAmp, 0);
AudioConnection          patchCord93(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord94(drumMixer, 0, masterWavefolder, 0);
AudioConnection          patchCord95(drumMixer, wfDelay);
AudioConnection          patchCord96(wfDelayAmp, 0, masterWavefolder, 1);
AudioConnection          patchCord97(masterWavefolder, 0, masterMixer, 1);
AudioConnection          patchCord98(masterMixer, 0, masterHighPass, 0);
AudioConnection          patchCord99(masterMixer, scopeQueue);
AudioConnection          patchCord100(delaySendMixer, masterDelay);
AudioConnection          patchCord101(masterHighPass, 2, masterLowPass, 0);
AudioConnection          patchCord102(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord103(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord104(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord105(delayFilter, 0, delaySendMixer, 3);
AudioConnection          patchCord106(delayFilter, 0, delayAmp, 0);
AudioConnection          patchCord107(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord108(masterAmp, finalFilter);
AudioConnection          patchCord109(finalFilter, finalAmp);
AudioConnection          patchCord110(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord111(finalAmp, 0, i2s1, 1);
AudioConnection          patchCord112(finalAmp, usbTrim);
AudioConnection          patchCord113(usbTrim, 0, usb1, 0);
AudioConnection          patchCord114(usbTrim, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=4255,1500
// GUItool: end automatically generated code

#endif // AUDIOTOOL_H
