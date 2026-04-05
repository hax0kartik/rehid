#include <3ds.h>
#include <cstdio>
#include <cstring>
#include "IRUserMock.hpp"
#include "../crc8.hpp"

char buf[0x100];

Result IRUserMock::InitializeIrNopShared(size_t sharedbufsize, size_t recvbufsize,
       size_t recvbufpacketcount, size_t sendbufsize, size_t sendbufpacketcount,
       uint8_t baudrate, Handle shmemhandle) {
    if (m_initialized) {
        return 0xD8210FF9;
    }

    Result ret = 0;

    m_shmemhandle = shmemhandle;

    m_shmemaddr = (uint32_t)mappableAlloc(sharedbufsize);
    // sprintf(buf, "m_sharedmemaddr: %08X, m_sharedmemend: %08X\n", m_shmemaddr, m_shmemaddr + sharedbufsize);
    // svcOutputDebugString(buf, strlen(buf));

    // sprintf(buf, "recvbufsize: %08X, recvbufpacketcount: %08X  \n", recvbufsize, recvbufpacketcount);
    // svcOutputDebugString(buf, strlen(buf));

    if (R_FAILED(ret = svcMapMemoryBlock(m_shmemhandle, m_shmemaddr, (MemPerm)(MEMPERM_READ | MEMPERM_WRITE), MEMPERM_READ))) {
        mappableFree((void *)m_shmemaddr);
        m_shmemaddr = 0;
        return ret;
    }

    m_shmem = (CPPSharedMem *)m_shmemaddr;

    for (int i = 0; i < 3; i++) {
        svcCreateEvent(&m_handles[i], RESET_ONESHOT);
    }

    LightLock_Init(&m_lock);

    m_recvbufcount = recvbufpacketcount;
    m_recvbufsize = recvbufsize;

    /* skip to the end of the struct and to start of where actual packets live */
    m_start = ((uint8_t *)&m_shmem->packetinfos[m_recvbufcount]);
    m_end = (uint8_t *)m_shmemaddr + m_recvbufsize + sizeof(CPPBufferInfo) + sizeof(CPPSharedMemHeader);
    m_curr = m_start;

    m_shmem->header.recieveresult = 0;
    m_shmem->header.sendresult = 0;
    m_shmem->header.connectionstatus = IRUSER_ConnectionStatus::Disconnected;
    m_shmem->header.tryingtoconnectstatus = 0;
    m_shmem->header.initialized = 0;
    m_shmem->header.connectionrole = 0;
    m_shmem->header.connected = 0;
    m_shmem->header.networkid = 0xAA; // This should be randomly generated but we do not care

    m_shmem->bufferinfo.packetcount = 0;
    m_shmem->bufferinfo.beginindex = 0;
    m_shmem->bufferinfo.endindex = 0;

    m_connected = false;
    m_initialized = 1;
    return 0;
}

Result IRUserMock::IrNopFinalize() {
    if (!m_initialized) {
        return 0xD8210FF8;
    }

    Result ret = 0;
    m_connected = false;

    LightLock_Lock(&m_lock);
    m_recvbufcount = 0;
    m_recvbufsize = 0;

    m_start = nullptr;
    m_end = nullptr;
    m_curr = nullptr;

    m_shmem->header.recieveresult = 0;
    m_shmem->header.sendresult = 0;
    m_shmem->header.connectionstatus = IRUSER_ConnectionStatus::Disconnected;
    m_shmem->header.tryingtoconnectstatus = 0;
    m_shmem->header.initialized = 0;
    m_shmem->header.connectionrole = 0;
    m_shmem->header.connected = 0;
    m_shmem->header.networkid = 0; // This should be randomly generated but we do not care
    m_shmem = nullptr;

    if (R_FAILED(ret = svcUnmapMemoryBlock(m_shmemhandle, m_shmemaddr))) {
        svcBreak(USERBREAK_ASSERT);
    }

    svcCloseHandle(m_shmemhandle);
    m_shmemhandle = 0;
    m_shmemaddr = 0;

    for (int i = 0; i < 3; i++) {
        svcCloseHandle(m_handles[i]);
    }

    LightLock_Unlock(&m_lock);

    m_initialized = 0;
    return 0;
}

Result IRUserMock::ClearReceiveBuffer() {
    LightLock_Lock(&m_lock);
    m_recvbufcount = 0;
    m_recvbufsize = 0;

    m_start = nullptr;
    m_end = nullptr;
    m_curr = nullptr;
    LightLock_Unlock(&m_lock);

    return 0;
}

Result IRUserMock::RequireConnection(uint8_t machineid) {
    if (machineid != 1)
        svcBreak(USERBREAK_ASSERT); // Trying to connect to a device other than cpp is not supported

    if (m_shmem->header.connectionstatus == IRUSER_ConnectionStatus::Connected) {
        return 0xC8A10C0B; // We are already connected
    }

    m_shmem->header.machineid = machineid;

    // Mark network as connected, even when not
    m_shmem->header.connectionstatus = IRUSER_ConnectionStatus::Connected;
    m_shmem->header.connected = 1;
    m_shmem->header.initialized = 1;

    // signal that we got connected
    svcSignalEvent(m_handles[CONNECTION_EVENT_HANDLE_IDX]);
    return 0;
}

Result IRUserMock::SendIrnop(uint8_t *buff, size_t size) {
    if (m_shmem->header.connectionstatus != IRUSER_ConnectionStatus::Connected) {
        return 0xC8A10C0D;
    }

    // sprintf(buf, "SendIrnop Called type : %X\n", buff[0]);
    // svcOutputDebugString(buf, strlen(buf));

    if (!buff) { // should not happen
        svcBreak(USERBREAK_ASSERT);
    }

    switch (buff[0]) {
        case 2: {
            CPPCalibrationRequest *request = (CPPCalibrationRequest *)buff;
            CPPCalibrationResponse response {};
            CreateCalibrationPayload(request, &response);
            AddPayload((uint8_t *)&response, sizeof(CPPCalibrationResponse));

            svcSignalEvent(m_handles[RECV_EVENT_HANDLE_IDX]);
            break;
        }

        case 1: {
            CPPInputResponse response {};
            CreateInputDataPayload(&response);
            AddPayload((uint8_t *)&response, sizeof(CPPInputResponse));
            m_connected = true;

            break;
        }
    }

    svcSignalEvent(m_handles[SEND_EVENT_HANDLE_IDX]);
    return 0;
}

Result IRUserMock::Disconnect() {
    if (m_shmem->header.connectionstatus == IRUSER_ConnectionStatus::Connected) {
        m_shmem->header.connectionstatus = IRUSER_ConnectionStatus::Disconnected;
        m_shmem->header.connected = 0;
        m_shmem->header.initialized = 0;
        m_connected = false;

        // Signal that we got disconnected
        svcSignalEvent(m_handles[CONNECTION_EVENT_HANDLE_IDX]);
    } else {
        return 0xC8A10C0D;
    }

    return 0;
}

void IRUserMock::CreateCalibrationPayload(CPPCalibrationRequest *request, CPPCalibrationResponse *response) {
    response->header = 0x11;

    /* as per 3dbrew, this remains the same */
    response->offset = request->offset;
    response->size = request->size;

    /* Write the default calibration for now, when possible this should come from cpp */
    for (int i = 0; i < 4; i++) {
        CPPCalibrationResponseData *candidate = &response->candidates[i];

        candidate->pad1 = 0;
        candidate->pad2[0] = 0;
        candidate->pad2[1] = 0;
        candidate->pad2[2] = 0;

        candidate->xoff = m_calibration.xoff;
        candidate->yoff = m_calibration.yoff;
        candidate->xscale = m_calibration.xscale;
        candidate->yscale = m_calibration.yscale;
        candidate->crc8 = crc8(candidate, 15);
    }

}

void IRUserMock::CreateInputDataPayload(CPPInputResponse *response) {
    response->header = 0x10;

    response->x = m_latestdata.x;
    response->y = m_latestdata.y;

    response->batterylevel = 0x1F; // max battery

    // Always keep them as true as this are set in pad
    response->zlup = true;
    response->zrup = true;
    response->rup = true;
}

void IRUserMock::AddPayload(uint8_t *buffer, size_t length) {
    /* Cannot add more packets */
    // sprintf(buf, "AddPayload packetcount : %d maxcount : %d\n", m_shmem->bufferinfo.packetcount, m_recvbufcount);
    // svcOutputDebugString(buf, strlen(buf));

    if (m_shmem->bufferinfo.packetcount == m_recvbufcount) {
        return;
    }

    LightLock_Lock(&m_lock);
    /* Calculate the packet header for the payload */
    uint8_t header[4] = {0};
    header[0] = 0xA5;
    header[1] = 0x1;

    /* max possible length is 0xFFFF */
    uint8_t headersize = 0;
    if (length < 0x40) {
        header[2] = length;
        headersize = 3;
    } else {
        header[2] = (1 << 6) | ((length & 0xFF00) >> 8); // higher byte of total length
        header[3] = (length & 0xFF); // lower byte of total length
        headersize = 4;
    }

    auto write = [&](uint8_t *buf2, size_t size) {
        for (size_t i = 0; i < size; i++) {
            if (m_curr >= m_end) {
                m_curr = m_start;
            }

            // sprintf(buf, "Writing %02X to %08X | Remaining size: %d\n", buf2[i], m_curr, size - i - 1);
            // svcOutputDebugString(buf, strlen(buf));

            *m_curr = buf2[i];
            m_curr++;
        }
    };

    /* offset is relative to m_start */
    uint32_t start = m_curr > m_start ? m_curr - m_start : m_start - m_curr;
    uint32_t size = 0;

    // sprintf(buf, "m_curr: %08X, m_start: %08X, offset: %08X, m_end: %08X\n", m_curr, m_start, start, m_end);
    // svcOutputDebugString(buf, strlen(buf));

    /* first write the header */
    write(header, headersize);
    size += headersize;

    /* Now write the payload */
    write(buffer, length);
    size += length;

    /* Finally the crc */
    uint8_t crc = crc8_loop(m_start, m_recvbufsize, (uint8_t*)(start + m_start), size);
    write(&crc, 1);
    size += 1;

    CPPPacketInfo *info = &m_shmem->packetinfos[m_shmem->bufferinfo.endindex];
    info->offset = start;
    info->size = size;

    m_shmem->bufferinfo.endindex = (m_shmem->bufferinfo.endindex + 1) % m_recvbufcount;
    m_shmem->bufferinfo.packetcount++;
    LightLock_Unlock(&m_lock);
}

Result IRUserMock::ReleaseRecievedData(uint32_t count) {
    Result res = 0;

    // sprintf(buf, "ReleaseRecievedData called count : %d\n", count);
    // svcOutputDebugString(buf, strlen(buf));

    if (m_shmem->header.connectionstatus == IRUSER_ConnectionStatus::Connected) {
        LightLock_Lock(&m_lock);
        if (m_shmem->bufferinfo.packetcount) {
            m_shmem->bufferinfo.beginindex = (m_shmem->bufferinfo.beginindex + count) % m_recvbufcount;
            m_shmem->bufferinfo.packetcount -= count;
        } else {
            res = 0xC8810FEF; // There is no data to release
        }
        LightLock_Unlock(&m_lock);
    } else {
        res = 0xC8A10C0D; // Can't use in the current state
    }

    return res;
}

void IRUserMock::SetCalibrationData(CPPCalibrationData *data) {
    // LightLock_Lock(&m_lock);
    m_calibration = *data;
    // LightLock_Unlock(&m_lock);
}
