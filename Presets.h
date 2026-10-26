#pragma once
#include <juce_core/juce_core.h>
#include <vector>

// Frames del wavetable (valor wtA/wtB):
// 0 sine | .143 tri | .286 saw | .429 square | .571 pulse | .714 folded | .857 sync | 1.0 real-under distorted
// Solo se listan los parametros distintos del default; el resto se resetea al cargar.
namespace laar
{
struct Preset { juce::String cat, name; std::vector<std::pair<juce::String, float>> v; };

inline const juce::StringArray& categories()
{
    // de mas mainstream a mas sucio
    static const juce::StringArray c { "ISRAELIZ", "JOSEADORES", "CART!", "SLAYR", "REAL UNDER" };
    return c;
}

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        // ---------------- ISRAELIZ: limpios, pop / y2k / surf / trap melodico
        {"ISRAELIZ", "Y2K Gloss Lead", {{"wtA",.286},{"wtB",.429},{"mix",.4},{"uni",5},{"uniDet",18},{"cut",6000},{"res",.15},{"fEnv",.3},{"drive",.1},{"aD",.4},{"aS",.7},{"aR",.35},{"fD",.4},{"fS",.3}}},
        {"ISRAELIZ", "Surf Pluck",     {{"wtA",.429},{"wtB",.143},{"mix",.3},{"uni",3},{"uniDet",8},{"cut",2500},{"res",.2},{"fEnv",.6},{"aD",.35},{"aS",.15},{"aR",.25},{"fD",.25},{"fS",0}}},
        {"ISRAELIZ", "Pop Stab",       {{"wtA",.286},{"wtB",.571},{"mix",.25},{"uni",3},{"uniDet",14},{"cut",4500},{"fEnv",.5},{"aD",.2},{"aS",.4},{"aR",.15},{"fD",.2},{"fS",.1}}},

        // ---------------- JOSEADORES: jerk / hyperpop brillante con mordida
        {"JOSEADORES", "Jerk Bounce Lead", {{"wtA",.429},{"wtB",.571},{"mix",.5},{"uni",2},{"uniDet",10},{"sub",.2},{"cut",3500},{"res",.3},{"fEnv",.5},{"drive",.25},{"aD",.25},{"aS",.5},{"fD",.25},{"fS",.2}}},
        {"JOSEADORES", "Hyper Glitter",    {{"wtA",.286},{"wtB",.857},{"mix",.5},{"detB",12},{"uni",7},{"uniDet",35},{"cut",9000},{"res",.25},{"drive",.2},{"fold",.2},{"aR",.3}}},
        {"JOSEADORES", "Rage Bell Saw",    {{"wtA",.286},{"wtB",.714},{"mix",.5},{"uni",5},{"uniDet",25},{"cut",5000},{"res",.35},{"fEnv",.4},{"drive",.4},{"aS",.6},{"fD",.3}}},

        // ---------------- CART!: fuzz, detune, reeses medios
        {"CART!", "Cart Fuzz Chord", {{"wtA",.286},{"wtB",.571},{"mix",.5},{"uni",5},{"uniDet",30},{"cut",2800},{"res",.3},{"drive",.55},{"fold",.1}}},
        {"CART!", "Opium Detune",    {{"wtA",.286},{"wtB",.286},{"mix",.5},{"detB",.25},{"uni",7},{"uniDet",45},{"cut",3500},{"res",.2},{"drive",.5},{"aA",.02},{"aR",.5}}},
        {"CART!", "Wavy Reese",      {{"wtA",.286},{"wtB",.286},{"mix",.5},{"detB",.12},{"uni",3},{"uniDet",20},{"sub",.5},{"cut",1400},{"res",.25},{"fEnv",.2},{"drive",.5},{"fold",.15},{"aS",1}}},

        // ---------------- SLAYR: rage / metalico / agresivo
        {"SLAYR", "Slayr Bite",   {{"wtA",.714},{"wtB",.857},{"mix",.5},{"uni",5},{"uniDet",22},{"cut",3200},{"res",.4},{"fEnv",.6},{"drive",.7},{"fold",.3},{"fD",.2},{"fS",.2}}},
        {"SLAYR", "Rage Siren",   {{"wtA",.286},{"wtB",.429},{"mix",.5},{"detB",7},{"uni",7},{"uniDet",40},{"cut",5500},{"res",.45},{"drive",.65},{"fold",.35},{"aA",.05}}},
        {"SLAYR", "Metal Reese",  {{"wtA",.857},{"wtB",.286},{"mix",.5},{"detB",.2},{"uni",4},{"uniDet",28},{"sub",.4},{"cut",1800},{"res",.5},{"drive",.8},{"fold",.25},{"aS",1}}},

        // ---------------- REAL UNDER: lo mas distorsionado
        {"REAL UNDER", "Real Under Sub", {{"wtA",1.0},{"wtB",0},{"mix",.3},{"sub",.7},{"cut",700},{"res",.2},{"drive",.9},{"fold",.4},{"aS",1}}},
        {"REAL UNDER", "Sewer Reese",    {{"wtA",1.0},{"wtB",.286},{"mix",.5},{"detB",.3},{"uni",5},{"uniDet",38},{"sub",.6},{"cut",1100},{"res",.55},{"fEnv",.3},{"drive",1.0},{"fold",.5},{"aS",1}}},
        {"REAL UNDER", "Dead Distort 808", {{"wtA",0},{"wtB",1.0},{"mix",.35},{"sub",1.0},{"cut",900},{"res",.1},{"fEnv",.2},{"drive",1.0},{"fold",.6},{"aD",1.5},{"aS",.6},{"aR",.4}}},
    };
    return p;
}
} // namespace laar
