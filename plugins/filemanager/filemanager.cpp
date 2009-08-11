/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo <adymo@kdevelop.org>               *
 *   Copyright 2006 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "filemanager.h"

#include <QDir>
#include <QLayout>
#include <QAbstractItemView>

#include <kurl.h>
#include <kurlnavigator.h>
#include <kfileplacesmodel.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kdiroperator.h>
#include <kfileitem.h>
#include <klineedit.h>

#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>

#include "kdevfilemanagerplugin.h"

FileManager::FileManager(KDevFileManagerPlugin *plugin, QWidget* parent)
    :QWidget(parent)
{
    Q_UNUSED( plugin );
    setObjectName("FileManager");
    setWindowIcon(SmallIcon("system-file-manager"));
    setWindowTitle(i18n("Filesystem"));
    setWhatsThis(i18n("Filesystem Browser"));

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->setSpacing(0);
    KFilePlacesModel* model = new KFilePlacesModel( this );
    urlnav = new KUrlNavigator(model, KUrl( QDir::homePath() ), this );
    connect(urlnav, SIGNAL(urlChanged(const KUrl& )), SLOT(gotoUrl(const KUrl&)));
    l->addWidget(urlnav);
    dirop = new KDirOperator(QDir::homePath(), this);
    dirop->setView( KFile::Tree );
    dirop->setupMenu( KDirOperator::SortActions | KDirOperator::FileActions | KDirOperator::NavActions | KDirOperator::ViewActions );
    connect(dirop, SIGNAL(urlEntered(const KUrl&)), SLOT(updateNav(const KUrl&)));
    //KDirOperator emits fileSelected() twice because both activated() and doubleClicked() emit fileClicked().
    //activated() should be enough, so disconnect doubleClicked()
    disconnect(dirop->view(), SIGNAL(doubleClicked(const QModelIndex&)),
            dirop, SLOT(_k_slotDoubleClicked(const QModelIndex&)));
    l->addWidget(dirop);

    connect( dirop, SIGNAL(fileSelected(const KFileItem&)), this, SLOT(openFile(const KFileItem&)) );

    setupActions();
}

void FileManager::openFile(const KFileItem& file)
{
    KDevelop::ICore::self()->documentController()->openDocument( file.url() );
}


void FileManager::gotoUrl( const KUrl& url ) 
{
     dirop->setUrl( url, true );
}

void FileManager::updateNav( const KUrl& url )
{
    urlnav->setUrl( url );
}

void FileManager::setupActions()
{
    KAction* action = new KAction(this);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    action->setText(i18n("Current Document Directory"));
    action->setIcon(KIcon("dirsync"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(syncCurrentDocumentDirectory()));
    tbActions << (dirop->actionCollection()->action("back"));
    tbActions << (dirop->actionCollection()->action("up"));
    tbActions << (dirop->actionCollection()->action("home"));
    tbActions << (dirop->actionCollection()->action("forward"));
    tbActions << (dirop->actionCollection()->action("reload"));
    tbActions << action;
    tbActions << (dirop->actionCollection()->action("sorting menu"));
    tbActions << (dirop->actionCollection()->action("show hidden"));

}

void FileManager::syncCurrentDocumentDirectory()
{
    if( KDevelop::IDocument* activeDoc = 
                    KDevelop::ICore::self()->documentController()->activeDocument() )
        updateNav( activeDoc->url().upUrl() );
}

QList<QAction*> FileManager::toolBarActions() const
{
    return tbActions;
}

#include "filemanager.moc"
