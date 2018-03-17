#ifndef NEURALNET_H
#define NEURALNET_H
#include <vector>
namespace ScoreProcessor {
	class NeuralNet {
		std::vector<std::vector<float>> layers;
	public:
		void train();
	};
}
#endif