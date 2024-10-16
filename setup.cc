#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-helper.h"
#include <ctime>
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WN_Assignment-2");
int main(int argc, char* argv[]) {
   int nw=5;
   NodeContainer ap_node;
   
   ap_node.Create(1); // AP
   
   NodeContainer sta_nodes; // Station Nodes
   
   sta_nodes.Create(nw); // WiFi Nodes
   
   YansWifiChannelHelper channel=YansWifiChannelHelper::Default(); // WiFi Setup
   YansWifiPhyHelper phy;
   
   phy.SetChannel(channel.Create());
   WifiMacHelper mac;
   
   Ssid ssid=Ssid("ns-3-ssid");
   WifiHelper wifi;
   // [W.O. const MCS Rates] wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager","DataMode",StringValue("HtMcs7"),"ControlMode",StringValue("HtMcs0"));
   
   NetDeviceContainer sta_devices;
   NetDeviceContainer ap_device;
   
   
   mac.SetType("ns3::StaWifiMac","Ssid",SsidValue(ssid));
   sta_devices=wifi.Install(phy,mac,sta_nodes);
   
   mac.SetType("ns3::ApWifiMac","Ssid",SsidValue(ssid));
   ap_device=wifi.Install(phy,mac,ap_node.Get(0));


   MobilityHelper mobility;
   mobility.SetPositionAllocator("ns3::GridPositionAllocator","MinX", DoubleValue(0.0),"MinY", DoubleValue(0.0),"DeltaX", DoubleValue(5.0),"DeltaY", DoubleValue(10.0),"GridWidth", UintegerValue(3),"LayoutType", StringValue("RowFirst"));
   
   mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
   mobility.Install(ap_node);
   
   mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel","Bounds",RectangleValue(Rectangle(-50,50,-50,50)));
   mobility.Install(sta_nodes);


   InternetStackHelper stack;
   stack.Install(ap_node);
   stack.Install(sta_nodes);


   Ipv4AddressHelper address;
   address.SetBase("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer ap_interface = address.Assign(ap_device);
   Ipv4InterfaceContainer sta_interfaces = address.Assign(sta_devices);
   
   
   for(int cl_num=0;cl_num<nw;cl_num++){
      std::cout<<"Running Client "<<cl_num+1<<std::endl;
      BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (sta_interfaces.GetAddress (cl_num), 9+cl_num));
      source.SetAttribute ("MaxBytes", UintegerValue(5*1024*1024));
      ApplicationContainer sourceApps = source.Install (ap_node.Get (0));
      sourceApps.Start (Seconds (0.0));
      sourceApps.Stop (Seconds (50.0));
   }
   
   
   Ipv4GlobalRoutingHelper::PopulateRoutingTables(); 
   Simulator::Stop(Seconds(50.0));
   Time begin=Simulator::Now();
   //phy.EnablePcapAll ("sim1_pcap",false);
   Simulator::Run();
   Time end=Simulator::Now();
   Simulator::Destroy();
   std::cout<< "Simulation Time" << end-begin<< std::endl;
   return 0;
}
