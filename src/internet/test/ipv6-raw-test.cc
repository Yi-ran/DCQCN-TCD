/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
/**
 * This is the test code for ipv6-raw-socket-impl.cc.
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/ipv6-raw-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/socket.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"

#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/ipv6-static-routing.h"

#include <string>
#include <limits>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace ns3;

static void
AddInternetStack (Ptr<Node> node)
{
  Ptr<Ipv6L3Protocol> ipv6 = CreateObject<Ipv6L3Protocol> ();
  Ptr<Icmpv6L4Protocol> icmpv6 = CreateObject<Icmpv6L4Protocol> ();
  node->AggregateObject (ipv6);
  node->AggregateObject (icmpv6);
  ipv6->Insert (icmpv6);
  icmpv6->SetAttribute ("DAD", BooleanValue (false));

  //Routing for Ipv6
  Ptr<Ipv6ListRouting> ipv6Routing = CreateObject<Ipv6ListRouting> ();
  ipv6->SetRoutingProtocol (ipv6Routing);
  Ptr<Ipv6StaticRouting> ipv6staticRouting = CreateObject<Ipv6StaticRouting> ();  ipv6Routing->AddRoutingProtocol (ipv6staticRouting, 0);
  /* register IPv6 extensions and options */
  ipv6->RegisterExtensions ();  ipv6->RegisterOptions ();
}


class Ipv6RawSocketImplTest : public TestCase
{
  Ptr<Packet> m_receivedPacket;
  Ptr<Packet> m_receivedPacket2;
  void DoSendData (Ptr<Socket> socket, std::string to);
  void SendData (Ptr<Socket> socket, std::string to);

public:
  virtual void DoRun (void);
  Ipv6RawSocketImplTest ();

  void ReceivePacket (Ptr<Socket> socket, Ptr<Packet> packet, const Address &from);
  void ReceivePacket2 (Ptr<Socket> socket, Ptr<Packet> packet, const Address &from);
  void ReceivePkt (Ptr<Socket> socket);
  void ReceivePkt2 (Ptr<Socket> socket);
};


Ipv6RawSocketImplTest::Ipv6RawSocketImplTest ()
  : TestCase ("Ipv6 Raw socket implementation") 
{
}

void Ipv6RawSocketImplTest::ReceivePacket (Ptr<Socket> socket, Ptr<Packet> packet, const Address &from)
{
  m_receivedPacket = packet;
}

void Ipv6RawSocketImplTest::ReceivePacket2 (Ptr<Socket> socket, Ptr<Packet> packet, const Address &from)
{
  m_receivedPacket2 = packet;
}

void Ipv6RawSocketImplTest::ReceivePkt (Ptr<Socket> socket)
{
  uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (2, MSG_PEEK);
  NS_ASSERT (m_receivedPacket->GetSize () == 2);
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_ASSERT (availableData == m_receivedPacket->GetSize ());
}

void Ipv6RawSocketImplTest::ReceivePkt2 (Ptr<Socket> socket)
{
  uint32_t availableData;
  Address addr;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket2 = socket->Recv (2, MSG_PEEK);
  NS_ASSERT (m_receivedPacket2->GetSize () == 2);
  m_receivedPacket2 = socket->RecvFrom (std::numeric_limits<uint32_t>::max (), 0, addr);
  NS_ASSERT (availableData == m_receivedPacket2->GetSize ());
  Inet6SocketAddress v6addr = Inet6SocketAddress::ConvertFrom (addr);
  NS_TEST_EXPECT_MSG_EQ (v6addr.GetIpv6 (),Ipv6Address ("fe80:0000:0000:0000:0200:00ff:fe00:0003"), "recvfrom");
}

void
Ipv6RawSocketImplTest::DoSendData (Ptr<Socket> socket, std::string to)
{
  Address realTo = Inet6SocketAddress (Ipv6Address (to.c_str ()), 0);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, to);
}

void
Ipv6RawSocketImplTest::SendData (Ptr<Socket> socket, std::string to)
{
  m_receivedPacket = Create<Packet> ();
  m_receivedPacket2 = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (0),
                                  &Ipv6RawSocketImplTest::DoSendData, this, socket, to);
  Simulator::Run ();
}

void
Ipv6RawSocketImplTest::DoRun (void)
{
  // Create topology

  // Receiver Node
  Ptr<Node> rxNode = CreateObject<Node> ();
  AddInternetStack (rxNode);
  Ptr<SimpleNetDevice> rxDev1, rxDev2;
  { // first interface
    rxDev1 = CreateObject<SimpleNetDevice> ();
    rxDev1->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    rxNode->AddDevice (rxDev1);
    Ptr<Ipv6> ipv6 = rxNode->GetObject<Ipv6> ();
    uint32_t netdev_idx = ipv6->AddInterface (rxDev1);
    Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (Ipv6Address ("2001:db8:0::1"), Ipv6Prefix (64));
    ipv6->AddAddress (netdev_idx, ipv6Addr);
    ipv6->SetUp (netdev_idx);
  }

  { // second interface
    rxDev2 = CreateObject<SimpleNetDevice> ();
    rxDev2->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    rxNode->AddDevice (rxDev2);
    Ptr<Ipv6> ipv6 = rxNode->GetObject<Ipv6> ();
    uint32_t netdev_idx = ipv6->AddInterface (rxDev2);
    Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (Ipv6Address ("2001:db8:1::1"), Ipv6Prefix (64));
    ipv6->AddAddress (netdev_idx, ipv6Addr);
    ipv6->SetUp (netdev_idx);
  }

  // Sender Node
  Ptr<Node> txNode = CreateObject<Node> ();
  AddInternetStack (txNode);
  Ptr<SimpleNetDevice> txDev1;
  {
    txDev1 = CreateObject<SimpleNetDevice> ();
    txDev1->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    txNode->AddDevice (txDev1);
    Ptr<Ipv6> ipv6 = txNode->GetObject<Ipv6> ();
    uint32_t netdev_idx = ipv6->AddInterface (txDev1);
    Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (Ipv6Address ("2001:db8:0::2"), Ipv6Prefix (64));
    ipv6->AddAddress (netdev_idx, ipv6Addr);
    ipv6->SetUp (netdev_idx);
  }
  Ptr<SimpleNetDevice> txDev2;
  {
    txDev2 = CreateObject<SimpleNetDevice> ();
    txDev2->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    txNode->AddDevice (txDev2);
    Ptr<Ipv6> ipv6 = txNode->GetObject<Ipv6> ();
    uint32_t netdev_idx = ipv6->AddInterface (txDev2);
    Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (Ipv6Address ("2001:db8:1::2"), Ipv6Prefix (64));
    ipv6->AddAddress (netdev_idx, ipv6Addr);
    ipv6->SetUp (netdev_idx);
  }

  // link the two nodes
  Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel> ();
  rxDev1->SetChannel (channel1);
  txDev1->SetChannel (channel1);

  Ptr<SimpleChannel> channel2 = CreateObject<SimpleChannel> ();
  rxDev2->SetChannel (channel2);
  txDev2->SetChannel (channel2);


  // Create the Ipv6 Raw sockets
  Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<Ipv6RawSocketFactory> ();
  Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (rxSocket->Bind (Inet6SocketAddress (Ipv6Address::GetAny (), 0)), 0, "trivial");
  rxSocket->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));
  rxSocket->SetRecvCallback (MakeCallback (&Ipv6RawSocketImplTest::ReceivePkt, this));

  Ptr<Socket> rxSocket2 = rxSocketFactory->CreateSocket ();
  rxSocket2->SetRecvCallback (MakeCallback (&Ipv6RawSocketImplTest::ReceivePkt2, this));
  rxSocket2->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));
  NS_TEST_EXPECT_MSG_EQ (rxSocket2->Bind (Inet6SocketAddress (Ipv6Address ("2001:db8:1::1"), 0)), 0, "trivial");

  Ptr<SocketFactory> txSocketFactory = txNode->GetObject<Ipv6RawSocketFactory> ();
  Ptr<Socket> txSocket = txSocketFactory->CreateSocket ();
  txSocket->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));

  // ------ Now the tests ------------

  // Unicast test
  SendData (txSocket, "2001:db8:0::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 163, "recv: 2001:db8:0::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket2->GetSize (), 0, "second interface should not receive it");

  m_receivedPacket->RemoveAllByteTags ();
  m_receivedPacket2->RemoveAllByteTags ();

  // Simple Link-local multicast test
  txSocket->Bind (Inet6SocketAddress (Ipv6Address ("2001:db8:0::2"), 0));
  SendData (txSocket, "ff02::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 163, "recv: ff02::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket2->GetSize (), 0, "second socket should not receive it (it is bound specifically to the second interface's address");

  m_receivedPacket->RemoveAllByteTags ();
  m_receivedPacket2->RemoveAllByteTags ();

  // Broadcast test with multiple receiving sockets

  // When receiving broadcast packets, all sockets sockets bound to
  // the address/port should receive a copy of the same packet -- if
  // the socket address matches.
  rxSocket2->Dispose ();
  rxSocket2 = rxSocketFactory->CreateSocket ();
  rxSocket2->SetRecvCallback (MakeCallback (&Ipv6RawSocketImplTest::ReceivePkt2, this));
  rxSocket2->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));
  NS_TEST_EXPECT_MSG_EQ (rxSocket2->Bind (Inet6SocketAddress (Ipv6Address::GetAny (), 0)), 0, "trivial");

  SendData (txSocket, "ff02::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 163, "recv: ff02::1");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket2->GetSize (), 163, "recv: ff02::1");

  m_receivedPacket = 0;
  m_receivedPacket2 = 0;

  Simulator::Destroy ();
}
//-----------------------------------------------------------------------------
class Ipv6RawTestSuite : public TestSuite
{
public:
  Ipv6RawTestSuite () : TestSuite ("ipv6-raw", UNIT)
  {
    AddTestCase (new Ipv6RawSocketImplTest, TestCase::QUICK);
  }
} g_ipv6rawTestSuite;
