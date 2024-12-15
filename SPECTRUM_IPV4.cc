#include <algorithm>
#include <functional>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WN_Assign-3");

void simulation(int zis, std::string tcp_variant) {
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
    p2pconn.EnablePcapAll(tcp_variant+"_"+zis+"_wired_capture",false);
    phy.EnablePcapAll(tcp_variant+"_"+zis+"_wireless_capture",false);
    
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
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
        
            std::cout<<tcpvars[i], datasizes[j]<<std::endl;
            simulation(datasizes[j],tcpvars[i]);
            }
        }
    std::cout<<"Completed"<<std::endl;
   return 0;
}
