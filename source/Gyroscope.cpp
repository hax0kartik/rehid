#include "Gyroscope.hpp"
#include "mcuhid.hpp"
extern "C" {
#include "gpio.h"
#include "i2c.h"
}

void Gyroscope::DisableSampling() {
    m_internalstruct.sampling = 0;
    uint8_t variant = m_internalstruct.variant;

    if (variant == 2) {
        I2C_WriteRegister8(m_internalstruct.devid, 34, 0);
        I2C_EnableRegisterBits8(m_internalstruct.devid, 57, 32);
        I2C_DisableRegisterBits8(m_internalstruct.devid, 32, 8);
    } else {
        I2C_WriteRegister8(m_internalstruct.devid, m_gyroregs[variant].interruptconfig, 0); // Disable Interrupts
        I2C_EnableRegisterBits8(m_internalstruct.devid, m_gyroregs[variant].powermgm, variant | 64); // Enable Sleep
    }

    m_internalstruct.sleeping = 1;
    m_enabled = false;
}

void Gyroscope::SetSamplingRate(uint8_t samplingrate) {
    uint8_t variant = m_internalstruct.variant;

    if (variant == 2)
        return (void)I2C_WriteRegister8(m_internalstruct.devid, 36, 0);

    I2C_WriteRegister8(m_internalstruct.devid, m_gyroregs[variant].dlpf, samplingrate);

    if (variant == 1)
        I2C_WriteRegister8(m_internalstruct.devid, m_gyroregs[variant].fssel, 24);
}

void Gyroscope::InternalSetup() {
    uint8_t variant = m_internalstruct.variant;
    SetSamplingRate(0x1A); // This sets the sampiling rate to 98Hz/92Hz

    if (variant == 2) {
        I2C_WriteRegister8(m_internalstruct.devid, 35, 240);
        I2C_DisableRegisterBits8(m_internalstruct.devid, 32, 240);
        I2C_WriteRegister8(m_internalstruct.devid, 34, 8);
        I2C_EnableRegisterBits8(m_internalstruct.devid, 57, 32);
    } else {
        I2C_WriteRegister8(m_internalstruct.devid, m_gyroregs[variant].sampleratedivider, 9); // Set to 100Hz
        I2C_WriteRegister8(m_internalstruct.devid, m_gyroregs[variant].interruptconfig, 1); // Enable Data Ready Interrupt
    }

    m_internalstruct.sleeping = 0;
}

void Gyroscope::SetupForSampling() {
    if (m_internalstruct.sampling)
        return;

    InternalSetup();
    m_internalstruct.sampling = 1;
}

void Gyroscope::EnableSampling() {
    if (m_internalstruct.variant == 2)
        I2C_EnableRegisterBits8(m_internalstruct.devid, 32, 8);
    else
        I2C_DisableRegisterBits8(m_internalstruct.devid, m_gyroregs[m_internalstruct.variant].powermgm, 64); // Disable Sleep

    m_enabled = true;
}

Result Gyroscope::InternalInit() {
    Result ret = 0;
    ret = GPIOHID_SetRegPart1(0, 0x100);

    if (R_SUCCEEDED(ret)) {
        GPIOHID_SetRegPart2(1, 0x100);
        GPIOHID_SetInterruptMask(0x100, 0x100);
        m_internalstruct.variant = 0;
        m_internalstruct.devid = 10;

        // Reset
        if (R_FAILED(I2C_DisableRegisterBits8(10, m_gyroregs[0].powermgm, 64))) {
            m_internalstruct.variant = 1;
            m_internalstruct.devid = 11;

            if (R_FAILED(I2C_DisableRegisterBits8(11, m_gyroregs[1].powermgm, 64))) {
                m_internalstruct.variant = 2;
                m_internalstruct.devid = 9;

                if (R_FAILED(I2C_EnableRegisterBits8(9, 32, 8)))
                    *(u32*)ret = 0x987;

            }
        }

        if (m_internalstruct.variant == 2)
            I2C_EnableRegisterBits8(m_internalstruct.devid, 57, 4);
        else
            I2C_EnableRegisterBits8(m_internalstruct.devid, m_gyroregs[m_internalstruct.variant].powermgm, 128); // Reset Mask

        svcSleepThread(4 * 1000 * 1000);
        m_internalstruct.sleeping = 0;
        m_internalstruct.sampling = 1;
        // By default we disable/sleep so as to ensure that gyroscope does not run at all times and battery is saved
        DisableSampling();
    }

    return ret;
}

void Gyroscope::ReadGyroData(GyroscopeEntry *entry) {
    uint8_t data[6];

    if (m_internalstruct.variant == 2) {
        I2C_ReadRegisterBuffer8(m_internalstruct.devid, 0xA8, (u8*)&data[0], 6);
        entry->x = (data[2] << 8) | data[3];
        entry->y = -((data[0] << 8) | data[1]);
        entry->z = (data[4] << 8) | data[5];
    } else {
        I2C_ReadRegisterBuffer8(m_internalstruct.devid, m_gyroregs[m_internalstruct.variant].gyroxout, (u8*)&data[0], 6);
        entry->x = (data[0] << 8) | data[1];
        entry->y = (data[2] << 8) | data[3];
        entry->z = (data[4] << 8) | data[5];
    }
}

void Gyroscope::GetCalibParam(GyroscopeCalibrateParam *param) {
    param->zeroX = m_calibparam.zeroX;
    param->plusX = m_calibparam.plusX;
    param->minusX = m_calibparam.minusX;
    param->zeroY = -m_calibparam.zeroZ;
    param->plusY = m_calibparam.plusZ;
    param->minusY = m_calibparam.minusZ;
    param->zeroZ = m_calibparam.zeroY;
    param->plusZ = m_calibparam.plusY;
    param->minusZ = m_calibparam.minusY;
}

void Gyroscope::Initialize() {
    if (!m_initialized) {
        cfguInit();
        Result ret = CFG_GetConfigInfoBlk4(0x12, 0x40002u, &m_calibparam);

        if (R_FAILED(ret))
            *(u32*)0x127 = ret;

        InternalInit();
        svcCreateEvent(&m_intrevent, RESET_ONESHOT);

        if (R_FAILED(GPIOHID_BindInterrupt(&m_intrevent)))
            *(u32*)0x7 = 0x1233;

        for (int i = 0; i < 6; i++)
            if (R_FAILED(svcCreateEvent(&m_event[i], RESET_ONESHOT)))
                *(u32*)0x8 = 0x1234;

        m_initialized = 1;
    }
}

void Gyroscope::Sampling() {
    GyroscopeEntry rawvals;
    GyroscopeEntry fixedvals;
    ReadGyroData(&rawvals);
    fixedvals.x = rawvals.x;

    if (rawvals.z == -32768)
        rawvals.z = -32767;

    fixedvals.y = -rawvals.z;
    fixedvals.z = rawvals.y;

    m_ring->SetRaw(rawvals);
    m_ring->WriteToRing(fixedvals);

    for (int i = 0; i < 6; i++)
        svcSignalEvent(m_event[i]);

    //svcClearEvent(m_qtmevent);
    //if(R_FAILED(svcSignalEvent(m_event))) *(u32*)0xF = 0x98765;
    //if(R_FAILED(svcSignalEvent(m_qtmevent))) *(u32*)0xF = 0x98764;
}