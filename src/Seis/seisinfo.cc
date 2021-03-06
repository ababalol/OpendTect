/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/


#include "seisinfo.h"
#include "seispacketinfo.h"
#include "seisbounds.h"
#include "seistrc.h"
#include "posauxinfo.h"
#include "survinfo.h"
#include "file.h"
#include "dbman.h"
#include "iopar.h"
#include "trckeyzsampling.h"
#include "enums.h"
#include "envvars.h"
#include "keystrs.h"
#include "timeser.h"
#include "ioobjctxt.h"
#include "seisblockstr.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seis2dlineio.h"
#include "od_istream.h"
#include "uistrings.h"

#include <float.h>
#include <iostream>
#include <math.h>

const char* SeisTrcInfo::sKeySamplingInfo = "Sampling information";
const char* SeisTrcInfo::sKeyNrSamples = "Nr of samples";
const char* SeisPacketInfo::sKeyBinIDs = "BinID range";
const char* SeisPacketInfo::sKeyZRange = "Z range";


static BufferString getUsrInfo()
{
    BufferString ret;
    const char* envstr = GetEnvVar( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File::exists(envstr) )
	return ret;

    od_istream strm( envstr );
    BufferString linebuf;
    while ( strm.isOK() )
    {
	strm.getLine( linebuf );
	if ( !linebuf.isEmpty() )
	    ret.addNewLine().add( linebuf );
    }

    return ret;
}


BufferString SeisPacketInfo::defaultusrinfo;


void SeisPacketInfo::initClass()
{
    defaultusrinfo = getUsrInfo();
}

class SeisEnum
{
public:

    typedef Seis::SelType SelType;
	    mDeclareEnumUtils(SelType)
    typedef Seis::GeomType GeomType;
	    mDeclareEnumUtils(GeomType)
    typedef Seis::WaveType WaveType;
	    mDeclareEnumUtils(WaveType)
    typedef Seis::DataType DataType;
	    mDeclareEnumUtils(DataType)

};

mDefineEnumUtils(SeisEnum,SelType,"Selection type")
{
	sKey::Range(),
	sKey::Table(),
	sKey::Polygon(),
	0
};
template<>
void EnumDefImpl<SeisEnum::SelType>::init()
{
    uistrings_ += uiStrings::sRange();
    uistrings_ += uiStrings::sTable();
    uistrings_ += uiStrings::sPolygon();
}

mDefineEnumUtils(SeisEnum,GeomType,"Geometry type")
{
	"2D Line",
	"Line 2D Pre-Stack",
	"3D Volume",
	"Pre-Stack Volume",
	0
};
template<>
void EnumDefImpl<SeisEnum::GeomType>::init()
{
    uistrings_ += uiStrings::s2DLine();
    uistrings_ += mEnumTr("Line 2D Pre-Stack",0);
    uistrings_ += mEnumTr("3D Volume",0);
    uistrings_ += mEnumTr("Pre-Stack Volume",0);
}

mDefineEnumUtils(SeisEnum,WaveType,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};
template<>
void EnumDefImpl<SeisEnum::WaveType>::init()
{
    uistrings_ += mEnumTr("P-Wave",0);
    uistrings_ += mEnumTr("Sh-PWave",0);
    uistrings_ += mEnumTr("Sv-Wave",0);
    uistrings_ += uiStrings::sOther();
}

mDefineEnumUtils(SeisEnum,DataType,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	"Azimuth",
	"Classification",
	"Other",
	0
};
template<>
void EnumDefImpl<SeisEnum::DataType>::init()
{
    uistrings_ += uiStrings::sAmplitude();
    uistrings_ += uiStrings::sDip();
    uistrings_ += uiStrings::sFrequency();
    uistrings_ += uiStrings::sPhase(false);
    uistrings_ += mEnumTr("AVO Gradient",0);
    uistrings_ += uiStrings::sAzimuth();
    uistrings_ += uiStrings::sClassification();
    uistrings_ += uiStrings::sOther();
}

const char* Seis::nameOf( SelType st )
{ return SeisEnum::toString(st); }

const char* Seis::nameOf( GeomType gt )
{ return SeisEnum::toString(gt); }

const char* Seis::nameOf( DataType dt )
{ return SeisEnum::toString(dt); }

const char* Seis::nameOf( WaveType wt )
{ return SeisEnum::toString(wt); }

Seis::SelType Seis::selTypeOf( const char* s )
{ return SeisEnum::SelTypeDef().parse(s); }

Seis::GeomType Seis::geomTypeOf( const char* s )
{ return SeisEnum::GeomTypeDef().parse(s); }

Seis::DataType Seis::dataTypeOf( const char* s )
{
    if ( sKey::Steering() == s )
	return Dip;

    return SeisEnum::DataTypeDef().parse(s);
}

Seis::WaveType Seis::waveTypeOf( const char* s )
{ return SeisEnum::WaveTypeDef().parse(s); }

const BufferStringSet& Seis::dataTypeNames()
{ return SeisEnum::DataTypeDef().keys(); }

bool Seis::isAngle( DataType dt )
{ return dt == Phase || dt == Azimuth; }

void Seis::putInPar( GeomType gt, IOPar& iop )
{
    iop.set( sKey::Geometry(), nameOf(gt) );
}

bool Seis::getFromPar( const IOPar& iop, GeomType& gt )
{
    const char* res = iop.find( sKey::Geometry() );
    if ( !res || !*res ) return false;
    gt = geomTypeOf( res );
    return true;
}


bool Seis::is2DGeom( const IOPar& iop )
{
    GeomType gt = Vol;
    getFromPar( iop, gt );
    return is2D( gt );
}

uiString Seis::dataName( GeomType tp, bool explprepost )
{
    return uiStrings::sSeisObjName( is2D(tp), is3D(tp), isPS(tp),
		    SI().survDataType() == OD::Both2DAnd3D, explprepost );
}

bool Seis::isPSGeom( const IOPar& iop )
{
    GeomType gt = Vol;
    getFromPar( iop, gt );
    return isPS( gt );
}

const char* Seis::iconIDOf( GeomType gt )
{
    switch ( gt )
    {
	case Line: return "seismicline2d";
	case LinePS: return "prestackdataset2d";
	case VolPS: return "prestackdataset";
	default: break;
    }
    return "seismiccube";
}


mDefineEnumUtils(SeisTrcInfo,Fld,"Header field") {
	"Trace number",
	"Pick position",
	"Ref/SP number",
	"X-coordinate",
	"Y-coordinate",
	"In-line",
	"Cross-line",
	"Offset",
	"Azimuth",
	0
};

template<>
void EnumDefImpl<SeisTrcInfo::Fld>::init()
{
    uistrings_ += uiStrings::sTraceNumber();
    uistrings_ += mEnumTr("Pick Position",0);
    uistrings_ += mEnumTr("Ref/SP Number",0);
    uistrings_ += uiStrings::sXcoordinate();
    uistrings_ += uiStrings::sYcoordinate();
    uistrings_ += uiStrings::sInline();
    uistrings_ += uiStrings::sCrossline();
    uistrings_ += uiStrings::sOffset();
    uistrings_ += uiStrings::sAzimuth();
}


SeisTrcInfo::SeisTrcInfo()
    : sampling_(0,defaultSampleInterval())
    , refnr_(mUdf(float)), pick_(mUdf(float))
    , offset_(0), azimuth_(0), zref_(0)
{
}


SeisTrcInfo::SeisTrcInfo( const SeisTrcInfo& oth )
{
    *this = oth;
}


SeisTrcInfo& SeisTrcInfo::operator=( const SeisTrcInfo& oth )
{
    if ( this != &oth )
    {
	sampling_ = oth.sampling_;
	trckey_ = oth.trckey_;
	coord_ = oth.coord_;
	offset_ = oth.offset_;
	azimuth_ = oth.azimuth_;
	refnr_ = oth.refnr_;
	pick_ = oth.pick_;
	zref_ = oth.zref_;
    }
    return *this;
}


void SeisPacketInfo::clear()
{
    usrinfo = defaultusrinfo;
    fullyrectandreg = false;
    cubedata = 0;
    if ( !DBM().isBad() )
    {
	SI().sampling(false).hsamp_.get( inlrg, crlrg );
	zrg = SI().zRange(false);
    }

    inlrev = crlrev = false;
}


float SeisTrcInfo::defaultSampleInterval( bool forcetime )
{
    if ( DBM().isBad() )
	return mUdf(float);

    float defsr = SI().zStep();
    if ( SI().zIsTime() || !forcetime )
	return defsr;

    defsr /= SI().zInFeet() ? 5000 : 2000; // div by velocity
    int ival = (int)(defsr * 1000 + .5);
    return ival * 0.001f;
}


double SeisTrcInfo::getValue( SeisTrcInfo::Fld fld ) const
{
    switch ( fld )
    {
    case CoordX:	return coord_.x_;
    case CoordY:	return coord_.y_;
    case BinIDInl:	return lineNr();
    case BinIDCrl:	return trcNr();
    case Offset:	return offset_;
    case Azimuth:	return azimuth_;
    case RefNr:		return refnr_;
    case Pick:		return pick_;
    default:		return trcNr();
    }
}


void SeisTrcInfo::getAxisCandidates( Seis::GeomType gt,
				     TypeSet<SeisTrcInfo::Fld>& flds )
{
    flds.erase();

    if ( Seis::is2D(gt) )
	{ flds += TrcNr; flds += RefNr; }
    else
	{ flds += BinIDInl; flds += BinIDCrl; }
    if ( Seis::isPS(gt) )
	{ flds += Offset; flds += Azimuth; }

    // Coordinates are always an option
    flds += CoordX; flds += CoordY;
}


int SeisTrcInfo::getDefaultAxisFld( Seis::GeomType gt,
				    const SeisTrcInfo* ti ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    if ( !ti )
	return isps ? Offset : (is2d ? TrcNr : BinIDCrl);

    if ( isps && !Seis::equalOffset(ti->offset_,offset_) )
	return Offset;
    if ( ti->trcNr() != trcNr() )
	return is2d ? TrcNr : BinIDCrl;
    if ( !is2d && ti->lineNr() != lineNr() )
	return BinIDInl;

    // 'normal' doesn't apply, try coordinates
    return mIsZero(ti->coord_.x_-coord_.x_,.1) ? CoordY : CoordX;
}


#define mIOIOPar(fn,fld,memb) iopar.fn( FldDef().getKey(fld), memb )

void SeisTrcInfo::getInterestingFlds( Seis::GeomType gt, IOPar& iopar ) const
{
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );

    if ( isps )
    {
	mIOIOPar( set, Offset, offset_ );
	mIOIOPar( set, Azimuth, azimuth_ );
    }

    if ( is2d )
    {
	mIOIOPar( set, TrcNr, trcNr() );
	if ( refnr_ && !mIsUdf(refnr_) )
	    mIOIOPar( set, RefNr, refnr_ );
    }
    else
    {
	mIOIOPar( set, BinIDInl, lineNr() );
	mIOIOPar( set, BinIDCrl, trcNr() );
	iopar.set( sKey::Position(), binID().toString() );
    }

    mIOIOPar( set, CoordX, coord_.x_ );
    mIOIOPar( set, CoordY, coord_.y_ );

    if ( pick_ && !mIsUdf(pick_) )
	mIOIOPar( set, Pick, pick_ );
    if ( refnr_ && !mIsUdf(refnr_) )
	mIOIOPar( set, RefNr, refnr_ );
}


void SeisTrcInfo::setPSFlds( const Coord& rcv, const Coord& src, bool setpos )
{
    offset_ = rcv.distTo<float>( src );
    azimuth_ = mCast(float, Math::Atan2( rcv.y_ - src.y_, rcv.x_ - src.x_ ) );
    if ( setpos )
    {
	coord_.x_ = .5 * (rcv.x_ + src.x_);
	coord_.y_ = .5 * (rcv.y_ + src.y_);
	setBinID( SI().transform(coord_) );
    }
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    BinID bid( binID() );
    mIOIOPar( get, TrcNr,	bid.crl() );
    mIOIOPar( get, BinIDInl,	bid.inl() );
    mIOIOPar( get, BinIDCrl,	bid.crl() );
    setBinID( bid );
    mIOIOPar( get, CoordX,	coord_.x_ );
    mIOIOPar( get, CoordY,	coord_.y_ );
    mIOIOPar( get, Offset,	offset_ );
    mIOIOPar( get, Azimuth,	azimuth_ );
    mIOIOPar( get, Pick,	pick_ );
    mIOIOPar( get, RefNr,	refnr_ );

    iopar.get( sKeySamplingInfo, sampling_.start, sampling_.step );
}


void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    mIOIOPar( set, TrcNr,	trcNr() );
    mIOIOPar( set, BinIDInl,	lineNr() );
    mIOIOPar( set, BinIDCrl,	trcNr() );
    mIOIOPar( set, CoordX,	coord_.x_ );
    mIOIOPar( set, CoordY,	coord_.y_ );
    mIOIOPar( set, Offset,	offset_ );
    mIOIOPar( set, Azimuth,	azimuth_ );
    mIOIOPar( set, Pick,	pick_ );
    mIOIOPar( set, RefNr,	refnr_ );

    iopar.set( sKeySamplingInfo, sampling_.start, sampling_.step );
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    return t > sampling_.start-1e-6 && t < samplePos(trcsz-1) + 1e-6;
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = mIsUdf(t) ? 0 : (t - sampling_.start) / sampling_.step;
    return mNINT32(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg ) const
{
    SampleGate sg;

    sg.start = sg.stop = 0;
    if ( mIsUdf(tg.start) && mIsUdf(tg.stop) )
	return sg;

    Interval<float> vals(
	mIsUdf(tg.start) ? 0 : (tg.start-sampling_.start) / sampling_.step,
	mIsUdf(tg.stop) ? 0 : (tg.stop-sampling_.start) / sampling_.step );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)Math::Floor(vals.start+1e-3);
	sg.stop =  (int)Math::Ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)Math::Ceil(vals.start-1e-3);
	sg.stop = (int)Math::Floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}


Seis::PosKey SeisTrcInfo::posKey( Seis::GeomType gt ) const
{
    switch ( gt )
    {
    case Seis::VolPS:	return Seis::PosKey( binID(), offset_ );
    case Seis::Line:	return Seis::PosKey( trcNr() );
    case Seis::LinePS:	return Seis::PosKey( trcNr(), offset_ );
    default:		return Seis::PosKey( binID() );
    }
}


void SeisTrcInfo::setPosKey( const Seis::PosKey& pk )
{
    Seis::GeomType gt = pk.geomType();
    if ( Seis::isPS(gt) )
	offset_ = pk.offset();
    if ( Seis::is2D(gt) )
	trckey_.setTrcNr( pk.trcNr() );
    else
	setBinID( pk.binID() );
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.trckey_ = trckey_;
    auxinf.startpos_ = sampling_.start;
    auxinf.coord_ = coord_;
    auxinf.offset_ = offset_;
    auxinf.azimuth_ = azimuth_;
    auxinf.pick_ = pick_;
    auxinf.refnr_ = refnr_;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    trckey_ = auxinf.trckey_;
    sampling_.start = auxinf.startpos_;
    coord_ = auxinf.coord_;
    offset_ = auxinf.offset_;
    azimuth_ = auxinf.azimuth_;
    pick_ = auxinf.pick_;
    refnr_ = auxinf.refnr_;
}


int Seis::Bounds::expectedNrTraces() const
{
    int rg0 = start(true); int rg1 = stop(true); int delta = step(true);
    int nr1 = (rg1 - rg0) / delta;
    if ( nr1 < 0 )	nr1 -= 1;
    else		nr1 += 1;

    rg0 = start(false);
    if ( mIsUdf(rg0) ) return nr1;

    rg1 = stop(false); delta = step(false);
    int nr2 = (rg1 - rg0) / delta;
    if ( nr2 < 0 )	nr2 -= 1;
    else		nr2 += 1;

    return nr1 * nr2; // Surveys with more than 2G traces ...? Nah.
}


Seis::Bounds3D::Bounds3D()
    : tkzs_(*new TrcKeyZSampling)
{
}


Seis::Bounds3D::Bounds3D( const Bounds3D& b )
    : tkzs_(*new TrcKeyZSampling(b.tkzs_))
{
}


Seis::Bounds3D::~Bounds3D()
{
    delete &tkzs_;
}


int Seis::Bounds3D::start( bool first ) const
{
    return first ? tkzs_.hsamp_.start_.inl() : tkzs_.hsamp_.start_.crl();
}


int Seis::Bounds3D::stop( bool first ) const
{
    return first ? tkzs_.hsamp_.stop_.inl() : tkzs_.hsamp_.stop_.crl();
}


int Seis::Bounds3D::step( bool first ) const
{
    return first ? tkzs_.hsamp_.step_.inl() : tkzs_.hsamp_.step_.crl();
}


StepInterval<float> Seis::Bounds3D::getZRange() const
{
    return tkzs_.zsamp_;
}


void Seis::Bounds3D::getCoordRange( Coord& mn, Coord& mx ) const
{
    mn = SI().transform(
	BinID(tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.start_.crl()) );
    Coord c = SI().transform(
	BinID(tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.start_.crl()) );
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
    c = SI().transform(
	BinID(tkzs_.hsamp_.stop_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
    c = SI().transform(
	BinID(tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.stop_.crl()) );
    if ( c.x_ < mn.x_ ) mn.x_ = c.x_; if ( c.x_ > mx.x_ ) mx.x_ = c.x_;
}


Seis::Bounds2D::Bounds2D()
{
    zrg_ = SI().zRange(false);
    nrrg_.step = 1;
    mincoord_ = SI().minCoord( false );
    maxcoord_ = SI().maxCoord( false );
}


IOObjContext* Seis::getIOObjContext( Seis::GeomType gt, bool forread )
{
    IOObjContext* ret = new IOObjContext( Seis::isPS(gt) ?
	 (Seis::is2D(gt) ? mIOObjContext(SeisPS2D) : mIOObjContext(SeisPS3D))
       : (Seis::is2D(gt) ? mIOObjContext(SeisTrc2D) : mIOObjContext(SeisTrc)) );
    ret->forread_ = forread;
    if ( gt == Seis::Vol )
    {
	ret->deftransl_ = BlocksSeisTrcTranslator::translKey();
	ret->destpolicy_ = IOObjContext::AllowShared;
    }
    else if ( gt == Seis::Line )
	ret->deftransl_ = CBVSSeisTrc2DTranslator::translKey();
    else if ( gt == Seis::VolPS )
	ret->deftransl_ = CBVSSeisPS3DTranslator::translKey();
    else
	ret->deftransl_ = CBVSSeisPS2DTranslator::translKey();

    return ret;
}
