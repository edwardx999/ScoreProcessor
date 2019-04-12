Program useful for editing batches of images, specializing in score images  

Useful side feature is mass renaming/copying files.
<pre>
Version: Apr 11 2019 23:36:26 Copyright 2017-2019 Edward Xie  
Syntax: filename_or_folder... command params... ...  
If you want to recursively search a folder, type -r before it  
If a file starts with a dash, double the starting dash: "-my-file.jpg" -> "--my-file.jpg"  
parameters that require multiple values are notated with a comma  
parameters can be tagged to reference a specific input with prefix:value  
prefixes sometimes allow switching between different types of input  
ex: img0.png --image1.jpg my_folder -r rec_folder -fg 180 -ccga 20,50,30 -fr l:100 w:100 h:30  
Type command alone to get readme  
Available commands:  
  Single Page Operations:  
    Convert to Grayscale:   -cg  
    Filter Gray:            -fg min max=255 replacer=255  
    Horizontal Padding:     -hp left right=l tolerance=0.005 background_threshold=128  
    Vertical Padding:       -vp top bottom=l tolerance=0.005 background_threshold=128  
    Straighten:             -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1 boundary=128 gamma=2  
    Rotate:                 -rot angle mode=cubic gamma=2  
    Fill Rectangle:         -fr left top horiz vert color=255 origin=tl  
    Rescale Brightness:     -rcg min mid max=255  
    Cluster Clear Gray:     -ccg required_color_range=0,255 bad_size_range=0,0 sel_range=0,200 repl_color=255 eight_way=false  
    Blur:                   -bl st_dev gamma=2  
    Extract First Layer:    -exl  
    Cover Transparency:     -ct red=255 green=r blue=r  
    Remove Border (DANGER): -rb tolerance=0.9  
    Rescale:                -rs factor interpolation_mode=auto gamma=2  
    Smart Scale:            -ss factor network_path=(search program directory for first network)  
    Crop:                   -crp left top right bottom  
    Rescale Absolute:       -rsa width height ratio=preserve mode=automatic gamma=2  
    Change Canvas Size:     -ccs width=preserve height=preserve origin=tl  
  Multi Page Operations:  
    Splice:                 -spl horiz_pad=3% opt_pad=5% min_pad=1.2% opt_hgt=55% excs_wgt=10 pad_wgt=1 bg=128  
    Cut:                    -cut min_width=66% min_height=8% horiz_weight=20 min_vert_space=0 bg=128  
  Options:  
    Output:                 -o pattern=%w move=false  
    Verbosity:              -vb level  
    Number of Threads:      -nt num  
    Boundary Select:        -bsel first_file1 last_file1 ... first_filen last_filen  
    Starting index:         -si index  
    Filter:                 -flt pattern keep_match  
    List Files:             -list  
    Quality:                -q quality  
Multiple Single Page Operations can be done at once. They are performed in the order they are given.  
A Multi Page Operation can not be done with other operations.
</pre>