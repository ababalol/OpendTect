/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visnormals.cc,v 1.2 2003-01-07 10:26:30 kristofer Exp $";

#include "visnormals.h"

#include "trigonometry.h"
#include "thread.h"

#include "Inventor/nodes/SoNormal.h"

mCreateFactoryEntry( visBase::Normals );


visBase::Normals::Normals()
    : normals( new SoNormal )
    , mutex( *new Threads::Mutex )
{
    normals->ref();
}


visBase::Normals::~Normals()
{
    normals->unref();
    delete &mutex;
}


void visBase::Normals::setNormal( int idx, const Vector3& normal )
{
    Threads::MutexLocker lock( mutex );
    if ( idx>=normals->vector.getNum() )
	return;

    normals->vector.set1Value( idx, SbVec3f( normal.x, normal.y, normal.z ));
}


int visBase::Normals::addNormal( const Vector3& normal )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    normals->vector.set1Value( res, SbVec3f( normal.x, normal.y, normal.z ));
    return res;
}


void visBase::Normals::removeNormal(int idx)
{
    Threads::MutexLocker lock( mutex );
    if ( idx==normals->vector.getNum()-1 )
    {
	normals->vector.deleteValues( idx );
    }
    else
    {
	unusednormals += idx;
    }
}


SoNode* visBase::Normals::getData()
{ return normals; }


int  visBase::Normals::getFreeIdx()
{
    if ( unusednormals.size() )
    {
	const int res = unusednormals[unusednormals.size()-1];
	unusednormals.remove(unusednormals.size()-1);
	return res;
    }

    return normals->vector.getNum();
}
