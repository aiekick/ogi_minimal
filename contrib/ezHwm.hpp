#pragma once

/*
MIT License

Copyright (c) 2014-2026 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// ezHwm is part of the ezLibs project : https://github.com/aiekick/ezLibs.git

#include <cstdint>
#include <memory>
#include <chrono>

#if defined(_WIN32)
    #include <windows.h>
    #include <pdh.h>
    #include <pdhmsg.h>
    #include <string>
    #include <vector>
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <fstream>
    #include <string>
    #include <map>
    #include <cstdlib>
#endif

namespace ez {
namespace hwm {

// forward declaration; the full definition is platform-specific and appears
// further down in this same header (header-only build cannot hide it in a .cpp)
struct PlatformState;

class Sampler {
public:
    typedef std::shared_ptr<Sampler> SamplerPtr;
    typedef std::weak_ptr<Sampler> SamplerWeak;
public:
    static SamplerPtr create();
private:
    std::unique_ptr<PlatformState> m_platformState{};
    std::chrono::steady_clock::time_point m_lastSampleTime{};
    double m_lastCpuUsagePercent{};
    double m_lastGpuUsagePercent{};
    uint32_t m_minSampleIntervalMs{500}; // 0 = recompute on every sample() call
    bool m_gpuMeasureAvailable{};
    bool m_initialized{};
public:
    bool init();
    void unit();
    ~Sampler();
    void sample();
    double getCpuUsagePercent() const;
    double getGpuUsagePercent() const;
    bool isGpuMeasureAvailable() const;
    void setMinSampleIntervalMs(uint32_t a_interval_ms);
protected:
    Sampler() = default;
private:
    bool refreshCpuUsage(double a_elapsed_seconds, double& ao_cpu_usage_percent);
    bool refreshGpuUsage(double a_elapsed_seconds, double& ao_gpu_usage_percent);
};

namespace detail {
inline double clampPercent(double a_value) {
    if (a_value < 0.0) {
        return 0.0;
    }
    if (a_value > 100.0) {
        return 100.0;
    }
    return a_value;
}
}

#if defined(_WIN32)

struct PlatformState {
    PDH_HQUERY pdhQueryHandle{nullptr};
    PDH_HCOUNTER pdhGpuUtilizationCounter{nullptr};
    uint64_t previousCpuTime100Nanoseconds{};
    uint32_t processorCount{1};
    ~PlatformState() {
        if (pdhQueryHandle != nullptr) {
            PdhCloseQuery(pdhQueryHandle);
            pdhQueryHandle = nullptr;
        }
    }
};

namespace detail {
inline uint64_t fileTimeToUint64(const FILETIME& a_file_time) {
    ULARGE_INTEGER converter{};
    converter.LowPart = a_file_time.dwLowDateTime;
    converter.HighPart = a_file_time.dwHighDateTime;
    return converter.QuadPart;
}
inline bool initGpuCounter(PlatformState& ao_platform_state) {
    if (PdhOpenQueryW(nullptr, 0, &ao_platform_state.pdhQueryHandle) != ERROR_SUCCESS) {
        ao_platform_state.pdhQueryHandle = nullptr;
        return false;
    }
    const DWORD currentProcessId = GetCurrentProcessId();
    // trailing underscore avoids matching pid_1234 against pid_12345
    const std::wstring counterPath = L"\\GPU Engine(pid_" + std::to_wstring(currentProcessId) + L"_*)\\Utilization Percentage";
    if (PdhAddEnglishCounterW(ao_platform_state.pdhQueryHandle, counterPath.c_str(), 0, &ao_platform_state.pdhGpuUtilizationCounter) != ERROR_SUCCESS) {
        PdhCloseQuery(ao_platform_state.pdhQueryHandle);
        ao_platform_state.pdhQueryHandle = nullptr;
        ao_platform_state.pdhGpuUtilizationCounter = nullptr;
        return false;
    }
    // prime so the first sample() has a previous collect to diff against
    PdhCollectQueryData(ao_platform_state.pdhQueryHandle);
    return true;
}
}

inline bool Sampler::init() {
    if (m_initialized) {
        return true;
    }
    m_platformState = std::unique_ptr<PlatformState>(new PlatformState());
    SYSTEM_INFO systemInformation{};
    GetSystemInfo(&systemInformation);
    m_platformState->processorCount = (systemInformation.dwNumberOfProcessors > 0) ? systemInformation.dwNumberOfProcessors : 1;
    FILETIME creationFileTime{};
    FILETIME exitFileTime{};
    FILETIME kernelFileTime{};
    FILETIME userFileTime{};
    if (GetProcessTimes(GetCurrentProcess(), &creationFileTime, &exitFileTime, &kernelFileTime, &userFileTime) != 0) {
        m_platformState->previousCpuTime100Nanoseconds = detail::fileTimeToUint64(kernelFileTime) + detail::fileTimeToUint64(userFileTime);
    }
    m_gpuMeasureAvailable = detail::initGpuCounter(*m_platformState);
    m_lastSampleTime = std::chrono::steady_clock::now();
    m_initialized = true;
    return true;
}

inline bool Sampler::refreshCpuUsage(double a_elapsed_seconds, double& ao_cpu_usage_percent) {
    FILETIME creationFileTime{};
    FILETIME exitFileTime{};
    FILETIME kernelFileTime{};
    FILETIME userFileTime{};
    if (GetProcessTimes(GetCurrentProcess(), &creationFileTime, &exitFileTime, &kernelFileTime, &userFileTime) == 0) {
        return false;
    }
    const uint64_t currentCpuTime100Nanoseconds = detail::fileTimeToUint64(kernelFileTime) + detail::fileTimeToUint64(userFileTime);
    uint64_t deltaCpuTime100Nanoseconds{};
    if (currentCpuTime100Nanoseconds > m_platformState->previousCpuTime100Nanoseconds) {
        deltaCpuTime100Nanoseconds = currentCpuTime100Nanoseconds - m_platformState->previousCpuTime100Nanoseconds;
    }
    m_platformState->previousCpuTime100Nanoseconds = currentCpuTime100Nanoseconds;
    const double consumedCpuSeconds = static_cast<double>(deltaCpuTime100Nanoseconds) / 10000000.0;
    const double normalizedPercent = (consumedCpuSeconds / a_elapsed_seconds) * 100.0 / static_cast<double>(m_platformState->processorCount);
    ao_cpu_usage_percent = detail::clampPercent(normalizedPercent);
    return true;
}

inline bool Sampler::refreshGpuUsage(double a_elapsed_seconds, double& ao_gpu_usage_percent) {
    (void)a_elapsed_seconds; // PDH derives the rate from the interval between collects
    if (m_platformState->pdhQueryHandle == nullptr) {
        return false;
    }
    if (PdhCollectQueryData(m_platformState->pdhQueryHandle) != ERROR_SUCCESS) {
        return false;
    }
    DWORD bufferByteSize{};
    DWORD itemCount{};
    const PDH_STATUS sizingStatus = PdhGetFormattedCounterArrayW(m_platformState->pdhGpuUtilizationCounter, PDH_FMT_DOUBLE, &bufferByteSize, &itemCount, nullptr);
    if (sizingStatus != PDH_MORE_DATA) {
        // no matching instance this interval (process idle on GPU): report zero, not failure
        ao_gpu_usage_percent = 0.0;
        return true;
    }
    std::vector<uint8_t> counterArrayBuffer(bufferByteSize);
    PDH_FMT_COUNTERVALUE_ITEM_W* counterArrayItems = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(counterArrayBuffer.data());
    if (PdhGetFormattedCounterArrayW(m_platformState->pdhGpuUtilizationCounter, PDH_FMT_DOUBLE, &bufferByteSize, &itemCount, counterArrayItems) != ERROR_SUCCESS) {
        return false;
    }
    double summedUtilizationPercent{};
    for (DWORD itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
        if (counterArrayItems[itemIndex].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            summedUtilizationPercent += counterArrayItems[itemIndex].FmtValue.doubleValue;
        }
    }
    ao_gpu_usage_percent = detail::clampPercent(summedUtilizationPercent);
    return true;
}

#elif defined(__linux__)

struct PlatformState {
    uint64_t previousCpuTimeMicroseconds{};
    std::map<std::string, uint64_t> previousEngineBusyNanoseconds{};
    long processorCount{1};
};

namespace detail {
// const namespace-scope: internal linkage, safe across translation units without inline (C++11)
const std::string kFdInfoDirectoryPath{"/proc/self/fdinfo"};
const std::string kDrmClientIdPrefix{"drm-client-id:"};
const std::string kDrmEnginePrefix{"drm-engine-"};

inline bool readProcessEngineBusyTotals(std::map<std::string, uint64_t>& ao_engine_busy_nanoseconds) {
    ao_engine_busy_nanoseconds.clear();
    DIR* directoryHandle = opendir(kFdInfoDirectoryPath.c_str());
    if (directoryHandle == nullptr) {
        return false;
    }
    std::map<uint64_t, std::map<std::string, uint64_t>> perClientEngineBusy{};
    struct dirent* directoryEntry{};
    while ((directoryEntry = readdir(directoryHandle)) != nullptr) {
        const std::string entryName{directoryEntry->d_name};
        if (entryName == "." || entryName == "..") {
            continue;
        }
        std::ifstream fdInfoStream{kFdInfoDirectoryPath + "/" + entryName};
        if (!fdInfoStream.is_open()) {
            continue;
        }
        bool clientIdFound{false};
        uint64_t clientId{};
        std::map<std::string, uint64_t> engineBusyForThisFd{};
        std::string currentLine{};
        while (std::getline(fdInfoStream, currentLine)) {
            if (currentLine.compare(0, kDrmClientIdPrefix.size(), kDrmClientIdPrefix) == 0) {
                clientId = std::strtoull(currentLine.c_str() + kDrmClientIdPrefix.size(), nullptr, 10);
                clientIdFound = true;
            } else if (currentLine.compare(0, kDrmEnginePrefix.size(), kDrmEnginePrefix) == 0) {
                const auto colonPosition = currentLine.find(':');
                if (colonPosition == std::string::npos) {
                    continue;
                }
                const std::string engineName = currentLine.substr(kDrmEnginePrefix.size(), colonPosition - kDrmEnginePrefix.size());
                const uint64_t busyNanoseconds = std::strtoull(currentLine.c_str() + colonPosition + 1, nullptr, 10);
                engineBusyForThisFd[engineName] = busyNanoseconds;
            }
        }
        if (clientIdFound && !engineBusyForThisFd.empty()) {
            // dup'd fds share the same client id and report identical counters: keep once
            perClientEngineBusy[clientId] = engineBusyForThisFd;
        }
    }
    closedir(directoryHandle);
    for (const auto& clientEntry : perClientEngineBusy) {
        for (const auto& engineEntry : clientEntry.second) {
            ao_engine_busy_nanoseconds[engineEntry.first] += engineEntry.second;
        }
    }
    return !ao_engine_busy_nanoseconds.empty();
}
}

inline bool Sampler::init() {
    if (m_initialized) {
        return true;
    }
    m_platformState = std::unique_ptr<PlatformState>(new PlatformState());
    const long onlineProcessorCount = sysconf(_SC_NPROCESSORS_ONLN);
    m_platformState->processorCount = (onlineProcessorCount > 0) ? onlineProcessorCount : 1;
    struct rusage usageSnapshot{};
    if (getrusage(RUSAGE_SELF, &usageSnapshot) == 0) {
        m_platformState->previousCpuTimeMicroseconds =
            static_cast<uint64_t>(usageSnapshot.ru_utime.tv_sec) * 1000000ull + static_cast<uint64_t>(usageSnapshot.ru_utime.tv_usec) +
            static_cast<uint64_t>(usageSnapshot.ru_stime.tv_sec) * 1000000ull + static_cast<uint64_t>(usageSnapshot.ru_stime.tv_usec);
    }
    m_gpuMeasureAvailable = detail::readProcessEngineBusyTotals(m_platformState->previousEngineBusyNanoseconds);
    m_lastSampleTime = std::chrono::steady_clock::now();
    m_initialized = true;
    return true;
}

inline bool Sampler::refreshCpuUsage(double a_elapsed_seconds, double& ao_cpu_usage_percent) {
    struct rusage usageSnapshot{};
    if (getrusage(RUSAGE_SELF, &usageSnapshot) != 0) {
        return false;
    }
    const uint64_t currentCpuTimeMicroseconds =
        static_cast<uint64_t>(usageSnapshot.ru_utime.tv_sec) * 1000000ull + static_cast<uint64_t>(usageSnapshot.ru_utime.tv_usec) +
        static_cast<uint64_t>(usageSnapshot.ru_stime.tv_sec) * 1000000ull + static_cast<uint64_t>(usageSnapshot.ru_stime.tv_usec);
    uint64_t deltaCpuTimeMicroseconds{};
    if (currentCpuTimeMicroseconds > m_platformState->previousCpuTimeMicroseconds) {
        deltaCpuTimeMicroseconds = currentCpuTimeMicroseconds - m_platformState->previousCpuTimeMicroseconds;
    }
    m_platformState->previousCpuTimeMicroseconds = currentCpuTimeMicroseconds;
    const double consumedCpuSeconds = static_cast<double>(deltaCpuTimeMicroseconds) / 1000000.0;
    const double normalizedPercent = (consumedCpuSeconds / a_elapsed_seconds) * 100.0 / static_cast<double>(m_platformState->processorCount);
    ao_cpu_usage_percent = detail::clampPercent(normalizedPercent);
    return true;
}

inline bool Sampler::refreshGpuUsage(double a_elapsed_seconds, double& ao_gpu_usage_percent) {
    std::map<std::string, uint64_t> currentEngineBusyNanoseconds{};
    if (!detail::readProcessEngineBusyTotals(currentEngineBusyNanoseconds)) {
        return false;
    }
    const double elapsedNanoseconds = a_elapsed_seconds * 1000000000.0;
    if (elapsedNanoseconds <= 0.0) {
        return false;
    }
    double summedEnginePercent{};
    for (const auto& engineEntry : currentEngineBusyNanoseconds) {
        const auto previousIterator = m_platformState->previousEngineBusyNanoseconds.find(engineEntry.first);
        uint64_t previousBusyNanoseconds{};
        if (previousIterator != m_platformState->previousEngineBusyNanoseconds.end()) {
            previousBusyNanoseconds = previousIterator->second;
        }
        uint64_t deltaBusyNanoseconds{};
        if (engineEntry.second > previousBusyNanoseconds) {
            deltaBusyNanoseconds = engineEntry.second - previousBusyNanoseconds;
        }
        summedEnginePercent += (static_cast<double>(deltaBusyNanoseconds) / elapsedNanoseconds) * 100.0;
    }
    m_platformState->previousEngineBusyNanoseconds = currentEngineBusyNanoseconds;
    ao_gpu_usage_percent = detail::clampPercent(summedEnginePercent);
    return true;
}

#endif

// platform-independent members, defined after PlatformState is a complete type
// so the unique_ptr destructor (via unit()) and create() see the full definition

inline Sampler::SamplerPtr Sampler::create() {
    auto pRet = SamplerPtr(new Sampler());
    if (!pRet->init()) {
        pRet.reset();
    }
    return pRet;
}

inline Sampler::~Sampler() {
    unit();
}

inline void Sampler::unit() {
    if (!m_initialized) {
        return;
    }
    m_platformState.reset(); // PlatformState destructor releases the PDH query on Windows
    m_lastCpuUsagePercent = 0.0;
    m_lastGpuUsagePercent = 0.0;
    m_gpuMeasureAvailable = false;
    m_initialized = false;
}

inline void Sampler::sample() {
    if (!m_initialized) {
        return;
    }
    const auto nowTime = std::chrono::steady_clock::now();
    const auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - m_lastSampleTime).count();
    if (m_minSampleIntervalMs != 0 && elapsedMilliseconds < static_cast<int64_t>(m_minSampleIntervalMs)) {
        return;
    }
    const double elapsedSeconds = std::chrono::duration<double>(nowTime - m_lastSampleTime).count();
    if (elapsedSeconds <= 0.0) {
        return;
    }
    double cpuUsagePercent{};
    if (refreshCpuUsage(elapsedSeconds, cpuUsagePercent)) {
        m_lastCpuUsagePercent = cpuUsagePercent;
    }
    if (m_gpuMeasureAvailable) {
        double gpuUsagePercent{};
        if (refreshGpuUsage(elapsedSeconds, gpuUsagePercent)) {
            m_lastGpuUsagePercent = gpuUsagePercent;
        }
    }
    m_lastSampleTime = nowTime;
}

inline double Sampler::getCpuUsagePercent() const {
    return m_lastCpuUsagePercent;
}

inline double Sampler::getGpuUsagePercent() const {
    return m_lastGpuUsagePercent;
}

inline bool Sampler::isGpuMeasureAvailable() const {
    return m_gpuMeasureAvailable;
}

inline void Sampler::setMinSampleIntervalMs(uint32_t a_interval_ms) {
    m_minSampleIntervalMs = a_interval_ms;
}

}
}

/*

#include "ezHwm.hpp"

#include <cstdio>
#include <cstdint>
#include <chrono>
#include <thread>
#include <cmath>

namespace {
// keeps one thread busy for roughly the requested duration so the sampler has
// real process CPU time to measure; a plain sleep would read ~0% CPU
void burnCpuForMilliseconds(uint32_t a_duration_ms) {
    const auto startTime = std::chrono::steady_clock::now();
    double accumulator = 0.0;
    uint64_t iterationIndex = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() < static_cast<int64_t>(a_duration_ms)) {
        accumulator += std::sin(static_cast<double>(iterationIndex) * 0.001);
        ++iterationIndex;
    }
    volatile double observableSink = accumulator; // stop the optimizer from eliding the busy loop
    (void)observableSink;
}
}

int main() {
    ez::hwm::Sampler::SamplerPtr sampler = ez::hwm::Sampler::create();
    if (!sampler->init()) {
        std::printf("failed to init hardware sampler\n");
        return 1;
    }
    // faster than the 500 ms default so the demo prints fresh values often;
    // in a real render loop you would leave the default and just call sample() per frame
    sampler->setMinSampleIntervalMs(250);

    const bool gpuAvailable = sampler->isGpuMeasureAvailable();
    if (gpuAvailable) {
        std::printf("gpu measure: available\n");
    } else {
        std::printf("gpu measure: not available (this process has no GPU context)\n");
    }

    const uint32_t updateCount = 20;
    for (uint32_t updateIndex = 0; updateIndex < updateCount; ++updateIndex) {
        // simulate one frame: some work, then a short idle slice
        burnCpuForMilliseconds(150);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        sampler->sample(); // cheap to call every frame; throttled internally

        if (gpuAvailable) {
            std::printf("update %2u/%2u   cpu %6.2f %%   gpu %6.2f %%\n", updateIndex + 1, updateCount, sampler->getCpuUsagePercent(), sampler->getGpuUsagePercent());
        } else {
            std::printf("update %2u/%2u   cpu %6.2f %%   gpu    n/a\n", updateIndex + 1, updateCount, sampler->getCpuUsagePercent());
        }
    }

    sampler->unit(); // optional; the destructor also calls unit()
    return 0;
}

*/