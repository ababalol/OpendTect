/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uigraphicsitem.cc,v 1.43 2012-09-13 12:43:01 cvsbert Exp $";


#include "uigraphicsitem.h"
#include "uigraphicsscene.h"

#include "uicursor.h"
#include "uimain.h"
#include "bufstringset.h"

#include "draw.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QPen>
#include <QTransform>

static const int cNoFillType = 0;
static const int cDotsFillType = 1;
static const int cLinesFillType = 2;

mUseQtnamespace


void uiGraphicsItem::getFillPatternTypes( BufferStringSet& res )
{
    res.add( "No Fill" );
    res.add( "Dots" );
    res.add( "Lines" );
}


void uiGraphicsItem::getFillPatternOpts( int fp, BufferStringSet& res )
{
    res.setEmpty();
    if ( fp == cDotsFillType )
    {
	res.add( "Uniform color" );
	res.add( "Extremely dense" );
	res.add( "Very dense" );
	res.add( "Somewhat dense" );
	res.add( "Half dense" );
	res.add( "Somewhat sparse" );
	res.add( "Very sparse" );
	res.add( "Extremely sparse" );
    }
    else if ( fp == cLinesFillType )
    {
	res.add( "Horizontal lines" );
	res.add( "Vertical lines" );
	res.add( "Crossing horizontal and vertical lines" );
	res.add( "Backward diagonal lines" );
	res.add( "Forward diagonal lines" );
	res.add( "Crossing diagonal lines" );
    }
    // else none
}


uiGraphicsItem::uiGraphicsItem( QGraphicsItem* itm )
    : qgraphicsitem_(itm)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
    , translation_( 0, 0 )
    , scale_( 1, 1 )
    , angle_( 0 )
{
}


uiGraphicsItem::~uiGraphicsItem()
{
    if ( scene_ )
    {
	scene_->removeItem( this );
	delete qgraphicsitem_;
    }
}


void uiGraphicsItem::setScene( uiGraphicsScene* scene )
{ scene_ = scene; }


int uiGraphicsItem::getNewID()
{
    static Threads::Mutex mutex;
    Threads::MutexLocker lock( mutex );
    static int curid = 1;
    return curid++;
}


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }

bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }

void uiGraphicsItem::setVisible( bool yn )
{ qgraphicsitem_->setVisible( yn ); }


uiPoint uiGraphicsItem::getPos() const
{
    return uiPoint( mNINT32(qgraphicsitem_->pos().x()),
		    mNINT32(qgraphicsitem_->pos().y()) );
}


uiRect uiGraphicsItem::boundingRect() const
{
    QRectF qr( qgraphicsitem_->sceneBoundingRect() );
    return uiRect( mNINT32(qr.left()), mNINT32(qr.top()),
	    	   mNINT32(qr.right()), mNINT32(qr.bottom()) );
}


void uiGraphicsItem::setPos( float x, float y )
{ stPos( x, y ); }


void uiGraphicsItem::setPos( const uiPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const uiWorldPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const Geom::Point2D<float>& p )
{ stPos(p.x, p.y); }



void uiGraphicsItem::stPos( float x, float y )
{
    translation_.x = x;
    translation_.y = y;

    updateTransform();
}


void uiGraphicsItem::moveBy( float x, float y )
{
    translation_.x += x;
    translation_.y += y;
    updateTransform();
}

void uiGraphicsItem::setRotation( float angle )
{
    angle_ = angle;
    updateTransform();
}


void uiGraphicsItem::setScale( float sx, float sy )
{
    scale_.x = sx;
    scale_.y = sy;

    updateTransform();
}


void uiGraphicsItem::updateTransform()
{
    QTransform transform;
    transform.translate( translation_.x, translation_.y );
    transform.scale( scale_.x, scale_.y );
    transform.rotate( angle_ );

    qgraphicsitem_->setTransform( transform );
}

/*
 * void uiGraphicsItem::scaleAroundXY( float sx, float sy, int x, int y )
{
    qgraphicsitem_->setTransform( QTransform().translate(x,y)
	   			  .scale(sx,sy).translate(-x,-y) );
}
*/

void uiGraphicsItem::setZValue( int zval )
{ qgraphicsitem_->setZValue( zval ); }


uiPoint uiGraphicsItem::transformToScenePos( const uiPoint& pt ) const
{
    QPointF qpt = qgraphicsitem_->mapToScene( pt.x, pt.y );
    return uiPoint( mNINT32(qpt.x()),  mNINT32( qpt.y()) );
}


void uiGraphicsItem::setItemIgnoresTransformations( bool yn )
{ qgraphicsitem_->setFlag( QGraphicsItem::ItemIgnoresTransformations, yn ); }

void uiGraphicsItem::setPenColor( const Color& col, bool withalpha )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor(QRgb(col.rgb()));
    if ( withalpha ) color.setAlpha( col.t() );

    QPen qpen( color );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
}


bool uiGraphicsItem::isSelectable()
{
    return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsSelectable );
}


void uiGraphicsItem::setSelected( bool yn )
{
    selected_ = yn;
}


void uiGraphicsItem::setSelectable( bool yn )
{
    qgraphicsitem_->setFlag( QGraphicsItem::ItemIsSelectable, yn );
}


void uiGraphicsItem::setParent( uiGraphicsItem* item )
{
    qgraphicsitem_->setParentItem( item ? item->qgraphicsitem_ : 0 );
}


void uiGraphicsItem::setPenStyle( const LineStyle& ls, bool colorwithalpha )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor( QRgb(ls.color_.rgb()) );
    if ( colorwithalpha ) color.setAlpha( ls.color_.t() );
    QBrush qbrush( color );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
}


void uiGraphicsItem::setFillColor( const Color& col, bool withalpha )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor( QRgb(col.rgb()) );
    if ( withalpha ) color.setAlpha( 255 - col.t() );

    /* TODO: why doesn't this work?
	mQtclass(QBrush) qbrush = agsitm->brush();
	qbrush.setColor( color );
    */

    // instead: hack
    mQtclass(QBrush) qbrush( color );

    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setFillPattern( int typ, int opt )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    mQtclass(QBrush) qbrush = agsitm->brush();
    mQtclass(Qt::BrushStyle) qbs = Qt::NoBrush;
    if ( typ == cDotsFillType )
    {
	if ( opt < 0 || opt > 7 ) opt = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::SolidPattern)+opt);
    }
    else if ( typ == cLinesFillType )
    {
	if ( opt < 0 || opt > 8 ) opt = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::HorPattern)+opt);
    }

    qbrush.setStyle( qbs );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qgraphicsitem_->setCursor( qcursor );
}


void uiGraphicsItemSet::setZValue( int zval )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx] )
	    (*this)[idx]->setZValue( zval );
    }
}



// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup( bool owner )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(owner)
{}


void uiGraphicsItemGroup::setScene( uiGraphicsScene* scene )
{
    uiGraphicsItem::setScene( scene );
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setScene( scene );

}


uiGraphicsItemGroup::uiGraphicsItemGroup( const ObjectSet<uiGraphicsItem>& grp )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(false)
{
    ObjectSet<uiGraphicsItem>& itms =
				const_cast<ObjectSet<uiGraphicsItem>&>(grp);
    for ( int idx=0; idx<itms.size(); idx++ )
	add( itms[idx] );
}


uiGraphicsItemGroup::~uiGraphicsItemGroup()
{
    removeAll( owner_ );
    deepErase( items2bdel_ );
}


QGraphicsItem* uiGraphicsItemGroup::mkQtObj()
{
    qgraphicsitemgrp_ = new QGraphicsItemGroup;
    return qgraphicsitemgrp_;
}


void uiGraphicsItemGroup::add( uiGraphicsItem* itm )
{
    if ( !isMainThreadCurrent() )
    {
	scene_->addUpdateToQueue(
		new uiGraphicsSceneChanger(*this,*itm,false) );
    }
    else
    {
	items_ += itm;
	itm->setScene( scene_ );
	itm->setParent( this );
	qgraphicsitemgrp_->addToGroup( itm->qGraphicsItem() );
    }
}


void uiGraphicsItemGroup::remove( uiGraphicsItem* itm, bool withdelete )
{
    if ( !itm ) return;

    items_ -= itm;
    itm->setScene( 0 );
    itm->setParent( 0 );

    QGraphicsItem* qitm = itm->qGraphicsItem();
    qgraphicsitemgrp_->removeFromGroup( qitm );
    if ( withdelete )
    {
	if ( qitm && qitm->scene() )
	    qitm->scene()->removeItem( qitm );
//	delete itm; TODO: This delete leads to crash in Qt 4.4.3
	itm->setVisible( false );
	items2bdel_ += itm;
    }
}


void uiGraphicsItemGroup::removeAll( bool withdelete )
{
    while ( !items_.isEmpty() )
	remove( items_[0], withdelete );
}


void uiGraphicsItemGroup::setVisible( bool yn )
{
    isvisible_ = yn;
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setVisible( yn );
}


bool uiGraphicsItemGroup::isVisible() const
{
    return isvisible_;
}


uiRect uiGraphicsItemGroup::boundingRect() const
{
    if ( isEmpty() ) return uiRect();

    uiRect ret( getUiItem(0)->boundingRect() );
    for ( int idx=1; idx<items_.size(); idx++ )
	ret.include( items_[idx]->boundingRect() );
    return ret;
}
