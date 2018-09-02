An application useful in editing musical scores.  

Version: Sep 02 2018 17:02:20 Copyright 2017-2018 Edward Xie
Syntax: filename_or_folder... command params... ...  
If you want to recursively search a folder, type -r before it  
If a file starts with a dash, double the starting dash: "-my-file.jpg" -> "--my-file.jpg"  
parameters that require multiple values are notated with a comma  
parameters can be tagged to reference a specific input with prefix:value  
prefixes sometimes allow switching between different types of input  
ex: img0.png --image1.jpg my_folder -r rec_folder -fg 180 -ccga 20,50,30  
Type command alone to get readme  
Available commands:  
&nbsp;&nbsp;Single Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Convert to Grayscale&nbsp;&nbsp;&nbsp;&nbsp;-cg   
&nbsp;&nbsp;&nbsp;&nbsp;Filter Gray&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-fg min max=255 replacer=255  
&nbsp;&nbsp;&nbsp;&nbsp;Horizontal Padding&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-hp left right=l tolerance=0.005 background_threshold=128  
&nbsp;&nbsp;&nbsp;&nbsp;Vertical Padding&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-vp top bottom=l tolerance=0.005 background_threshold=128  
&nbsp;&nbsp;&nbsp;&nbsp;Straighten&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1 boundary=128 gamma=2  
&nbsp;&nbsp;&nbsp;&nbsp;Rotate&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-rot angle mode=cubic gamma=2  
&nbsp;&nbsp;&nbsp;&nbsp;Fill Rectangle&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-fr left top horiz vert color=255 origin=tl  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale Brightness&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-rcg min mid max=255  
&nbsp;&nbsp;&nbsp;&nbsp;Cluster Clear Gray&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-ccg required_color_range=0,255 bad_size_range=0,0 sel_range=0,200 repl_color=255 eight_way=false  
&nbsp;&nbsp;&nbsp;&nbsp;Blur&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-bl st_dev gamma=2  
&nbsp;&nbsp;&nbsp;&nbsp;Extract First Layer&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-exl   
&nbsp;&nbsp;&nbsp;&nbsp;Cover Transparency&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-ct red=255 green=r blue=r  
&nbsp;&nbsp;&nbsp;&nbsp;Remove Border (DANGER)&nbsp;&nbsp;-rb tolerance=0.9  
&nbsp;&nbsp;&nbsp;&nbsp;Rescale&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-rs factor interpolation_mode=auto gamma=2  
&nbsp;&nbsp;Multi Page Operations:  
&nbsp;&nbsp;&nbsp;&nbsp;Splice&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-spl horiz_pad=3% opt_pad=5% min_pad=1.2% opt_hgt=55% excs_wgt=10 pad_wgt=1 bg=128  
&nbsp;&nbsp;&nbsp;&nbsp;Cut&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-cut min_width=66% min_height=8% horiz_weight=20 min_vert_space=0 bg=128  
&nbsp;&nbsp;Options:  
&nbsp;&nbsp;&nbsp;&nbsp;Output&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-o pattern=%w move=false  
&nbsp;&nbsp;&nbsp;&nbsp;Verbosity&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-vb level  
&nbsp;&nbsp;&nbsp;&nbsp;Number of Threads&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-nt num  
&nbsp;&nbsp;&nbsp;&nbsp;Boundary Select&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-bsel first_file1 last_file1 ... first_filen last_filen  
&nbsp;&nbsp;&nbsp;&nbsp;Starting index&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-si index  
&nbsp;&nbsp;&nbsp;&nbsp;Filter&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-flt pattern keep_match  
&nbsp;&nbsp;&nbsp;&nbsp;List Files&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-list   
Multiple Single Page Operations can be done at once. They are performed in the order they are given.  
A Multi Page Operation can not be done with other operations.  

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,either version 3 of the License, or (at your option) any later version.  
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.  
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.