# python3
import sys
import pandas as pd
import matplotlib.pyplot as plt
import os

if len(sys.argv) < 2:
    print("usage: plot_results.py <csv_file>\n")
    sys.exit(1)

csv_file = sys.argv[1]
df = pd.read_csv(csv_file)

queues = df['queue'].unique()

out_dir = "plots"
os.makedirs(out_dir, exist_ok=True)

plt.figure()
for q in queues:
    qdf = df[df['queue'] == q].sort_values('threads')
    plt.plot(qdf['threads'], qdf['seconds'], marker='o', label=q)
plt.xlabel('Threads')
plt.ylabel('Seconds')
plt.legend()
plt.grid(True)
plt.tight_layout()
fname = os.path.join(out_dir, "result.png")
plt.savefig(fname)
print("saved", fname)