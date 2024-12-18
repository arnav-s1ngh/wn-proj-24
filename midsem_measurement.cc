// WORKS!!
// Network topology
//                           WiFi
//                            AP
//       n2<------------------n0------------------>n1
//             3GPPHTTP        |      BulkSend       
//                             |
//                             | BulkSend+3GPPHTTP
//                             |
//                             v
//                             n3
//
//
//   
//    A Fair bit of code has been borrowed from https://www.nsnam.org/doxygen/dc/d86/three-gpp-http-example_8cc_source.html
#include <iostream>
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/error-model.h"
#include <fstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Project_Mid_Review");

void

ServerConnectionEstablished(Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
    NS_LOG_INFO("Client has established a connection to the server.");
}

void

MainObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated a main object of " << size << " bytes.");
}

void

EmbeddedObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated an embedded object of " << size << " bytes.");
}

void

ServerTx(Ptr<const Packet> packet)
{
    NS_LOG_INFO("Server sent a packet of " << packet->GetSize() << " bytes.");
}

void

ClientRx(Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Client received a packet of " << packet->GetSize() << " bytes from " << address);
}

void

ClientMainObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::MAIN_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received a main object of " << p->GetSize()
                                                                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse a main object. ");
    }
}

void

ClientEmbeddedObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received an embedded object of " << p->GetSize()
                                                                              << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse an embedded object. ");
    }
}

void

ClientPageReceived(Ptr<const ThreeGppHttpClient> client,
                   const Time& time,
                   uint32_t numObjects,
                   uint32_t numBytes)
{
    NS_LOG_INFO("Client " << client << " has received a page that took " << time.As(Time::MS)
                          << " ms to load with " << numObjects << " objects and " << numBytes
                          << " bytes.");
}


void simulation(std::string data_rate, std::string tcp_variant ){
    
    uint32_t maxBytes = 1000*1000*5;


    LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll (LOG_PREFIX_FUNC);
    // LogComponentEnable ("ThreeGppHttpClient", LOG_INFO);
    /// LogComponentEnable ("ThreeGppHttpServer", LOG_INFO);
    LogComponentEnable("Project_Mid_Review", LOG_INFO);
    
    NodeContainer nodes;
    nodes.Create(4);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager","DataMode",StringValue(data_rate));
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
    NetDeviceContainer staDevice3 = wifi.Install(phy, mac, nodes.Get(3));

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
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp_variant));
    internet.Install(nodes);

    // Assign IP addresses
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ap_interface = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer sta_interface = ipv4.Assign(staDevice);
    Ipv4InterfaceContainer sta_interface2 = ipv4.Assign(staDevice2);
    Ipv4InterfaceContainer sta_interface3 = ipv4.Assign(staDevice3);

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
    
    BulkSendHelper source2("ns3::TcpSocketFactory", InetSocketAddress(sta_interface3.GetAddress(0), tcp_port+1));
    source2.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps2 = source2.Install(nodes.Get(0));
    sourceApps2.Start(Seconds(0.0));
    sourceApps2.Stop(Seconds(10.0));

    PacketSinkHelper sink2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcp_port));
    ApplicationContainer sinkApps2 = sink2.Install(nodes.Get(3));
    sinkApps2.Start(Seconds(0.0));
    sinkApps2.Stop(Seconds(10.0));
    
    //3GPPHTTP
    ThreeGppHttpServerHelper serverHelper(ap_interface.GetAddress(0));
    ApplicationContainer serverApps = serverHelper.Install(nodes.Get(0));
    Ptr<ThreeGppHttpServer> httpServer = serverApps.Get(0)->GetObject<ThreeGppHttpServer>();

    httpServer->TraceConnectWithoutContext("ConnectionEstablished",
                                           MakeCallback(&ServerConnectionEstablished));
    httpServer->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
    httpServer->TraceConnectWithoutContext("EmbeddedObject",
                                           MakeCallback(&EmbeddedObjectGenerated));
    httpServer->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));

    // Configure server variables
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(1024000);  // ~1MB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB
    
    // Setup HTTP client
    ThreeGppHttpClientHelper clientHelper(ap_interface.GetAddress(0));
    ApplicationContainer clientApps = clientHelper.Install(nodes.Get(2));
    Ptr<ThreeGppHttpClient> httpClient = clientApps.Get(0)->GetObject<ThreeGppHttpClient>();

    httpClient->TraceConnectWithoutContext("RxMainObject", MakeCallback(&ClientMainObjectReceived));
    httpClient->TraceConnectWithoutContext("RxEmbeddedObject",
                                           MakeCallback(&ClientEmbeddedObjectReceived));
    httpClient->TraceConnectWithoutContext("Rx", MakeCallback(&ClientRx));
    httpClient->TraceConnectWithoutContext("RxPage", MakeCallback(&ClientPageReceived));
 
    
    clientApps.Stop(Seconds(10.0));
    
    
    //3GPPHTTP V2
    ThreeGppHttpServerHelper serverHelper2(ap_interface.GetAddress(0));
    ApplicationContainer serverApps2 = serverHelper2.Install(nodes.Get(0));
    Ptr<ThreeGppHttpServer> httpServer2 = serverApps2.Get(0)->GetObject<ThreeGppHttpServer>();

    httpServer2->TraceConnectWithoutContext("ConnectionEstablished",
                                           MakeCallback(&ServerConnectionEstablished));
    httpServer2->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
    httpServer2->TraceConnectWithoutContext("EmbeddedObject",
                                           MakeCallback(&EmbeddedObjectGenerated));
    httpServer2->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));

    // Configure server variables
    PointerValue varPtr2;
    httpServer2->GetAttribute("Variables", varPtr2);
    Ptr<ThreeGppHttpVariables> httpVariables2 = varPtr2.Get<ThreeGppHttpVariables>();
    httpVariables2->SetMainObjectSizeMean(1024000);  // 1Mb
    httpVariables2->SetMainObjectSizeStdDev(40960); // 40kB
    
    // Setup HTTP client
    ThreeGppHttpClientHelper clientHelper2(ap_interface.GetAddress(0));
    ApplicationContainer clientApps2 = clientHelper2.Install(nodes.Get(2));
    Ptr<ThreeGppHttpClient> httpClient2 = clientApps2.Get(0)->GetObject<ThreeGppHttpClient>();

    httpClient2->TraceConnectWithoutContext("RxMainObject", MakeCallback(&ClientMainObjectReceived));
    httpClient2->TraceConnectWithoutContext("RxEmbeddedObject",
                                           MakeCallback(&ClientEmbeddedObjectReceived));
    httpClient2->TraceConnectWithoutContext("Rx", MakeCallback(&ClientRx));
    httpClient2->TraceConnectWithoutContext("RxPage", MakeCallback(&ClientPageReceived));
 
    
    clientApps2.Stop(Seconds(10.0));
    
    Ptr<FlowMonitor> fm;
    FlowMonitorHelper fmh;
    fm=fmh.InstallAll();
    

    AsciiTraceHelper ascii;
    phy.EnableAsciiAll(ascii.CreateFileStream("project.tr"));
    phy.EnablePcapAll("project", false);

    // Run simulation
    NS_LOG_INFO("Run Simulation.");
    std::cout<<"crossed"<<std::endl;
    std::ofstream csvFile;
    csvFile.open("trial1_flow_stats.csv", std::ios::out | std::ios::app);  
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
    
        csvFile << tcp_variant<<","<<flowId << "," 
                << txPackets << "," 
                << rxPackets << "," 
                << throughput << "," 
                << meanDelay << "," 
                << packetLoss << "\n";
    }
    
    
    Simulator::Destroy();
    csvFile.close();

    NS_LOG_INFO("Done.");
}

int main(){
    std::vector<std::string> tcpvars = {
        "ns3::TcpIllinois",
        "ns3::TcpWestwoodPlus",
        "ns3::TcpNewReno",
        "ns3::TcpVegas"
    };
    std::vector<std::string> wifirate = {
        "OfdmRate6Mbps", 
        "OfdmRate9Mbps", 
        "OfdmRate12Mbps", 
        "OfdmRate18Mbps", 
        "OfdmRate24Mbps", 
        "OfdmRate36Mbps", 
        "OfdmRate48Mbps", 
        "OfdmRate54Mbps"
    };
    for(int i=0;i<4;i++){
        for(int j=0;j<8;j++){
            simulation(wifirate[j],tcpvars[i]);
            }
    }
    std::cout<<("completed")<<std::endl;

    return 0;
            
}
