#pragma once
#include <vector>
#include "ImageUtils.h"
#include <memory>
namespace ScoreProcessor {
	class Cluster {
	private:
		::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>> ranges;
	public:
		Cluster();
		~Cluster();
		unsigned int size();
		::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>> const& getRanges() const;
		friend void clusterRanges(::std::vector<::std::unique_ptr<Cluster>>& clusterContainer,::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>>& ranges);
	};
}