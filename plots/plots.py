import matplotlib.pyplot as plt
import numpy as np

# Data from benchmark logs
datasets = ["CA-CondMat", "CA-HepTh", "Email-Enron", "Wiki-Vote", "email-Eu-core", "facebook"]
k_values = [1, 2, 4, 8, 16, 32, 64]

# 1. Runtime Efficiency (k=64 vs RV)
kbfs_times = [9.10, 3.46, 11.48, 4.12, 0.80, 1.64]
rv_times = [468.03, 100.48, 748.06, 99.67, 4.89, 39.60]

# 2. Accuracy Convergence (Estimates over k)
# Format: {Dataset: [estimates for k=1, 2, 4, 8, 16, 32, 64]}
acc_data = {
    "CA-CondMat": [15, 15, 15, 15, 15, 15, 15],
    "Email-Enron": [13, 13, 13, 13, 13, 13, 13],
    "email-Eu-core": [6, 6, 7, 7, 7, 7, 7],
    "facebook": [8, 8, 8, 8, 8, 8, 8]
}

# 3. Hardware Scaling (Runtime over k)
scale_data = {
    "Email-Enron": [2.37, 3.16, 5.32, 6.50, 10.26, 10.64, 11.48],
    "CA-CondMat": [1.64, 2.67, 3.94, 5.11, 5.93, 7.96, 9.10],
    "Wiki-Vote": [0.91, 1.36, 1.97, 2.59, 3.14, 3.59, 4.12]
}

# 4. Error Margin
ground_truth = [14, 17, 11, 7, 7, 8]
kbfs_final = [15, 18, 13, 7, 7, 8]

# --- PLOTTING ---

# Graph 1: Runtime Efficiency (Log Scale)
plt.figure(figsize=(10, 6))
x = np.arange(len(datasets))
plt.bar(x - 0.2, kbfs_times, 0.4, label='k-BFS (k=64)', color='skyblue')
plt.bar(x + 0.2, rv_times, 0.4, label='RV', color='salmon')
plt.yscale('log')
plt.xticks(x, datasets, rotation=15)
plt.ylabel('Time (ms) - Log Scale')
plt.title('Runtime Efficiency: k-BFS vs RV')
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()

# Graph 2: Accuracy Convergence
plt.figure(figsize=(10, 6))
for label, values in acc_data.items():
    plt.plot(k_values, values, marker='o', label=label)
plt.xlabel('k (Number of parallel searches)')
plt.ylabel('Diameter Estimate')
plt.title('Accuracy Convergence: The "Flatline" Effect')
plt.legend()
plt.grid(True, alpha=0.3)

# Graph 3: Hardware Scaling
plt.figure(figsize=(10, 6))
for label, values in scale_data.items():
    plt.plot(k_values, values, marker='s', label=label)
plt.xlabel('k (Number of parallel searches)')
plt.ylabel('Execution Time (ms)')
plt.title('Hardware Scaling: Bit-Vector Parallelism Advantage')
plt.legend()
plt.grid(True, alpha=0.3)

# Graph 4: Error Margin
plt.figure(figsize=(10, 6))
plt.bar(x, ground_truth, alpha=0.5, label='Known Ground Truth', color='gray')
plt.bar(x, kbfs_final, alpha=0.8, label='k-BFS (k=64) Estimate', color='teal', width=0.4)
plt.xticks(x, datasets, rotation=15)
plt.ylabel('Diameter')
plt.title('Error Margin: k-BFS Estimate vs. Ground Truth')
plt.legend()
plt.tight_layout()

plt.show()
