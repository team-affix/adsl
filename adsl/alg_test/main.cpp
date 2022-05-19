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

	affix_base::data::ptr<affix_services::client_configuration>
		l_client_configuration_1(new affix_services::client_configuration("config/client_configuration_1.json"));

	l_client_configuration_1->import_resource();
	l_client_configuration_1->export_resource();


	affix_services::client l_client_0(l_io_context,
		l_client_configuration_0);

	if (!fs::exists("training_data_0/"))
	{
		fs::create_directory("training_data_0/");
	}

	agent l_agent_0(l_client_0, "example-session-0", "training_data_0/");
	l_agent_0.disclose_agent_information();


	affix_services::client l_client_1(l_io_context,
		l_client_configuration_1);

	if (!fs::exists("training_data_1/"))
	{
		fs::create_directory("training_data_1/");
	}

	agent l_agent_1(l_client_1, "example-session-0", "training_data_1/");
	l_agent_1.disclose_agent_information();


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

	std::uniform_real_distribution<double> l_urd(-1, 1);
	std::default_random_engine l_re(25);

	for (int l_message_index = 0; true;)
	{
		try
		{
			l_client_0.process();
			l_client_1.process();

			l_agent_0.process();
			l_agent_1.process();
		}
		catch (std::exception a_exception)
		{

		}

		/*if (l_client_1.fastest_path_to_identity(l_client_0.m_local_identity).size() > 0)
		{
			l_agent_1.invoke<adsl::training_set>(l_client_0.m_local_identity, function_types::training_set, adsl::training_set(aurora::maths::tensor::new_1d(10, l_urd, l_re), aurora::maths::tensor::new_1d(10, l_urd, l_re)));
			l_message_index++;
		}*/

		std::vector<training_set> l_training_sets = l_agent_0.get_training_sets();

		if (l_message_index >= 100)
		{
			affix_base::threading::locked_resource l_authenticated_connections = l_client_1.m_authenticated_connections.lock();

			for (int i = l_authenticated_connections->size() - 1; i >= 0; i--)
			{
				l_authenticated_connections->at(i)->close();
			}

		}

	}


	l_context_thread_continue = false;
	if (l_context_thread.joinable())
		l_context_thread.join();

	return 0;

}
