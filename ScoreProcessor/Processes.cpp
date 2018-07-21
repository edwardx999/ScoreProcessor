#include "stdafx.h"
#include "Processes.h"

namespace ScoreProcessor {

	bool ChangeToGrayscale::process(Img& img) const
	{
		if(img._spectrum>=3)
		{
			img=get_grayscale_simple(img);
			return true;
		}
		return false;
	}

	bool FillTransparency::process(Img& img) const
	{
		if(img._spectrum>=4)
		{
			return fill_transparency(img,background);
		}
		return false;
	}

	bool RemoveBorderGray::process(Img& img) const
	{
		if(img._spectrum==1)
		{
			return remove_border(img,0,tolerance);
		}
		else
		{
			throw std::invalid_argument("Remove process requires Grayscale image");
		}
	}
	bool FilterGray::process(Img& img) const
	{
		if(img._spectrum<3)
		{
			return replace_range(img,min,max,replacer);
		}
		else
		{
			return replace_by_brightness(img,min,max,ImageUtils::ColorRGB({replacer,replacer,replacer}));
		}
	}

	bool FilterHSV::process(Img& img) const
	{
		if(img._spectrum>=3)
		{
			return replace_by_hsv(img,start,end,replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}

	bool FilterRGB::process(Img& img) const
	{
		if(img._spectrum>=3)
		{
			return replace_by_rgb(img,start,end,replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}

	bool PadHoriz::process(Img& img) const
	{
		return horiz_padding(img,left,right,tol(img._height),background);
	}

	bool PadVert::process(Img& img) const
	{
		return vert_padding(img,top,bottom,tol(img._width),background);
	}

	bool PadAuto::process(Img& img) const
	{
		return auto_padding(img,vert,max_h,min_h,hoff,opt_rat);
	}

	bool PadCluster::process(Img& img) const
	{
		return cluster_padding(img,left,right,top,bottom,bt);
	}

	bool Crop::process(Img& img) const
	{
		auto const w=img.width();
		auto const h=img.height();
		using rint=ImageUtils::Rectangle<int>;
		rint region;
		region.left=left(w,h);
		region.right=right(w,h);
		region.top=top(w,h);
		region.bottom=bottom(w,h);
		if(region==rint({0,w,0,h}))
		{
			return false;
		}
		region.right--;
		region.bottom--;
		img=get_crop_fill(img,region,unsigned char(255));
		return true;
	}

	bool Rescale::process(Img& img) const
	{
		img.resize(
			static_cast<unsigned int>(std::round(img._width*val)),
			static_cast<unsigned int>(std::round(img._height*val)),
			img._depth,
			img._spectrum,
			interpolation);
		return true;
	}

	bool ExtractLayer0NoRealloc::process(Img& img) const
	{
		if(img._spectrum>1U)
		{
			img._spectrum=1U;
			return true;
		}
		return false;
	}

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

	bool ClusterClearGrayAlt::process(Img& img) const
	{
		if(img._spectrum<3)
		{
			return clear_clusters(img,std::array<unsigned char,1>({background}),
				[this](std::array<unsigned char,1> v)
			{
				return v[0]>=sel_min&&v[0]<=sel_max;
			},
				[this,&img](Cluster const& c)
			{
				auto size=c.size();
				if(size>=min_size&&size<=max_size)
				{
					return true;
				}
				if(sel_min>=required_min&&sel_max<=required_max)
				{
					return false;
				}
				for(auto rect:c.get_ranges())
				{
					for(unsigned int y=rect.top;y<rect.bottom;++y)
					{
						auto row=img._data+y*img._width;
						for(unsigned int x=rect.left;x<rect.right;++x)
						{
							auto pix=*(row+x);
							if(pix>=required_min&&pix<=required_max)
							{
								return false;
							}
						}
					}
				}
				return true;
			});
		}
		else
		{
			return clear_clusters(img,std::array<unsigned char,3>({background,background,background}),
				[this](std::array<unsigned char,3> v)
			{
				auto const brightness=(float(v[0])+v[1]+v[2])/3.0f;
				return brightness>=sel_min&&brightness<=sel_max;
			},
				[this,&img](Cluster const& c)
			{
				auto size=c.size();
				if(size>=min_size&&size<=max_size)
				{
					return true;
				}
				if(sel_min>=required_min&&sel_max<=required_max)
				{
					return false;
				}
				unsigned int const img_size=img._width*img._height;
				for(auto rect:c.get_ranges())
				{
					for(unsigned int y=rect.top;y<rect.bottom;++y)
					{
						auto row=img._data+y*img._width;
						for(unsigned int x=rect.left;x<rect.right;++x)
						{
							auto pix=(row+x);
							float brightness=0;
							for(unsigned int s=0;s<3;++s)
							{
								brightness+=*(pix+s*img_size);
							}
							if(brightness>=3U*required_min&&brightness<=3U*required_max)
							{
								return false;
							}
						}
					}
				}
				return true;
			});
		}
	}

	bool RescaleGray::process(Img& img) const
	{
		if(img._spectrum<3)
		{
			rescale_colors(img,min,mid,max);
		}
		else
		{
			rescale_colors(img,min,mid,max);
			rescale_colors(img.get_shared_channel(1),min,mid,max);
			rescale_colors(img.get_shared_channel(2),min,mid,max);
		}
		return true;
	}

	ImageUtils::Point<signed int> get_origin(FillRectangle::origin_reference origin_code,int width,int height)
	{
		ImageUtils::Point<signed int> porigin;
		switch(origin_code%3)
		{
			case 0:
				porigin.x=0;
				break;
			case 1:
				porigin.x=width/2;
				break;
			case 2:
				porigin.x=width;
		}
		switch(origin_code/3)
		{
			case 0:
				porigin.y=0;
				break;
			case 1:
				porigin.y=height/2;
				break;
			case 2:
				porigin.y=height;
		}
		return porigin;
	}

	bool FillRectangle::process(Img& img) const
	{
		auto const porigin=get_origin(origin,img.width(),img.height());
		ImageUtils::Rectangle<signed int> rect;
		rect.left=offsets.left+porigin.x;
		rect.right=offsets.right+porigin.x;
		rect.top=offsets.top+porigin.y;
		rect.bottom=offsets.bottom+porigin.y;
		if(rect.left<0)
		{
			rect.left=0;
		}
		if(rect.top<0)
		{
			rect.top=0;
		}
		if(rect.right>img.width())
		{
			rect.right=img.width();
		}
		if(rect.bottom>img.height())
		{
			rect.bottom=img.height();
		}
		if(num_layers<5)
		{
			switch(img._spectrum)
			{
				case 1:
				case 2:
					if(num_layers==3||num_layers==4&&color[3]==255)
						img=cil::get_map<1,3>(img,[](auto color)
					{
						return std::array<unsigned char,3>({color[0],color[0],color[0]});
					});
					else if(num_layers==4)
						img=cil::get_map<1,4>(img,[a=color[3]](auto color)
					{
						return std::array<unsigned char,4>({color[0],color[0],color[0],a});
					});
					break;
				case 3:
					if(num_layers==4&&color[3]!=255)
						img=cil::get_map<3,4>(img,[a=color[3]](auto color){
						return std::array<unsigned char,4>({color[0],color[1],color[2],a});});
			}
		}
#define ucast static_cast<unsigned int>
		return fill_selection(
			img,
			{ucast(rect.left),ucast(rect.right),ucast(rect.top),ucast(rect.bottom)},
			color.data(),check_fill_t());
#undef ucast
	}

	bool Blur::process(Img& img) const
	{
		img.blur(radius);
		return true;
	}

	bool Straighten::process(Img& img) const
	{
		auto angle=find_angle_bare(img,pixel_prec,min_angle,max_angle,num_steps,boundary);
		if(angle==0)
		{
			return false;
		}
		apply_gamma(img,gamma);
		img.rotate(angle*RAD_DEG,2,1);
		apply_gamma(img,1/gamma);
		return true;
	}

	bool Rotate::process(Img& img) const
	{
		img.rotate(angle,mode,1);
		return true;
	}

	bool Gamma::process(Img& img) const
	{
		apply_gamma(img,gamma);
		return true;
	}
}