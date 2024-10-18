#include <fstream>  // Required for file handling

// Open or create a CSV file
std::ofstream csvFile;
csvFile.open("flow_stats.csv", std::ios::out | std::ios::app);  // Open file in append mode

// Check if the file was opened successfully
if (csvFile.is_open()) {
    // Write header if the file is empty
    csvFile << "FlowId,TxPackets,RxPackets,Throughput(Mbps),MeanDelay(s),PacketLoss(%)\n";

    // Iterate through flow statistics and store values
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
        
        // Write the collected data into the CSV file
        csvFile << flowId << "," 
                << txPackets << "," 
                << rxPackets << "," 
                << throughput << "," 
                << meanDelay << "," 
                << packetLoss << "\n";
    }

    csvFile.close();  // Close the file after writing
} else {
    std::cerr << "Unable to open file for writing!" << std::endl;
}
