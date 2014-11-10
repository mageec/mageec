#!/usr/bin/env python2

#   File to parse execute directory (in format used by Simon)
#
#   This script should be run from the base directory for all of the runs, and
#   will place the resultant database in this directory
#
#   Copyright (C) 2014 Embecosm Limited and University of Bristol
#
#   This file is part of MAGEEC.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

import glob, gzip, hashlib, json, os, re, sqlite3, sys

# Dictionary of the scale factors of individual BEEBS benchmarks, extracted
# using the grab_scale_factor.sh script in BEEBS
test_factor = {}
test_factor['src/2dfir/2dfir'] = 4096. / 4096
test_factor['src/adpcm/adpcm'] = 4096. / 32
test_factor['src/aha-compress/aha-compress'] = 4096. / 256
test_factor['src/aha-mont64/aha-mont64'] = 4096. / 256
test_factor['src/blowfish/blowfish'] = 4096. / 4096
test_factor['src/bs/bs'] = 4096. / 4096
test_factor['src/bubblesort/bubblesort'] = 4096. / 512
test_factor['src/cnt/cnt'] = 4096. / 1024
test_factor['src/compress/compress'] = 4096. / 1024
test_factor['src/cover/cover'] = 4096. / 2048
test_factor['src/crc/crc'] = 4096. / 2048
test_factor['src/crc32/crc32'] = 4096. / 4096
test_factor['src/ctl-stack/ctl-stack'] = 4096. / 256
test_factor['src/ctl-string/ctl-string'] = 4096. / 512
test_factor['src/ctl-vector/ctl-vector'] = 4096. / 512
test_factor['src/cubic/cubic'] = 4096. / 4096
test_factor['src/dhrystone/dhrystone'] = 4096. / 4096
test_factor['src/dijkstra/dijkstra'] = 4096. / 2048
test_factor['src/dtoa/dtoa'] = 4096. / 512
test_factor['src/duff/duff'] = 4096. / 4096
test_factor['src/edn/edn'] = 4096. / 64
test_factor['src/expint/expint'] = 4096. / 2048
test_factor['src/fac/fac'] = 4096. / 4096
test_factor['src/fasta/fasta'] = 4096. / 256
test_factor['src/fdct/fdct'] = 4096. / 2048
test_factor['src/fft/fft'] = 4096. / 4096
test_factor['src/fibcall/fibcall'] = 4096. / 4096
test_factor['src/fir/fir'] = 4096. / 16
test_factor['src/frac/frac'] = 4096. / 64
test_factor['src/huffbench/huffbench'] = 4096. / 1
test_factor['src/insertsort/insertsort'] = 4096. / 4096
test_factor['src/janne_complex/janne_complex'] = 4096. / 4096
test_factor['src/jfdctint/jfdctint'] = 4096. / 1024
test_factor['src/lcdnum/lcdnum'] = 4096. / 4096
test_factor['src/levenshtein/levenshtein'] = 4096. / 64
test_factor['src/lms/lms'] = 4096. / 64
test_factor['src/ludcmp/ludcmp'] = 4096. / 512
test_factor['src/matmult-float/matmult-float'] = 4096. / 2048
test_factor['src/matmult-int/matmult-int'] = 4096. / 2048
test_factor['src/mergesort/mergesort'] = 4096. / 4096
test_factor['src/miniz/miniz'] = 4096. / 1
test_factor['src/minver/minver'] = 4096. / 1024
test_factor['src/nbody/nbody'] = 4096. / 1024
test_factor['src/ndes/ndes'] = 4096. / 64
test_factor['src/nettle-arcfour/nettle-arcfour'] = 4096. / 256
test_factor['src/nettle-cast128/nettle-cast128'] = 4096. / 2048
test_factor['src/nettle-des/nettle-des'] = 4096. / 2048
test_factor['src/nettle-md5/nettle-md5'] = 4096. / 4096
test_factor['src/newlib-exp/newlib-exp'] = 4096. / 2048
test_factor['src/newlib-log/newlib-log'] = 4096. / 2048
test_factor['src/newlib-mod/newlib-mod'] = 4096. / 4096
test_factor['src/newlib-sqrt/newlib-sqrt'] = 4096. / 2048
test_factor['src/ns/ns'] = 4096. / 4096
test_factor['src/nsichneu/nsichneu'] = 4096. / 4096
test_factor['src/picojpeg/picojpeg'] = 4096. / 4
test_factor['src/prime/prime'] = 4096. / 512
test_factor['src/qrduino/qrduino'] = 4096. / 1024
test_factor['src/qsort/qsort'] = 4096. / 2048
test_factor['src/qurt/qurt'] = 4096. / 512
test_factor['src/recursion/recursion'] = 4096. / 2048
test_factor['src/rijndael/rijndael'] = 4096. / 4096
test_factor['src/select/select'] = 4096. / 2048
test_factor['src/sglib-arraybinsearch/sglib-arraybinsearch'] = 4096. / 256
test_factor['src/sglib-arrayheapsort/sglib-arrayheapsort'] = 4096. / 128
test_factor['src/sglib-arrayquicksort/sglib-arrayquicksort'] = 4096. / 256
test_factor['src/sglib-dllist/sglib-dllist'] = 4096. / 128
test_factor['src/sglib-hashtable/sglib-hashtable'] = 4096. / 128
test_factor['src/sglib-listinsertsort/sglib-listinsertsort'] = 4096. / 128
test_factor['src/sglib-listsort/sglib-listsort'] = 4096. / 128
test_factor['src/sglib-queue/sglib-queue'] = 4096. / 128
test_factor['src/sglib-rbtree/sglib-rbtree'] = 4096. / 64
test_factor['src/sha/sha'] = 4096. / 1024
test_factor['src/slre/slre'] = 4096. / 128
test_factor['src/sqrt/sqrt'] = 4096. / 4096
test_factor['src/st/st'] = 4096. / 4096
test_factor['src/statemate/statemate'] = 4096. / 4096
test_factor['src/stb_perlin/stb_perlin'] = 4096. / 4096
test_factor['src/stringsearch1/stringsearch1'] = 4096. / 1024
test_factor['src/strstr/strstr'] = 4096. / 4096
test_factor['src/tarai/tarai'] = 4096. / 4096
test_factor['src/template/template'] = 4096. / 4096
test_factor['src/trio-snprintf/trio-snprintf'] = 4096. / 512
test_factor['src/trio-sscanf/trio-sscanf'] = 4096. / 512
test_factor['src/ud/ud'] = 4096. / 4096
test_factor['src/whetstone/whetstone'] = 4096. / 4096
test_factor['src/wikisort/wikisort'] = 4096. / 2

""" Convert a string from "2.2m to 0.0022. """
def refloat(number):
  if number[-1] == 'm':
    return int((float(number[:-1]) / 1000) * 1000 * 1000)
  elif number[-1] == 'k':
    return int((float(number[:-1]) * 1000) * 1000 * 1000)
  elif number[-1] == 'u':
    return int((float(number[:-1]) / 1000 / 1000) * 1000 * 1000)
  elif number[-1] == 'M':
    return int((float(number[:-1]) * 1000 * 1000) * 1000 * 1000)
  else:
    return int((float(number)) * 1000 * 1000)

""" Function to parse buildlog to get features and passes. """
def getPassesFeatures(filename, result):
  filename = filename.replace('execute.log', 'build.log')
  file2 = open(filename)
  file2 = file2.read()

  # FIXME: Make this more durable, this is GCC specific
  # (and ignores multifile for now)
  #buildlog = re.search(r'-MT ' + r[0] + r'\.o .*mv -f \.deps/' +
  #                     r[0] + r'.Tpo', file2, flags=re.DOTALL)
  #print r

  dirname = r[0].split('/')[1]

  buildlog = re.search(r'Making all in src/' + dirname + r'\n.*' +
                       r'Leaving directory `[/A-Za-z-0-9]+/src/' + dirname + r'\'',
                       file2, flags=re.DOTALL)
  buildlog = buildlog.group(0)


  # Find functions
  functions = re.findall(r'Current Function: (.*)\n', buildlog)
  functions = sorted(set(functions))
  #print functions
  # FIXME: Temporary
  #functions.remove('(nofn)')
  functions.remove('stop_trigger')
  functions.remove('start_trigger')
  functions.remove('initialise_board')
  functions.remove('main')
  functions.remove('__aeabi_memset')
  functions.remove('__aeabi_memcpy')
  functions.remove('initialise_benchmark')
  functions.remove('software_init_hook')

  #functions = [functions[0]]

  # Build pass list for each function
  executed = {}
  features = {}
  for f in functions:
    passes = re.findall(r'Fn, \"' + f +r'\", \"(.*)\"', buildlog)
    
    #passes = re.findall(r'Pass: \'(.*)\',\s*Type:.*Function: \'(' + f +
    #                    r')\',  Gate: ([0-9]+)\n(  New gate: [01])?', buildlog,
    #                    flags=re.MULTILINE)
    executed[f] = []
    for p in passes:
      executed[f].append(p)
      #if p[3] == '  New gate: 0':
      #  continue
      #elif p[3] == '  New gate: 1':
      #  executed[f].append(p[0])
      #elif p[2] == '1':
      #  executed[f].append(p[0])

    # Build JSON feature vector
    vector = re.search(r'Current Function: ' + f + r'\n' +
                         r'(\s+\( *ft[0-9]+\).*\n)+' +
                         r'(\[.*)',
                         buildlog, flags=re.MULTILINE);
    try:
      vector = vector.group(0)
      vector = vector.split('\n')[-1]
      vector = json.loads(vector)
      features[f] = vector
    except:
      pass

  #print executed
  #print features
  return (executed, features)


if __name__ == '__main__':
  con = sqlite3.connect('result.db')
  cur = con.cursor()

  files = glob.glob('run-*/execute.log')
  #files = glob.glob('run-597/beebs.log')
  #files = sorted(files, key=lambda s: int(s[4:-10]))
  #FIXME: Only care about the first execution
  #files = [files[143]]

  # Set up database (So if we don't already have a database we can train)
  #cur.execute("PRAGMA foreign_keys = ON")
  cur.execute("CREATE TABLE IF NOT EXISTS passes (passname TEXT PRIMARY KEY)");
  cur.execute("CREATE TABLE IF NOT EXISTS passorder (seq INTEGER NOT NULL, " +
              "pos INTEGER NOT NULL, pass TEXT NOT NULL, " +
              "PRIMARY KEY (seq, pos))")
  cur.execute("CREATE TABLE IF NOT EXISTS features (prog TEXT NOT NULL, " +
              "feature INTEGER NOT NULL, value INTEGER NOT NULL, " +
              "PRIMARY KEY (prog, feature))")
  cur.execute("CREATE TABLE IF NOT EXISTS results (prog TEXT NOT NULL, " +
              "seq INTEGER NOT NULL, time INTEGER, energy INTEGER, " +
              "PRIMARY KEY (prog, seq))")
  cur.execute("CREATE TABLE IF NOT EXISTS passblob (pass TEXT, blob TEXT, " +
              "PRIMARY KEY (pass))")

  for filename in files:
    print filename

    energylog = file(filename, 'r')
    energylog = energylog.read()
    result_expr = re.compile(r'([^\n]+)\r?\n' +
                             r'Energy:\s+([0-9\.]+ [mu]?)J\r?\n' +
                             r'Time:\s+([0-9\.]+ [mu]?)s\r?\n' +
                             r'Power:\s+([0-9\.]+ [mu]?)W\r?\n' +
                             r'Average current:\s+([0-9\.]+ [mu]?)A\r?\n' +
                             r'Average voltage:\s+([0-9\.]+ [mu]?)V\r?\n',
                             flags=re.MULTILINE)

    results = re.findall(result_expr, energylog)

    # Skip to next set of results if nothing ran
    if len(results) == 0:
      continue

    #print results
    # Here we extract out the values for our Basic tests
    # (test, energy, time, power, current, voltage)
    calib_results = []
    for r in results:
      if r[0].startswith('Basic test'):
        calib_results.append(r)
    for r in calib_results:
      results.remove(r)
    
    # Generate calibration values for this test
    calib_energy = map (lambda x: float(refloat(x[1])), calib_results)
    calib_energy = sum(calib_energy)/len(calib_energy)
    calib_energy = refloat('3.7 m') / calib_energy
    calib_time = map (lambda x: float(refloat(x[2])), calib_results)
    calib_time = sum(calib_time)/len(calib_time)
    calib_time = refloat('7.5') / calib_time
    print 'Calibration: Energy -', calib_energy, ' Time - ', calib_time

    # Here we convert the energy and time results to numbers so we can
    # normalize
    # (We do a cheap trick to allow us to modify the results table)
    for i in xrange(len(results)):
      r = list(results[i])
      r[1] = int(refloat(r[1]) * calib_energy * test_factor[r[0]])
      r[2] = int(refloat(r[2]) * calib_time   * test_factor[r[0]])
      r = tuple(r)
      results[i] = r

    #continue
    # Result array:
    # (test, energy, time, power, current, voltage)

    # FIXME: This will be replaced with a python wrapping of MAGEEC ml library
    # in the mean time I manipulate the database manually.
    for r in results:
      passfeat = getPassesFeatures(filename, r)
      print passfeat[1].keys()
      
      # FIXME!!! Only worry about functions with just one function
      # This should eventually be a for f in function
      if len(passfeat[1]) == 1:
        print (r, passfeat[1].keys())
        function = passfeat[1].keys()[0]
        passes = passfeat[0][function]
        features = passfeat[1][function]
        # Add passes to pass table
        for p in passes:
          try:
            cur.execute("INSERT INTO `passes` VALUES ('%s')" % p)
            #con.commit()
          except:
            pass
        # Generate sequence ID
        # (FIXME Note this may not exactly the same as the way mageec does this,
        #  so databases might be incompatible. This will be fixed shortly)
        passkeyid=''
        for p in passes:
          passkeyid = passkeyid+p
        passkeyid = hashlib.sha256(passkeyid).hexdigest()
        passkeyid = int(passkeyid[0:16], 16) ^ int(passkeyid[16:32], 16) ^ int(passkeyid[32:48], 16) ^ int(passkeyid[48:64], 16)
        passkeyid = passkeyid - 0x8000000000000000
        # Add data to passorder table
        # This is done by a range/len because we need to count at the same time
        for i in range(len(passes)):
          try:
            cur.execute("INSERT INTO `passorder` VALUES (%li, %i, '%s')" % (passkeyid, i, passes[i]))
            #con.commit()
          except:
            pass
        # Generate hash of test name
        # (FIXME Pretty sure this isn't the method!!!)
        proghash = hashlib.sha256(r[0]).hexdigest()
        proghash = int(proghash[0:16], 16) ^ int(proghash[16:32], 16) ^ int(proghash[32:48], 16) ^ int(proghash[48:64], 16)
        proghash = proghash - 0x8000000000000000
        # Add features to featuretable
        for feat in features:
          featid = int(feat['name'].replace('ft',''))
          featval = feat['value']
          #print (featid, feat)
          try:
            cur.execute("INSERT into `features` VALUES ('%s', %i, %i)" % (r[0], featid, featval))
            #con.commit()
          except:
            pass
        # Add result entry
        try:
          cur.execute("INSERT INTO `results` VALUES ('%s', %li, %li, %li)" % (r[0], passkeyid, r[2], r[1]))
          #con.commit()
        except:
          pass

  con.commit()
        #sys.exit(0)
