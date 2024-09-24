//
// Created by huaqiang on 24-9-7.
//
#include <iostream>
#include <vector>
#include <immintrin.h>
#include <cstdlib>

#include <unistd.h>
#include <thread>
#include "benchbase.h"
#include "bert_util.h"
#include "util.h"

class BenchVexp: public BenchBase {
public:
    BenchVexp(): BenchBase("vexp"), bufferSize_(0) {
        loopRuning_ = 1000;
        loopWarmup_ = 10;
        maincpu_ = 1;
        benchcpu_ = 2;
    }
    ~BenchVexp() {
        if (bufferSize_ > 0) {
            free(buffer_);
        }
    }

    void describe() {
        std::cout << "avx512 version exp() perf bench"
        << std::endl;
    }

    void prepare_input() {
        if (bufferSize_ == 0) {
            // each MLP layer, the tensor shape for GELU/RELU operation is [seq-len, batch, ~hidden-size*4]
#define HIDDEN_SIZE 4096
#define SEQ_LEN 64
#define BATCH_SIZE 1
#define MLP_BUF_SIZE (HIDDEN_SIZE*SEQ_LEN*BATCH_SIZE*4)
            bufferSize_ = MLP_BUF_SIZE;
            buffer_ = std::aligned_alloc(64, bufferSize_ * 2);
        }
        auto pIntBuf = static_cast<int*>(buffer_);
        for(auto i = 0;i<bufferSize_/sizeof(int);i++)
            *(pIntBuf+i) = std::rand();
    }

    void bench(){
        prepare_input();
        // How to get address of static function
        auto vexp_ip = reinterpret_cast<uint64_t>(BertUtil::vexp);
        disassemble(vexp_ip, 256);

        pin_cpu(0, maincpu_);
        for(auto l=0;l<loopWarmup_;l++)
            bench_xft_kernel();

        auto worker = std::thread(&BenchVexp::bench_xft, this);
        worker.join();
    }

    void bench_xft() {
        pin_cpu(0, benchcpu_);
        for (auto l=0;l<loopRuning_;l++)
            bench_xft_kernel();
    }


private:
    void* buffer_;
    size_t bufferSize_;
    int loopWarmup_;
    int loopRuning_;
    pid_t maincpu_;
    pid_t benchcpu_;

    // TODO: report performance based result
    void bench_xft_kernel() {
        __m512 m512buf;
        __m512 m512vexpresult;

        auto loop = bufferSize_ / 64;
        for (auto i = 0;i<loop;i++){
            void *pbuf = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(buffer_) + i * 64);
            void *pbufoutput = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(buffer_)+bufferSize_/2+64*i);
            m512buf = _mm512_load_ps(pbuf);
            m512vexpresult = BertUtil::vexp(m512buf);
            _mm512_storeu_ps(pbufoutput, m512vexpresult);
        }
    }
};
