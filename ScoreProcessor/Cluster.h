#pragma once
#include <vector>
#include "ImageUtils.h"
#include <memory>
namespace ScoreProcessor {
	class Cluster {
	private:
		std::unique_ptr<std::vector<ImageUtils::Rectangle*>> ranges;
	public:
		Cluster();
		~Cluster();
		static void clusterRanges(std::vector<Cluster*>& container,std::vector<ImageUtils::Rectangle*>& ranges);
	};
}