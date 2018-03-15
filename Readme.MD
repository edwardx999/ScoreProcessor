An application useful in editing musical scores.

Syntax:  
filename_or_folder command params... ...  
Type command alone to get readme

Available commands:  
  Single Page Operations:  
    Convert to Grayscale:     -cg  
    Filter Gray:              -fg min_value max_value=255 replacer=255  
    Cluster Clear Grayscale:  -ccg max_size min_size=0 background_color=255 tolerance=0.042  
    Horizontal Padding:       -hp amount  
    Vertical Padding:         -vp amount  
    Auto Padding:             -ap vert_padding min_horiz_padding max_horiz_padding horiz_offset=0 optimal_ratio=1.777778  
    Rescale Colors Grayscale: -rcg min mid max  
    Blur:                     -bl radius  
    Straighten:               -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1  
    Remove Border:            -rb tolerance=0.5  
    Rescale:                  -rs factor  
    Fill Rectangle Gray:      -fr left top right bottom color=255  
  Multi Page Operations:  
    Cut:                      -cut  
    Splice:                   -spl horiz_padding optimal_padding min_vert_padding optimal_height=(6/11 width of first page)  
  Options:  
    Output:                   -o format  
    Verbosity:                -vb level  
    Filter Files:             -flt regex remove=false
    
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
