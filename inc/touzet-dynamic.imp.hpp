// The MIT License (MIT)
// Copyright (c) 2022 Jonathan Stacey.
// Copyright (c) 2017 Mateusz Pawlik, Nikolaus Augsten, and Daniel Kocher.
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
#include "touzet-dynamic.hpp"

namespace ted {

    template <typename CostModel, typename TreeIndex>
    double DynamicTozuetTreeIndex<CostModel, TreeIndex>::ted(const TreeIndex& t1, const TreeIndex& t2) {

        t1_d_ = t2_d_ = 0;

        int k = std::abs(t1.tree_size_ - t2.tree_size_) + 1;

        auto start = std::chrono::high_resolution_clock::now();
        double distance = ted_k(t1, t2, k);
        auto stop = std::chrono::high_resolution_clock::now();

        while (k < distance) {
            k <<= 2;
            start = std::chrono::high_resolution_clock::now();
            distance = ted_k(t1, t2, k);
            stop = std::chrono::high_resolution_clock::now();
        }

        // we only vaguely care about the last iteration for problem set-up, this value isn't recorded anyway...
        ted_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        k_old_ = distance;
        d_old_ = distance;
        td_old_ = std::move(td_);

        return distance;
    };

    template <typename CostModel, typename TreeIndex>
    double DynamicTozuetTreeIndex<CostModel, TreeIndex>::ted(
        const TreeIndex& t1_old, const TreeIndex& t1_new,
        const std::unordered_map<size_t, size_t>& t1_preserved_nodes,
        const TreeIndex& t2_old, const TreeIndex& t2_new,
        const std::unordered_map<size_t, size_t>& t2_preserved_nodes
    ) {

        t1_preserved_subtrees.clear();
        t2_preserved_subtrees.clear();

        hit = 0;
        missed = 0;

        auto start = std::chrono::high_resolution_clock::now();

        t1_d_ = TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::ted(t1_old, t1_new); // get distances for t1
        if (t1_d_) for (auto [new_prel, old_prel] : t1_preserved_nodes) {
            auto new_postl = t1_new.prel_to_postl_[new_prel];
            auto old_postl = t1_old.prel_to_postl_[old_prel];
            if (td_.read_at(old_postl, new_postl) == 0) t1_preserved_subtrees[new_postl] = old_postl;
        }

        auto stop = std::chrono::high_resolution_clock::now();

        t1_prep_problems = subproblem_counter_;

        t1_prep_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        start = std::chrono::high_resolution_clock::now();

        t2_d_ = TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::ted(t2_old, t2_new); // get distances for t2
        if (t2_d_) for (auto [new_prel, old_prel] : t2_preserved_nodes) {
            auto new_postl = t2_new.prel_to_postl_[new_prel];
            auto old_postl = t2_old.prel_to_postl_[old_prel];
            if (td_.read_at(old_postl, new_postl) == 0) t2_preserved_subtrees[new_postl] = old_postl;
        }

        stop = std::chrono::high_resolution_clock::now();

        t2_prep_problems = subproblem_counter_;

        t2_prep_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        int k = t1_d_ + t2_d_ + d_old_;

        start = std::chrono::high_resolution_clock::now();

        double distance;
        if (t1_d_ && t2_d_) distance = dynamic_ted_k<false, false>(t1_new, t2_new, k);
        else if (t1_d_) distance = dynamic_ted_k<false, true>(t1_new, t2_new, k);
        else if (t2_d_) distance = dynamic_ted_k<true, false>(t1_new, t2_new, k);
        else distance = d_old_;

        stop = std::chrono::high_resolution_clock::now();

        ted_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        k_old_ = k;
        d_old_ = distance;

        if (t1_d_ || t2_d_) td_old_ = std::move(td_);

        return distance;
    };

    template <typename CostModel, typename TreeIndex>
    double DynamicTozuetTreeIndex<CostModel, TreeIndex>::ted(
        const TreeIndex& t1_old, const TreeIndex& t1_new,
        const std::unordered_map<size_t, size_t>& t1_preserved_nodes,
        const TreeIndex& t2_old
    ) {

        t2_d_ = 0;

        t1_preserved_subtrees.clear();
        t2_preserved_subtrees.clear();

        t2_prep_problems = 0;
        t2_prep_millis = 0;

        hit = 0;
        missed = 0;

        auto start = std::chrono::high_resolution_clock::now();

        t1_d_ = TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::ted(t1_old, t1_new); // get distances for t2
        if (t1_d_) for (auto [new_prel, old_prel] : t1_preserved_nodes) {
            auto new_postl = t1_new.prel_to_postl_[new_prel];
            auto old_postl = t1_old.prel_to_postl_[old_prel];
            if (td_.read_at(old_postl, new_postl) == 0) t1_preserved_subtrees[new_postl] = old_postl;
        }

        auto stop = std::chrono::high_resolution_clock::now();

        t1_prep_problems = subproblem_counter_;

        t1_prep_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        int k = t1_d_ + t2_d_ + d_old_;

        start = std::chrono::high_resolution_clock::now();

        double distance;
        if (t1_d_)distance = dynamic_ted_k<false, true>(t1_new, t2_old, k);
        else distance = d_old_;

        stop = std::chrono::high_resolution_clock::now();

        ted_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        k_old_ = k;
        d_old_ = distance;

        if (t1_d_) td_old_ = std::move(td_);

        return distance;
    };

    template <typename CostModel, typename TreeIndex>
    double DynamicTozuetTreeIndex<CostModel, TreeIndex>::ted(
        const TreeIndex& t1_old,
        const TreeIndex& t2_old, const TreeIndex& t2_new,
        const std::unordered_map<size_t, size_t>& t2_preserved_nodes  // new_prel -> old_prel
    ) {

        t1_d_ = 0;

        t1_preserved_subtrees.clear();
        t2_preserved_subtrees.clear();

        t1_prep_problems = 0;
        t1_prep_millis = 0;

        hit = 0;
        missed = 0;

        auto start = std::chrono::high_resolution_clock::now();

        t2_d_ = TouzetDepthPruningTruncatedTreeFixTreeIndex<CostModel, TreeIndex>::ted(t2_old, t2_new); // get distances for t2
        if (t2_d_) for (auto [new_prel, old_prel] : t2_preserved_nodes) {
            auto new_postl = t2_new.prel_to_postl_[new_prel];
            auto old_postl = t2_old.prel_to_postl_[old_prel];
            if (td_.read_at(old_postl, new_postl) == 0) t2_preserved_subtrees[new_postl] = old_postl;
        }

        auto stop = std::chrono::high_resolution_clock::now();

        t2_prep_problems = subproblem_counter_;

        t2_prep_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        int k = t1_d_ + t2_d_ + d_old_;

        start = std::chrono::high_resolution_clock::now();

        double distance;
        if (t2_d_) distance = dynamic_ted_k<true, false>(t1_old, t2_new, k);
        else distance = d_old_;

        stop = std::chrono::high_resolution_clock::now();

        ted_millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        k_old_ = k;
        d_old_ = distance;

        if (t2_d_) td_old_ = std::move(td_);

        return distance;
    };

    template <typename CostModel, typename TreeIndex>
    template <bool t1_same, bool t2_same>
    double DynamicTozuetTreeIndex<CostModel, TreeIndex>::dynamic_ted_k(const TreeIndex& t1, const TreeIndex& t2, const int k) {

        const int t1_size = t1.tree_size_;
        const int t2_size = t2.tree_size_;

        init_matrices(t1_size, k);

        subproblem_counter_ = 0;

        if (std::abs(t1_size - t2_size) > k) {
            return std::numeric_limits<double>::infinity();
        }

        for (int x = 0; x < t1_size; ++x) {
            for (int y = std::max(0, x - k); y <= std::min(x + k, t2_size - 1); ++y) {

                double distance = std::numeric_limits<double>::infinity();

                if constexpr (!t1_same && !t2_same) {
                    if (t1_preserved_subtrees.contains(x) && t2_preserved_subtrees.contains(y) && std::abs(t1_preserved_subtrees[x] - t2_preserved_subtrees[y]) <= k_old_) {
                        distance = td_old_.read_at(t1_preserved_subtrees[x], t2_preserved_subtrees[y]);
                    }
                }
                else if constexpr (t1_same) {
                    if (t2_preserved_subtrees.contains(y) && std::abs(x - t2_preserved_subtrees[y]) <= k_old_) {
                        distance = td_old_.read_at(x, t2_preserved_subtrees[y]);
                    }
                }
                else if constexpr (t2_same) {
                    if (t1_preserved_subtrees.contains(x) && std::abs(t1_preserved_subtrees[x] - y) <= k_old_) {
                        distance = td_old_.read_at(t1_preserved_subtrees[x], y);
                    }
                }

                if (!std::isinf(distance)) {
                    td_.at(x, y) = distance;
                    hit++;
                }
                else if (k_relevant(t1, t2, x, y, k)) {
                    td_.at(x, y) = tree_dist(t1, t2, x, y, k, e_budget(t1, t2, x, y, k));
                    missed++;
                } // otherwise it wasn't computed orginally and still isn't needed now

            }
        }

        return td_.at(t1.tree_size_ - 1, t2.tree_size_ - 1);
    }
}