#pragma once
#include "affix-base/pch.h"
#include "affix-base/serializable.h"

namespace adsl
{
	class param_vector_information : public affix_base::data::serializable
	{
	public:
		/// <summary>
		/// The vector of parameters.
		/// </summary>
		std::vector<double> m_param_vector;

		/// <summary>
		/// The number of training sets that have been digested to get the current param vector.
		/// </summary>
		uint64_t m_training_sets_digested = 0;

	public:
		param_vector_information(

		);

		param_vector_information(
			const std::vector<double>& a_param_vector,
			const uint64_t& a_training_sets_digested = 0
		);

		param_vector_information(
			const param_vector_information& a_param_vector_information
		);

		param_vector_information& operator=(
			const param_vector_information& a_param_vector_information
		);

	};
}
