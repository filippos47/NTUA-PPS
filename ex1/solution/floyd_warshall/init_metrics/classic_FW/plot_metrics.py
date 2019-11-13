import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys

def parse_file(fname):
        with open(fname) as f:
                lines = f.readlines()

        size_stats = {}
        for line in lines:
                # line of the form:
                # FW_SR,<size>,time,processes
                splitted = line.split(",")
                # size of array
                array_size = splitted[1].strip()
                # elapsed time
                elapsed_time = splitted[2].strip()
                # thread number
                thread_num = splitted[3].strip()
                if not size_stats.get(array_size, None):
                    size_stats[array_size] = []

                size_stats[array_size].append({"elapsed": elapsed_time, "nthread": thread_num})
        return size_stats

if len(sys.argv) < 2:
    print ("Usage parse_stats.py <input_file>")
    exit(-1)

stats_by_size = parse_file(sys.argv[1])
markers = ['.', 'o', 'v', '*', 'D', 'X']

x_ticks = [1, 2, 4, 8, 16, 32, 64]
fig = plt.figure(1)
plt.grid(True)
ax = plt.subplot(111)
ax.set_xlabel("Number of threads")
ax.set_ylabel("Time (seconds)")
ax.xaxis.set_ticks(x_ticks)
ax.xaxis.set_ticklabels(map(str, x_ticks))

for j, size in enumerate(sorted(stats_by_size.keys(), key=lambda x: int(x))):
    stats = stats_by_size[size]
    y_axis = [0 for _ in range(len(x_ticks))]
    for stat in stats:
        pos = x_ticks.index(int(stat["nthread"]))
        y_axis[pos] = float(stat["elapsed"])

    ax.plot(x_ticks, tuple(y_axis), label="N="+str(size), marker=markers[j])

lgd = ax.legend(ncol=len(stats_by_size.keys()), bbox_to_anchor=(0.9, -0.1), prop={'size':8})
plt.savefig("classic-fw-init.png", bbox_extra_artists=(lgd,), bbox_inches='tight')
