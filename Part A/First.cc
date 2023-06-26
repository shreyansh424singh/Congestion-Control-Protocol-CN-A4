/*
              Node1
              / 
             / 10.10.1.0/24
            /                                                       
      Node3   
            \
             \ 10.10.2.0/24
              \
              Node2

*/
 
#include <fstream>
#include <string>
#include <string>
#include <cassert>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/double.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include <bits/stdc++.h>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("jcomp");

//MyApp Application
class MyApp : public Application{

        public:
                MyApp ();
                virtual ~MyApp();
                void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

        private:
                virtual void StartApplication (void);
                virtual void StopApplication (void);
                void ScheduleTx (void);
                void SendPacket (void);

                Ptr<Socket>     m_socket;
                Address         m_peer;
                uint32_t        m_packetSize;
                uint32_t        m_nPackets;
                DataRate        m_dataRate;
                EventId         m_sendEvent;
                bool            m_running;
                uint32_t        m_packetsSent;
};

//Constructor and Destructor
MyApp::MyApp ()
        : m_socket (0),
        m_peer (),
        m_packetSize (0),
        m_nPackets (0),
        m_dataRate (0),
        m_sendEvent (),
        m_running (false),
        m_packetsSent (0)
{
}

MyApp::~MyApp(){
        m_socket = 0;
}

void MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate){
        m_socket = socket;
        m_peer = address;
        m_packetSize = packetSize;
        m_nPackets = nPackets;
        m_dataRate = dataRate;
}

//Initializing member variables
void MyApp::StartApplication (void){
        m_running = true;
        m_packetsSent = 0;
        m_socket->Bind ();
        m_socket->Connect (m_peer);
        SendPacket ();
}

//Stop creating simulation events
void MyApp::StopApplication (void){
        m_running = false;
        if (m_sendEvent.IsRunning ()){
                Simulator::Cancel (m_sendEvent);
        }
        if (m_socket){
                m_socket->Close ();
        }
}

//Recall that StartApplication called SendPacket to start the chain of events that describes the Application behavior
void MyApp::SendPacket (void){
        Ptr<Packet> packet = Create<Packet> (m_packetSize);
        m_socket->Send (packet);
        if (++m_packetsSent < m_nPackets){
                ScheduleTx ();
        }
}

//Call ScheduleTx to schedule another transmit event (a SendPacket) until the Application decides it has sent enough.
void MyApp::ScheduleTx (void){
        if (m_running){
                double time = Simulator::Now().GetSeconds();
                // change the data rate to 500Kbps for UDP socket after 30 seconds.
                if(time>=30.0 && m_socket->GetSocketType() == 2){
                        m_dataRate = DataRate("500Kbps");
                }
                Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate())));
                m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
        }
}

static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd){
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newCwnd-oldCwnd << " " << newCwnd << std::endl;
}

int c=0;

static void RxDrop(Ptr<const Packet> p){
  c++;
  NS_LOG_UNCOND("RxDrop at " << Simulator::Now().GetSeconds());
}


int main (int argc, char *argv[]){
    uint32_t packetSize=3000;
    uint32_t nPackets=100000;

    std::string tcp_t;

    CommandLine cmd;
    cmd.AddValue ("tcp", "turn on log components", tcp_t);
    cmd.Parse(argc,argv);

    std::string tcp_type = "ns3::" + tcp_t;
    std::cout<<tcp_type<<endl;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",StringValue(tcp_type));

    NodeContainer nodes;
    nodes.Create (3);
    NodeContainer nodes13 = NodeContainer (nodes.Get (0), nodes.Get (2));
    NodeContainer nodes23 = NodeContainer (nodes.Get (1), nodes.Get (2));

    InternetStackHelper internet;
    internet.Install (nodes);

    PointToPointHelper pointToPoint1;
    pointToPoint1.SetDeviceAttribute ("DataRate", StringValue("10Mbps"));
    pointToPoint1.SetChannelAttribute ("Delay", StringValue ("3ms"));
    NetDeviceContainer device13 = pointToPoint1.Install (nodes13);

    PointToPointHelper pointToPoint2;
    pointToPoint2.SetDeviceAttribute ("DataRate", StringValue("9Mbps"));
    pointToPoint2.SetChannelAttribute ("Delay", StringValue ("3ms"));
    NetDeviceContainer device23 = pointToPoint2.Install (nodes23);


    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    device13.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    device23.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.10.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface13 = ipv4.Assign (device13);
    ipv4.SetBase ("10.10.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface23 = ipv4.Assign (device23);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    //Set up TCP server connection
    uint16_t port1 = 8000;
    ApplicationContainer sinkApp;
    Address sinkLocalAddress(InetSocketAddress (Ipv4Address::GetAny (), port1));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

    sinkApp.Add(sinkHelper.Install(nodes13.Get(1)));
    sinkApp.Start (Seconds (0.5));
    sinkApp.Stop (Seconds (30.5));

    uint16_t port2 = 8001;
    ApplicationContainer sinkApp1;
    Address sinkLocalAddress1(InetSocketAddress (Ipv4Address::GetAny (), port2));
    PacketSinkHelper sinkHelper1 ("ns3::TcpSocketFactory", sinkLocalAddress1);

    sinkApp1.Add(sinkHelper1.Install(nodes13.Get(1)));
    sinkApp1.Start (Seconds (0.5));
    sinkApp1.Stop (Seconds (30.5));

    uint16_t port3 = 8002;
    ApplicationContainer sinkApp2;
    Address sinkLocalAddress2(InetSocketAddress (Ipv4Address::GetAny (), port3));
    PacketSinkHelper sinkHelper2 ("ns3::TcpSocketFactory", sinkLocalAddress2);

    sinkApp2.Add(sinkHelper2.Install(nodes23.Get(1)));
    sinkApp2.Start (Seconds (0.5));
    sinkApp2.Stop (Seconds (30.5));

    //Create the socket and connect the Trace source
    Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());

    //Create object of MyApp
    Ptr<MyApp> clientApp1 = CreateObject<MyApp>();
    Ptr<MyApp> clientApp2 = CreateObject<MyApp>();
    Ptr<MyApp> clientApp3 = CreateObject<MyApp>();

    //Make connections
    Ipv4Address serverAddress1 = interface13.GetAddress (1);
    Address remoteAddress1 (InetSocketAddress (serverAddress1, port1));

    Ipv4Address serverAddress2 = interface13.GetAddress (1);
    Address remoteAddress2 (InetSocketAddress (serverAddress2, port2));

    Ipv4Address serverAddress3 = interface23.GetAddress (1);
    Address remoteAddress3 (InetSocketAddress (serverAddress3, port3));

    //Bind socket with application and connect to server
    clientApp1->Setup(ns3TcpSocket1, remoteAddress1, packetSize, nPackets, DataRate("1.5Mbps"));
    clientApp2->Setup(ns3TcpSocket2, remoteAddress2, packetSize, nPackets, DataRate("1.5Mbps"));
    clientApp3->Setup(ns3TcpSocket3, remoteAddress3, packetSize, nPackets, DataRate("1.5Mbps"));

    //Install application to Client
    nodes.Get (0)->AddApplication(clientApp1);
    nodes.Get (0)->AddApplication(clientApp2);
    nodes.Get (1)->AddApplication(clientApp3);

    //Set start and stop time
    clientApp1->SetStartTime(Seconds(1.0));
    clientApp1->SetStopTime(Seconds(20.0));
    clientApp2->SetStartTime(Seconds(5.0));
    clientApp2->SetStopTime(Seconds(25.0));
    clientApp3->SetStartTime(Seconds(15.0));
    clientApp3->SetStopTime(Seconds(30.0));

    // pcap enable
    // pointToPoint.EnablePcapAll ("task1");

    string wnd_file1 = tcp_t + "_N1_1_Source.cwnd";
    string wnd_file2 = tcp_t + "_N1_2_Source.cwnd";
    string wnd_file3 = tcp_t + "_N2_Source.cwnd";
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (wnd_file1);
    Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (wnd_file2);
    Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream (wnd_file3);
    ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
    ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));
    ns3TcpSocket3->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream3));

    device13.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
    device23.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

    Simulator::Stop (Seconds(30));
    Simulator::Run ();
    Simulator::Destroy ();

    cout<<"No of packet drop: "<<c<<endl;
}