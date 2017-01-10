#!/bin/env python3

import argparse
import os
import random
import re
import subprocess
import sys
import mageec_common

mageec_feature_extract_plugin='libgcc_feature_extract.so'
mageec_gcc_wrapper='mageec-gcc'


gcc_flags = [
    '-faggressive-loop-optimizations',
    '-falign-functions',
    '-falign-jumps',
    '-falign-labels',
    '-falign-loops',
    '-fbranch-count-reg',
    '-fbranch-target-load-optimize',
    '-fbranch-target-load-optimize2',
    '-fbtr-bb-exclusive',
    '-fcaller-saves',
    '-fcombine-stack-adjustments',
    '-fcommon',
    '-fcompare-elim',
    '-fconserve-stack',
    '-fcprop-registers',
    '-fcrossjumping',
    '-fcse-follow-jumps',
    '-fdata-sections',
    '-fdce',
    '-fdefer-pop',
    '-fdelete-null-pointer-checks',
    '-fdevirtualize',
    '-fdse',
    '-fearly-inlining',
    '-fexpensive-optimizations',
    '-fforward-propagate',
    '-fgcse',
    '-fgcse-after-reload',
    '-fgcse-las',
    '-fgcse-lm',
    '-fgcse-sm',
    '-fguess-branch-probability',
    '-fhoist-adjacent-loads',
    '-fif-conversion',
    '-fif-conversion2',
    '-finline',
    '-finline-atomics',
    '-finline-functions',
    '-finline-functions-called-once',
    '-finline-small-functions', 
    '-fipa-cp',
    '-fipa-cp-clone',
    '-fipa-profile',
    '-fipa-pta',
    '-fipa-pure-const',
    '-fipa-reference',
    '-fipa-sra',
    '-fira-hoist-pressure',
    '-fivopts',
    '-fmerge-constants',
    '-fmodulo-sched',
    '-fmove-loop-invariants',
    '-fomit-frame-pointer',
    '-foptimize-sibling-calls',
    '-foptimize-strlen',
    '-fpeephole',
    '-fpeephole2',
    '-fpredictive-commoning',
    '-fprefetch-loop-arrays',
    '-fregmove',
    '-frename-registers',
    '-freorder-blocks',
    '-freorder-functions',
    '-frerun-cse-after-loop',
    '-freschedule-modulo-scheduled-loops',
    '-fsched-critical-path-heuristic',
    '-fsched-dep-count-heuristic',
    '-fsched-group-heuristic',
    '-fsched-interblock',
    '-fsched-last-insn-heuristic',
    '-fsched-pressure',
    '-fsched-rank-heuristic',
    '-fsched-spec',
    '-fsched-spec-insn-heuristic',
    '-fsched-spec-load',
    '-fsched-stalled-insns',
    '-fsched-stalled-insns-dep',
    '-fschedule-insns',
    '-fschedule-insns2',
    #'-fsection-anchors',   # may conflict with other flags
    '-fsel-sched-pipelining',
    '-fsel-sched-pipelining-outer-loops',
    '-fsel-sched-reschedule-pipelined',
    '-fselective-scheduling',
    '-fselective-scheduling2',
    '-fshrink-wrap',
    '-fsplit-ivs-in-unroller',
    '-fsplit-wide-types',
    '-fstrict-aliasing',
    '-fthread-jumps',
    '-ftoplevel-reorder',
    '-ftree-bit-ccp',
    '-ftree-builtin-call-dce',
    '-ftree-ccp',
    '-ftree-ch',
    #'-ftree-coalesce-inlined-vars', # there is no equivalent -fno for this flag
    '-ftree-coalesce-vars',
    '-ftree-copy-prop',
    '-ftree-copyrename',
    '-ftree-cselim',
    '-ftree-dce',
    '-ftree-dominator-opts',
    '-ftree-dse',
    '-ftree-forwprop',
    '-ftree-fre',
    '-ftree-loop-distribute-patterns',
    '-ftree-loop-distribution',
    '-ftree-loop-if-convert',
    '-ftree-loop-im',
    '-ftree-loop-ivcanon',
    '-ftree-loop-optimize',
    '-ftree-partial-pre',
    '-ftree-phiprop',
    '-ftree-pre',
    '-ftree-pta',
    '-ftree-reassoc',
    '-ftree-scev-cprop',
    '-ftree-sink',
    '-ftree-slp-vectorize',
    '-ftree-slsr',
    '-ftree-sra',
    '-ftree-switch-conversion',
    '-ftree-tail-merge',
    '-ftree-ter',
    '-ftree-vect-loop-version',
    '-ftree-vectorize',
    '-ftree-vrp',
    '-funroll-all-loops',
    '-funroll-loops',
    '-funswitch-loops',
    '-fvariable-expansion-in-unroller',
    '-fvect-cost-model',
    '-fweb'
]


def feature_extract(benchmark_src, build_dir, install_dir, build_system,
                    cc, cflags, database_path, plugin_path, features_path):
    assert(os.path.exists(benchmark_src))
    assert(os.path.exists(build_dir))
    assert(os.path.exists(install_dir))
    assert(os.path.exists(database_path))
    assert(os.path.exists(plugin_path))

    plugin_flags = '-fplugin=' + plugin_path
    plugin_flags += ' -fplugin-arg-libgcc_feature_extract-debug'
    plugin_flags += ' -fplugin-arg-libgcc_feature_extract-database=' + database_path
    plugin_flags += ' -fplugin-arg-libgcc_feature_extract-out=' + features_path

    mageec_common.build(src_dir=benchmark_src,
                        build_dir=build_dir,
                        install_dir=install_dir,
                        build_system=build_system,
                        cc=cc,
                        cflags=plugin_flags + ' ' + cflags)


def generate_configurations(flags, num_configs, generator):
    configs = []
    if generator == 'random':
        for i in range(0, num_configs):
            num_enabled = random.randint(0, len(flags))
            flag_seq = random.sample(flags, num_enabled)
            configs.append(' '.join(flag_seq))
    else:
        assert False, 'custom configuration generators not supported yet'
    return configs


def main():
    parser = argparse.ArgumentParser(
        description='Generate and build multiple versions of a benchmark')
    parser.add_argument('--database', nargs=1, required=True,
        help='Path of the mageec database')
    parser.add_argument('--benchmark', nargs=1, required=True,
        help='Path of the benchmark to build')
    parser.add_argument('--cc', nargs=1, required=True,
        help='Compiler to build the benchmark with')
    parser.add_argument('--cflags', nargs=1, required=False,
        help='Flag provided to the compiler when building the benchmarks')
    parser.add_argument('--mageec-library-path', nargs=1, required=True,
        help='Path to the mageec libraries')
    parser.add_argument('--num-configs', nargs=1, required=True,
        help='Number of configurations of the compiler to generate')
    parser.add_argument('--build-system', nargs=1, required=False,
        help='Build system to use, may be \'cmake\', \'configure\' or a custom script')
    parser.add_argument('--config-generator', nargs=1, required=False,
        help='Configuration generator to use, may be \'random\' or a custom script')

    args = parser.parse_args(sys.argv[1:])
    database_path       = os.path.abspath(args.database[0])
    benchmark_src       = os.path.abspath(args.benchmark[0])
    cc                  = args.cc[0]
    cflags = ''
    if args.cflags:
        cflags = args.cflags[0]
    mageec_library_path = os.path.abspath(args.mageec_library_path[0])
    num_configs         = int(args.num_configs[0])
    build_system = None
    if args.build_system:
        build_system = args.build_system[0]
    config_generator = 'random'
    if args.config_generator:
        args.config_generator = args.config_generator[0]

    plugin_path = os.path.join(mageec_library_path, mageec_feature_extract_plugin)

    # The structure for this script is as follows
    # base/
    #   build/
    #     feature_extract/
    #     run-X/
    #   install/
    #     run-X/
    #       compilations.csv
    #       bin/
    #         a.out
    #   features.csv
    #   compilations.csv
    base_dir = os.getcwd()
    build_dir = os.path.join(base_dir, 'build')
    install_dir = os.path.join(base_dir, 'install')
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    if not os.path.exists(install_dir):
        os.makedirs(install_dir)
    features_path = os.path.join(base_dir, 'features.csv')

    print ('-- Checking that mageec database exists')
    if not os.path.exists(database_path):
        print ('-- Could not find database ' + database_path)
        sys.exit(1)

    print ('-- Checking that benchmark source exists')
    if not os.path.exists(benchmark_src):
        print ('-- Could not find benchmark source in ' + benchmark_src)
        sys.exit(1)

    print ('-- Checking for mageec-gcc wrapper')
    if not mageec_common.is_command_on_path(mageec_gcc_wrapper):
        print ('-- mageec-gcc is not available on the PATH')
        sys.exit(1)

    print ('-- Checking for mageec feature extractor in ' + mageec_library_path)
    if not os.path.exists(plugin_path):
        print ('-- Could not find ' + mageec_feature_extract_plugin + ' in ' + mageec_library_path)
        sys.exit(1)

    print ('-- Retrieving gcc version for ' + cc)
    try:
        pipe = subprocess.Popen([cc, '--version'], stdout=subprocess.PIPE)
        version_str = pipe.communicate()[0].decode()
    except subprocess.CalledProcessError:
        print ('-- Could not retrieve gcc version for ' + cc)
        sys.exit(1)
    match = re.search('\s*([0-9]+)\.([0-9]+)\.([0-9]+)\s*', version_str)
    if match:
        gcc_major = match.group(1)
        gcc_minor = match.group(2)
        gcc_patch = match.group(3)
        gcc_version = gcc_major + '.' + gcc_minor + '.' + gcc_patch
    else:
        print ('-- Could not extract gcc version for ' + cc)
        sys.exit(1)
    print ('-- Underlying gcc version ' + gcc_version)

    # Generate flag configurations appropriate to the version of gcc
    # being targetted
    gcc_major = int(gcc_major)
    gcc_minor = int(gcc_minor)
    gcc_patch = int(gcc_patch)
    if gcc_major == 6 and gcc_minor == 3:
        flags = gcc_flags
    else:
        print ('-- Unsupported gcc version ' + gcc_version)
        sys.exit(1)
    configs = generate_configurations(flags=flags, num_configs=num_configs,
                                      generator=config_generator)

    # Do a feature extraction run to generate a set of features for all of the
    # elements of the benchmark
    feature_extract_build_dir = os.path.join(build_dir, 'feature_extract')
    feature_extract_install_dir = os.path.join(install_dir, 'feature_extract')
    if not os.path.exists(feature_extract_build_dir):
        os.makedirs(feature_extract_build_dir)
    if not os.path.exists(feature_extract_install_dir):
        os.makedirs(feature_extract_install_dir)

    feature_extract(benchmark_src=benchmark_src,
                    build_dir=feature_extract_build_dir,
                    install_dir=feature_extract_install_dir,
                    build_system=build_system,
                    cc=cc,
                    cflags=cflags,
                    database_path=database_path,
                    plugin_path=plugin_path,
                    features_path=features_path)

    # build the benchmark for every configuration
    # Here, a wrapper around gcc is used which records each compilation in the
    # database
    run_id = 0
    for config in configs:
        run_build_dir = os.path.join(build_dir, 'run-' + str(run_id))
        run_install_dir = os.path.join(install_dir, 'run-' + str(run_id))
        if not os.path.exists(run_build_dir):
            os.makedirs(run_build_dir)
        if not os.path.exists(run_install_dir):
            os.makedirs(run_install_dir)
        run_id += 1

        compilations_path = os.path.join(run_install_dir, 'compilations.csv')

        wrapper_cflags = '-fmageec-gcc=' + cc
        wrapper_cflags += ' -fmageec-debug'
        wrapper_cflags += ' -fmageec-mode=gather'
        wrapper_cflags += ' -fmageec-database=' + database_path
        wrapper_cflags += ' -fmageec-features=' + features_path
        wrapper_cflags += ' -fmageec-out=' + compilations_path
        cflags = wrapper_cflags + ' ' + cflags + ' ' + config

        mageec_common.build(src_dir=benchmark_src,
                            build_dir=run_build_dir, 
                            install_dir=run_install_dir,
                            build_system=build_system,
                            cc=mageec_gcc_wrapper,
                            cflags=cflags)


if __name__ == '__main__':
    main()
