#!/usr/bin/python
import numpy as np
import pandas as pd
import scipy.stats as stats
import matplotlib.pyplot as plt
import matplotlib
import argparse

df = pd.read_csv("output_overhead_processed.txt", sep=" ", header=None)
df_list = df.values.tolist()

font = {
    # 'family' : 'normal',
    'family': 'monospace',
    'weight': 'normal',
    'size': 14
}
matplotlib.rc('font', **font)
plt.rc('grid', linestyle="-", linewidth=0.5)
fig = plt.figure()
ax = fig.add_subplot(111)
width = 0.2

labels = ['NDN Multicast', 'WSMP Flooding']

ax.bar(labels, df_list[0], width, edgecolor='white', hatch='.', label='Interest Packet')
ax.bar(labels, df_list[1], width, yerr=df_list[2], hatch='', bottom=df_list[0],
       label='Data Packet')

ax.legend()
plt.ylabel("# Packets")
# plt.legend(loc = 'center right', fancybox=True)
plt.legend(loc='upper center', fancybox=True)
plt.grid(True, axis='y')
plt.style.use('classic')
fig.set_size_inches(8, 4)

if False:
    fig.savefig("plot.pdf", format='pdf', dpi=1000)
    fig.savefig("plot.svg", format='svg', dpi=1000)
    fig.savefig("plot.png", format='png', dpi=1000)
else:
    plt.show()
