#include "affix-base/pch.h"
#include "affix-base/timing.h"
#include "affix-services/agent.h"
#include "aurora/pseudo.h"
#include "aurora/params.h"
#include "aurora/param_init.h"
#include "param_vector_information.h"
#include "affix-base/files.h"
#include "aurora/models.h"

int main(

)
{
	namespace fs = std::filesystem;

	asio::io_context l_context;

	// Create folders if they do not already exist
	if (!fs::exists("config/"))
		fs::create_directory("config/");
	if (!fs::exists("training_data/"))
		fs::create_directory("training_data/");

	affix_base::data::ptr<affix_services::client_configuration> l_client_configuration(new affix_services::client_configuration("config/client_configuration.json"));
	l_client_configuration->import_resource();
	l_client_configuration->export_resource();

	affix_services::client l_client(l_context, l_client_configuration);

	// Function which returns the compute speed of the local machine.
	auto l_compute_speed = []() -> double
	{
		affix_base::timing::stopwatch l_stopwatch;

		const int ITERATIONS = 1000000000;

		double l_0 = 1.2;
		double l_1 = 13.6;
		double l_2 = 0;

		l_stopwatch.start();

		for (int i = 0; i < ITERATIONS; i++)
		{
			l_2 = l_0 + l_1;
			l_2 = l_0 * l_1;
		}

		return (double)ITERATIONS / (double)l_stopwatch.duration_milliseconds();

	};

	affix_services::agent<double, std::string> l_agent(l_client, "adsl-example", l_compute_speed());

	auto normalized_compute_speed = [&]() -> double
	{
		std::scoped_lock l_lock(l_agent.m_guarded_data);

		double l_total_compute_speed = 0;

		for (auto i : l_agent.m_guarded_data->m_registered_agents)
		{
			double l_registered_agent_compute_speed = 0;
			if (!i.second.get_parsed_agent_specific_information(l_registered_agent_compute_speed))
				continue;
			l_total_compute_speed += l_registered_agent_compute_speed;
		}

		double l_local_agent_compute_speed = 0;

		if (!l_agent.m_guarded_data->m_local_agent_information.get_parsed_agent_specific_information(l_local_agent_compute_speed))
			return 0;

		return l_local_agent_compute_speed / l_total_compute_speed;

	};

	l_agent.disclose_agent_information();

	std::vector<std::pair<adsl::param_vector_information, adsl::param_vector_information>> l_synchronization_requests;
	affix_base::data::ptr<adsl::param_vector_information> l_synchronized;

	l_agent.add_function("request_synchronize",
		std::function([&](std::string, adsl::param_vector_information a_param_vector_information, adsl::param_vector_information a_update_vector_information)
			{
				l_synchronization_requests.push_back({ a_param_vector_information, a_update_vector_information });
			}));

	l_agent.add_function("response_synchronize",
		std::function([&](std::string, adsl::param_vector_information a_param_vector_information)
			{
				l_synchronized = new adsl::param_vector_information(a_param_vector_information);
			}));

	auto l_synchronize = [&]() -> adsl::param_vector_information
	{
		adsl::param_vector_information l_result;

		// Gets the basis for synchronization based on most sophisticated param vector.
		for (const auto& i : l_synchronization_requests)
		{
			if (i.first.m_training_sets_digested >= l_result.m_training_sets_digested)
				l_result = i.first;
		}

		for (const auto& l_synchronization_request : l_synchronization_requests)
		{
			if (std::equal(
				l_synchronization_request.first.m_param_vector.begin(), l_synchronization_request.first.m_param_vector.end(),
				l_result.m_param_vector.begin(), l_result.m_param_vector.end()) &&
				l_synchronization_request.second.m_param_vector.size() == l_result.m_param_vector.size())
			{
				for (int i = 0; i < l_synchronization_request.first.m_param_vector.size(); i++)
				{
					l_result.m_param_vector[i] -= 0.002 * l_synchronization_request.second.m_param_vector[i];
				}
				l_result.m_training_sets_digested += l_synchronization_request.second.m_training_sets_digested;
			}
		}

		l_synchronization_requests.clear();

		return l_result;

	};

	std::thread l_thread(
		[&]
		{
			while (true)
			{
				l_context.run();
				l_context.reset();
			}
		});

	std::thread l_thread_2(
		[&]
		{
			while (true)
			{
				l_client.process();
				l_agent.process();

				std::scoped_lock l_lock(l_agent.m_guarded_data);

				if (l_synchronization_requests.size() >= l_agent.m_guarded_data->m_registered_agents.size())
				{
					l_agent.invoke_on_all("response_synchronize", l_synchronize());
				}
			}
		});

	adsl::param_vector_information l_param_vector_information;
	adsl::param_vector_information l_update_vector_information;

	auto l_synchronize_with_dl = [&]
	{
		l_synchronized = nullptr;
		
		std::string l_previous_distribution_lead;

		while (l_synchronized == nullptr)
		{
			std::string l_current_distribution_lead = l_agent.largest_identity();

			if (l_current_distribution_lead != l_previous_distribution_lead)
			{
				l_previous_distribution_lead = l_current_distribution_lead;

				l_agent.invoke(l_previous_distribution_lead, "request_synchronize", l_param_vector_information, l_update_vector_information);

			}

		}

	};




	// Create param vector
	aurora::params::param_vector l_pv;

	auto l_import_params = [&]
	{
		for (int i = 0; i < l_pv.size(); i++)
			l_pv[i]->state() = l_param_vector_information.m_param_vector[i];
	};

	auto l_export_params = [&]
	{
		l_param_vector_information.m_param_vector.resize(l_pv.size());
		for (int i = 0; i < l_pv.size(); i++)
			l_param_vector_information.m_param_vector[i] = l_pv[i]->state();
	};

	auto l_export_gradient = [&]
	{
		l_update_vector_information.m_param_vector.resize(l_pv.size());
		for (int i = 0; i < l_pv.size(); i++)
		{
			l_update_vector_information.m_param_vector[i] = ((aurora::params::param_sgd*)l_pv[i])->gradient();
			((aurora::params::param_sgd*)l_pv[i])->gradient() = 0;
		}
	};



	// Create model
	aurora::models::Mse_loss l_mse_loss(aurora::pseudo::tnn({ 2, 5, 1 }, aurora::pseudo::nlr(0.3)));

	l_mse_loss->param_recur(aurora::pseudo::param_init(new aurora::params::param_sgd(0.02), l_pv));

	l_mse_loss->compile();


	// Import params from file
	if (affix_base::files::file_read("config/param_vector.bin", l_param_vector_information) &&
		l_param_vector_information.m_param_vector.size() == l_pv.size())
	{
		l_import_params();
	}
	else
	{
		l_pv.rand_norm();
		l_export_params();
	}

	for (int i = 0; true; i++)
	{
		l_synchronize_with_dl();
		affix_base::files::file_write("config/param_vector.bin", l_param_vector_information);
		l_import_params();

		double l_cost = 0;
		aurora::maths::tensor x = { 0,0 };
		aurora::maths::tensor y = { 0 };
		l_cost += l_mse_loss->cycle(x, y);
		l_cost += l_mse_loss->cycle({ 0, 1 }, { 1 });
		l_cost += l_mse_loss->cycle({ 1, 0 }, { 1 });
		l_cost += l_mse_loss->cycle({ 1, 1 }, { 0 });

		if (i % 10 == 0)
			std::cout << l_cost << std::endl;

		l_param_vector_information = *l_synchronized;
		l_export_gradient();
		l_update_vector_information.m_training_sets_digested += 4;
	}

	return 0;
}
