#ifndef CLUSTER_H
#define CLUSTER_H
#include <vector>
#include "ImageUtils.h"
#include <memory>
#include <utility>
namespace ScoreProcessor {
	
	class Cluster {
	private:
		::std::vector<ImageUtils::Rectangle<unsigned int>> ranges;
	public:
		/*
			Constructs a cluster with no ranges
		*/
		Cluster();
		~Cluster();
		/*
			Returns the area of the cluster
			(i.e. the sum of the areas of all sub-rectangles)
		*/
		unsigned int size() const;
		/*
			Returns the bounding box of the cluster
			If cluster contains no rectangles, returns {MAX_UINT,0,MAXUINT,0}
		*/
		ImageUtils::Rectangle<unsigned int> bounding_box() const;
		/*
			Returns the center of mass of the cluster
			If cluster contains no rectangles, returns {0,0}
		*/
		ImageUtils::Point<unsigned int> center() const;
		/*
			Returns the highest rightmost point of the cluster
		*/
		ImageUtils::Point<unsigned int> right_top() const;
		/*
		Returns the lowest rightmost point of the cluster
		*/
		ImageUtils::Point<unsigned int> right_bottom() const;
		ImageUtils::Point<unsigned int> left_top() const;
		ImageUtils::Point<unsigned int> left_bottom() const;
		/*
			Returns the vertical_line representing the right side of the cluster
		*/
		ImageUtils::vertical_line<unsigned int> right_side() const;
		/*
			Gets the ranges of the cluster
		*/
		::std::vector<ImageUtils::Rectangle<unsigned int>> const& get_ranges() const;
		/*
			Given a vector of rectangles,
			Returns a vector of pointers to clusters made from those rectangles
		*/
		static ::std::vector<::std::unique_ptr<Cluster>> cluster_ranges(::std::vector<ImageUtils::Rectangle<unsigned int>> const& ranges);
	};
}
#endif // !CLUSTER_H
