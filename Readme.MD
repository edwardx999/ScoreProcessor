An application useful in editing musical scores.

Syntax:  
filename_or_folder command params... ...  
Type command alone to get readme

Available commands:  
&nbsp;&nbsp;Single Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Convert to Grayscale:&nbsp&nbsp&nbsp&nbsp&nbsp-cg  
&nbsp;&nbsp;&nbsp;&nbsp;Filter Gray:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-fg min_value max_value=255 replacer=255  
&nbsp;&nbsp;&nbsp;&nbsp;Cluster Clear Grayscale:&nbsp&nbsp-ccg max_size min_size=0 background_color=255 tolerance=0.042  
&nbsp;&nbsp;&nbsp;&nbsp;Horizontal Padding:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-hp amount  
&nbsp;&nbsp;&nbsp;&nbsp;Vertical Padding:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-vp amount  
&nbsp;&nbsp;&nbsp;&nbsp;Auto Padding:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-ap vert_padding min_horiz_padding max_horiz_padding horiz_offset=0 optimal_ratio=1.777778  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale Colors Grayscale:&nbsp-rcg min mid max  
&nbsp;&nbsp;&nbsp;&nbsp;Blur:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-bl radius  
&nbsp;&nbsp;&nbsp;&nbsp;Straighten:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1  
&nbsp;&nbsp;&nbsp;&nbsp;Remove Border:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-rb tolerance=0.5  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-rs factor  
&nbsp;&nbsp;&nbsp;&nbsp;Fill Rectangle Gray:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-fr left top right bottom color=255  
&nbsp;&nbsp;Multi Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Cut:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-cut  
&nbsp;&nbsp;&nbsp;&nbsp;Splice:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-spl horiz_padding optimal_padding min_vert_padding optimal_height=(6/11 width of first page)  
&nbsp;&nbsp;Options:  
&nbsp;&nbsp;&nbsp;&nbsp;Output:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-o format  
&nbsp;&nbsp;&nbsp;&nbsp;Verbosity:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-vb level  
&nbsp;&nbsp;&nbsp;&nbsp;Filter Files:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp-flt regex remove=false
    
Multiple Single Page Operations can be done at once. They are performed in the order they are given.  
A Multi Page Operation can not be done with other operations.


Copyright (C) 2017-2018 Edward Xie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
