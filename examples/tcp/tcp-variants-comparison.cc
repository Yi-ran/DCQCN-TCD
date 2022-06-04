/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * “TCP Westwood(+) Protocol Implementation in ns-3”
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpVariantsComparison");

double old_time = 0.0;
EventId output;
Time current = Time::FromInteger(3, Time::S);  //Only record cwnd and ssthresh values every 3 seconds
bool first = true;

static void
OutputTrace ()
{
 // *stream->GetStream() << newtime << " " << newval << std::endl;
 // old_time = newval;
}

static void
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
  double new_time = Simulator::Now().GetSeconds();
  if (old_time == 0 && first)
  {
    double mycurrent = current.GetSeconds();
    *stream->GetStream() << new_time << " " << mycurrent << " " << newval << std::endl;
    first = false;
    output = Simulator::Schedule(current,&OutputTrace);
  }
  else
  {
    if (output.IsExpired())
    {
      *stream->GetStream() << new_time << " " << newval << std::endl;
      output.Cancel();
      output = Simulator::Schedule(current,&OutputTrace);
    }
  }
}

static void
SsThreshTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
  double new_time = Simulator::Now().GetSeconds();
  if (old_time == 0 && first)
  {
    double mycurrent = current.GetSeconds();
    *stream->GetStream() << new_time << " " << mycurrent << " " << newval << std::endl;
    first = false;
    output = Simulator::Schedule(current,&OutputTrace);
  }
  else
  {
    if (output.IsExpired())
    {
      *stream->GetStream() << new_time << " " << newval << std::endl;
      output.Cancel();
      output = Simulator::Schedule(current,&OutputTrace);
    }
  }
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  if (cwnd_tr_file_name.compare("") == 0)
     {
       NS_LOG_DEBUG ("No trace file for cwnd provided");
       return;
     }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwnd_tr_file_name.c_str());
      Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",MakeBoundCallback (&CwndTracer, stream));
    }
}

static void
TraceSsThresh(std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  if (ssthresh_tr_file_name.compare("") == 0)
    {
      NS_LOG_DEBUG ("No trace file for ssthresh provided");
      return;
    }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(ssthresh_tr_file_name.c_str());
      Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",MakeBoundCallback (&SsThreshTracer, stream));
    }
}

int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpWestwood";
  double error_p = 0.0;
  std::string bandwidth = "2Mbps";
  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "45ms";
  bool tracing = false;
  std::string tr_file_name = "";
  std::string cwnd_tr_file_name = "";
  std::string ssthresh_tr_file_name = "";
  double data_mbytes = 0;
  uint32_t mtu_bytes = 400;
  uint16_t num_flows = 1;
  float duration = 100;
  uint32_t run = 0;
  bool flow_monitor = true;


  CommandLine cmd;
  cmd.AddValue("transport_prot", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus ", transport_prot);
  cmd.AddValue("error_p", "Packet error rate", error_p);
  cmd.AddValue("bandwidth", "Bottleneck bandwidth", bandwidth);
  cmd.AddValue("access_bandwidth", "Access link bandwidth", access_bandwidth);
  cmd.AddValue("delay", "Access link delay", access_delay);
  cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue("tr_name", "Name of output trace file", tr_file_name);
  cmd.AddValue("cwnd_tr_name", "Name of output trace file", cwnd_tr_file_name);
  cmd.AddValue("ssthresh_tr_name", "Name of output trace file", ssthresh_tr_file_name);
  cmd.AddValue("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue("num_flows", "Number of flows", num_flows);
  cmd.AddValue("duration", "Time to allow flows to run in seconds", duration);
  cmd.AddValue("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.Parse (argc, argv);

  SeedManager::SetSeed(1);
  SeedManager::SetRun(run);

  // User may find it convenient to enable logging
  //LogComponentEnable("TcpVariantsComparison", LOG_LEVEL_ALL);
  //LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable("DropTailQueue", LOG_LEVEL_ALL);

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header();
  uint32_t ip_header = temp_header->GetSerializedSize();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader();
  uint32_t tcp_header = temp_header->GetSerializedSize();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  // Set the simulation start and stop time
  float start_time = 0.1;
  float stop_time = start_time + duration;

  // Select TCP variant
  if (transport_prot.compare("TcpTahoe") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpTahoe::GetTypeId()));
  else if (transport_prot.compare("TcpReno") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpReno::GetTypeId()));
  else if (transport_prot.compare("TcpNewReno") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));
  else if (transport_prot.compare("TcpWestwood") == 0)
    {// the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId()));
      Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));
    }
  else if (transport_prot.compare("TcpWestwoodPlus") == 0)
    {
      Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId()));
      Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
      Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));
    }
  else
    {
      NS_LOG_DEBUG ("Invalid TCP version");
      exit (1);
    }

  // Create gateways, sources, and sinks
  NodeContainer gateways;
  gateways.Create (1);
  NodeContainer sources;
  sources.Create(num_flows);
  NodeContainer sinks;
  sinks.Create(num_flows);

  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable(uv);
  error_model.SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate(error_p);

  PointToPointHelper UnReLink;
  UnReLink.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  UnReLink.SetChannelAttribute ("Delay", StringValue ("0.01ms"));
  UnReLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));


  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the sources and sinks net devices
  // and the channels between the sources/sinks and the gateways
  PointToPointHelper LocalLink;
  LocalLink.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth));
  LocalLink.SetChannelAttribute ("Delay", StringValue (access_delay));
  Ipv4InterfaceContainer sink_interfaces;
  for (int i=0; i<num_flows; i++)
    {
      NetDeviceContainer devices;
      devices = LocalLink.Install(sources.Get(i), gateways.Get(0));
      address.NewNetwork();
      Ipv4InterfaceContainer interfaces = address.Assign (devices);
      devices = UnReLink.Install(gateways.Get(0), sinks.Get(i));
      address.NewNetwork();
      interfaces = address.Assign (devices);
      sink_interfaces.Add(interfaces.Get(1));
    }

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  for(uint16_t i=0; i<sources.GetN(); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress(i, 0), port));

      if (transport_prot.compare("TcpTahoe") == 0
          || transport_prot.compare("TcpReno") == 0
          || transport_prot.compare("TcpNewReno") == 0
          || transport_prot.compare("TcpWestwood") == 0
          || transport_prot.compare("TcpWestwoodPlus") == 0)
        {
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
          BulkSendHelper ftp("ns3::TcpSocketFactory", Address());
          ftp.SetAttribute ("Remote", remoteAddress);
          ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
          ftp.SetAttribute ("MaxBytes", UintegerValue (int(data_mbytes*1000000)));

          ApplicationContainer sourceApp = ftp.Install (sources.Get(i));
          sourceApp.Start (Seconds (start_time*i));
          sourceApp.Stop (Seconds (stop_time - 3));
          Time check_start (Seconds((start_time*i)+3));

          sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
          ApplicationContainer sinkApp = sinkHelper.Install (sinks);
          sinkApp.Start (Seconds (start_time*i));
          sinkApp.Stop (Seconds (stop_time));
        }
      else
        {
          NS_LOG_DEBUG ("Invalid transport protocol " << transport_prot << " specified");
          exit (1);
        }
    }

  // Set up tracing if enabled
  if (tracing)
    {
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      if (tr_file_name.compare("") == 0)
        {
          NS_LOG_DEBUG ("No trace file provided");
          exit (1);
        }
      else
        {
          ascii.open (tr_file_name.c_str());
          ascii_wrap = new OutputStreamWrapper(tr_file_name.c_str(), std::ios::out);
        }

      stack.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule(Seconds(0.00001), &TraceCwnd, cwnd_tr_file_name);
      Simulator::Schedule(Seconds(0.00001), &TraceSsThresh, ssthresh_tr_file_name);
    }

  UnReLink.EnablePcapAll("TcpVariantsComparison", true);
  LocalLink.EnablePcapAll("TcpVariantsComparison", true);

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowMonitor = flowHelper.InstallAll();
    }

  Simulator::Stop (Seconds(stop_time));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowMonitor->SerializeToXmlFile("TcpVariantsComparison.flowmonitor", true, true);
    }

  Simulator::Destroy ();
  return 0;
}
