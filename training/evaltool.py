#!/usr/bin/env python2

#   File to parse execute directory (in format used by Simon)
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
  filename = filename.replace('beebs.log','output.log.gz')
  file2 = gzip.open(filename)
  file2 = file2.read()
  # FIXME: Make this more durable, this is GCC specific
  # (and ignores multifile for now)
  #buildlog = re.search(r'-MT ' + r[0] + r'\.o .*mv -f \.deps/' +
  #                     r[0] + r'.Tpo', file2, flags=re.DOTALL)
  buildlog = re.search(r'Making all in src/' + r[0] + r'\n.*' +
                       r'Leaving directory `[/A-Za-z-0-9]+/src/' + r[0] + r'\'',
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
  #functions = [functions[0]]

  # Build pass list for each function
  executed = {}
  features = {}
  for f in functions:
    passes = re.findall(r'Pass: \'(.*)\',\s*Type:.*Function: \'(' + f +
                        r')\',  Gate: ([0-9]+)\n(  New gate: [01])?', buildlog,
                        flags=re.MULTILINE)
    executed[f] = []
    for p in passes:
      if p[3] == '  New gate: 0':
        continue
      elif p[3] == '  New gate: 1':
        executed[f].append(p[0])
      elif p[2] == '1':
        executed[f].append(p[0])

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
  con = sqlite3.connect('test.db')
  cur = con.cursor()

  # Move to correct directory, don't commit this for me
  os.chdir('/home/simon/work/beebs-results-avr')

  files = glob.glob('run-*/beebs.log')
  #files = glob.glob('run-597/beebs.log')
  files = sorted(files, key=lambda s: int(s[4:-10]))

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
    result_expr = re.compile(r'Energy for (.*): Measurement point ([0-9]+)\r?\n' +
                             r'Energy:\s+([0-9\.]+ [mu]?)J\r?\n' +
                             r'Time:\s+([0-9\.]+ [mu]?)s\r?\n' +
                             r'Power:\s+([0-9\.]+ [mu]?)W\r?\n' +
                             r'Average current:\s+([0-9\.]+ [mu]?)A\r?\n' +
                             r'Average voltage:\s+([0-9\.]+ [mu]?)V\r?\n',
                             flags=re.MULTILINE)

    results = re.findall(result_expr, energylog)
    # Result array:
    # (test, mpoint, energy, time, power, current, voltage)

    # FIXME: This will be replaced with a python wrapping of MAGEEC ml library
    # in the mean time I manipulate the database manually.
    for r in results:
      passfeat = getPassesFeatures(filename, r)
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
          cur.execute("INSERT INTO `results` VALUES ('%s', %li, %li, %li)" % (r[0], passkeyid, refloat(r[3]), refloat(r[2])))
          #con.commit()
        except:
          pass

  con.commit()
        #sys.exit(0)
