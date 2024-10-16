/*
 * SPDX-License-Identifier: GPL-2.0-only
 */
// Network topology
//
//       n0 )))))))))) n1
//         WiFi 500 Kbps
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "wifi-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

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

NS_LOG_COMPONENT_DEFINE("WifiBulkSendExample");

int
main(int argc, char* argv[])
{
    bool tracing = false;
    uint32_t maxBytes = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send", maxBytes);
    cmd.Parse(argc, argv);

    // Create nodes
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    // Configure WiFi
    NS_LOG_INFO("Configure WiFi.");
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);  // Using 802.11g standard

    // Configure PHY layer
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    
    // Set WiFi rate to match original 500Kbps
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("OfdmRate500KbpsBW20MHz"),
                                "ControlMode", StringValue("OfdmRate500KbpsBW20MHz"));

    // Configure MAC layer
    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("bulk-send-wifi");
    
    // Setup AP (node 0)
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, nodes.Get(0));

    // Setup STA (node 1)
    wifiMac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid));
    NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, nodes.Get(1));

    // Combine devices
    NetDeviceContainer devices(apDevice, staDevice);

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
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    // Create applications
    NS_LOG_INFO("Create Applications.");
    uint16_t port = 9;

    // Configure bulk send application
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(i.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(10.0));

    // Configure packet sink application
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    // Enable tracing
    if (tracing)
    {
        AsciiTraceHelper ascii;
        wifiPhy.EnableAsciiAll(ascii.CreateFileStream("wifi-bulk-send.tr"));
        wifiPhy.EnablePcapAll("wifi-bulk-send", false);
    }

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
