#!/usr/bin/python
import scipy.stats as stats
import os
import numpy as np
import fnmatch
import xmltodict


def process_trip_files(file):
    with open(file) as fd:
        doc = xmltodict.parse(fd.read())
        # print(doc['tripinfos']['tripinfo'][0]['@duration'])
        # print(len(doc['tripinfos']['tripinfo']))

    trips_duration = []
    for i in range(0, len(doc['tripinfos']['tripinfo'])):
        trips_duration.append(float(doc['tripinfos']['tripinfo'][i]['@duration']))
    return np.mean(trips_duration)


# set here experiment number 
EXPN="*exp2*tripinfo.xml"

log_trip_files = []

listOfFiles = os.listdir('../logfiles')
pattern = EXPN
for entry in listOfFiles:
    if fnmatch.fnmatch(entry, pattern):
        log_trip_files.append("../logfiles/" + entry)
print(log_trip_files)

sum_trips_duration = []
trips_duration_str = 0

for f in log_trip_files:
    sum_trips_duration.append(process_trip_files(f))

if len(sum_trips_duration) >= 10:
    trips_duration_str = stats.sem(sum_trips_duration) * stats.t.ppf((1 + .95) / 2, len(sum_trips_duration) - 1)

print("media:", np.mean(sum_trips_duration))
print("ic:", trips_duration_str)

print("Quartil Q1:", np.quantile(sum_trips_duration, .25))
print("Quartil Q3:", np.quantile(sum_trips_duration, .75))

