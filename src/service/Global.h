#ifndef _MFW_GLOBAL_H_
#define _MFW_GLOBAL_H_

#include "util/util_log.h"
#include <tr1/memory>
using namespace std;

namespace mfw
{

class Connector;
class ConnectorImp;
class ObjectProxy;
class AdapterProxy;
class Transport;
class ServiceProxy;
class ServiceProxyCallback;
class SdpCurrent;
class ReqMessage;

typedef tr1::shared_ptr<Connector> ConnectorPtr;
typedef tr1::shared_ptr<ServiceProxy> ServicePrx;
typedef tr1::shared_ptr<ServiceProxyCallback> ServiceProxyCallbackPtr;
typedef tr1::shared_ptr<SdpCurrent> SdpCurrentPtr;

}
#endif
