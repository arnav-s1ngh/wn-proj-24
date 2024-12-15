#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
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
 
#include <algorithm>
#include <functional>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WN_Assign-3");

// Response Time
int packet_cnt=0;
double resp_time=0;

// Collision and reception counters
int totalreceptions=0;
int totalcollisions=0;

// Track request and response times
std::map<uint32_t,Time> packetsendtimes; // Map packet ID to send time

void PacketReception(std::string context,Ptr<const Packet> packet,double snr,WifiMode mode,WifiPreamble preamble) {
    totalreceptions++;
}

void PacketCollision(std::string context,Ptr<const Packet> packet,double snr) {
    totalcollisions++;
}

// Callback for PHY transmission
void PhyTxTrace(std::string context,Ptr<const Packet> packet,WifiMode mode,WifiPreamble preamble,uint8_t txPower) {
    // Store the transmission time for each packet based on its ID
    packetsendtimes[packet->GetUid()]=Simulator::Now();
}

// Callback for PHY reception
void PhyRxOkTrace(std::string context,Ptr<const Packet> packet,double snr,WifiMode mode,WifiPreamble preamble) {
    uint32_t packetid=packet->GetUid();
    
    if (packetsendtimes.find(packetid) != packetsendtimes.end()) {
        // Calculate the response time
        Time sendtime=packetsendtimes[packetid];
        Time receivetime=Simulator::Now();
        Time responsetime=receivetime-sendtime;
        
        //std::cout << "Packet ID: " << packetid << ",Response time: " << responsetime.GetMicroSeconds() << " microseconds" << std::endl;
        resp_time+=responsetime.GetMicroSeconds();
        packet_cnt+=1;
        
        // Remove the entry to keep the map clean
        packetsendtimes.erase(packetid);
    }
}

void simulation(int zis, std::string tcp_variant) {
    //Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    //Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));
    //Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    int nw=20; // Number of STAs in the WiFi network
    NodeContainer sta_nodes;
    sta_nodes.Create(nw);
    
    NodeContainer ap_node; // AP
    ap_node.Create(1);
    
    NodeContainer server_node; // Server
    server_node.Create(1);

    // Establish Server-AP P2P Connection
    PointToPointHelper p2pconn;
    p2pconn.SetDeviceAttribute("DataRate",StringValue("1000Mbps"));
    p2pconn.SetChannelAttribute("Delay",StringValue("100ms"));
    NodeContainer p2p_nodes;
    NetDeviceContainer p2p_device;
    p2p_nodes.Add(ap_node);
    p2p_nodes.Add(server_node);
    p2p_device=p2pconn.Install(p2p_nodes);
    
    // Wifi setup
    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    //Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel>(); 
    //spectrumChannel->AddPropagationLossModel(lossModel);
    //Ptr<NakagamiPropagationLossModel> fadingModel = CreateObject<NakagamiPropagationLossModel> ();
    //spectrumChannel->AddPropagationLossModel (fadingModel);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    StringValue value("{38, 40, BAND_5GHZ, 0}");

    SpectrumWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(spectrumChannel);
    
    WifiMacHelper mac;
    Ssid ssid=Ssid("ns-3-ssid");

    // STAs
    phy.Set("ChannelSettings", value);
    mac.SetType("ns3::StaWifiMac","Ssid",SsidValue(ssid));
    NetDeviceContainer sta_devices=wifi.Install(phy,mac,sta_nodes);

    // AP
    mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler","EnableUlOfdma",BooleanValue(true),"EnableBsrp",BooleanValue(true));
    mac.SetType("ns3::ApWifiMac","Ssid",SsidValue(ssid));
    NetDeviceContainer ap_device=wifi.Install(phy,mac,ap_node.Get(0));

    
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator","MinX",DoubleValue(0.0),"MinY",DoubleValue(0.0),"DeltaX",DoubleValue(1),"DeltaY",DoubleValue(1.0),"GridWidth",UintegerValue(20),"LayoutType",StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ap_node);
    mobility.Install(sta_nodes);
    mobility.Install(server_node);
    
    // Install internet stack
    InternetStackHelper internet;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp_variant));
    internet.Install(ap_node);
    internet.Install(sta_nodes);
    internet.Install(server_node);
    
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer p2p_interface;
    p2p_interface=address.Assign(p2p_device);
 
    address.SetBase("10.1.2.0","255.255.255.0");
    Ipv4InterfaceContainer ap_interface;
    Ipv4InterfaceContainer sta_interface;
    ap_interface=address.Assign(ap_device);
    sta_interface=address.Assign(sta_devices);
    
    int bs_port=9;
    int oo_port=40;
    
    for (int i=0; i < 20; i++) {
        BulkSendHelper source("ns3::TcpSocketFactory",InetSocketAddress(sta_interface.GetAddress(i),bs_port));
        source.SetAttribute("MaxBytes",UintegerValue(zis * 1024 * 1024));
        ApplicationContainer sourceApps=source.Install(ap_node.Get(0));
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(10.0));
        
        PacketSinkHelper sink("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),bs_port));
        ApplicationContainer sinkApps=sink.Install(sta_nodes.Get(i));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(10.0));
        bs_port+=1;
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); 
    p2pconn.EnablePcapAll("wired_capture",false);
    phy.EnablePcapAll("wireless_capture",false);

    // Connect to reception and collision events
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/RxOk",MakeCallback(&PacketReception));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/RxError",MakeCallback(&PacketCollision));
    
    // Connect to PHY Tx and RxOk events for response time calculation
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/Tx",MakeCallback(&PhyTxTrace));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/RxOk",MakeCallback(&PhyRxOkTrace));
    
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    
    // Calculate collision percentage
    double collisionpercentage=(totalcollisions / (double)(totalreceptions+totalcollisions))*100;
    std::cout << "Average Throughput: " << totalreceptions/nw << "packets" <<std::endl;
    std::cout << "Collision Percentage: " << collisionpercentage << "%" << std::endl;
    std::cout<<"Average Response Time: "<<resp_time/packet_cnt<<" microseconds"<<std::endl;

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
        for(int j=0;j<7;j++){
        
            std::cout<<tcpvars[i]<<std::endl;
            simulation(datasizes[j],tcpvars[i]);
            }
        }
    std::cout<<"Completed"<<std::endl;
   return 0;
}
