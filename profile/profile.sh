perf record -e intel_pt//u -p $1 -- sleep 1
#perf record -e intel_pt//u --filter "tracestop xdnn_bgemm_f32bf16f32_packb._omp_fn.1 @ /usr/local/lib/python3.10/dist-packages/xfastertransformer-1.7.1-py3.10-linux-x86_64.egg/xfastertransformer/libxfastertransformer_pt.so" -p $1
