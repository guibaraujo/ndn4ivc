#!/usr/bin/python
import scipy.stats as stats
import os
import fnmatch


def process_ndn_files():
    print
    "processing log files Ndn"
    for filename in log_files_ndn:
        f = open(filename)
        interest_pkt = 0
        data_pkt = 0
        for line in f:
            if line.find(
                    "Forwarder:onOutgoingInterest(): [DEBUG] onOutgoingInterest out=(257,0) interest=/service/traffic/") != -1:
                interest_pkt += 1
            elif line.find(
                    "Forwarder:onOutgoingData(): [DEBUG] onOutgoingData out=(257,0) data=/service/traffic/") != -1:
                data_pkt += 1
        sum_interest_ndn.append(interest_pkt)
        sum_data_ndn.append(data_pkt)
        sum_pkt_ndn.append(interest_pkt + data_pkt)


os.system("rm -f output_overhead_processed.txt")
log_files_ndn = []

listOfFiles = os.listdir('../')
pattern = "*.log"
for entry in listOfFiles:
    if fnmatch.fnmatch(entry, pattern):
        log_files_ndn.append("../" + entry)
print(log_files_ndn)

sum_interest_ndn = []
sum_data_ndn = []
sum_pkt_ndn = []
ndn_str = 0

process_ndn_files()

if len(sum_interest_ndn) >= 10:
    ndn_str = stats.sem(sum_pkt_ndn) * stats.t.ppf((1 + .95) / 2, len(sum_pkt_ndn) - 1)

print(len(sum_interest_ndn))
output_file = open("output_overhead_processed.txt", mode="w", encoding="utf-8")
output_file.write(sum(sum_interest_ndn).__str__() + " " + sum(sum_interest_ndn).__str__() + "\n")
output_file.write(sum(sum_data_ndn).__str__() + " " + sum(sum_data_ndn).__str__() + "\n")
output_file.write(str(ndn_str) + " " + str(ndn_str) + "\n")
output_file.close()
