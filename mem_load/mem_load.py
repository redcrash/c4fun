#! /usr/bin/python2

import os
import re
import shutil
import subprocess

SIZE = '64000'
NUMA_NODE = '0'
NB_ITER = '100000'
NB_READS = long(NB_ITER) * 100
NB_THREADS_MAX = 11

for mode in ['rand', 'seq']:
    latencies = []
    procs = []
    for i in range(0, NB_THREADS_MAX + 1):
        # Launch concurrent thread
        if (i > 0):
            cmd = ['taskset', '-c', str(i), './mem_load', SIZE, mode, NUMA_NODE, '-1']
            procs.append(subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE))

        # Measure latency
        cmd = ['sudo', 'chrt', '-rr' ,'30', 'taskset', '-c', '0', './mem_load', SIZE, mode, NUMA_NODE, NB_ITER]
        print 'Measuring average latency with ' + str(i) + ' concurrent threads'
        out = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[1]
        latencies.append(float(out.split('\n')[0].split(' ')[2]) / NB_READS)

    for proc in procs:
        proc.kill()

    print mode + ' :'
    for i in range(0, NB_THREADS_MAX + 1):
        print '  ' + str(latencies[i])

# Replace in template file
# print 'Creating results pdf file'
# if os.path.exists('results'):
#     shutil.rmtree('results')
# os.makedirs('results')
# with open("results/results.tex", 'w+') as texFile:
#     with open("results.tmpl.tex", "rt") as texTmplFile:
#         for line in texTmplFile:
#             line = line.replace('_data_seq_', _data_seq_)
#             line = line.replace('_data_rand_', _data_rand_)
#             line = line.replace('_L1_', str(int(_L1_) / 1024))
#             line = line.replace('_L2_', str(int(_L2_) / 1024))
#             line = line.replace('_L3_', str(int(_L3_) / 1024))
#             texFile.write(line)
# subprocess.call(['pdflatex', 'results.tex'], cwd='results')#, stdout=subprocess.NONE, stderr=subprocess.NONE)
