import csv
import matplotlib.pyplot as plt

filename = "cache_refined_tlb_results.csv"

working_set = []
median_latency = []

with open(filename, "r") as file:
    reader = csv.DictReader(file)

    for row in reader:
        working_set.append(float(row["Working_Set_KB"]))
        median_latency.append(float(row["Median_ns"]))

# Plotting the data
plt.figure(figsize=(10, 6))

plt.plot(
    working_set,
    median_latency,
    marker='o',
    linestyle='-',
    linewidth=1.5,
    markersize=4
)

plt.xlabel("Working Set Size (KiB)")
plt.ylabel("Median Memory Access Latency (ns)")
plt.title("Cache Access Latency vs Working Set Size")

plt.xscale("log", base=2)
plt.grid(True, linestyle="--", alpha=0.5)

plt.tight_layout()
plt.savefig("cache_latency_plot.png", dpi=300)
plt.show()

print("Plot saved as cache_latency_plot.png")
