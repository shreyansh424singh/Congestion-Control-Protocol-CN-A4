/* -*- Mode:R3++; R3-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (R3) 2016 Universita' di Firenze, Italy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR R1 PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received R1 copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

/* Network topology

      SRC
       |<=== source network
       R1
       | \     all networks have cost 1
       |  \    
       \   R2
        \   \
         \   \
          \   \
           \--R3
               |
               |
               |<=== target network
              DST
*/

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RipSimpleRouting");

void TearDownLink (Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
  nodeA->GetObject<Ipv4> ()->SetDown (interfaceA);
  nodeB->GetObject<Ipv4> ()->SetDown (interfaceB);
}

int main (int argc, char **argv)
{
  bool verbose = false;
  bool printRoutingTables = true;
  bool showPings = false;
  std::string SplitHorizon ("SplitHorizon");

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.AddValue ("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnableAll (LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE));
      LogComponentEnable ("RipSimpleRouting", LOG_LEVEL_INFO);
      LogComponentEnable ("Rip", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv4L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("ArpCache", LOG_LEVEL_ALL);
      LogComponentEnable ("V4Ping", LOG_LEVEL_ALL);
    }

  if (SplitHorizon == "NoSplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::NO_SPLIT_HORIZON));
    }
  else if (SplitHorizon == "SplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::SPLIT_HORIZON));
    }
  else
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::POISON_REVERSE));
    }

  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> src = CreateObject<Node> ();
  Names::Add ("SrcNode", src);
  Ptr<Node> dst = CreateObject<Node> ();
  Names::Add ("DstNode", dst);
  Ptr<Node> R1 = CreateObject<Node> ();
  Names::Add ("RouterA", R1);
  Ptr<Node> R2 = CreateObject<Node> ();
  Names::Add ("RouterB", R2);
  Ptr<Node> R3 = CreateObject<Node> ();
  Names::Add ("RouterC", R3);
  NodeContainer net1 (src, R1);
  NodeContainer net2 (R1, R2);
  NodeContainer net3 (R1, R3);
  NodeContainer net4 (R2, R3);
  NodeContainer net5 (R3, dst);
  NodeContainer routers (R1, R2, R3);
  NodeContainer nodes (src, dst);


  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer ndc1 = csma.Install (net1);
  NetDeviceContainer ndc2 = csma.Install (net2);
  NetDeviceContainer ndc3 = csma.Install (net3);
  NetDeviceContainer ndc4 = csma.Install (net4);
  NetDeviceContainer ndc5 = csma.Install (net5);

  NS_LOG_INFO ("Create IPv4 and routing");
  RipHelper ripRouting;

  // Rule of thumb:
  // Interfaces are added sequentially, starting from 0
  // However, interface 0 is always the loopback...
  ripRouting.ExcludeInterface (R1, 1);
  ripRouting.ExcludeInterface (R3, 3);

  Ipv4ListRoutingHelper listRH;
  listRH.Add (ripRouting, 0);
//  Ipv4StaticRoutingHelper staticRh;
//  listRH.Add (staticRh, 5);

  InternetStackHelper internet;
  internet.SetIpv6StackInstall (false);
  internet.SetRoutingHelper (listRH);
  internet.Install (routers);

  InternetStackHelper internetNodes;
  internetNodes.SetIpv6StackInstall (false);
  internetNodes.Install (nodes);

  // Assign addresses.
  // The source and destination networks have global addresses
  // The "core" network just needs link-local addresses for routing.
  // We assign global addresses to the routers as well to receive
  // ICMPv6 errors.
  NS_LOG_INFO ("Assign IPv4 Addresses.");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic1 = ipv4.Assign (ndc1);

  ipv4.SetBase (Ipv4Address ("10.0.1.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic2 = ipv4.Assign (ndc2);

  ipv4.SetBase (Ipv4Address ("10.0.2.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic3 = ipv4.Assign (ndc3);

  ipv4.SetBase (Ipv4Address ("10.0.3.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic4 = ipv4.Assign (ndc4);

  ipv4.SetBase (Ipv4Address ("10.0.4.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic5 = ipv4.Assign (ndc5);

  Ptr<Ipv4StaticRouting> staticRouting;
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (src->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.0.2", 1 );
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (dst->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.4.1", 1 );

  if (printRoutingTables)
    {
      RipHelper routingHelper;

      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> routingStream = asciiTraceHelper.CreateFileStream ("Routing_Table_Second2.txt");

      routingHelper.PrintRoutingTableAt (Seconds (121.0), R1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (121.0), R2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (121.0), R3, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (180.0), R1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (180.0), R2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (180.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (49.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (49.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (49.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (51.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (51.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (51.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (60.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (60.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (60.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (119.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (119.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (119.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (121.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (121.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (121.0), R3, routingStream);

      // routingHelper.PrintRoutingTableAt (Seconds (150.0), R1, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (150.0), R2, routingStream);
      // routingHelper.PrintRoutingTableAt (Seconds (150.0), R3, routingStream);
    }

  NS_LOG_INFO ("Create Applications.");
  uint32_t packetSize = 1024;
  Time interPacketInterval = Seconds (1.0);
  V4PingHelper ping ("10.0.4.2");

  ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping.SetAttribute ("Size", UintegerValue (packetSize));
  if (showPings)
    {
      ping.SetAttribute ("Verbose", BooleanValue (true));
    }
  ApplicationContainer apps = ping.Install (src);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (600.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("rip-simple-routing.tr"));
//   csma.EnablePcapAll ("rip-simple-routing", true);

  Simulator::Schedule (Seconds (50), &TearDownLink, R1, R2, 2, 1);
  Simulator::Schedule (Seconds (120), &TearDownLink, R1, R3, 3, 1);

  /* Now, do the actual simulation. */
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (601.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}

