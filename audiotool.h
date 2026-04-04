#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       d3606Osc3;      //xy=170,941
AudioSynthWaveform       d3606Osc6;      //xy=170,1073
AudioSynthWaveform       d3606Osc5;      //xy=171,1023
AudioSynthWaveform       d3606Osc4;      //xy=174,982
AudioSynthWaveform       d3606Osc2;      //xy=175,903
AudioSynthWaveform       d3606Osc1;      //xy=176,865
AudioSynthWaveformDc     d1PitchDC;      //xy=183,185
AudioSynthWaveformModulated d2Osc;          //xy=183,435
AudioSynthNoiseWhite     d2Noise;        //xy=183,485
AudioSynthNoiseWhite     clapNoise1;     //xy=183,715
AudioSynthWaveform       d3FmMod2;       //xy=180,1498
AudioSynthNoiseWhite     clapNoise2;     //xy=183,785
AudioSynthWaveform       d3FmMod1;       //xy=189,1427
AudioSynthWaveformModulated d3FmCarrier2;   //xy=372,1508
AudioEffectEnvelope      d1PitchEnv;     //xy=383,215
AudioEffectEnvelope      d2AmpEnv;       //xy=383,435
AudioEffectEnvelope      d2NoiseEnv;     //xy=383,485
AudioSynthSimpleDrum     d2ClickTransient; //xy=383,545
AudioSynthWaveformModulated d3FmCarrier1;   //xy=380,1440
AudioEffectEnvelope      clapAmpEnv1;    //xy=383,715
AudioEffectEnvelope      clapAmpEnv2;    //xy=383,785
AudioMixer4              d3606OscMixer1; //xy=544,868
AudioMixer4              d3606OscMixer2; //xy=550,956
AudioSynthWaveformModulated d1OscSine;      //xy=583,175
AudioSynthWaveformModulated d1OscSaw;       //xy=583,225
AudioSynthWaveformModulated d1OscSquare;    //xy=583,275
AudioSynthSimpleDrum     d2Body;         //xy=583,435
AudioFilterStateVariable d2NoiseFilter;  //xy=583,485
AudioFilterStateVariable d2ClickFilter;  //xy=583,545
AudioFilterStateVariable clapFilter1;    //xy=583,715
AudioFilterStateVariable clapFilter2;    //xy=583,785
AudioMixer4              d3FmCarrierMixer; //xy=596,1365
AudioMixer4              d3FmBusMixer;   //xy=724,1210
AudioMixer4              d1OscMixer;     //xy=783,225
AudioMixer4              d3606BusMixer;  //xy=781,888
AudioMixer4              d2VoiceMixer;   //xy=783,475
AudioEffectDelay         clapDelay1;     //xy=783,715
AudioEffectDelay         clapDelay2;     //xy=783,785
AudioFilterStateVariable d3FmBandPass;   //xy=921,1218
AudioFilterStateVariable d3606HighPass;  //xy=980,962
AudioEffectEnvelope      d1AmpEnv;       //xy=983,225
AudioFilterStateVariable d2VoiceHighPass; //xy=983,475
AudioMixer4              clapMixer1;     //xy=983,715
AudioMixer4              clapMixer2;     //xy=983,785
AudioFilterStateVariable d3FmHighPass;   //xy=1126,1141
AudioFilterStateVariable d1LowPass;      //xy=1183,225
AudioSynthWaveformDc     d1WfDrive;      //xy=1183,315
AudioMixer4              clapBusMixer;   //xy=1183,745
AudioFilterStateVariable d3606BandPass;  //xy=1183,965
AudioSynthSimpleDrum     d3Perc;         //xy=1231,1205
AudioEffectEnvelope      d3FmAmpEnv;     //xy=1368,1137
AudioEffectEnvelope      d3606AmpEnv;    //xy=1375,967
AudioSynthSimpleDrum     d1Snap;         //xy=1383,165
AudioEffectWaveFolder    d1Wavefolder;   //xy=1383,295
AudioFilterStateVariable clapMasterFilter; //xy=1383,745
AudioFilterStateVariable d3PercFilter;   //xy=1388,1211
AudioAmplifier           d1SnapAmp;      //xy=1583,165
AudioMixer4              d1VoiceMixer;   //xy=1583,245
AudioSynthWaveformSine   d2WfSineOsc;    //xy=1583,565
AudioAmplifier           clapAmp;        //xy=1583,745
AudioMixer4              d3VoiceMixer;   //xy=1583,1185
AudioSynthWaveform       d3WfOsc;        //xy=1583,1275
AudioFilterStateVariable d1HighPass;     //xy=1783,245
AudioEffectWaveFolder    d3Wavefolder;   //xy=1779,1270
AudioMixer4              snareClapMixer; //xy=1783,475
AudioAmplifier           d2WfAmp;        //xy=1783,565
AudioEffectEnvelope      clapMasterEnv;  //xy=1783,745
AudioFilterBiquad        d1EQ;           //xy=1983,245
AudioEffectFreeverb      d2Reverb;       //xy=1983,415
AudioEffectWaveFolder    d2Wavefolder;   //xy=1983,565
AudioMixer4              d3MasterMixer;  //xy=1983,1215
AudioAmplifier           d1Amp;          //xy=2183,245
AudioMixer4              d2MasterMixer;  //xy=2183,465
AudioFilterStateVariable d2WfLowPass;    //xy=2183,565
AudioFilterLadder        d3MasterFilter; //xy=2183,1215
AudioSynthWaveform       masterWfOscSine; //xy=2183,1445
AudioSynthWaveform       masterWfOscSaw; //xy=2183,1495
AudioMixer4              drumMixer;      //xy=2383,1395
AudioMixer4              masterWfInputMixer; //xy=2383,1475
AudioEffectWaveFolder    masterWavefolder; //xy=2583,1425
AudioMixer4              masterMixer;    //xy=2783,1425
AudioMixer4              delaySendMixer; //xy=2783,1565
AudioRecordQueue         scopeQueue;     //xy=2983,1365
AudioFilterStateVariable masterHighPass; //xy=2983,1425
AudioAmplifier           delayAmp;       //xy=2983,1515
AudioEffectDelay         masterDelay;    //xy=2983,1625
AudioFilterLadder        masterLowPass;  //xy=3183,1425
AudioFilterStateVariable delayFilter;    //xy=3183,1625
AudioFilterStateVariable masterBandPass; //xy=3383,1425
AudioAmplifier           masterAmp;      //xy=3583,1425
AudioFilterBiquad        finalFilter;    //xy=3783,1425
AudioFilterBiquad        masterRolloff;        //xy=3913.3334789276123,1335.0000190734863
AudioAmplifier           finalAmp;       //xy=3983,1425
AudioAmplifier           usbTrim;        //xy=4103,1355
AudioOutputUSB           usb1;           //xy=4183,1355
AudioOutputI2S           i2s1;           //xy=4183,1425
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
AudioConnection          patchCord11(d3FmMod2, 0, d3FmCarrier2, 0);
AudioConnection          patchCord12(clapNoise2, clapAmpEnv2);
AudioConnection          patchCord13(d3FmMod1, 0, d3FmCarrier1, 0);
AudioConnection          patchCord14(d3FmCarrier2, 0, d3FmCarrierMixer, 1);
AudioConnection          patchCord15(d1PitchEnv, 0, d1OscSine, 0);
AudioConnection          patchCord16(d1PitchEnv, 0, d1OscSaw, 0);
AudioConnection          patchCord17(d1PitchEnv, 0, d1OscSquare, 0);
AudioConnection          patchCord18(d2AmpEnv, 0, d2VoiceMixer, 0);
AudioConnection          patchCord19(d2AmpEnv, 0, masterWfInputMixer, 3);
AudioConnection          patchCord20(d2NoiseEnv, 0, d2NoiseFilter, 0);
AudioConnection          patchCord21(d2ClickTransient, 0, d2ClickFilter, 0);
AudioConnection          patchCord22(d3FmCarrier1, 0, d3FmCarrierMixer, 0);
AudioConnection          patchCord23(clapAmpEnv1, 0, clapFilter1, 0);
AudioConnection          patchCord24(clapAmpEnv2, 0, clapFilter2, 0);
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
AudioConnection          patchCord38(d3606BusMixer, 0, d3606HighPass, 0);
AudioConnection          patchCord39(d2VoiceMixer, 0, d2VoiceHighPass, 0);
AudioConnection          patchCord40(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord41(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord42(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord43(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord44(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord45(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord46(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord47(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord48(d3FmBandPass, 1, d3FmHighPass, 0);
AudioConnection          patchCord49(d3606HighPass, 2, d3606BandPass, 0);
AudioConnection          patchCord50(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord51(d1AmpEnv, 0, masterWfInputMixer, 2);
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
AudioConnection          patchCord67(d3PercFilter, 1, d3VoiceMixer, 2);
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
AudioConnection          patchCord110(finalFilter, masterRolloff);
AudioConnection          patchCord111(masterRolloff, finalAmp);
AudioConnection          patchCord112(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord113(finalAmp, 0, i2s1, 1);
AudioConnection          patchCord114(finalAmp, usbTrim);
AudioConnection          patchCord115(usbTrim, 0, usb1, 0);
AudioConnection          patchCord116(usbTrim, 0, usb1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=4183,1515
// GUItool: end automatically generated code
