#include "adsl/agent.h"
#include "aurora/tensor.h"
#include "adsl/training_set.h"
#include "aurora/models.h"
#include "aurora/pseudo.h"

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

	affix_services::client l_client(l_io_context,
		l_client_configuration_0);

	if (!fs::exists("training_data_0/"))
	{
		fs::create_directory("training_data_0/");
	}

	agent l_agent(
		l_client,
		"example-session-0",
		"example-session-0/param_vector.bin",
		"example-session-0/training-data/",
		100000,
		10);

	l_agent.disclose_agent_information();

	// Boolean describing whether the context thread should continue
	bool l_context_thread_continue = true;

	// Run context
	std::thread l_context_thread(
		[&]
		{
			while (l_context_thread_continue)
			{
				try
				{
					l_client.process();
					l_agent.process();
				}
				catch (std::exception a_exception)
				{
					std::cout << a_exception.what() << std::endl;
				}

				//asio::io_context::work l_idle_work(l_io_context);
				l_io_context.run();
				l_io_context.reset();
			}
		});


	std::uniform_real_distribution<double> l_urd(-1, 1);
	std::default_random_engine l_re(25);

	// Generate random training sets and add them to the session's training data
	/*while (true)
	{
		l_agent.invoke<training_set>(l_client.m_local_identity, function_types::training_set,
			training_set(
				aurora::maths::tensor::new_1d(2, l_urd, l_re),
				aurora::maths::tensor::new_1d(1, l_urd, l_re)
			));
	}*/

	////////////////////////////////////
	// Create param vector and model
	////////////////////////////////////

	param_vector_update_information l_param_vector_update_information;

	aurora::models::Ce_loss l_model = 
		new aurora::models::ce_loss(aurora::pseudo::tnn({ 2, 5, 1 }, aurora::pseudo::nlr(0.3)));

	aurora::params::param_vector l_param_vector;
	
	l_model->param_recur(aurora::pseudo::param_init(new aurora::params::param_mom(0.02, 0.9), l_param_vector));

	l_model->compile();


	// Prepare the storing of parameters in the exported param vector
	l_param_vector_update_information.m_param_vector_information.m_param_vector.resize(l_param_vector.size());


	if (!l_agent.try_get_param_vector_information_from_disk(l_param_vector_update_information.m_param_vector_information))
	{
		l_param_vector.rand_norm();
		for (int i = 0; i < l_param_vector.size(); i++)
			l_param_vector_update_information.m_param_vector_information.m_param_vector[i] = l_param_vector[i]->state();
	}



	while (true)
	{
		std::cout 
			<< "SYNCHRONIZING PARAM VECTOR; param vector training sets digested: " 
			<< l_param_vector_update_information.m_param_vector_information.m_training_sets_digested 
			<< ", update vector training sets digested: " 
			<< l_param_vector_update_information.m_update_vector_information.m_training_sets_digested 
			<< std::endl;

		l_param_vector_update_information.m_param_vector_information = l_agent.synchronize(l_param_vector_update_information);

		if (!l_agent.try_set_param_vector_information_in_disk(l_param_vector_update_information.m_param_vector_information))
			std::cout << "Could not write param vector information to disk." << std::endl;

		// Clear the update vector information
		l_param_vector_update_information.m_update_vector_information = param_vector_information();

		// Populate param vector with newly updated param vector states
		for (int i = 0; i < l_param_vector.size(); i++)
		{
			l_param_vector[i]->state() = l_param_vector_update_information.m_param_vector_information.m_param_vector[i];
		}

		//////////////////////////////////
		// Actually train the model
		//////////////////////////////////

		size_t l_training_sets_to_digest = l_agent.training_sets_to_digest_count();

		for (int i = 0; i < l_training_sets_to_digest; i++)
		{
			training_set l_training_set;

			if (!l_agent.try_get_random_training_set_from_disk(l_training_set))
				continue;

			l_model->cycle(l_training_set.m_x, l_training_set.m_y);

		}


		// Synchronize with the distribution lead
		
		// Prepare for storing of parameter graident in exported vector form
		l_param_vector_update_information.m_update_vector_information.m_param_vector.resize(l_param_vector.size());

		for (int i = 0; i < l_param_vector.size(); i++)
		{
			l_param_vector_update_information.m_update_vector_information.m_param_vector[i] = 0.008 * ((aurora::params::param_mom*)l_param_vector[i])->gradient();
		}

		// Save how many training sets were digested in this epoch
		l_param_vector_update_information.m_update_vector_information.m_training_sets_digested = l_training_sets_to_digest;

	}

	l_context_thread_continue = false;

	if (l_context_thread.joinable())
		l_context_thread.join();

	return 0;

}
