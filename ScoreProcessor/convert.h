#ifndef SCP_CONVERT_H
#define SCP_CONVERT_H
#include <string>
#include "CImg.h"
namespace ScoreProcessor {
	void convert(std::string const& filename,std::string const& end) {
		cimg_library::CImg<unsigned char> img(filename.c_str());
		size_t period=filename.find_last_of('.');
		std::string new_name=filename.substr(0,period+1)+end;
		img.save(new_name.c_str());
	}
}
#endif