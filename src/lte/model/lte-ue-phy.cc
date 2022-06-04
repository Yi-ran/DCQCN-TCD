/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include "lte-ue-phy.h"
#include "lte-enb-phy.h"
#include "lte-net-device.h"
#include "lte-ue-net-device.h"
#include "lte-enb-net-device.h"
#include "lte-spectrum-value-helper.h"
#include "lte-amc.h"
#include "lte-ue-mac.h"
#include "ff-mac-common.h"
#include "lte-sinr-chunk-processor.h"
#include <ns3/lte-common.h>
#include <ns3/pointer.h>

NS_LOG_COMPONENT_DEFINE ("LteUePhy");

namespace ns3 {




// duration of data portion of UL subframe
// = TTI - 1 symbol for SRS - 1ns as margin to avoid overlapping simulator events
// (symbol duration in nanoseconds = TTI / 14 (rounded))
// in other words, duration of data portion of UL subframe = TTI*(13/14) -1ns
static const Time UL_DATA_DURATION = NanoSeconds (1e6 - 71429 - 1); 

// delay from subframe start to transmission of SRS 
// = TTI - 1 symbol for SRS 
static const Time UL_SRS_DELAY_FROM_SUBFRAME_START = NanoSeconds (1e6 - 71429); 




////////////////////////////////////////
// member SAP forwarders
////////////////////////////////////////


class UeMemberLteUePhySapProvider : public LteUePhySapProvider
{
public:
  UeMemberLteUePhySapProvider (LteUePhy* phy);

  // inherited from LtePhySapProvider
  virtual void SendMacPdu (Ptr<Packet> p);
  virtual void SendLteControlMessage (Ptr<LteControlMessage> msg);
  virtual void SendRachPreamble (uint32_t prachId, uint32_t raRnti);

private:
  LteUePhy* m_phy;
};

UeMemberLteUePhySapProvider::UeMemberLteUePhySapProvider (LteUePhy* phy) : m_phy (phy)
{

}

void
UeMemberLteUePhySapProvider::SendMacPdu (Ptr<Packet> p)
{
  m_phy->DoSendMacPdu (p);
}

void
UeMemberLteUePhySapProvider::SendLteControlMessage (Ptr<LteControlMessage> msg)
{
  m_phy->DoSendLteControlMessage (msg);
}

void
UeMemberLteUePhySapProvider::SendRachPreamble (uint32_t prachId, uint32_t raRnti)
{
  m_phy->DoSendRachPreamble (prachId, raRnti);
}


////////////////////////////////////////
// LteUePhy methods
////////////////////////////////////////


NS_OBJECT_ENSURE_REGISTERED (LteUePhy);


LteUePhy::LteUePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

LteUePhy::LteUePhy (Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy)
  : LtePhy (dlPhy, ulPhy),
    m_p10CqiPeriocity (MilliSeconds (1)),  // ideal behavior  
    m_a30CqiPeriocity (MilliSeconds (1)),  // ideal behavior
    m_uePhySapUser (0),
    m_ueCphySapUser (0),
    m_rsReceivedPowerUpdated (false),
    m_rsrpSinrSampleCounter (0)
{
  m_amc = CreateObject <LteAmc> ();
  m_uePhySapProvider = new UeMemberLteUePhySapProvider (this);
  m_ueCphySapProvider = new MemberLteUeCphySapProvider<LteUePhy> (this);
  m_macChTtiDelay = UL_PUSCH_TTIS_DELAY;
  
  NS_ASSERT_MSG (Simulator::Now ().GetNanoSeconds () == 0,
                 "Cannot create UE devices after simulation started");
  Simulator::ScheduleNow (&LteUePhy::SubframeIndication, this, 1, 1);

  DoReset ();
}


LteUePhy::~LteUePhy ()
{
  m_txModeGain.clear ();
}

void
LteUePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_uePhySapProvider;
  delete m_ueCphySapProvider;
  LtePhy::DoDispose ();
}



TypeId
LteUePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteUePhy")
    .SetParent<LtePhy> ()
    .AddConstructor<LteUePhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (10.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxPower, 
                                       &LteUePhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0.\" "
                   "In this model, we consider T0 = 290K.",
                   DoubleValue (9.0),
                   MakeDoubleAccessor (&LteUePhy::SetNoiseFigure, 
                                       &LteUePhy::GetNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode1Gain",
                  "Transmission mode 1 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode1Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode2Gain",
                    "Transmission mode 2 gain in dB",
                    DoubleValue (4.2),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode2Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode3Gain",
                    "Transmission mode 3 gain in dB",
                    DoubleValue (-2.8),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode3Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode4Gain",
                    "Transmission mode 4 gain in dB",
                    DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode4Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode5Gain",
                  "Transmission mode 5 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode5Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode6Gain",
                    "Transmission mode 6 gain in dB",
                    DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode6Gain                       ),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode7Gain",
                  "Transmission mode 7 gain in dB",
                  DoubleValue (0.0),
                   MakeDoubleAccessor (&LteUePhy::SetTxMode7Gain                       ),
                  MakeDoubleChecker<double> ())
    .AddTraceSource ("ReportCurrentCellRsrpSinr",
                     "RSRP and SINR statistics.",
                     MakeTraceSourceAccessor (&LteUePhy::m_reportCurrentCellRsrpSinrTrace))
    .AddAttribute ("RsrpSinrSamplePeriod",
                   "The sampling period for reporting RSRP-SINR stats (default value 1)",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteUePhy::m_rsrpSinrSamplePeriod),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("UlPhyTransmission",
                     "DL transmission PHY layer statistics.",
                     MakeTraceSourceAccessor (&LteUePhy::m_ulPhyTransmission))
    .AddAttribute ("DlSpectrumPhy",
                   "The downlink LteSpectrumPhy associated to this LtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&LteUePhy::GetDlSpectrumPhy),
                   MakePointerChecker <LteSpectrumPhy> ())
    .AddAttribute ("UlSpectrumPhy",
                   "The uplink LteSpectrumPhy associated to this LtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&LteUePhy::GetUlSpectrumPhy),
                   MakePointerChecker <LteSpectrumPhy> ())
  ;
  return tid;
}

void
LteUePhy::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  LtePhy::DoInitialize ();
}

void
LteUePhy::SetLteUePhySapUser (LteUePhySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_uePhySapUser = s;
}

LteUePhySapProvider*
LteUePhy::GetLteUePhySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_uePhySapProvider);
}


void
LteUePhy::SetLteUeCphySapUser (LteUeCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_ueCphySapUser = s;
}

LteUeCphySapProvider*
LteUePhy::GetLteUeCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_ueCphySapProvider);
}

void
LteUePhy::SetNoiseFigure (double nf)
{
  NS_LOG_FUNCTION (this << nf);
  m_noiseFigure = nf;
}

double
LteUePhy::GetNoiseFigure () const
{
  NS_LOG_FUNCTION (this);
  return m_noiseFigure;
}

void
LteUePhy::SetTxPower (double pow)
{
  NS_LOG_FUNCTION (this << pow);
  m_txPower = pow;
}

double
LteUePhy::GetTxPower () const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}


uint8_t
LteUePhy::GetMacChDelay (void) const
{
  return (m_macChTtiDelay);
}

Ptr<LteSpectrumPhy>
LteUePhy::GetDlSpectrumPhy () const
{
  return m_downlinkSpectrumPhy;
}

Ptr<LteSpectrumPhy>
LteUePhy::GetUlSpectrumPhy () const
{
  return m_uplinkSpectrumPhy;
}

void
LteUePhy::DoSendMacPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);

  SetMacPdu (p);
}


void
LteUePhy::PhyPduReceived (Ptr<Packet> p)
{
  m_uePhySapUser->ReceivePhyPdu (p);
}

void
LteUePhy::SetSubChannelsForTransmission (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);

  m_subChannelsForTransmission = mask;

  Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
  m_uplinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}


void
LteUePhy::SetSubChannelsForReception (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);
  m_subChannelsForReception = mask;
}


std::vector <int>
LteUePhy::GetSubChannelsForTransmission ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForTransmission;
}


std::vector <int>
LteUePhy::GetSubChannelsForReception ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForReception;
}


Ptr<SpectrumValue>
LteUePhy::CreateTxPowerSpectralDensity ()
{
  NS_LOG_FUNCTION (this);
  LteSpectrumValueHelper psdHelper;
  Ptr<SpectrumValue> psd = psdHelper.CreateTxPowerSpectralDensity (m_ulEarfcn, m_ulBandwidth, m_txPower, GetSubChannelsForTransmission ());

  return psd;
}

void
LteUePhy::GenerateCtrlCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);

  if (!(m_dlConfigured && m_ulConfigured))
    {
      return;
    }

  // check periodic wideband CQI
  if (Simulator::Now () > m_p10CqiLast + m_p10CqiPeriocity)
    {
      Ptr<LteUeNetDevice> thisDevice = GetDevice ()->GetObject<LteUeNetDevice> ();
      Ptr<DlCqiLteControlMessage> msg = CreateDlCqiFeedbackMessage (sinr);
      if (msg)
        {
          DoSendLteControlMessage (msg);
        }
      m_p10CqiLast = Simulator::Now ();
    }
  // check aperiodic high-layer configured subband CQI
  if  (Simulator::Now () > m_a30CqiLast + m_a30CqiPeriocity)
    {
      Ptr<LteUeNetDevice> thisDevice = GetDevice ()->GetObject<LteUeNetDevice> ();
      Ptr<DlCqiLteControlMessage> msg = CreateDlCqiFeedbackMessage (sinr);
      if (msg)
        {
          DoSendLteControlMessage (msg);
        }
      m_a30CqiLast = Simulator::Now ();
    }
}

void
LteUePhy::GenerateDataCqiReport (const SpectrumValue& sinr)
{
  // Not used by UE, CQI are based only on RS
}

void
LteUePhy::ReportInterference (const SpectrumValue& interf)
{
  // Currently not used by UE
}

void
LteUePhy::ReportRsReceivedPower (const SpectrumValue& power)
{
  NS_LOG_FUNCTION (this << power);
  m_rsReceivedPowerUpdated = true;
  m_rsReceivedPower = power;  
}



Ptr<DlCqiLteControlMessage>
LteUePhy::CreateDlCqiFeedbackMessage (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);

  if (m_rnti == 0)
    {
      // abort method, the UE is still not registered
      return (0);
    }
  
  // apply transmission mode gain
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());
  SpectrumValue newSinr = sinr;
  newSinr *= m_txModeGain.at (m_transmissionMode);

  m_rsrpSinrSampleCounter++;
  if (m_rsrpSinrSampleCounter==m_rsrpSinrSamplePeriod)
    {
      NS_ASSERT_MSG (m_rsReceivedPowerUpdated, " RS received power info obsolete");
      // RSRP evaluated as averaged received power among RBs
      double sum = 0.0;
      uint8_t rbNum = 0;
      Values::const_iterator it;
      for (it = m_rsReceivedPower.ConstValuesBegin (); it != m_rsReceivedPower.ConstValuesEnd (); it++)
        {
          // convert PSD [W/Hz] to linear power [W] for the single RE
          // we consider only one RE for the RS since the channel is
          // flat within the same RB
          double powerTxW = ((*it) * 180000.0) / 12.0;
          sum += powerTxW;
          rbNum++;
        }
      double rsrp = sum / (double)rbNum;
      // averaged SINR among RBs
      sum = 0.0;
      rbNum = 0;
      for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
        {
          sum += (*it);
          rbNum++;
        }
      double avSinr = sum / (double)rbNum;
      NS_LOG_INFO (this << " cellId " << m_cellId << " rnti " << m_rnti << " RSRP " << rsrp << " SINR " << avSinr);
 
      m_reportCurrentCellRsrpSinrTrace (m_cellId, m_rnti, rsrp, avSinr);
      m_rsrpSinrSampleCounter = 0;
    }



  // CREATE DlCqiLteControlMessage
  Ptr<DlCqiLteControlMessage> msg = Create<DlCqiLteControlMessage> ();
  CqiListElement_s dlcqi;
  std::vector<int> cqi;
  if (Simulator::Now () > m_p10CqiLast + m_p10CqiPeriocity)
    {
      cqi = m_amc->CreateCqiFeedbacks (newSinr, m_dlBandwidth);
      
      int nLayer = TransmissionModesLayers::TxMode2LayerNum (m_transmissionMode);
      int nbSubChannels = cqi.size ();
      double cqiSum = 0.0;
      int activeSubChannels = 0;
      // average the CQIs of the different RBs
      for (int i = 0; i < nbSubChannels; i++)
        {
          if (cqi.at (i) != -1)
            {
              cqiSum += cqi.at (i);
              activeSubChannels++;
            }
          NS_LOG_DEBUG (this << " subch " << i << " cqi " <<  cqi.at (i));
        }
      dlcqi.m_rnti = m_rnti;
      dlcqi.m_ri = 1; // not yet used
      dlcqi.m_cqiType = CqiListElement_s::P10; // Peridic CQI using PUCCH wideband
      NS_ASSERT_MSG (nLayer > 0, " nLayer negative");
      NS_ASSERT_MSG (nLayer < 3, " nLayer limit is 2s");
      for (int i = 0; i < nLayer; i++)
        {
          if (activeSubChannels > 0)
            {
              dlcqi.m_wbCqi.push_back ((uint16_t) cqiSum / activeSubChannels);
            }
          else
            {
              // approximate with the worst case -> CQI = 1
              dlcqi.m_wbCqi.push_back (1);
            }
        }
      //NS_LOG_DEBUG (this << " Generate P10 CQI feedback " << (uint16_t) cqiSum / activeSubChannels);
      dlcqi.m_wbPmi = 0; // not yet used
      // dl.cqi.m_sbMeasResult others CQI report modes: not yet implemented
    }
  else if (Simulator::Now () > m_a30CqiLast + m_a30CqiPeriocity)
    {
      cqi = m_amc->CreateCqiFeedbacks (newSinr, GetRbgSize ());
      int nLayer = TransmissionModesLayers::TxMode2LayerNum (m_transmissionMode);
      int nbSubChannels = cqi.size ();
      int rbgSize = GetRbgSize ();
      double cqiSum = 0.0;
      int cqiNum = 0;
      SbMeasResult_s rbgMeas;
      //NS_LOG_DEBUG (this << " Create A30 CQI feedback, RBG " << rbgSize << " cqiNum " << nbSubChannels << " band "  << (uint16_t)m_dlBandwidth);
      for (int i = 0; i < nbSubChannels; i++)
        {
          if (cqi.at (i) != -1)
            {
              cqiSum += cqi.at (i);
            }
          // else "nothing" no CQI is treated as CQI = 0 (worst case scenario)
          cqiNum++;
          if (cqiNum == rbgSize)
            {
              // average the CQIs of the different RBGs
              //NS_LOG_DEBUG (this << " RBG CQI "  << (uint16_t) cqiSum / rbgSize);
              HigherLayerSelected_s hlCqi;
              hlCqi.m_sbPmi = 0; // not yet used
              for (int i = 0; i < nLayer; i++)
                {
                  hlCqi.m_sbCqi.push_back ((uint16_t) cqiSum / rbgSize);
                }
              rbgMeas.m_higherLayerSelected.push_back (hlCqi);
              cqiSum = 0.0;
              cqiNum = 0;
            }
        }
      dlcqi.m_rnti = m_rnti;
      dlcqi.m_ri = 1; // not yet used
      dlcqi.m_cqiType = CqiListElement_s::A30; // Aperidic CQI using PUSCH
      //dlcqi.m_wbCqi.push_back ((uint16_t) cqiSum / nbSubChannels);
      dlcqi.m_wbPmi = 0; // not yet used
      dlcqi.m_sbMeasResult = rbgMeas;
    }

  msg->SetDlCqi (dlcqi);
  return msg;
}



void
LteUePhy::DoSendLteControlMessage (Ptr<LteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  SetControlMessages (msg);
}

void 
LteUePhy::DoSendRachPreamble (uint32_t raPreambleId, uint32_t raRnti)
{
  NS_LOG_FUNCTION (this << raPreambleId);

  // unlike other control messages, RACH preamble is sent ASAP
  Ptr<RachPreambleLteControlMessage> msg = Create<RachPreambleLteControlMessage> ();
  msg->SetRapId (raPreambleId);
  m_raPreambleId = raPreambleId;
  m_raRnti = raRnti;
  m_controlMessagesQueue.at (0).push_back (msg);
}


void
LteUePhy::ReceiveLteControlMessageList (std::list<Ptr<LteControlMessage> > msgList)
{
  NS_LOG_FUNCTION (this);
  
  std::list<Ptr<LteControlMessage> >::iterator it;
  for (it = msgList.begin (); it != msgList.end(); it++)
  {
    Ptr<LteControlMessage> msg = (*it);
  
    if (msg->GetMessageType () == LteControlMessage::DL_DCI)
    {
      Ptr<DlDciLteControlMessage> msg2 = DynamicCast<DlDciLteControlMessage> (msg);
      
      DlDciListElement_s dci = msg2->GetDci ();
      if (dci.m_rnti != m_rnti)
        {
          // DCI not for me
          continue;
        }
      
      if (dci.m_resAlloc != 0)
      {
        NS_FATAL_ERROR ("Resource Allocation type not implemented");
      }
      
      std::vector <int> dlRb;
      
      // translate the DCI to Spectrum framework
      uint32_t mask = 0x1;
      for (int i = 0; i < 32; i++)
      {
        if (((dci.m_rbBitmap & mask) >> i) == 1)
        {
          for (int k = 0; k < GetRbgSize (); k++)
          {
            dlRb.push_back ((i * GetRbgSize ()) + k);
//             NS_LOG_DEBUG(this << " RNTI " << m_rnti << " RBG " << i << " DL-DCI allocated PRB " << (i*GetRbgSize()) + k);
          }
        }
        mask = (mask << 1);
      }
      
      // send TB info to LteSpectrumPhy
      NS_LOG_DEBUG (this << " UE " << m_rnti << " DL-DCI " << dci.m_rnti << " bitmap "  << dci.m_rbBitmap);
      for (uint8_t i = 0; i < dci.m_tbsSize.size (); i++)
      {
        m_downlinkSpectrumPhy->AddExpectedTb (dci.m_rnti, dci.m_ndi.at (i), dci.m_tbsSize.at (i), dci.m_mcs.at (i), dlRb, i, dci.m_harqProcess, dci.m_rv.at (i), true /* DL */);
      }
      
      SetSubChannelsForReception (dlRb);
      
      
    }
    else if (msg->GetMessageType () == LteControlMessage::UL_DCI) 
    {
      // set the uplink bandwidht according to the UL-CQI
      Ptr<UlDciLteControlMessage> msg2 = DynamicCast<UlDciLteControlMessage> (msg);
      UlDciListElement_s dci = msg2->GetDci ();
      if (dci.m_rnti != m_rnti)
        {
          // DCI not for me
          continue;
        }
      NS_LOG_INFO (this << " UL DCI");
      std::vector <int> ulRb;
      for (int i = 0; i < dci.m_rbLen; i++)
      {
        ulRb.push_back (i + dci.m_rbStart);
        //NS_LOG_DEBUG (this << " UE RB " << i + dci.m_rbStart);
      }
      QueueSubChannelsForTransmission (ulRb);
      // fire trace of UL Tx PHY stats
      HarqProcessInfoList_t harqInfoList = m_harqPhyModule->GetHarqProcessInfoUl (m_rnti, 0);
      PhyTransmissionStatParameters params;
      params.m_cellId = m_cellId;
      params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in LteHelper
      params.m_timestamp = Simulator::Now ().GetMilliSeconds () + UL_PUSCH_TTIS_DELAY;
      params.m_rnti = m_rnti;
      params.m_txMode = 0; // always SISO for UE
      params.m_layer = 0;
      params.m_mcs = dci.m_mcs;
      params.m_size = dci.m_tbSize;
      params.m_rv = harqInfoList.size ();
      params.m_ndi = dci.m_ndi;
      m_ulPhyTransmission (params);
      // pass the info to the MAC
      m_uePhySapUser->ReceiveLteControlMessage (msg);
    }
    else if (msg->GetMessageType () == LteControlMessage::RAR)
      {
        Ptr<RarLteControlMessage> rarMsg = DynamicCast<RarLteControlMessage> (msg);
        if (rarMsg->GetRaRnti () == m_raRnti)
          {
            for (std::list<RarLteControlMessage::Rar>::const_iterator it = rarMsg->RarListBegin (); it != rarMsg->RarListEnd (); ++it)
              {
                if (it->rapId != m_raPreambleId)
                  {
                    // UL grant not for me
                    continue;
                  }
                else
                  {
                    NS_LOG_INFO ("received RAR RNTI " << m_raRnti);
                    // set the uplink bandwidht according to the UL grant
                    std::vector <int> ulRb;
                    for (int i = 0; i < it->rarPayload.m_grant.m_rbLen; i++)
                    {
                      ulRb.push_back (i + it->rarPayload.m_grant.m_rbStart);
                    }

                    QueueSubChannelsForTransmission (ulRb);
                    // pass the info to the MAC
                    m_uePhySapUser->ReceiveLteControlMessage (msg);
                    // reset RACH variables with out of range values
                    m_raPreambleId = 255;
                    m_raRnti = 11;
                  }
              }
          }
      }
    else if (msg->GetMessageType () == LteControlMessage::MIB) 
      {
        NS_LOG_INFO ("received MIB");
        Ptr<MibLteControlMessage> msg2 = DynamicCast<MibLteControlMessage> (msg);
        m_ueCphySapUser->RecvMasterInformationBlock (msg2->GetMib ());
      }
    else
    {
      // pass the message to UE-MAC
      m_uePhySapUser->ReceiveLteControlMessage (msg);
    }
    
  }
  
  
}


void
LteUePhy::QueueSubChannelsForTransmission (std::vector <int> rbMap)
{
  m_subChannelsForTransmissionQueue.at (m_macChTtiDelay - 1) = rbMap;
}


void
LteUePhy::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this << frameNo << subframeNo);

  NS_ASSERT_MSG (frameNo > 0, "the SRS index check code assumes that frameNo starts at 1");
  
  // refresh internal variables
  m_rsReceivedPowerUpdated = false;
  
  if (m_ulConfigured)
    {
      // update uplink transmission mask according to previous UL-CQIs
      SetSubChannelsForTransmission (m_subChannelsForTransmissionQueue.at (0));
   
      // shift the queue
      for (uint8_t i = 1; i < m_macChTtiDelay; i++)
        {
          m_subChannelsForTransmissionQueue.at (i-1) = m_subChannelsForTransmissionQueue.at (i);
        }
      m_subChannelsForTransmissionQueue.at (m_macChTtiDelay-1).clear ();
  
      if (m_srsConfigured && (m_srsStartTime <= Simulator::Now ()))
        {

          NS_ASSERT_MSG (subframeNo > 0 && subframeNo <= 10, "the SRS index check code assumes that subframeNo starts at 1");
          if ((((frameNo-1)*10 + (subframeNo-1)) % m_srsPeriodicity) == m_srsSubframeOffset)
            {
              NS_LOG_INFO ("frame " << frameNo << " subframe " << subframeNo << " sending SRS (offset=" << m_srsSubframeOffset << ", period=" << m_srsPeriodicity << ")");
              m_sendSrsEvent = Simulator::Schedule (UL_SRS_DELAY_FROM_SUBFRAME_START, 
                                                    &LteUePhy::SendSrs,
                                                    this);
            }
        }
      
      std::list<Ptr<LteControlMessage> > ctrlMsg = GetControlMessages ();
      // send packets in queue
      NS_LOG_LOGIC (this << " UE - start slot for PUSCH + PUCCH - RNTI " << m_rnti << " CELLID " << m_cellId);
      // send the current burts of packets
      Ptr<PacketBurst> pb = GetPacketBurst ();
      if (pb)
        {
          m_uplinkSpectrumPhy->StartTxDataFrame (pb, ctrlMsg, UL_DATA_DURATION);
        }
      else
        {
      // send only PUCCH (ideal: fake null bandwidth signal)
          if (ctrlMsg.size ()>0)
            {
              NS_LOG_LOGIC (this << " UE - start TX PUCCH (NO PUSCH)");
              std::vector <int> dlRb;
              SetSubChannelsForTransmission (dlRb);
              m_uplinkSpectrumPhy->StartTxDataFrame (pb, ctrlMsg, UL_DATA_DURATION);
            }
          else
            {
              NS_LOG_LOGIC (this << " UE - UL NOTHING TO SEND");
            }
        }
    }  // m_configured
  
  // trigger the MAC
  m_uePhySapUser->SubframeIndication (frameNo, subframeNo);
  
  
  ++subframeNo;
  if (subframeNo > 10)
    {
      ++frameNo;
      subframeNo = 1;
    }
  
  // schedule next subframe indication
  Simulator::Schedule (Seconds (GetTti ()), &LteUePhy::SubframeIndication, this, frameNo, subframeNo);  
}
  
void
LteUePhy::SendSrs ()
{
  NS_LOG_FUNCTION (this << " UE " << m_rnti << " start tx SRS, cell Id " << (uint32_t) m_cellId);
  NS_ASSERT (m_cellId > 0);
  // set the current tx power spectral density (full bandwidth)
  std::vector <int> dlRb;
  for (uint8_t i = 0; i < m_ulBandwidth; i++)
    {
      dlRb.push_back (i);
    }
  SetSubChannelsForTransmission (dlRb);
  m_uplinkSpectrumPhy->StartTxUlSrsFrame ();
}


void
LteUePhy::DoReset ()
{
  NS_LOG_FUNCTION (this);
  
  m_rnti = 0;
  m_transmissionMode = 0;
  m_srsPeriodicity = 0;
  m_srsConfigured = false;
  m_dlConfigured = false;
  m_ulConfigured = false;
  m_raPreambleId = 255; // value out of range
  m_raRnti = 11; // value out of range
  m_rsrpSinrSampleCounter = 0;
  m_p10CqiLast = Simulator::Now ();
  m_a30CqiLast = Simulator::Now ();

  m_packetBurstQueue.clear ();
  m_controlMessagesQueue.clear ();
  m_subChannelsForTransmissionQueue.clear ();
  for (int i = 0; i < m_macChTtiDelay; i++)
    {
      Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      m_packetBurstQueue.push_back (pb);
      std::list<Ptr<LteControlMessage> > l;
      m_controlMessagesQueue.push_back (l);
    }
  std::vector <int> ulRb;
  m_subChannelsForTransmissionQueue.resize (m_macChTtiDelay, ulRb);

  m_sendSrsEvent.Cancel ();
  m_downlinkSpectrumPhy->Reset ();
  m_uplinkSpectrumPhy->Reset ();
}

void
LteUePhy::DoSyncronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << cellId);
  m_cellId = cellId;
  m_dlEarfcn = dlEarfcn;
  m_downlinkSpectrumPhy->SetCellId (cellId);
  m_uplinkSpectrumPhy->SetCellId (cellId);

  // configure DL for receing the BCH with the minimum bandwith
  m_dlBandwidth = 6;
  Ptr<SpectrumValue> noisePsd = LteSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_noiseFigure);
  m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
  m_downlinkSpectrumPhy->GetChannel ()->AddRx (m_downlinkSpectrumPhy);  
  
  m_dlConfigured = false;
  m_ulConfigured = false;
}

void
LteUePhy::DoSetDlBandwidth (uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << (uint32_t) dlBandwidth);
  if (m_dlBandwidth != dlBandwidth)
    {
      m_dlBandwidth = dlBandwidth;

      int Type0AllocationRbg[4] = {
        10,     // RGB size 1
        26,     // RGB size 2
        63,     // RGB size 3
        110     // RGB size 4
      };  // see table 7.1.6.1-1 of 36.213
      for (int i = 0; i < 4; i++)
        {
          if (dlBandwidth < Type0AllocationRbg[i])
            {
              m_rbgSize = i + 1;
              break;
            }
        }
  
      Ptr<SpectrumValue> noisePsd = LteSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_noiseFigure);
      m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
      m_downlinkSpectrumPhy->GetChannel ()->AddRx (m_downlinkSpectrumPhy);  
    }
  m_dlConfigured = true;
}


void 
LteUePhy::DoConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth)
{
  m_ulEarfcn = ulEarfcn;
  m_ulBandwidth = ulBandwidth;
  m_ulConfigured = true;
}

 
void
LteUePhy::DoSetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  m_rnti = rnti;
}
 
void
LteUePhy::DoSetTransmissionMode (uint8_t txMode)
{
  NS_LOG_FUNCTION (this << (uint16_t)txMode);
  m_transmissionMode = txMode;
  m_downlinkSpectrumPhy->SetTransmissionMode (txMode);
}

void
LteUePhy::DoSetSrsConfigurationIndex (uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << srcCi);
  m_srsPeriodicity = GetSrsPeriodicity (srcCi);
  m_srsSubframeOffset = GetSrsSubframeOffset (srcCi);
  m_srsConfigured = true;

  // a guard time is needed for the case where the SRS periodicity is changed dynamically at run time
  // if we use a static one, we can have a 0ms guard time
  m_srsStartTime = Simulator::Now () + MilliSeconds (0);
  NS_LOG_DEBUG (this << " UE SRS P " << m_srsPeriodicity << " RNTI " << m_rnti << " offset " << m_srsSubframeOffset << " cellId " << m_cellId << " CI " << srcCi);
}


void 
LteUePhy::SetTxMode1Gain (double gain)
{
  SetTxModeGain (1, gain);
}

void 
LteUePhy::SetTxMode2Gain (double gain)
{
  SetTxModeGain (2, gain);
}

void 
LteUePhy::SetTxMode3Gain (double gain)
{
  SetTxModeGain (3, gain);
}

void 
LteUePhy::SetTxMode4Gain (double gain)
{
  SetTxModeGain (4, gain);
}

void 
LteUePhy::SetTxMode5Gain (double gain)
{
  SetTxModeGain (5, gain);
}

void 
LteUePhy::SetTxMode6Gain (double gain)
{
  SetTxModeGain (6, gain);
}

void 
LteUePhy::SetTxMode7Gain (double gain)
{
  SetTxModeGain (7, gain);
}


void
LteUePhy::SetTxModeGain (uint8_t txMode, double gain)
{
  NS_LOG_FUNCTION (this << gain);
  // convert to linear
  double gainLin = std::pow (10.0, (gain / 10.0));
  if (m_txModeGain.size () < txMode)
    {
      m_txModeGain.resize (txMode);
    }
  std::vector <double> temp;
  temp = m_txModeGain;
  m_txModeGain.clear ();
  for (uint8_t i = 0; i < temp.size (); i++)
    {
      if (i==txMode-1)
        {
          m_txModeGain.push_back (gainLin);
        }
      else
        {
          m_txModeGain.push_back (temp.at (i));
        }
    }
  // forward the info to DL LteSpectrumPhy
  m_downlinkSpectrumPhy->SetTxModeGain (txMode, gain);
}



void
LteUePhy::ReceiveLteDlHarqFeedback (DlInfoListElement_s m)
{
  NS_LOG_FUNCTION (this);
  // generate feedback to eNB and send it through ideal PUCCH
  Ptr<DlHarqFeedbackLteControlMessage> msg = Create<DlHarqFeedbackLteControlMessage> ();
  msg->SetDlHarqFeedback (m);
  SetControlMessages (msg);
}

void
LteUePhy::SetHarqPhyModule (Ptr<LteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}


} // namespace ns3
