#pragma once
#include "affix-base/pch.h"
#include "affix-base/serializable.h"
#include "affix-base/semantic_version_number.h"

namespace adsl
{
	class agent_specific_information : public affix_base::data::serializable
	{
	public:
		/// <summary>
		/// Vector of training set hashes.
		/// </summary>
		std::vector<std::string> m_training_set_file_names;

		/// <summary>
		/// Some measure of the compute speed of this machine.
		/// </summary>
		double m_compute_speed = 0;

	public:
		/// <summary>
		/// Default constructor, initializes fields to their default values.
		/// </summary>
		agent_specific_information(

		);

		/// <summary>
		/// Constructs the adt_information object with the argued values for the fields.
		/// </summary>
		/// <param name="a_distribution_lead_number"></param>
		/// <param name="a_executable_hash"></param>
		/// <param name="a_training_set_hashes"></param>
		agent_specific_information(
			const std::vector<std::string>& a_training_set_file_names,
			const double& a_compute_speed
		);

		/// <summary>
		/// Copy constructor for adt_information.
		/// </summary>
		/// <param name="a_adt_information"></param>
		agent_specific_information(
			const agent_specific_information& a_agent_specific_information
		);

	};
}
