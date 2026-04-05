#pragma once
#include <3ds.h>
#include "CPPDefs.hpp"
#include "ir.hpp"
#include "services/iruser.hpp"
#include "mock/IRUserMock.hpp"

extern "C" {
    #include "mythread.h"
}

enum {
    SAR_EXIT = -1,
    SAR_READERROR = -2,
    SAR_TIMEOUT = -3,
};

constexpr size_t CPP_SHARED_MEM_SIZE = 0x1000;
constexpr size_t CPP_RECV_PACKET_COUNT = 0xA0;
constexpr size_t CPP_SEND_PACKET_COUNT = 0x20;
constexpr size_t CPP_RECV_DATA_SIZE = 2000;
constexpr size_t CPP_SEND_DATA_SIZE = 0x200;
constexpr uint8_t CPP_BAUD_RATE = 4;
constexpr uint64_t MILLIS = 1000000;

class CPPO3DS : public IR {
public:
    Result Initialize() override;

    uint8_t IsInitialized() override {
        return m_initialized;
    }

    void Sampling() override;

    void SetTimer() override;

    Handle *GetTimer() override {
        return &m_timer;
    }

    void ScanInput(Remapper *remapper) override;

    virtual CPPEntry GetInputs() override {
        return m_cppstate;
    }

    virtual uint8_t IsEnteringSleep() override {
        return 0;
    }

    virtual void SetEnteringSleep(uint8_t val) override {
        m_sleep = val;
        if (m_sleep) {
            svcSignalEvent(*GetExitEvent());
            MyThread_Join(&m_thread, 1000 * MILLIS);
        } else {
            m_ring.Reset();
            m_latestkeys = 0;
            m_cppstate = {};
            MCUHWC_SetPowerLedState(LED_OFF);
            CreateThread(); 
        }

        m_mock.DoSleep(val);
        return;
    }

    void SetConnected(bool connected) {
        m_isconnected = connected;
    }

    bool IsConnected() override {
        return m_isconnected;
    }

    void Disconnect();

    int WaitForConnection();
    int GetCalibrationData();
    int GetInputPackets();

    Handle *GetConnectionEvent() {
        return &m_events[0];
    }

    Handle *GetRecvEvent() {
        return &m_events[1];
    }

    Handle *GetSharedMemHandle() {
        return &m_sharedmemhandle;
    }

    Handle *GetExitEvent() {
        return &m_events[2];
    }

    CPPSharedMem *GetSharedMem() {
        return m_sharedmem;
    }

    LightLock *GetSleepLock() {
        return &m_sleeplock;
    }

    Handle *GetSleepEvent() {
        return &m_events[3];
    }

    bool GetSleep() {
        return m_sleep;
    }

    Result CreateThread();

    IRUserMock *GetMockIRUSERCommands() {
        return &m_mock;
    }

private:
    Result SendReceive(const void *request, size_t requestsize, void *response, int responsesize, Handle *events, int64_t timeout, uint8_t expectedhead);
    int ReadPacket(void *out, size_t outlen);
    Result ClearPackets(int count);
    CPPEntry GetLatestInputFromRing();

    uint8_t m_initialized = 0;

    CPPSharedMem *m_sharedmem = nullptr;
    Handle m_sharedmemhandle;
    Handle m_connectionstatushandle;
    Handle m_events[4];
    Handle m_timer;

    LightLock m_sleeplock;

    MyThread m_thread;

    static constexpr size_t m_recvbufsize = CPP_RECV_DATA_SIZE + CPP_RECV_PACKET_COUNT * 8;
    static constexpr size_t m_sendbufsize = CPP_SEND_DATA_SIZE + CPP_SEND_PACKET_COUNT * 8;

    uint8_t m_packetid = 0;

    bool m_isconnected = false;
    bool m_sleep = false;

    uint8_t m_calibrationpacketcount = 0;
    CPPCalibrationData m_calibrationdata {};

    uint32_t m_latestkeys = 0;
    CirclePadEntry m_latestentry {};
    CirclePadEntry m_latestentryraw {};

    uint32_t m_oldkeys = 0;
    CPPEntry m_cppstate {};

    PacketType m_expectedpacket = PacketType::None;
    RecvPackageErrorType m_lastreaderror = RecvPackageErrorType::None;

    CPPRing m_ring {};

    IRUserMock m_mock {};
};