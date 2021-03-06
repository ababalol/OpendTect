/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	nageswara
 Date:		March 2018
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "segyvintageimporter.h"

#include "dbman.h"
#include "keystrs.h"
#include "repos.h"
#include "seisimporter.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survgeom.h"
#include "ui2dgeomman.h"

SEGY::Vintage::Importer::Importer( const SEGY::Vintage::Info& vntinfo,
				   const OD::String& trnalnm,
				   const Seis::GeomType gt, Seis::SelData* sd,
				   const char* attr2dnm )
	: ExecutorGroup( vntinfo.vintagenm_ )
{
    Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
    const int selidx = parset.find( vntinfo.vintagenm_ );
    Repos::IOPar* iop = parset[selidx];
    FullSpec fullspec( gt, false);
    IOObjContext* ctxt =  Seis::getIOObjContext( gt, false );
    ctxt->fixTranslator( trnalnm );
    const int size = vntinfo.filenms_.size();
    for ( int nmidx=0; nmidx<size; nmidx++ )
    {
	CtxtIOObj ctio( *ctxt );
	const BufferString fnm( vntinfo.filenms_.get( nmidx ) );
	const File::Path fp( vntinfo.fp_.pathOnly(), fnm );
	if ( Seis::is2D(gt) && attr2dnm )
	    ctio.setName( attr2dnm );
	else if ( Seis::is3D(gt) )
	    ctio.setName( fp.baseName() );

	DBM().getEntry( ctio );
	if ( !ctio.ioobj_ )
	    continue;

	iop->set( sKey::FileName(), fp.fullPath() );
	fullspec.usePar( *iop );
	IOObj* inioobj = fullspec.spec_.getIOObj( true );
	if ( !inioobj )
	    continue;

	inioobj->pars() = *iop;
	DBM().setEntry(*inioobj);

	SeisTrcWriter* writer = new SeisTrcWriter( ctio.ioobj_ );
	SeisStdImporterReader* rdr =
				new SeisStdImporterReader( *inioobj, "SEG-Y" );
	if ( sd )
	{
	    Pos::GeomID geomid = mUdfGeomID;
	    //TODOSegyImpl Haldle overwrite 2D lines
	    if ( Seis::is2D(gt) )
		geomid = Geom2DImpHandler::getGeomID( fp.baseName(), true );

	    sd->setGeomID( geomid );
	    rdr->setSelData( sd->clone() );
	    writer->setSelData( sd->clone() );
	}

	SeisImporter* imp = new SeisImporter( rdr, *writer, gt );

	seistrcwriters_.add( writer );
	add( imp );
    }
}


SEGY::Vintage::Importer::~Importer()
{
    deepErase( seistrcwriters_ );
}
