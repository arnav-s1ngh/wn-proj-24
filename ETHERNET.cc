#include <algorithm>
#include <functional>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include <fstream>
#include <string>


#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/he-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include "ns3/flow-monitor-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WN_Assign-3");

void simulation(int zis, std::string tcp_variant) {
    NodeContainer csma_nodes;
    csma_nodes.Create(16);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("1000Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("100ms"));

    NetDeviceContainer csma_devices = csma.Install(csma_nodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator", "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX", DoubleValue(1), "DeltaY", DoubleValue(1.0), "GridWidth", UintegerValue(20), "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(csma_nodes);

    InternetStackHelper internet;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp_variant));
    internet.Install(csma_nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csma_interfaces = address.Assign(csma_devices);

    int bs_port = 9;

    for (int i = 1; i < 16; i++) {
        BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(csma_interfaces.GetAddress(i), bs_port));
        source.SetAttribute("MaxBytes", UintegerValue(zis * 1024 * 1024));
        ApplicationContainer sourceApps = source.Install(csma_nodes.Get(0));
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(10.0));

        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), bs_port));
        ApplicationContainer sinkApps = sink.Install(csma_nodes.Get(i));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(10.0));
        bs_port += 1;
    }
    
    Ptr<FlowMonitor> fm;
    FlowMonitorHelper fmh;
    fm=fmh.InstallAll();

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    csma.EnablePcapAll(tcp_variant + "_csma_capture", false);
    
    std::ofstream csvFile;
    csvFile.open("z_csma_flow_stats.csv", std::ios::out | std::ios::app);

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    fm->CheckForLostPackets(); 
    Ptr<Ipv4FlowClassifier> c = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier()); 
    std::map<FlowId, FlowMonitor::FlowStats> s = fm->GetFlowStats(); 
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = s.begin(); i != s.end(); ++i) { 
        uint32_t flowId = i->first;  // Flow identifier
        uint32_t txPackets = i->second.txPackets;  // Transmitted packets
        uint32_t rxPackets = i->second.rxPackets;  // Received packets
        double throughput = i->second.rxBytes * 8.0 / (10000000.0);  // Throughput in Mbps
        double meanDelay = i->second.delaySum.GetSeconds() / i->second.rxPackets;  // Mean delay in seconds
        double packetLoss = (i->second.txPackets - i->second.rxPackets) * 100.0 / i->second.txPackets;  // Packet loss percentage
    
        csvFile << tcp_variant<<","<<zis<<","<<flowId << "," 
                << txPackets << "," 
                << rxPackets << "," 
                << throughput << "," 
                << meanDelay << "," 
                << packetLoss << "\n";
    }
    Simulator::Destroy();
}


int main(){
   std::vector<int> datasizes={5,10,20,50,100,200,500};
   std::vector<std::string> tcpvars = {
        "ns3::TcpIllinois",
        "ns3::TcpWestwoodPlus",
        "ns3::TcpNewReno",
        "ns3::TcpVegas",
        "ns3::TcpCubic"
    };
   for(int i=0;i<5;i++){
        std::cout<<tcpvars[i]<<std::endl;
        for(int j=0;j<7;j++){
            simulation(datasizes[j],tcpvars[i]);
            }
        }
    std::cout<<"Completed"<<std::endl;
   return 0;
}
