//
// Created by david on 9/24/24.
//
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <map>
#include <memory>

#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

#include <unistd.h>


#ifndef DEPENDENCY_PERF_H
#define DEPENDENCY_PERF_H


struct PMUInfo {
    struct PMU{
        std::string name;
        int type;
    };
    std::vector<int> cpumask;
    std::vector<std::shared_ptr<PMU>>devs;
};

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
} GroupEventResult;
typedef struct
{
    uint64_t value;
    uint64_t time_enabled;
    uint64_t time_running;
} NonGroupEventResult;

class PerfMon {
public:
    PerfMon():grpfds_(), nongrpfds_(), pmu_(){
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
    bool GetCounters();
private:
    inline uint64_t getStamp();
    void getPMUList();
    bool getEventTypeConfig(const std::string& event, std::string& dev, uint64_t& config);

    std::vector<int> grpfds_;
    std::shared_ptr<GroupEventResult> grpresult_;

    std::vector<int> nongrpfds_;
    std::vector<std::shared_ptr<NonGroupEventResult>> nongrpresult_;
    std::unordered_map<std::string, std::shared_ptr<PMUInfo>> pmu_;
};

#endif //DEPENDENCY_PERF_H
