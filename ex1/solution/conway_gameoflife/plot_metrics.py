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
                # GameOfLife: Size <array_size> Steps 1000 Time <elapsed_time> Threads <thread_num>
                splitted = line.split(" ")
                # size of array
                array_size = splitted[2]
                # elapsed time
                elapsed_time = splitted[6]
                # thread number
                thread_num = splitted[8]
                if not size_stats.get(array_size, None):
                        size_stats[array_size] = []
                size_stats[array_size].append({"elapsed": elapsed_time, "nthread": thread_num})
        return size_stats

if len(sys.argv) < 2:
    print ("Usage parse_stats.py <input_file>")
    exit(-1)

stats_by_size = parse_file(sys.argv[1])

markers = set(['.', 'o', 'v', '*', 'D', 'X'])

x_ticks = [1, 2, 4, 6, 8]
serial_time = {}
i = 0
for size, stats in stats_by_size.items():
    i += 1
    fig = plt.figure(i)
    plt.grid(True)
    ax = plt.subplot(111)
    ax.set_xlabel("Number of threads")
    ax.set_ylabel("Time (seconds)")
    ax.xaxis.set_ticks(x_ticks)
    ax.xaxis.set_ticklabels(map(str, x_ticks))
    y_axis = [0 for _ in range(len(x_ticks))]
    for stat in stats:
        pos = x_ticks.index(int(stat["nthread"]))
        if int(stat["nthread"]) == 1:
            serial_time[size] = float(stat["elapsed"])
        y_axis[pos] = float(stat["elapsed"])

    ax.plot(x_ticks, tuple(y_axis), label="N="+size, marker=markers.pop())
    lgd = ax.legend(ncol=len(stats_by_size.keys()), bbox_to_anchor=(0.9, -0.1), prop={'size':8})
    plt.savefig("stats-time-" + size + ".png", bbox_extra_artists=(lgd,), bbox_inches='tight')

for size, stats in stats_by_size.items():
    i += 1
    fig = plt.figure(i)
    plt.grid(True)
    ax = plt.subplot(111)
    ax.set_xlabel("Number of threads")
    ax.set_ylabel("Speedup (Serial Time / Parallel Time)")
    ax.xaxis.set_ticks(x_ticks)
    ax.xaxis.set_ticklabels(map(str, x_ticks))
    y_axis = [0 for _ in range(len(x_ticks))]
    for stat in stats:
        pos = x_ticks.index(int(stat["nthread"]))
        y_axis[pos] = serial_time[size] / float(stat["elapsed"])

    ax.plot(x_ticks, tuple(y_axis), label="N="+size, marker=markers.pop())
    lgd = ax.legend(ncol=len(stats_by_size.keys()), bbox_to_anchor=(0.9, -0.1), prop={'size':8})
    plt.savefig("stats-speedup-" + size + ".png", bbox_extra_artists=(lgd,), bbox_inches='tight')
