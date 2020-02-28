#!/usr/bin/python3.5

import os
import time

HOME_DIR = '/root/3s3s.org'
os.chdir(HOME_DIR)

f = open('start.log', 'w')
print (f)
f.write('start log\n')
f.flush()

time.sleep(60)
#f.write('sleep(60) end\n')
#f.flush()

strLogName = "FullLog"
nCounter = 0

#print strLogName + str(nCounter) + ".crash"
#nCounter = nCounter + 1
#print strLogName + str(nCounter) + ".crash"
while True:
    f.write('while - start\n')
    f.flush()
    os.system("ulimit -n 10000")
    os.system("nohup " + HOME_DIR + "/test_server.exe")
    f.write('program stopped\n')
    f.flush()
    os.system("cp FullLog.txt " + strLogName + str(nCounter) + ".crash")
    os.system("rm FullLog.txt")
    f.write('old logs created\n')
    f.flush()
    nCounter = nCounter + 1
    #time.sleep(60)
    os.system("reboot")
    break
    #f.write('sleep 600 end\n')
    #f.flush()