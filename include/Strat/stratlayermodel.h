#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"

#include "elasticpropsel.h"
#include "propertyref.h"
#include "stratlayersequence.h"
#include "od_iosfwd.h"


namespace Strat
{
class Layer;
class UnitRef;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.

 */

mExpClass(Strat) LayerModel
{
public:

				LayerModel();
				LayerModel( const LayerModel& lm )
							{ *this = lm; }
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const	{ return seqs_.isEmpty(); }
    bool			isValid() const;
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence( int idx )	  { return *seqs_[idx];}
    const LayerSequence&	sequence( int idx ) const { return *seqs_[idx];}
    Interval<float>		zRange() const;

    void			setEmpty();
    LayerSequence&		addSequence();
    LayerSequence&		addSequence(const LayerSequence&);
				//!< Does a match of props
    void			removeSequence(int);

    PropertyRefSelection&	propertyRefs()		{ return proprefs_; }
    const PropertyRefSelection&	propertyRefs() const	{ return proprefs_; }
    void			prepareUse() const;

    void			setElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const {return elasticpropsel_;}

    const RefTree&		refTree() const;

    bool			read(od_istream&);
    bool			write(od_ostream&,int modnr=0,
					bool mathpreserve=false) const;

    static const char*		sKeyNrSeqs()		{return "Nr Sequences";}
    static FixedString          defSVelStr()		{ return "DefaultSVel";}

protected:

    ObjectSet<LayerSequence>	seqs_;
    PropertyRefSelection	proprefs_;
    ElasticPropSelection	elasticpropsel_;

};


mExpClass(Strat) LayerModelProvider
{
public:

    virtual		~LayerModelProvider()	{}

    virtual LayerModel&	getCurrent()		= 0;
    virtual LayerModel&	getEdited(bool)		= 0;

    const LayerModel&	getCurrent() const
			{ return const_cast<LayerModelProvider*>(this)
						->getCurrent(); }
    const LayerModel&	getEdited( bool yn ) const
			{ return const_cast<LayerModelProvider*>(this)
						->getEdited(yn); }
};


}; // namespace Strat
