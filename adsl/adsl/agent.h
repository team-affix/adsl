#pragma once
#include "affix-base/pch.h"
#include "affix-services/agent.h"
#include "agent_specific_information.h"
#include "function_types.h"
#include "training_set.h"

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

	public:
		/// <summary>
		/// Initializes the agent with the given client.
		/// </summary>
		/// <param name="a_client"></param>
		agent(
			affix_services::client& a_client,
			const std::string& a_session_identifier,
			const std::string& a_training_data_folder_path
		);

		/// <summary>
		/// Acquires a number of training sets approximately directly proportional to the total number of 
		/// registered training sets and the normalized local compute speed.
		/// </summary>
		/// <returns></returns>
		std::vector<training_set> get_training_sets(

		);

	protected:
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
		size_t training_set_count(

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
