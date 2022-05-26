#pragma once
#include "affix-base/pch.h"
#include "affix-services/client.h"
#include "affix-services/agent.h"
#include "agent_specific_information.h"
#include "function_types.h"
#include "training_set.h"
#include "param_vector_information.h"
#include "param_vector_update_information.h"

namespace adsl
{
	class trainer
	{
	protected:
		const std::string m_session_name;

		asio::io_context m_io_context;
		affix_services::client m_client;
		affix_services::agent<agent_specific_information, adsl::function_types> m_agent;
		std::thread m_context_thread;
		std::thread m_process_thread;

		std::map<std::string, param_vector_update_information> m_param_vector_updates;
		affix_base::threading::guarded_resource<std::function<void(param_vector_information)>> m_synchronize_callback;

		std::function<std::vector<double>(std::vector<double>)> m_set_param_vector;
		std::function<double(training_set)> m_cycle;
		std::function<std::vector<double>()> m_get_update_vector;

		CryptoPP::AutoSeededRandomPool m_random;

		uint64_t m_compute_speed_test_interval_in_seconds = 0;
		uint64_t m_compute_speed_test_last_time_taken = 0;

		volatile bool m_continue_background_threads = true;

	public:
		virtual ~trainer(

		);

		trainer(
			const std::string& a_client_json_file_path,
			const std::string& a_session_name,
			const std::function<std::vector<double>(std::vector<double>)>& a_set_param_vector,
			const std::function<double(training_set)>& a_cycle,
			const std::function<std::vector<double>()>& a_get_update_vector,
			const uint64_t& a_compute_speed_test_interval_in_seconds
		);

		void process_epoch(

		);

		bool add_training_set(
			const training_set& a_training_set
		);

	protected:
		std::string session_root_path(

		) const;

		std::string training_sets_path(

		) const;

		std::string param_vector_path(

		) const;

		bool write_param_vector_information_to_disk(
			const param_vector_information& a_param_vector_information
		) const;

		bool read_param_vector_information_from_disk(
			param_vector_information& a_param_vector_information
		) const;

		double absolute_compute_speed(

		);

		double normalized_compute_speed(

		);

		std::vector<std::vector<uint8_t>> training_set_hashes(

		);

		bool write_training_set_to_disk(
			const std::vector<uint8_t>& a_hash,
			const training_set& a_training_set
		);

		bool read_training_set_from_disk(
			const std::vector<uint8_t>& a_hash,
			training_set& a_training_set
		);

		bool write_training_set_to_disk(
			const std::string& a_path,
			const training_set& a_training_set
		);

		bool read_training_set_from_disk(
			const std::string& a_path,
			training_set& a_training_set
		);

		std::string await_distribution_lead(

		);

		void synchronize_local_training_sets(

		);

		void synchronize_local_training_sets(
			const std::string& a_remote_client_identity
		);

		size_t training_sets_to_digest_count(

		);

		param_vector_information synchronize_param_vector(
			const param_vector_update_information& a_param_vector_update_information
		);

		param_vector_information apply_param_vector_updates(

		);

		bool read_random_training_set_from_disk(
			training_set& a_training_set
		);

		bool refresh_agent_specific_information(

		);

	};
}
