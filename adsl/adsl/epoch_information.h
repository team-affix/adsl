#pragma once
#include "affix-base/pch.h"

namespace adsl
{
	class epoch_information
	{
	public:
		size_t m_number_of_machines_training = 0;
		double m_cost = 0;
		uint64_t m_start_of_epoch_all_time_global_training_sets_digested = 0;
		uint64_t m_end_of_epoch_all_time_global_training_sets_digested = 0;
		uint64_t m_last_epoch_local_training_sets_digested = 0;

	public:
		epoch_information(
			const size_t& a_number_of_machines_training,
			const double& a_cost,
			const uint64_t& a_start_of_epoch_all_time_global_training_sets_digested,
			const uint64_t& a_end_of_epoch_all_time_global_training_sets_digested,
			const uint64_t& a_last_epoch_local_training_sets_digested
		);

		uint64_t last_epoch_global_training_sets_digested(

		) const;

		double last_epoch_local_proportion_contribution(

		) const;

	};
}
