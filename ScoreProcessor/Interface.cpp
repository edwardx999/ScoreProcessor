#include "stdafx.h"
#include "Interface.h"

namespace ScoreProcessor {
	namespace Output {
		MakerTFull<UseTuple,Precheck,LabelId,PatternParser,MoveParser>
			maker("Specifies the output format\n"
				"pattern: the output template, see below; tags: o, out, p, pat\n"
				"move: whether to copy or move files (ignored by multi); tags: m, mv, move\n"
				"template specifiers:\n"
				"  %w filename and extension\n"
				"  %c entire filename\n"
				"  %p path without trailing slash\n"
				"  %0 numbers 0-9 indicate index with number of padding\n"
				"  %f filename\n"
				"  %x extension\n"
				"  %% literal percent\n"
				"Double the starting dash if you want the file to start with a dash.",
				"Output",
				"pattern=%w move=false");
	}

	namespace NumThreads {
		MakerTFull<UseTuple,Precheck,empty2,IntegerParser<unsigned int,Name,force_positive>> maker("Controls number of threads, will not exceed number of files","Number of Threads","num");
	}

	namespace Verbosity {
		MakerTFull<UseTuple,Precheck,empty,Level> maker("Changes verbosity of output: Silent=0=s, Errors-only=1=e, Count=2=c (default), Loud=3=l","Verbosity","level");
	}

	namespace StrMaker {
		SingMaker<UseTuple,LabelId,
			DoubleParser<MinAngle,no_check>,DoubleParser<MaxAngle,no_check>,
			DoubleParser<AnglePrec>,DoubleParser<PixelPrec>,
			IntegerParser<unsigned char,Boundary>,FloatParser<Gamma>>
			maker("Straightens the image\n"
				"min angle: minimum angle to consider rotation; tags: mn, min, mna\n"
				"max angle: maximum angle to consider rotation; tags: mx, max, mxa\n"
				"angle prec: quantization of angles to consider; tags: a, ap\n"
				"pixel prec: pixels this close are considered the same; tags: p, pp\n"
				"boundary, vertical transition across this is considered an edge; tags: b\n"
				"gamma: gamma correction applied; tags: g, gam",
				"Straighten",
				"min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1 boundary=128 gamma=2");
	}

	namespace CGMaker {
		SingMaker<UseTuple,empty> maker("Converts the image to grayscale","Convert to Grayscale","");
	}

	namespace FRMaker {
		SingMaker<UseTuple,LabelId,IntegerParser<int,Left>,IntegerParser<int,Top>,Right,Bottom,Color,Origin>
			maker("Fills in a rectangle of specified color\n"
				"left: left coord of rectangle; tags: l, left\n"
				"top: top coord of rectangle; tags: t, top\n"
				"horiz: right coord or width, defaults to right coord; tags: r, right, w, width\n"
				"vert: bottom coord or height, defaults to bottom coord; tags: b, bottom, h, height\n"
				"color: color to fill with, can be grayscale, rgb, or rgba with comma-separated values,\n"
				"  tags: clr, color\n"
				"origin: origin from which coords are taken, +y is always down, +x is always right; tags: o, or",
				"Fill Rectangle",
				"left top horiz vert color=255 origin=tl");
	}

	namespace GamMaker {
		Maker maker;
	}

	namespace RotMaker {
		SingMaker<UseTuple,LabelId,Degrees,Mode,GammaParser> maker(
			"Rotates the image\n"
			"angle: angle to rotate the image, ccw is positive, defaults to degrees; tags: d, deg, r, rad\n"
			"interpolation_mode: see below; tags: i, im, m\n"
			"gamma: gamma correction for rotation; tags: g, gam\n"
			"Modes are:\n"
			"  nearest neighbor\n"
			"  linear\n"
			"  cubic\n"
			"To specify mode, type the first letter of the mode; other characters are ignored",
			"Rotate",
			"angle mode=cubic gamma=2");
	}

	namespace RsMaker {
		SingMaker<UseTuple,LabelId,FloatParser<Factor,no_negatives>,Mode,RotMaker::GammaParser>
			maker("Rescales image by given factor\n"
				"factor: factor to scale image by; tags: f, fact\n"
				"interpolation_mode: see below; tags: i, im\n"
				"gamma: gamma correction applied; tags: g, gam\n"
				"Rescale modes are:\n"
				"  auto (moving average if downscaling, else cubic)\n"
				"  nearest neighbor\n"
				"  moving average\n"
				"  linear\n"
				"  grid\n"
				"  cubic\n"
				"  lanczos\n"
				"To specify mode, type as many letters as needed to unambiguously identify mode",
				"Rescale",
				"factor interpolation_mode=auto gamma=2");
	}

	namespace FGMaker {
		SingMaker<UseTuple,LabelId,
			IntegerParser<unsigned char,Min>,
			IntegerParser<unsigned char,Max>,
			IntegerParser<unsigned char,Replacer>> maker(
				"Replacers pixels with brightness [min,max] with replacer\n"
				"min: minimum brightness to replace; tags: mn, min, mnv\n"
				"max: maximum brightness to replace; tags: mx, max, mxv\n"
				"replacer: color to replacer with; tags: r, rep\n",
				"Filter Gray",
				"min max=255 replacer=255");
	}

	namespace BSel {
		BoundSelMaker maker;
	}

	namespace List {
		MakerTFull<UseTuple,Precheck,empty> maker("Makes program list out files","List Files","");
	}

	namespace SIMaker {
		MakerTFull<UseTuple,Precheck,empty,IntegerParser<unsigned int,Number>> maker("Indicates the starting index to number files","Starting index","index");
	}

	namespace RgxFilter {
		MakerTFull<UseTuple,empty,empty2,Regex,KeepMatch> maker(
			"Filters the input files by a regex pattern\n"
			"regex: pattern\n"
			"keep_match: whether matches are kept or removed",
			"Filter",
			"pattern keep_match");
	}

	namespace CCGMaker {
		MakerTFull<UseTuple,Precheck,LabelId,RCR,BSR,SelRange,IntegerParser<unsigned char,Replacer>> maker(
			"Clears clusters of specific constraints\n"
			"required_color_range: clusters that do not contains a color in this range are replaced;\n"
			"  tags: rcr\n"
			"bad_size_range: clusters within this size range are replaced; tags: bsr\n"
			"selection_range: pixels in this color range will be clustered; tags: sr\n"
			"replacement_color: chosen colors are replaced by this color; tags: rc, bc\n",
			"Cluster Clear Gray",
			"required_color_range=0,255 bad_size_range=0,0 sel_range=0,200 repl_color=255");
	}

	namespace BlurMaker {
		SingMaker<UseTuple,empty,FloatParser<StDev,force_positive>,RotMaker::GammaParser> maker(
			"Does a gaussian blur of given standard deviation\n"
			"st_dev: standard deviation of blur\n"
			"gamma: gamma correction applied\n",
			"Blur",
			"st_dev gamma=2");
	}

	namespace EXLMaker {
		SingMaker<UseTuple,empty> maker("Extracts the first layer with no reallocation (cheap convert to grayscale)","Extract first layer","");
	}

	namespace CTMaker {
		SingMaker<UseTuple,LabelId,Red,Green,Blue> maker(
			"Mixes transparent pixels with the given rgb color"
			"red tags: r, red\n"
			"green tags: g, green\n"
			"blue tags: b, blue\n"
			"A value may be given or you can specify that one channel should be equal to another",
			"Cover Transparency",
			"red=255 green=r blue=r");
	}

	namespace RBMaker {
		SingMaker<UseTuple,empty,FloatParser<Tol,force_positive>> maker(
			"Flood fills pixels from edge with tolerance of black with white\n"
			"Neither reliable nor safe and you should probably not use it",
			"Remove Border (DANGER)",
			"tolerance=0.9");
	}

	namespace HPMaker {
		SingMaker<UseTuple,LabelId,Left,Right,Tol,BGParser> maker(
			"Pads the left and right sides of image.\n"
			"left: left padding, use k to keep padding, or r to assign equal to right,\n"
			"  use lpw or lph to calculate it as a proportion of width or height respectively;\n"
			"  tags: l, left, lph, lpw\n"
			"right: right padding, use k to keep padding, or l to assign equal to left,\n"
			"  use rpw or rph to calculate it as a proportion of width or height respectively;\n"
			"  tags: r, right, rph, rpw\n"
			"tolerance: this many pixels below background threshold is considered the side,\n"
			"  use tpw or tph to calculate it as a proportion of width or height respectively;\n"
			"  tags: tol, tph, tpw\n"
			"background_threshold: see tolerance; tags: bg",
			"Horizontal Padding",
			"left right=l tolerance=0.005 background_threshold=128"
		);
	}

	namespace VPMaker {
		extern SingMaker<UseTuple,LabelId,Top,Bottom,Tol,HPMaker::BGParser> maker(
			"Pads the top and bottom sides of image.\n"
			"top: top padding, use k to keep padding, or b to assign equal to bottom,\n"
			"  use lpw or lph to calculate it as a proportion of width or height respectively;\n"
			"  tags: t, top, tph, tpw\n"
			"bottom: bottom padding, use k to keep padding, or t to assign equal to top,\n"
			"  use rpw or rph to calculate it as a proportion of width or height respectively;\n"
			"  tags: b, bottom, bph, bpw\n"
			"tolerance: this many pixels below background threshold is considered the side,\n"
			"  use tpw or tph to calculate it as a proportion of width or height respectively;\n"
			"  tags: tol, tph, tpw\n"
			"background_threshold: see tolerance; tags: bg",
			"Horizontal Padding",
			"top bottom=l tolerance=0.005 background_threshold=128"
		);
	}
}