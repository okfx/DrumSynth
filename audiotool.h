#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
// GUItool: begin automatically generated code
AudioSynthWaveformDc     d1DC;           //xy=100,80
AudioSynthWaveformModulated d2;             //xy=100,280
AudioSynthNoiseWhite     d2Noise;        //xy=100,320
AudioSynthNoiseWhite     clapNoise1;     //xy=100,500
AudioSynthNoiseWhite     clapNoise2;     //xy=100,560
AudioSynthWaveform       d3W2;           //xy=100,660
AudioSynthWaveform       d3W4;           //xy=100,700
AudioSynthWaveform       d3606W1;        //xy=100,740
AudioSynthWaveform       d3606W2;        //xy=100,780
AudioSynthWaveform       d3606W3;        //xy=100,820
AudioEffectEnvelope      d1PitchEnv;     //xy=260,100
AudioEffectEnvelope      d2AmpEnv;       //xy=260,280
AudioEffectEnvelope      d2NoiseEnvelope; //xy=260,320
AudioSynthSimpleDrum     d2Attack;       //xy=260,370
AudioEffectEnvelope      clap1AmpEnv;    //xy=260,500
AudioEffectEnvelope      clap2AmpEnv;    //xy=260,560
AudioSynthWaveformModulated d3W1;           //xy=260,660
AudioSynthWaveformModulated d3W3;           //xy=260,700
AudioSynthWaveform       d3606W4;        //xy=260,740
AudioSynthWaveform       d3606W5;        //xy=260,780
AudioSynthWaveform       d3606W6;        //xy=260,820
AudioSynthWaveformModulated d1;             //xy=420,70
AudioSynthWaveformModulated d1b;            //xy=420,110
AudioSynthWaveformModulated d1c;            //xy=420,150
AudioSynthSimpleDrum     drum2;          //xy=420,280
AudioFilterStateVariable d2NoiseFilter;  //xy=420,320
AudioFilterStateVariable d2AttackFilter; //xy=420,370
AudioFilterStateVariable clap1Filter;    //xy=420,500
AudioFilterStateVariable clap2Filter;    //xy=420,560
AudioMixer4              d3Mixer1;       //xy=420,680
AudioMixer4              d3606Mixer1;    //xy=420,760
AudioMixer4              d3606Mixer2;    //xy=420,820
AudioMixer4              d1OscMixer;     //xy=580,110
AudioMixer4              d2Mixer;        //xy=580,310
AudioEffectDelay         clapDelay1;     //xy=580,500
AudioEffectDelay         clapDelay2;     //xy=580,560
AudioMixer4              d3MasterMixer;  //xy=580,680
AudioMixer4              d3606MasterMixer; //xy=580,790
AudioEffectEnvelope      d1AmpEnv;       //xy=740,110
AudioFilterStateVariable d2Filter;       //xy=740,310
AudioMixer4              clapMixer1;     //xy=740,500
AudioMixer4              clapMixer2;     //xy=740,560
AudioFilterStateVariable d3BPF;          //xy=740,680
AudioFilterStateVariable d3606HPF;       //xy=740,790
AudioFilterStateVariable d1LowPass;      //xy=900,110
AudioSynthWaveformDc     d1DCwf;         //xy=900,180
AudioMixer4              clapMixerMaster; //xy=900,530
AudioFilterStateVariable d3Filter;       //xy=900,680
AudioFilterStateVariable d3606BPF;       //xy=900,790
AudioSynthSimpleDrum     drum1;          //xy=1060,60
AudioEffectWaveFolder    d1Wavefolder;   //xy=1060,170
AudioFilterStateVariable clapMasterFilter; //xy=1060,530
AudioEffectEnvelope      d3AmpEnv;       //xy=1060,680
AudioEffectEnvelope      d3606AmpEnv;    //xy=1060,790
AudioSynthSimpleDrum     drum3;          //xy=1060,880
AudioAmplifier           drum1Amp;       //xy=1220,60
AudioMixer4              d1Mixer;        //xy=1220,130
AudioSynthWaveformSine   d2WfSine;       //xy=1220,380
AudioAmplifier           clapAmp;        //xy=1220,530
AudioMixer4              d3WfMixer;      //xy=1220,880
AudioSynthWaveform       d3WfSine;       //xy=1220,950
AudioFilterStateVariable d1Filter;       //xy=1380,130
AudioMixer4              snareClapMixer; //xy=1380,310
AudioAmplifier           d2WfAmp;        //xy=1380,380
AudioEffectEnvelope      clapMasterEnv;  //xy=1380,530
AudioEffectWaveFolder    d3Wavefolder;   //xy=1380,910
AudioFilterBiquad        d1EQ;           //xy=1540,130
AudioEffectFreeverb      d2Verb;         //xy=1540,260
AudioEffectWaveFolder    d2Wavefolder;   //xy=1540,380
AudioMixer4              d3Mixer;        //xy=1540,900
AudioAmplifier           d1Amp;          //xy=1700,130
AudioMixer4              d2MasterMixer;  //xy=1700,300
AudioFilterStateVariable d2WfLowpass;    //xy=1700,380
AudioFilterLadder        d3MasterFilter; //xy=1700,900
AudioSynthWaveform       wfSine;         //xy=1700,1090
AudioSynthWaveform       wfSaw;          //xy=1700,1130
AudioMixer4              drumMixer;      //xy=1860,1050
AudioMixer4              wfMixer;        //xy=1860,1110
AudioEffectWaveFolder    masterWf;       //xy=2020,1070
AudioMixer4              masterMixer;    //xy=2180,1070
AudioMixer4              delayMixer;     //xy=2180,1180
AudioRecordQueue         scopeQueue;     //xy=2340,1020
AudioFilterStateVariable masterHiPass;   //xy=2340,1070
AudioAmplifier           delayAmp;       //xy=2340,1140
AudioEffectDelay         masterDelay;    //xy=2340,1230
AudioFilterLadder        masterLowPass;  //xy=2500,1070
AudioFilterStateVariable delayFilter;    //xy=2500,1230
AudioFilterStateVariable masterBandPass; //xy=2660,1070
AudioAmplifier           masterAmp;      //xy=2820,1070
AudioFilterBiquad        finalFilter;    //xy=2980,1070
AudioAmplifier           finalAmp;       //xy=3140,1070
AudioOutputI2S           i2s1;           //xy=3300,1070
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
AudioConnection          patchCord14(d1DC, d1PitchEnv);
AudioConnection          patchCord15(clapNoise2, clap2AmpEnv);
AudioConnection          patchCord16(d3606Mixer1, 0, d3606MasterMixer, 0);
AudioConnection          patchCord17(d3606Mixer2, 0, d3606MasterMixer, 1);
AudioConnection          patchCord18(d3Mixer1, 0, d3MasterMixer, 0);
AudioConnection          patchCord19(clapNoise1, clap1AmpEnv);
AudioConnection          patchCord20(clap2AmpEnv, 0, clap2Filter, 0);
AudioConnection          patchCord21(d1b, 0, d1OscMixer, 1);
AudioConnection          patchCord22(d1, 0, d1OscMixer, 0);
AudioConnection          patchCord23(d1c, 0, d1OscMixer, 2);
AudioConnection          patchCord24(d3606MasterMixer, 0, d3606HPF, 0);
AudioConnection          patchCord25(clap1AmpEnv, 0, clap1Filter, 0);
AudioConnection          patchCord26(d3MasterMixer, 0, d3BPF, 0);
AudioConnection          patchCord27(clap2Filter, 1, clapDelay2, 0);
AudioConnection          patchCord28(d2Noise, d2NoiseEnvelope);
AudioConnection          patchCord29(clap1Filter, 1, clapDelay1, 0);
AudioConnection          patchCord30(d3606HPF, 2, d3606BPF, 0);
AudioConnection          patchCord31(d3BPF, 1, d3Filter, 0);
AudioConnection          patchCord32(d1OscMixer, d1AmpEnv);
AudioConnection          patchCord33(d2, d2AmpEnv);
AudioConnection          patchCord34(d2NoiseEnvelope, 0, d2NoiseFilter, 0);
AudioConnection          patchCord35(d2NoiseEnvelope, 0, d2NoiseFilter, 1);
AudioConnection          patchCord36(d3606BPF, 1, d3606AmpEnv, 0);
AudioConnection          patchCord37(clapDelay1, 0, clapMixer1, 0);
AudioConnection          patchCord38(clapDelay1, 1, clapMixer1, 1);
AudioConnection          patchCord39(clapDelay1, 2, clapMixer1, 2);
AudioConnection          patchCord40(clapDelay1, 3, clapMixer1, 3);
AudioConnection          patchCord41(clapDelay2, 0, clapMixer2, 0);
AudioConnection          patchCord42(clapDelay2, 1, clapMixer2, 1);
AudioConnection          patchCord43(clapDelay2, 2, clapMixer2, 2);
AudioConnection          patchCord44(clapDelay2, 3, clapMixer2, 3);
AudioConnection          patchCord45(drum3, 0, d3WfMixer, 2);
AudioConnection          patchCord46(d1AmpEnv, 0, d1LowPass, 0);
AudioConnection          patchCord47(d1AmpEnv, 0, wfMixer, 2);
AudioConnection          patchCord48(d3Filter, 2, d3AmpEnv, 0);
AudioConnection          patchCord49(d2AmpEnv, 0, d2Mixer, 0);
AudioConnection          patchCord50(d2AmpEnv, 0, wfMixer, 3);
AudioConnection          patchCord51(d3AmpEnv, 0, d3WfMixer, 1);
AudioConnection          patchCord52(d2Attack, 0, d2AttackFilter, 0);
AudioConnection          patchCord53(clapMixer1, 0, clapMixerMaster, 0);
AudioConnection          patchCord54(clapMixer2, 0, clapMixerMaster, 1);
AudioConnection          patchCord55(d1LowPass, 0, d1Wavefolder, 0);
AudioConnection          patchCord56(d1LowPass, 0, d1Mixer, 0);
AudioConnection          patchCord57(d3606AmpEnv, 0, d3WfMixer, 0);
AudioConnection          patchCord58(d2NoiseFilter, 2, d2Mixer, 2);
AudioConnection          patchCord59(drum2, 0, d2Mixer, 1);
AudioConnection          patchCord60(d2AttackFilter, 1, d2Mixer, 3);
AudioConnection          patchCord61(clapMixerMaster, 0, clapMasterFilter, 0);
AudioConnection          patchCord62(drum1, drum1Amp);
AudioConnection          patchCord63(d1DCwf, 0, d1Wavefolder, 1);
AudioConnection          patchCord64(d3WfMixer, 0, d3Wavefolder, 0);
AudioConnection          patchCord65(d3WfMixer, 0, d3Mixer, 0);
AudioConnection          patchCord66(d3WfSine, 0, d3Wavefolder, 1);
AudioConnection          patchCord67(d2Mixer, 0, d2Filter, 0);
AudioConnection          patchCord68(clapAmp, clapMasterEnv);
AudioConnection          patchCord69(clapMasterFilter, 2, clapAmp, 0);
AudioConnection          patchCord70(drum1Amp, 0, d1Mixer, 1);
AudioConnection          patchCord71(d2WfSine, d2WfAmp);
AudioConnection          patchCord72(clapMasterEnv, 0, snareClapMixer, 1);
AudioConnection          patchCord73(d1Wavefolder, 0, d1Mixer, 3);
AudioConnection          patchCord74(d2Filter, 2, snareClapMixer, 0);
AudioConnection          patchCord75(d3Wavefolder, 0, d3Mixer, 1);
AudioConnection          patchCord76(d2Verb, 0, d2MasterMixer, 1);
AudioConnection          patchCord77(d2WfAmp, 0, d2Wavefolder, 1);
AudioConnection          patchCord78(d1Mixer, 0, d1Filter, 0);
AudioConnection          patchCord79(snareClapMixer, 0, d2MasterMixer, 0);
AudioConnection          patchCord80(snareClapMixer, d2Verb);
AudioConnection          patchCord81(snareClapMixer, 0, d2Wavefolder, 0);
AudioConnection          patchCord82(snareClapMixer, 0, delayMixer, 1);
AudioConnection          patchCord83(d2Wavefolder, 0, d2WfLowpass, 0);
AudioConnection          patchCord84(d3Mixer, 0, d3MasterFilter, 0);
AudioConnection          patchCord85(d2MasterMixer, 0, drumMixer, 1);
AudioConnection          patchCord86(d1Filter, 2, d1EQ, 0);
AudioConnection          patchCord87(d2WfLowpass, 0, d2MasterMixer, 2);
AudioConnection          patchCord88(d1EQ, d1Amp);
AudioConnection          patchCord89(wfSine, 0, wfMixer, 0);
AudioConnection          patchCord90(wfSaw, 0, wfMixer, 1);
AudioConnection          patchCord91(drumMixer, 0, masterMixer, 0);
AudioConnection          patchCord92(drumMixer, 0, masterWf, 0);
AudioConnection          patchCord93(d3MasterFilter, 0, drumMixer, 2);
AudioConnection          patchCord94(d3MasterFilter, 0, delayMixer, 2);
AudioConnection          patchCord95(wfMixer, 0, masterWf, 1);
AudioConnection          patchCord96(d1Amp, 0, drumMixer, 0);
AudioConnection          patchCord97(d1Amp, 0, delayMixer, 0);
AudioConnection          patchCord98(masterWf, 0, masterMixer, 1);
AudioConnection          patchCord99(masterMixer, 0, masterHiPass, 0);
AudioConnection          patchCord100(masterMixer, scopeQueue);
AudioConnection          patchCord101(delayAmp, 0, masterMixer, 2);
AudioConnection          patchCord102(masterDelay, 0, delayFilter, 0);
AudioConnection          patchCord103(delayFilter, 0, delayMixer, 3);
AudioConnection          patchCord104(delayMixer, masterDelay);
AudioConnection          patchCord105(delayMixer, delayAmp);
AudioConnection          patchCord106(masterHiPass, 2, masterLowPass, 0);
AudioConnection          patchCord107(masterLowPass, 0, masterBandPass, 0);
AudioConnection          patchCord108(masterBandPass, 1, masterAmp, 0);
AudioConnection          patchCord109(masterAmp, finalFilter);
AudioConnection          patchCord110(finalFilter, finalAmp);
AudioConnection          patchCord111(finalAmp, 0, i2s1, 0);
AudioConnection          patchCord112(finalAmp, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=3300,1140
// GUItool: end automatically generated code
