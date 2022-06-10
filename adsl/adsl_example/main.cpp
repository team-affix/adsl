#include "affix-base/pch.h"
#include "affix-base/timing.h"
#include "affix-services/agent.h"
#include "aurora/pseudo.h"
#include "aurora/params.h"
#include "aurora/param_init.h"
#include "param_vector_information.h"
#include "affix-base/files.h"
#include "aurora/models.h"

class trainer
{
protected:
	struct update_information
	{
		adsl::param_vector_information m_param_vector_information;
		adsl::param_vector_information m_update_vector_information;
	};

	asio::io_context m_io_context;

	affix_base::data::ptr<affix_services::client> m_client;

	affix_base::data::ptr<affix_services::agent<double, std::string>> m_agent;

	std::map<std::string, update_information> m_synchronization_requests;

	volatile bool m_background_thread_continue = false;

	std::thread m_asio_thread;

	std::thread m_affix_services_thread;

	size_t m_ideal_mini_batch_size = 0;

	double m_learn_rate = 0;

public:
	~trainer(

	)
	{
		m_background_thread_continue = false;
		if (m_asio_thread.joinable())
			m_asio_thread.join();
		if (m_affix_services_thread.joinable())
			m_affix_services_thread.join();
	}

	trainer(
		const size_t& a_ideal_mini_batch_size,
		const double& a_learn_rate
	) :
		m_ideal_mini_batch_size(a_ideal_mini_batch_size),
		m_learn_rate(a_learn_rate)
	{
		if (!std::filesystem::exists("config/"))
			std::filesystem::create_directory("config/");
		affix_base::data::ptr<affix_services::client_configuration> l_client_configuration(new affix_services::client_configuration("config/client_configuration.json"));
		l_client_configuration->import_resource();
		l_client_configuration->export_resource();
		m_client = new affix_services::client(m_io_context, l_client_configuration);
		m_agent = new affix_services::agent<double, std::string>(*m_client, "adsl-example", get_absolute_compute_speed());
		m_agent->add_function("request_synchronize",
			std::function([&](
				std::string a_remote_client_identity,
				adsl::param_vector_information a_param_vector_information,
				adsl::param_vector_information a_update_vector_information
			)
				{
					if (m_agent->largest_identity() != m_client->m_local_identity)
					{
						// The local client is not locally recognized as being the distribution lead
						m_agent->invoke(
							a_remote_client_identity,
							"response_synchronize",
							adsl::param_vector_information(),
							false);
						return;
					}

					m_synchronization_requests.insert(m_synchronization_requests.end(), { a_remote_client_identity, { a_param_vector_information, a_update_vector_information } });

				}));

		m_agent->disclose_agent_information();

		m_background_thread_continue = true;

		m_asio_thread = std::thread(
			[&]
			{
				while (m_background_thread_continue)
				{
					m_io_context.run();
					m_io_context.reset();
				}
			});

		m_affix_services_thread = std::thread(
			[&]
			{
				while (m_background_thread_continue)
				{
					m_client->process();
					m_agent->process();

					std::scoped_lock l_lock(m_agent->m_guarded_data);

					if (m_synchronization_requests.size() >= m_agent->m_guarded_data->m_registered_agents.size())
					{
						m_agent->invoke_on_all("response_synchronize", process_synchronization_requests(), true);
					}
				}
			});

	}

	/// <summary>
	/// Returns an updated param_vector_information object.
	/// </summary>
	/// <param name="a_param_vector_information"></param>
	/// <param name="a_update_vector_information"></param>
	void synchronize(
		aurora::params::param_vector& a_param_vector,
		uint64_t& a_previous_training_sets_digested,
		const uint64_t& a_last_epoch_training_sets_digested
	)
	{
		adsl::param_vector_information l_result;
		bool l_contacted_distribution_lead = false;

		adsl::param_vector_information l_request_param_vector;
		adsl::param_vector_information l_request_update_vector;

		// Export a_param_vector into individual param_vector_information objects
		l_request_param_vector.m_param_vector.resize(a_param_vector.size());
		l_request_update_vector.m_param_vector.resize(a_param_vector.size());
		for (int i = 0; i < a_param_vector.size(); i++)
		{
			aurora::params::param_sgd* l_param = a_param_vector[i];
			l_request_param_vector.m_param_vector[i] = l_param->state();
			l_request_update_vector.m_param_vector[i] = l_param->gradient();
			l_param->gradient() = 0;
		}
		l_request_param_vector.m_training_sets_digested = a_previous_training_sets_digested;
		l_request_update_vector.m_training_sets_digested = a_last_epoch_training_sets_digested;

		// LOOP UNTIL WE CONTACT THE DISTRIBUTION LEAD SUCCESSFULLY
		while (
			!m_agent->synchronize(m_agent->largest_identity(), "request_synchronize", "response_synchronize",
				std::make_tuple(l_request_param_vector, l_request_update_vector),
				std::forward_as_tuple(l_result, l_contacted_distribution_lead)) ||
			l_contacted_distribution_lead == false);

		// Export new information back out of function
		for (int i = 0; i < l_result.m_param_vector.size(); i++)
		{
			a_param_vector[i]->state() = l_result.m_param_vector[i];
		}
		a_previous_training_sets_digested = l_result.m_training_sets_digested;

		m_synchronization_requests.clear();

	}

	size_t training_sets_to_digest_count(

	)
	{
		std::scoped_lock l_lock(m_agent->m_guarded_data);

		double l_total_absolute_compute_speed = 0;

		for (auto i : m_agent->m_guarded_data->m_registered_agents)
		{
			double l_registered_agent_compute_speed = 0;
			if (!i.second.get_parsed_agent_specific_information(l_registered_agent_compute_speed))
				continue;
			l_total_absolute_compute_speed += l_registered_agent_compute_speed;
		}

		double l_local_absolute_compute_speed = 0;

		if (!m_agent->m_guarded_data->m_local_agent_information.get_parsed_agent_specific_information(l_local_absolute_compute_speed) ||
			l_total_absolute_compute_speed == 0)
			return 0;

		return
			(double)m_ideal_mini_batch_size *
			(double)m_agent->m_guarded_data->m_registered_agents.size() *
			l_local_absolute_compute_speed /
			l_total_absolute_compute_speed;
	}

	void update_agent_specific_information(

	)
	{
		if (!m_agent->m_guarded_data->m_local_agent_information.set_parsed_agent_specific_information(get_absolute_compute_speed()))
			return;

		m_agent->disclose_agent_information();

	}

protected:
	double get_absolute_compute_speed(

	)
	{
		aurora::params::param_vector l_pv;

		aurora::models::Model l_model = aurora::pseudo::tnn({ 2, 5, 1 }, aurora::pseudo::nlr(0.3));

		aurora::models::Mse_loss l_mse_loss = new aurora::models::mse_loss(l_model);

		l_mse_loss->param_recur(aurora::pseudo::param_init(new aurora::params::param_sgd(0.02), l_pv));

		l_mse_loss->compile();

		l_pv.rand_norm();

		aurora::maths::tensor x = {
			{0.1, 0},
			{0, 1},
			{1, 0},
			{1, 1}
		};

		aurora::maths::tensor y = {
			{0},
			{1},
			{1},
			{0}
		};

		const int l_ITERATIONS = 100000;

		affix_base::timing::stopwatch l_stopwatch;

		l_stopwatch.start();

		for (int epoch = 0; epoch < l_ITERATIONS; epoch++)
		{
			l_mse_loss->cycle(x[0], y[0]);
			l_mse_loss->cycle(x[1], y[1]);
			l_mse_loss->cycle(x[2], y[2]);
			l_mse_loss->cycle(x[3], y[3]);
			l_pv.update();
		}

		return (double)l_ITERATIONS / (double)l_stopwatch.duration_microseconds();

	}

	adsl::param_vector_information process_synchronization_requests(

	)
	{
		adsl::param_vector_information l_result;

		// Select a synchronization base
		for (auto i : m_synchronization_requests)
		{
			if (i.second.m_param_vector_information.m_training_sets_digested >= l_result.m_training_sets_digested)
			{
				l_result = i.second.m_param_vector_information;
			}
		}

		for (auto l_request : m_synchronization_requests)
		{
			if (std::equal(
					l_request.second.m_param_vector_information.m_param_vector.begin(),
					l_request.second.m_param_vector_information.m_param_vector.end(),
					l_result.m_param_vector.begin(), l_result.m_param_vector.end()) &&
				l_request.second.m_update_vector_information.m_param_vector.size() == l_result.m_param_vector.size())
			{
				for (int i = 0; i < l_request.second.m_update_vector_information.m_param_vector.size(); i++)
					l_result.m_param_vector[i] -= m_learn_rate * l_request.second.m_update_vector_information.m_param_vector[i];
				l_result.m_training_sets_digested += l_request.second.m_update_vector_information.m_training_sets_digested;
			}
		}

		m_synchronization_requests.clear();

		return l_result;

	}


};

void get_random_training_set_from_disk(
	aurora::maths::tensor& a_x,
	aurora::maths::tensor& a_y
)
{
	CryptoPP::AutoSeededRandomPool l_random;

	while (true)
	{
		int l_training_set_count = 0;

		for (auto i : std::filesystem::directory_iterator("training_data/"))
			l_training_set_count++;

		if (l_training_set_count == 0)
			continue;

		int l_training_set_index = l_random.GenerateWord32(0, l_training_set_count - 1);

		auto l_iterator = std::filesystem::directory_iterator("training_data/");
		std::advance(l_iterator, l_training_set_index);

		aurora::maths::tensor l_training_set;

		if (affix_base::files::file_read(l_iterator->path().u8string(), l_training_set))
		{
			a_x = l_training_set[0];
			a_y = l_training_set[1];
			return;
		}

	}
}

int main(

)
{
	/*aurora::maths::tensor l_training_sets = {
		{ {0, 0}, {0} },
		{ {0, 1}, {1} },
		{ {1, 0}, {1} },
		{ {1, 1}, {0} }
	};

	for (int i = 0; i < l_training_sets.size(); i++)
	{
		affix_base::files::file_write("training_data/" + std::to_string(i), l_training_sets[i]);
	}

	return 0;*/

	aurora::params::param_vector l_param_vector;

	aurora::models::Model l_model = aurora::pseudo::tnn({ 2, 5, 1 }, aurora::pseudo::nlr(0.3));

	aurora::models::Mse_loss l_mse_loss = new aurora::models::mse_loss(l_model);

	l_mse_loss->param_recur(aurora::pseudo::param_init(new aurora::params::param_sgd(0.02), l_param_vector));

	l_mse_loss->compile();

	uint64_t l_param_vector_training_sets_digested = 0;

	{
		aurora::maths::tensor l_param_vector_tensor;

		if (affix_base::files::file_read("config/param_vector_tensor.bin", l_param_vector_tensor) &&
			affix_base::files::file_read("config/param_vector_training_sets_digested.bin", l_param_vector_training_sets_digested) &&
			l_param_vector.size() == l_param_vector_tensor.size())
		{
			l_param_vector.pop(l_param_vector_tensor);
		}
		else
		{
			l_param_vector.rand_norm();
		}
	}

	trainer l_trainer(20, 0.002);

	uint64_t l_epoch_training_sets_digested = 0;

	for (int epoch = 0; true; epoch++)
	{
		l_trainer.synchronize(l_param_vector, l_param_vector_training_sets_digested, l_epoch_training_sets_digested);

		affix_base::files::file_write("config/param_vector_tensor.bin", (aurora::maths::tensor)l_param_vector);
		affix_base::files::file_write("config/param_vector_training_sets_digested.bin", l_param_vector_training_sets_digested);

		l_epoch_training_sets_digested = l_trainer.training_sets_to_digest_count();

		double l_cost = 0;

		for (int i = 0; i < l_epoch_training_sets_digested; i++)
		{
			// GET A TRAINING SET FROM DISK
			aurora::maths::tensor l_x;
			aurora::maths::tensor l_y;
			get_random_training_set_from_disk(l_x, l_y);

			// TRAIN (CYCLE) THE TRAINING SET
			l_cost += l_mse_loss->cycle(l_x, l_y);

		}

		if (epoch % 100 == 0)
			std::cout << l_cost << std::endl;

	}

	return 0;

}
