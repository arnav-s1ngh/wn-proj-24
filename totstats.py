import numpy as np
import pandas as pd
import json
from collections import defaultdict
import matplotlib.pyplot as plt
import seaborn as sns
df=pd.read_csv('trial1_flow_stats.csv')
speeds=[6, 9, 12, 18, 24, 36, 48, 54] * 32
df['speed']=speeds

tcp_variants = df['tcp_variant'].unique()

# Create a figure with 3 subplots (one for each metric)
fig, axes = plt.subplots(3, 1, figsize=(10, 18))

# Define colors and markers for better differentiation
colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']
markers = ['o', 's', '^', 'D', 'x', 'P', '*']

# Plot throughput vs speed for different TCP variants
for i, tcp in enumerate(tcp_variants):
    df_variant = df[df['tcp_variant'] == tcp]
    axes[0].plot(df_variant['speed'], df_variant['throughput'], label=tcp, color=colors[i % len(colors)], marker=markers[i % len(markers)])
axes[0].set_title('Throughput vs Speed')
axes[0].set_xlabel('Speed (Mbps)')
axes[0].set_ylabel('Throughput (Mbps)')
axes[0].legend(title='TCP Variant')

# Plot meandelay vs speed for different TCP variants
for i, tcp in enumerate(tcp_variants):
    df_variant = df[df['tcp_variant'] == tcp]
    axes[1].plot(df_variant['speed'], df_variant['meandelay'], label=tcp, color=colors[i % len(colors)], marker=markers[i % len(markers)])
axes[1].set_title('Mean Delay vs Speed')
axes[1].set_xlabel('Speed (Mbps)')
axes[1].set_ylabel('Mean Delay (ms)')
axes[1].legend(title='TCP Variant')

# Plot packetloss vs speed for different TCP variants
for i, tcp in enumerate(tcp_variants):
    df_variant = df[df['tcp_variant'] == tcp]
    axes[2].plot(df_variant['speed'], df_variant['packetloss'], label=tcp, color=colors[i % len(colors)], marker=markers[i % len(markers)])
axes[2].set_title('Packet Loss vs Speed')
axes[2].set_xlabel('Speed (Mbps)')
axes[2].set_ylabel('Packet Loss (%)')
axes[2].legend(title='TCP Variant')

# Adjust layout for better spacing
plt.tight_layout()

# Show the plot
plt.show()
