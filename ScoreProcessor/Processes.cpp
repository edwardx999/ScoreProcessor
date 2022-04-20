#include "stdafx.h"
#include "Processes.h"

namespace ScoreProcessor {

	bool ChangeToGrayscale::process(Img& img) const
	{
		if(img._spectrum >= 3)
		{
			img = get_grayscale_simple(img);
			return true;
		}
		return false;
	}

	bool FillTransparency::process(Img& img) const
	{
		if(img._spectrum >= 4)
		{
			return fill_transparency(img, background);
		}
		return false;
	}

	bool RemoveBorderGray::process(Img& img) const
	{
		if(img._spectrum == 1)
		{
			return remove_border(img, 0, tolerance);
		}
		else
		{
			throw std::invalid_argument("Remove process requires Grayscale image");
		}
	}
	bool FilterGray::process(Img& img) const
	{
		if(img._spectrum < 3)
		{
			return replace_range(img, min, max, replacer);
		}
		else
		{
			return replace_by_brightness(img, min, max, ImageUtils::ColorRGB({replacer,replacer,replacer}));
		}
	}

	bool FilterHSV::process(Img& img) const
	{
		if(img._spectrum >= 3)
		{
			return replace_by_hsv(img, start, end, replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}

	bool FilterRGB::process(Img& img) const
	{
		if(img._spectrum >= 3)
		{
			return replace_by_rgb(img, start, end, replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}

	bool PadHoriz::process(Img& img) const
	{
		std::array<unsigned int, 2> dims{{img._width,img._height}};
		return horiz_padding(img, side1(dims), side2(dims), tolerance(dims), background, cumulative);
	}

	bool PadVert::process(Img& img) const
	{
		std::array<unsigned int, 2> dims{{img._width,img._height}};
		return vert_padding(img, side1(dims), side2(dims), tolerance(dims), background, cumulative);
	}

	bool PadAuto::process(Img& img) const
	{
		return auto_padding(img, vert, max_h, min_h, hoff, opt_rat);
	}

	bool PadCluster::process(Img& img) const
	{
		return cluster_padding(img, left, right, top, bottom, bt);
	}

	bool Crop::process(Img& img) const
	{
		auto const w = img.width();
		auto const h = img.height();
		using rint = ImageUtils::Rectangle<int>;
		rint region;
		region.left = left;
		region.right = right.value_or(w);
		region.top = top;
		region.bottom = bottom.value_or(h);
		if(region == rint({0,w,0,h}))
		{
			return false;
		}
		if(region.right <= region.left || region.bottom <= region.top)
		{
			img = Img();
		}
		else
		{
			region.right--;
			region.bottom--;
			img = get_crop_fill(img, region, unsigned char(255));
		}
		return true;
	}

	bool Rescale::process(Img& img) const
	{
		img.resize(
			static_cast<unsigned int>(std::round(img._width * val)),
			static_cast<unsigned int>(std::round(img._height * val)),
			img._depth,
			img._spectrum,
			interpolation);
		return true;
	}

	bool ExtractLayer0NoRealloc::process(Img& img) const
	{
		if(img._spectrum > 1U)
		{
			img._spectrum = 1U;
			return true;
		}
		return false;
	}

	/*
	bool ClusterClearGray::process(Img& img) const
	{
		if(img._spectrum==1)
		{
			return clear_clusters(img,
				std::array<unsigned char,1>({background}),
				[this](std::array<unsigned char,1> v)
			{
				return ImageUtils::gray_diff(v[0],background)<tolerance;
			},
				[this](Cluster const& cluster)
			{
				auto size=cluster.size();
				return size>=min&&size<=max;
			});
		}
		else
		{
			throw std::invalid_argument("Cluster Clear Grayscale requires grayscale image");
		}
	}
	*/

	bool ClusterClearGrayAlt::process(Img& img) const
	{
		auto grayscale_sel = [smn = sel_min, smx = sel_max](std::array<unsigned char, 1> v)
		{
			return v[0] >= smn && v[0] <= smx;
		};
		auto color_sel = [smn = 3U * sel_min, smx = 3U * sel_max](std::array<unsigned char, 3> v)
		{
			auto brightness = static_cast<unsigned int>(v[0]) + v[1] + v[2];
			return brightness >= smn && brightness <= smx;
		};
		auto clear_base = [this, &img](auto func)
		{
			return [this, &img, func](Cluster const& c)
			{
				auto const size = c.size();
				if(size >= min_size && size <= max_size)
				{
					return true;
				}
				if(sel_min >= required_min && sel_max <= required_max)
				{
					return false;
				};
				for(auto rect : c.get_ranges())
				{
					for(unsigned int y = rect.top; y < rect.bottom; ++y)
					{
						auto row = img._data + y * img._width;
						for(unsigned int x = rect.left; x < rect.right; ++x)
						{
							auto pix = row + x;
							if(func(pix))
							{
								return false;
							}
						}
					}
				}
				return true;
			};
		};
		auto grayscale_clear = clear_base([rmn = required_min, rmx = required_max](unsigned char const* pix)
			{
				return *pix >= rmn && *pix <= rmx;
			});
		auto color_clear = clear_base([rmn = 3U * required_min, rmx = 3U * required_max, img_size = size_t{img._height}*img._width](unsigned char const* pix)
		{
			unsigned int brightness = 0;
			for(int s = 0; s < 3; ++s)
			{
				brightness += *(pix + s * img_size);
			}
			return brightness >= rmn && brightness <= rmx;
		});
		if(!eight)
		{
			if(img._spectrum < 3)
			{
				return clear_clusters(img, std::array<unsigned char, 1>({background}), grayscale_sel, grayscale_clear);
			}
			else
			{
				return clear_clusters(img, std::array<unsigned char, 3>({background,background,background}), color_sel, color_clear);
			}
		}
		else
		{
			if(img._spectrum < 3)
			{
				return clear_clusters_8way(img, std::array<unsigned char, 1>({background}), grayscale_sel, grayscale_clear);
			}
			else
			{
				return clear_clusters_8way(img, std::array<unsigned char, 3>({background,background,background}), color_sel, color_clear);
			}
		}
	}

	bool RescaleGray::process(Img& img) const
	{
		if(img._spectrum < 3)
		{
			rescale_colors(img, min, mid, max);
		}
		else
		{
			rescale_colors(img, min, mid, max);
			rescale_colors(img.get_shared_channel(1), min, mid, max);
			rescale_colors(img.get_shared_channel(2), min, mid, max);
		}
		return true;
	}

	ImageUtils::Point<signed int> get_origin(FillRectangle::origin_reference origin_code, int width, int height)
	{
		ImageUtils::Point<signed int> porigin;
		switch(origin_code % 3)
		{
		case 0:
			porigin.x = 0;
			break;
		case 1:
			porigin.x = width / 2;
			break;
		case 2:
			porigin.x = width;
		}
		switch(origin_code / 3)
		{
		case 0:
			porigin.y = 0;
			break;
		case 1:
			porigin.y = height / 2;
			break;
		case 2:
			porigin.y = height;
		}
		return porigin;
	}

	ImageUtils::RectangleUINT resolve_origin_rectangle(cil::CImg<unsigned char> const& img, ImageUtils::Rectangle<int> offsets, FillRectangle::origin_reference origin)
	{
		auto const porigin = get_origin(origin, img.width(), img.height());
		ImageUtils::Rectangle<int> rect;
		rect.left = offsets.left + porigin.x;
		rect.right = offsets.right + porigin.x;
		rect.top = offsets.top + porigin.y;
		rect.bottom = offsets.bottom + porigin.y;
		if(rect.left < 0)
		{
			rect.left = 0;
		}
		if(rect.top < 0)
		{
			rect.top = 0;
		}
		if(rect.right > img.width())
		{
			rect.right = img.width();
		}
		if(rect.bottom > img.height())
		{
			rect.bottom = img.height();
		}
		return rect;
	}

	void fill_fix_layers(ImageProcess<>::Img& img,std::array<unsigned char,4> color,unsigned int num_layers)
	{
		if(num_layers<5)
		{
			switch(img._spectrum)
			{
			case 1:
			case 2:
				if(num_layers==3||num_layers==4&&color[3]==255)
				{
					cil::CImg<unsigned char> temp(img._width,img._height,1,3);
					auto const size=std::size_t{img._width}*img._height;
					memcpy(temp.data(),img.data(),size);
					memcpy(temp.data()+size,img.data(),size);
					memcpy(temp.data()+2U*size,img.data(),size);
					img.swap(temp);
				}
				else if(num_layers==4)
				{
					cil::CImg<unsigned char> temp(img._width,img._height,1,4);
					auto const size=std::size_t{img._width}*img._height;
					memcpy(temp.data(),img.data(),size);
					memcpy(temp.data()+size,img.data(),size);
					memcpy(temp.data()+2U*size,img.data(),size);
					memset(temp.data()+3U*size,255,size);
					img.swap(temp);
				};
				break;
			case 3:
				if(num_layers==4&&color[3]!=255)
				{
					cil::CImg<unsigned char> temp(img._width,img._height,1,4);
					auto const size=std::size_t{img._width}*img._height;
					memcpy(temp.data(),img.data(),3U*size);
					memset(temp.data()+3U*size,255,size);
					img.swap(temp);
				}
			}
		}
	}

	bool FillRectangle::process(Img& img) const
	{
		auto rect = resolve_origin_rectangle(img, offsets, origin);
		fill_fix_layers(img,color,num_layers);
#define ucast static_cast<unsigned int>
		return fill_selection(
			img,
			rect,
			color.data(), check_fill_t());
#undef ucast
	}

	bool Blur::process(Img& img) const
	{
		img.blur(radius);
		return true;
	}

	bool Straighten::process(Img& img) const
	{
		auto angle = find_angle_bare(img, pixel_prec, min_angle, max_angle, num_steps, boundary, use_horiz);
		if(angle == 0)
		{
			return false;
		}
		apply_gamma(img, gamma);
		img.rotate(angle * RAD_DEG, 2, 1);
		apply_gamma(img, 1 / gamma);
		return true;
	}

	bool Rotate::process(Img& img) const
	{
		img.rotate(-angle, mode, 1);
		return true;
	}

	bool Gamma::process(Img& img) const
	{
		apply_gamma(img, gamma);
		return true;
	}

	bool HorizontalShift::process(Img& img) const
	{
		horizontal_shift(img, side, direction, background_threshold);
		return true;
	}

	bool VerticalShift::process(Img& img) const
	{
		vertical_shift(img, side, direction, background_threshold);
		return true;
	}


	bool RescaleAbsolute::process(Img& img) const
	{
		constexpr unsigned int interpolate = -1;
		unsigned int true_width, true_height;
		if(height != interpolate && width != interpolate)
		{
			true_height = height;
			true_width = width;
		}
		else
		{
			float true_ratio;
			if(ratio < 0)
			{
				true_ratio = static_cast<double>(img.width()) / img.height();
			}
			else
			{
				true_ratio = ratio;
			}
			if(height != interpolate)
			{
				true_height = height;
				true_width = static_cast<unsigned int>(std::round(true_height * true_ratio));
			}
			else
			{
				true_width = width;
				true_height = static_cast<unsigned int>(std::round(width / true_ratio));
			}
		}
		Rescale::rescale_mode true_mode;
		if(mode == Rescale::automatic)
		{
			true_mode = (true_height < img._height || true_width < img._width) ? Rescale::moving_average : Rescale::cubic;
		}
		else
		{
			true_mode = mode;
		}
		if(true_width != img._width || true_height != img._height)
		{
			img.resize(true_width, true_height, img._depth, img._spectrum, true_mode);
			return true;
		}
		return false;
	}

	bool ChangeCanvasSize::process(Img& img) const
	{
		auto const true_width = width == -1 ? img.width() : width;
		auto const true_height = height == -1 ? img.height() : height;
		if(true_width != img._width || true_height != img._height)
		{
			auto region = [origin = this->origin, &img, true_width, true_height]()
			{
				ImageUtils::Rectangle<int> ret;
				switch(origin % 3)
				{
				case 0: //left
					ret.left = 0;
					ret.right = true_width;
					break;
				case 1: //middle
				{
					ret.left = (img.width() - true_width) / 2;
					ret.right = ret.left + true_width;
					break;
				}
				case 2: //right
					ret.left = img.width() - true_width;
					ret.right = img.width();
				}
				switch(origin / 3)
				{
				case 0: //top
					ret.top = 0;
					ret.bottom = true_height;
					break;
				case 1: //middle
				{
					ret.top = (img.height() - true_height) / 2;
					ret.bottom = ret.top + true_height;
					break;
				}
				case 2: //bottom
					ret.top = img.height() - true_height;
					ret.bottom = img.height();
				}
				return ret;
			}();
			--region.right;
			--region.bottom;
			img = get_crop_fill(img, region, fill);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool MLAA::process(Img& img) const
	{
		return mlaa(img, contrast_threshold, gamma);
	}

	bool NeuralScale::process(Img& img) const
	{
		scaler.smart_scale(img, ratio, ThreadOverride::num_threads());
		return true;
	}
	template<typename T, typename DoWhatWithPoint>
	void find_local_min_below_thresh(cil::CImg<T> const& img, T thresh, DoWhatWithPoint const& f)
	{
		std::size_t const width = img._width;
		std::size_t const height = img._height;
		if(width == 0 || height == 0) return;
		auto const data = img._data;
		if(width == 1)
		{
			if(height == 1)
			{
				if(data[0] <= thresh) f(0, 0);
			}
			else
			{
				if(data[0] <= thresh &&
					data[0] <= data[1]) f(0, 0);
				auto const last = height - 1;
				for(std::size_t y = 1; y < last; ++y)
				{
					if(data[y] <= thresh &&
						data[y] <= data[y - 1] &&
						data[y] >= data[y + 1])
					{
						f(0, y);
					}
				}
				if(data[last] <= thresh &&
					data[last] <= data[last - 1]) f(0, last);
			}
		}
		else //width>1
		{
			if(height == 1)
			{
				if(data[0] <= thresh &&
					data[0] <= data[1]) f(0, 0);
				auto const last = width - 1;
				for(std::size_t x = 0; x < last; ++x)
				{
					if(data[x] <= thresh &&
						data[x] <= data[x - 1] &&
						data[x] <= data[x + 1]) f(x, 0);
				}
				if(data[last] <= thresh &&
					data[last] <= data[last - 1]) f(last, 0);
			}
			else
			{
				auto const lastx = width - 1;
				auto const lasty = height - 1;
				{ //row 0
					if(data[0] <= thresh &&
						data[0] <= data[1] &&
						data[0] <= data[width] &&
						data[0] <= data[width + 1]) f(0, 0);
					for(std::size_t x = 1; x < lastx; ++x)
					{
						if(data[x] <= thresh &&
							data[x] <= data[x - 1] &&
							data[x] <= data[x + 1] &&
							data[x] <= data[x + width - 1] &&
							data[x] <= data[x + width] &&
							data[x] <= data[x + width + 1]) f(x, 0);
					}
					if(data[lastx] <= thresh &&
						data[lastx] <= data[lastx - 1] &&
						data[lastx] <= data[lastx + width - 1] &&
						data[lastx] <= data[lastx + width]
						) f(lastx, 0);
				}
				for(std::size_t y = 1; y < lasty; ++y)
				{
					auto const row = data + y * width;
					auto const prow = row - width;
					auto const nrow = row + width;
					if(row[0] <= thresh &&
						row[0] <= row[1] &&
						row[0] <= prow[0] &&
						row[0] <= prow[1] &&
						row[0] <= nrow[0] &&
						row[0] <= nrow[1]) f(0, y);
					for(std::size_t x = 1; x < lastx; ++x)
					{
						if(row[x] <= thresh &&
							row[x] <= row[x - 1] &&
							row[x] <= row[x + 1] &&
							row[x] <= prow[x - 1] &&
							row[x] <= prow[x] &&
							row[x] <= prow[x + 1] &&
							row[x] <= nrow[x - 1] &&
							row[x] <= nrow[x] &&
							row[x] <= nrow[x + 1]) f(x, y);
					}
					if(row[lastx] <= thresh &&
						row[lastx] <= row[lastx - 1] &&
						row[lastx] <= prow[lastx - 1] &&
						row[lastx] <= prow[lastx] &&
						row[lastx] <= nrow[lastx - 1] &&
						row[lastx] <= nrow[lastx]
						) f(lastx, y);
				}
				{
					auto const row = data + lasty * width;
					auto const prow = row - width;
					if(row[0] <= thresh &&
						row[0] <= row[1] &&
						row[0] <= prow[0] &&
						row[0] <= prow[1]) f(0, lasty);
					for(std::size_t x = 1; x < lastx; ++x)
					{
						if(row[x] <= thresh &&
							row[x] <= row[x - 1] &&
							row[x] <= row[x + 1] &&
							row[x] <= prow[x - 1] &&
							row[x] <= prow[x] &&
							row[x] <= prow[x + 1]) f(x, lasty);
					}
					if(row[lastx] <= thresh &&
						row[lastx] <= row[lastx - 1] &&
						row[lastx] <= prow[lastx - 1] &&
						row[lastx] <= prow[lastx]
						) f(lastx, lasty);
				}
			}
		}
	}
	bool TemplateMatchErase::process(Img& img) const
	{
		auto rects = global_select<1>(img, [](std::array<unsigned char, 1> val)
			{
				return val[0] != 255;
			});
		auto clusters = Cluster::cluster_ranges(rects);
		return cluster_template_match_erase(img, clusters, this->tmplt, this->threshold);
	}
	bool SlidingTemplateMatchEraseExact::process(Img& img) const
	{
		auto const region = resolve_origin_rectangle(img, offsets, origin);
		auto const downsized = (downscaling == 1 && region.left == 0 && region.top == 0 && region.right == img._width && region.bottom == img._height) ?
			cil::CImg(img, true) :
			integral_downscale(img, downscaling, region);
		//downsized.display();
		using Gray = std::array<unsigned char, 1>;
		bool found = false;
		for(std::size_t i = 0; i < downsized_tmplts.size(); ++i)
		{
			auto& downsized_tmplt = downsized_tmplts[i];
			auto counts = sliding_template_match<1, float>(downsized, downsized_tmplt, [](Gray t, Gray i)
				{
					return ImageUtils::gray_diff({t[0]}, {i[0]});
				});
			auto const real_threshold = (1 - threshold) * downsized_tmplt._width * downsized_tmplt._height;
			//unsigned char white[]={255,255,255,255};
			//counts.display();
			auto& tmplt = tmplts[i];
			find_local_min_below_thresh(counts, real_threshold, [&](unsigned int x, unsigned int y)
				{
					found = true;
					auto point = downscaling * ImageUtils::PointUINT{x,y}+region.top_left();
					replacer(img, tmplt, point);
				});
		}
		return found;
	}

	bool PyramidTemplateErase::process(Img& img) const
	{
		auto const smallest = get_downscale(img, scales[0]);
		using Gray = std::array<unsigned char, 1>;
		std::vector<unsigned char> downscale_buffer;
		std::vector<ImageUtils::PointUINT> valid_points;
		decltype(valid_points) valid_points_buff;
		for(size_t i = 0; i < num_images; ++i)
		{
			auto& smallest_template = tmplts[i * (scales.size() + 1)];
			auto counts = sliding_template_match<1, float>(smallest, tmplts[i * (scales.size() + 1)], [](Gray t, Gray i)
				{
					return ImageUtils::gray_diff({t[0]}, {i[0]});
				});
			auto const real_threshold = (1 - threshold) * smallest_template._width * smallest_template._height;
			find_local_min_below_thresh(counts, real_threshold, [&](unsigned int x, unsigned int y)
				{
					valid_points.push_back({x,y});
				});
			for(size_t j = 1; j < scales.size(); ++j)
			{

			}
			for(auto const point : valid_points)
			{
				replacer(img, tmplts[(i + 1) * (scales.size() + 1) - 1], point);
			}
		}
	}

	bool RemoveEmptyLines::process(Img& img) const
	{
		return remove_empty_lines(img, background_threshold, min_space, max_presence);
	}

	bool VertCompress::process(Img& img) const
	{
		return compress_vertical(img, background_threshold, min_vert_space, min_horiz_space, min_horizontal_protection, max_vertical_protection, 0, only_straight_paths, staff_line_length, min_staff_separation);
	}

	bool ResizeToBound::process(Img& img) const
	{
		auto res = [this, &img]()
		{
			if(img._width == width && img._height == height)
			{
				return false;
			}
			auto const needed_ratio = double(width) / double(height);
			auto const img_ratio = double(img._width) / double(img._height);
			unsigned int new_height;
			unsigned int new_width;
			if(needed_ratio < img_ratio)
			{
				new_height = unsigned int(width / img_ratio);
				new_width = width;
			}
			else
			{
				new_width = unsigned int(img_ratio * height);
				new_height = height;
			}
			if(gamma != 1)
			{
				Gamma(gamma).process(img);
				RescaleAbsolute(new_width, new_height, -1, mode).process(img);
				Gamma(1/gamma).process(img);
			}
			else
			{
				RescaleAbsolute(new_width, new_height, -1, mode).process(img);
			}
			return true;
		}();
		if(pad)
		{
			res |= ChangeCanvasSize(width, height, FillRectangle::middle, fill).process(img);
		}
		return res;
	}

	bool SquishToBound::process(Img& img) const
	{
		if(img._width <= this->width && img._height <= this->height)
		{
			return false;
		}
		return ResizeToBound::process(img);
	}

	bool ClusterWiden::process(Img& img) const
	{
		auto const rects = global_select<1>(img, [this](std::array<unsigned char, 1> pixel)
			{
				return (pixel[0] >= _lower_bound) && (pixel[0] <= _upper_bound);
			});
		if(rects.empty())
		{
			return false;
		}
		auto const clusters = Cluster::cluster_ranges(rects);
		auto largest = clusters.begin();
		auto largest_bbox = largest->bounding_box();
		for(auto it = next(largest); it != clusters.end(); ++it)
		{
			auto const bbox = it->bounding_box();
			if(bbox.width() > largest_bbox.width())
			{
				largest = it;
				largest_bbox = bbox;
			}
		}
		auto const rescale_ratio = float(_widen_to) / float(largest_bbox.width());
		auto const new_width = unsigned int(std::round(rescale_ratio * img._width));
		auto const new_height = unsigned int(std::round(rescale_ratio * img._height));
		if(new_width != img._width || new_height != img._height)
		{
			apply_gamma(img, _gamma);
			Rescale(rescale_ratio, _mode).process(img);
			apply_gamma(img, 1.0f/_gamma);
			return true;
		}
		return false;
	}

	bool Invert::process(Img& img) const
	{
		auto const size = [&img]() -> std::size_t
		{
			switch (img._spectrum)
			{
			case 1:
			case 2:
				return img._width * std::size_t(img._height);
			case 3:
			case 4:
				return img._width * std::size_t(img._height) * 3;
			default:
				return 0;
			}
		}();
		auto const data = img.data();
		for(std::size_t i = 0; i < size; ++i)
		{
			data[i] = ~data[i];
		}
		return true;
	}

	bool WhiteToTransparent::process(Img& img) const
	{
		switch(img._spectrum)
		{
			using uchar = unsigned char;
		case 1:
			img = cil::get_map<1>(img, [](std::array<uchar, 1> color)
				{
					return exlib::make_array<uchar>(0, 0, 0, 255 - color[0]);
				});
			break;
		case 3:
			img = cil::get_map<3>(img, [](std::array<uchar, 3> color)
				{
					uchar const brightness = ImageUtils::brightness({color[0], color[1], color[2]});
					return exlib::make_array<uchar>(0, 0, 0, 255 - brightness);
				});
			break;
		default:
			return false;
		}
		return true;
	}

	bool FloodFill::process(Img& img) const
	{
		auto rect=resolve_origin_rectangle(img,_region,_origin);
		std::vector<ImageUtils::horizontal_line<>> lines;
		std::vector<bool> checked(img._width*img._height);
		switch(img._spectrum)
		{
		case 1:
		case 2:
			for(unsigned int y=rect.top;y<rect.bottom;++y)
			{
				for(unsigned int x=rect.left;x<rect.right;++x)
				{
					flood_operation(img,{x,y},
						[&checked,&img,this](auto point)
						{
							auto const coord=point.y*img._width+point.x;
							std::vector<bool>::reference ref=checked[coord];
							if(ref)
							{
								return false;
							}
							ref=true;
							auto const pixel=img(point.x,point.y);
							return pixel>=_lower_bound&&pixel<=_upper_bound;
						},
						[&lines](ImageUtils::horizontal_line<> line)
						{
							lines.push_back(line);
						});
				}
			}
			break;
		case 3:
		case 4:
			for(unsigned int y=rect.top;y<rect.bottom;++y)
			{
				for(unsigned int x=rect.left;x<rect.right;++x)
				{
					flood_operation(img,{x,y},
						[&checked,&img,this](auto point)
						{
							auto const coord=point.y*img._width+point.x;
							std::vector<bool>::reference ref=checked[coord];
							if(ref)
							{
								return false;
							}
							ref=true;
							auto const pixel=ImageUtils::brightness(ImageUtils::ColorRGB{img(point.x,point.y,0,0),img(point.x,point.y,0,1),img(point.x,point.y,0,2)});
							return pixel>=_lower_bound&&pixel<=_upper_bound;
						},
						[&lines](ImageUtils::horizontal_line<> line)
						{
							lines.push_back(line);
						});
				}
			}
			break;
		default:
			return false;
		}
		if(lines.empty())
		{
			return false;
		}
		fill_fix_layers(img,_replacer_color,_replacer_num_layers);
		for(auto const& line:lines)
		{
			fill_selection(img,{line.left, line.right, line.y, line.y+1},_replacer_color.data());
		}
		return true;
	}

	bool FlipVertical::process(Img& img) const
	{
		if (img._height < 2U)
		{
			return false;
		}
		auto const width = std::size_t(img._width);
		auto const height = std::size_t(img._height);
		auto const layers = std::size_t(img._spectrum);
		std::unique_ptr<unsigned char[]> swap_buffer(new unsigned char[width]);
		for (std::size_t layer = 0; layer < layers; ++layer)
		{
			for (std::size_t top = 0, bottom = height - 1; top < bottom; ++top, --bottom)
			{
				auto const top_data = &img(0, top, 0, layer);
				auto const bottom_data = &img(0, bottom, 0, layer);
				std::memcpy(swap_buffer.get(), top_data, width);
				std::memcpy(top_data, bottom_data, width);
				std::memcpy(bottom_data, swap_buffer.get(), width);
			}
		}
		return true;
	}

	bool FlipHorizontal::process(Img& img) const
	{
		if (img._width < 2U)
		{
			return false;
		}
		auto const width = std::size_t(img._width);
		auto const total = width * img._height * img._spectrum/* *img._depth*/;
		auto data_begin = img._data;
		auto const data_end = img._data + total;
		while (data_begin < data_end)
		{
			auto const row_begin = data_begin;
			auto const row_end = row_begin + width;
			std::reverse(row_begin, row_end);
			data_begin = row_end;
		}
		return true;
	}

	bool NormalizeBrightness::process(Img& img) const
	{
		std::vector<unsigned char> good_pixels;
		if (_select_lower_bound == 0)
		{
			switch (img._spectrum)
			{
			case 1:
			case 2:
				std::copy_if(img.begin(), img.end(), std::back_inserter(good_pixels), [this](unsigned char value)
							 {
								 return value <= _select_upper_bound;
							 });
					break;
			case 3:
			case 4:
			{
				auto const size = std::size_t(img._width) * img._height;
				auto const data = img._data;
				for (std::size_t i = 0; i < size; ++i)
				{
					auto const brightness = ImageUtils::brightness({ data[i], data[i + size], data[i + size * 2] });
					if (brightness <= _select_upper_bound)
					{
						good_pixels.push_back(brightness);
					}
				}
				break;
			}
			default:
				return false;
			}
		}
		else
		{
			switch (img._spectrum)
			{
			case 1:
			case 2:
			{
				std::copy_if(img.begin(), img.end(), std::back_inserter(good_pixels), [this](unsigned char value)
							 {
								 return value <= _select_upper_bound && value >= _select_lower_bound;
							 });
				break;
			}
			case 3:
			case 4:
			{
				auto const size = std::size_t(img._width) * img._height;
				auto const data = img._data;
				for (std::size_t i = 0; i < size; ++i)
				{
					auto const brightness = ImageUtils::brightness({ data[i], data[i + size], data[i + size * 2] });
					if (brightness <= _select_upper_bound && brightness >= _select_lower_bound)
					{
						good_pixels.push_back(brightness);
					}
				}
				break;
			}
			default:
				return false;
			}
		}
		if (good_pixels.empty())
		{
			return false;
		}
		auto const median_iterator = good_pixels.begin() + good_pixels.size() / 2;
		std::nth_element(good_pixels.begin(), median_iterator, good_pixels.end());
		auto const median = *median_iterator;
		auto const offset = _median - int(median);
		auto const img_data = img._data;
		auto const img_data_end = [&img]()
		{
			switch (img._spectrum)
			{
			case 1:
			case 2:
				return img._data + img._width * std::size_t(img._height);
			default:
				return img._data + img._width * std::size_t(img._height) * 3;
			}
		}();
		for (auto it = img_data; it != img_data_end; ++it)
		{
			if (*it >= _select_lower_bound && *it <= _select_upper_bound)
			{
				auto const value = *it + offset;
				*it = exlib::clamp<unsigned char>(value);
			}
		}
		return true;
	}

	template<typename T>
	constexpr T saturated_subtract(T a, T b)
	{
		if (b >= a)
		{
			return T{};
		}
		return a - b;
	}

	bool MedianAdaptiveThreshold::process(Img& img) const
	{
		if (img._width == 0 || img._height == 0 || img._spectrum == 0 || img._spectrum > 4)
		{
			return false;
		}
		std::array<unsigned int, 256> histogram{};
		auto const gray_image = [&img, gamma = _gamma]()
		{
			if (gamma != 1)
			{
				auto const gamma_adjust = [gamma](unsigned char input){
					return static_cast<unsigned char>(std::round(std::pow(input / 255.0f, gamma) * 255));
				};
				switch (img._spectrum)
				{
				case 1:
				case 2:
					return cil::get_map<1>(img, [gamma_adjust](std::array<unsigned char, 1> color)
										   {
											   return std::array{ gamma_adjust(color[0]) };
										   });
				case 3:
				case 4:
					return cil::get_map<3>(img, [gamma_adjust](std::array<unsigned char, 3> color)
										   {
											   return std::array{ gamma_adjust(ImageUtils::brightness({ color[0], color[1], color[2] })) };
										   });
				}
			}
			else
			{
				switch (img._spectrum)
				{
				case 1:
					return img;
				case 2:
					return cil::get_map<1>(img, [](std::array<unsigned char, 1> color)
										   {
											   return std::array{ color[0] };
										   });
				case 3:
				case 4:
					return cil::get_map<3>(img, [](std::array<unsigned char, 3> color)
										   {
											   return std::array{ ImageUtils::brightness({ color[0], color[1], color[2] }) };
										   });
				}
			}
			return cil::CImg(img, true);
		}();
		auto const hwidth = _window_width / 2;
		auto const hheight = _window_height / 2;
		auto const hwidth_inv = _window_width - hwidth;
		auto const hheight_inv = _window_height - hheight;
		unsigned int window_left = 0;
		unsigned int window_top = 0;
		unsigned int window_right = std::min(hwidth_inv, img._width);
		unsigned int window_bottom = std::min(hheight_inv, img._height);
		for (unsigned int y = 0; y < window_bottom; ++y)
		{
			for (unsigned int x = 0; x < window_right; ++x)
			{
				++histogram[gray_image(x, y)];
			}
		}
		auto const find_median = [&]()
		{
			auto const window_size = std::size_t(window_right - window_left) * (window_bottom - window_top);
			auto const threshold = window_size / 2;
			std::size_t count = 0;
			for (std::size_t i = 0; i < histogram.size(); ++i)
			{
				count += histogram[i];
				if (count >= threshold)
				{
					return (unsigned char)i;
				}
			}
			return (unsigned char)255;
		};
		bool changed = false;
		auto const color_pixel = [&](unsigned int x, unsigned int y)
		{
			changed = true;
			img(x, y) = _replacer;
			if (img._spectrum >= 3)
			{
				img(x, y, 0, 1) = _replacer;
				img(x, y, 0, 2) = _replacer;
			}
		};
		for (unsigned int x = 0; x < img._width; ++x)
		{
			if (x & 1)
			{
				for (unsigned int y = img._height; y-- > 0;)
				{
					auto const wt_new = saturated_subtract(y, hheight);
					if (wt_new != window_top)
					{
						for (unsigned int x2 = window_left; x2 < window_right; ++x2)
						{
							++histogram[gray_image(x2, wt_new)];
						}
						window_top = wt_new;
					}
					auto const wb_new = std::min(img._height, y + hheight_inv);
					if (wb_new != window_bottom)
					{
						for (unsigned int x2 = window_left; x2 < window_right; ++x2)
						{
							--histogram[gray_image(x2, wb_new)];
						}
						window_bottom = wb_new;
					}
					auto const median = exlib::clamp<unsigned char>(_median_adjustment + find_median());
					if (gray_image(x, y) > median)
					{
						color_pixel(x, y);
					}
				}
			}
			else
			{
				for (unsigned int y = 0; y < img._height; ++y)
				{
					auto const wt_new = saturated_subtract(y, hheight);
					if (wt_new != window_top)
					{
						for (unsigned int x2 = window_left; x2 < window_right; ++x2)
						{
							--histogram[gray_image(x2, window_top)];
						}
						window_top = wt_new;
					}
					auto const wb_new = std::min(img._height, y + hheight_inv);
					if (wb_new != window_bottom)
					{
						for (unsigned int x2 = window_left; x2 < window_right; ++x2)
						{
							++histogram[gray_image(x2, window_bottom)];
						}
						window_bottom = wb_new;
					}
					auto const median = exlib::clamp<unsigned char>(_median_adjustment + find_median());
					if (gray_image(x, y) > median)
					{
						color_pixel(x, y);
					}
				}
			}
			{
				auto const wl_new = saturated_subtract(x, hwidth);
				if (wl_new != window_left)
				{
					for (unsigned int y = window_top; y < window_bottom; ++y)
					{
						--histogram[gray_image(window_left, y)];
					}
					window_left = wl_new;
				}
			}
			auto const wr_new = std::min(img._width, x + hwidth_inv);
			if (wr_new != window_right)
			{
				for (unsigned int y = window_top; y < window_bottom; ++y)
				{
					++histogram[gray_image(window_right, y)];
				}
				window_right = wr_new;
			}
		}
		return changed;
	}

	bool HathiCorrect::process(Img& img) const
	{
		hathi_correct(img, 118, 255, 255, 10);
		return true;
	}
}