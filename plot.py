import numpy as np
import matplotlib.pyplot as plt

# List of file names
files = ["sl-1w-1r-time", "sl-2w-2r-time", "sl-3w-3r-time"]

means = []
std_devs = []
labels = []
freq = 2250000

# Read each file and calculate statistics
for name in files:
    try:
        file = name+".txt"
       	data = np.loadtxt(file)  # Load numbers from file
        data = data/freq
        mean = np.mean(data)
        std_dev = np.std(data, ddof=1)  # Using sample standard deviation

        means.append(mean)
        std_devs.append(std_dev)
        labels.append(name)

    except Exception as e:
        print(f"Error reading {file}: {e}")

# Plot the results
plt.figure(figsize=(8, 5))
plt.errorbar(labels, means, yerr=std_devs, fmt='o', capsize=5, capthick=2, markersize=8, label="Mean ± Std Dev")

plt.xlabel("Core composition")
plt.ylabel("Completion in ms")
plt.title("Mean and Standard Deviation of Composition")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)

# Save plot as PDF
plt.savefig("output_plot.pdf", format="pdf", bbox_inches="tight")
plt.show()

print("Plot saved as output_plot.pdf")
