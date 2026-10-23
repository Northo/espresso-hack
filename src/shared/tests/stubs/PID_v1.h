#pragma once
#define DIRECT 0
#define P_ON_M 0
#define AUTOMATIC 1
#define MANUAL 0
class PID {
public:
    template<class... T> PID(T...) {}
    void SetTunings(double, double, double) {}
    void SetMode(int) {}
    void SetOutputLimits(double, double) {}
    void SetSampleTime(int) {}
    void Compute() {}
};
