#include "affix-base/pch.h"
#include "affix-base/timing.h"
#include "affix-services/agent.h"
#include "param_vector_information.h"
#include "affix-base/files.h"
#include "aurora-v4/aurora.h"

using namespace aurora;

int main(

)
{
	asio::io_context l_io_context;

	affix_base::data::ptr<affix_services::client_configuration> l_client_configuration = new affix_services::client_configuration("");

	affix_services::client l_client(l_io_context, new affix_services::client_configuration(""));

	return 0;

}
