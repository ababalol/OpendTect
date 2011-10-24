#ifndef httptask_h
#define httptask_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Oct 2011 
 RCS:		$Id: httptask.h,v 1.1 2011-10-24 05:24:50 cvsumesh Exp $
________________________________________________________________________

-*/

#include "executor.h"

class ODHttp;

mClass HttpTask : public Executor
{
public:
    			HttpTask(ODHttp&);
			~HttpTask();

    int			nextStep();
    virtual void	controlWork(Control);

    const char*		message() const         { return msg_; }
    od_int64		totalNr() const		{ return totalnr_; }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         nrDoneText() const      { return "Bytes done"; }

protected:

    void		progressCB(CallBacker*);
    void		doneCB(CallBacker*);

    ODHttp&		http_;

    od_int64            totalnr_;
    od_int64            nrdone_;
    BufferString        msg_;

    int                 state_;
};


#endif
