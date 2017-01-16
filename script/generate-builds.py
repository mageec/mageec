#!/bin/env python3

import argparse
import os
import random
import sys
import mageec


gcc_wrapper = 'mageec-gcc'
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
    #'-fcommon',          # affects semantics, unlikely to affect performance
    '-fcompare-elim',
    '-fconserve-stack',
    '-fcprop-registers',
    '-fcrossjumping',
    '-fcse-follow-jumps',
    #'-fdata-sections',   # affects semantics unlikely to affect performance
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
    #'-fstrict-aliasing',   # affects semantics
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


# Make generic to the type of choice which needs to be made
def generate_configs(flags, num_configs, generator):
    configs = []
    if generator == 'random':
        for i in range(0, num_configs):
            num_enabled = random.randint(0, len(flags))
            flag_seq = random.sample(flags, num_enabled)
            configs.append(' '.join(flag_seq))
    else:
        assert False, 'Unsupported configuration generator'
    return configs


def generate_configurations(src_dir, build_dir, install_dir, build_system,
                            cc, cflags, database_path, features_path,
                            num_configs, generator, debug):
    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(os.path.exists(database_path))
    assert(os.path.exists(features_path))
    assert(mageec.is_command_on_path(cc))
    assert(mageec.is_command_on_path(gcc_wrapper))
    assert(num_configs > 0)

    configs = generate_configs(gcc_flags, num_configs, generator)

    run_id = 0
    for config in configs:
        run_build_dir = os.path.join(build_dir, 'run-' + str(run_id))
        run_install_dir = os.path.join(install_dir, 'run-' + str(run_id))
        if not os.path.exists(run_build_dir):
            os.makedirs(run_build_dir)
        if not os.path.exists(run_install_dir):
            os.makedirs(run_install_dir)
        run_id += 1

        print ('-- Building configuration:\n'
               '   Configuration: \'' + config + '\'')

        compilations_path = os.path.join(run_install_dir, 'compilations.csv')

        wrapper_cflags = '-fmageec-gcc=' + cc
        if debug:
            wrapper_cflags += ' -fmageec-debug'
        wrapper_cflags += ' -fmageec-mode=gather'
        wrapper_cflags += ' -fmageec-database=' + database_path
        wrapper_cflags += ' -fmageec-features=' + features_path
        wrapper_cflags += ' -fmageec-out=' + compilations_path

        new_cflags = wrapper_cflags + ' ' + cflags + ' ' + config

        res = mageec.build(src_dir=src_dir,
                           build_dir=run_build_dir,
                           install_dir=run_install_dir,
                           build_system=build_system,
                           cc=gcc_wrapper,
                           cflags=new_cflags)
        # just ignore failed builds
        if not res:
            print ('-- Build failed. Continuing regardless')
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Generate and build multiple versions of a source project')

    # required arguments
    parser.add_argument('--src-dir', nargs=1, required=True,
        help='Directory containing the source to build')
    parser.add_argument('--build-dir', nargs=1, required=True,
        help='Build directory')
    parser.add_argument('--install-dir', nargs=1, required=True,
        help='Install directory')
    parser.add_argument('--cc', nargs=1, required=True,
        help='Command to use to compile the source')
    parser.add_argument('--database', nargs=1, required=True,
        help='mageec database to store generated compilations in')
    parser.add_argument('--features', nargs=1, required=True,
        help='File containing extracted features for the source being built')
    parser.add_argument('--num-configs', nargs=1, required=True,
        help='Number of configurations of the source to generate')
    parser.add_argument('--generator', nargs=1, required=True,
        help='Generator to use to generate configurations')

    # optional arguments
    parser.add_argument('--debug', action='store_true', required=False,
        help='Enable debug when doing feature extraction')
    parser.add_argument('--build-system', nargs=1, required=False,
        help='Build system to be used to build the source. May be \'cmake\', '
             '\'configure\', or a script to be used to build the source')
    parser.add_argument('--cflags', nargs=1, required=False,
        help='Common arguments to be used when building')
    parser.set_defaults(debug=False,
                        build_system=[None],
                        cflags=[''])

    args = parser.parse_args(sys.argv[1:])
    src_dir         = os.path.abspath(args.src_dir[0])
    build_dir       = os.path.abspath(args.build_dir[0])
    install_dir     = os.path.abspath(args.install_dir[0])
    cc              = args.cc[0]
    database_path   = os.path.abspath(args.database[0])
    features_path   = os.path.abspath(args.features[0])
    num_configs     = int(args.num_configs[0])
    generator       = args.generator[0]

    if not os.path.exists(src_dir):
        print ('-- Source directory \'' + src_dir + '\' does not exist')
        return -1
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    if not os.path.exists(install_dir):
        os.makedirs(install_dir)
    if not os.path.exists(database_path):
        print ('-- Database \'' + database_path + '\' does not exist')
        return -1
    if not os.path.exists(features_path):
        print ('-- Features file \'' + features_path + '\' does not exist')
        return -1

    if not mageec.is_command_on_path(cc):
        print ('-- Compiler \'' + cc + '\' is not on the path')
        return -1
    if not mageec.is_command_on_path(gcc_wrapper):
        print ('-- mageec gcc wrapper \'' + gcc_wrapper + '\' is not on the path')
        return -1

    if num_configs <= 0:
        print ('-- Cannot generate a negative or zero number of configurations')
        return -1

    debug        = args.debug
    build_system = args.build_system[0]
    cflags       = args.cflags[0]

    res = generate_configurations(src_dir=src_dir,
                                  build_dir=build_dir,
                                  install_dir=install_dir,
                                  build_system=build_system,
                                  cc=cc,
                                  cflags=cflags,
                                  database_path=database_path,
                                  features_path=features_path,
                                  num_configs=num_configs,
                                  generator=generator,
                                  debug=debug)
    if not res:
        return -1
    return 0


if __name__ == '__main__':
    main()
