#pragma once
#include "affix-base/pch.h"
#include "affix-services/agent.h"
#include "agent_specific_information.h"
#include "function_types.h"
#include "training_set.h"
#include "param_vector_update_information.h"

namespace adsl
{
	class agent : public affix_services::agent<agent_specific_information, function_types>
	{
	protected:
		/// <summary>
		/// Path to the folder of the training data store.
		/// </summary>
		std::string m_training_data_folder_path;

		/// <summary>
		/// Path to the file which contains local param vector information.
		/// </summary>
		std::string m_param_vector_information_path;

		/// <summary>
		/// Boolean suggesting whether or not we should disclose the local agent information.
		/// </summary>
		affix_base::threading::guarded_resource<bool> m_should_disclose_agent_information = false;

		/// <summary>
		/// Updates waiting to be applied to the parameter vector.
		/// </summary>
		affix_base::threading::guarded_resource<std::map<std::string, param_vector_update_information>> m_param_vector_updates;

		/// <summary>
		/// Callback for when an updated param vector is received.
		/// </summary>
		affix_base::threading::guarded_resource<std::function<void(param_vector_information)>> m_synchonize_callback;

		/// <summary>
		/// A map of times to call certain delayed functions.
		/// </summary>
		affix_base::threading::guarded_resource<std::map<uint64_t, std::function<void()>>> m_pending_function_calls;

	public:
		/// <summary>
		/// Initializes the agent with the given client.
		/// </summary>
		/// <param name="a_client"></param>
		agent(
			affix_services::client& a_client,
			const std::string& a_session_identifier,
			const std::string& a_param_vector_information_path,
			const std::string& a_training_data_folder_path,
			const size_t& a_iterations_for_compute_speed_test,
			const uint64_t& a_refresh_agent_information_interval
		);

		/// <summary>
		/// Returns the number of training sets that should be digested by the local executable in each epoch.
		/// </summary>
		/// <returns></returns>
		size_t training_sets_to_digest_count(

		);

		/// <summary>
		/// Retrieves a random training set from the disk.
		/// </summary>
		/// <returns></returns>
		bool try_get_random_training_set_from_disk(
			training_set& a_output
		);

		/// <summary>
		/// Attempts to retrieve information about the param vector from local LTS.
		/// </summary>
		/// <param name="a_param_vector_information"></param>
		/// <returns></returns>
		bool try_get_param_vector_information_from_disk(
			param_vector_information& a_param_vector_information
		);

		/// <summary>
		/// Attempts to write the local param vector information to disk.
		/// </summary>
		/// <param name="a_param_vector_information"></param>
		/// <returns></returns>
		bool try_set_param_vector_information_in_disk(
			const param_vector_information& a_param_vector_information
		);

		/// <summary>
		/// Updates the param vector.
		/// </summary>
		/// <param name="a_param_vector_informaiton"></param>
		/// <param name="a_update_vector_information"></param>
		/// <returns></returns>
		param_vector_information synchronize(
			const param_vector_update_information& a_param_vector_update_informaiton
		);

		/// <summary>
		/// Gets the normalized compute speed with respect to all connected adsl agents.
		/// </summary>
		/// <returns></returns>
		double normalized_compute_speed(

		);

	protected:
		/// <summary>
		/// Waits until the distribution lead is not undefined, and then returns the distribution lead.
		/// </summary>
		/// <returns></returns>
		std::string await_distribution_lead(

		);

		/// <summary>
		/// Applies relevant updates to the most sophisticated parameter vector.
		/// </summary>
		void apply_param_vector_updates(
			std::map<std::string, param_vector_update_information>& a_param_vector_updates,
			param_vector_information& a_updated_param_vector_information
		);

		/// <summary>
		/// Begins an asynchonous loop refreshing agent specific information.
		/// </summary>
		void begin_refresh_agent_specific_information(
			const uint64_t& a_refresh_interval,
			const size_t& a_compute_speed_test_iterations
		);

		/// <summary>
		/// Retrieves the compute speed of the local machine by way of conducting a standardized test.
		/// </summary>
		/// <param name="a_iterations"></param>
		/// <returns></returns>
		double get_compute_speed(
			const size_t& a_iterations
		);

		/// <summary>
		/// Loads all training set hashes.
		/// </summary>
		std::vector<std::vector<uint8_t>> get_training_data_hashes(

		);

		/// <summary>
		/// Sends all training sets which the remote agent does not have to it.
		/// </summary>
		/// <param name="a_remote_client_identity"></param>
		/// <param name="a_remote_agent_specific_information"></param>
		void send_desynchronized_training_sets(
			const std::string& a_remote_client_identity
		);

		/// <summary>
		/// Gets the number of files in a given folder.
		/// </summary>
		/// <param name="a_folder_path"></param>
		/// <returns></returns>
		size_t training_sets_on_disk_count(

		);

		/// <summary>
		/// Returns a base64 string from a vector of bytes.
		/// </summary>
		/// <returns></returns>
		std::string base64_from_bytes(
			const std::vector<uint8_t>& a_bytes
		);

		/// <summary>
		/// Returns the full training set file path.
		/// </summary>
		/// <param name="a_hash"></param>
		/// <returns></returns>
		std::string training_set_file_path_from_hash(
			const std::vector<uint8_t>& a_hash
		);

		/// <summary>
		/// Get a training set from file based on the hash.
		/// </summary>
		/// <param name="a_hash"></param>
		/// <returns></returns>
		bool try_get_training_set(
			const std::string& a_training_set_file_path,
			adsl::training_set& a_output
		);

		/// <summary>
		/// Gets the current largest identity. This is often used for choosing a distribution lead.
		/// </summary>
		/// <returns></returns>
		std::string distribution_lead(

		);

		/// <summary>
		/// Returns the identity which is larger in numerical value.
		/// </summary>
		/// <param name="a_identity_0"></param>
		/// <param name="a_identity_1"></param>
		/// <returns></returns>
		std::string larger_identity(
			const std::string& a_identity_0,
			const std::string& a_identity_1
		) const;

		/// <summary>
		/// Processes all agent data. Processes inbound messages, and sends outbound messages.
		/// </summary>
		virtual void agent_specific_process(

		);

		/// <summary>
		/// Process to see
		/// </summary>
		void process_remote_agent_training_sets(

		);

		/// <summary>
		/// Processes all pending function calls.
		/// </summary>
		void process_pending_function_calls(

		);

		/// <summary>
		/// The ADSL override for when a remote agent connects to the network.
		/// </summary>
		virtual void on_remote_agent_connect(
			const std::string& a_remote_client_identity
		);

		/// <summary>
		/// The ADSL override for when a remote agent disconnects from the network.
		/// </summary>
		virtual void on_remote_agent_disconnect(
			const std::string& a_remote_client_identity
		);

		/// <summary>
		/// The ADSL override for when a remote agent's information changes.
		/// </summary>
		virtual void on_remote_agent_information_changed(
			const std::string& a_remote_client_identity,
			const agent_specific_information& a_agent_specific_information
		);

	};
}
