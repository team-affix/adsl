#include "agent_specific_information.h"

using namespace adsl;

agent_specific_information::agent_specific_information(

) :
	affix_base::data::serializable(m_training_set_hashes, m_compute_speed)
{

}

agent_specific_information::agent_specific_information(
	const std::vector<std::vector<uint8_t>>& a_training_set_hashes,
	const double& a_compute_speed
) :
	affix_base::data::serializable(m_training_set_hashes, m_compute_speed),
	m_training_set_hashes(a_training_set_hashes),
	m_compute_speed(a_compute_speed)
{

}

agent_specific_information::agent_specific_information(
	const agent_specific_information& a_agent_specific_information
) :
	agent_specific_information(
		a_agent_specific_information.m_training_set_hashes,
		a_agent_specific_information.m_compute_speed)
{

}
