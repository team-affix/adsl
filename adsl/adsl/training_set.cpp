#include "training_set.h"
#include "affix-base/sha.h"

using namespace adsl;

training_set::training_set(

) :
	affix_base::data::serializable(m_x, m_y)
{

}

training_set::training_set(
	const aurora::maths::tensor& a_x,
	const aurora::maths::tensor& a_y
) :
	affix_base::data::serializable(m_x, m_y),
	m_x(a_x),
	m_y(a_y)
{

}

training_set::training_set(
	const training_set& a_training_set
) :
	training_set(
		a_training_set.m_x,
		a_training_set.m_y)
{

}

std::vector<uint8_t> training_set::hash(

) const
{
	affix_base::data::byte_buffer l_byte_buffer;
	l_byte_buffer.push_back(m_x);
	l_byte_buffer.push_back(m_y);
	std::vector<uint8_t> l_result;
	affix_base::data::sha256_digest(l_byte_buffer.data(), l_result);
	return l_result;
}
