//
// Created by huaqiang on 24-9-7.
//

#ifndef DEPENDENCY_BENCHBASE_H
#define DEPENDENCY_BENCHBASE_H

#include <string>
class BenchBase {
public:
    BenchBase() = delete;
    BenchBase(std::string name):
        name_(name), enabled_(false){};
    BenchBase(const BenchBase &) = delete;
    BenchBase& operator=(const BenchBase &) = delete;

    void enable(){enabled_ = true;}
    void disable() {enabled_ = false;}
    virtual void bench() = 0;
    virtual void describe() = 0;
    virtual void prepare_input() = 0;
    //virtual void getresult() = 0;
    std::string& getname(){return name_;}
    bool bEnabled(){return enabled_;}
private:
    void bench_xft();
    std::string name_;
    bool enabled_;
};

#endif //DEPENDENCY_BENCHBASE_H

