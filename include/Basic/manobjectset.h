#ifndef manobjectset_h
#define manobjectset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "objectset.h"


/*!\brief ObjectSet where the objects contained are owned by this set. */

template <class T>
class ManagedObjectSet : public ObjectSet<T>
{
public:

    inline 			ManagedObjectSet(bool objs_are_arrs);
    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
    inline virtual		~ManagedObjectSet();
    inline ManagedObjectSet<T>&	operator =(const ObjectSet<T>&);
    inline ManagedObjectSet<T>&	operator =(const ManagedObjectSet<T>&);
    virtual bool		isManaged() const	{ return true; }

    inline virtual ManagedObjectSet<T>& operator -=( T* ptr );

    inline virtual void		erase();
    inline virtual void		remove(od_int64,od_int64);
    inline virtual T*		remove( int idx, bool kporder=true )
				{ return ObjectSet<T>::remove(idx,kporder); }

    inline void			setEmpty()		{ erase(); }

protected:

    bool	isarr_;

};


//ObjectSet implementation
template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( bool isarr ) : isarr_(isarr)
{}


template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ManagedObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ManagedObjectSet<T>::~ManagedObjectSet()
{ erase(); }


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =( const ObjectSet<T>& os )
{
    if ( &os != this ) deepCopy( *this, os );
    return *this;
}

template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =(
					const ManagedObjectSet<T>& os )
{
    if ( &os != this ) { isarr_ = os.isarr_; deepCopy( *this, os ); }
    return *this;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator -=( T* ptr )
{
    if ( !ptr ) return *this;
    this->vec_.erase( (void*)ptr );
    if ( isarr_ )	delete [] ptr;
    else		delete ptr;
    return *this;
}


template <class T> inline
void ManagedObjectSet<T>::erase()
{
    if ( isarr_ )	deepEraseArr( *this );
    else		deepErase( *this );
}


template <class T> inline
void ManagedObjectSet<T>::remove( od_int64 i1, od_int64 i2 )
{
    for ( int idx=i1; idx<=i2; idx++ )
    {
	if ( isarr_ )
	    delete [] (*this)[idx];
	else
	    delete (*this)[idx];
    }
    ObjectSet<T>::remove( i1, i2 );
}


#endif
