#ifndef AUDIOTOOL_H
#define AUDIOTOOL_H
// ============================================================================
//  Audio Graph — audiotool.h
//
//  Auto-generated Teensy Audio Design Tool graph plus manual additions.
//  Declares all AudioStream objects, connections, and the output DAC.
// ============================================================================

#include <Audio.h>
// GUItool: begin automatically generated code
AudioSynthWaveform       d3W2;           //xy=90.625,1285
AudioSynthWaveform       d3W4;           //xy=90.625,1324
AudioSynthWaveform       d3606W1;        //xy=178.625,1045
AudioSynthWaveform       d3606W2;        //xy=178.625,1077
AudioSynthWaveform       d3606W3;        //xy=178.625,1107
AudioSynthWaveform       d3606W4;        //xy=178.625,1137
AudioSynthWaveform       d3606W5;        //xy=178.625,1168
AudioSynthWaveform       d3606W6;        //xy=178.625,1199
AudioSynthWaveformModulated d3W3;           //xy=213.625,1330
AudioSynthWaveformModulated d3W1;           //xy=216.625,1291
AudioEffectEnvelope      d1PitchEnv;     //xy=303.625,132
AudioSynthWaveformDc     d1DC;           //xy=304.625,91
AudioSynthNoiseWhite     clapNoise2;     //xy=308.625,968
AudioMixer4              d3606Mixer1;    //xy=348.625,1092
AudioMixer4              d3606Mixer2;    //xy=348.625,1154
AudioMixer4              d3Mixer1;       //xy=354.625,1337
AudioSynthNoiseWhite     clapNoise1;     //xy=374.625,777
AudioEffectEnvelope      clap2AmpEnv;    //xy=466.625,967
AudioSynthWaveformModulated d1b;            //xy=516.625,117
AudioSynthWaveformModulated d1;             //xy=517.625,78
AudioSynthWaveformModulated d1c;            //xy=517.625,152
AudioMixer4              d3606MasterMixer; //xy=527.625,1127
AudioEffectEnvelope      clap1AmpEnv;    //xy=532.625,777
AudioMixer4              d3MasterMixer;  //xy=537.625,1385
AudioFilterStateVariable clap2Filter;    //xy=633.625,975
AudioSynthNoiseWhite     d2Noise;        //xy=661.625,510
AudioFilterStateVariable clap1Filter;    //xy=691.625,784
AudioFilterStateVariable d3606HPF;       //xy=696.625,1128
AudioFilterStateVariable d3BPF;          //xy=708.625,1386
AudioMixer4              d1OscMixer;     //xy=716.625,133
AudioSynthWaveformModulated d2;             //xy=744.625,359
AudioEffectEnvelope      d2NoiseEnvelope; //xy=830.625,510
AudioFilterStateVariable d3606BPF;       //xy=849.625,1147
AudioEffectDelay         clapDelay1;     //xy=853.625,784
AudioEffectDelay         clapDelay2;     //xy=854.625,906
AudioSynthSimpleDrum     drum3;          //xy=859.625,1522
AudioEffectEnvelope      d1AmpEnv;       //xy=868.625,134
AudioFilterStateVariable d3Filter;       //xy=865.625,1290
AudioEffectEnvelope      d2AmpEnv;       //xy=902.625,359
AudioEffectEnvelope      d3AmpEnv;       //xy=913.625,1384
AudioSynthSimpleDrum     d2Attack;       //xy=983.625,615
AudioFilterStateVariable d1LowPass;      //xy=1023.625,141
AudioEffectEnvelope      d3606AmpEnv;    //xy=1021.625,1147
AudioFilterStateVariable d2NoiseFilter;  //xy=1056.625,510
AudioSynthSimpleDrum     drum2;          //xy=1076.625,455
AudioMixer4              clapMixer1;     //xy=1078.625,758
AudioMixer4              clapMixer2;     //xy=1082.625,880
AudioFilterStateVariable d2AttackFilter; //xy=1130.625,622
AudioSynthSimpleDrum     drum1;          //xy=1198.625,166
AudioSynthWaveformDc     d1DCwf;         //xy=1232.625,261
AudioMixer4              d3WfMixer;      //xy=1229.625,1442
AudioSynthWaveformDc     d3DCwf;         //xy=1230.625,1600
AudioAmplifier           clapAmp;        //xy=1272.625,756
AudioFilterStateVariable clapMasterFilter; //xy=1282.625,827
AudioMixer4              d2Mixer;        //xy=1313.625,462
AudioAmplifier           drum1Amp;       //xy=1369.625,166
AudioSynthWaveformSine   d2WfSine;       //xy=1372.625,913
AudioEffectEnvelope      clapMasterEnv;  //xy=1389.625,604
AudioEffectWaveFolder    d1Wavefolder;   //xy=1396.625,255
AudioFilterStateVariable d2Filter;       //xy=1405.625,559
AudioEffectWaveFolder    d3Wavefolder;   //xy=1457.625,1449
AudioEffectFreeverb      d2Verb;         //xy=1485.625,395
AudioAmplifier           d2WfAmp;        //xy=1517.625,913
AudioMixer4              d1Mixer;        //xy=1560.625,175
AudioMixer4              snareClapMixer; //xy=1646.625,610
AudioEffectWaveFolder    d2Wavefolder;   //xy=1651.625,838
AudioMixer4              d3Mixer;        //xy=1683.625,1253
AudioMixer4              d2MasterMixer;  //xy=1687.625,401
AudioFilterStateVariable d1Filter;       //xy=1720.625,182
AudioFilterStateVariable d2WfLowpass;    //xy=1730.625,764
AudioFilterBiquad        d1EQ;           //xy=1883.625,193
AudioSynthWaveform       wfSine;      //xy=1890.148681640625,513.5773315429688
AudioSynthWaveform       wfSaw;      //xy=1890.091064453125,545.727294921875
AudioMixer4              drumMixer;      //xy=1896.625,406
AudioFilterLadder        d3MasterFilter; //xy=1910.625,1131
AudioMixer4              wfMixer;         //xy=2013.1817626953125,553.9091186523438
AudioAmplifier           d1Amp;          //xy=2034.625,194
AudioEffectWaveFolder    masterWf;       //xy=2075.8977127075195,437.36367893218994
AudioMixer4              masterMixer;    //xy=2296.625,418
AudioAmplifier           delayAmp;       //xy=2296.625,521
AudioEffectDelay         masterDelay;    //xy=2297.625,877
AudioFilterStateVariable delayFilter;    //xy=2298.625,720
AudioMixer4              delayMixer;     //xy=2299.625,613
AudioRecordQueue         scopeQueue;     //xy=2505.625,460
AudioFilterStateVariable masterHiPass;   //xy=2581.625,519
AudioFilterLadder        masterLowPass;  //xy=2583.625,581
AudioFilterStateVariable masterBandPass; //xy=2591.625,646
AudioAmplifier           masterAmp;      //xy=2769.625,646
AudioFilterBiquad        finalFilter;    //xy=2919.625,645
AudioAmplifier           finalAmp;       //xy=3054.625,644
AudioOutputI2S           i2s1;           //xy=3197.625,643
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
AudioConnection          patchCord15(d1DC, d1PitchEnv);
AudioConnection          patchCord16(clapNoise2, clap2AmpEnv);
AudioConnection          patchCord17(d3606Mixer1, 0, d3606MasterMixer, 0);
AudioConnection          patchCord18(d3606Mixer2, 0, d3606MasterMixer, 1);
AudioConnection          patchCord19(d3Mixer1, 0, d3MasterMixer, 0);
AudioConnection          patchCord20(clapNoise1, clap1AmpEnv);
AudioConnection          patchCord21(clap2AmpEnv, 0, clap2Filter, 0);
AudioConnection          patchCord22(d1b, 0, d1OscMixer, 1);
AudioConnection          patchCord23(d1, 0, d1OscMixer, 0);
AudioConnection          patchCord24(d1c, 0, d1OscMixer, 2);
AudioConnection          patchCord26(d3606MasterMixer, 0, d3606HPF, 0);
AudioConnection          patchCord27(clap1AmpEnv, 0, clap1Filter, 0);
AudioConnection          patchCord28(d3MasterMixer, 0, d3BPF, 0);
AudioConnection          patchCord29(clap2Filter, 1, clapDelay2, 0);
AudioConnection          patchCord30(d2Noise, d2NoiseEnvelope);
AudioConnection          patchCord31(clap1Filter, 1, clapDelay1, 0);
AudioConnection          patchCord32(d3606HPF, 2, d3606BPF, 0);
AudioConnection          patchCord33(d3BPF, 1, d3Filter, 0);
AudioConnection          patchCord34(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord35(d2, d2AmpEnv);
AudioConnection          patchCord36(d2NoiseEnvelope, 0, d2NoiseFilter, 0);
AudioConnection          patchCord37(d2NoiseEnvelope, 0, d2NoiseFilter, 1);
AudioConnection          patchCord38(d3606BPF, 1, d3606AmpEnv, 0);
AudioConnection          patchCord39(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord40(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord41(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord42(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord43(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord44(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord45(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord46(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord47(drum3, 0, d3WfMixer, 2);
AudioConnection          patchCord48(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord49(d3Filter, 2, d3AmpEnv, 0);
AudioConnection          patchCord50(d2AmpEnv, 0, d2Mixer, 0);
AudioConnection          patchCord51(d3AmpEnv, 0, d3WfMixer, 1);
AudioConnection          patchCord52(d2Attack, 0, d2AttackFilter, 0);
AudioConnection          patchCord53(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord54(d1LowPass, 0, d1Mixer, 0);
AudioConnection          patchCord55(d3606AmpEnv, 0, d3WfMixer, 0);
AudioConnection          patchCord56(d2NoiseFilter, 2, d2Mixer, 2);
AudioConnection          patchCord57(drum2, 0, d2Mixer, 1);
AudioConnection          patchCord58(clapMixer1, 0, clapMasterFilter, 0);
AudioConnection          patchCord59(clapMixer2, 0, clapMasterFilter, 0);
AudioConnection          patchCord60(d2AttackFilter, 1, d2Mixer, 3);
AudioConnection          patchCord61(drum1, drum1Amp);
AudioConnection          patchCord62(d1DCwf, 0, d1Wavefolder, 1);
AudioConnection          patchCord63(d3WfMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord64(d3WfMixer, 0, d3Mixer, 0);
AudioConnection          patchCord65(d3DCwf, 0, d3Wavefolder, 1);
AudioConnection          patchCord66(clapAmp, clapMasterEnv);
AudioConnection          patchCord67(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord68(d2Mixer, 0, d2Filter, 0);
AudioConnection          patchCord69(drum1Amp, 0, d1Mixer, 1);
AudioConnection          patchCord70(d2WfSine, d2WfAmp);
AudioConnection          patchCord71(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord72(d1Wavefolder, 0, d1Mixer, 3);
AudioConnection          patchCord73(d2Filter, 2, snareClapMixer, 0);
AudioConnection          patchCord74(d3Wavefolder, 0, d3Mixer, 1);
AudioConnection          patchCord75(d2Verb, 0, d2MasterMixer, 1);
AudioConnection          patchCord76(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord77(d1Mixer, 0, d1Filter, 0);
AudioConnection          patchCord78(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord79(snareClapMixer, d2Verb);
AudioConnection          patchCord80(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord81(snareClapMixer, 0, delayMixer, 1);
AudioConnection          patchCord82(d2Wavefolder, 0, d2WfLowpass, 0);
AudioConnection          patchCord83(d3Mixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord84(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord85(d1Filter, 2, d1EQ, 0);
AudioConnection          patchCord86(d2WfLowpass, 0, d2MasterMixer, 2);
AudioConnection          patchCord87(d1EQ, d1Amp);
AudioConnection          patchCord88(wfSine, 0, wfMixer, 0);
AudioConnection          patchCord89(wfSaw, 0, wfMixer, 1);
AudioConnection          patchCord90(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord91(drumMixer, 0, masterWf, 0);
AudioConnection          patchCord92(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord93(d3MasterFilter, 0, delayMixer, 2);
AudioConnection          patchCord94(wfMixer, 0, masterWf, 1);
AudioConnection          patchCord95(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord96(d1Amp, 0, delayMixer, 0);
AudioConnection          patchCord97(masterWf, 0, masterMixer, 1);
AudioConnection          patchCord98(masterMixer, 0, masterHiPass, 0);
AudioConnection          patchCord99(masterMixer, scopeQueue);
AudioConnection          patchCord100(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord101(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord102(delayFilter, 0, delayMixer, 3);
AudioConnection          patchCord103(delayMixer, masterDelay);
AudioConnection          patchCord104(delayMixer, delayAmp);
AudioConnection          patchCord105(masterHiPass, 2, masterLowPass, 0);
AudioConnection          patchCord106(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord107(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord108(masterAmp, finalFilter);
AudioConnection          patchCord109(finalFilter, finalAmp);
AudioConnection          patchCord110(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord111(finalAmp, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=1246.625,75
// GUItool: end automatically generated code

#endif
