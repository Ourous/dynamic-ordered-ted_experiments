// The MIT License (MIT)
// Copyright (c) 2022 Jonathan Stacey.
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

#include "parser.hpp"
#include "string_label.h"

#include "touzet-dynamic.hpp"
#include "touzet_depth_pruning_truncated_tree_fix_tree_index.h"
#include "touzet_kr_set_tree_index.h"

#include "unit_cost_model.h"
#include "tree_indexer.h"
#include "label_dictionary.h"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

std::string content_as_string(std::string path) {
    return std::string((std::stringstream() << std::ifstream(path).rdbuf()).str());
}

std::pair<std::optional<std::string>, std::optional<std::string>> get_new_trees() {
    std::string t1_path, t2_path;
    std::getline(std::cin, t1_path);
    std::getline(std::cin, t2_path);
    return std::make_pair(
        t1_path.empty() ? std::nullopt : std::make_optional(t1_path),
        t2_path.empty() ? std::nullopt : std::make_optional(t2_path)
    );
}

int main(int argc, char* argv[]) {

    label::LabelDictionary<label::StringLabel> labels;
    cost_model::UnitCostModelLD<label::StringLabel> model(labels);

    ted::TouzetKRSetTreeIndex<cost_model::UnitCostModelLD<label::StringLabel>, node::TreeIndexAll> topdiff(model);
    ted::TouzetDepthPruningTruncatedTreeFixTreeIndex<cost_model::UnitCostModelLD<label::StringLabel>, node::TreeIndexAll> touzet(model);
    ted::DynamicTozuetTreeIndex<cost_model::UnitCostModelLD<label::StringLabel>, node::TreeIndexAll> dynamic_ted(model);

    node::TreeIndexAll t1_old, t2_old;

    // TODO: add flags to enable / disable the Offline and Static algorithms

    {
        auto [t1_path, t2_path] = get_new_trees();
        if (t1_path.has_value() && t2_path.has_value()) {

            auto start = std::chrono::high_resolution_clock::now();
            node::index_tree(t1_old, parser::parse<label::StringLabel>(content_as_string(t1_path.value())), labels, model);
            auto stop = std::chrono::high_resolution_clock::now();
            std::cerr << "Parsing + Indexing Tree 1 took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

            start = std::chrono::high_resolution_clock::now();
            node::index_tree(t2_old, parser::parse<label::StringLabel>(content_as_string(t2_path.value())), labels, model);
            stop = std::chrono::high_resolution_clock::now();
            std::cerr << "Parsing + Indexing Tree 2 took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;
        }
        else {
            std::cerr << "First two trees must be provided" << std::endl;
            return 1;
        }

        std::cout << "Instance: Distance, Subproblems (trees + forests), Time (milliseconds), Hit (tree pairs), Missed (tree pairs)" << std::endl;

        std::cout << "Baseline: " << dynamic_ted.ted(t1_old, t2_old) << " " << dynamic_ted.get_subproblem_count() << " " << dynamic_ted.ted_millis << std::endl;
    }

    while (true) {

        std::unordered_map<size_t, size_t> t1_preserved_nodes, t2_preserved_nodes;
        node::TreeIndexAll t1_new, t2_new;

        auto [t1_path, t2_path] = get_new_trees();

        if (t1_path.has_value()) {

            auto start = std::chrono::high_resolution_clock::now();

            auto [t1, t1_retained] = parser::parse<label::StringLabel>(
                content_as_string(t1_path.value()),
                [&labels, &t1_old](size_t prel) {
                    return labels.get(t1_old.prel_to_label_id_[prel]);
                }
            );
            node::index_tree(t1_new, t1, labels, model);

            auto stop = std::chrono::high_resolution_clock::now();

            std::cerr << "Parsing + Indexing Tree 1 took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

            t1_preserved_nodes = t1_retained;
        }
        else std::cerr << "Tree 1 is unchanged..." << std::endl;

        if (t2_path.has_value()) {

            auto start = std::chrono::high_resolution_clock::now();

            auto [t2, t2_retained] = parser::parse<label::StringLabel>(
                content_as_string(t2_path.value()),
                [&labels, &t2_old](size_t prel) {
                    return labels.get(t2_old.prel_to_label_id_[prel]);
                }
            );
            node::index_tree(t2_new, t2, labels, model);

            auto stop = std::chrono::high_resolution_clock::now();

            std::cerr << "Parsing + Indexing Tree 2 took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;

            t2_preserved_nodes = t2_retained;
        }
        else std::cerr << "Tree 2 is unchanged..." << std::endl;

        double distance;

        if (t1_path.has_value() && t2_path.has_value()) {

            distance = dynamic_ted.ted(t1_old, t1_new, t1_preserved_nodes, t2_old, t2_new, t2_preserved_nodes);
            t1_old = t1_new;
            t2_old = t2_new;

        }
        else if (t1_path.has_value()) {

            distance = dynamic_ted.ted(t1_old, t1_new, t1_preserved_nodes, t2_old);
            t1_old = t1_new;

        }
        else if (t2_path.has_value()) {

            distance = dynamic_ted.ted(t1_old, t2_old, t2_new, t2_preserved_nodes);
            t2_old = t2_new;

        }
        else  return 0;

        std::cout << "T1 Preprocessing: " << dynamic_ted.t1_d_ << " " << dynamic_ted.t1_prep_problems << " " << dynamic_ted.t1_prep_millis << std::endl;
        std::cout << "T2 Preprocessing: " << dynamic_ted.t2_d_ << " " << dynamic_ted.t2_prep_problems << " " << dynamic_ted.t2_prep_millis << std::endl;
        std::cout << "Dynamic Touzet: " << dynamic_ted.d_old_ << " " << dynamic_ted.get_subproblem_count() << " " << dynamic_ted.ted_millis << " " << dynamic_ted.hit << " " << dynamic_ted.missed << std::endl;
        std::cerr << "Hit " << ((double)dynamic_ted.hit / (double)(dynamic_ted.hit + dynamic_ted.missed)) * 100.0 << "% of subtree pairs" << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        distance = topdiff.ted_k(t1_old, t2_old, dynamic_ted.k_old_);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Bounded TopDiff: " << distance << " " << topdiff.get_subproblem_count() << " " << duration << std::endl; // << normal_topdiff.td_.get_band_width() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        distance = touzet.ted_k(t1_old, t2_old, dynamic_ted.k_old_);
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Bounded Touzet: " << distance << " " << touzet.get_subproblem_count() << " " << duration << std::endl; // << normal_topdiff.td_.get_band_width() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        distance = topdiff.ted(t1_old, t2_old);
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Bound-Finding TopDiff: " << distance << " " << topdiff.get_subproblem_count() << " " << duration << std::endl; // << normal_topdiff.td_.get_band_width() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        distance = touzet.ted(t1_old, t2_old);
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Bound-Finding Touzet: " << distance << " " << touzet.get_subproblem_count() << " " << duration << std::endl; // << normal_topdiff.td_.get_band_width() << std::endl;
    }

    return 0;
}