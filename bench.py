#!/usr/bin/env python3

# The MIT License (MIT)
# Copyright (c) 2022 Jonathan Stacey.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import subprocess
import os.path
import bisect
import sys
from typing import Optional, Union, List
import csv
import itertools
import sys

# TODO: actually swap to class-based nodes with .version, .index, .label, .children


class Node:
    def __init__(self, label, version=None, children=None):
        self.label = label
        self.version = version
        self.children = children or list()
        self.index = None
        self.dirty = True

    def __str__(self):
        return (
            (("[" + str(self.index) + "]") if self.index is not None else "")
            + (("(" + self.label + ")") if self.dirty else "")
            + "{"
            + "".join(map(str, self.children))
            + "}"
        )

    def update(self, label=None, version=None):
        if label is not None:
            self.label = label
        if version is not None:
            self.version = version
        self.dirty = True

    def insert(self, label, version=None):
        node = Node(label, version=version)
        bisect.insort_left(self.children, node)
        return node

    def remove(self, label):
        self.children.pop(bisect.bisect_left(self.children, label))

    def __lt__(self, obj):
        if isinstance(obj, str):
            return self.label < obj
        elif isinstance(obj, Node):
            return self.label < obj.label
        else:
            raise NotImplementedError()

    def __gt__(self, obj):
        if isinstance(obj, str):
            return self.label > obj
        elif isinstance(obj, Node):
            return self.label > obj.label
        else:
            raise NotImplementedError()

    def __eq__(self, obj):
        if isinstance(obj, str):
            return self.label == obj
        elif isinstance(obj, Node):
            return self.label == obj.label
        else:
            raise NotImplementedError()


class Tree:
    def __init__(self):
        self.root = Node(os.path.sep)

    def getByPath(self, path):
        node = self.root
        for label in path:
            node = node.children[bisect.bisect_left(node.children, label)]
        return node

    def setIndices(self):
        index = 0
        stack = []
        stack.append(([self.root], 0))
        while len(stack) > 0:
            node_list, cur_cid = stack[-1]
            if cur_cid >= len(node_list):
                stack.pop()
                continue

            cur_node = node_list[cur_cid]
            cur_node.index = index
            cur_node.dirty = False
            index += 1
            stack[-1] = (node_list, cur_cid + 1)
            stack.append((cur_node.children, 0))

    def update(self, path, version=None):
        self.getByPath(path).update(version=version)

    def insert(self, path, version=None):
        node = self.root
        for label in path:
            would_be_at = bisect.bisect_left(node.children, label)
            if would_be_at != len(node.children) and node.children[would_be_at] == label:
                node = node.children[would_be_at]
            else:
                node = node.insert(label, version=version)
        return node

    def remove(self, path):
        self.getByPath(path[:-1]).remove(path[-1])
        for offset in range(1, len(path)):
            node = self.getByPath(path[:-offset])
            if len(node.children) == 0:
                self.getByPath(path[: -(offset + 1)]).remove(node.label)
            else:
                break

    def __str__(self):
        return str(self.root)


def get_baseline(at_id: str):
    tree = Tree()
    files = subprocess.run(
        ["git", "ls-tree", "--full-tree", "--name-only", "-r", at_id], capture_output=True, text=True
    ).stdout.splitlines()
    for path in map(lambda f: f.split(os.path.sep), files):
        tree.insert(path, version=at_id)
    return tree


def get_commits(from_id: str, to_id: str):
    return subprocess.run(
        ["git", "rev-list", "--first-parent", "--reverse", "--ancestry-path", from_id + ".." + to_id],
        capture_output=True,
        text=True,
    ).stdout.splitlines()


def get_diff(commit_id: str, parent_id: Optional[str] = None):
    return subprocess.run(
        ["git", "diff", "--name-status", parent_id or (commit_id + "~"), commit_id], capture_output=True, text=True
    ).stdout.splitlines()


def get_edits(tree: Tree, commit_id: str, parent_id: Optional[str] = None):
    only_revisions = True
    for mode, *paths in map(lambda l: l.split(), get_diff(commit_id, parent_id)):
        files = [tuple(path.split(os.path.sep)) for path in paths]
        if mode[0] == "M":
            continue
        elif mode[0] == "A":
            tree.insert(files[0], version=commit_id)
            only_revisions = False
        elif mode[0] == "D":
            tree.remove(files[0])
            only_revisions = False
        elif mode[0] == "R":
            tree.remove(files[0])
            tree.insert(files[1], version=commit_id)
            only_revisions = False
    return only_revisions


def run_test(
    data_dir: str,
    t1_ref: Union[str, List[str]],
    t2_ref: Union[str, List[str]],
    save_as: Optional[str] = None,
    drop_zeroes: bool = True,
):
    t1_is_fixed = not isinstance(t1_ref, list)
    t2_is_fixed = not isinstance(t2_ref, list)

    if t1_is_fixed and t2_is_fixed:
        t1_refs = [t1_ref]
        t2_refs = [t2_ref]
    else:
        t1_refs = list(itertools.repeat(t1_ref, len(t2_ref))) if t1_is_fixed else t1_ref
        t2_refs = list(itertools.repeat(t2_ref, len(t1_ref))) if t2_is_fixed else t2_ref

    if len(t1_refs) != len(t2_refs):
        print("ERROR: different reflist lengths", t1_is_fixed, t2_is_fixed)
        exit(1)

    old_t1, old_t2 = t1_refs[0], t2_refs[0]
    t1, t2 = get_baseline(old_t1), get_baseline(old_t2)

    ted = subprocess.Popen(sys.argv[1], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=sys.stderr)

    with open(save_as, "w") as out:
        writer = csv.writer(out)
        writer.writerow(
            [
                "Edit-Distance",
                "Dynamic-Subproblems",
                "Dynamic-Time",
                "Dynamic-Hit",
                "Dynamic-Missed",
                "Bounded-TopDiff-Subproblems",
                "Bounded-TopDiff-Time",
                "Bounded-Touzet-Subproblems",
                "Bounded-Touzet-Time",
                "Bound-Finding-TopDiff-Subproblems",  # only counts last iteration, not total
                "Bound-Finding-TopDiff-Time",
                "Bound-Finding-Touzet-Subproblems",  # only counte last iteration, not total
                "Bound-Finding-Touzet-Time",
                "Tree1-Delta",
                "Tree1-Subproblems",
                "Tree1-Time",
                "Tree2-Delta",
                "Tree2-Subproblems",
                "Tree2-Time",
            ]
        )

        # TODO: add caching here
        with open(data_dir + os.path.sep + old_t1, "w") as f1, open(data_dir + os.path.sep + old_t2, "w") as f2:
            f1.write(str(t1))
            f2.write(str(t2))

            f1.flush()
            f2.flush()

            ted.stdin.write((f1.name + "\n").encode())
            ted.stdin.write((f2.name + "\n").encode())

            ted.stdin.flush()

            print(ted.stdout.readline().decode())
            print(ted.stdout.readline().decode())

        for new_t1, new_t2 in zip(t1_refs[1:], t2_refs[1:]):
            t1.setIndices()
            t2.setIndices()

            if (
                all(
                    [
                        t1_is_fixed or get_edits(t1, new_t1, parent_id=old_t1),
                        t2_is_fixed or get_edits(t2, new_t2, parent_id=old_t2),
                    ]
                )
                and drop_zeroes
            ):
                continue

            if not t1_is_fixed:
                with open(data_dir + os.path.sep + old_t1 + "-" + new_t1, "w") as f1:
                    f1.write(str(t1))
                    ted.stdin.write(f1.name.encode())

            ted.stdin.write("\n".encode())

            if not t2_is_fixed:
                with open(data_dir + os.path.sep + old_t2 + "-" + new_t2, "w") as f2:
                    f2.write(str(t2))
                    ted.stdin.write(f2.name.encode())

            ted.stdin.write("\n".encode())

            ted.stdin.flush()
            print(new_t1, new_t2)

            output = ted.stdout.readline().decode()
            prep_t1 = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            prep_t2 = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            dynamic = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            b_topdiff = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            b_touzet = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            bf_topdiff = list(map(int, output.split(":")[1].split()))
            print(output)

            output = ted.stdout.readline().decode()
            bf_touzet = list(map(int, output.split(":")[1].split()))
            print(output)

            if dynamic[0] != bf_touzet[0]:
                # should not happen - used for sanity testing during development
                print("Distance Mismatch at:")
                print("| t1:", old_t1, "->", new_t1)
                print("| t2:", old_t2, "->", new_t2)
                print("Got", dynamic[0], "Wanted", bf_touzet[0])
                exit(1)
            writer.writerow(
                list(
                    map(
                        str,
                        dynamic + b_topdiff[1:] + b_touzet[1:] + bf_topdiff[1:] + bf_touzet[1:] + prep_t1 + prep_t2,
                    )
                )
            )
            old_t1, old_t2 = new_t1, new_t2
    ted.stdin.write("\n\n".encode())
    ted.stdin.flush()
    ted.kill()


if len(sys.argv) != 4:
    print("Usage: <ted executable path> <scratch path> <output path>")
    exit(1)

input("This script should be run from within a copy of the linux git repo. Send a newline to continue.")

data_dir = sys.argv[2]
out_dir = sys.argv[3]

has_8_rc = {0, 3, 9, 12, 16, 17, 19}

tags = [
    (
        "v5." + str(v - 1),
        [
            "v5.12-rc1-dontuse" if rc == 1 and v == 12 else "v5." + str(v) + "-rc" + str(rc)
            for rc in range(1, 9 if v in has_8_rc else 8)
        ],
        "v5." + str(v),
    )
    for v in range(1, 20)
]

for from_v, rcs, to_v in tags:
    commits = get_commits(from_v, to_v)

    # per- commit forwards (ideal, insertion heavy)
    run_test(
        data_dir,
        from_v,
        commits,
        save_as=out_dir + os.path.sep + from_v + "_to_" + to_v + "_per-commit_t1-fixed_fwd.csv",
    )

    # per- commit forwards decreasing distance (overestimate, insertion heavy)
    run_test(
        data_dir,
        commits,
        to_v,
        save_as=out_dir + os.path.sep + from_v + "_to_" + to_v + "_per-commit_t2-fixed_fwd.csv",
    )

    # per- commit backwards (ideal, deletion heavy)
    run_test(
        data_dir,
        to_v,
        list(reversed(commits)),
        save_as=out_dir + os.path.sep + from_v + "_to_" + to_v + "_per-commit_t1-fixed_rev.csv",
    )

    # per- commit backwards decreasing distance (overestimate, deletion heavy)
    run_test(
        data_dir,
        list(reversed(commits)),
        from_v,
        save_as=out_dir + os.path.sep + from_v + "_to_" + to_v + "_per-commit_t2-fixed_rev.csv",
    )
