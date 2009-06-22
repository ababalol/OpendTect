#ifndef i_qlistbox_h
#define i_qlistbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.13 2009-06-22 15:57:27 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

#include <QListWidget>
#include <QObject>

//! Helper class for uiListBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListBoxBody;

protected:
			i_listMessenger( QListWidget* sender,
					 uiListBox* receiver )
			: sender_( sender )
			, receiver_( receiver )
			{
			    connect( sender,
				SIGNAL(itemDoubleClicked(QListWidgetItem*)),
				this,
				SLOT(itemDoubleClicked(QListWidgetItem*)) );

			    connect( sender,
				SIGNAL(itemClicked(QListWidgetItem*)),
				this,
				SLOT(itemClicked(QListWidgetItem*)) );

			    connect( sender, SIGNAL(itemSelectionChanged()),
				     this, SLOT(itemSelectionChanged()) );

			    connect( sender,
				 SIGNAL(itemEntered(QListWidgetItem*)),
				 this, SLOT(itemEntered(QListWidgetItem*)) );
			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		receiver_;
    QListWidget*  	sender_;


#define mTrigger( notifier, itm ) \
{ \
    BufferString msg = #notifier; \
    if ( itm ) \
    { \
	QListWidgetItem* qlwi = itm; \
	msg += " "; msg += qlwi->listWidget()->row( qlwi ); \
    } \
    const int refnr = receiver_->beginCmdRecEvent( msg ); \
    receiver_->notifier.trigger( receiver_ ); \
    receiver_->endCmdRecEvent( refnr, msg ); \
}

private slots:

void itemDoubleClicked( QListWidgetItem* itm )
{ mTrigger( doubleClicked, itm ); }


void itemClicked( QListWidgetItem* itm )
{
    if ( receiver_->buttonstate_ == OD::RightButton )
	mTrigger( rightButtonClicked, itm )
    else if ( receiver_->buttonstate_ == OD::LeftButton )
	mTrigger( leftButtonClicked, itm );
}

void itemSelectionChanged()
{
// TODO: Remove this hack when using Qt 4.3
    QList<QListWidgetItem*> selitems = sender_->selectedItems();
    if ( selitems.count() == 0 )
	sender_->setCurrentItem( 0 );
    else if ( selitems.count() == 1 )
	sender_->setCurrentItem( selitems.first() );

    mTrigger( selectionChanged, 0 );
}

void itemEntered( QListWidgetItem* itm )
{
    const int refnr = receiver_->beginCmdRecEvent( "itemEntered" );
    receiver_->endCmdRecEvent( refnr, "itemEntered" );
}

};

#endif
