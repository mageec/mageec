/*  MAGEEC Machine Learner
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

/** @file ml.cpp MAGEEC Machine Learner */
#include "mageec/mageec-ml.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <cstdlib>

using namespace mageec;

// Modified C5 classifier which is currently compiled as C
extern "C" {
  int c5_classify_main(char *filestem);
}

int mageec_ml::init (std::string dbfilename)
{
  std::cerr << "LEARNER: Hello!" << std::endl;
  /* FIXME: The second parameter should be false, we do not want to be creating
     a new database here */
  db = new database(dbfilename, true);
  known_passes = db->get_pass_list();
  return 0;
}

int mageec_ml::init (std::string compiler_version,
                     std::string compiler_target)
{
  return init (compiler_version + "-" + compiler_target + ".db");
}

void mageec_ml::new_file (void)
{
  std::cerr << "LEARNER: New file" << std::endl;
}

void mageec_ml::end_file (void)
{
  std::cerr << "LEARNER: End file" << std::endl;
}

void mageec_ml::finish (void)
{
  std::cerr << "LEARNER: Goodbye!" << std::endl;
  if (db)
    delete db;
}

std::vector<mageec_pass*> mageec_ml::all_passes (void)
{
  return known_passes;
}

void mageec_ml::add_result (std::vector<mageec_feature*> features,
                            std::vector<mageec_pass*> passes,
                            int64_t metric,
                            bool good __attribute__((unused)))
{
  if (!db)
    return;

  result res;
  res.passlist = passes,
  res.featlist = features,
  res.progname = "",
  res.metric = metric;

  db->add_result(res);
}

void mageec_ml::add_result (result res)
{
  if (!db)
    return;
  db->add_result(res);
}

decision mageec_ml::make_decision (mageec_pass *pass,
                                   std::vector<mageec_feature*> features)
{
  assert(pass != NULL && "Decision of null pass requested.");
  // We cannot make a decision if we don't have a database backend
  if (!db)
    return NATIVE_DECISION;

  unsigned long featurecount = features.size();
  assert(featurecount > 0 && "Empty feature vector.");
  const char *passname = pass->name().c_str();

  // namebuf will be used as a buffer to generate file names
  char namebuf[1024];

  // "tree" file - Learnt machine learning tree (and return if no tree)
  const char *treedata = db->get_pass_blob (pass->name());
  if (treedata == NULL)
    return NATIVE_DECISION;

  snprintf (namebuf, 1024, "/tmp/%s.tree", passname);
  std::ofstream treefile(namebuf);
  treefile << treedata;
  treefile.close();

  // "names" file - Columns for classifier
  snprintf (namebuf, 1024, "/tmp/%s.names", passname);
  std::ofstream namefile(namebuf);
  namefile << "runpass." << std::endl;
  for (unsigned long i = 0; i < featurecount; i++)
    namefile << features[i]->name() << ": continuous." << std::endl;
  namefile << "runpass: t,f" << std::endl;;
  namefile.close();

  // "cases" file - Data to make decision on (i.e. our function)
  snprintf (namebuf, 1024, "/tmp/%s.cases", passname);
  std::ofstream casesfile(namebuf);
  for (unsigned int i = 0; i < featurecount; i++)
    casesfile << features[i]->get_feature() << ',';
  casesfile << '?' << std::endl;
  casesfile.close();

  // Classify program as run/don't run
  snprintf (namebuf, 1024, "/tmp/%s", passname);
  int execute = c5_classify_main(namebuf);

  // Delete temporary files
  /*snprintf (namebuf, 1024, "/tmp/%s.cases", passname);
  remove (namebuf);
  snprintf (namebuf, 1024, "/tmp/%s.names", passname);
  remove (namebuf);
  snprintf (namebuf, 1024, "/tmp/%s.tree", passname);
  remove (namebuf);
  */

  return (execute == 1) ? FORCE_EXECUTE : FORCE_NOEXECUTE;
}

// FIXME: Add random file name prefix
void mageec_ml::process_results()
{
  // We cannot learn if we don't have a database backend
  if (!db)
    return;

  std::vector<result> results = db->get_all_results();

  // If we have no results, then there is nothing to learn.
  if (results.size() == 0)
    return;

  // Calculate number of features from length of first result's feature vector
  unsigned long featurecount = results[0].featlist.size();

  /* Brief note to myself:
     Write out the column file for the pass, using "torun" as the variable to
     find
   */
  for (unsigned long i = 0, size = known_passes.size(); i < size; i++)
  {
    // namebuf will be used as a buffer to generate file names
    char namebuf[1024];
    std::string passnamestr = known_passes[i]->name();
    const char *passname = passnamestr.c_str();
    std::cerr << "Training for " << passname << std::endl;

    // Output names file (columns for classifier)
    snprintf (namebuf, 1024, "/tmp/%s.names", passname);
    std::ofstream namefile(namebuf);
    namefile << "runpass." << std::endl;
    for (unsigned long j = 0; j < featurecount; j++)
      namefile << results[0].featlist[j]->name()
               << ": continuous." << std::endl;
    namefile << "runpass: t,f" << std::endl;;
    namefile.close();

    // Output test data (.data file)
    snprintf (namebuf, 1024, "/tmp/%s.data", passname);
    std::ofstream testfile(namebuf);
    for (unsigned long j = 0, jsize = results.size(); j < jsize; j++)
    {
      for (unsigned long k = 0; k < featurecount; k++)
        testfile << results[j].featlist[k]->get_feature() << ',';
      bool ran = false;
      for (unsigned long k = 0, ksize = results[j].passlist.size(); k < ksize; k++)
        if (results[j].passlist[k]->name() == passname)
        {
          ran = true;
          break;
        }
      if (ran)
        testfile << 't' << std::endl;
      else
        testfile << 'f' << std::endl;
    }
    testfile.close();

    // Call the machine learner
    FILE *fpipe;
    snprintf (namebuf, 1024, "%s/c5.0 -f /tmp/%s", LIBEXECDIR, passname);
    if (!(fpipe = static_cast<FILE *>(popen(namebuf, "r"))))
      std::cerr << "Error Training for " << passname << std::endl;
    else
      while (fgets(namebuf, 1024, fpipe))
        ;
    pclose(fpipe);

    // Store Machine Learnt Tree
    snprintf (namebuf, 1024, "/tmp/%s.tree", passname);
    std::ifstream treefile(namebuf, std::ifstream::binary);
    if (treefile)
    {
      treefile.seekg (0, treefile.end);
      long treelength = treefile.tellg();
      treefile.seekg (0, treefile.beg);
      char *treebuf = new char[treelength];
      treefile.read (treebuf, treelength);

      db->store_pass_blob (passnamestr, treebuf);

      delete[] treebuf;
    }

    // Delete temporary files (namebuf holds tree file for storing)
    /*remove (namebuf);
    snprintf (namebuf, 1024, "/tmp/%s.names", passname);
    remove (namebuf);
    snprintf (namebuf, 1024, "/tmp/%s.data", passname);
    remove (namebuf);
    snprintf (namebuf, 1024, "/tmp/%s.tmp", passname);
    remove (namebuf);*/

  }
}

// Initilizer for file based machine learner
int file_ml::init (std::string dbfilename __attribute__((unused)))
{
  // Attempt to load the file pointed to by MAGEEC_EXECUTELIST or a default.
  const char *pass_file = getenv("MAGEEC_EXECUTELIST");
  if (pass_file == NULL)
    pass_file = static_cast<const char *>("/tmp/mageec-executelist");
  std::ifstream input_file(pass_file);
  if (!input_file.is_open())
    return 1;

  // Each line in this file holds the name of a pass we want to execute
  // Empty lines are ignored as they do not contain a pass
  for (std::string line; getline(input_file, line); )
    if (line != "")
      passlist.push_back(line);

  input_file.close();
  return 0;
}

// Initilizer for file based machine learner
int file_ml::init (std::string compiler_version __attribute__((unused)),
                   std::string compiler_target __attribute__((unused)))
{
  return init("");
}

// File based decision maker
decision file_ml::make_decision (mageec_pass *pass,
                                 std::vector<mageec_feature*> features
                                   __attribute__((unused)))
{
  unsigned long passcount = passlist.size();
  std::string passname = pass->name();
  for (unsigned long i = 0; i < passcount; i++)
    if (passlist[i] == passname)
      return FORCE_EXECUTE;
  return FORCE_NOEXECUTE;
}
