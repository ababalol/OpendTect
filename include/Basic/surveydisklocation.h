#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2018
________________________________________________________________________

-*/

#include "bufstring.h"

namespace File { class Path; }


/*!\brief Survey location on disk.

  The survey itself is the only OpendTect database 'object' that has no ID by
  itself. Thus, if you have to work accross surveys, this has to be the ID
  of a survey. To make things like 'relocate' easier, the path is split in a
  base path (usualy the 'Data Root') and the survey directory name.

 */

mExpClass(Basic) SurveyDiskLocation
{
public:

			SurveyDiskLocation()	{} //!< current
			SurveyDiskLocation(const char* dirnm,const char* bp=0);
			SurveyDiskLocation(const File::Path& fulldir);
    bool		operator ==(const SurveyDiskLocation&) const;
    bool		operator !=( const SurveyDiskLocation& oth ) const
			{ return !(*this == oth); }

    BufferString	basepath_;	//!< The 'data root'
    BufferString	dirname_;	//!< The subdirectory name

    void		set(const File::Path&);
    BufferString	fullPath() const;
    BufferString	surveyName() const;

    bool		isEmpty() const;    //!< meaning current survey
    bool		isCurrentSurvey() const;
    void		setEmpty();	    //!< always current survey
    void		setCurrentSurvey(); //!< a snapshot, will remain OK
					    //!< if you change to another survey

};
