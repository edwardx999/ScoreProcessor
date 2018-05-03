An application useful in editing musical scores.
Using -o in combination with -flt to mass rename files is a very useful side feature of this program.

Syntax:  
filename_or_folder command params... ...  
Type command alone to get readme  
Parameters that require multiple values are notated with a comma  
Example: myimage.png -cg -fg 180 -ccga 0,50 ,30 -o %f2.jpg  
  
Available commands:  
&nbsp;&nbsp;Single Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Convert to Grayscale:     -cg  
&nbsp;&nbsp;&nbsp;&nbsp;Filter Gray:              -fg min_value max_value=255 replacer=255  
&nbsp;&nbsp;&nbsp;&nbsp;Cluster Clear Grayscale:  -ccg max_size min_size=0 background_color=255 tolerance=0.042  
&nbsp;&nbsp;&nbsp;&nbsp;Cluster Clear Gray Alt:   -ccga required_color_range=0, bad_size_range=0, sel_range=0,200 bg_color=255  
&nbsp;&nbsp;&nbsp;&nbsp;Horizontal Padding:       -hp amount  
&nbsp;&nbsp;&nbsp;&nbsp;Vertical Padding:         -vp amount  
&nbsp;&nbsp;&nbsp;&nbsp;Auto Padding:             -ap vert_padding min_horiz_padding max_horiz_padding horiz_offset=0 optimal_ratio=1.777778  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale Colors Grayscale: -rcg min mid max  
&nbsp;&nbsp;&nbsp;&nbsp;Blur:                     -bl radius  
&nbsp;&nbsp;&nbsp;&nbsp;Straighten:               -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1  
&nbsp;&nbsp;&nbsp;&nbsp;Remove Border:            -rb tolerance=0.5  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale:                  -rs factor  
&nbsp;&nbsp;&nbsp;&nbsp;Fill Rectangle Gray:      -fr left top right bottom color=255  
&nbsp;&nbsp;Multi Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Cut:                      -cut  
&nbsp;&nbsp;&nbsp;&nbsp;Splice:                   -spl horiz_padding optimal_padding min_vert_padding optimal_height=(6/11 width of first page)  
&nbsp;&nbsp;Options:  
&nbsp;&nbsp;&nbsp;&nbsp;Output:                   -o format  
&nbsp;&nbsp;&nbsp;&nbsp;Verbosity:                -vb level  
&nbsp;&nbsp;&nbsp;&nbsp;Filter Files:             -flt regex remove=false  
&nbsp;&nbsp;&nbsp;&nbsp;Number of Threads         -nt num
    
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
