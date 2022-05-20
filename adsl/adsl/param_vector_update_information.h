#pragma once
#include "affix-base/pch.h"
#include "affix-base/serializable.h"
#include "param_vector_information.h"

namespace adsl
{
	class param_vector_update_information : public affix_base::data::serializable
	{
	public:
		/// <summary>
		/// Information about the current param vector.
		/// </summary>
		param_vector_information m_param_vector_information;

		/// <summary>
		/// The update that should be applied to the parameter vector.
		/// </summary>
		param_vector_information m_update_vector_information;

	public:
		param_vector_update_information(

		);

		param_vector_update_information(
			const param_vector_information& a_param_vector_information,
			const param_vector_information& a_update_vector_information
		);

		param_vector_update_information(
			const param_vector_update_information& a_param_vector_update_information
		);

	};
}
