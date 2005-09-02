#ifndef attribprocessor_h
#define attribprocessor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprocessor.h,v 1.10 2005-09-02 14:11:33 cvshelene Exp $
________________________________________________________________________

-*/

#include "executor.h"

class CubeSampling;
class BinID;
template <class T> class Interval;

namespace Attrib
{
class DataHolder;
class Desc;
class Output;
class Provider;

class Processor : public Executor
{
public:
    			Processor(Desc&,const char* lk="");
    			~Processor();

    virtual bool	isOK() const;
    void		addOutput(Output*);

    int			nextStep();
    void		init();
    int			totalNr() const;
    int 		nrDone() const 		{ return nrdone; }
    const char*         message() const
			{ return *(const char*)errmsg ? (const char*)errmsg
						      : "Processing"; }

    void		addOutputInterest(int sel)     { outpinterest_ += sel; }
    void		setOutputIndex(int& index);
    bool		setZIntervals(TypeSet< Interval<int> >&, BinID);
    
    Notifier<Attrib::Processor>      moveonly;
                     /*!< triggered after a position is reached that requires
                          no processing, e.g. during initial buffer fills. */
    
    const char*		getAttribName(); 	
    Provider*		getProvider() 		{ return provider; }
    ObjectSet<Output>   outputs;

protected:

    Desc&		desc_;
    BufferString	lk_;
    Provider*		provider;
    int			nriter;
    int			nrdone;
    bool 		is2d_;
    TypeSet<int>	outpinterest_;
    BufferString	errmsg;

    int			outputindex_;
};


} // namespace Attrib


#endif
