#! /usr/bin/python

import os
import shutil
import subprocess

count = 0
for i in range(100000):
    proc = subprocess.Popen(['mem_model'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out = proc.communicate()[0]
    if out.count('\n') == 2:
        count += 1
        #print out,
print 'count = ' + str(count)
