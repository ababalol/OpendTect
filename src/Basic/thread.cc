/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.11 2002-09-11 06:26:47 kristofer Exp $";

#include "thread.h"
#include "callback.h"

#include "settings.h"		// Only for getNrProcessors

Threads::Mutex::Mutex()
{
#ifdef __pthread__
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mutex, &attr );
#endif
}

Threads::Mutex::~Mutex()
{
#ifdef __pthread__
    pthread_mutex_destroy( &mutex );
    pthread_mutexattr_destroy( &attr );
#endif
}

int Threads::Mutex::lock()
{ 
#ifdef __pthread__
    return pthread_mutex_lock( &mutex ); 
#endif
    return 0;
}

int Threads::Mutex::unlock()
{
#ifdef __pthread__
    return pthread_mutex_unlock( &mutex );
#endif
    return 0;
}


Threads::ConditionVar::ConditionVar()
{
#ifdef __pthread__
    pthread_condattr_init( &condattr );
    pthread_cond_init( &cond, &condattr );
#endif
}

Threads::ConditionVar::~ConditionVar()
{
#ifdef __pthread__
    pthread_cond_destroy( &cond );
    pthread_condattr_destroy( &condattr );
#endif
}


int Threads::ConditionVar::wait()
{
#ifdef __pthread__
    return pthread_cond_wait( &cond, &mutex );
#endif
    return 0;
}


int Threads::ConditionVar::signal(bool all)
{
#ifdef __pthread__
    return all 	? pthread_cond_broadcast( &cond )
		: pthread_cond_signal( &cond );
#endif
    return 0;
}


Threads::Thread::Thread( void (func)(void*) )
    	: id(0)
{
#ifdef __pthread__
    pthread_create( &id, 0, (void* (*)(void*)) func, 0 );
#endif
}


static void* thread_exec_fn( void* obj )
{
    CallBack* cbptr = reinterpret_cast<CallBack*>( obj );
    cbptr->doCall( 0 );
    return 0;
}


Threads::Thread::Thread( const CallBack& cbin )
    	: id(0)
    	, cb(cbin)
{
    if ( !cb.willCall() ) return;
#ifdef __pthread__
    pthread_create( &id, 0, thread_exec_fn, (void*)(&cb) );
#endif
}


void Threads::Thread::stop()
{
#ifdef __pthread__
    pthread_join( id, 0 );
    delete this;
#endif
}


void Threads::Thread::detach()
{
#ifdef __pthread__
    pthread_detach( id );
#endif
}


void Threads::Thread::threadExit()
{
#ifdef __pthread__
    pthread_exit( 0 );
#endif
}


#ifdef __win__
int Threads::getNrProcessors()
{
    int res = 2;
    Settings::common().get("Nr Processors", res );
    return res;
}
#else

#include <unistd.h>

int Threads::getNrProcessors()
{
    int res;
    if ( Settings::common().get("Nr Processors", res ) )
	return res;

    int maxnrproc = sysconf(_SC_CHILD_MAX);
    int nrprocessors = sysconf(_SC_NPROCESSORS_ONLN);

    if ( maxnrproc==-1 && nrprocessors==-1 ) return 2;
    else if ( maxnrproc==-1 ) return nrprocessors;
    else if ( nrprocessors==-1 ) return maxnrproc;
    return mMIN(nrprocessors,maxnrproc);
}

#endif
