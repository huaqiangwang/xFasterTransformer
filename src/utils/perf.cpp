//
// Created by david on 9/25/24.
//
#include "perf.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <memory.h>
#include <cerrno>
#include <dirent.h>
#include <filesystem>
#include <cstring>

uint64_t PerfMon::getStamp() {
    auto now = std::chrono::high_resolution_clock::now();
    // TODO: Try to verify if time_since_epoch() is an action need to run later.
    auto timestamp = now.time_since_epoch().count();
    return timestamp;
}

/* return the group id, 0 for error
 TODO: Currently only support RAW_EVENT, need to add support for other types of events
 */
bool PerfMon::Enable(std::vector<std::string>& events) {
    struct perf_event_attr pe = {};
    unsigned int type;
    unsigned long config;
    long fd = 0;

    grpfd_ = -1;
    for (auto event: events) {
        // Raw CPU event(type 4), starting with 'r'
        if (!event.empty() && event.front() == 'r') {
            config = std::stoul(event.substr(1), nullptr, 16);
            type = PERF_TYPE_RAW;

            // TODO: to be clear for how to set (1) sample mode, (2) count mode
            memset(&pe, 0, sizeof(struct perf_event_attr));
            pe.size = sizeof(struct perf_event_attr);
            pe.type = type;
            pe.config = config;
            pe.disabled = 1;
            pe.exclude_kernel = 1;
            pe.exclude_user = 0;
            pe.exclude_hv = 1;
            pe.read_format = (PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING);
            fd = syscall(__NR_perf_event_open, &pe, 0, -1, grpfd_, 0);
            if (fd == -1) {
                std::cerr << "Error opening leader for event " << event << std::endl;
                //how to get reason for error?
                std::cerr << strerror(errno) << std::endl;
                return false;
            }
            fds_.push_back(fd);
            if (grpfd_ == -1) grpfd_ = fd;
        } else if (!event.empty() && !event.compare(0, 11, "uncore_imc_")) {
            // use perf stat **--vv** -e <> to get real configuration of any kind of pmu
            //
            if (pmulist_.find("uncore_imc_") == pmulist_.end()) {
                std::cerr << "Unsupported PMU type: " << event << std::endl;
                return false;
            }
            // (?) Need to put all uncore_imc_0, uncore_imc_1 in one perf group?
        }else{
            std::cerr << "Unsupported event type: " << event << std::endl;
        }
    }
    if (grpfd_ != -1)
        ioctl(grpfd_, PERF_EVENT_IOC_RESET, 0);
    return true;
}

void PerfMon::Disable() {
    for (auto fd : fds_) {
        if (fd != 0)
            close(fd);
    }
    grpfd_ = -1;
    fds_.clear();
}

int PerfMon::Start() const {
    auto ret = ioctl(grpfd_, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    if (ret  == -1)
        std::cerr << "PERF_EVENT_IOC_ENABLE failed, error: " << ret << std::endl;
    return ret;
}

void PerfMon::Stop() const {
    auto ret = ioctl(grpfd_, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    if (ret  == -1)
        std::cerr << "PERF_EVENT_IOC_DISABLE failed, error: " << ret << std::endl;
}

bool PerfMon::GetCounters(PerfEventResult *result) {
    auto ret = read(grpfd_, result, sizeof(uint64_t )*(3+fds_.size()));
    if (ret == 0) {
        std::cerr << "Error reading counters, error: " << ret << std::endl;
        return false;
    }
    return true;
}

void PerfMon::getPMUList() {
    const std::string pmuDeivceFolder = "/sys/bus/event_source/devices/";
    DIR* dir = opendir(pmuDeivceFolder.c_str());
    if (dir == nullptr) {
        std::cerr << "Error opening directory: " << pmuDeivceFolder << std::endl;
        return;
    }
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_type == DT_DIR) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            // get the key
            size_t end = strlen(ent->d_name);
            while(end > 0 && std::isdigit(ent->d_name[end-1])) {
                end--;
            }
            std::string key = std::string(ent->d_name, end);
            if (pmulist_.find(key) == pmulist_.end()) {
                pmulist_[key] = {{"event", {}}, {"cpumask", {}}};
            }
            pmulist_[key]["event"].push_back(ent->d_name);
            if(pmulist_[key]["cpumask"].empty()){
                std::string cpumaskfile = pmuDeivceFolder + ent->d_name + "/cpumask";
                if(std::filesystem::exists(cpumaskfile)){
                    std::ifstream maskfile(cpumaskfile);
                    std::string cpumask;
                    const char * delimiter = ",";
                    maskfile >> cpumask;
                    char *cstr = new char[cpumask.length()+1];
                    std::strcpy(cstr, cpumask.c_str());

                    char* token = std::strtok(cstr, delimiter);
                    while (token != nullptr) {
                        pmulist_[key]["cpumask"].push_back(token);
                        token = std::strtok(nullptr, delimiter);
                    }
                    delete []cstr;
                }
            }
            std::cout << ent->d_name << std::endl;
        }
    }
}

