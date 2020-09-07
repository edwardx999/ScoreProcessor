Program useful for editing batches of images, specializing in score images  
Useful side feature is mass renaming/copying files.  
Give no arguments to get readme
<pre>
Version: Nov 03 2019 22:31:12 Copyright 2017-2019 Edward Xie
Syntax: filename_or_folder... command params... ...
If you want to recursively search a folder, type -r before it
If a file starts with a dash, double the starting dash: "-my-file.jpg" -> "--my-file.jpg"
parameters that require multiple values are notated with a comma
parameters can be tagged to reference a specific input with prefix:value
prefixes sometimes allow switching between different types of input
ex: img0.png --image1.jpg my_folder -r rec_folder -fg 180 -ccg bsr:0,30 -fr l:100 w:100 h:30 t:0 -o %f.%x t
Type command alone to get readme
Available commands:
  Single Page Operations:
    Convert to Grayscale:          -cg 
    Filter Gray:                   -fg min max=255 replacer=255
    Horizontal Padding:            -hp left right=l tolerance=0.005 background_threshold=128
    Vertical Padding:              -vp top bottom=t tolerance=0.005 background_threshold=128
    Straighten:                    -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1 boundary=128 gamma=2 use_horiz=t
    Rotate:                        -rot angle mode=cubic gamma=2
    Fill Rectangle:                -fr left top horiz vert color=255 origin=tl
    Rescale Brightness:            -rcg min mid max=255
    Cluster Clear Gray:            -ccg required_color_range=0,255 bad_size_range=0,0 sel_range=0,200 repl_color=255 eight_way=false
    Blur:                          -bl st_dev gamma=2
    Extract First Layer:           -exl 
    Cover Transparency:            -ct red=255 green=r blue=r
    Remove Border (DANGER):        -rb tolerance=0.9
    Rescale:                       -rs factor interpolation_mode=auto gamma=2
    Crop:                          -crp left top horizontal vertical
    Rescale Absolute:              -rsa width height ratio=preserve mode=automatic gamma=2
    Change Canvas Size:            -ccs width=preserve height=preserve origin=tl
    Morphological AA:              -mlaa contrast_threshold=128 gamma=2
    Cluster Match Erase:           -tme template_file_name threshold=0.95
    Sliding Erase Downscale Match: -stme template_file_names downscale thresh=0.95 replace=fill:255 l=-99999 t=-999999 h=99999 v=99999 o=tl
    Remove Empty Lines:            -rel min_space max_presence=5 background_threshold=128
    Vertical Compress:             -vc min_vert_space min_horiz_pr max_vert_pr background=128 min_horiz_space=mvs
  Multi Page Operations:
    Splice:                        -spl horiz_pad=3% opt_pad=5% min_pad=1.2% opt_hgt=55% excs_wgt=10 pad_wgt=1 bg=128 divider=""
    Cut:                           -cut min_width=66% min_height=8% horiz_weight=20 min_vert_space=0 bg=128
  Options:
    Output:                        -o pattern=%w move=false
    Verbosity:                     -vb level
    Number of Threads:             -nt num
    Boundary Select:               -bsel first_file1 last_file1 ... first_filen last_filen
    Starting index:                -si index
    Filter:                        -flt pattern keep_match
    List Files:                    -list 
    Quality:                       -q quality
Multiple Single Page Operations can be done at once. They are performed in the order they are given.
A Multi Page Operation can not be done with other operations.
</pre>

Building

Just a Visual Studio Solution

Get the following dependencies with vcpkg:

libjpeg-turbo:x(64|86)-windows-static
tiff:x(64|86)-windows-static
libpng:x(64|86)-windows-static

~~See READMEs in ScoreProcessor/lib/cudnn and ScoreProcessor/lib/mkldnn~~

Because I started this as a C++ noob, I broke some stuff with the Debug build, and have not bothered to fix it. Only Release is known to work.

Install

get sproc.zip from Releases, unzip to a directory, and add that directory to PATH. Running addme.bat should do this automatically.
You will need microsoft redistributable c++. If you do not have it, you can get it here: https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads
