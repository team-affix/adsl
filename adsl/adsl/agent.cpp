#include "agent.h"
#include "training_set.h"
#include "aurora/pseudo.h"
#include "aurora/params.h"
#include "affix-base/stopwatch.h"

using namespace adsl;
using affix_base::threading::locked_resource;
using affix_base::threading::const_locked_resource;

agent::agent(
	affix_services::client& a_client,
	const std::string& a_session_identifier,
	const std::string& a_training_data_folder_path,
	const size_t& a_iterations_for_compute_speed_test,
	const size_t& a_allowed_loaded_training_sets_count
) :
	affix_services::agent<agent_specific_information, function_types>(
		a_client,
		"adsl-" + a_session_identifier,
		agent_specific_information()),
	m_training_data_folder_path(a_training_data_folder_path),
	m_allowed_loaded_training_sets_count(a_allowed_loaded_training_sets_count)
{
	m_remote_invocation_processor.add_function(function_types::training_set,
		std::function([&](std::string a_remote_client_identity, training_set a_training_set)
			{
				locked_resource l_parsed_agent_information = m_local_agent_information.m_parsed_agent_specific_information.lock();

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

				// Notify that the agent information should be disclosed at the start of the next processing cycle.
				locked_resource l_should_disclose_agent_information = m_should_disclose_agent_information.lock();
				*l_should_disclose_agent_information = true;

				// Create file name based off of training set hash
				std::string l_training_set_file_path = training_set_file_path_from_hash(l_hash);

				// Open a file output stream
				std::ofstream l_ofs(l_training_set_file_path, std::ios::out | std::ios::binary | std::ios::beg);

				if (!l_ofs.is_open())
				{
					std::clog << "[ ADSL ] Error; unable to write training set to file: " << std::filesystem::absolute(l_training_set_file_path) << std::endl;
					return;
				}

				// Serialize the training set
				affix_base::data::byte_buffer l_byte_buffer;
				l_byte_buffer.push_back(a_training_set);

				std::vector<uint8_t> l_bytes = l_byte_buffer.data();

				// Write the binary data to the output stream
				l_ofs.write((const char*)l_bytes.data(), l_bytes.size());

				l_ofs.close();

				// Push the training set hash to the list of training set hashes
				l_parsed_agent_information->m_training_set_hashes.push_back(l_hash);

				// Log the fact that we received a new training set.
				std::clog << "[ ADSL ] Received training set; " << l_training_set_file_path << std::endl;

			}));

	locked_resource l_local_agent_information = m_local_agent_information.m_parsed_agent_specific_information.lock();

	// Save information regarding the local agent
	l_local_agent_information->m_training_set_hashes = get_training_data_hashes();
	l_local_agent_information->m_compute_speed = get_compute_speed(a_iterations_for_compute_speed_test);

	// Begin pulling training sets from disk asynchronously.
	begin_pull_training_sets_from_disk();

}

std::vector<training_set> agent::get_training_set_ration(

)
{
	// Get the vector of all training sets that are loaded into memory.
	const_locked_resource l_loaded_training_sets = m_loaded_training_sets.const_lock();

	size_t l_allowed_loaded_training_sets_count = 0;

	{
		// Get the singular number of training sets allowed to occupy the loaded training sets buffer.
		const_locked_resource l_allowed_loaded_training_sets_count_resource = m_allowed_loaded_training_sets_count.const_lock();
		l_allowed_loaded_training_sets_count = *l_allowed_loaded_training_sets_count_resource;
	}

	std::vector<training_set> l_result;

	if (l_loaded_training_sets->size() != l_allowed_loaded_training_sets_count)
		// Return an empty vector of training sets, since the training sets have not yet been fully loaded from disk.
		return l_result;

	// Get the total number of training sets to digest.
	size_t l_training_sets_to_digest = (size_t)((double)l_loaded_training_sets->size() * normalized_compute_speed());


	double l_normalized_compute_speed = normalized_compute_speed();

	if (isnan(l_normalized_compute_speed))
		return l_result;

	// Construct a random number generator
	CryptoPP::AutoSeededRandomPool l_random;

	for (int i = 0; i < l_training_sets_to_digest; i++)
	{
		size_t l_training_set_index = l_random.GenerateWord32(0, l_loaded_training_sets->size() - 1);
		l_result.push_back(l_loaded_training_sets->at(l_training_set_index));
	}

	return l_result;

}

void agent::begin_pull_training_sets_from_disk(

)
{
	locked_resource l_continue = m_continue_loading_training_sets.lock();

	// Set the continuing state to true
	*l_continue = true;

	locked_resource l_training_set_loading_thread = m_training_set_loading_thread.lock();

	 *l_training_set_loading_thread = std::thread(
		[&]
		{
			while (true)
			{
				// Lock the resource describing whether or not we should continue
				const_locked_resource l_should_continue = m_continue_loading_training_sets.const_lock();

				if (!*l_should_continue)
					// Quit the thread if it should happen.
					return;


				locked_resource l_loaded_training_sets = m_loaded_training_sets.lock();

				size_t l_allowable_loaded_training_sets_count = 0;

				{
					// Lock the resource describing whether or not we should continue
					const_locked_resource l_allowable_loaded_training_sets_count_resource = m_allowed_loaded_training_sets_count.const_lock();
					l_allowable_loaded_training_sets_count = *l_allowable_loaded_training_sets_count_resource;
				}

				training_set l_training_set;
					
				if (!try_get_random_training_set_from_disk(l_training_set))
					// Do nothing, since we were unable to load the training set.
					continue;

				l_loaded_training_sets->push_back(l_training_set);

				if (l_loaded_training_sets->size() > l_allowable_loaded_training_sets_count)
					// If there are too many training sets loaded, erase one at the beginning to return to maximum capacity.
					l_loaded_training_sets->erase(l_loaded_training_sets->begin());

			}
		});
}

bool agent::try_get_random_training_set_from_disk(
	training_set& a_output
)
{
	size_t l_total_training_sets = training_sets_on_disk_count();

	CryptoPP::AutoSeededRandomPool l_random;

	size_t l_training_set_index = l_random.GenerateWord32(0, l_total_training_sets - 1);

	int i = 0;

	for (auto l_entry : std::filesystem::directory_iterator(m_training_data_folder_path))
	{
		if (i != l_training_set_index)
		{
			i++;
			continue;
		}
		
		// Import this training set.
		return try_get_training_set(l_entry.path().u8string(), a_output);

	}

}

std::vector<std::vector<uint8_t>> agent::get_training_data_hashes(

)
{
	std::vector<std::vector<uint8_t>> l_result;

	namespace fs = std::filesystem;

	if (!fs::exists(m_training_data_folder_path))
	{
		std::clog << "[ ADSL ] Error; training data folder does not exist: " << std::filesystem::absolute(m_training_data_folder_path) << std::endl;
		return l_result;
	}

	for (const auto& a_entry : fs::directory_iterator(m_training_data_folder_path))
	{
		training_set l_training_set;

		if (!try_get_training_set(a_entry.path().u8string(), l_training_set))
		{
			// Do nothing, the error has already been logged.
			continue;
		}

		// Push hash of training set to vector
		l_result.push_back(l_training_set.hash());

	}

	return l_result;

}

double agent::get_compute_speed(
	const size_t& a_iterations
)
{
	using namespace aurora;
	using namespace aurora::models;
	using namespace aurora::params;

	std::clog << "[ ADSL ] Performing standardized compute speed test." << std::endl;

	Model l_model = pseudo::tnn({ 2, 20, 1 }, pseudo::nlr(0.3));
	param_vector l_pv;
	l_model->param_recur(pseudo::param_init(new param_mom(0.02, 0.9), l_pv));
	l_pv.rand_norm();
	l_model->compile();

	affix_base::timing::stopwatch l_stopwatch;
	l_stopwatch.start();

	for (int i = 0; i < a_iterations; i++)
	{
		l_model->fwd();
		l_model->bwd();
	}

	double l_compute_duration = l_stopwatch.duration_milliseconds();

	double l_compute_speed = ((double)a_iterations) / l_compute_duration;

	std::clog << "[ ADSL ] Absolute compute speed: " << l_compute_speed << std::endl;

	return l_compute_speed;

}

void agent::send_desynchronized_training_sets(
	const std::string& a_remote_client_identity
)
{
	// Get map of remote client identities to their associated agents
	locked_resource l_remote_agents = m_remote_agents.lock();

	// Get the local agent information
	const_locked_resource l_local_agent_information = m_local_agent_information.m_parsed_agent_specific_information.const_lock();

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
			if (!try_get_training_set(training_set_file_path_from_hash(l_local_hash), l_local_training_set))
				// Failed to get raw training set from disk
				continue;

			// Write message to log
			std::clog << "[ ADSL ] Sending training set to remote agent: " << a_remote_client_identity.substr(50, 10) << "; hash: " << base64_from_bytes(l_local_hash) << std::endl;

			// Send the training set to the remote agent.
			invoke(a_remote_client_identity, function_types::training_set, l_local_training_set);


		}

	}

}

size_t agent::training_sets_on_disk_count(

)
{
	size_t l_result = 0;

	for (auto l_entry : std::filesystem::directory_iterator(m_training_data_folder_path))
	{
		l_result++;
	}

	return l_result;

}

double agent::normalized_compute_speed(

)
{
	const_locked_resource l_local_agent_information = m_local_agent_information.m_parsed_agent_specific_information.const_lock();

	double l_total_compute_speed = 0;

	locked_resource l_remote_agents = m_remote_agents.lock();

	for (auto i = l_remote_agents->begin(); i != l_remote_agents->end(); i++)
	{
		const_locked_resource l_remote_agent_information = i->second->m_parsed_agent_specific_information.const_lock();

		l_total_compute_speed += l_remote_agent_information->m_compute_speed;

	}

	return l_local_agent_information->m_compute_speed / l_total_compute_speed;
	
}

std::string agent::base64_from_bytes(
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

std::string agent::training_set_file_path_from_hash(
	const std::vector<uint8_t>& a_hash
)
{

	return m_training_data_folder_path + "/" + affix_base::data::string_trim(base64_from_bytes(a_hash), {'\\','/',':','*','?','\"','<','>','|'});
}

bool agent::try_get_training_set(
	const std::string& a_training_set_file_path,
	adsl::training_set& a_output
)
{
	namespace fs = std::filesystem;

	std::ifstream l_ifs(a_training_set_file_path, std::ios::in | std::ios::binary | std::ios::beg);

	if (!l_ifs.is_open())
	{
		// Don't try to load training set, the file could not be opened for some reason
		std::clog << "[ ADSL ] Error opening training set file: " << fs::absolute(a_training_set_file_path) << std::endl;
		return false;
	}

	// Initialize an empty vector of bytes
	std::vector<uint8_t> l_bytes;

	// Populate vector of bytes in a little loop
	char l_char = 0;
	while (l_ifs.get(l_char))
	{
		l_bytes.push_back(l_char);
	}

	// Close the file stream
	l_ifs.close();

	// Deserialize training set object
	affix_base::data::byte_buffer l_byte_buffer(l_bytes);
	return l_byte_buffer.pop_front(a_output);

}

std::string agent::distribution_lead(

)
{
	affix_base::threading::const_locked_resource l_registered_agents = m_remote_agents.const_lock();

	std::string l_max_identity_value;

	for (auto i = l_registered_agents->begin(); i != l_registered_agents->end(); i++)
		l_max_identity_value = larger_identity(l_max_identity_value, i->first);

	return l_max_identity_value;

}

std::string agent::larger_identity(
	const std::string& a_identity_0,
	const std::string& a_identity_1
) const
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

void agent::agent_specific_process(

)
{
	// Disclose agent information if necessary.
	locked_resource l_should_disclose_agent_information = m_should_disclose_agent_information.lock();
	if (*l_should_disclose_agent_information)
	{
		disclose_agent_information();
		*l_should_disclose_agent_information = false;
	}

	process_remote_agent_training_sets();

}

void agent::process_remote_agent_training_sets(

)
{
	// Get map of remote client identities to their associated agents
	locked_resource l_remote_agents = m_remote_agents.lock();

	// Get distribution lead identity
	std::string l_distribution_lead_identity = distribution_lead();

	if (l_distribution_lead_identity == m_local_client.m_local_identity)
	{
		// If the local agent is the distribution lead, send training sets to all agents

		for (auto l_agent_iterator = l_remote_agents->begin(); l_agent_iterator != l_remote_agents->end(); l_agent_iterator++)
		{
			send_desynchronized_training_sets(l_agent_iterator->first);
		}

	}
	else
	{
		// If the local agent is not the distribution lead, send training sets only to the DL.

		send_desynchronized_training_sets(l_distribution_lead_identity);

	}

	
}

void agent::on_remote_agent_connect(
	const std::string& a_remote_client_identity
)
{
	std::cout << "REMOTE AGENT CONNECTED: " << a_remote_client_identity.substr(50, 10) << std::endl;
}

void agent::on_remote_agent_disconnect(
	const std::string& a_remote_client_identity
)
{
	std::cout << "REMOTE AGENT DISCONNECTED: " << a_remote_client_identity.substr(50, 10) << std::endl;
}

void agent::on_remote_agent_information_changed(
	const std::string& a_remote_client_identity,
	const agent_specific_information& a_agent_specific_information
)
{
	std::cout << "REMOTE AGENT INFORMATION CHANGED: " << a_remote_client_identity.substr(50, 10) << std::endl;
}
