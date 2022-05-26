#include "aurora/tensor.h"
#include "adsl/training_set.h"
#include "aurora/models.h"
#include "aurora/pseudo.h"
#include "adsl/trainer.h"

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

	Mse_loss l_model = new mse_loss(pseudo::tnn({ 2, 8, 1 }, pseudo::nlr(0.3)));

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

	std::clog.rdbuf(l_nullstream.rdbuf());

	trainer l_trainer(
		"config/client_configuration.json",
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

	while (true)
	{
		l_trainer.process_epoch();
	}

	return 0;

}
