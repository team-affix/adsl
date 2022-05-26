#include "trainer.h"
#include "aurora/models.h"
#include "aurora/params.h"
#include "aurora/pseudo.h"
#include "affix-base/timing.h"
#include "affix-base/files.h"

using namespace adsl;

std::string base64_from_bytes(
	const std::vector<uint8_t>& a_bytes
)
{
	std::string l_base64_encoded;
	CryptoPP::Base64Encoder l_encoder;
	l_encoder.Put(a_bytes.data(), a_bytes.size());
	l_base64_encoded.resize(l_encoder.MaxRetrievable());
	l_encoder.Get((CryptoPP::byte*)l_base64_encoded.data(), l_base64_encoded.size());
	return l_base64_encoded;
}

std::string file_name_from_hash(
	const std::vector<uint8_t>& a_hash
)
{
	return affix_base::data::string_trim(base64_from_bytes(a_hash), { '\\','/',':','*','?','\"','<','>','|' });
}

std::string larger_identity(
	const std::string& a_identity_0,
	const std::string& a_identity_1
)
{
	if (a_identity_0.empty())
		return a_identity_1;
	if (a_identity_1.empty())
		return a_identity_0;

	for (int i = 0; i < a_identity_0.size(); i++)
	{
		if (a_identity_0[i] != a_identity_1[i])
		{
			if ((uint8_t)a_identity_0[i] > (uint8_t)a_identity_1[i])
				return a_identity_0;
			else
				return a_identity_1;
		}
	}

	return "";

}


trainer::trainer(
	const std::string& a_client_json_file_path,
	const std::string& a_session_name,
	const std::function<std::vector<double>(std::vector<double>)>& a_set_param_vector,
	const std::function<double(training_set)>& a_cycle,
	const std::function<std::vector<double>()>& a_get_update_vector,
	const uint64_t& a_compute_speed_test_interval_in_seconds
) :
	m_session_name(a_session_name),
	m_client(m_io_context, new affix_services::client_configuration(a_client_json_file_path)),
	m_agent(m_client, "adsl-" + a_session_name, adsl::agent_specific_information(training_set_hashes(), absolute_compute_speed())),
	m_set_param_vector(a_set_param_vector),
	m_cycle(a_cycle),
	m_get_update_vector(a_get_update_vector),
	m_compute_speed_test_interval_in_seconds(a_compute_speed_test_interval_in_seconds)
{
	using namespace affix_base::threading;
	using namespace affix_base::files;

	m_agent.m_remote_invocation_processor.add_function(
		function_types::training_set,
		std::function([&](
			std::string a_remote_client_identity,
			training_set a_training_set
			)
			{
				locked_resource l_parsed_agent_information = m_agent.m_local_agent_information.m_parsed_agent_specific_information.lock();

				std::vector<uint8_t> l_hash = a_training_set.hash();

				auto l_entry = std::find_if(l_parsed_agent_information->m_training_set_hashes.begin(),
					l_parsed_agent_information->m_training_set_hashes.end(),
					[&](const std::vector<uint8_t>& a_entry)
					{
						return std::equal(a_entry.begin(), a_entry.end(), l_hash.begin(), l_hash.end());
					});

				if (l_entry != l_parsed_agent_information->m_training_set_hashes.end())
					// Do nothing, (ignore this request), a local copy of the training set already exists. just return.
					return;

				// Log the fact that we received a new training set.
				std::clog << "[ ADSL ] Received training set." << std::endl;

				if (!write_training_set_to_disk(l_hash, a_training_set))
				{
					std::clog << "[ ADSL ] Failed to write training set to disk." << std::endl;
					return;
				}

			}));

	m_agent.m_remote_invocation_processor.add_function(
		function_types::request_synchronize,
		std::function([&](
			std::string a_remote_client_identity,
			param_vector_update_information a_param_vector_update_information
			)
			{
				const_locked_resource l_remote_agents = m_agent.m_remote_agents.const_lock();

				m_param_vector_updates.insert({ a_remote_client_identity, a_param_vector_update_information });

				if (m_param_vector_updates.size() >= l_remote_agents->size())
				{
					// Synchronize all parameter vector updates and apply relevant updates to param vector.
					param_vector_information l_updated_param_vector = apply_param_vector_updates();

					for (auto l_agent_iterator = l_remote_agents->begin(); l_agent_iterator != l_remote_agents->end(); l_agent_iterator++)
					{
						m_agent.invoke<param_vector_information>(
							l_agent_iterator->first,
							function_types::response_synchronize,
							l_updated_param_vector
						);
					}

					m_param_vector_updates.clear();

				}

			}));

	m_agent.m_remote_invocation_processor.add_function(
		function_types::response_synchronize,
		std::function([&](
			std::string a_remote_client_identity,
			param_vector_information a_updated_param_vector_information)
			{
				locked_resource l_synchronize_callback = m_synchronize_callback.lock();

				if (*l_synchronize_callback == nullptr)
					// No callback was set.
					return;

				// Call the synchronize callback.
				(*l_synchronize_callback)(a_updated_param_vector_information);

				// Reset the callback.
				*l_synchronize_callback = nullptr;

			}));

	m_context_thread = std::thread(
		[&]
		{
			while (true)
			{
				// Repeatedly run the io context
				asio::io_context::work l_idlework(m_io_context);
				m_io_context.run();
				m_io_context.reset();
			}
		});

	m_process_thread = std::thread(
		[&]
		{
			while (true)
			{
				m_client.process();
				m_agent.process();
			}
		});

	// Create necessary directories if they do not already exist
	std::filesystem::create_directory(session_root_path());
	std::filesystem::create_directory(training_sets_path());

	/*std::uniform_real_distribution<double> l_urd(-1, 1);
	std::default_random_engine l_dre(25);

	for (int i = 0; i < 10000; i++)
	{
		training_set l_training_set(
			aurora::maths::tensor::new_1d(2, l_urd, l_dre),
			aurora::maths::tensor::new_1d(1, l_urd, l_dre));
		write_training_set_to_disk(l_training_set.hash(), l_training_set);
	}*/

	m_agent.disclose_agent_information();

	while (true)
	{
		// Actually train the model
		process_epoch();
	}
}

std::string trainer::session_root_path(

) const
{
	return "adsl-" + m_session_name + "/";
}

std::string trainer::training_sets_path(

) const
{
	return session_root_path() + "training-data/";
}

std::string trainer::param_vector_path(

) const
{
	return session_root_path() + "param_vector.bin";
}

bool trainer::write_param_vector_information_to_disk(
	const param_vector_information& a_param_vector_information
) const
{
	return affix_base::files::file_write(param_vector_path(), a_param_vector_information);
}

bool trainer::read_param_vector_information_from_disk(
	param_vector_information& a_param_vector_information
) const
{
	return affix_base::files::file_read(param_vector_path(), a_param_vector_information);
}

double trainer::absolute_compute_speed(

)
{
	if (affix_base::timing::utc_time() - m_compute_speed_test_last_time_taken 
		< m_compute_speed_test_interval_in_seconds)
	{
		// Don't perform the test, just return the cached value
		return
			m_agent.m_local_agent_information.m_parsed_agent_specific_information.const_lock()->m_compute_speed;
	}

	m_compute_speed_test_last_time_taken = affix_base::timing::utc_time();

	const size_t l_iterations = 10000;

	using namespace aurora;
	using namespace aurora::models;
	using namespace aurora::params;

	Model l_model = pseudo::tnn({ 2, 20, 1 }, pseudo::nlr(0.3));
	param_vector l_pv;
	l_model->param_recur(pseudo::param_init(new param_mom(0.02, 0.9), l_pv));
	l_pv.rand_norm();
	l_model->compile();

	affix_base::timing::stopwatch l_stopwatch;
	l_stopwatch.start();

	for (int i = 0; i < l_iterations; i++)
	{
		l_model->fwd();
		l_model->bwd();
	}

	double l_compute_duration = l_stopwatch.duration_microseconds();

	double l_compute_speed = ((double)l_iterations) / l_compute_duration;
	return l_compute_speed;

}

double trainer::normalized_compute_speed(

)
{
	using namespace affix_base::threading;

	while (true)
	{
		const_locked_resource l_local_agent_information = m_agent.m_local_agent_information.m_parsed_agent_specific_information.const_lock();

		double l_total_compute_speed = 0;

		locked_resource l_remote_agents = m_agent.m_remote_agents.lock();

		for (auto i = l_remote_agents->begin(); i != l_remote_agents->end(); i++)
		{
			const_locked_resource l_remote_agent_information = i->second->m_parsed_agent_specific_information.const_lock();

			l_total_compute_speed += l_remote_agent_information->m_compute_speed;

		}

		if (l_total_compute_speed >= l_local_agent_information->m_compute_speed)
			return l_local_agent_information->m_compute_speed / l_total_compute_speed;

	}

}

std::vector<std::vector<uint8_t>> trainer::training_set_hashes(

)
{
	std::vector<std::vector<uint8_t>> l_result;

	namespace fs = std::filesystem;

	using namespace affix_base::files;

	if (!fs::exists(training_sets_path()))
	{
		std::clog << "[ ADSL ] Error; training data folder does not exist: " << std::filesystem::absolute(training_sets_path()) << std::endl;
		return l_result;
	}

	for (const auto& a_entry : fs::directory_iterator(training_sets_path()))
	{
		training_set l_training_set;

		if (!file_read(a_entry.path().u8string(), l_training_set))
		{
			std::clog << "[ ADSL ] Error; failed to read training set from file: " << fs::absolute(a_entry.path()).u8string() << std::endl;
			continue;
		}

		// Push hash of training set to vector
		l_result.push_back(l_training_set.hash());

	}

	return l_result;

}

bool trainer::write_training_set_to_disk(
	const std::vector<uint8_t>& a_hash,
	const training_set& a_training_set
)
{
	return write_training_set_to_disk(
		training_sets_path() + file_name_from_hash(a_hash),
		a_training_set
	);

}

bool trainer::read_training_set_from_disk(
	const std::vector<uint8_t>& a_hash,
	training_set& a_training_set
)
{
	return read_training_set_from_disk(
		training_sets_path() + file_name_from_hash(a_hash),
		a_training_set
	);
}

bool trainer::write_training_set_to_disk(
	const std::string& a_path,
	const training_set& a_training_set
)
{
	return affix_base::files::file_write(a_path, a_training_set);

}

bool trainer::read_training_set_from_disk(
	const std::string& a_path,
	training_set& a_training_set
)
{
	return affix_base::files::file_read(a_path, a_training_set);

}

std::string trainer::await_distribution_lead(

)
{
	while (true)
	{
		affix_base::threading::const_locked_resource l_registered_agents = m_agent.m_remote_agents.const_lock();

		std::string l_max_identity_value;

		for (auto i = l_registered_agents->begin(); i != l_registered_agents->end(); i++)
			l_max_identity_value = larger_identity(l_max_identity_value, i->first);

		if (!l_max_identity_value.empty())
			return l_max_identity_value;

	}
}

void trainer::synchronize_local_training_sets(

)
{
	using namespace affix_base::threading;

	// Get map of remote client identities to their associated agents
	locked_resource l_remote_agents = m_agent.m_remote_agents.lock();

	// Get distribution lead identity
	std::string l_distribution_lead_identity = await_distribution_lead();

	if (l_distribution_lead_identity == m_client.m_local_identity)
	{
		// If the local agent is the distribution lead, send training sets to all agents

		for (auto l_agent_iterator = l_remote_agents->begin(); l_agent_iterator != l_remote_agents->end(); l_agent_iterator++)
		{
			synchronize_local_training_sets(l_agent_iterator->first);
		}

	}
	else if (!l_distribution_lead_identity.empty())
	{
		// If the local agent is not the distribution lead, send training sets only to the DL.

		synchronize_local_training_sets(l_distribution_lead_identity);

	}

}

void trainer::synchronize_local_training_sets(
	const std::string& a_remote_client_identity
)
{
	using namespace affix_base::threading;
	using namespace affix_base::files;
	namespace fs = std::filesystem;

	// Get map of remote client identities to their associated agents
	locked_resource l_remote_agents = m_agent.m_remote_agents.lock();

	// Get the local agent information
	const_locked_resource l_local_agent_information = m_agent.m_local_agent_information.m_parsed_agent_specific_information.const_lock();

	auto l_agent_iterator = l_remote_agents->find(a_remote_client_identity);

	// Get the remote agent's parsed information.
	const_locked_resource l_remote_agent_parsed_information = l_agent_iterator->second->m_parsed_agent_specific_information.const_lock();

	for (int i = 0; i < l_local_agent_information->m_training_set_hashes.size(); i++)
	{
		const std::vector<uint8_t>& l_local_hash = l_local_agent_information->m_training_set_hashes.at(i);

		// Try to get an entry for the local training set having been registered with the remote agent.
		auto l_entry = std::find_if(l_remote_agent_parsed_information->m_training_set_hashes.begin(),
			l_remote_agent_parsed_information->m_training_set_hashes.end(),
			[&](const std::vector<uint8_t>& a_hash)
			{
				return std::equal(a_hash.begin(), a_hash.end(), l_local_hash.begin(), l_local_hash.end());
			});

		if (l_entry == l_remote_agent_parsed_information->m_training_set_hashes.end())
		{
			// Remote agent DOES NOT have the training set in question.

			training_set l_local_training_set;

			std::string l_training_set_path = training_sets_path() + file_name_from_hash(l_local_hash);

			if (!file_read(l_training_set_path, l_local_training_set))
			{
				std::clog << "[ ADSL ] Error; failed to read training set from file: " << fs::absolute(l_training_set_path).u8string() << std::endl;
				continue;
			}

			// Write message to log
			std::clog << "[ ADSL ] Sending training set to remote agent: " << a_remote_client_identity.substr(50, 10) << "; hash: " << base64_from_bytes(l_local_hash) << std::endl;

			// Send the training set to the remote agent.
			m_agent.invoke(a_remote_client_identity, function_types::training_set, l_local_training_set);

		}

	}

}

size_t trainer::training_sets_to_digest_count(

)
{
	size_t l_training_set_hash_count = m_agent.m_local_agent_information.m_parsed_agent_specific_information.const_lock()->m_training_set_hashes.size();

	return (size_t)((double)l_training_set_hash_count * normalized_compute_speed());

}

param_vector_information trainer::synchronize_param_vector(
	const param_vector_update_information& a_param_vector_update_information
)
{
	using namespace affix_base::threading;

	param_vector_information l_updated_param_vector_information;

	volatile bool l_callback_completed = false;

	// Set the callback function
	{
		locked_resource l_synchronize_callback = m_synchronize_callback.lock();
		*l_synchronize_callback = [&](param_vector_information a_updated_param_vector_information)
		{
			l_updated_param_vector_information = a_updated_param_vector_information;
			l_callback_completed = true;
		};
	}

	// Invoke the update request.
	m_agent.invoke<param_vector_update_information>(
		await_distribution_lead(),
		function_types::request_synchronize,
		a_param_vector_update_information
	);

	// Wait for the callback to complete
	while (!l_callback_completed);

	return l_updated_param_vector_information;

}

param_vector_information trainer::apply_param_vector_updates(

)
{
	param_vector_information l_param_vector_information;

	for (auto i = m_param_vector_updates.begin(); i != m_param_vector_updates.end(); i++)
	{
		if (i->second.m_param_vector_information.m_training_sets_digested >= l_param_vector_information.m_training_sets_digested)
		{
			l_param_vector_information = i->second.m_param_vector_information;
		}
	}

	for (auto l_iterator = m_param_vector_updates.begin(); l_iterator != m_param_vector_updates.end(); l_iterator++)
	{
		if (std::equal(
			l_iterator->second.m_param_vector_information.m_param_vector.begin(),
			l_iterator->second.m_param_vector_information.m_param_vector.end(),
			l_param_vector_information.m_param_vector.begin(),
			l_param_vector_information.m_param_vector.end()) &&
			l_iterator->second.m_param_vector_information.m_training_sets_digested == l_param_vector_information.m_training_sets_digested &&
			l_iterator->second.m_update_vector_information.m_param_vector.size() == l_param_vector_information.m_param_vector.size())
		{
			for (int i = 0; i < l_param_vector_information.m_param_vector.size(); i++)
			{
				l_param_vector_information.m_param_vector[i] -=
					l_iterator->second.m_update_vector_information.m_param_vector[i];
			}
			l_param_vector_information.m_training_sets_digested +=
				l_iterator->second.m_update_vector_information.m_training_sets_digested;
		}
	}

	return l_param_vector_information;

}

bool trainer::read_random_training_set_from_disk(
	training_set& a_training_set
)
{
	size_t l_total_training_sets = 0;

	for (auto l_entry : std::filesystem::directory_iterator(training_sets_path()))
	{
		l_total_training_sets++;
	}

	size_t l_training_set_index = m_random.GenerateWord32(0, l_total_training_sets - 1);

	auto l_directory_iterator = std::filesystem::directory_iterator(training_sets_path());
	std::advance(l_directory_iterator, l_training_set_index);

	// Import this training set.
	return read_training_set_from_disk(l_directory_iterator->path().u8string(), a_training_set);

}

bool trainer::refresh_agent_specific_information(

)
{
	std::clog << "[ ADSL ] Performing standardized compute speed test." << std::endl;
	double l_compute_speed = absolute_compute_speed();
	std::clog << "[ ADSL ] Absolute compute speed: " << l_compute_speed << std::endl;

	std::clog << "[ ADSL ] Collecting training set hashes from disk." << std::endl;
	std::vector<std::vector<uint8_t>> l_training_set_hashes = training_set_hashes();
	std::clog << "[ ADSL ] Collected: " << l_training_set_hashes.size() << " training set hashes from disk." << std::endl;

	bool l_asi_changed = false;

	{
		using namespace affix_base::threading;

		// Save new info
		locked_resource l_agent_specific_information = m_agent.m_local_agent_information.m_parsed_agent_specific_information.lock();

		l_asi_changed =
			l_agent_specific_information->m_compute_speed != l_compute_speed
			|| !std::equal(
				l_agent_specific_information->m_training_set_hashes.begin(),
				l_agent_specific_information->m_training_set_hashes.end(),
				l_training_set_hashes.begin(),
				l_training_set_hashes.end()
			);

		l_agent_specific_information->m_compute_speed = l_compute_speed;
		l_agent_specific_information->m_training_set_hashes = l_training_set_hashes;

	}

	return l_asi_changed;

}

void trainer::process_epoch(

)
{
	param_vector_update_information l_param_vector_update_information;
	
	if (!read_param_vector_information_from_disk(l_param_vector_update_information.m_param_vector_information))
	{
		std::clog << "[ ADSL ] Error; unable to read param vector from disk." << std::endl;
		// keep going, even though this failure happened.
	}

	l_param_vector_update_information.m_param_vector_information.m_param_vector = m_set_param_vector(l_param_vector_update_information.m_param_vector_information.m_param_vector);

	const size_t l_training_sets_to_digest = training_sets_to_digest_count();

	double l_cost = 0;

	for (int i = 0; i < l_training_sets_to_digest; i++)
	{
		training_set l_training_set;

		if (!read_random_training_set_from_disk(l_training_set))
		{
			std::clog << "[ ADSL ] Error; unable to read training set from disk." << std::endl;
			continue;
		}

		l_cost += m_cycle(l_training_set);

	}

	std::cout << "[ ADSL ] local cost: " << l_cost << std::endl;

	l_param_vector_update_information.m_update_vector_information.m_param_vector = m_get_update_vector();
	l_param_vector_update_information.m_update_vector_information.m_training_sets_digested = l_training_sets_to_digest;

	param_vector_information l_updated_param_vector_information =
		synchronize_param_vector(l_param_vector_update_information);

	if (!write_param_vector_information_to_disk(l_updated_param_vector_information))
	{
		std::clog << "[ ADSL ] Error; failed to write the param vector to disk." << std::endl;
	}

	// Refresh ASI and if it has changed, disclose the new ASI
	if (refresh_agent_specific_information())
		m_agent.disclose_agent_information();

}
