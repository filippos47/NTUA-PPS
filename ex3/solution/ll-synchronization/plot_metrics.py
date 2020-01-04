import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys

def parse_file(fname):
        with open(fname) as f:
                lines = f.readlines()

        cnt = 1
        size_stats = {}
        for line in lines:
            if line.startswith("Nthreads:"):
                # line of the form:
                # Nthreads: N Runtime(sec): 10 Throughput(Kops/sec): t
                splitted = line.split()
                # lock
                if cnt < 8:
                    lock = "Fine Grained"
                elif cnt < 15:
                    lock = "Lazy"
                else:
                    lock = "Optimistic"
                # throughput
                throughput = splitted[7].strip()
                # thread number
                thread_num = splitted[1].strip()
                if not size_stats.get(lock, None):
                    size_stats[lock] = []

                size_stats[lock].append({"throughput": throughput, "nthread": thread_num})
                cnt += 1
        return size_stats

if len(sys.argv) < 2:
    print ("Usage plot_metrics.py <input_file>")
    exit(-1)
stats_by_size = parse_file(sys.argv[1])
markers = ['.', 'o', 'v', '*', 'D', 'X']
print(stats_by_size)
x_ticks = [1, 2, 4, 8, 16, 32, 64]
fig = plt.figure(1)
plt.grid(True)
ax = plt.subplot(111)
ax.set_xlabel("Number of threads")
ax.set_ylabel("Throughput (Kops / second)")
ax.xaxis.set_ticks(x_ticks)
ax.xaxis.set_ticklabels(map(str, x_ticks))

for j, size in enumerate(sorted(stats_by_size.keys(), key=lambda x: sorted(x))):
    stats = stats_by_size[size]
    y_axis = [0 for _ in range(len(x_ticks))]
    for stat in stats:
        pos = x_ticks.index(int(stat["nthread"]))
        y_axis[pos] = float(stat["throughput"])
    ax.plot(x_ticks, tuple(y_axis), label=str(size), marker=markers[j])

lgd = ax.legend(ncol=len(stats_by_size.keys()), bbox_to_anchor=(0.9, -0.1), prop={'size':8})
plt.savefig("linked_list-1024-20-40-40.png", bbox_extra_artists=(lgd,), bbox_inches='tight')
