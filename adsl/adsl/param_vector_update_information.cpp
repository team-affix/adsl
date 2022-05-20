#include "param_vector_update_information.h"

using namespace adsl;

param_vector_update_information::param_vector_update_information(

) :
	affix_base::data::serializable(
		m_param_vector_information,
		m_update_vector_information
	)
{

}

param_vector_update_information::param_vector_update_information(
	const param_vector_information& a_param_vector_information,
	const param_vector_information& a_update_vector_information
) :
	affix_base::data::serializable(
		m_param_vector_information,
		m_update_vector_information
	),
	m_param_vector_information(a_param_vector_information),
	m_update_vector_information(a_update_vector_information)
{

}

param_vector_update_information::param_vector_update_information(
	const param_vector_update_information& a_param_vector_update_information
) :
	param_vector_update_information(
		a_param_vector_update_information.m_param_vector_information,
		a_param_vector_update_information.m_update_vector_information
	)
{

}
