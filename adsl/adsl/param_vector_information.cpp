#include "param_vector_information.h"

using namespace adsl;

param_vector_information::param_vector_information(

) :
	affix_base::data::serializable(
		m_param_vector,
		m_training_sets_digested
	)
{

}

param_vector_information::param_vector_information(
	const std::vector<double>& a_param_vector,
	const uint64_t& a_training_sets_digested
) :
	affix_base::data::serializable(
		m_param_vector,
		m_training_sets_digested
	),
	m_param_vector(a_param_vector),
	m_training_sets_digested(a_training_sets_digested)
{

}

param_vector_information::param_vector_information(
	const param_vector_information& a_param_vector_information
) :
	param_vector_information(
		a_param_vector_information.m_param_vector,
		a_param_vector_information.m_training_sets_digested
	)
{

}
