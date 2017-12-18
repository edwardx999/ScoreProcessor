template<typename T>
void ScoreProcessor::copy_shift_selection(cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> selection,int const shiftx,int const shifty) {
	if(shiftx==0&&shifty==0) return;
	selection.left=selection.left+shiftx;if(selection.left>image._width) selection.left=0;
	selection.right=selection.right+shiftx;if(selection.right>image._width) selection.right=image._width;
	selection.top=selection.top+shifty;if(selection.top>image._height) selection.top=0;
	selection.bottom=selection.bottom+shifty;if(selection.bottom>image._height) selection.bottom=image._height;
	unsigned int numChannels=image._spectrum;
	if(shiftx>0)
	{
		if(shifty>0) //shiftx>0 and shifty>0
		{
			for(unsigned int x=selection.right;x-->selection.left;)
			{
				for(unsigned int y=selection.bottom;y-->selection.top;)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx>0 and shifty<=0
		{
			for(unsigned int x=selection.right;x-->selection.left;)
			{
				for(unsigned int y=selection.top;y<selection.bottom;++y)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
	else
	{
		if(shifty>0) //shiftx<=0 and shifty>0
		{
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				for(unsigned int y=selection.bottom;y-->selection.top;)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx<=0 and shifty<=0
		{
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				for(unsigned int y=selection.top;y<selection.bottom;++y)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
}
template<typename T>
void ScoreProcessor::fill_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> const& selection,T const* const values) {
	for(unsigned int x=selection.left;x<selection.right;++x)
	{
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			for(unsigned int s=0;s<image._spectrum;++s)
			{
				image(x,y,s)=values[s];
			}
		}
	}
}
inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const& selection,ImageUtils::ColorRGB const color) {
	ScoreProcessor::fill_selection(image,selection,reinterpret_cast<unsigned char const*>(&color));
}
inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const& selection,ImageUtils::Grayscale const gray) {
	ScoreProcessor::fill_selection(image,selection,reinterpret_cast<unsigned char const*>(&gray));
}
template<typename T>
void ScoreProcessor::copy_paste(::cimg_library::CImg<T> dest,::cimg_library::CImg<T> src,ImageUtils::Rectangle<unsigned int> selection,ImageUtils::Point<signed int> destloc) {
	if(destloc.x<0)
	{
		select.left-=destloc.x;
		destloc.x=0;
	}
	if(destloc.y<0)
	{
		select.top-=destloc.y;
		destloc.y=0;
	}
	for(;destloc.x<dest._width&&select.left<select.right;++destloc.x,++select.left)
	{
		for(;destloc.y<dest._height&&select.top<select.bottom;++destloc.y,++select.top)
		{
			dest(destloc.x,destloc.y)=src(select.left,select.top);
		}
	}
}
template<typename T>
void ScoreProcessor::clear_clusters(
	::cimg_library::CImg<T>& image,
	T const* const color,
	float(*color_diff)(T const* const,T const* const),
	float tolerance,
	bool invert_selection,
	unsigned int min_size,unsigned int max_size,
	T const* const background) {
	auto ranges=global_select(image,color,color_diff,tolerance,invert_selection);
	auto clusters=ScoreProcessor::Cluster::cluster_ranges(ranges);
	for(auto& cluster:clusters)
	{
		unsigned int size=cluster->size();
		if(min_size<=size&&size<max_size)
		{
			auto const& rects=cluster->get_ranges();
			for(auto const& rect:rects)
			{
				ScoreProcessor::fill_selection(image,rect,background);
			}
		}
	}
}
template<typename T>
::std::vector<ImageUtils::Rectangle<unsigned int>> ScoreProcessor::global_select(
	::cimg_library::CImg<T>& image,
	T const* const color,
	float(*color_diff)(T const* const,T const* const),
	float tolerance,
	bool invert_selection) {
	vector<RectangleUINT> container;
	//std::unique_ptr<T[]> pixel_color=make_unique<T[]>(3);
	T* pixel_color=new T[image._spectrum];

	unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
	for(unsigned int y=0;y<image._height;++y)
	{
		for(unsigned int x=0;x<image._width;++x)
		{
			switch(rangeFound)
			{
				case 0: {
				#define load_color() for(unsigned int i=0;i<image._spectrum;++i) pixel_color[i]=image(x,y,i);
					load_color();
					if((color_diff(color,pixel_color)<=tolerance)^invert_selection)
					{
						rangeFound=1;
						rangeStart=x;
					}
					break;
				}
				case 1: {
					load_color();
					if((color_diff(color,pixel_color)>tolerance)^invert_selection)
					{
						rangeFound=2;
						rangeEnd=x;
					}
					else
					{
						break;
					}
				}
					#undef load_color
				case 2: {
					container.push_back(RectangleUINT{rangeStart,rangeEnd,y,y+1});
					rangeFound=0;
					break;
				}
			}
		}
		if(1==rangeFound)
		{
			container.push_back(RectangleUINT{rangeStart,image._width,y,y+1});
			rangeFound=0;
		}
	}

	ImageUtils::compress_rectangles(container);
	delete[] pixel_color;
	return container;
}