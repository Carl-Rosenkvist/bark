import time
from binaryreader import BinaryReader, CollectorAccessor
import numpy as np 
import pandas as pd
import matplotlib.pyplot as plt

start = time.time()
file_path = "/Users/carl/Phd/check_rivet/data/particles_binary.bin"
quantities = ["p0", "px", "py", "pz", "pdgid", "charge"]

# Read binary file
accessor = CollectorAccessor()
reader = BinaryReader(file_path, quantities, accessor)
reader.read()
event_sizes = accessor.get_event_sizes()

# Build data dictionary
events = np.repeat(np.arange(len(event_sizes)), event_sizes)
data = {
    "event": events,
    **{q: (accessor.get_int_array(q) if q in ["pdgid", "charge"] else accessor.get_double_array(q))
       for q in quantities}
}

# Compute pseudorapidity
pabs = np.sqrt(data["px"]**2 + data["py"]**2 + data["pz"]**2)
data["eta"] = 0.5 * np.log((pabs + data["pz"]) / (pabs - data["pz"]))

# Create DataFrame and filter charged particles
df = pd.DataFrame(data)
charged_df = df[df["charge"] != 0]

# Multiplicity and η count in |η| < 0.5
mult = charged_df.groupby("event").size()
eta_range = charged_df[(charged_df["eta"] >= -0.5) & (charged_df["eta"] <= 0.5)]
eta_counts = eta_range.groupby("event").size().reindex(mult.index, fill_value=0)

# Compute centrality bins
centrality_percentiles = np.arange(0, 110, 10)
bin_edges = np.percentile(mult, 100 - centrality_percentiles)
centrality_ids = np.digitize(mult, bins=bin_edges)
centrality_labels = centrality_percentiles[:-1]
centrality = [centrality_labels[i - 1] if i > 0 else centrality_labels[0] for i in centrality_ids]

# Summary and average η count per bin
summary = pd.DataFrame({"eta_counts": eta_counts, "centrality": centrality})
avg_eta = summary.groupby("centrality")["eta_counts"].mean()

# Plot
plt.scatter(avg_eta.index, avg_eta.values, s=60)
plt.xlabel('Centrality Bin (%)', fontsize=16)
plt.ylabel('$dN/d\eta$', fontsize=16)
plt.title('AuAu at $\sqrt{s} = 200$ GeV')
plt.grid(True)
plt.tight_layout()
plt.show()

print(f"Elapsed time: {1000 * (time.time() - start):.2f} ms")
