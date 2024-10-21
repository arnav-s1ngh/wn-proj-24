import pandas as pd
import matplotlib.pyplot as plt
a=pd.read_csv('trial1_flow_stats.csv')
b=[6,9,12,18,24,36,48,54]*32
a['speed']=b
c=a['tcp_variant'].unique()
d,e=plt.subplots(3,1,figsize=(12,20))
f=['b','g','r','c','m','y','k']
g=['o','s','^','D','x','P','*']
for h,i in enumerate(c):
    j=a[a['tcp_variant']==i]
    e[0].plot(j['speed'],j['throughput'],label=i,color=f[h%len(f)],marker=g[h%len(g)])
    e[0].set_title('Throughput vs Speed')
    e[0].set_xlabel('Speed (Mbps)')
    e[0].set_ylabel('Throughput (Mbps)')
    e[0].legend(title='TCP Variant')
for h,i in enumerate(c):
    j=a[a['tcp_variant']==i]
    e[1].plot(j['speed'],j['meandelay'],label=i,color=f[h%len(f)],marker=g[h%len(g)])
    e[1].set_title('Mean Delay vs Speed')
    e[1].set_xlabel('Speed (Mbps)')
    e[1].set_ylabel('Mean Delay (ms)')
    e[1].legend(title='TCP Variant')
for h,i in enumerate(c):
    j=a[a['tcp_variant']==i]
    e[2].plot(j['speed'],j['packetloss'],label=i,color=f[h%len(f)],marker=g[h%len(g)])
    e[2].set_title('Packet Loss vs Speed')
    e[2].set_xlabel('Speed (Mbps)')
    e[2].set_ylabel('Packet Loss (%)')
    e[2].legend(title='TCP Variant')
plt.show()
