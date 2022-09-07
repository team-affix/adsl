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

	affix_services::client l_client(l_io_context, "config/client_configuration.json");

	affix_services::agent<std::string, double> l_agent(l_client, "adsl-example-agent", 1);




	return 0;

}
