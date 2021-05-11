#pragma once

#include <vector>
#include <cmath>

#define TAU 1
#define VTHRESH 10
#define VREST 0

struct LIF 
{
    float Vm = VREST;
    bool Simulate (float dt, float I=0) 
    {
        Vm = Vm * exp(-dt / TAU) + I;
        if(Vm < VTHRESH)
            return false;

        // Neuron fired.
        Vm = VREST;
        return true;
    }
};
