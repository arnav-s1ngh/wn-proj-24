// Network topology
//
//       n0------------------>n1
//         WiFi
//
//   Flow from n0 to n1 using BulkSendApplication and ThreeGppHttpClientHelper

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <fstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Project_Mid_Review");

int main(int argc, char* argv[]){
    uint32_t maxBytes = 1000*1000*5;
    NodeContainer nodes;
    nodes.Create(3);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g); 

    YansWifiPhyHelper phy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    phy.SetChannel(wifiChannel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    
    //AP
    mac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, nodes.Get(0));

    //STA
    mac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid));
    NetDeviceContainer staDevice = wifi.Install(phy, mac, nodes.Get(1));
    NetDeviceContainer staDevice2 = wifi.Install(phy, mac, nodes.Get(2));

    // Combine devices

    // Configure mobility
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue(0.0),
                                 "MinY", DoubleValue(0.0),
                                 "DeltaX", DoubleValue(5.0),
                                 "DeltaY", DoubleValue(0.0),
                                 "GridWidth", UintegerValue(2),
                                 "LayoutType", StringValue("RowFirst"));
    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Install internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ap_interface = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer sta_interface = ipv4.Assign(staDevice);
    Ipv4InterfaceContainer sta_interface2 = ipv4.Assign(staDevice2);

    // Create applications
    uint16_t tcp_port = 9;

    // Configure bulk send application
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(sta_interface.GetAddress(0), tcp_port));
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(10.0));

    // Configure packet sink application
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcp_port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));
    
    //3GPPHTTP
    ThreeGppHttpServerHelper serverHelper(sta_interface2.GetAddress(0));
    ApplicationContainer serverApps = serverHelper.Install(nodes.Get(0));
    Ptr<ThreeGppHttpServer> httpServer = serverApps.Get(0)->GetObject<ThreeGppHttpServer>();

    // Configure server variables
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB
    
    // Setup HTTP client
    ThreeGppHttpClientHelper clientHelper(ap_interface.GetAddress(0));
    ApplicationContainer clientApps = clientHelper.Install(nodes.Get(2));
    Ptr<ThreeGppHttpClient> httpClient = clientApps.Get(0)->GetObject<ThreeGppHttpClient>();
    
    clientApps.Stop(Seconds(10.0));

    AsciiTraceHelper ascii;
    phy.EnableAsciiAll(ascii.CreateFileStream("fuck.tr"));
    phy.EnablePcapAll("fuck", false);

    // Run simulation
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Done.");
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;

    return 0;
}
