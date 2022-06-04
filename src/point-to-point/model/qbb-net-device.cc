/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
* Author: Yibo Zhu <yibzh@microsoft.com>
*/

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include "ns3/qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/flow-id-tag.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"

#include <iostream>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

	TypeId
		QbbNetDevice::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::QbbNetDevice")
			.SetParent<PointToPointNetDevice>()
			.AddConstructor<QbbNetDevice>()
			.AddAttribute("QbbEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(true),
				MakeBooleanAccessor(&QbbNetDevice::m_qbbEnabled),
				MakeBooleanChecker())
			.AddAttribute("QcnEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_qcnEnabled),
				MakeBooleanChecker())
			.AddAttribute("DynamicThreshold",
				"Enable dynamic threshold.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_dynamicth),
				MakeBooleanChecker())
			.AddAttribute("ClampTargetRate",
				"Clamp target rate.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_EcnClampTgtRate),
				MakeBooleanChecker())
			.AddAttribute("ClampTargetRateAfterTimeInc",
				"Clamp target rate after timer increase.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_EcnClampTgtRateAfterTimeInc),
				MakeBooleanChecker())
			.AddAttribute("PauseTime",
				"Number of microseconds to pause upon congestion",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_pausetime),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("CNPInterval",
				"The interval of generating CNP",
				DoubleValue(50.0),
				MakeDoubleAccessor(&QbbNetDevice::m_qcn_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("AlphaResumInterval",
				"The interval of resuming alpha",
				DoubleValue(55.0),
				MakeDoubleAccessor(&QbbNetDevice::m_alpha_resume_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("RPTimer",
				"The rate increase timer at RP in microseconds",
				DoubleValue(1500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_rpgTimeReset),
				MakeDoubleChecker<double>())
			.AddAttribute("FastRecoveryTimes",
				"The rate increase timer at RP",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_rpgThreshold),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("DCTCPGain",
				"Control gain parameter which determines the level of rate decrease",
				DoubleValue(1.0 / 16),
				MakeDoubleAccessor(&QbbNetDevice::m_g),
				MakeDoubleChecker<double>())
			.AddAttribute("MinRate",
				"Minimum rate of a throttled flow",
				DataRateValue(DataRate("100Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_minRate),
				MakeDataRateChecker())
			.AddAttribute("ByteCounter",
				"Byte counter constant for increment process.",
				UintegerValue(150000),
				MakeUintegerAccessor(&QbbNetDevice::m_bc),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("RateAI",
				"Rate increment unit in AI period",
				DataRateValue(DataRate("5Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_rai),
				MakeDataRateChecker())
			.AddAttribute("RateHAI",
				"Rate increment unit in hyperactive AI period",
				DataRateValue(DataRate("50Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_rhai),
				MakeDataRateChecker())
			.AddAttribute("NPSamplingInterval",
				"The QCN NP sampling interval",
				DoubleValue(0.0),
				MakeDoubleAccessor(&QbbNetDevice::m_qcn_np_sampling_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_nack_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_backto0),
				MakeBooleanChecker())
			.AddAttribute("L2TestRead",
				"Layer 2 test read go back 0 but NACK from n.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_testRead),
				MakeBooleanChecker())
			.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_chunk),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_ack_interval),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("L2WaitForAck",
				"Wait for Ack before sending out next message.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_waitAck),
				MakeBooleanChecker())
			.AddAttribute("L2WaitForAckTimer",
				"Sender's timer of waiting for the ack",
				DoubleValue(500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_waitAckTimer),
				MakeDoubleChecker<double>())
			.AddAttribute("EnableNPECN",
				"EnableNPECN",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_npecn),
				MakeBooleanChecker())
			.AddAttribute("EnableRootECN",
				"EnableRootECN",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_rootecn),
				MakeBooleanChecker())
			.AddAttribute("RootECNInterval",
				"Detection interval of RootECN",
				UintegerValue(40),
				MakeUintegerAccessor(&QbbNetDevice::m_interval),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("EnableRootECNCC",
				"Rate adjustment of RootECN",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_rootecncc),
				MakeBooleanChecker())
			;

		return tid;
	}

	QbbNetDevice::QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
		m_ecn_source = new std::vector<ECNAccount>;
		m_data_source = new std::vector<DataAccount>;
		m_ack_source = new std::vector<ACKAccount>;
		for (uint32_t i = 0; i < qCnt; i++)
		{
			m_paused[i] = false;
			m_victim[i] = 0;
			lastresume_time[i] = 0;
			m_sendout[i] = 0;
			last_caltime[i] = 0;
			//last_inputEgressBytes[i] = 0;
			//grow[i] = 0;
  			pause_num[i] = 0;
  			pause_time[i] = 0;

  			pausedPacketsNum[i] = 0;
  			queue_upon_resume[i] = 0;
  			last_timeout[i] = 0;
  			last_timeout_queue[i] = 0;
  			pre_state[i] = 0;// 0: non-congestion; 1: checking qlen trend (previous is undetermined); 2: undetermined; 3 congestion
		}
		m_qcn_np_sampling = 0;
		for (uint32_t i = 0; i < fCnt; i++)
		{
			m_credits[i] = 0;
			m_nextAvail[i] = Time(0);
			m_findex_udpport_map[i] = 0;
			m_findex_qindex_map[i] = 0;
			m_waitingAck[i] = false;
			for (uint32_t j = 0; j < maxHop; j++)
			{
				m_txBytes[i][j] = m_bc;				//we don't need this at the beginning, so it doesn't matter what value it has
				m_rpWhile[i][j] = m_rpgTimeReset;	//we don't need this at the beginning, so it doesn't matter what value it has
				m_rpByteStage[i][j] = 0;
				m_rpTimeStage[i][j] = 0;
				if(m_rootecncc == true)
				{
					m_alpha[i][j] = 0.5;
				}
				else{
					m_alpha[i][j] = 0.5;
				}
				m_rpStage[i][j] = 0; //not in any qcn stage
			}
		}
		for (uint32_t i = 0; i < pCnt; i++)
		{
			m_ECNState[i] = 0;
			m_ECNIngressCount[i] = 0;
			m_ECNEgressCount[i] = 0;
		}
		m_rate_prevtime = 0;
		m_realrate_prevtime = 0;
		//if(m_node->GetId() == 20 || m_node->GetId() == 7)
		//{
			//m_qcnEnabled = false;
		//}
	}

	QbbNetDevice::~QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		QbbNetDevice::DoDispose()
	{
		NS_LOG_FUNCTION(this);
		// Cancel all the Qbb events
		for (uint32_t i = 0; i < qCnt; i++)
		{
			Simulator::Cancel(m_resumeEvt[i]);
			Simulator::Cancel(m_victimEvt[i]);
		}

		for (uint32_t i = 0; i < fCnt; i++)
		{
			Simulator::Cancel(m_rateIncrease[i]);
		}

		for (uint32_t i = 0; i < pCnt; i++)
			for (uint32_t j = 0; j < qCnt; j++)
				Simulator::Cancel(m_recheckEvt[i][j]);
		PointToPointNetDevice::DoDispose();
	}

	void
		QbbNetDevice::TransmitComplete(void)
	{
		NS_LOG_FUNCTION(this);
		NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
		m_txMachineState = READY;
		NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
		m_phyTxEndTrace(m_currentPkt);
		m_currentPkt = 0;
		DequeueAndTransmit();
	}

	void
		QbbNetDevice::DequeueAndTransmit(void)
	{
		NS_LOG_FUNCTION(this);
		if (m_txMachineState == BUSY) return;	// Quit if channel busy
		//rate limiting
		if(m_node->GetId() == 20 || m_node->GetId() == 7 || m_node->GetId() == 61)
                {
                        //m_qcnEnabled = false;
                }		

		Ptr<Packet> p;
		if (m_node->GetNodeType() == 0 && m_qcnEnabled) //QCN enable NIC    
		{
			p = m_queue->DequeueQCN(m_paused, m_nextAvail, m_findex_qindex_map);
		}
		else if (m_node->GetNodeType() == 0) //QCN disable NIC
		{
			p = m_queue->DequeueNIC(m_paused);
		}
		else   //switch, doesn't care about qcn, just send
		{
			//p = m_queue->Dequeue(m_paused);		//this is strict priority
			p = m_queue->DequeueRR(m_paused);		//this is round-robin
		}
		if (p != 0)
		{
			m_snifferTrace(p);
			m_promiscSnifferTrace(p);
			Ipv4Header h;
			Ptr<Packet> packet = p->Copy();
			uint16_t protocol = 0;
			ProcessHeader(packet, protocol);
			packet->RemoveHeader(h);
			FlowIdTag t;
			if (m_node->GetNodeType() == 0) //I am a NIC, do QCN
			{
				uint32_t fIndex = m_queue->GetLastQueue();
				if (m_rate[fIndex] == 0)			//late initialization	
				{
					m_rate[fIndex] = m_bps;
					for (uint32_t j = 0; j < maxHop; j++)
					{
						m_rateAll[fIndex][j] = m_bps;
						m_targetRate[fIndex][j] = m_bps;
					}
					//yiran : initial rate setting 
				}

				double creditsDue = std::max(0.0, m_bps / m_rate[fIndex] * (p->GetSize() - m_credits[fIndex]));
				Time nextSend = m_tInterframeGap + Seconds(m_bps.CalculateTxTime(creditsDue));
				m_nextAvail[fIndex] = Simulator::Now() + nextSend;
				for (uint32_t i = 0; i < m_queue->m_fcount; i++)	//distribute credits
				{
					if (m_nextAvail[i].GetTimeStep() <= Simulator::Now().GetTimeStep())
						m_credits[i] += m_rate[i] / m_bps*creditsDue;
				}
				m_credits[fIndex] = 0;	//reset credits
				for (uint32_t i = 0; i < 1; i++)
				{
					if (m_rpStage[fIndex][i] > 0)
						m_txBytes[fIndex][i] -= p->GetSize();
					else
						m_txBytes[fIndex][i] = m_bc;
					if (m_txBytes[fIndex][i] < 0)
					{
						if (m_rpStage[fIndex][i] == 1)
						{
							rpr_fast_byte(fIndex, i);
						}
						else if (m_rpStage[fIndex][i] == 2)
						{
							rpr_active_byte(fIndex, i);
						}
						else if (m_rpStage[fIndex][i] == 3)
						{
							rpr_hyper_byte(fIndex, i);
						}
					}
				}
				//yiran cc rate:
				if(m_node->GetId() == 21)
				{
					if(!m_rate_prevtime)
					{
						m_rate_prevtime = Simulator::Now().GetMicroSeconds();
					}
					if(m_rate_prevtime && Simulator::Now().GetMicroSeconds() - m_rate_prevtime >= 100)
					{
						//if(Simulator::Now().GetSeconds() > 2.015 && Simulator::Now().GetSeconds() < 2.04)
						//{
					//std::cout<<Simulator::Now().GetSeconds()<<" "<<m_node->GetId()<<" m_cc_rate: "<< m_rate[fIndex].GetBitRate() /1e9 << std::endl;
						//}
					m_rate_prevtime = Simulator::Now().GetMicroSeconds();
					}
				}
				if (h.GetProtocol() == 17 && m_waitAck) //if it's udp, check wait_for_ack
				{
					UdpHeader udph;
					packet->RemoveHeader(udph);
					SeqTsHeader sth;
					packet->RemoveHeader(sth);
					if (sth.GetSeq() >= m_milestone_tx[fIndex] - 1)
					{
						m_nextAvail[fIndex] = Simulator::Now() + Seconds(32767); //stop sending this flow until acked
						if (!m_waitingAck[fIndex])
						{
							//std::cout << "Waiting the ACK of the message of flow " << fIndex << " " << sth.GetSeq() << ".\n";
							//fflush(stdout);
							m_retransmit[fIndex] = Simulator::Schedule(MicroSeconds(m_waitAckTimer), &QbbNetDevice::Retransmit, this, fIndex);
							m_waitingAck[fIndex] = true;
						}
					}
				}
				p->RemovePacketTag(t);
				TransmitStart(p);
			}
			else //I am a switch, do ECN if this is not a pause
			{
				if (m_queue->GetLastQueue() == qCnt - 1)//this is a pause or cnp, send it immediately!
				{
					//yiran: bug fixed found in github
					if (h.GetProtocol() != 0xFE) //not PFC , here h refers to ipv4header
                    {
                       p->RemovePacketTag(t);
                    }
					TransmitStart(p);
				}
				else
				{
					//switch ECN
					p->PeekPacketTag(t);
					uint32_t inDev = t.GetFlowId();
					m_node->m_broadcom->RemoveFromIngressAdmission(inDev, m_queue->GetLastQueue(), p->GetSize());
					m_node->m_broadcom->RemoveFromEgressAdmission(m_ifIndex, m_queue->GetLastQueue(), p->GetSize());

					//yiran check
					if(m_node->GetId() == 1)
					{
						//std::cout << Simulator::Now().GetMicroSeconds()/1000.0 << " m_id: "<< m_node->GetId() << " inDev: "<<inDev <<" m_ifIndex: "<<m_ifIndex /*<<" Interval: "<< interval*/<< std::endl;
					}

					if ((m_qcnEnabled || !m_qcnEnabled ))
					{
						PppHeader ppp;
						p->RemoveHeader(ppp);
						p->RemoveHeader(h);
						bool egressCongested = ShouldSendCN(inDev, m_ifIndex, m_queue->GetLastQueue());
						if(m_npecn == false && m_rootecn == false)
						{
							
							if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
								if( m_queue->GetLastQueue() == 3 && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
								{
							   		//std::cout <<m_node->GetId()<<" Queue "<< Simulator::Now().GetSeconds() <<" "<< m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] <<std::endl;
								}
							}
							if (egressCongested)
							{
								h.SetEcn((Ipv4Header::EcnType)0x03);
								if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
								if(h.GetSource()==Ipv4Address("10.1.20.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                                {
                                                                       //std::cout<<m_node->GetId()<<" F0 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                                }
								else if(h.GetSource()==Ipv4Address("10.1.21.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                                {
                                                                     //std::cout<<m_node->GetId()<<" F1 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                                }
								else if(h.GetSource()==Ipv4Address("10.1.61.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                                {
                                                                     //std::cout<<m_node->GetId()<<" F2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                                }

								}

							}
						}
						else if(m_npecn == true && m_rootecn == false) 
						{
							if (egressCongested && pausedPacketsNum[m_queue->GetLastQueue()] == 0)
							{
								h.SetEcn((Ipv4Header::EcnType)0x03);
								
							}
							else{
								;
							}
								
						}
						else if(m_npecn == false && m_rootecn == true) 
						{
							
							double interval = Simulator::Now().GetMicroSeconds() - lastresume_time[m_queue->GetLastQueue()];
							/*if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
								if( m_queue->GetLastQueue() == 3 && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
								{
							   		std::cout <<m_node->GetId()<<" Queue "<< Simulator::Now().GetSeconds() <<" "<< m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] <<std::endl;
								}
							}*/
						
								if(interval > m_interval)
								{

									
										if(pre_state[m_queue->GetLastQueue()] == 2 && (m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] >= queue_upon_resume[m_queue->GetLastQueue()]))
										{
											if(egressCongested){
												h.SetEcn((Ipv4Header::EcnType)0x03);
											}
											if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
												if(h.GetSource()==Ipv4Address("10.1.20.1") /*&& Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03 */)
                                                {
                                                        //std::cout<<m_node->GetId()<<" F0 here1 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }
												else if(h.GetSource()==Ipv4Address("10.1.21.1") /*&& Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03*/)
                                                {
                                                        //std::cout<<m_node->GetId()<<" F1 here1 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }
												else if(h.GetSource()==Ipv4Address("10.1.61.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                {
                                                        //std::cout<<m_node->GetId()<<" F2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }

											}
											pre_state[m_queue->GetLastQueue()] = 3;
										}
										else if(pre_state[m_queue->GetLastQueue()] == 2 && (m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] < queue_upon_resume[m_queue->GetLastQueue()]))
										{
											last_timeout[m_queue->GetLastQueue()] = Simulator::Now().GetMicroSeconds();
											last_timeout_queue[m_queue->GetLastQueue()] = m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()];
											pre_state[m_queue->GetLastQueue()] = 1;
										}
										else if(pre_state[m_queue->GetLastQueue()] == 1 && Simulator::Now().GetMicroSeconds() - last_timeout[m_queue->GetLastQueue()] >= m_interval)
										{
											if(m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] < last_timeout_queue[m_queue->GetLastQueue()]+1000*0.9)
											{
												last_timeout[m_queue->GetLastQueue()] = Simulator::Now().GetMicroSeconds();
												last_timeout_queue[m_queue->GetLastQueue()] = m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()];
												pre_state[m_queue->GetLastQueue()] = 1;
												if (m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] <= 2* 1030)
												{
													pre_state[m_queue->GetLastQueue()] = 0;
												}
											}
											else
											{
												if(egressCongested)
												{
													h.SetEcn((Ipv4Header::EcnType)0x03);
												}
												if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
													if(h.GetSource()==Ipv4Address("10.1.20.1"))
                                                	{
                                                        //std::cout<<m_node->GetId()<<" F0 here2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
							//std::cout<<m_node->GetId()<<" F0 here2 "<<last_timeout[m_queue->GetLastQueue()]<<" "<<last_timeout_queue[m_queue->GetLastQueue()]<<" "<<std::endl;
                                                	}
													else if(h.GetSource()==Ipv4Address("10.1.21.1"))
                                                	{
                                                        //std::cout<<m_node->GetId()<<" F1 here2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
							//std::cout<<m_node->GetId()<<" F1 here2 "<<last_timeout[m_queue->GetLastQueue()]<<" "<<last_timeout_queue[m_queue->GetLastQueue()]<<" "<<std::endl;
                                                	}
													else if(h.GetSource()==Ipv4Address("10.1.61.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                	{
                                                        //std::cout<<m_node->GetId()<<" F2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                	}

												}
												pre_state[m_queue->GetLastQueue()] = 3;
											}
											
										}
										else if(pre_state[m_queue->GetLastQueue()] == 1 && Simulator::Now().GetMicroSeconds() - last_timeout[m_queue->GetLastQueue()] < m_interval)
										{
											pre_state[m_queue->GetLastQueue()] = 1;
											if (m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] <= 2 * 1030)
											{
												pre_state[m_queue->GetLastQueue()] = 0;
											}

											
										}
										else if(pre_state[m_queue->GetLastQueue()] == 0 || pre_state[m_queue->GetLastQueue()] == 3)
										{
											if(egressCongested)
											{
												h.SetEcn((Ipv4Header::EcnType)0x03);
											if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
												if(h.GetSource()==Ipv4Address("10.1.20.1") )
                                                {
                                                        //std::cout<<m_node->GetId()<<" F0 here3 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }
												else if(h.GetSource()==Ipv4Address("10.1.21.1"))
                                                {
                                                        //std::cout<<m_node->GetId()<<" F1 here3 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }
												else if(h.GetSource()==Ipv4Address("10.1.61.1") && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                                                {
                                                        //std::cout<<m_node->GetId()<<" F2 "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;
                                                }

											}
												pre_state[m_queue->GetLastQueue()] = 3;
											}
											else
											{
												pre_state[m_queue->GetLastQueue()] = 0;
											}
										}
										
									
								    

									
								}
								else
								{
										if(h.GetEcn() <= 0x02)
										{
											h.SetEcn((Ipv4Header::EcnType)0x02);
											/*if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
												
                                                        std::cout<<m_node->GetId()<<" UE "<<Simulator::Now().GetSeconds()<<" "<<m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()]<< std::endl;											

											}*/
											
										}
										pre_state[m_queue->GetLastQueue()] = 2;
									
									

								}
								
						}
						
						p->AddHeader(h);
						p->AddHeader(ppp);
					}
					p->RemovePacketTag(t);
					//NP--
					if(m_node->m_broadcom->m_usedEgressPGPacketsCounter[m_ifIndex][m_queue->GetLastQueue()])
					{
						m_node->m_broadcom->m_usedEgressPGPacketsCounter[m_ifIndex][m_queue->GetLastQueue()]--;
						//std::cout<<"NP-- "<<m_node->GetId() << " "<< m_ifIndex <<" "<< m_queue->GetLastQueue()<<" "<<m_node->m_broadcom->m_usedEgressPGPacketsCounter[m_ifIndex][m_queue->GetLastQueue()]<<std::endl;
					}
					
					

					if(TransmitStart(p))
					{
						if(pausedPacketsNum[m_queue->GetLastQueue()])
						{
							pausedPacketsNum[m_queue->GetLastQueue()] --;
						}
					}
				}
			}
			return;
		}
		else //No queue can deliver any packet
		{
			NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
			if (m_node->GetNodeType() == 0 && m_qcnEnabled) //nothing to send, possibly due to qcn flow control, if so reschedule sending
			{
				Time t = Simulator::GetMaximumSimulationTime();
				for (uint32_t i = 0; i < m_queue->m_fcount; i++)
				{
					if (m_queue->GetNBytes(i) == 0)
						continue;
					t = Min(m_nextAvail[i], t);
				}
				if (m_nextSend.IsExpired() &&
					t < Simulator::GetMaximumSimulationTime() &&
					t.GetTimeStep() > Simulator::Now().GetTimeStep())
				{
					NS_LOG_LOGIC("Next DequeueAndTransmit at " << t << " or " << (t - Simulator::Now()) << " later");
					NS_ASSERT(t > Simulator::Now());
					m_nextSend = Simulator::Schedule(t - Simulator::Now(), &QbbNetDevice::DequeueAndTransmit, this);
				}
			}
		}
		return;
	}

	void
		QbbNetDevice::Retransmit(uint32_t findex)
	{
		std::cout << "Resending the message of flow " << findex << ".\n";
		fflush(stdout);
		m_queue->RecoverQueue(m_sendingBuffer[findex], findex);
		m_nextAvail[findex] = Simulator::Now();
		m_waitingAck[findex] = false;
		DequeueAndTransmit();
	}


	void
		QbbNetDevice::Resume(unsigned qIndex)
	{
		NS_LOG_FUNCTION(this << qIndex);
		NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
		m_paused[qIndex] = false;
		NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex <<
			" resumed at " << Simulator::Now().GetSeconds());
		lastresume_time[qIndex] = Simulator::Now().GetMicroSeconds();
		if(m_node->GetNodeType() != 0)
		{
			//grow[qIndex] = 0;
			double now = Simulator::Now().GetMicroSeconds();
			
			PAUSE_period[qIndex] = now - last_caltime[qIndex]; // PAUSE duration
			pausedPacketsNum[qIndex] = m_queue->GetNPackets(qIndex);

			//queue length when RESUME
			queue_upon_resume[qIndex] = m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][qIndex];
			m_node->m_broadcom->m_usedEgressPGPacketsCounter[m_ifIndex][qIndex] = m_node->m_broadcom->m_usedEgressPGPackets[m_ifIndex][qIndex];
			//if(m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][qIndex] >= queue_upon_pause[qIndex])
			//{
				//grow[qIndex] = (m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][qIndex] - queue_upon_pause[qIndex]) * 8.0 / 1000.0 / PAUSE_period[qIndex];
			//}
		}
		//m_sendout[0] = 0;
		//m_realrate_prevtime = Simulator::Now().GetMicroSeconds();

		DequeueAndTransmit();
	}

	void
		QbbNetDevice::PauseFinish(unsigned qIndex)
	{
		Resume(qIndex);

		//yiran
		if(m_node->GetId() == 21)
		{
			//std::cout<< m_node->GetId() << " !!!!!!!!!! Time out: resumed at "<< Simulator::Now().GetMicroSeconds() <<std::endl;
		}
	}

	void
		QbbNetDevice::StopVictim(unsigned qIndex)
	{
		if(m_victim[qIndex])
		{
			m_victim[qIndex] = 0;
		}
        }


	void
		QbbNetDevice::Receive(Ptr<Packet> packet)
	{
		NS_LOG_FUNCTION(this << packet);
		
		// rate limiting
                if(m_node->GetId() == 20 || m_node->GetId() == 7 || m_node->GetId() == 61)
                {
                        //m_qcnEnabled = false;
                }

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
			return;
		}

		uint16_t protocol = 0;

		Ptr<Packet> p = packet->Copy();
		ProcessHeader(p, protocol);

		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);

		if ((ipv4h.GetProtocol() != 0xFF && ipv4h.GetProtocol() != 0xFD && ipv4h.GetProtocol() != 0xFC) || m_node->GetNodeType() > 0)
		{ //This is not QCN feedback, not NACK, or I am a switch so I don't care
			if (ipv4h.GetProtocol() != 0xFE) //not PFC
			{
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				if (m_node->GetNodeType() == 0) //NIC
				{
					if (ipv4h.GetProtocol() == 17)	//look at udp only
					{
						uint16_t ecnbits = ipv4h.GetEcn();
						UdpHeader udph;
						p->RemoveHeader(udph);
						SeqTsHeader sth;
						p->PeekHeader(sth);

						p->AddHeader(udph);

						bool found = false;
						uint32_t i, key = 0;

						/*Throughput calculate. add by Yiran*/
						bool founddata = false;
						for (i = 0; i < m_data_source->size(); ++i)
						{
							if ((*m_data_source)[i].source == ipv4h.GetSource() && (*m_data_source)[i].port == udph.GetSourcePort())
							{
								founddata = true;
							
								(*m_data_source)[i].total++;
								(*m_data_source)[i].sendnum++;
								if(ecnbits == 3)
								{
									(*m_data_source)[i].ecnnum3++;
									if(m_node->GetId() == 7)
									{
										std::cout<<Simulator::Now().GetSeconds() << " "<< ipv4h.GetSource() << " "<<udph.GetSourcePort() << " 1"<<std::endl;

									}
								}
								if(ecnbits == 2)
								{
									(*m_data_source)[i].ecnnum2++;
									if(m_node->GetId() == 7)
                                                                        {
                                                                                std::cout<<Simulator::Now().GetSeconds() << " "<< ipv4h.GetSource() << " "<<udph.GetSourcePort() << " 2"<<std::endl;

                                                                        }
								}
								if(ecnbits == 1)
								{
									(*m_data_source)[i].ecnnum1++;
								}
								double interval = Simulator::Now().GetMicroSeconds() - (*m_data_source)[i].prevtime;
								/*if(interval >= 500)
								{
									if((*m_data_source)[i].total < 0)
									{
										(*m_data_source)[i].sendnum = 0;
										(*m_data_source)[i].prevtime = Simulator::Now().GetMicroSeconds();
									}
									else
									{
										double throughput; //Bytes / microscecond
										uint32_t sendnum = (*m_data_source)[i].sendnum;
										throughput = 8.0 * sendnum / 1020.0 * packet->GetSize() / (interval);
										double ecnfraction1 = (*m_data_source)[i].ecnnum1 * 1.0 / (*m_data_source)[i].sendnum;
										double ecnfraction2 = (*m_data_source)[i].ecnnum2 * 1.0 / (*m_data_source)[i].sendnum;
										double ecnfraction3 = (*m_data_source)[i].ecnnum3 * 1.0 / (*m_data_source)[i].sendnum;
										if(m_node->GetId() ==8 || m_node->GetId()==7 || m_node->GetId()==30 ipv4h.GetSource()==Ipv4Address("10.1.2.1") || ipv4h.GetSource()==Ipv4Address("10.1.1.1") && (*m_data_source)[i].ecnnum3){
									//std::cout<<Simulator::Now().GetSeconds() << " "<< ipv4h.GetSource() << " "<<udph.GetSourcePort() << " " << throughput << " "<<(*m_data_source)[i].ecnnum3 <<" "<<sendnum<<" "<<ecnfraction3<<std::endl;
										}
										(*m_data_source)[i].prevtime = Simulator::Now().GetMicroSeconds();
										(*m_data_source)[i].sendnum = 0;
										(*m_data_source)[i].ecnnum1 = 0;
										(*m_data_source)[i].ecnnum2 = 0;
										(*m_data_source)[i].ecnnum3 = 0;
										//(*m_data_source)[i].total = 0;
									}
									
									

								}*/
								
							}
						}
						if (!founddata)
						{
							DataAccount tmp;
							//tmp.qIndex = GetPriority(p->Copy());
							tmp.source = ipv4h.GetSource();
							tmp.total = 1;
							tmp.sendnum = 1;
							tmp.port = udph.GetSourcePort();
							tmp.prevtime = Simulator::Now().GetMicroSeconds();
							m_data_source->push_back(tmp);
							tmp.ecnnum1 = 0;
							tmp.ecnnum2 = 0;
							tmp.ecnnum3 = 0;

						}
						/*end. add by Yiran*/



						for (i = 0; i < m_ecn_source->size(); ++i)
						{
							if ((*m_ecn_source)[i].source == ipv4h.GetSource() && (*m_ecn_source)[i].qIndex == GetPriority(p->Copy()) && (*m_ecn_source)[i].port == udph.GetSourcePort())
							{
								found = true;
								if (ecnbits != 0 && Simulator::Now().GetMicroSeconds() > m_qcn_np_sampling)
								{
										(*m_ecn_source)[i].ecnbits |= ecnbits;
										(*m_ecn_source)[i].qfb++;
								}
								(*m_ecn_source)[i].total++;
								key = i;
							}
						}
						if (!found)
						{
							ECNAccount tmp;
							tmp.qIndex = GetPriority(p->Copy());
							tmp.source = ipv4h.GetSource();
							if (ecnbits != 0 && Simulator::Now().GetMicroSeconds() > m_qcn_np_sampling && tmp.qIndex != 1) //dctcp
							{
								tmp.ecnbits |= ecnbits;
								tmp.qfb = 1;
							}
							else
							{
								tmp.ecnbits = 0;
								tmp.qfb = 0;
							}
							tmp.total = 1;
							tmp.port = udph.GetSourcePort();
							ReceiverNextExpectedSeq[m_ecn_source->size()] = 0;
							m_nackTimer[m_ecn_source->size()] = Time(0);
							m_milestone_rx[m_ecn_source->size()] = m_ack_interval;
							m_lastNACK[m_ecn_source->size()] = -1;
							key = m_ecn_source->size();
							m_ecn_source->push_back(tmp);
							CheckandSendQCN(tmp.source, tmp.qIndex, tmp.port);
						}

						int x = ReceiverCheckSeq(sth.GetSeq(), key);
						if (x == 2) //generate NACK
						{
							Ptr<Packet> newp = Create<Packet>(0);
							qbbHeader seqh;
							seqh.SetSeq(ReceiverNextExpectedSeq[key]);
							seqh.SetPG(GetPriority(p->Copy()));
							seqh.SetPort(udph.GetSourcePort());
							newp->AddHeader(seqh);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFD); //nack=0xFD
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
							if (m_qcnEnabled)
								m_queue->Enqueue(newp, 0);
							else
								m_queue->Enqueue(newp, qCnt - 1);
							DequeueAndTransmit();
						}
						else if (x == 1) //generate ACK
						{
							Ptr<Packet> newp = Create<Packet>(0);
							qbbHeader seqh;
							seqh.SetSeq(ReceiverNextExpectedSeq[key]);
							seqh.SetPG(GetPriority(p->Copy()));
							seqh.SetPort(udph.GetSourcePort());
							newp->AddHeader(seqh);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFC); //ack=0xFC
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							//yiran
							if(ecnbits == 3)
							{
								//head.SetEcn((Ipv4Header::EcnType)0x03);
							}
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
							if (m_qcnEnabled)
								m_queue->Enqueue(newp, 0);
							else
								m_queue->Enqueue(newp, qCnt - 1);
							DequeueAndTransmit();
						}
					}
				}
				else
				{
					//yiran
					if(m_node->GetId()== 1 && ipv4h.GetProtocol() == 17)
					{
						uint16_t ecnbits = ipv4h.GetEcn();
						UdpHeader udph;
						p->RemoveHeader(udph);
						SeqTsHeader sth;
						p->PeekHeader(sth);

						p->AddHeader(udph);
						bool found = false;
						uint32_t i, key = 0;

						/*Throughput calculate. add by Yiran*/
						bool founddata = false;
						for (i = 0; i < m_data_source->size(); ++i)
						{
							if ((*m_data_source)[i].source == ipv4h.GetSource() && (*m_data_source)[i].port == udph.GetSourcePort())
							{
								founddata = true;
								(*m_data_source)[i].total++;
								(*m_data_source)[i].sendnum++;
								if(ecnbits == 3)
								{
									(*m_data_source)[i].ecnnum3++;
								}
								if(ecnbits == 2)
								{
									(*m_data_source)[i].ecnnum2++;
								}
								if(ecnbits == 1)
								{
									(*m_data_source)[i].ecnnum1++;
								}
								double interval = Simulator::Now().GetMicroSeconds() - (*m_data_source)[i].prevtime;
								if(interval >= 250)
								{
									if((*m_data_source)[i].total < 100)
									{
										(*m_data_source)[i].sendnum = 0;
										(*m_data_source)[i].prevtime = Simulator::Now().GetMicroSeconds();
									}
									else
									{
										double throughput; //Bytes / microscecond
										//double interval = Simulator::Now().GetMicroSeconds() - (*m_data_source)[i].prevtime;
										uint32_t sendnum = (*m_data_source)[i].sendnum;
										throughput = 8.0 * sendnum / 1000.0 * packet->GetSize() / (interval);
										double ecnfraction1 = (*m_data_source)[i].ecnnum1 * 1.0 / (*m_data_source)[i].sendnum;
										double ecnfraction2 = (*m_data_source)[i].ecnnum2 * 1.0 / (*m_data_source)[i].sendnum;
										double ecnfraction3 = (*m_data_source)[i].ecnnum3 * 1.0 / (*m_data_source)[i].sendnum;
										//std::cout<<Simulator::Now().GetMicroSeconds()/1000.0 << " "<< ipv4h.GetSource() << " "<<udph.GetSourcePort() << " " << throughput << " "<<ecnfraction1 <<" "<<ecnfraction2<<" "<<ecnfraction3<<std::endl;
										(*m_data_source)[i].prevtime = Simulator::Now().GetMicroSeconds();
										(*m_data_source)[i].sendnum = 0;
										(*m_data_source)[i].ecnnum1 = 0;
										(*m_data_source)[i].ecnnum2 = 0;
										(*m_data_source)[i].ecnnum3 = 0;
										//(*m_data_source)[i].total = 0;
									}
									
									

								}
								
							}
						}
						if (!founddata)
						{
							DataAccount tmp;
							//tmp.qIndex = GetPriority(p->Copy());
							tmp.source = ipv4h.GetSource();
							tmp.total = 1;
							tmp.sendnum = 1;
							tmp.port = udph.GetSourcePort();
							tmp.prevtime = Simulator::Now().GetMicroSeconds();
							m_data_source->push_back(tmp);
							tmp.ecnnum1 = 0;
							tmp.ecnnum2 = 0;
							tmp.ecnnum3 = 0;

						}
						/*end. add by Yiran*/


					}
				}
				PointToPointReceive(packet);
			}
			else // If this is a Pause, stop the corresponding queue
			{
				if (!m_qbbEnabled) return;
				PauseHeader pauseh;
				p->RemoveHeader(pauseh);
				unsigned qIndex = pauseh.GetQIndex();
				bool prestate = m_paused[qIndex];
				m_paused[qIndex] = true;
				// receive a PAUSE
				if (pauseh.GetTime() > 0)
				{
					
					Simulator::Cancel(m_resumeEvt[qIndex]);
					
										
					//m_paused[qIndex] = true;
					m_resumeEvt[qIndex] = Simulator::Schedule(MicroSeconds(pauseh.GetTime()), &QbbNetDevice::PauseFinish, this, qIndex);
					//yiran
				    if(m_node->GetId() == 0 && m_node->GetNodeType() != 0)
				    {
				       pause_num[qIndex] ++;
					   //std::cout<< m_node->GetId() <<" "<< qIndex << " paused at "<< Simulator::Now().GetMicroSeconds() <<std::endl;
					   if(pause_time[qIndex] == 0)
					   {
					   		pause_time[qIndex] = Simulator::Now().GetMicroSeconds();
					   }
					   else if(Simulator::Now().GetMicroSeconds() - pause_time[qIndex] >= 100)
					   {
					   	  	//std::cout<< pause_time[qIndex] << " " << m_node->GetId() << " pause rate "<< pause_num[qIndex] / (Simulator::Now().GetMicroSeconds() - pause_time[qIndex]) <<std::endl;
					   	  	pause_time[qIndex] = Simulator::Now().GetMicroSeconds();
					   	  	pause_num[qIndex] = 0;
					   }
				    }
				    m_victim[qIndex] = 1;
				    if(/*m_node->GetNodeType() != 0 && */prestate == false)
				    {
				    	
				    	last_caltime[qIndex] = Simulator::Now().GetMicroSeconds();
				    	double interval = Simulator::Now().GetMicroSeconds() - lastresume_time[qIndex];
						if(m_node->GetId() == 3 || m_node->GetId() == 2 ||m_node->GetId() == 62){
				    		if(Simulator::Now().GetSeconds() < 2.023)
				    		{
				    			//resume_interval
				    			double sendrate = 8.0 * m_sendout[0] / 1000 /(interval);
                                //std::cout<< m_node->GetId() << " "<< Simulator::Now().GetSeconds() - interval/1000000.0<<" "<< interval <<" "<< 0 <<std::endl;
								//std::cout<<m_node->GetId() << " "<< Simulator::Now().GetSeconds() - interval/1000000.0<<" "<< interval <<" "<< 40 <<std::endl;
					   			//std::cout<<m_node->GetId() << " "<<Simulator::Now().GetSeconds()<<" "<< interval <<" "<< 40 <<std::endl;
                                //std::cout<<m_node->GetId() << " "<<Simulator::Now().GetSeconds() <<" "<< interval <<" "<< 0 <<std::endl;
				    		}
					}
				    	
				    }
				    
				    
				    /*if(prestate == false && lastresume_time[qIndex])
				    {
				    	double interval = Simulator::Now().GetMicroSeconds() - lastresume_time[qIndex];
				    	if(m_node->GetId() == 21)
				    	{
				    		//double sendrate = 8.0 * m_sendout[qIndex] / 1000 * 1030 / (interval);
				    		//std::cout << m_node->GetId() << " m_sendout: "<< m_sendout[qIndex] << " qIndex: "<< qIndex <<" rate: "<< sendrate <<Simulator::Now().GetMicroSeconds()<<std::endl;
				    		//std::cout << m_node->GetId() << " interval: "<< interval << " qIndex: "<< qIndex <<" "<<Simulator::Now().GetMicroSeconds()<<std::endl;

				    	}
				    	//m_sendout[qIndex] = 0;				    	
				    	if(m_node->GetNodeType() != 0)
				    	{
				    		m_victim[qIndex] = 1;
				    		if(interval < m_interval)
				    		{
				    			m_victim[qIndex] = 1;
				    			if(m_node->GetId() == 0)
				    			{
				    				//std::cout << m_node->GetId() << " interval: "<< interval << " qIndex: "<< qIndex << " set as victim at " <<Simulator::Now().GetMicroSeconds()<<std::endl;
				    			}
				    			//Simulator::Cancel(m_victimEvt[qIndex]);
				    		}
				    		else
				    		{
				    			//m_victim[qIndex] = 0;
				    			//Simulator::Cancel(m_victimEvt[qIndex]);
				    			
				    		}
				    	}
				    }
				    else if(prestate == true && lastresume_time[qIndex])
				    {
				    	double interval = Simulator::Now().GetMicroSeconds() - lastresume_time[qIndex];
				    	if(m_node->GetId() == 0){
				    	//std::cout << m_node->GetId() << " interval: "<< interval << " qIndex: "<< qIndex <<" "<<Simulator::Now().GetMicroSeconds()<<std::endl;
				    	}

				    }*/

				}
				//receive a RESUME
				else
				{
					//Simulator::Cancel(m_resumeEvt[qIndex]);
					//PauseFinish(qIndex);
					
					if(prestate == true){
					Simulator::Cancel(m_resumeEvt[qIndex]);
					PauseFinish(qIndex);
					}
					else
					{
						m_paused[qIndex] = false;
					}
					//yiran
				    if(m_node->GetNodeType() != 0 )
				    {
				    	m_node->m_broadcom->m_usedEgressPGPacketsCounter[m_ifIndex][qIndex] = m_node->m_broadcom->m_usedEgressPGPackets[m_ifIndex][qIndex];
				    }
				}
			}
		}
		else if (ipv4h.GetProtocol() == 0xFF)
		{
			// QCN on NIC
			// This is a Congestion signal
			// Then, extract data from the congestion packet.
			// We assume, without verify, the packet is destinated to me
			CnHeader cnHead;
			p->RemoveHeader(cnHead);
			uint32_t qIndex = cnHead.GetQindex();
			if (qIndex == 1)		//DCTCP
			{
				std::cout << "TCP--ignore\n";
				return;
			}
			uint32_t udpport = cnHead.GetFlow();
			uint16_t ecnbits = cnHead.GetECNBits();
			uint16_t qfb = cnHead.GetQfb();
			uint16_t total = cnHead.GetTotal();

			uint32_t i;
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_udpport_map[i] == udpport && m_findex_qindex_map[i] == qIndex)
					break;
			}
			if (i == m_queue->m_fcount)
				std::cout << "ERROR: QCN NIC cannot find the flow\n";

			if (qfb == 0)
			{
				std::cout << "ERROR: Unuseful QCN\n";
				return;	// Unuseful CN
			}
			if (m_rate[i] == 0)			//lazy initialization	
			{
				m_rate[i] = m_bps;
				for (uint32_t j = 0; j < maxHop; j++)
				{
					m_rateAll[i][j] = m_bps;
					m_targetRate[i][j] = m_bps;	//targetrate remembers the last rate
				}
			}
			if (ecnbits == 0x03)
			{
				rpr_cnm_received(i, 0, qfb*1.0 / (total + 1));
			}
			if (m_rootecncc == true && ecnbits == 0x02)
			{
				rpr_cnm_received(i, 0, 2); //quasi-
			}
			m_rate[i] = m_bps;
			for (uint32_t j = 0; j < maxHop; j++)
				m_rate[i] = std::min(m_rate[i], m_rateAll[i][j]);
			PointToPointReceive(packet);
		}

		else if (ipv4h.GetProtocol() == 0xFD)//NACK on NIC
		{
			qbbHeader qbbh;
			p->Copy()->RemoveHeader(qbbh);

			int qIndex = qbbh.GetPG();
			int seq = qbbh.GetSeq();
			int port = qbbh.GetPort();
			int i;
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_udpport_map[i] == port && m_findex_qindex_map[i] == qIndex)
				{
					break;
				}
			}
			if (i == m_queue->m_fcount)
			{
				std::cout << "ERROR: NACK NIC cannot find the flow\n";
			}

			uint32_t buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
			if (!m_backto0)
			{
				if (buffer_seq > seq)
				{
					std::cout << "ERROR: Sendingbuffer miss!\n";
				}
				while (seq > buffer_seq)
				{
					m_sendingBuffer[i]->Dequeue();
					buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
				}
			}
			else
			{
				uint32_t goback_seq = seq / m_chunk*m_chunk;
				if (buffer_seq > goback_seq)
				{
					std::cout << "ERROR: Sendingbuffer miss!\n";
				}
				while (goback_seq > buffer_seq)
				{
					m_sendingBuffer[i]->Dequeue();
					buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
				}

			}

			m_queue->RecoverQueue(m_sendingBuffer[i], i);

			if (m_waitAck && m_waitingAck[i])
			{
				m_nextAvail[i] = Simulator::Now();
				Simulator::Cancel(m_retransmit[i]);
				m_waitingAck[i] = false;
				DequeueAndTransmit();
			}

			PointToPointReceive(packet);
		}
		else if (ipv4h.GetProtocol() == 0xFC)//ACK on NIC
		{
			qbbHeader qbbh;
			p->Copy()->RemoveHeader(qbbh);

			int qIndex = qbbh.GetPG();
			int seq = qbbh.GetSeq();
			int port = qbbh.GetPort();
			int i;

			/*ACK receicing rate. calculate by Yiran*/
			bool founddata = false;
			for (i = 0; i < m_ack_source->size(); ++i)
			{
				if ((*m_ack_source)[i].source == ipv4h.GetSource() && (*m_ack_source)[i].port == port)
				{
					founddata = true;
					(*m_ack_source)[i].total++;
					(*m_ack_source)[i].recvnum++;
					if(ipv4h.GetEcn()/* && Simulator::Now().GetMicroSeconds() > 2010080 && Simulator::Now().GetMicroSeconds() < 2013080*/)
					{
						(*m_ack_source)[i].ecnnum++;
					}
					double gap = Simulator::Now().GetNanoSeconds()-(*m_ack_source)[i].prevacktime;
					double interval = Simulator::Now().GetMicroSeconds() - (*m_ack_source)[i].prevtime;
					if(m_node->GetId() == 3 && Simulator::Now().GetMicroSeconds() > 2014780 && Simulator::Now().GetMicroSeconds() < 2015580)
					{
						//std::cout<<"per-ACK gap: "<< Simulator::Now().GetMicroSeconds() << " "<< ipv4h.GetSource() << " "<<port << " "<<gap<<std::endl;

					}

					if(interval >= 65)
					{
						
						double recvrate; //Bytes / microscecond
						uint32_t recvnum = (*m_ack_source)[i].recvnum;
						recvrate = 8.0 * recvnum / 1000.0 * 1000.0 / (interval);
						double ecnfraction = (*m_ack_source)[i].ecnnum * 1.0 / (*m_ack_source)[i].recvnum;
						//std::cout<<"ACK: "<< Simulator::Now().GetMicroSeconds()/1000.0 << " "<< ipv4h.GetSource() << " "<<port << " " << recvrate << " "<< (*m_ack_source)[i].ecnnum<<" "<<(*m_ack_source)[i].recvnum<<" "<<ecnfraction<< " "<< (*m_ack_source)[i].gapsum <<std::endl;
						(*m_ack_source)[i].prevtime = Simulator::Now().GetMicroSeconds();
						(*m_ack_source)[i].recvnum = 0;
						(*m_ack_source)[i].ecnnum = 0;	
						(*m_ack_source)[i].gapsum = 0;												

					}
					(*m_ack_source)[i].prevacktime = Simulator::Now().GetNanoSeconds();
					if(gap >= 11)
					{
						(*m_ack_source)[i].gapsum = 1;
					}
					
								
				}
			}
			if (!founddata)
			{
				ACKAccount tmp;
				tmp.source = ipv4h.GetSource();
				tmp.total = 1;
				tmp.recvnum = 1;
				tmp.port = port;
				tmp.prevtime = Simulator::Now().GetMicroSeconds();
				tmp.prevacktime = tmp.prevtime;
				tmp.gapsum = 0;
				m_ack_source->push_back(tmp);
				tmp.ecnnum = 0;

			}
			/*end. add by Yiran*/


			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_udpport_map[i] == port && m_findex_qindex_map[i] == qIndex)
				{
					break;
				}
			}
			if (i == m_queue->m_fcount)
			{
				std::cout << "ERROR: ACK NIC cannot find the flow\n";
			}

			uint32_t buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());

			if (m_ack_interval == 0)
			{
				std::cout << "ERROR: shouldn't receive ack\n";
			}
			else
			{
				if (!m_backto0)
				{
					while (seq > buffer_seq)
					{
						m_sendingBuffer[i]->Dequeue();
						if (m_sendingBuffer[i]->IsEmpty())
						{
							break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
					}
				}
				else
				{
					uint32_t goback_seq = seq / m_chunk*m_chunk;
					while (goback_seq > buffer_seq)
					{
						m_sendingBuffer[i]->Dequeue();
						if (m_sendingBuffer[i]->IsEmpty())
						{
							break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
					}
				}
			}

			if (m_waitAck && seq >= m_milestone_tx[i])
			{
				//Got ACK, resume sending
				m_nextAvail[i] = Simulator::Now();
				Simulator::Cancel(m_retransmit[i]);
				m_waitingAck[i] = false;
				m_milestone_tx[i] += m_chunk;
				DequeueAndTransmit();

			}

			PointToPointReceive(packet);
		}
	}

	void
		QbbNetDevice::PointToPointReceive(Ptr<Packet> packet)
	{
		NS_LOG_FUNCTION(this << packet);
		uint16_t protocol = 0;

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
		}
		else
		{
			// 
			// Hit the trace hooks.  All of these hooks are in the same place in this 
			// device becuase it is so simple, but this is not usually the case in 
			// more complicated devices.
			//
			m_snifferTrace(packet);
			m_promiscSnifferTrace(packet);
			m_phyRxEndTrace(packet);
			//
			// Strip off the point-to-point protocol header and forward this packet
			// up the protocol stack.  Since this is a simple point-to-point link,
			// there is no difference in what the promisc callback sees and what the
			// normal receive callback sees.
			//
			ProcessHeader(packet, protocol);

			if (!m_promiscCallback.IsNull())
			{
				m_macPromiscRxTrace(packet);
				m_promiscCallback(this, packet, protocol, GetRemote(), GetAddress(), NetDevice::PACKET_HOST);
			}

			m_macRxTrace(packet);
			m_rxCallback(this, packet, protocol, GetRemote());
		}
	}

	uint32_t
		QbbNetDevice::GetPriority(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh.GetPG();
	}

	uint32_t
		QbbNetDevice::GetSeq(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol;
		ProcessHeader(p, protocol);
		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh.GetSeq();
	}

	bool
		QbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
	{
		NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
		NS_LOG_LOGIC("UID is " << packet->GetUid());
		if (IsLinkUp() == false) {
			m_macTxDropTrace(packet);
			return false;
		}

		Ipv4Header h;
		packet->PeekHeader(h);
		unsigned qIndex;
		if (h.GetProtocol() == 0xFF || h.GetProtocol() == 0xFE || h.GetProtocol() == 0xFD)  //QCN or PFC or NACK, go highest priority
		{
			qIndex = qCnt - 1;
		}
		else
		{
			Ptr<Packet> p = packet->Copy();
			p->RemoveHeader(h);

			if (h.GetProtocol() == 17)
				qIndex = GetPriority(p);
			else
				qIndex = 1; //dctcp
		}

		Ptr<Packet> p = packet->Copy();
		AddHeader(packet, protocolNumber);

		if (m_node->GetNodeType() == 0)
		{
			if (m_qcnEnabled && qIndex == qCnt - 1)
			{
				m_queue->Enqueue(packet, 0); //QCN uses 0 as the highest priority on NIC
			}
			else
			{
				Ipv4Header ipv4h;
				p->RemoveHeader(ipv4h);
				UdpHeader udph;
				p->RemoveHeader(udph);
				uint32_t port = udph.GetSourcePort();
				uint32_t i;
				for (i = 1; i < fCnt; i++)
				{
					if (m_findex_udpport_map[i] == 0)
					{
						m_queue->m_fcount = i + 1;
						m_findex_udpport_map[i] = port;
						m_findex_qindex_map[i] = qIndex;
						m_sendingBuffer[i] = CreateObject<DropTailQueue>();
						if (m_waitAck)
						{
							m_milestone_tx[i] = m_chunk;
						}
						break;
					}
					if (m_findex_udpport_map[i] == port && m_findex_qindex_map[i] == qIndex)
						break;
				}
				if (m_sendingBuffer[i]->GetNPackets() == 8000)
				{
					m_sendingBuffer[i]->Dequeue();
				}
				m_sendingBuffer[i]->Enqueue(packet->Copy());

				if (m_qcnEnabled)
				{
					m_queue->Enqueue(packet, i);
				}
				else
				{
					m_queue->Enqueue(packet, qIndex);
				}
			}

			DequeueAndTransmit();
		}
		else //switch
		{
			//AddHeader(packet, protocolNumber);
			if (qIndex != qCnt - 1)			//not pause frame
			{
				FlowIdTag t;
				packet->PeekPacketTag(t);
				uint32_t inDev = t.GetFlowId();
				if (m_node->m_broadcom->CheckIngressAdmission(inDev, qIndex, packet->GetSize()) && m_node->m_broadcom->CheckEgressAdmission(m_ifIndex, qIndex, packet->GetSize()))			// Admission control
				{
					m_node->m_broadcom->UpdateIngressAdmission(inDev, qIndex, packet->GetSize());
					m_node->m_broadcom->UpdateEgressAdmission(m_ifIndex, qIndex, packet->GetSize());

					m_macTxTrace(packet);
					m_queue->Enqueue(packet, qIndex); // go into MMU and queues
					
					/*if(m_node->GetId() == 62 || m_node->GetId() == 3 || m_node->GetId() == 2){
                        if( m_queue->GetLastQueue() == 3 && Simulator::Now().GetSeconds() > 2.02 && Simulator::Now().GetSeconds() <= 2.03)
                        {
                            std::cout <<m_node->GetId()<<" Queue "<< Simulator::Now().GetSeconds() <<" "<< m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][m_queue->GetLastQueue()] <<std::endl;
                        }
                    }*/
				}
				else
				{
					std::cout<<"Ohh!!!"<<std::endl;
				}
				DequeueAndTransmit();
				if (m_node->GetNodeType() == 1)
				{
					if(m_node->GetId() == 1 && inDev == 1 && Simulator::Now().GetMicroSeconds() > 2010000 && Simulator::Now().GetMicroSeconds() < 2020000)
					{
						//std::cout<<"CheckQueueFull: "<< Simulator::Now().GetMicroSeconds() << std::endl;
					}
					CheckQueueFull(inDev, qIndex); //check queue full

				}
			}
			else			//pause or cnp, doesn't need admission control, just go
			{
				m_queue->Enqueue(packet, qIndex);
				DequeueAndTransmit();
			}

		}
		return true;
	}

	void
		QbbNetDevice::CheckQueueFull(uint32_t inDev, uint32_t qIndex)
	{
		NS_LOG_FUNCTION(this);
		Ptr<Ipv4> m_ipv4 = m_node->GetObject<Ipv4>();
		bool pClasses[qCnt] = { 0 };
		m_node->m_broadcom->GetPauseClasses(inDev, qIndex, pClasses);
		if(m_node->GetId() == 21 && m_ifIndex == 3 && Simulator::Now().GetMicroSeconds() >= 2019700 && Simulator::Now().GetMicroSeconds() < 2020500)
		{
			//std::cout<<Simulator::Now().GetMicroSeconds() <<" CheckQueueFull: "<< inDev << " "<< qIndex << " "<< m_ifIndex << " current egress queue: "<< m_node->m_broadcom->m_usedEgressQSharedBytes[m_ifIndex][qIndex] << std::endl;
		}
		Ptr<NetDevice> device = m_ipv4->GetNetDevice(inDev);
		for (uint32_t j = 0; j < qCnt; j++)
		{
			if (pClasses[j])			// Create the PAUSE packet
			{
				Ptr<Packet> p = Create<Packet>(0);
				PauseHeader pauseh(m_pausetime, m_queue->GetNBytes(j), j);
				p->AddHeader(pauseh);
				Ipv4Header ipv4h;  // Prepare IPv4 header
				ipv4h.SetProtocol(0xFE);
				ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
				ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
				ipv4h.SetPayloadSize(p->GetSize());
				ipv4h.SetTtl(1);
				ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
				p->AddHeader(ipv4h);
				device->Send(p, Mac48Address("ff:ff:ff:ff:ff:ff"), 0x0800);
				    // queue length when trigger PAUSE to upstream
				//m_node->m_broadcom->m_pause_remote[inDev][qIndex] = true;
				m_node->m_broadcom->m_pause_remote[inDev][j] = true;
				//Simulator::Cancel(m_recheckEvt[inDev][qIndex]);
				Simulator::Cancel(m_recheckEvt[inDev][j]);
				//m_recheckEvt[inDev][qIndex] = Simulator::Schedule(MicroSeconds(m_pausetime / 2), &QbbNetDevice::CheckQueueFull, this, inDev, qIndex);				
				m_recheckEvt[inDev][j] = Simulator::Schedule(MicroSeconds(m_pausetime / 2), &QbbNetDevice::CheckQueueFull, this, inDev, j);				      if(m_node->GetId() == 4)
				{
					//std::cout<<"send PAUSE to" << inDev << std::endl;
				}		
			}
		}

		//ON-OFF
		for (uint32_t j = 0; j < qCnt; j++)
		{
			//if (!m_node->m_broadcom->m_pause_remote[inDev][qIndex])
				//continue;
			if (!m_node->m_broadcom->m_pause_remote[inDev][j])
				continue;
			//if (m_node->m_broadcom->GetResumeClasses(inDev, qIndex))  // Create the PAUSE packet
			if (m_node->m_broadcom->GetResumeClasses(inDev, j))  // Create the PAUSE packet
			{
				Ptr<Packet> p = Create<Packet>(0);
				PauseHeader pauseh(0, m_queue->GetNBytes(j), j); //resume
				p->AddHeader(pauseh);
				Ipv4Header ipv4h;  // Prepare IPv4 header
				ipv4h.SetProtocol(0xFE);
				ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
				ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
				ipv4h.SetPayloadSize(p->GetSize());
				ipv4h.SetTtl(1);
				ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
				p->AddHeader(ipv4h);
				device->Send(p, Mac48Address("ff:ff:ff:ff:ff:ff"), 0x0800);
				//m_node->m_broadcom->m_pause_remote[inDev][qIndex] = false;
				m_node->m_broadcom->m_pause_remote[inDev][j] = false;
				if(m_node->GetId() == 1 && inDev == 1)
				{
					//std::cout<<"!!!!!!!!!! A resume sent: "<< j << " "<< Simulator::Now().GetMicroSeconds() << std::endl;
				}
				//Simulator::Cancel(m_recheckEvt[inDev][qIndex]);
				Simulator::Cancel(m_recheckEvt[inDev][j]);
			}
		}
	}

	bool
		QbbNetDevice::IsLocal(const Ipv4Address& addr) const
	{
		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
		for (unsigned i = 0; i < ipv4->GetNInterfaces(); i++) {
			for (unsigned j = 0; j < ipv4->GetNAddresses(i); j++) {
				if (ipv4->GetAddress(i, j).GetLocal() == addr) {
					return true;
				};
			};
		};
		return false;
	}

	void
		QbbNetDevice::ConnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.ConnectWithoutContext(callback);
	}

	void
		QbbNetDevice::DisconnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.DisconnectWithoutContext(callback);
	}

	int32_t
		QbbNetDevice::PrintStatus(std::ostream& os)
	{
		os << "Size:";
		uint32_t sum = 0;
		for (unsigned i = 0; i < qCnt; ++i) {
			os << " " << (m_paused[i] ? "q" : "Q") << "[" << i << "]=" << m_queue->GetNBytes(i);
			sum += m_queue->GetNBytes(i);
		};
		os << " sum=" << sum << std::endl;
		return sum;
	};

	bool
		QbbNetDevice::Attach(Ptr<QbbChannel> ch)
	{
		NS_LOG_FUNCTION(this << &ch);
		m_channel = ch;
		m_channel->Attach(this);
		NotifyLinkUp();
		return true;
	}

	bool
		QbbNetDevice::TransmitStart(Ptr<Packet> p)
	{
		NS_LOG_FUNCTION(this << p);
		NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
		//
		// This function is called to start the process of transmitting a packet.
		// We need to tell the channel that we've started wiggling the wire and
		// schedule an event that will be executed when the transmission is complete.
		//
		NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
		m_txMachineState = BUSY;
		m_currentPkt = p;
		m_phyTxBeginTrace(m_currentPkt);
		Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
		Time txCompleteTime = txTime + m_tInterframeGap;
		NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
		Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this);

		bool result = m_channel->TransmitStart(p, this, txTime);
		if (result == false)
		{
			m_phyTxDropTrace(p);
		}
		//sendout
		
		

		

		if(m_node->GetId() == 2 || m_node->GetId() == 3 || m_node->GetId() == 62)
		{
			if(p->GetSize() >= 800 && Simulator::Now().GetSeconds() > 2.019 && Simulator::Now().GetSeconds() < 2.06)
			{
				uint16_t protocol = 0;

				Ptr<Packet> packet = p->Copy();
				ProcessHeader(packet, protocol);

				Ipv4Header ipv4h;
				packet->RemoveHeader(ipv4h);

				m_sendout[0] = m_sendout[0] + p->GetSize();


				double interval = 10;

				if(m_realrate_prevtime == 0)
				{
					Simulator::Schedule(MicroSeconds(interval), &QbbNetDevice::PrintSendRate, this, interval);
				}

					
						
			    	m_realrate_prevtime = Simulator::Now().GetMicroSeconds();	

					
					
				
			}			
			
			
		}

		

		return result;
	}

	Address
		QbbNetDevice::GetRemote(void) const
	{
		NS_ASSERT(m_channel->GetNDevices() == 2);
		for (uint32_t i = 0; i < m_channel->GetNDevices(); ++i)
		{
			Ptr<NetDevice> tmp = m_channel->GetDevice(i);
			if (tmp != this)
			{
				return tmp->GetAddress();
			}
		}
		NS_ASSERT(false);
		return Address();  // quiet compiler.
	}

	bool
		QbbNetDevice::ShouldSendCN(uint32_t indev, uint32_t ifindex, uint32_t qIndex)
	{
		return m_node->m_broadcom->ShouldSendCN(indev, ifindex, qIndex);
	}

	void
		QbbNetDevice::CheckandSendQCN(Ipv4Address source, uint32_t qIndex, uint32_t port) //port is udp port
	{
		if (m_node->GetNodeType() > 0)
			return;
		if (!m_qcnEnabled)
			return;
		for (uint32_t i = 0; i < m_ecn_source->size(); ++i)
		{
			ECNAccount info = (*m_ecn_source)[i];
			if (info.source == source && info.qIndex == qIndex && info.port == port)
			{
				if(m_npecn == true)
				{       
                     if(info.total)
					{
						info.ecnbits = info.qfb < info.total * 0.95 ? 0 : 0x03;
						if(m_node->GetId() == 7)
						{
							if(info.ecnbits == 0x03)
							{
						   		//std::cout<<"identified as congested" << std::endl;
							}
							else
							{
						   		//std::cout<<"info.qfb: "<<info.qfb<<" info.total: "<<info.total << " *0.95= "<< info.total*0.95<<std::endl;
							}
						}
					}
					else
					{
						info.ecnbits = 0;
					}
				}
				
				//if (info.ecnbits == 0x03)
				if (info.ecnbits > 0)//Pay attention!!!!!!
				{
					Ptr<Packet> p = Create<Packet>(0);
					CnHeader cn(port, qIndex, info.ecnbits, info.qfb, info.total);	// Prepare CN header
					p->AddHeader(cn);
					Ipv4Header head;	// Prepare IPv4 header
					head.SetDestination(source);
					Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
					head.SetSource(myAddr);
					head.SetProtocol(0xFF);
					head.SetTtl(64);
					head.SetPayloadSize(p->GetSize());
					head.SetIdentification(UniformVariable(0, 65536).GetValue());
					p->AddHeader(head);
					uint32_t protocolNumber = 2048;
					AddHeader(p, protocolNumber);	// Attach PPP header
					if (m_qcnEnabled)
						m_queue->Enqueue(p, 0);
					else
						m_queue->Enqueue(p, qCnt - 1);
					((*m_ecn_source)[i]).ecnbits = 0;
					((*m_ecn_source)[i]).qfb = 0;
					((*m_ecn_source)[i]).total = 0;
					DequeueAndTransmit();
					Simulator::Schedule(MicroSeconds(m_qcn_interval), &QbbNetDevice::CheckandSendQCN, this, source, qIndex, port);

				}
				else
				{
					((*m_ecn_source)[i]).ecnbits = 0;
					((*m_ecn_source)[i]).qfb = 0;
					((*m_ecn_source)[i]).total = 0;
					Simulator::Schedule(MicroSeconds(m_qcn_interval), &QbbNetDevice::CheckandSendQCN, this, source, qIndex, port);
				}
			
				
				
				break;
			}
			else
			{
				continue;
			}
		}
		return;
	}

	void
		QbbNetDevice::SetBroadcomParams(
			uint32_t pausetime,
			double qcn_interval,
			double qcn_resume_interval,
			double g,
			DataRate minRate,
			DataRate rai,
			uint32_t fastrecover_times
		)
	{
		m_pausetime = pausetime;
		m_qcn_interval = qcn_interval;
		m_rpgTimeReset = qcn_resume_interval;
		m_g = g;
		m_minRate = m_minRate;
		m_rai = rai;
		m_rpgThreshold = fastrecover_times;
	}

	Ptr<Channel>
		QbbNetDevice::GetChannel(void) const
	{
		return m_channel;
	}

	uint32_t
		QbbNetDevice::GetUsedBuffer(uint32_t port, uint32_t qIndex)
	{
		uint32_t i;
		if (m_qcnEnabled)
		{
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_qindex_map[i] == qIndex && m_findex_udpport_map[i] == port)
					break;
			}
			return m_queue->GetNBytes(i);
		}
		else
		{
			return m_queue->GetNBytes(qIndex);
		}
	}


	void
		QbbNetDevice::SetQueue(Ptr<BEgressQueue> q)
	{
		NS_LOG_FUNCTION(this << q);
		m_queue = q;
	}

	Ptr<BEgressQueue>
		QbbNetDevice::GetQueue()
	{
		return m_queue;
	}


	void
		QbbNetDevice::ResumeECNState(uint32_t inDev)
	{
		m_ECNState[inDev] = 0;
	}

	void
		QbbNetDevice::ResumeECNIngressState(uint32_t inDev)
	{
		m_ECNIngressCount[inDev] = 0;
	}


	void
		QbbNetDevice::ResumeECNEgressState(uint32_t inDev)
	{
		m_ECNEgressCount[inDev] = 0;
	}


	void
		QbbNetDevice::rpr_adjust_rates(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, DataRate("0bps"));
		rpr_fast_recovery(fIndex, hop);
		return;
	}

	void
		QbbNetDevice::rpr_fast_recovery(uint32_t fIndex, uint32_t hop)
	{
		m_rpStage[fIndex][hop] = 1;
		return;
	}


	void
		QbbNetDevice::rpr_active_increase(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, m_rai);
		m_rpStage[fIndex][hop] = 2;
	}


	void
		QbbNetDevice::rpr_active_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc;
		rpr_active_increase(fIndex, hop);
	}

	void
		QbbNetDevice::rpr_active_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		rpr_active_select(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_fast_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc;
		if (m_rpByteStage[fIndex][hop] < m_rpgThreshold)
		{
			rpr_adjust_rates(fIndex, hop);
		}
		else
		{
			rpr_active_select(fIndex, hop);
		}
			
		return;
	}

	void
		QbbNetDevice::rpr_fast_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		if (m_rpTimeStage[fIndex][hop] < m_rpgThreshold)
			rpr_adjust_rates(fIndex, hop);
		else
			rpr_active_select(fIndex, hop);
		return;
	}


	void
		QbbNetDevice::rpr_hyper_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc / 2;
		rpr_hyper_increase(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_hyper_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset / 2;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		rpr_hyper_increase(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_active_select(uint32_t fIndex, uint32_t hop)
	{
		if (m_rpByteStage[fIndex][hop] < m_rpgThreshold || m_rpTimeStage[fIndex][hop] < m_rpgThreshold)
			rpr_active_increase(fIndex, hop);
		else
			rpr_hyper_increase(fIndex, hop);
		return;
	}


	void
		QbbNetDevice::rpr_hyper_increase(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, m_rhai*(std::min(m_rpByteStage[fIndex][hop], m_rpTimeStage[fIndex][hop]) - m_rpgThreshold + 1));
		m_rpStage[fIndex][hop] = 3;
		return;
	}

	void
		QbbNetDevice::AdjustRates(uint32_t fIndex, uint32_t hop, DataRate increase)
	{
		if (((m_rpByteStage[fIndex][hop] == 1) || (m_rpTimeStage[fIndex][hop] == 1)) && (m_targetRate[fIndex][hop] > 10 * m_rateAll[fIndex][hop]))
			m_targetRate[fIndex][hop] /= 8;
		else
			m_targetRate[fIndex][hop] += increase;

		m_rateAll[fIndex][hop] = (m_rateAll[fIndex][hop] / 2) + (m_targetRate[fIndex][hop] / 2);

		if (m_rateAll[fIndex][hop] > m_bps)
			m_rateAll[fIndex][hop] = m_bps;

		m_rate[fIndex] = m_bps;
		for (uint32_t j = 0; j < maxHop; j++)
			m_rate[fIndex] = std::min(m_rate[fIndex], m_rateAll[fIndex][j]);

		
		
		return;
	}

	void
		QbbNetDevice::rpr_cnm_received(uint32_t findex, uint32_t hop, double fraction)
	{
		if (!m_EcnClampTgtRateAfterTimeInc && !m_EcnClampTgtRate)
		{
			if (m_rpByteStage[findex][hop] != 0)
			{
				m_targetRate[findex][hop] = m_rateAll[findex][hop];
				m_txBytes[findex][hop] = m_bc;
			}
		}
		else if (m_EcnClampTgtRate)
		{
			m_targetRate[findex][hop] = m_rateAll[findex][hop];
			m_txBytes[findex][hop] = m_bc; //for fluid model, QCN standard doesn't have this.
		}
		else
		{
			if (m_rpByteStage[findex][hop] != 0 || m_rpTimeStage[findex][hop] != 0)
			{
				m_targetRate[findex][hop] = m_rateAll[findex][hop];
				m_txBytes[findex][hop] = m_bc;
			}
		}
		m_rpByteStage[findex][hop] = 0;
		m_rpTimeStage[findex][hop] = 0;
		//m_alpha[findex][hop] = (1-m_g)*m_alpha[findex][hop] + m_g*fraction;
		//yiran: do not change rate when quasi
		if(fraction <= 1) 
		{
			m_alpha[findex][hop] = (1 - m_g)*m_alpha[findex][hop] + m_g; 	//binary feedback
			m_rateAll[findex][hop] = std::max(m_minRate, m_rateAll[findex][hop] * (1 - m_alpha[findex][hop] / 2));
		}
		Simulator::Cancel(m_resumeAlpha[findex][hop]);
		m_resumeAlpha[findex][hop] = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &QbbNetDevice::ResumeAlpha, this, findex, hop);
		m_rpWhile[findex][hop] = m_rpgTimeReset;
		Simulator::Cancel(m_rptimer[findex][hop]);
		m_rptimer[findex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[findex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, findex, hop);
		rpr_fast_recovery(findex, hop);
	}

	void
		QbbNetDevice::rpr_timer_wrapper(uint32_t fIndex, uint32_t hop)
	{
		if (m_rpStage[fIndex][hop] == 1)
		{
			rpr_fast_time(fIndex, hop);
		}
		else if (m_rpStage[fIndex][hop] == 2)
		{
			rpr_active_time(fIndex, hop);
		}
		else if (m_rpStage[fIndex][hop] == 3)
		{
			rpr_hyper_time(fIndex, hop);
		}
		return;
	}

	void
		QbbNetDevice::ResumeAlpha(uint32_t fIndex, uint32_t hop)
	{
		m_alpha[fIndex][hop] = (1 - m_g)*m_alpha[fIndex][hop];
		Simulator::Cancel(m_resumeAlpha[fIndex][hop]);
		m_resumeAlpha[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &QbbNetDevice::ResumeAlpha, this, fIndex, hop);
	}


	int
		QbbNetDevice::ReceiverCheckSeq(uint32_t seq, uint32_t key)
	{
		uint32_t expected = ReceiverNextExpectedSeq[key];
		if (seq == expected)
		{
			ReceiverNextExpectedSeq[key] = expected + 1;
			if (ReceiverNextExpectedSeq[key] > m_milestone_rx[key])
			{
				m_milestone_rx[key] += m_ack_interval;
				return 1; //Generate ACK
			}
			else if (ReceiverNextExpectedSeq[key] % m_chunk == 0)
			{
				return 1;
			}
			else
			{
				return 5;
			}

		}
		else if (seq > expected)
		{
			// Generate NACK
			if (Simulator::Now() > m_nackTimer[key] || m_lastNACK[key] != expected)
			{
				m_nackTimer[key] = Simulator::Now() + MicroSeconds(m_nack_interval);
				m_lastNACK[key] = expected;
				if (m_backto0 && !m_testRead)
				{
					ReceiverNextExpectedSeq[key] = ReceiverNextExpectedSeq[key] / m_chunk*m_chunk;
				}
				return 2;
			}
			else
				return 4;
		}
		else
		{
			// Duplicate. 
			return 3;
		}
	}


	void
		QbbNetDevice::PrintSendRate(double interval)
		{
			double sendrate = m_sendout[0]*8.0/1000/interval;
			if((Simulator::Now().GetSeconds() < 2.0202 && Simulator::Now().GetSeconds() > 2.02) || (Simulator::Now().GetSeconds() < 2.06 && Simulator::Now().GetSeconds() > 2.0215))
			{
				//std::cout<< m_node->GetId()<< " "<< Simulator::Now().GetSeconds()<<" "<<interval <<" "<< sendrate <<std::endl;
				if(Simulator::Now().GetSeconds() > 2.0216)
				{
					interval = 20;
				}

			}
			m_sendout[0] = 0;
			if(Simulator::Now().GetSeconds() < 2.06){
				//Simulator::Schedule(MicroSeconds(interval), &QbbNetDevice::PrintSendRate, this, interval);
			}


		}

} // namespace ns3

