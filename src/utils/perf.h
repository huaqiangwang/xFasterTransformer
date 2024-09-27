//
// Created by david on 9/24/24.
//
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

#include <unistd.h>


#ifndef DEPENDENCY_PERF_H
#define DEPENDENCY_PERF_H

/* -----------------------------------------------
 * Perf Interface
 */
#define MAX_PERF_EVENT_HANDLERS 20
typedef struct
{
    uint64_t nr;
    uint64_t time_enabled;
    uint64_t time_running;
    uint64_t value[MAX_PERF_EVENT_HANDLERS];
} PerfEventResult;

class PerfMon {
public:
    PerfMon(): grpfd_(-1) {
        getPMUList();
    }
    ~PerfMon(){
        Stop();
        Disable();
    }

    bool Enable(std::vector<std::string>& events);
    void Disable();
    int Start() const;
    void Stop() const;
    bool GetCounters(PerfEventResult* result);
private:
    inline uint64_t getStamp();
    void getPMUList();

    long grpfd_;
    std::vector<long> fds_;
    // { "uncore_imc_": {"event": ["uncore_imc_0", "uncore_imc_1", ...]; "cpumask": ["0"]},
    // ... }
    std::unordered_map<std::string,
            std::unordered_map<std::string,
                    std::vector<std::string>>> pmulist_;
};
#endif //DEPENDENCY_PERF_H
