/*  GIMPLE Feature Extractor
    Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/** @file feature-extract.cpp GIMPLE Feature Extractor. */
/* We need to undefine these as gcc-plugin.h redefines them */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "gcc-plugin.h"
#include "tree-pass.h"
#include "gimple.h"
#include "function.h"
#include "toplev.h"
#include "mageec-plugin.h"
#include <iostream>
#include <vector>

using namespace mageec;

/**
 * Minimum element in a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_min (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  if (size == 1)
    return a[0];
  T min_val = std::min(a[0], a[1]);
  for (unsigned int i=2; i < size; i++)
    min_val = std::min(min_val, a[i]);
  return min_val;
}

/**
 * Maximum element in a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_max (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  if (size == 1)
    return a[0];
  T min_val = std::max(a[0], a[1]);
  for (unsigned int i=2; i < size; i++)
    min_val = std::max(min_val, a[i]);
  return min_val;
}

/**
 * Sum of a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_sum (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  T total = a[0];
  for (unsigned int i=1; i < size; i++)
    total += a[i];
  return total;
}

/**
 * Gate for whether to run the MAGEEC Feature Extractor.
 * Returns true to always run
 */
static bool mageec_featextract_gate(void)
{
  return true;
}

/**
 * MAGEEC Feature Extractor
 */
static unsigned mageec_featextract_exec(void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  std::vector<unsigned> count;

  unsigned bb_count = 0;
  unsigned stmt_count = 0;

  FOR_EACH_BB(bb)
  {
    if (bb_count != 0)
      count.push_back(stmt_count);
    stmt_count = 0;
    bb_count++;
    for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi))
    {
      stmt_count++;
    }
  }
  count.push_back(stmt_count);

  std::cerr << "Current Function: " << current_function_name() << std::endl;
  std::cerr << "  Basic Block Count:     " << bb_count << std::endl;
  std::cerr << "  Min Statement in BB:   " << vector_min(count) << std::endl;
  std::cerr << "  Max Statement in BB:   " << vector_max(count) << std::endl;
  std::cerr << "  Avg Statement in BB:   " << vector_sum(count)/bb_count
            << std::endl;
  std::cerr << "  Total Statement in BB: " << vector_sum(count) << std::endl;

  /* Build feature vector to pass to machine learner */
  std::vector<mageec::mageec_feature*> features;
  features.push_back(new basic_feature("1", bb_count));
  features.push_back(new basic_feature("2", vector_min(count)));
  //features.push_back(new basic_feature("ft3a", vector_max(count)));
  //features.push_back(new basic_feature("ft4a", (vector_sum(count)/bb_count)));
  //features.push_back(new basic_feature("ft5a", vector_sum(count)));

  mageec_inst.take_features(current_function_name(), features);

  return 0;
}

/**
 * MAGEEC Feature Extractor Pass Definition
 */
static struct gimple_opt_pass mageec_featextract =
{
  {
    GIMPLE_PASS,
    "mageec-extractor",                   /* name */
    OPTGROUP_NONE,                        /* optinfo_flags */
    mageec_featextract_gate,              /* gate */
    mageec_featextract_exec,              /* execute */
    NULL,                                 /* sub */
    NULL,                                 /* next */
    0,                                    /* static_pass_number */
    TV_NONE,                              /* tv_id */
    PROP_ssa,                             /* properties_required */
    0,                                    /* properties_provided */
    0,                                    /* properties_destroyed */
    0,                                    /* todo_flags_start */
    0                                     /* todo_flags_finish */
  }
};

/**
 * Registers the Feature Extractor in the pass list
 * Currently runs after the cfg pass
 */
void register_featextract (void)
{
  struct register_pass_info pass;

  pass.pass = &mageec_featextract.pass;
  pass.reference_pass_name = "cfg";
  pass.ref_pass_instance_number = 1;
  pass.pos_op = PASS_POS_INSERT_AFTER;
  
  register_callback (mageec_gcc_plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass);
}
