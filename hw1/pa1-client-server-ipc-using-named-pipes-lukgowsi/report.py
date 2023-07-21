import time
import os
import csv

data_buffer = []
data_bin = []
truncate = "truncate -s {}K BIMDC/test.bin".format(256)

for i in range(64, 4097, 32):
    args = "./client -m {} -f 1.csv".format(i)
    start_time = time.time()
    os.system(args)
    secs = time.time() - start_time
    os.system("rm -fr received/1.csv")

    data_buffer.append([i, secs])


with open("data_buffer.csv", 'w', newline='') as file:
    wrt = csv.writer(file)
    wrt.writerow(["buffer", "time(s)"])
    wrt.writerows(data_buffer)

for i in range(1024, 1048577, 4092):
    truncate = "truncate -s {} BIMDC/test.bin".format(i)
    args = "./client -f test.bin"
    os.system(truncate)
    start_time = time.time()
    os.system(args)
    secs = time.time() - start_time
    os.system("rm -fr received/test.bin")

    data_bin.append([i, secs])


with open("data_bin.csv", 'w', newline='') as file:
    wrt = csv.writer(file)
    wrt.writerow(["file_size", "time(s)"])
    wrt.writerows(data_bin)
