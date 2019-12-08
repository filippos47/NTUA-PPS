import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys

def parse_file(fname):
        with open(fname) as f:
                lines = f.readlines()
        
        i = 1
        size_stats = {}
        for line in lines:
                # line of the form:
                # executable X Y Px Py Iter ComputationTime TotalTime midpoint processes
                splitted = line.split()
                # executable
                executable = splitted[0].strip()
                # elapsed time
                elapsed_time = splitted[14].strip()
                # processes number
                process_num = splitted[18].strip()
                if not size_stats.get(executable, None):
                    size_stats[executable] = []

                size_stats[executable].append({"elapsed": elapsed_time, "processes": process_num})
        return size_stats

if len(sys.argv) < 2:
    print ("Usage parse_stats.py <input_file>")
    exit(-1)

stats_by_size = parse_file(sys.argv[1])
markers = ['.', 'o', 'v', '*', 'D', 'X']

serial_time={}


x_ticks = [1, 2, 4, 8, 16, 32, 64]
fig = plt.figure(1)
plt.grid(True)
ax = plt.subplot(111)
ax.set_xlabel("Number of processes")
ax.set_ylabel("Speedup (Serial Time / Parallel Time)")
ax.xaxis.set_ticks(x_ticks)
ax.xaxis.set_ticklabels(map(str, x_ticks))

for j, size in enumerate(sorted(stats_by_size.keys(), key=lambda x: sorted(x))):
    stats = stats_by_size[size]
    y_axis = [0 for _ in range(len(x_ticks))]

    for stat in stats:
        if int(stat["processes"]) == 1:
            serial_time[size] = float(stat["elapsed"])

    for stat in stats:
        pos = x_ticks.index(int(stat["processes"]))
        y_axis[pos] = serial_time[size] / float(stat["elapsed"])

    ax.plot(x_ticks, tuple(y_axis), label=str(size), marker=markers[j])

lgd = ax.legend(ncol=len(stats_by_size.keys()), bbox_to_anchor=(0.9, -0.1), prop={'size':8})
plt.savefig("heat-diffusion-6144-speedup.png", bbox_extra_artists=(lgd,), bbox_inches='tight')

