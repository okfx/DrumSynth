#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       d3606Osc3;      //xy=167.5,876.25
AudioSynthWaveform       d3606Osc6;      //xy=167.5,1008.75
AudioSynthWaveform       d3606Osc5;      //xy=168.75,958.75
AudioSynthWaveform       d3606Osc4;      //xy=171.25,917.5
AudioSynthWaveform       d3606Osc2;      //xy=172.5,838.75
AudioSynthWaveform       d3606Osc1;      //xy=173.75,800
AudioSynthWaveformDc     d1PitchDC;      //xy=180,120
AudioSynthWaveformModulated d2Osc;          //xy=180,370
AudioSynthNoiseWhite     d2Noise;        //xy=180,420
AudioSynthNoiseWhite     clapNoise1;     //xy=180,650
AudioSynthNoiseWhite     clapNoise2;     //xy=180,720
AudioSynthWaveform       d3FmMod2;       //xy=177.5,1433.75
AudioSynthWaveform       d3FmMod1;       //xy=186.25,1362.5
AudioSynthWaveformModulated d3FmCarrier2;   //xy=369.9999694824219,1443.75
AudioEffectEnvelope      d1PitchEnv;     //xy=380,150
AudioEffectEnvelope      d2AmpEnv;       //xy=380,370
AudioEffectEnvelope      d2NoiseEnv;     //xy=380,420
AudioSynthSimpleDrum     d2ClickTransient; //xy=380,480
AudioEffectEnvelope      clapAmpEnv1;    //xy=380,650
AudioEffectEnvelope      clapAmpEnv2;    //xy=380,720
AudioSynthWaveformModulated d3FmCarrier1;   //xy=377.4999694824219,1375
AudioMixer4              d3606OscMixer1; //xy=541.25,803.75
AudioMixer4              d3606OscMixer2; //xy=547.5,891.25
AudioSynthWaveformModulated d1OscSine;      //xy=580,110
AudioSynthWaveformModulated d1OscSaw;       //xy=580,160
AudioSynthWaveformModulated d1OscSquare;    //xy=580,210
AudioSynthSimpleDrum     d2Body;         //xy=580,370
AudioFilterStateVariable d2NoiseFilter;  //xy=580,420
AudioFilterStateVariable d2ClickFilter;  //xy=580,480
AudioFilterStateVariable clapFilter1;    //xy=580,650
AudioFilterStateVariable clapFilter2;    //xy=580,720
AudioMixer4              d3FmCarrierMixer; //xy=593.75,1300
AudioMixer4              d3FmBusMixer;   //xy=721.2500305175781,1145
AudioMixer4              d1OscMixer;     //xy=780,160
AudioMixer4              d2VoiceMixer;   //xy=780,410
AudioMixer4              d3606BusMixer;  //xy=778.7500305175781,823.75
AudioEffectDelay         clapDelay1;     //xy=780,650
AudioEffectDelay         clapDelay2;     //xy=780,720
AudioFilterStateVariable d3FmBandPass;   //xy=918.75,1153.75
AudioEffectEnvelope      d1AmpEnv;       //xy=980,160
AudioFilterStateVariable d3606HighPass;  //xy=977.5000610351562,897.5
AudioFilterStateVariable d2VoiceHighPass; //xy=980,410
AudioMixer4              clapMixer1;     //xy=980,650
AudioMixer4              clapMixer2;     //xy=980,720
AudioFilterStateVariable d3FmHighPass;   //xy=1123.75,1076.25
AudioFilterStateVariable d1LowPass;      //xy=1180,160
AudioSynthWaveformDc     d1WfDrive;      //xy=1180,250
AudioMixer4              clapBusMixer;   //xy=1180,680
AudioFilterStateVariable d3606BandPass;  //xy=1180,900
AudioSynthSimpleDrum     d3Perc;         //xy=1228.75,1140
AudioEffectEnvelope      d3FmAmpEnv;     //xy=1365,1072.5
AudioEffectEnvelope      d3606AmpEnv;    //xy=1372.5,902.5
AudioSynthSimpleDrum     d1Snap;         //xy=1380,100
AudioEffectWaveFolder    d1Wavefolder;   //xy=1380,230
AudioFilterStateVariable clapMasterFilter; //xy=1380,680
AudioFilterStateVariable d3PercFilter;        //xy=1385,1146.25
AudioAmplifier           d1SnapAmp;      //xy=1580,100
AudioMixer4              d1VoiceMixer;   //xy=1580,180
AudioSynthWaveformSine   d2WfSineOsc;    //xy=1580,500
AudioAmplifier           clapAmp;        //xy=1580,680
AudioMixer4              d3VoiceMixer;   //xy=1580,1120
AudioSynthWaveform       d3WfOsc;        //xy=1580,1210
AudioFilterStateVariable d1HighPass;     //xy=1780,180
AudioEffectWaveFolder    d3Wavefolder;   //xy=1776.2500610351562,1205
AudioMixer4              snareClapMixer; //xy=1780,410
AudioAmplifier           d2WfAmp;        //xy=1780,500
AudioEffectEnvelope      clapMasterEnv;  //xy=1780,680
AudioFilterBiquad        d1EQ;           //xy=1980,180
AudioEffectFreeverb      d2Reverb;       //xy=1980,350
AudioEffectWaveFolder    d2Wavefolder;   //xy=1980,500
AudioMixer4              d3MasterMixer;  //xy=1980,1150
AudioAmplifier           d1Amp;          //xy=2180,180
AudioMixer4              d2MasterMixer;  //xy=2180,400
AudioFilterStateVariable d2WfLowPass;    //xy=2180,500
AudioFilterLadder        d3MasterFilter; //xy=2180,1150
AudioSynthWaveform       masterWfOscSine; //xy=2180,1380
AudioSynthWaveform       masterWfOscSaw; //xy=2180,1430
AudioMixer4              drumMixer;      //xy=2380,1330
AudioMixer4              masterWfInputMixer; //xy=2380,1410
AudioEffectWaveFolder    masterWavefolder; //xy=2580,1360
AudioMixer4              masterMixer;    //xy=2780,1360
AudioMixer4              delaySendMixer; //xy=2780,1500
AudioRecordQueue         scopeQueue;     //xy=2980,1300
AudioFilterStateVariable masterHighPass; //xy=2980,1360
AudioAmplifier           delayAmp;       //xy=2980,1450
AudioEffectDelay         masterDelay;    //xy=2980,1560
AudioFilterLadder        masterLowPass;  //xy=3180,1360
AudioFilterStateVariable delayFilter;    //xy=3180,1560
AudioFilterStateVariable masterBandPass; //xy=3380,1360
AudioAmplifier           masterAmp;      //xy=3580,1360
AudioFilterBiquad        finalFilter;    //xy=3780,1360
AudioAmplifier           finalAmp;       //xy=3980,1360
AudioAmplifier           usbTrim;        //xy=4100,1290
AudioOutputUSB           usb1;           //xy=4180,1290
AudioOutputI2S           i2s1;           //xy=4180,1360
AudioConnection          patchCord1(d3606Osc3, 0, d3606OscMixer1, 2);
AudioConnection          patchCord2(d3606Osc6, 0, d3606OscMixer2, 1);
AudioConnection          patchCord3(d3606Osc5, 0, d3606OscMixer2, 0);
AudioConnection          patchCord4(d3606Osc4, 0, d3606OscMixer1, 3);
AudioConnection          patchCord5(d3606Osc2, 0, d3606OscMixer1, 1);
AudioConnection          patchCord6(d3606Osc1, 0, d3606OscMixer1, 0);
AudioConnection          patchCord7(d1PitchDC, d1PitchEnv);
AudioConnection          patchCord8(d2Osc, d2AmpEnv);
AudioConnection          patchCord9(d2Noise, d2NoiseEnv);
AudioConnection          patchCord10(clapNoise1, clapAmpEnv1);
AudioConnection          patchCord11(clapNoise2, clapAmpEnv2);
AudioConnection          patchCord12(d3FmMod2, 0, d3FmCarrier2, 0);
AudioConnection          patchCord13(d3FmMod1, 0, d3FmCarrier1, 0);
AudioConnection          patchCord14(d3FmCarrier2, 0, d3FmCarrierMixer, 1);
AudioConnection          patchCord15(d1PitchEnv, 0, d1OscSine, 0);
AudioConnection          patchCord16(d1PitchEnv, 0, d1OscSaw, 0);
AudioConnection          patchCord17(d1PitchEnv, 0, d1OscSquare, 0);
AudioConnection          patchCord18(d2AmpEnv, 0, d2VoiceMixer, 0);
AudioConnection          patchCord19(d2AmpEnv, 0, masterWfInputMixer, 3);
AudioConnection          patchCord20(d2NoiseEnv, 0, d2NoiseFilter, 0);
AudioConnection          patchCord21(d2ClickTransient, 0, d2ClickFilter, 0);
AudioConnection          patchCord22(clapAmpEnv1, 0, clapFilter1, 0);
AudioConnection          patchCord23(clapAmpEnv2, 0, clapFilter2, 0);
AudioConnection          patchCord24(d3FmCarrier1, 0, d3FmCarrierMixer, 0);
AudioConnection          patchCord25(d3606OscMixer1, 0, d3606BusMixer, 0);
AudioConnection          patchCord26(d3606OscMixer2, 0, d3606BusMixer, 1);
AudioConnection          patchCord27(d1OscSine, 0, d1OscMixer, 0);
AudioConnection          patchCord28(d1OscSaw, 0, d1OscMixer, 1);
AudioConnection          patchCord29(d1OscSquare, 0, d1OscMixer, 2);
AudioConnection          patchCord30(d2Body, 0, d2VoiceMixer, 1);
AudioConnection          patchCord31(d2NoiseFilter, 2, d2VoiceMixer, 2);
AudioConnection          patchCord32(d2ClickFilter, 1, d2VoiceMixer, 3);
AudioConnection          patchCord33(clapFilter1, 1, clapDelay1, 0);
AudioConnection          patchCord34(clapFilter2, 1, clapDelay2, 0);
AudioConnection          patchCord35(d3FmCarrierMixer, 0, d3FmBusMixer, 0);
AudioConnection          patchCord36(d3FmBusMixer, 0, d3FmBandPass, 0);
AudioConnection          patchCord37(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord38(d2VoiceMixer, 0, d2VoiceHighPass, 0);
AudioConnection          patchCord39(d3606BusMixer, 0, d3606HighPass, 0);
AudioConnection          patchCord40(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord41(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord42(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord43(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord44(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord45(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord46(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord47(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord48(d3FmBandPass, 1, d3FmHighPass, 0);
AudioConnection          patchCord49(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord50(d1AmpEnv, 0, masterWfInputMixer, 2);
AudioConnection          patchCord51(d3606HighPass, 2, d3606BandPass, 0);
AudioConnection          patchCord52(d2VoiceHighPass, 2, snareClapMixer, 0);
AudioConnection          patchCord53(clapMixer1, 0, clapBusMixer, 0);
AudioConnection          patchCord54(clapMixer2, 0, clapBusMixer, 1);
AudioConnection          patchCord55(d3FmHighPass, 2, d3FmAmpEnv, 0);
AudioConnection          patchCord56(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord57(d1LowPass, 0, d1VoiceMixer, 0);
AudioConnection          patchCord58(d1WfDrive, 0, d1Wavefolder, 1);
AudioConnection          patchCord59(clapBusMixer, 0, clapMasterFilter, 0);
AudioConnection          patchCord60(d3606BandPass, 1, d3606AmpEnv, 0);
AudioConnection          patchCord61(d3Perc, 0, d3PercFilter, 0);
AudioConnection          patchCord62(d3FmAmpEnv, 0, d3VoiceMixer, 1);
AudioConnection          patchCord63(d3606AmpEnv, 0, d3VoiceMixer, 0);
AudioConnection          patchCord64(d1Snap, d1SnapAmp);
AudioConnection          patchCord65(d1Wavefolder, 0, d1VoiceMixer, 3);
AudioConnection          patchCord66(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord67(d3PercFilter, 0, d3VoiceMixer, 2);
AudioConnection          patchCord68(d1SnapAmp, 0, d1VoiceMixer, 1);
AudioConnection          patchCord69(d1VoiceMixer, 0, d1HighPass, 0);
AudioConnection          patchCord70(d2WfSineOsc, d2WfAmp);
AudioConnection          patchCord71(clapAmp, clapMasterEnv);
AudioConnection          patchCord72(d3VoiceMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord73(d3VoiceMixer, 0, d3MasterMixer, 0);
AudioConnection          patchCord74(d3WfOsc, 0, d3Wavefolder, 1);
AudioConnection          patchCord75(d1HighPass, 2, d1EQ, 0);
AudioConnection          patchCord76(d3Wavefolder, 0, d3MasterMixer, 1);
AudioConnection          patchCord77(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord78(snareClapMixer, d2Reverb);
AudioConnection          patchCord79(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord80(snareClapMixer, 0, delaySendMixer, 1);
AudioConnection          patchCord81(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord82(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord83(d1EQ, d1Amp);
AudioConnection          patchCord84(d2Reverb, 0, d2MasterMixer, 1);
AudioConnection          patchCord85(d2Wavefolder, 0, d2WfLowPass, 0);
AudioConnection          patchCord86(d3MasterMixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord87(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord88(d1Amp, 0, delaySendMixer, 0);
AudioConnection          patchCord89(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord90(d2WfLowPass, 0, d2MasterMixer, 2);
AudioConnection          patchCord91(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord92(d3MasterFilter, 0, delaySendMixer, 2);
AudioConnection          patchCord93(masterWfOscSine, 0, masterWfInputMixer, 0);
AudioConnection          patchCord94(masterWfOscSaw, 0, masterWfInputMixer, 1);
AudioConnection          patchCord95(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord96(drumMixer, 0, masterWavefolder, 0);
AudioConnection          patchCord97(masterWfInputMixer, 0, masterWavefolder, 1);
AudioConnection          patchCord98(masterWavefolder, 0, masterMixer, 1);
AudioConnection          patchCord99(masterMixer, 0, masterHighPass, 0);
AudioConnection          patchCord100(masterMixer, scopeQueue);
AudioConnection          patchCord101(delaySendMixer, masterDelay);
AudioConnection          patchCord102(masterHighPass, 2, masterLowPass, 0);
AudioConnection          patchCord103(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord104(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord105(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord106(delayFilter, 0, delaySendMixer, 3);
AudioConnection          patchCord107(delayFilter, 0, delayAmp, 0);
AudioConnection          patchCord108(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord109(masterAmp, finalFilter);
AudioConnection          patchCord110(finalFilter, finalAmp);
AudioConnection          patchCord111(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord112(finalAmp, 0, i2s1, 1);
AudioConnection          patchCord113(finalAmp, usbTrim);
AudioConnection          patchCord114(usbTrim, 0, usb1, 0);
AudioConnection          patchCord115(usbTrim, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=4180,1450
// GUItool: end automatically generated code
