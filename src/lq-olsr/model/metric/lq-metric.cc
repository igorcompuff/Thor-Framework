#include "lq-metric.h"

namespace ns3{
namespace lqmetric{

LqAbstractMetric::LqAbstractMetric(){}


LqAbstractMetric::~LqAbstractMetric() { }

TypeId
LqAbstractMetric::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::lqmetric::LqAbstractMetric")
	    .SetParent<Object> ()
	    .SetGroupName ("LqAbstractMetric");
	    //.AddConstructor<LqAbstractMetric> ()
	   //	.AddAttribute ("MetricInfo", "LQ metric information acquired from Hello and TC messages",
	   //	               UintegerValue(0),
	   //	               MakeUintegerAccessor(&LqAbstractMetric::metricInfo),
	   //	MakeUintegerChecker<uint32_t> ());

	return tid;
}

}
}
