#include "../src/GrooveCore.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void writeLE16(std::ostream& out, uint16_t x) {
    out.put(char(x & 255)); out.put(char((x >> 8) & 255));
}
static void writeLE32(std::ostream& out, uint32_t x) {
    writeLE16(out, uint16_t(x & 65535)); writeLE16(out, uint16_t(x >> 16));
}
int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "HYPNO_GROOVE_preview.wav";
    constexpr int sr=44100; constexpr double bpm=124.; constexpr int bars=8;
    constexpr int count=int(bars*4*60/bpm*sr);
    std::vector<float> mix(count,0.f);
    std::array<std::vector<float>,hypno::lanes> sounds;
    for(int lane=0;lane<hypno::lanes;++lane) sounds[lane]=hypno::makeFactoryHit(lane,sr);
    const float levels[hypno::lanes]={.87f,.67f,.45f,.38f,.30f,.32f};
    for(int step=0;step<bars*16;++step){
        const auto pattern=step<bars*8?hypno::patternA():hypno::patternB();
        for(int lane=0;lane<hypno::lanes;++lane){
            if(!hypno::isActive(pattern[lane],step%16)) continue;
            int start= int(std::round(hypno::stepPpq(step,18.f)*60/bpm*sr));
            const auto& sound=sounds[lane];
            const float gain=levels[lane]*hypno::stepVelocity(step,lane,12.f);
            for(size_t i=0;i<sound.size()&&start+int(i)<count;++i)
                mix[size_t(start)+i]+=sound[i]*gain;
        }
    }
    std::ofstream file(path,std::ios::binary);
    file.write("RIFF",4);writeLE32(file,36+count*2);file.write("WAVEfmt ",8);
    writeLE32(file,16);writeLE16(file,1);writeLE16(file,1);writeLE32(file,sr);
    writeLE32(file,sr*2);writeLE16(file,2);writeLE16(file,16);
    file.write("data",4);writeLE32(file,count*2);
    for(int i=0;i<count;++i){
        const float fade=std::min(1.f,std::min(i/(sr*.08f),(count-i)/(sr*.4f)));
        const float soft=std::tanh(mix[size_t(i)]*.60f)*fade;
        writeLE16(file,uint16_t(int16_t(std::round(std::clamp(soft,-1.f,1.f)*32767.f))));
    }
    std::cout<<"Rendered "<<bars<<" bars to "<<path<<" ("<<count<<" samples)\n";
}
