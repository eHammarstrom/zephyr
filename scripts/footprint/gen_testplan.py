#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
"""
Convert a plan.txt CSV file into a twister testplan.json.

Usage:
    ./gen_testplan.py -i plan.txt -o testplan.json

This script reads plan.txt (CSV with columns: name,feature,board,application,options,suite_name)
and generates a twister-compatible testplan.json that can be used with:
    twister --load-tests testplan.json
"""

import argparse
import csv
import json
import os
import subprocess
import sys
import tempfile
import shutil

def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert plan.txt to twister testplan.json",
        allow_abbrev=False)
    parser.add_argument('-i', '--input', required=True,
                        help="Path to plan.txt CSV file")
    parser.add_argument('-o', '--output', required=True,
                        help="Path to output testplan.json file")
    parser.add_argument('-z', '--zephyr_base', default=None,
                        help="Path to Zephyr base (default: $ZEPHYR_BASE)")
    return parser.parse_args()


def read_plan(plan_file):
    """Parse plan.txt CSV and return list of (application, board, suite_name) entries."""
    plan = []
    with open(plan_file) as f:
        csvreader = csv.reader(f)
        for row in csvreader:
            if len(row) >= 4:
                app = row[3]  # application
                board = row[2]  # board
                suite_name = row[5] if len(row) > 5 else ''  # suite_name (optional)
                plan.append({'application': app, 'board': board, 'suite_name': suite_name})
    return plan


def generate_testplan_entry(entry, zephyr_base, tmpdir):
    """Run twister -E to generate a testplan for a single plan entry."""
    app = entry['application']
    board = entry['board']
    suite_name = entry.get('suite_name', '')

    tmp_plan = os.path.join(tmpdir, f'{board.replace("/", "_")}_{os.path.basename(app)}.json')

    cmd_parts = ['source ./zephyr-env.sh && twister',
                 '-T', app,
                 '-p', board,
                 '--clobber-output',
                 '-E', tmp_plan]

    cmd = '/bin/bash -c "' + ' '.join(cmd_parts) + '"'
    print(f"  Generating testplan for: {app} on {board}" + (f" (suite: {suite_name})" if suite_name else ""))

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            cwd=zephyr_base, shell=True)
    output, _ = proc.communicate()

    if os.path.exists(tmp_plan):
        with open(tmp_plan) as f:
            data = json.load(f)
        os.remove(tmp_plan)
        testsuites = data.get('testsuites', [])

        # Filter by suite_name if specified, otherwise filter by application path
        if suite_name:
            filtered = [t for t in testsuites if t['name'] == suite_name]
            if not filtered:
                print(f"    -> WARNING: suite '{suite_name}' not found in {len(testsuites)} discovered suites")
            else:
                print(f"    -> Matched suite: {suite_name}")
            return filtered
        else:
            # Filter to only include suites from this application path
            filtered = [t for t in testsuites if t['path'] == app]
            print(f"    -> Found {len(filtered)} test suite(s) from {app}")
            return filtered

    print(f"  WARNING: Failed to generate testplan for {app} on {board}")
    if output:
        lines = output.decode().strip().split('\n')
        for line in lines[-3:]:
            print(f"    {line}")
    return []


def merge_testplans(all_testsuites, plan_entries):
    """Merge individual testplan entries into a single testplan."""
    # Collect unique values
    testsuite_roots = list(set(e['application'] for e in plan_entries))
    platforms = list(set(e['board'] for e in plan_entries))

    merged = {
        "environment": {
            "options": {
                "testsuite_root": testsuite_roots,
                "platform": platforms,
            }
        },
        "testsuites": all_testsuites
    }
    return merged


def main():
    args = parse_args()

    zephyr_base = args.zephyr_base or os.environ.get('ZEPHYR_BASE')
    if not zephyr_base:
        print("ERROR: ZEPHYR_BASE not set and --zephyr_base not provided", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(args.input):
        print(f"ERROR: Input file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    print(f"Reading plan from: {args.input}")
    plan_entries = read_plan(args.input)
    print(f"Found {len(plan_entries)} plan entries")

    all_testsuites = []
    tmpdir = tempfile.mkdtemp()

    try:
        for entry in plan_entries:
            testsuites = generate_testplan_entry(entry, zephyr_base, tmpdir)
            all_testsuites.extend(testsuites)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)

    print(f"Generated {len(all_testsuites)} total test suites")

    merged = merge_testplans(all_testsuites, plan_entries)

    with open(args.output, 'w') as f:
        json.dump(merged, f, indent=4)

    print(f"Testplan written to: {args.output}")


if __name__ == '__main__':
    main()
