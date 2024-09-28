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
#include <sys/stat.h>
#include <regex>

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
    long fd = 0;
    int cpu = -1;
    int pid = -1;

    long grpfd_ = -1;
    for (auto event: events) {
        std::string devStr;
        uint64_t config;
        if (!getEventTypeConfig(event, devStr, config)){
            std::cerr << "Unsupported event type: " << event << std::endl;
            continue;
        }

        memset(&pe, 0, sizeof(struct perf_event_attr));
        // Raw CPU event(type 4), starting with 'r'
        if (devStr == "CPU") {
            cpu = -1;
            pid = 0;
            // TODO: to be clear for how to set (1) sample mode, (2) count mode
            pe.type = PERF_TYPE_RAW;
            pe.config = config;
            pe.disabled = 1;
            pe.exclude_kernel = 1;
            pe.exclude_user = 0;
            pe.exclude_hv = 1;
            pe.read_format = (PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING);
            pe.size = sizeof(struct perf_event_attr);

            fd = syscall(__NR_perf_event_open, &pe, 0, -1, grpfd_, 0);
            if (fd == -1) {
                std::cerr << "Error opening leader for event " << event << std::endl;
                //how to get reason for error?
                std::cerr << strerror(errno) << std::endl;
                return false;
            }
            grpfds_.push_back(fd);
            if (grpfd_ == -1) {
                grpfd_ = fd;
                grpresult_ = std::make_shared<GroupEventResult>();
            }
        } else if (devStr == "uncore_imc") {
            auto imc = pmu_[devStr];
            pid = -1;
            for (auto dev : imc->devs) {
                pe.type = dev->type;
                pe.config = config;
                pe.disabled = 1;
                pe.inherit = 1;
                pe.sample_type = PERF_SAMPLE_IDENTIFIER;
                pe.read_format = (PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING);
                pe.size = sizeof(struct perf_event_attr);

                for (auto i = 0; i < imc->cpumask.size(); i++) {
                    cpu = imc->cpumask[i];
                    fd = syscall(__NR_perf_event_open, &pe, pid, cpu, -1, 0);
                    if (fd == -1) {
                        std::cerr << "Error opening leader for event " << event << std::endl;
                        //how to get reason for error?
                        std::cerr << strerror(errno) << std::endl;
                        continue;
                    }
                    nongrpfds_.push_back(fd);
                    nongrpresult_.push_back(std::make_shared<NonGroupEventResult>());
                }
            }
        }else{
            std::cerr << "Unsupported event type: " << event << std::endl;
        }
    }
    if (grpfd_ != -1)
        ioctl(grpfd_, PERF_EVENT_IOC_RESET, 0);
    return true;
}

void PerfMon::Disable() {
    for (auto fd : grpfds_) {
        if (fd != -1)
            close(fd);
        grpresult_ = nullptr;
    }
    for (auto fd: nongrpfds_){
        if (fd != -1)
            close(fd);
    }
    nongrpresult_.clear();

    grpfds_.clear();
    nongrpfds_.clear();
}

int PerfMon::Start() const {
    if (!grpfds_.empty()){
        auto ret = ioctl(grpfds_[0], PERF_EVENT_IOC_ENABLE, PERF_EVENT_IOC_ENABLE);
        if (ret == -1) std::cerr << "PERF_EVENT_IOC_ENABLE failed, error: " << ret << std::endl;
    }
    for (auto fd: nongrpfds_) {
        auto ret = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        if (ret == -1) std::cerr << "PERF_EVENT_IOC_ENABLE failed, error: " << ret << std::endl;
    }
    return 1;
}

void PerfMon::Stop() const {
    if (!grpfds_.empty()){
        auto ret = ioctl(grpfds_[0], PERF_EVENT_IOC_DISABLE, 0);
        if (ret == -1) std::cerr << "PERF_EVENT_IOC_ENABLE failed, error: " << ret << std::endl;
    }
    for (auto fd: nongrpfds_) {
        auto ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        if (ret == -1) std::cerr << "PERF_EVENT_IOC_ENABLE failed, error: " << ret << std::endl;
    }
}

bool PerfMon::GetCounters() {
    if(!grpfds_.empty()){
        auto ret = read(grpfds_[0], grpresult_.get(), sizeof(uint64_t)*(3+grpfds_.size()));
        if (ret == 0) {
            std::cerr << "Error reading counters, error: " << ret << std::endl;
        }
    }
    for (auto i = 0; i < nongrpfds_.size(); i++){
        auto fd = nongrpfds_[i];
        auto ret = read(fd, nongrpresult_[i].get(), sizeof(uint64_t)*3);
        if (ret == 0) {
            std::cerr << "Error reading counters, error: " << ret << std::endl;
        }
        std::cout << "value" << nongrpresult_[i]->value << std::endl;
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
        bool isDir = false;
        if (ent->d_type == DT_DIR)
            isDir = true;
        else if (ent->d_type == DT_LNK){
            char full_path[PATH_MAX];
            struct stat statbuf;
            snprintf(full_path, sizeof(full_path), "%s/%s", pmuDeivceFolder.c_str(), ent->d_name);
            if(stat(full_path, &statbuf)!=-1){
                if(S_ISDIR(statbuf.st_mode))
                    isDir = true;
            }
        }
        if (isDir){
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            // get the key
            size_t end = strlen(ent->d_name);
            while(end > 0 && std::isdigit(ent->d_name[end-1])) {
                end--;
            }
            if (ent->d_name[end-1] == '_') {
                end--;
            }
            std::string key = std::string(ent->d_name, end);
            if (pmu_.find(key) == pmu_.end()) {
                pmu_[key] = std::make_shared<PMUInfo>();
            }
            std::string typefile = pmuDeivceFolder + ent->d_name + "/type";
            int  type = -1;
            if (std::filesystem::exists(typefile)) {
                std::ifstream typef(typefile);
                std::string strtype;
                typef >> strtype;
                type = std::stoi(strtype);
            }
            pmu_[key]->devs.push_back(std::make_shared<PMUInfo::PMU>(PMUInfo::PMU{ent->d_name, type}));

            if(pmu_[key]->cpumask.empty()){
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
                        pmu_[key]->cpumask.push_back(std::stoi(token));
                        token = std::strtok(nullptr, delimiter);
                    }
                    delete []cstr;
                }
            }
//std::cout << ent->d_name << std::endl;
        }
    }
}

bool PerfMon::getEventTypeConfig(const std::string &event, std::string &dev, uint64_t &config) {
    // Raw CPU event(type 4), starting with 'r'
    if (!event.empty() && event.front() == 'r') {
        config = std::stoul(event.substr(1), nullptr, 16);
        dev = "CPU";
        return true;
    }
    // Next only consider 'xxx/rxxxxx/' is the valid format
    std::regex pattern("([a-zA-Z0-9_]+)\\/r([0-9a-fA-F]+)\\/");
    if (std::regex_match(event, pattern)) {
        auto match = std::sregex_iterator(event.begin(), event.end(), pattern);
        dev = match->str(1);
        config = std::stoul(match->str(2), nullptr, 16);
        return true;
    }
    return false;
}
