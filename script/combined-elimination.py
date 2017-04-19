#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import mageec

from multiprocessing.pool import Pool

all_flags = [
    #'-faggressive-loop-optimizations', # Not supported in 4.5
    '-falign-functions',
    '-falign-jumps',
    '-falign-labels',
    '-falign-loops',
    '-fbranch-count-reg',
    '-fbranch-target-load-optimize',
    '-fbranch-target-load-optimize2',
    '-fbtr-bb-exclusive',
    '-fcaller-saves',
    #'-fcombine-stack-adjustments',     # Not supported in 4.5
    #'-fcommon',                        # affects semantics, unlikely to affect performance
    #'-fcompare-elim',                  # Not supported in 4.5
    '-fconserve-stack',
    '-fcprop-registers',
    '-fcrossjumping',
    '-fcse-follow-jumps',
    #'-fdata-sections',                 # affects semantics unlikely to affect performance
    '-fdce',
    '-fdefer-pop',
    '-fdelete-null-pointer-checks',
    #'-fdevirtualize',                  # Not supported in 4.5
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
    #'-fhoist-adjacent-loads',          # Not supported in 4.5
    '-fif-conversion',
    '-fif-conversion2',
    '-finline',
    #'-finline-atomics',                # Not supported in 4.5
    '-finline-functions',
    '-finline-functions-called-once',
    '-finline-small-functions', 
    '-fipa-cp',
    '-fipa-cp-clone',
    #'-fipa-profile',                   # Not supported in 4.5
    '-fipa-pta',
    '-fipa-pure-const',
    '-fipa-reference',
    '-fipa-sra',
    #'-fira-hoist-pressure',            # Not supported in 4.5
    '-fivopts',
    '-fmerge-constants',
    '-fmodulo-sched',
    '-fmove-loop-invariants',
    '-fomit-frame-pointer',
    '-foptimize-sibling-calls',
    #'-foptimize-strlen',               # Not supported in 4.5
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
    #'-fsection-anchors',               # may conflict with other flags
    '-fsel-sched-pipelining',
    '-fsel-sched-pipelining-outer-loops',
    '-fsel-sched-reschedule-pipelined',
    '-fselective-scheduling',
    '-fselective-scheduling2',
    #'-fshrink-wrap',                   # Not supported in 4.5
    '-fsplit-ivs-in-unroller',
    '-fsplit-wide-types',
    #'-fstrict-aliasing',               # affects semantics
    '-fthread-jumps',
    '-ftoplevel-reorder',
    #'-ftree-bit-ccp',                  # Not supported in 4.5
    '-ftree-builtin-call-dce',
    '-ftree-ccp',
    '-ftree-ch',
    #'-ftree-coalesce-inlined-vars',    # No equivalent -fno for this flag
    #'-ftree-coalesce-vars',            # Not supported in 4.5
    '-ftree-copy-prop',
    '-ftree-copyrename',
    '-ftree-cselim',
    '-ftree-dce',
    '-ftree-dominator-opts',
    '-ftree-dse',
    '-ftree-forwprop',
    '-ftree-fre',
    #'-ftree-loop-distribute-patterns', # Not supported in 4.5
    '-ftree-loop-distribution',
    #'-ftree-loop-if-convert',          # Not supported in 4.5
    '-ftree-loop-im',
    '-ftree-loop-ivcanon',
    '-ftree-loop-optimize',
    #'-ftree-partial-pre',              # Not supported in 4.5
    '-ftree-phiprop',
    '-ftree-pre',
    '-ftree-pta',
    '-ftree-reassoc',
    '-ftree-scev-cprop',
    '-ftree-sink',
    '-ftree-slp-vectorize',
    #'-ftree-slsr',                     # Not supported in 4.5
    '-ftree-sra',
    '-ftree-switch-conversion',
    #'-ftree-tail-merge',               # Not supported in 4.5
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

def build_config(all_flags, enabled_flags):
    config = []
    for f in enabled_flags:
        assert f in all_flags

    for f in all_flags:
        if f in enabled_flags:
            config.append(f)
        else:
            config.append('-fno-' + f[2:])
    return config


def build_and_measure(src_dir, build_dir, install_dir, build_system,
                      cc, cxx, fort, flags, database_path, features_path,
                      measure_script, exec_flags, debug):
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    if not os.path.exists(install_dir):
        os.makedirs(install_dir)

    # build the benchmark using the wrapper script
    # This will record the flags which the benchmark is being compiled with,
    # and associate these flags with previously extracted features through
    # a compilation id
    compilations_path = os.path.join(install_dir, 'compilations.csv')
    results_path = os.path.join(install_dir, 'results.csv')

    cc_wrapper = 'mageec-' + cc
    cxx_wrapper = 'mageec-' + cxx
    fort_wrapper = 'mageec-' + fort
    assert(mageec.is_command_on_path(cc_wrapper))
    assert(mageec.is_command_on_path(cxx_wrapper))
    assert(mageec.is_command_on_path(fort_wrapper))
    wrapper_flags = ""
    if debug:
        wrapper_flags += ' -fmageec-debug'
    wrapper_flags += ' -fmageec-mode=gather'
    wrapper_flags += ' -fmageec-database=' + database_path
    wrapper_flags += ' -fmageec-features=' + features_path
    wrapper_flags += ' -fmageec-out=' + compilations_path

    # Add the wrapper flags to the front of the flags for the build
    new_flags = wrapper_flags + ' ' + flags

    res = mageec.build(src_dir=src_dir,
                       build_dir=build_dir,
                       install_dir=install_dir,
                       build_system=build_system,
                       cc=cc_wrapper,
                       cxx=cxx_wrapper,
                       fort=fort_wrapper,
                       flags=new_flags)
    
    # Run the measurement script to produce a results file which can be
    # loaded into the database
    cmd_path = measure_script
    if not os.path.isabs(cmd_path):
        cmd_path = os.path.join(os.getcwd(), cmd_path)

    if os.path.exists(cmd_path):
        cmd_name = cmd_path
    elif mageec.is_command_on_path(measure_script):
        cmd_name = measure_script
    else:
        print ('Failed to find script to measure benchmark result')
        return 0

    cmd = [cmd_name,
           '--install-dir', install_dir,
           '--flags', exec_flags,
           '--compilation-ids', compilations_path,
           '--out', results_path]
    ret = subprocess.call(cmd)
    if ret != 0:
        return 0

    # Read in the results file, we want to extract the results for all of the
    # source files which make up the executable
    total = 0
    result_files = set()
    for line in open(results_path):
        values = line.split(',')

        # Ignore anything which isn't a 'result' line
        if len(values) != 7:
            continue
        if values[3] != 'result':
            continue
        # Only accumulate results for modules
        if values[1] != 'module':
            continue

        src_file = values[0]
        module_name = values[2]
        result = float(values[6])

        if src_file in result_files:
            print ('-- Duplicate results for module ' + module_name)
            return 0
        result_files.add(src_file)

        # accumulate the measured results for each of the modules in the
        # program. This total will be used to drive the next stage of
        # combined elimination
        #
        # FIXME: This isn't ideal, as improvements in some modules could be
        # masked by regressions in other modules. Ideally we would do combined
        # elimination on each module individually, but that would be
        # prohibitively expensive.
        total += result
    
    # FIXME: Actually adding the results to the database is currently left as an
    # exercise for the user
    return total


def combined_elimination(src_dir, build_dir, install_dir, build_system,
                         cc, cxx, fort, flags, jobs, database_path,
                         features_path, measure_script, exec_flags, debug):
    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(os.path.exists(database_path))
    assert(os.path.exists(features_path))
    assert(mageec.is_command_on_path(cc))
    assert(mageec.is_command_on_path(cxx))
    assert(mageec.is_command_on_path(fort))
    assert(jobs > 0)

    # Run at -O3 to get a point of comparison
    o3_flags = ['-O3']
    run_flags = flags + ' ' + ' '.join(o3_flags)

    o3_build_dir = os.path.join(build_dir, 'o3')
    o3_install_dir = os.path.join(install_dir, 'o3')
    o3_res = build_and_measure(src_dir=src_dir,
                               build_dir=o3_build_dir,
                               install_dir=o3_install_dir,
                               build_system=build_system,
                               cc=cc, cxx=cxx, fort=fort,
                               flags=run_flags,
                               database_path=database_path,
                               features_path=features_path,
                               measure_script=measure_script,
                               exec_flags=exec_flags,
                               debug=debug)
    if o3_res <= 0:
        print ('-- O3 build failed. Exiting')
        return False

    # Also run at -Os to get a point of comparison
    os_flags = ['-Os']
    run_flags = flags + ' ' + ' '.join(os_flags)
    os_build_dir = os.path.join(build_dir, 'os')
    os_install_dir = os.path.join(install_dir, 'os')
    os_res = build_and_measure(src_dir=src_dir,
                               build_dir=os_build_dir,
                               install_dir=os_install_dir,
                               build_system=build_system,
                               cc=cc, cxx=cxx, fort=fort,
                               flags=run_flags,
                               database_path=database_path,
                               features_path=features_path,
                               measure_script=measure_script,
                               exec_flags=exec_flags,
                               debug=debug)
    if os_res <= 0:
        print ('-- Os build failed. Exiting')
        return False

    run_id = 0
    run_metadata = []

    # Start with all flags enabled for the base line
    base_flags = list(all_flags)
    run_flags = flags + ' ' + ' '.join(base_flags)

    base_build_dir = os.path.join(build_dir, 'base-' + str(run_id))
    base_install_dir = os.path.join(install_dir, 'base-' + str(run_id))
    base_res = build_and_measure(src_dir=src_dir,
                                 build_dir=base_build_dir,
                                 install_dir=base_install_dir,
                                 build_system=build_system,
                                 cc=cc, cxx=cxx, fort=fort,
                                 flags=run_flags,
                                 database_path=database_path,
                                 features_path=features_path,
                                 measure_script=measure_script,
                                 exec_flags=exec_flags,
                                 debug=debug)
    if base_res <= 0:
        print ('-- Base build failed. Exiting')
        return False
    run_metadata.append(('base-' + str(run_id), run_flags, base_res))
    run_id += 1

    # Store the initial base run result for comparison later
    initial_base_res = base_res

    improvement = True
    flags_to_consider = list(all_flags)
    while improvement:
        improvement = False
        flags_with_improvement = []

        # identify flags that bring improvement when disabled.
        # Each build is done in a seperate process
        test_pool = Pool(jobs)
        test_results = []

        for flag in flags_to_consider:
            # do a run with each flag disabled in turn
            tmp_flags = list(base_flags)
            tmp_flags.remove(flag)

            tmp_flags = build_config(all_flags, tmp_flags)
            run_flags = flags + ' ' + ' '.join(tmp_flags)

            test_build_dir = os.path.join(build_dir, 'test-' + str(run_id))
            test_install_dir = os.path.join(install_dir, 'test-' + str(run_id))

            res = test_pool.apply_async(build_and_measure,
                                        (src_dir,
                                         test_build_dir,
                                         test_install_dir,
                                         build_system,
                                         cc, cxx, fort,
                                         run_flags,
                                         database_path,
                                         features_path,
                                         measure_script,
                                         exec_flags,
                                         debug))
            test_results.append((run_id, run_flags, flag, res))
            run_id += 1

        # Wait for all of the test runs to complete, and then get their results
        for test_run_id, run_flags, flag, res in test_results:
            res.wait()
        for test_run_id, run_flags, flag, res in test_results:
            test_res = res.get()
            run_metadata.append(('test-' + str(test_run_id), run_flags, test_res))
            if test_res <= 0:
                print ('-- Test run ' + str(test_run_id) + ' failed. Exiting')
                return False
            if test_res < base_res:
                flags_with_improvement += [(flag, test_res)]

        # Starting from the biggest improvement, check if disabling the
        # flag still brings improvement. If it does, then permanently
        # disable the flag, update the base flag configuration and remeasure
        flags_with_improvement = sorted(flags_with_improvement, key=lambda x: x[1])
        for flag, result in flags_with_improvement:
            # do a run with each flag disabled in turn
            tmp_flags = list(base_flags)
            tmp_flags.remove(flag)
            tmp_flags = build_config(all_flags, tmp_flags)
            run_flags = flags + ' ' + ' '.join(tmp_flags)

            test_build_dir = os.path.join(build_dir, 'test-' + str(run_id))
            test_install_dir = os.path.join(install_dir, 'test-' + str(run_id))
            test_res = build_and_measure(src_dir=src_dir,
                                         build_dir=test_build_dir,
                                         install_dir=test_install_dir,
                                         build_system=build_system,
                                         cc=cc, cxx=cxx, fort=fort,
                                         flags=run_flags,
                                         database_path=database_path,
                                         features_path=features_path,
                                         measure_script=measure_script,
                                         exec_flags=exec_flags,
                                         debug=debug)
            if test_res <= 0:
                print ('-- Test run ' + str(run_id) + ' failed. Exiting')
                return False
            run_metadata.append(('test-' + str(run_id), run_flags, test_res))
            run_id += 1

            if test_res < base_res:
                # remove the flag permanently from the baseline build, and
                # remove it from further consideration
                base_flags.remove(flag)
                flags_to_consider.remove(flag)
                
                # build and measure the new baseline
                tmp_flags = build_config(all_flags, base_flags)
                run_flags = flags + ' ' + ' '.join(tmp_flags)

                base_build_dir = os.path.join(build_dir, 'base-' + str(run_id))
                base_install_dir = os.path.join(install_dir, 'base-' + str(run_id))
                base_res = build_and_measure(src_dir=src_dir,
                                             build_dir=base_build_dir,
                                             install_dir=base_install_dir,
                                             build_system=build_system,
                                             cc=cc, cxx=cxx, fort=fort,
                                             flags=run_flags,
                                             database_path=database_path,
                                             features_path=features_path,
                                             measure_script=measure_script,
                                             exec_flags=exec_flags,
                                             debug=debug)
                if base_res <= 0:
                    print ('-- Base run ' + str(run_id) + ' failed. Exiting')
                    return False
                run_metadata.append(('base-' + str(run_id), run_flags, base_res))
                run_id += 1
                improvement = True

    # Dump statistics now that we are no longer seeing any improvement
    for run in run_metadata:
        name = run[0]
        flags = run[1]
        res = run[2]

        base_improvement = float(initial_base_res) / res
        o3_improvement = float(o3_res) / res
        os_improvement = float(os_res) / res
        print (name + ',' + str(res) + ',' + str(base_improvement) + ',' + str(o3_improvement) + ',' + str(os_improvement) + ',' + flags)


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
        help='Command to use to compile C source')
    parser.add_argument('--cxx', nargs=1, required=True,
        help='Command to use to compile C++ source')
    parser.add_argument('--fort', nargs=1, required=True,
        help='Command to use to compile Fortran source')
    parser.add_argument('--database', nargs=1, required=True,
        help='mageec database to store generated compilations in')
    parser.add_argument('--features', nargs=1, required=True,
        help='File containing extracted features for the source being built')
    parser.add_argument('--measure-script', nargs=1, required=True,
        help='Script to measure the resultant executables')
    parser.add_argument('--exec-flags', nargs=1, required=True,
        help='Flags to use when executing generated programs')

    # optional arguments
    parser.add_argument('--debug', action='store_true', required=False,
        help='Enable debug when doing feature extraction')
    parser.add_argument('--build-system', nargs=1, required=False,
        help='Build system to be used to build the source. May be \'cmake\', '
             '\'configure\', or a script to be used to build the source')
    parser.add_argument('--flags', nargs=1, required=False,
        help='Common arguments to be used when building')
    parser.add_argument('--jobs', nargs=1, required=False,
        help='Number of jobs to run when building')
    parser.set_defaults(debug=False,
                        build_system=[None],
                        flags=[''],
                        jobs=[1])

    args = parser.parse_args(sys.argv[1:])
    src_dir         = os.path.abspath(args.src_dir[0])
    build_dir       = os.path.abspath(args.build_dir[0])
    install_dir     = os.path.abspath(args.install_dir[0])
    cc              = args.cc[0]
    cxx             = args.cxx[0]
    fort            = args.fort[0]
    database_path   = os.path.abspath(args.database[0])
    features_path   = os.path.abspath(args.features[0])
    measure_script  = args.measure_script[0]
    exec_flags      = args.exec_flags[0]

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
    if not mageec.is_command_on_path(cxx):
        print ('-- Compiler \'' + cxx + '\' is not on the path')
        return -1
    if not mageec.is_command_on_path(fort):
        print ('-- Compiler \'' + fort + '\' is not on the path')
        return -1

    debug        = args.debug
    build_system = args.build_system[0]
    flags        = args.flags[0]
    jobs         = int(args.jobs[0])
    if jobs < 1:
        print ('-- Number of jobs must be a positive integer')
        return -1

    res = combined_elimination(src_dir=src_dir,
                               build_dir=build_dir,
                               install_dir=install_dir,
                               build_system=build_system,
                               cc=cc,
                               cxx=cxx,
                               fort=fort,
                               flags=flags,
                               jobs=jobs,
                               database_path=database_path,
                               features_path=features_path,
                               measure_script=measure_script,
                               exec_flags=exec_flags,
                               debug=debug)
    if not res:
        return -1
    return 0


if __name__ == '__main__':
    main()
