# -*- coding: utf-8 -*- 
import sys
import os
import fileinput
import numpy as np
 
vecfile = []
scafile = []
OWD = 0.015  #ms
def formate_filename(filename, deep = 0):
	tab = ''
	d = 0
	while d < deep:
		tab += '  '
		d += 1
	return tab + os.path.basename(filename)
 
def list_dir(dirname, deep = 0): 
	if not os.path.exists(dirname):
		print dirname, 'is not existed'
		sys.exit(1)
 
	if os.path.isfile(dirname):
		if "Hadoop60_D" in dirname:
			vecfile.append(dirname)
		#print dirname
		#print formate_filename(dirname, deep)
 
	if os.path.isdir(dirname):
		#print formate_filename(dirname, deep) + ":"
		# 列出目录的所有文件和子目录
		filenames = os.listdir(dirname)
		for filename in filenames:
			list_dir(dirname + os.sep + filename, deep + 1)

def averagenum(num):
    nsum = 0
    for i in range(len(num)):
        nsum += num[i]
    return nsum / len(num)


def parse_vector(filename): 
	print '----------------------------------------------------------------------'
	linenum = 0
	print filename
	latency =[]
	slowdown = []
	slowdowns = []
	slowdownm = []
	slowdownl = []
	latencys =[]
	smallflow = []
	latencyl =[]
	largeflow = []
	latencym =[]
	mediumflow = []

	latencyH2 = []
	slowdownH2 = []
	throughputH2 = []
	latencyH3 = []
	slowdownH3 = []
	throughputH3 = []
	latencyBurst = []
	slowdownBurst = []
	throughputBurst = []

	#lines[0]: src lines[2]: size(bytes) lines[4]:fct(s)
	endtime = 0
	starttime = 0
	with open(filename) as f:
		for line in f.readlines():
			lines = []
			lines = line.split(' ')
			'''
			if long(lines[2]) <= 100*1000:
				latencys.append(float(lines[4])*1000.0)
				slowdowns.append((float(lines[4])*1000.0-0.015)/(long(lines[2])/5000000.0))
			elif long(lines[2]) >= 10*1000*1000:
				latencyl.append(float(lines[4])*1000.0)
				slowdownl.append((float(lines[4])*1000.0-0.015)/(long(lines[2])/5000000.0))
			elif long(lines[2]) > 100*1000 and long(lines[2]) < 10*1000*1000:
				latencym.append(float(lines[4])*1000.0)
				slowdownm.append((float(lines[4])*1000.0-0.015)/(long(lines[2])/5000000.0))
			'''

			latency.append(float(lines[4])*1000.0)
			slowdown.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0))

			if int(lines[0]) == 2:
				latencyH2.append(float(lines[4])*1000.0)
				slowdownH2.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.012))
				throughputH2.append(8 * (long(lines[2])/ float(lines[4])) / 1e9 )
				
				if long(lines[2]) <= 100*1000:
					latencys.append(float(lines[4])*1000.0)
					slowdowns.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.012))
				elif long(lines[2]) >= 10*1000*1000:
					latencyl.append(float(lines[4])*1000.0)
					slowdownl.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.012))
				elif long(lines[2]) > 100*1000 and long(lines[2]) < 10*1000*1000:
					latencym.append(float(lines[4])*1000.0)
					slowdownm.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.012))
				
			elif int(lines[0]) == 3:
				latencyH3.append(float(lines[4])*1000.0)
				slowdownH3.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.012))
				#throughputH3.append(8 * (long(lines[2])/ float(lines[4])) / 1e9 )
			elif int(lines[0]) > 5 and int(lines[0]) < 30:
				latencyBurst.append(float(lines[4])*1000.0)
				slowdownBurst.append((float(lines[4])*1000.0)/(long(lines[2])/5000000.0+0.008))
			'''
			if starttime == 0:
				starttime = float(lines[5])/1000.0
			if starttime > float(lines[5])/1000.0:
				starttime = float(lines[5])/1000.0
			if endtime < float(lines[5])/1000.0 + float(lines[6])/1000.0:
				endtime = float(lines[5])/1000.0 + float(lines[6])/1000.0
			'''
			linenum = linenum + 1
	'''
	print 'Average FCT: ' + str(averagenum(latency)) + ' ms'
	print 'Average SLOWDOWN: ' + str(averagenum(slowdown))
	print 'Number of SMALL flows (0,100KB): ' + str(len(latencys))
	print 'Number of MEDIUM flows (100KB,10MB): ' + str(len(latencym))
	print 'Number of LARGE flows (10MB,+): ' + str(len(latencyl))
	if len(latencys) > 0:
		print 'Average FCT of SMALL flows: ' + str(averagenum(latencys))
	if len(latencym) > 0:
		print 'Average FCT of MEDIUM flows: ' + str(averagenum(latencym))
	print 'Average latency of LARGE flows: ' + str(averagenum(latencyl))
	
	print 'Average SLOWDOWN of SMALL flows: ' + str(averagenum(slowdowns))
	print 'Average SLOWDOWN of MEDIUM flows: ' + str(averagenum(slowdownm))
	print 'Average SLOWDOWN of LARGE flows: ' + str(averagenum(slowdownl))
	if len(latencys) > 0:
		print 'p99 FCT SMALL flows: ' + str(np.percentile(latencys,99))
	'''

	print 'Number of H2 flows: ' + str(len(latencyH2))
	print 'Number of H3 flows: ' + str(len(latencyH3))
	print 'Number of Burst flows: ' + str(len(latencyBurst))
	print 'Number of H2 SMALL flows: ' + str(len(latencys))
	print 'Number of H2 MEDIUM flows: ' + str(len(latencym))
	print 'Number of H2 LARGE flows: ' + str(len(latencyl))
	print '****************************'
	if len(latencyH2) > 0:
		print 'H2: Average FCT ' + str(averagenum(latencyH2))
		print 'H2: Average SLOWDOWN ' + str(averagenum(slowdownH2))
		print 'H2: p99 FCT ' + str(np.percentile(latencyH2,99))
		
		print 'H2: Average Throughput ' + str(averagenum(throughputH2))
		print 'H2: Average FCT of SMALL flows ' + str(averagenum(latencys))
		print 'H2: Average FCT of MEDIUM flows ' + str(averagenum(latencym))
		if len(latencyl) > 0:
			print 'H2: Average FCT of LARGE flows ' + str(averagenum(latencyl))
		'''
		print 'H2: Average SLOWDOWN of SMALL flows ' + str(averagenum(slowdowns))
		print 'H2: Average SLOWDOWN of MEDIUM flows ' + str(averagenum(slowdownm))
		print 'H2: Average SLOWDOWN of LARGE flows ' + str(averagenum(slowdownl))
		'''
		
	print '****************************'
	if len(latencyH3) > 0:
		print 'H3: Average FCT ' + str(averagenum(latencyH3))
		print 'H3: Average SLOWDOWN ' + str(averagenum(slowdownH3))
		print 'H3: p99 FCT ' + str(np.percentile(latencyH3,99))
	print '****************************'
	if len(latencyBurst) > 0:
		print 'H6-H19: Average FCT ' + str(averagenum(latencyBurst))
		print 'H6-H19: Average SLOWDOWN ' + str(averagenum(slowdownBurst))		
		print 'H6-H19: p99 FCT ' + str(np.percentile(latencyBurst,99))

	print '****************************'
        if len(latency) > 0:
                print 'Overall: Average FCT ' + str(averagenum(latency))
                print 'Overall: Average SLOWDOWN ' + str(averagenum(slowdown))
                print 'Overall: p99 FCT ' + str(np.percentile(latency,99))
	#print 'Flow completion rate (per second): ' + str(1000000 * (endtime - starttime) / linenum )
	#print 'max latency: ' + str(max(latency))

 


 
if len(sys.argv) < 2:
	print 'you should input the dirname'
	sys.exit(1)
 
del sys.argv[0]
for dirname in sys.argv:
	list_dir(dirname)
	print

#parse vec file
for v in vecfile:
	parse_vector(v)
