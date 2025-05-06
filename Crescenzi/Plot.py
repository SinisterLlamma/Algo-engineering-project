# plot_ifub2_avg.py
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv')  
# times are ~0.01s, convert to ms for readability
df['time_ms'] = df['avg_time_s'] * 1000

plt.figure(figsize=(6,4))
plt.bar(df['strategy'], df['time_ms'])
plt.xticks(df['strategy'])
plt.xlabel('Strategy')
plt.ylabel('Avg. Runtime (ms)')
plt.title('iFUB2: Average Runtime by Strategy')
plt.ylim(0, df['time_ms'].max()*1.2)
plt.tight_layout()
plt.show()
