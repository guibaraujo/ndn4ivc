#!/usr/bin/python

import numpy as np
import scipy as sc
from scipy.stats import sem, t

sample = [1,2,3,4,5,6,7,8,8,8,8]


print("amostra:", sample) 

sample_mean = np.mean(sample)
sample_median = np.median(sample)
sample_mode = sc.stats.mode(sample)

print("media:", sample_mean)
print("mediana:", sample_median)
print("moda:", sample_mode.mode[0])
print("moda qtd:", sample_mode.count[0])


confidence_level = 0.95
degrees_freedom = len(sample) - 1

sample_standard_error = sc.stats.sem(sample)
print(sample_standard_error)

#confidence_interval = scipy.stats.t.interval(confidence_level, degrees_freedom, sample_mean, sample_standard_error)
