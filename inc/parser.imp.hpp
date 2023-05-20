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

#pragma once
#include "parser.hpp"

#include <vector>
#include <optional>

namespace parser {

    template <typename Label>
    node::Node<Label> parse(const std::string& source) {

        std::vector<std::reference_wrapper<node::Node<Label>>> stack;

        std::string::const_iterator label_begin = source.begin();
        std::optional<Label> label;

        auto it = source.begin();

        do {
            /// TODO: handoff to Label parser here
            if (*it == '(') {
                label_begin = std::next(it);
            }
            else if (*it == ')') {
                label = std::make_optional(Label(std::string(label_begin, it)));
            }
        } while (*(++it) != '{');

        node::Node<Label> root(label.value());
        stack.push_back(std::ref(root));

        while (++it != source.end()) {
            if (*it == '(') {
                label_begin = std::next(it);
                while (*(++it) != ')');
                label = std::make_optional(Label(std::string(label_begin, it)));
            }
            else if (*it == '{') {
                stack.push_back(std::ref(stack.back().get().add_child(node::Node(label.value()))));
                label.reset();
            }
            else if (*it == '}') {
                stack.pop_back();
            }
        }

        return root;
    }

    template <typename Label>
    std::pair<node::Node<Label>, std::unordered_map<size_t, size_t>> parse(const std::string& source, std::function<Label(size_t)> label_lookup) {

        std::vector<std::reference_wrapper<node::Node<Label>>> stack;
        std::unordered_map<size_t, size_t> retain;

        std::string::const_iterator slice_begin = source.begin();
        size_t new_index = 0;
        std::optional<size_t> old_index;

        std::optional<Label> label;

        auto it = source.begin();

        do {
            /// TODO: handoff to Label parser in here
            if (*it == '[') {
                slice_begin = std::next(it);
            }
            else if (*it == ']') {
                old_index = std::stoull(std::string(slice_begin, it));
            }
            else if (*it == '(') {
                slice_begin = std::next(it);
            }
            else if (*it == ')') {
                label = Label(std::string(slice_begin, it));
            }
        } while (*(++it) != '{');

        if (old_index.has_value()) {
            retain.emplace(new_index, old_index.value());
            if (!label.has_value()) {
                label = std::make_optional(label_lookup(old_index.value()));
            }
        }

        node::Node<Label> root(label.value());
        stack.push_back(std::ref(root));

        old_index.reset();
        label.reset();

        while (++it != source.end()) {
            if (*it == '[') {
                slice_begin = std::next(it);
                while (*(++it) != ']');
                old_index = std::make_optional(std::stoull(std::string(slice_begin, it)));
            }
            else if (*it == '(') {
                slice_begin = std::next(it);
                while (*(++it) != ')');
                label = std::make_optional(Label(std::string(slice_begin, it)));
            }
            else if (*it == '{') {
                new_index++;
                if (old_index.has_value()) {
                    retain.emplace(new_index, old_index.value());
                    if (!label.has_value()) {
                        label = std::make_optional(label_lookup(old_index.value()));
                    }
                }
                stack.push_back(std::ref(stack.back().get().add_child(node::Node(label.value()))));
                old_index.reset();
                label.reset();
            }
            else if (*it == '}') {
                stack.pop_back();
            }
        }

        return std::pair{ root, retain };
    }
}