#ifndef cbvswriter_h
#define cbvswriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswriter.h,v 1.15 2002-07-25 21:48:44 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <iostream>

template <class T> class DataInterpreter;
class DataBuffer;


/*!\brief Writer for CBVS format

Works on an ostream that will be deleted on destruction, or when finished.

For the inline/xline info, you have two choices:
1) if you know you have a fully rectangular and regular survey, you can
   set this in the SurvGeom.
2) if this is not the case, or you don't know whether this will be the case,
   you will have to provide the BinID in the PosAuxInfo.

*/

class CBVSWriter : public CBVSIO
{
public:

			CBVSWriter(ostream*,const CBVSInfo&,
				   const PosAuxInfo* =0);
			//!< If info.posauxinfo has a true, the PosAuxInfo
			//!< is mandatory. The relevant field(s) should then be
			//!< filled before the first put() of any position
			CBVSWriter(ostream*,const CBVSWriter&,const CBVSInfo&);
			//!< For usage in CBVS pack
			~CBVSWriter();

    unsigned long	byteThreshold() const	{ return thrbytes_; }		
			//!< The default is unlimited
    void		setByteThreshold( unsigned long n )
						{ thrbytes_ = n; }		

    int			put(void**,int offs=0);
			//!< Expects a buffer for each component
			//!< returns -1 = error, 0 = OK,
			//!< 1=not written (threshold reached)
    void		close()			{ doClose(true); }
			//!< has no effect (but doesn't hurt) if put() returns 1
    const CBVSInfo::SurvGeom& survGeom() const	{ return survgeom; }
    const PosAuxInfoSelection& auxInfoSel()	{ return auxinfosel; }

protected:

    ostream&		strm_;
    unsigned long	thrbytes_;
    ObjectSet<DataBuffer> dbufs;
    int			bytesperwrite;
    int			auxnrbytes;
    bool		rectnreg;
    int*		nrbytespersample_;

    void		writeHdr(const CBVSInfo&);
    void		putAuxInfoSel(unsigned char*) const;
    void		writeComps(const CBVSInfo&);
    void		writeGeom();
    void		doClose(bool);

    void		getRealGeometry();
    bool		writeTrailer();


private:

    streampos		geomfo; //!< file offset of geometry data
    streampos		newblockfo; //!< file offset for next block write
    int			trcswritten;
    BinID		prevbinid_;
    bool		finishing_inline;
    bool		nrtrcsperposn_known;

    int			nrtrcsperposn;
    PosAuxInfoSelection	auxinfosel;
    CBVSInfo::SurvGeom	survgeom;

    const PosAuxInfo*	auxinfo;
    ObjectSet<CBVSInfo::SurvGeom::InlineInfo>	inldata;

    void		init(const CBVSInfo&);
    void		getBinID();
    void		newSeg(bool);
    bool		writeAuxInfo();

};


#endif
