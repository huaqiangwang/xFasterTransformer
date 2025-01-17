// Copyright (c) 2023 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ============================================================================
#pragma once
#include <cmath>
#include <cstring>
#include <iostream>

/*  Sample:
        int bs = 2 headnum = 3 seq = 4  dim = 6;
        int max_len = 10;
        int pos_ids[4] = {2,0,1,3}; //  seq = 4 , Each batch have same value
        int pos_shape[2] = {bs, seq};
        float x[144] = {0, 1, 1,...}; // bs * h * seq * dim = 144
        int xshape[4] = {bs,headnum,seq,dim};
        Forward
        LlamaRotaryEmbedding emb(dim, seq);
        float *embd = emb.forward(x, x_shape, pos_ids, pos_shape);
*/

class ChatGLM2RotaryEmbedding {
public:
    ChatGLM2RotaryEmbedding(const int dim, const int max_position_embeddings = 32768, const float base = 10000.0);

    ~ChatGLM2RotaryEmbedding() {}

    void forward(float *buf, int bufStride, int batch_size, int seq_len, int qk_size,
            int hidden_size_per_attention_head, const int *position_ids);

private:
    void glm2CalEmb();

private:
    static bool initialized;
};
