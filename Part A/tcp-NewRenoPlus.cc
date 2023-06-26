#include "tcp-NewRenoPlus.h"
#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"
#include "ns3/log.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("TcpNewRenoPlusCongestion");
    // RENO
    NS_OBJECT_ENSURE_REGISTERED (TcpNewRenoPlus);

    TypeId TcpNewRenoPlus::GetTypeId (void){
        static TypeId tid = TypeId ("ns3::TcpNewRenoPlus")
            .SetParent<TcpNewReno> ()
            .SetGroupName ("Internet")
            .AddConstructor<TcpNewRenoPlus> ();
        return tid;
    }

    TcpNewRenoPlus::TcpNewRenoPlus (void) : TcpNewReno (){
    NS_LOG_FUNCTION (this);
    }

    TcpNewRenoPlus::TcpNewRenoPlus (const TcpNewRenoPlus& sock): TcpNewReno (sock){
    NS_LOG_FUNCTION (this);
    }

    TcpNewRenoPlus::~TcpNewRenoPlus (void){
    }

    uint32_t TcpNewRenoPlus::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked){
    NS_LOG_FUNCTION (this << tcb << segmentsAcked);

    if (segmentsAcked >= 1){
        double adder = static_cast<double> (pow(tcb->m_segmentSize, 1.91)) / tcb->m_cWnd.Get ();
        tcb->m_cWnd += static_cast<uint32_t> (adder);
        NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
        return segmentsAcked - 1;
        }

    return 0;
    }

    void TcpNewRenoPlus::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked){
    NS_LOG_FUNCTION (this << tcb << segmentsAcked);

    if (segmentsAcked > 0){
        // tcb->m_cWnd += tcb->m_segmentSize;
        double adder = static_cast<double> (tcb->m_segmentSize * 0.51);
        tcb->m_cWnd += static_cast<uint32_t> (adder);
        NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
        }
    }

    std::string TcpNewRenoPlus::GetName () const{
        return "TcpNewRenoPlus";
    }

} // namespace ns3

