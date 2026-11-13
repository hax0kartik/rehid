#pragma once
#include <string>
#include <cstdio>
#include <3ds.h>
#include <sys/stat.h>
#include <mbedtls/sha256.h>

extern "C" Handle hidHandle;

namespace Utils {
namespace Misc {
static inline void RebootToSelf() {
    nsInit();
    Result ret = NS_RebootToTitle(MEDIATYPE_SD, 0x0004000000DF1000, 0);

    if (R_FAILED(ret))
        *(u32*)ret = 0x123; // Shouldn't have happened
}

static inline void Reboot() {
    nsInit();
    Result ret = NS_RebootSystem();

    if (R_FAILED(ret))
        *(u32*)ret = 0x122; // Shouldn't have happened
}

static inline bool IsReboot() {
    uint8_t *firmparams = new uint8_t[0x1000];
    Result ret = pmAppInit();

    if (R_SUCCEEDED(ret)) {
        ret = PMAPP_GetFIRMLaunchParams(firmparams, 0x1000);

        if (R_SUCCEEDED(ret)) {
            u64 tid = 0x0004000000DF1000;
            return memcmp(firmparams + 0x440, &tid, sizeof(tid)) == 0;
        }
    }

    pmAppExit();
    delete[] firmparams;
    return false;
}

static inline bool CheckRehid() {
    /* The below is a special IPC command which is only implemented in rehid */
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0x19, 0, 0);

    if (R_SUCCEEDED(svcSendSyncRequest(hidHandle))) {
        Result ret = cmdbuf[1];
        return ret != 0xD900182F;
    }

    return false;
}

static inline bool SetRehidState(bool newstate) {
    if (newstate == false) { // disable rehid
        if (rename("/luma/sysmodules/0004013000001D02.cxi", "/luma/sysmodules/rehid.cxi") != 0)
            return false;

        if (rename("/luma/sysmodules/0004013000003302.ips", "/luma/sysmodules/rehid_ir.ips") != 0)
            return false;
    } else {
        if (rename("/luma/sysmodules/rehid.cxi", "/luma/sysmodules/0004013000001D02.cxi") != 0)
            return false;

        if (rename("/luma/sysmodules/rehid_ir.ips", "/luma/sysmodules/0004013000003302.ips") != 0)
            return false;
    }

    return true;
}

static inline void GenerateRemapping(u64 tid, bool global, const std::string &data) {
    std::string loc = "/rehid/";
    mkdir(loc.c_str(), 0777);
    std::string fileloc;
    char hex[20];

    if (global)
        fileloc = loc.substr(0, loc.size() - 1);
    else {
        sprintf(hex, "%016llX", tid);
        fileloc = loc + hex;
        mkdir(fileloc.c_str(), 0777);
    }

    fileloc = fileloc + "/rehid.json";
    FILE *f = fopen(fileloc.c_str(), "wb+");
    fwrite(data.c_str(), 1, data.size(), f);
    fclose(f);
}

static inline void EnableGamePatching() {
    std::string data;
    FILE *f = fopen("/luma/config.ini", "r+");

    if (f) {
        fseek(f, 0L, SEEK_END);
        size_t size = ftell(f);
        data.resize(size);

        fseek(f, 0L, SEEK_SET);
        fread(&data[0], size, 1, f);

        const std::string s = "enable_external_firm_and_modules = 1";

        auto found = data.find("enable_external_firm_and_modules");

        if (found != std::string::npos) {
            data.replace(found, s.length(), s);
        }

        fseek(f, 0L, SEEK_SET);
        fwrite(&data[0], data.length(), 1, f);
        fclose(f);
    }
}

bool VerifySHA256(const std::string &file, const std::string &sha256) {
    const size_t offset = sha256.compare(0, 7, "sha256:") == 0 ? 7 : 0;
    if (sha256.size() != offset + 64)
        return false;

    for (size_t i = offset; i < sha256.size(); ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(sha256[i])))
            return false;
    }

    FILE *input = fopen(file.c_str(), "rb");
    if (!input)
        return false;

    mbedtls_sha256_context context;
    mbedtls_sha256_init(&context);
    int result = mbedtls_sha256_starts_ret(&context, 0);
    unsigned char buffer[4096];
    size_t bytesRead;
    while (result == 0 && (bytesRead = fread(buffer, 1, sizeof(buffer), input)) > 0)
        result = mbedtls_sha256_update_ret(&context, buffer, bytesRead);

    const bool readError = ferror(input) != 0;
    const int closeResult = fclose(input);
    unsigned char digest[32];
    if (result == 0 && !readError && closeResult == 0)
        result = mbedtls_sha256_finish_ret(&context, digest);
    mbedtls_sha256_free(&context);

    if (result != 0 || readError || closeResult != 0)
        return false;

    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(digest); ++i) {
        if (std::tolower(static_cast<unsigned char>(sha256[offset + 2 * i])) != hex[digest[i] >> 4] ||
            std::tolower(static_cast<unsigned char>(sha256[offset + 2 * i + 1])) != hex[digest[i] & 0x0f])
            return false;
    }
    return true;
}
}
}
