import time
from binaryreader import BinaryReader, CollectorAccessor
import numpy as np 
import matplotlib.pyplot as plt

start = time.time()

quantities = ["mass", "p0", "pz", "pdgid", "ncoll"]
accessor = CollectorAccessor()
reader = BinaryReader("/home/carl/Phd/systemSize/runs/mod_smash_158AGeV/out-0/particles_binary.bin", quantities, accessor)
reader.read()

elapsed = time.time() - start
print(f"Elapsed time: {elapsed * 1000:.2f} ms")
pdg = accessor.get_int_array("pdgid")
unwounded = np.sum(accessor.get_int_array("ncoll")==0)/100
mask = np.where((pdg == 3212) | (pdg == 3122))[0]
# Extract arrays
pz = accessor.get_double_array("pz")[mask]
p0 = accessor.get_double_array("p0")[mask]

# Compute rapidity
y = 0.5 * np.log((p0 + pz) / (p0 - pz))

# Histogram setup
n_bins = 50
y_range = (-5, 5)
counts, bins = np.histogram(y, bins=n_bins, range=y_range)

# Bin width
bin_width = bins[1] - bins[0]

# dN/dy = counts / bin width
dndy = counts / (bin_width*100)
bin_centers = 0.5 * (bins[:-1] + bins[1:])

# Plot
plt.plot(bin_centers, dndy, drawstyle='steps-mid')
plt.xlabel('Rapidity $y$')
plt.ylabel('$\\frac{dN}{dy}$')
plt.title('Rapidity Distribution')
plt.grid(True)
plt.tight_layout()
plt.show()
