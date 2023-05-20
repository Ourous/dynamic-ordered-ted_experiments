// The MIT License (MIT)
// Copyright (c) 2022 Jonathan Stacey.
// Copyright (c) 2019 Mateusz Pawlik.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include "touzet-dynamic.fwd.hpp"

#include "matrix.h"
#include "ted_algorithm_touzet.h"
#include "touzet_baseline_tree_index.h"
#include "touzet_depth_pruning_truncated_tree_fix_tree_index.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <chrono>

namespace ted {

    template <typename CostModel, typename TreeIndex>
    class DynamicTozuetTreeIndex : public TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex> {

        data_structures::BandMatrix<double> td_old_;
        std::unordered_map<int, int> t1_preserved_subtrees;
        std::unordered_map<int, int> t2_preserved_subtrees;

    public:

        using TEDAlgorithmTouzet<CostModel, TreeIndex>::td_;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::fd_;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::init_matrices;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::subproblem_counter_;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::e_budget;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::k_relevant;
        using TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::TouzetDepthPruningTruncatedTreeFixTreeIndex;
        using TEDAlgorithmTouzet<CostModel, TreeIndex>::tree_dist;

        using TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::ted_k;

        double t1_d_;
        double t2_d_;
        double d_old_;
        int k_old_;
        long long int subproblem_counter_precomp_;

        std::chrono::milliseconds::rep t1_prep_millis;
        std::chrono::milliseconds::rep t2_prep_millis;
        std::chrono::milliseconds::rep ted_millis;
        long long int t1_prep_problems;
        long long int t2_prep_problems;
        long long int hit;
        long long int missed;

        double ted(const TreeIndex& t1, const TreeIndex& t2);

        double ted(
            const TreeIndex& t1_old, const TreeIndex& t1_new,
            const std::unordered_map<size_t, size_t>& t1_preserved_nodes,
            const TreeIndex& t2_old, const TreeIndex& t2_new,
            const std::unordered_map<size_t, size_t>& t2_preserved_nodes
        );

        double ted(
            const TreeIndex& t1_old, const TreeIndex& t1_new,
            const std::unordered_map<size_t, size_t>& t1_preserved_nodes,
            const TreeIndex& t2_old
        );

        double ted(
            const TreeIndex& t1_old,
            const TreeIndex& t2_old, const TreeIndex& t2_new,
            const std::unordered_map<size_t, size_t>& t2_preserved_nodes
        );

        template<bool t1_same, bool t2_same>
        double dynamic_ted_k(const TreeIndex& t1, const TreeIndex& t2, const int k);
    };
}

#include "touzet-dynamic.imp.hpp"