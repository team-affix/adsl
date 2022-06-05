#include "aurora/tensor.h"
#include "adsl/training_set.h"
#include "aurora/models.h"
#include "aurora/pseudo.h"
#include "adsl/trainer.h"
#include "affix-base/stopwatch.h"

int main(

)
{
	using namespace adsl;
	namespace fs = std::filesystem;
	using namespace aurora::models;
	using namespace aurora::params;
	using namespace aurora;

	asio::io_context l_io_context;

	if (!fs::exists("config/"))
	{
		// If config directory doesn't exist, create it.
		fs::create_directory("config/");
	};

	Mse_loss l_model = new mse_loss(pseudo::tnn({ 2, 200, 200, 1 }, pseudo::nlr(0.3)));

	param_vector l_param_vector;

	l_model->param_recur(pseudo::param_init(new param_sgd(0.02), l_param_vector));
	l_param_vector.rand_norm();

	l_model->compile();


	auto l_set_param_vector = [&](const std::vector<double>& a_param_vector)
	{
		if (a_param_vector.size() != l_param_vector.size())
		{
			std::vector<double> l_result(l_param_vector.size());
			for (int i = 0; i < l_param_vector.size(); i++)
				l_result[i] = l_param_vector[i]->state();
			return l_result;
		}
		else
		{
			for (int i = 0; i < a_param_vector.size(); i++)
				l_param_vector[i]->state() = a_param_vector[i];
			return a_param_vector;
		}
	};

	auto l_cycle = [&](const training_set& a_training_set)
	{
		return l_model->cycle(a_training_set.m_x, a_training_set.m_y);
	};

	auto l_get_update_vector = [&]
	{
		std::vector<double> l_update_vector(l_param_vector.size());
		for (int i = 0; i < l_param_vector.size(); i++)
		{
			param_sgd* l_param = l_param_vector[i];
			l_update_vector[i] = 0.02 * l_param->gradient();

			// Clear the gradient for next epoch
			l_param->gradient() = 0;

		}

		return l_update_vector;

	};

	std::ofstream l_nullstream;

	//std::clog.rdbuf(l_nullstream.rdbuf());

	affix_base::data::ptr<affix_services::client_configuration> l_client_configuration(new affix_services::client_configuration("config/client_configuration.json"));

	l_client_configuration->import_resource();
	l_client_configuration->export_resource();

	trainer l_trainer(
		l_client_configuration,
		"example-session-0",
		l_set_param_vector,
		l_cycle,
		l_get_update_vector,
		60
	);

	l_trainer.add_training_set(training_set({ 0, 0 }, { 0 }));
	l_trainer.add_training_set(training_set({ 0, 1 }, { 1 }));
	l_trainer.add_training_set(training_set({ 1, 0 }, { 1 }));
	l_trainer.add_training_set(training_set({ 1, 1 }, { 0 }));

	affix_base::timing::stopwatch l_stopwatch;

	for (int i = 0; true; i++)
	{
		l_stopwatch.start();
		epoch_information l_epoch_information = l_trainer.process_epoch();
		if (i % 100 == 0)
			std::cout 
			<< "[ SESSION ] Iteration: " << i << std::endl
			<< "  Number of machines training: " << l_epoch_information.m_number_of_machines_training << std::endl
			<< "  Cost: " << l_epoch_information.m_cost << std::endl
			<< "  Period for single epoch: " << (l_stopwatch.duration_microseconds() / (double)1000) << " ms" << std::endl
			<< "  All-time digested: " << l_epoch_information.m_end_of_epoch_all_time_global_training_sets_digested << std::endl
			<< "  Last-epoch digested globally: " << l_epoch_information.last_epoch_global_training_sets_digested() << std::endl
			<< "  Last-epoch digested locally: " << l_epoch_information.m_last_epoch_local_training_sets_digested << std::endl
			<< "  Last-epoch local proportion of contribution: " << l_epoch_information.last_epoch_local_proportion_contribution() << std::endl
			<< std::endl;
	}

	return 0;

}
