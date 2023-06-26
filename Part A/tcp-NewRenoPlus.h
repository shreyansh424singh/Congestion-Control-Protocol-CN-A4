#ifndef TCPNEWRENOPLUS_H
#define TCPNEWRENOPLUS_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"

namespace ns3 {

class TcpNewRenoPlus : public TcpNewReno
{
public:

  static TypeId GetTypeId (void);

  TcpNewRenoPlus ();

  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  TcpNewRenoPlus (const TcpNewRenoPlus& sock);

  ~TcpNewRenoPlus ();

  std::string GetName () const;

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
};

} // namespace ns3

#endif // TCPNEWRENOPLUS_H
