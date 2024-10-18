std::ofstream csvFile;
csvFile.open("flow_stats.csv", std::ios::out | std::ios::app);  // Open file in append mode

fm->CheckForLostPackets(); 
Ptr<Ipv4FlowClassifier> c = DynamicCast<Ipv4FlowClassifier>(fh.GetClassifier()); 
std::map<FlowId, FlowMonitor::FlowStats> s = fm->GetFlowStats(); 

for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = s.begin(); i != s.end(); ++i) { 
    uint32_t flowId = i->first;  // Flow identifier
    uint32_t txPackets = i->second.txPackets;  // Transmitted packets
    uint32_t rxPackets = i->second.rxPackets;  // Received packets
    double throughput = i->second.rxBytes * 8.0 / (simulationTime * 1000000.0);  // Throughput in Mbps
    double meanDelay = i->second.delaySum.GetSeconds() / i->second.rxPackets;  // Mean delay in seconds
    double packetLoss = (i->second.txPackets - i->second.rxPackets) * 100.0 / i->second.txPackets;  // Packet loss percentage
    
    csvFile << tcp_variant<<","<<flowId << "," 
            << txPackets << "," 
            << rxPackets << "," 
            << throughput << "," 
            << meanDelay << "," 
            << packetLoss << "\n";
}
csvFile.close();

