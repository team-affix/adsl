#include "agent_specific_information.h"

using namespace adsl;

agent_specific_information::agent_specific_information(

) :
	affix_base::data::serializable(m_training_set_file_names, m_compute_speed)
{

}

agent_specific_information::agent_specific_information(
	const std::vector<std::string>& a_training_set_file_names,
	const double& a_compute_speed
) :
	affix_base::data::serializable(m_training_set_file_names, m_compute_speed),
	m_training_set_file_names(a_training_set_file_names),
	m_compute_speed(a_compute_speed)
{

}

agent_specific_information::agent_specific_information(
	const agent_specific_information& a_agent_specific_information
) :
	agent_specific_information(
		a_agent_specific_information.m_training_set_file_names,
		a_agent_specific_information.m_compute_speed)
{

}
