#pragma once
#include "affix-base/pch.h"
#include "aurora/tensor.h"
#include "affix-base/serializable.h"

namespace adsl
{
	class training_set : public affix_base::data::serializable
	{
	public:
		aurora::maths::tensor m_x;
		aurora::maths::tensor m_y;

	public:
		/// <summary>
		/// Constructs the training_set object with default initialization of the fields.
		/// </summary>
		training_set(

		);

		/// <summary>
		/// Constructs the training_set object with the argued values for the fields.
		/// </summary>
		/// <param name="a_x"></param>
		/// <param name="a_y"></param>
		training_set(
			const aurora::maths::tensor& a_x,
			const aurora::maths::tensor& a_y
		);

		/// <summary>
		/// Copy constructor for the training_set object.
		/// </summary>
		/// <param name="a_training_set"></param>
		training_set(
			const training_set& a_training_set
		);

		/// <summary>
		/// Returns a vector of bytes representing the sha256 hash of the 
		/// training set data.
		/// </summary>
		/// <returns></returns>
		std::vector<uint8_t> hash(

		) const;

	};
}
