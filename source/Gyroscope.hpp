#pragma once
#include "GyroscopeRing.hpp"

struct GyroscopeInternalStruct
{
    uint8_t devid;
    uint8_t variant;
    uint8_t sampling;
    uint8_t sleeping;
};

struct GyroscopeRegs
{
    uint8_t sampleratedivider;
    uint8_t dlpf;
    uint8_t fssel;
    uint8_t interruptconfig;
    uint8_t gyroxout;
    uint8_t powermgm;
};

struct GyroscopeCalibrateParam
{
    int16_t zeroX;
    int16_t plusX;
    int16_t minusX;
    int16_t zeroY;
    int16_t plusY;
    int16_t minusY;
    int16_t zeroZ;
    int16_t plusZ;
    int16_t minusZ;
};

class Gyroscope
{
    public:
        void Initialize();
        void Sampling();
        void SetGyroscopeRing(GyroscopeRing *ring) { m_ring = ring; };
        Handle *GetEvent() { return &m_event[m_refcount]; };
        Handle *GetIntrEvent() { return &m_intrevent; };
        void IncreementHandleIndex() { ++m_refcount; };
        void DecreementhandleIndex() { --m_refcount; };
        uint8_t GetRefCount() { return m_refcount; };
        void EnableSampling();
        void DisableSampling();
        void SetupForSampling();
        void GetCalibParam(GyroscopeCalibrateParam *param);
        uint64_t timeenable;
        uint8_t m_issetupdone = 0;
    private:
        Result InternalInit();
        void SetSamplingRate(uint8_t samplingrate);
        void InternalSetup();
        void ReadGyroData(GyroscopeEntry *entry);
        GyroscopeRing *m_ring = nullptr;
        Handle m_event[6];
        Handle m_intrevent;
        uint8_t m_refcount = 0; 
        uint8_t m_initialized = 0;
        GyroscopeInternalStruct m_internalstruct = { 0 };
        GyroscopeRegs m_gyroregs[2] = {
            {.sampleratedivider = 0x15, .dlpf = 0x16, .fssel = 0, .interruptconfig = 0x17, .gyroxout = 0x1D, .powermgm = 0x3E},
            {.sampleratedivider = 0x19, .dlpf = 0x1A, .fssel = 0x1B, .interruptconfig = 0x38, .gyroxout = 0x43, .powermgm = 0x6B}
        };
        GyroscopeCalibrateParam m_calibparam;
};