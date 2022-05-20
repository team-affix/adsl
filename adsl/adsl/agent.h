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
		/// Path to the file of the training data store.
		/// </summary>
		std::string m_training_data_folder_path;

		/// <summary>
		/// Boolean suggesting whether or not we should disclose the local agent information.
		/// </summary>
		affix_base::threading::guarded_resource<bool> m_should_disclose_agent_information = false;

		/// <summary>
		/// A vector of all training sets that are currently loaded into RAM from the disk.
		/// (This is not necessarily all of the training sets on disk.)
		/// </summary>
		affix_base::threading::guarded_resource<std::vector<training_set>> m_loaded_training_sets;

		/// <summary>
		/// Boolean describing whether or not the agent should continue loading random training sets from disk / offloading old ones.
		/// </summary>
		affix_base::threading::guarded_resource<bool> m_continue_loading_training_sets;

		/// <summary>
		/// Defines the minimum AND maximum number of training sets allowed to be loaded at any one time.
		/// </summary>
		affix_base::threading::guarded_resource<size_t> m_allowed_loaded_training_sets_count;

		/// <summary>
		/// The thread on which asynchonous loading from disk to memory of the 
		/// training sets is occuring.
		/// </summary>
		affix_base::threading::guarded_resource<std::thread> m_training_set_loading_thread;

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
			const std::string& a_training_data_folder_path,
			const size_t& a_iterations_for_compute_speed_test,
			const size_t& a_allowed_loaded_training_sets_count,
			const uint64_t& a_refresh_agent_information_interval
		);

		/// <summary>
		/// Acquires a number of training sets approximately directly proportional to the total number of 
		/// registered training sets and the normalized local compute speed.
		/// </summary>
		/// <returns></returns>
		std::vector<training_set> get_training_set_ration(

		);

		/// <summary>
		/// Updates the param vector.
		/// </summary>
		/// <param name="a_param_vector_informaiton"></param>
		/// <param name="a_update_vector_information"></param>
		/// <returns></returns>
		param_vector_information synchronize(
			const param_vector_information& a_param_vector_informaiton,
			const param_vector_information& a_update_vector_information
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
		void apply_training_set_updates(
			std::map<std::string, param_vector_update_information>& a_param_vector_updates,
			param_vector_information& a_updated_param_vector_information
		);

		/// <summary>
		/// Begins an asynchronous loop, which will repeatedly pull random training sets from disk.
		/// </summary>
		void begin_pull_training_sets_from_disk(

		);

		/// <summary>
		/// Begins an asynchonous loop refreshing agent specific information.
		/// </summary>
		void begin_refresh_agent_specific_information(
			const uint64_t& a_refresh_interval,
			const size_t& a_compute_speed_test_iterations
		);

		/// <summary>
		/// Retrieves a random training set from the disk.
		/// </summary>
		/// <returns></returns>
		bool try_get_random_training_set_from_disk(
			training_set& a_output
		);

		/// <summary>
		/// Loads all training set hashes.
		/// </summary>
		std::vector<std::vector<uint8_t>> get_training_data_hashes(

		);

		/// <summary>
		/// Returns a standardized compute frequency for this machine.
		/// </summary>
		/// <returns></returns>
		double get_compute_speed(
			const size_t& a_iterations
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
		/// Gets the normalized compute speed with respect to all connected adsl agents.
		/// </summary>
		/// <returns></returns>
		double normalized_compute_speed(

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
