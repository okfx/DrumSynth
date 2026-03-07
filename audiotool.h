#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformDc     d1PitchDC;      //xy=240,130
AudioSynthWaveformModulated d2Osc;          //xy=240,380
AudioSynthNoiseWhite     d2Noise;        //xy=240,430
AudioSynthNoiseWhite     clapNoise1;     //xy=240,660
AudioSynthNoiseWhite     clapNoise2;     //xy=240,730
AudioSynthWaveform       d3FmMod1;       //xy=240,860
AudioSynthWaveform       d3FmMod2;       //xy=240,910
AudioSynthWaveform       d3606Osc1;      //xy=240,960
AudioSynthWaveform       d3606Osc2;      //xy=240,1010
AudioSynthWaveform       d3606Osc3;      //xy=240,1060
AudioEffectEnvelope      d1PitchEnv;     //xy=440,160
AudioEffectEnvelope      d2AmpEnv;       //xy=440,380
AudioEffectEnvelope      d2NoiseEnv;     //xy=440,430
AudioSynthSimpleDrum     d2ClickTransient; //xy=440,490
AudioEffectEnvelope      clapAmpEnv1;    //xy=440,660
AudioEffectEnvelope      clapAmpEnv2;    //xy=440,730
AudioSynthWaveformModulated d3FmCarrier1;   //xy=440,860
AudioSynthWaveformModulated d3FmCarrier2;   //xy=440,910
AudioSynthWaveform       d3606Osc4;      //xy=440,960
AudioSynthWaveform       d3606Osc5;      //xy=440,1010
AudioSynthWaveform       d3606Osc6;      //xy=440,1060
AudioSynthWaveformModulated d1OscSine;      //xy=640,120
AudioSynthWaveformModulated d1OscSaw;       //xy=640,170
AudioSynthWaveformModulated d1OscSquare;    //xy=640,220
AudioSynthSimpleDrum     d2Body;         //xy=640,380
AudioFilterStateVariable d2NoiseFilter;  //xy=640,430
AudioFilterStateVariable d2ClickFilter;  //xy=640,490
AudioFilterStateVariable clapFilter1;    //xy=640,660
AudioFilterStateVariable clapFilter2;    //xy=640,730
AudioMixer4              d3FmCarrierMixer; //xy=640,880
AudioMixer4              d3606OscMixer1; //xy=640,980
AudioMixer4              d3606OscMixer2; //xy=640,1060
AudioMixer4              d1OscMixer;     //xy=840,170
AudioMixer4              d2VoiceMixer;   //xy=840,420
AudioEffectDelay         clapDelay1;     //xy=840,660
AudioEffectDelay         clapDelay2;     //xy=840,730
AudioMixer4              d3FmBusMixer;   //xy=840,880
AudioMixer4              d3606BusMixer;  //xy=840,1020
AudioEffectEnvelope      d1AmpEnv;       //xy=1040,170
AudioFilterStateVariable d2VoiceHighPass; //xy=1040,420
AudioMixer4              clapMixer1;     //xy=1040,660
AudioMixer4              clapMixer2;     //xy=1040,730
AudioFilterStateVariable d3FmBandPass;   //xy=1040,880
AudioFilterStateVariable d3606HighPass;  //xy=1040,1020
AudioFilterStateVariable d1LowPass;      //xy=1240,170
AudioSynthWaveformDc     d1WfDrive;      //xy=1240,260
AudioMixer4              clapBusMixer;   //xy=1240,690
AudioFilterStateVariable d3FmHighPass;   //xy=1240,880
AudioFilterStateVariable d3606BandPass;  //xy=1240,1020
AudioSynthSimpleDrum     d1Snap;         //xy=1440,110
AudioEffectWaveFolder    d1Wavefolder;   //xy=1440,240
AudioFilterStateVariable clapMasterFilter; //xy=1440,690
AudioEffectEnvelope      d3FmAmpEnv;     //xy=1440,880
AudioEffectEnvelope      d3606AmpEnv;    //xy=1440,1020
AudioSynthSimpleDrum     d3Perc;         //xy=1440,1130
AudioAmplifier           d1SnapAmp;      //xy=1640,110
AudioMixer4              d1VoiceMixer;   //xy=1640,190
AudioSynthWaveformSine   d2WfSineOsc;    //xy=1640,510
AudioAmplifier           clapAmp;        //xy=1640,690
AudioMixer4              d3VoiceMixer;   //xy=1640,1130
AudioSynthWaveform       d3WfOsc;        //xy=1640,1220
AudioFilterStateVariable d1HighPass;     //xy=1840,190
AudioMixer4              snareClapMixer; //xy=1840,420
AudioAmplifier           d2WfAmp;        //xy=1840,510
AudioEffectEnvelope      clapMasterEnv;  //xy=1840,690
AudioEffectWaveFolder    d3Wavefolder;   //xy=1840,1170
AudioFilterBiquad        d1EQ;           //xy=2040,190
AudioEffectFreeverb      d2Reverb;       //xy=2040,360
AudioEffectWaveFolder    d2Wavefolder;   //xy=2040,510
AudioMixer4              d3MasterMixer;  //xy=2040,1160
AudioAmplifier           d1Amp;          //xy=2240,190
AudioMixer4              d2MasterMixer;  //xy=2240,410
AudioFilterStateVariable d2WfLowPass;    //xy=2240,510
AudioFilterLadder        d3MasterFilter; //xy=2240,1160
AudioSynthWaveform       masterWfOscSine; //xy=2240,1390
AudioSynthWaveform       masterWfOscSaw; //xy=2240,1440
AudioMixer4              drumMixer;      //xy=2440,1340
AudioMixer4              masterWfInputMixer; //xy=2440,1420
AudioEffectWaveFolder    masterWavefolder; //xy=2640,1370
AudioMixer4              masterMixer;    //xy=2840,1370
AudioMixer4              delaySendMixer; //xy=2840,1510
AudioRecordQueue         scopeQueue;     //xy=3040,1310
AudioFilterStateVariable masterHighPass; //xy=3040,1370
AudioAmplifier           delayAmp;       //xy=3040,1460
AudioEffectDelay         masterDelay;    //xy=3040,1570
AudioFilterLadder        masterLowPass;  //xy=3240,1370
AudioFilterStateVariable delayFilter;    //xy=3240,1570
AudioFilterStateVariable masterBandPass; //xy=3440,1370
AudioAmplifier           masterVolume;   //xy=3640,1370
AudioFilterBiquad        finalFilter;    //xy=3840,1370
AudioAmplifier           finalAmp;       //xy=4040,1370
AudioOutputI2S           i2s1;           //xy=4240,1370
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
AudioConnection          patchCord84(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord85(d1Amp, 0, delaySendMixer, 0);
AudioConnection          patchCord86(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord87(d2WfLowPass, 0, d2MasterMixer, 2);
AudioConnection          patchCord88(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord89(d3MasterFilter, 0, delaySendMixer, 2);
AudioConnection          patchCord90(masterWfOscSine, 0, masterWfInputMixer, 0);
AudioConnection          patchCord91(masterWfOscSaw, 0, masterWfInputMixer, 1);
AudioConnection          patchCord92(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord93(drumMixer, 0, masterWavefolder, 0);
AudioConnection          patchCord94(masterWfInputMixer, 0, masterWavefolder, 1);
AudioConnection          patchCord95(masterWavefolder, 0, masterMixer, 1);
AudioConnection          patchCord96(masterMixer, 0, masterHighPass, 0);
AudioConnection          patchCord97(masterMixer, scopeQueue);
AudioConnection          patchCord98(delaySendMixer, masterDelay);
AudioConnection          patchCord99(masterHighPass, 2, masterLowPass, 0);
AudioConnection          patchCord100(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord101(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord102(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord103(delayFilter, 0, delaySendMixer, 3);
AudioConnection          patchCord104(delayFilter, 0, delayAmp, 0);
AudioConnection          patchCord105(masterBandPass, 1, masterVolume, 0);
AudioConnection          patchCord106(masterVolume, finalFilter);
AudioConnection          patchCord107(finalFilter, finalAmp);
AudioConnection          patchCord108(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord109(finalAmp, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=4240,1460
// GUItool: end automatically generated code
