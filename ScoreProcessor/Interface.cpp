#include "stdafx.h"
#include "Interface.h"

namespace ScoreProcessor {
	namespace Output {
		MakerTFull<UseTuple,Precheck,LabelId,PatternParser,MoveParser>
			maker;
	}

	namespace NumThreads {
		MakerTFull<UseTuple,empty,empty2,IntegerParser<unsigned int,Name,positive>> maker;
	}

	namespace Verbosity {
		MakerTFull<UseTuple,Precheck,empty,Level> maker;
	}

	namespace StrMaker {
		SingMaker<UseTuple,LabelId,
			DoubleParser<MinAngle,no_check>,DoubleParser<MaxAngle,no_check>,
			DoubleParser<AnglePrec>,DoubleParser<PixelPrec>,
			IntegerParser<unsigned char,Boundary>,FloatParser<Gamma>>
			maker;
	}

	namespace CGMaker {
		SingMaker<UseTuple,empty> maker;
	}

	namespace FRMaker {
		SingMaker<UseTuple,LabelId,IntegerParser<int,Left>,IntegerParser<int,Top>,Right,Bottom,Color,Origin>
			maker;
	}

	namespace GamMaker {
		Maker maker;
	}

	namespace RotMaker {
		SingMaker<UseTuple,LabelId,Radians,Mode,GammaParser> maker;
	}

	namespace RsMaker {
		SingMaker<UseTuple,LabelId,FloatParser<Factor,no_negatives>,Mode,RotMaker::GammaParser>
			maker;
	}
}