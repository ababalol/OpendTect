#ifndef wellextractdata_h
#define wellextractdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: wellextractdata.h,v 1.2 2004-05-05 20:54:27 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "position.h"
#include "enums.h"

class MultiID;
class IODirEntryList;


namespace Well
{
class Info;
class Marker;

/*!\brief Collects info about all wells in store */

class InfoCollector : public ::Executor
{
public:

			InfoCollector(bool wellloginfo=true,bool markers=true);
			~InfoCollector();

    int			nextStep();
    const char*		message() const		{ return curmsg_.buf(); }
    const char*		nrDoneText() const	{ return "Wells inspected"; }
    int			nrDone() const		{ return curidx_; }
    int			totalNr() const		{ return totalnr_; }

    typedef ObjectSet<Marker>	MarkerSet;

    const ObjectSet<MultiID>&	ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
    				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
    				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return logs_; }
    				//!< If selected, same size as ids()


protected:

    ObjectSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_;
    int				totalnr_;
    int				curidx_;
    BufferString		curmsg_;
    bool			domrkrs_;
    bool			dologs_;

};

/*!\brief Collects positions along selected well tracks */

class TrackSampler : public ::Executor
{
public:

    typedef TypeSet<BinIDValue>	BinIDValueSet;
    enum HorPol		{ Med, Avg, MostFreq, Nearest };
    			DeclareEnumUtils(HorPol)
    enum VerPol		{ Corners, NearPos, AvgCorners };
    			DeclareEnumUtils(VerPol)

			TrackSampler(const BufferStringSet& ioobjids,
				     ObjectSet<BinIDValueSet>&);

    BufferString	topmrkr;
    BufferString	botmrkr;
    float		above;
    float		below;
    HorPol		horpol;
    VerPol		verpol;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    int			nrDone() const	   { return curidx; }
    int			totalNr() const	   { return ids.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids; }
    ObjectSet<BinIDValueSet>&	bivSets()		{ return bivsets; }

    static const char*	sKeyTopMrk;
    static const char*	sKeyBotMrk;
    static const char*	sKeyLimits;
    static const char*	sKeyHorSamplPol;
    static const char*	sKeyVerSamplPol;
    static const char*	sKeyDataStart;
    static const char*	sKeyDataEnd;

protected:

    const BufferStringSet&	ids;
    ObjectSet<BinIDValueSet>&	bivsets;
    int				curidx;

};


}; // namespace Well


#endif
