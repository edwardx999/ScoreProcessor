#include "stdafx.h"
#include "Interface.h"

namespace ScoreProcessor {
	namespace Output {
		MakerTFull<UseTuple,Precheck,LabelId,PatternParser,MoveParser>
			maker("Specifies the output format:\n"
				" %w filename and extension\n"
				" %c entire filename\n"
				" %p path without trailing slash\n"
				" %0 numbers 0-9 indicate index with number of padding\n"
				" %f filename\n"
				" %x extension\n"
				" %% literal percent\n"
				"pattern tags: o, out, p, pat\n"
				"move tags: mv, move",
				"Output",
				"pattern move=false");
	}

	namespace NumThreads {
		MakerTFull<UseTuple,empty,empty2,IntegerParser<unsigned int,Name,positive>> maker("Controls number of threads, will not exceed number of files","Number of Threads","num");
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
		SingMaker<UseTuple,LabelId,Radians,Mode,GammaParser> maker(
			"Rotates the image\n"
			"angle: angle to rotate the image, ccw is positive, defaults to degrees; tags: d, deg, r, rad\n"
			"interpolation_mode: see below; tags: i, im\n"
			"gamma: gamma correction for rotation; tags: g, gam\n"
			"Modes are:\n"
			"  nearest neighbor\n"
			"  linear\n"
			"  cubic\n"
			"To specify mode, type as many letters as needed to unambiguously identify mode",
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
}