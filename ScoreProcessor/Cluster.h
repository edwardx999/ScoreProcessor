#pragma once
#include <vector>
#include "ImageUtils.h"
#include <memory>
namespace ScoreProcessor {
	class Cluster {
	private:
		std::vector<std::shared_ptr<ImageUtils::Rectangle>> ranges;
	public:
		Cluster();
		~Cluster();
		int size();
		std::vector<std::shared_ptr<ImageUtils::Rectangle>> const& getRanges() const;
		friend void clusterRanges(std::vector<std::unique_ptr<Cluster>>& clusterContainer,std::vector<std::shared_ptr<ImageUtils::Rectangle>>& ranges);
	};
}