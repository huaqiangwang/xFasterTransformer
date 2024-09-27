//
// Created by huaqiang on 24-9-6.
//
#include <iostream>
#include <vector>
#include <memory>
#include "benchbase.h"
#include "vexp.h"
#include "util.h"
#include "perf.h"

void print_components(std::vector<std::shared_ptr<BenchBase>> benches){
    std::cout << std::endl << "Component Benchmarks List" << std::endl;
    std::cout << "Bench\tEnabled" << std::endl;
    std::cout << "--------------------------" << std::endl;
    for (auto& bench : benches ){
        std::cout << bench->getname() << "\t";
        if (bench->bEnabled())
            std::cout << "enabled" << std::endl;
        else
            std::cout << "disabled" << std::endl;
    }
    std::cout << std::endl;
}

void registerBenchmarks(std::vector<std::shared_ptr<BenchBase>> & benches){
    std::shared_ptr<BenchVexp> vexp = std::make_shared<BenchVexp>();
    std::shared_ptr<BenchBase> bench = std::static_pointer_cast<BenchBase>(vexp);
    bench->enable();
    benches.push_back(bench);
}

void runBenchmarks(std::vector<std::shared_ptr<BenchBase>> &benches){
    for (auto &bench:benches){
        if (bench->bEnabled()){
            bench->bench();
        }
    }
}

int main(int argc, char*argv[]){
    std::vector<std::shared_ptr<BenchBase>> benchTests;\
    PerfEventResult result;
    registerBenchmarks(benchTests);

    std::cout << "xFT Component Micro Bench" << std::endl;
    print_components(benchTests);

    //uint64_t  ip = get_ip_after_this();
    //std::cout << "Current IP is 0x" <<std::hex << ip << std::dec << std::endl;
    //disassemble(ip, 1024);
    PerfMon perfmon;
    std::vector<std::string> events;
    events.emplace_back("r81d0");
    perfmon.Enable(events);
    perfmon.Start();
    runBenchmarks(benchTests);
    perfmon.GetCounters(&result);
    std::cout << "1:" << result.value[0]<< std::endl;

    runBenchmarks(benchTests);
    perfmon.GetCounters(&result);
    std::cout << "2:" << result.value[0]<< std::endl;

    perfmon.Stop();
    runBenchmarks(benchTests);
    perfmon.GetCounters(&result);
    std::cout << "3:" << result.value[0]<< std::endl;

    return 1;
}
