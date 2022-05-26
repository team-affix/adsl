#include "epoch_information.h"

using namespace adsl;

epoch_information::epoch_information(
	const double& a_cost,
	const uint64_t& a_start_of_epoch_all_time_global_training_sets_digested,
	const uint64_t& a_end_of_epoch_all_time_global_training_sets_digested,
	const uint64_t& a_last_epoch_local_training_sets_digested
) :
	m_cost(a_cost),
	m_start_of_epoch_all_time_global_training_sets_digested(a_start_of_epoch_all_time_global_training_sets_digested),
	m_end_of_epoch_all_time_global_training_sets_digested(a_end_of_epoch_all_time_global_training_sets_digested),
	m_last_epoch_local_training_sets_digested(a_last_epoch_local_training_sets_digested)
{
	
}

uint64_t epoch_information::last_epoch_global_training_sets_digested(

) const
{
	return m_end_of_epoch_all_time_global_training_sets_digested - m_start_of_epoch_all_time_global_training_sets_digested;
}

double epoch_information::last_epoch_local_proportion_contribution(

) const
{
	return (double)m_last_epoch_local_training_sets_digested / (double)last_epoch_global_training_sets_digested();
}
