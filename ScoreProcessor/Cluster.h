#ifndef CLUSTER_H
#define CLUSTER_H
#include <vector>
#include "ImageUtils.h"
#include <memory>
namespace ScoreProcessor {
	class Cluster {
	private:
		::std::vector<ImageUtils::RectangleUINT> ranges;
	public:
		Cluster();
		~Cluster();
		unsigned int size();
		::std::vector<ImageUtils::RectangleUINT> const& get_ranges() const;
		static ::std::vector<::std::unique_ptr<Cluster>> cluster_ranges(::std::vector<ImageUtils::RectangleUINT>& ranges);
	};
}
#endif // !CLUSTER_H
