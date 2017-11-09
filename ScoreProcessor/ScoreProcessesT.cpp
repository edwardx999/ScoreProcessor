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
		destloc=0;
	}
}