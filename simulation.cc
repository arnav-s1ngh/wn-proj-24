// Network topology
//                           WiFi
//                            AP
//       n2<------------------n0------------------>n1
//             3GPPHTTP        |      BulkSend       
//                             |
//                             | BulkSend+3GPPHTTP
//                             |
//                             n3
//
//
//   Flow from n0 to n1 using BulkSendApplication and ThreeGppHttpClientHelper
//    A Fair bit of code has been borrowed from https://www.nsnam.org/doxygen/dc/d86/three-gpp-http-example_8cc_source.html
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


int main(int argc, char* argv[]){
    uint32_t maxBytes = 1000*1000*5;



    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll (LOG_PREFIX_FUNC);
    // LogComponentEnable ("ThreeGppHttpClient", LOG_INFO);
    /// LogComponentEnable ("ThreeGppHttpServer", LOG_INFO);
    LogComponentEnable("Project_Mid_Review", LOG_INFO);
    
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
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
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
