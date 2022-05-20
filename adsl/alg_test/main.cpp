#include "adsl/agent.h"
#include "aurora/tensor.h"
#include "adsl/training_set.h"

int main(

)
{
	using namespace adsl;
	namespace fs = std::filesystem;

	asio::io_context l_io_context;

	if (!fs::exists("config/"))
	{
		// If config directory doesn't exist, create it.
		fs::create_directory("config/");
	};

	affix_base::data::ptr<affix_services::client_configuration>
		l_client_configuration_0(new affix_services::client_configuration("config/client_configuration_0.json"));

	l_client_configuration_0->import_resource();
	l_client_configuration_0->export_resource();

	affix_services::client l_client_0(l_io_context,
		l_client_configuration_0);

	if (!fs::exists("training_data_0/"))
	{
		fs::create_directory("training_data_0/");
	}

	agent l_agent_0(
		l_client_0,
		"example-session-0",
		"training_data_0/",
		100000,
		1000,
		10);
	l_agent_0.disclose_agent_information();

	// Boolean describing whether the context thread should continue
	bool l_context_thread_continue = true;

	// Run context
	std::thread l_context_thread(
		[&]
		{
			while (l_context_thread_continue)
			{
				asio::io_context::work l_idle_work(l_io_context);
				l_io_context.run();
			}
		});

	// Boolean describing whether the process thread should continue
	bool l_process_thread_continue = true;

	std::thread l_process_thread(
		[&]
		{
			while (l_process_thread_continue)
			{
				try
				{
					l_client_0.process();
					l_agent_0.process();
				}
				catch (std::exception a_exception)
				{
					std::cout << a_exception.what() << std::endl;
				}
			}
		});

	std::uniform_real_distribution<double> l_urd(-1, 1);
	std::default_random_engine l_re(25);

	while (true)
	{

	}





	l_context_thread_continue = false;
	l_process_thread_continue = false;

	if (l_context_thread.joinable())
		l_context_thread.join();

	if (l_process_thread.joinable())
		l_process_thread.join();

	return 0;

}
