/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2005 by Sascha Cunz                                     *
 *   sascha@kdevelop.org                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QHash>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <kparts/componentfactory.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kmacroexpander.h>
#include <ktempfile.h>
#include <ktempdir.h>
#include <kfileitem.h>
#include <kio/chmodjob.h>
#include <kiconloader.h>
#include <karchive.h>
#include <ktar.h>
#include <kstandarddirs.h>

#include "kdevappfrontend.h"
#include "filetemplate.h"

#include "kdevapptemplate.h"
#include "appwizardfactory.h"
#include "misc.h"

inline void expandMacros( QString& str, const QHash<QString,QString>& hash )
{
    QString s( KMacroExpander::expandMacros( str, hash ) );
    str = s;
}

KDevAppItem::KDevAppItem( const QString &name, KDevItemGroup *parent )
        : KDevItemCollection( name, parent )
{}

KDevAppItem::~KDevAppItem()
{}


KDevAppGroup::KDevAppGroup(const QString &name, const QString path, KDevItemGroup *parent)
    : KDevAppItem(name, parent), m_path( path )
{
    m_icon = SmallIcon( "folder" );
}

KDevAppTemplate::KDevAppTemplate( KConfig* config, const QString& rootDir, KDevAppGroup* parent )
        : KDevAppItem("root", parent), m_config( config  ), m_basePath( rootDir )
{
    m_haveLoadedDetails = false;

    // do load only things that we need for the GUI
    // load every other thing from delayedLoadDetails

    m_name = m_config->readEntry("Name");
    m_iconName = m_config->readEntry("Icon");
    m_icon = SmallIcon( "kdevelop" );
    m_comment = m_config->readEntry("Comment");
    m_fileTemplates = m_config->readEntry("FileTemplates");
    m_sourceArchive = m_config->readEntry("Archive");
    m_openFilesAfterGeneration = m_config->readListEntry("ShowFilesAfterGeneration");

    // Grab includes list
    QStringList groups = m_config->groupList();
    groups.remove("General");
    foreach( QString group, groups )
    {
        config->setGroup( group );
        if( config->readEntry("Type").lower() == "include" )
        {
            QString include( m_config->readEntry( "File" ) );
            // kDebug(9010) << "Adding: " << include << endl;
            QHash<QString,QString> hash;
            hash["kdevelop"] = rootDir;
            expandMacros( include, hash );
            if( !include.isEmpty() )
            {
                KConfig tmpCfg( include );
                tmpCfg.copyTo( "", m_config);
                // kDebug(9010) << "Merging: " << tmpCfg.name() << endl;
            }
        }
    }

    groups = m_config->groupList();    // may be changed by the merging above
    groups.remove("General");
    foreach( QString group, groups )
    {
        m_config->setGroup( group );
        QString type = m_config->readEntry("Type").lower();
        if( type == "value" )  // Add value
        {
            QString name = m_config->readEntry( "Value" );
            QString label = m_config->readEntry( "Comment" );
            QString type = m_config->readEntry( "ValueType", "String" );
            QVariant::Type variantType = QVariant::nameToType( type.latin1());
            QVariant value = m_config->readPropertyEntry( "Default", variantType );
            value.cast( variantType );  // fix this in kdelibs...
            //if( !name.isEmpty() && !label.isEmpty() )
                //info->propValues->addProperty( new PropertyLib::Property( (int)variantType, name, label, value ) );
        }
        else if( type == "ui")
        {
            m_customUI = m_config->readPathEntry("File");
        }
    }
}

KDevAppTemplate::~KDevAppTemplate()
{
    delete m_config;
}

void KDevAppTemplate::delayedLoadDetails()
{
    if( m_haveLoadedDetails )
        return;

    QStringList groups = m_config->groupList();
    groups.remove("General");
    foreach( QString group, groups )
    {
        m_config->setGroup( group );
        QString type = m_config->readEntry("Type").lower();
        if( type == "install" ) // copy dir
        {
            File file;
            file.source = m_config->readPathEntry("Source");
            file.dest = m_config->readPathEntry("Dest");
            file.process = m_config->readBoolEntry("Process",true);
            file.isXML = m_config->readBoolEntry("EscapeXML",false);
            file.option = m_config->readEntry("Option");
            addFile( file );
        }
        else if( type == "install archive" )
        {
            Archive arch;
            arch.source = m_config->readPathEntry("Source");
            arch.dest = m_config->readPathEntry("Dest");
            arch.process = m_config->readBoolEntry("Process",true);
            arch.option = m_config->readEntry("Option", "" );
            m_archList.append( arch );
        }
        else if( type == "mkdir" )
        {
            Dir dir;
            dir.dir = m_config->readPathEntry("Dir");
            dir.option = m_config->readEntry("Option", "" );
            dir.perms = m_config->readNumEntry("Perms", 0777 );
            m_dirList.append( dir );
        }
        else if( type == "finishcmd" )
        {
            m_finishCmd = m_config->readPathEntry("Command");
            m_finishCmdDir = m_config->readPathEntry("Directory");
        }
        else if( type == "message" )
        {
            m_message = m_config->readEntry( "Comment" );
        }
    }

    m_haveLoadedDetails = true;

    // no need to store the config any further
    delete m_config;
    m_config = 0;
}

void KDevAppTemplate::addDir( Dir& dir )
{
    m_dirList.append( dir );
}

void KDevAppTemplate::addFile( File file )
{
    kDebug(9010) << "Adding file: " << file.dest << endl;
    m_fileList.append( file );
    foreach( File f, m_fileList )
        kDebug(9010) << f.source << "/" << f.dest << endl;
}

void KDevAppTemplate::expandLists()
{
    kDebug(9010) << "KDevAppTemplate::expandLists()" << endl;

    QList<File>::iterator fit( m_fileList.begin() );
    for( ; fit != m_fileList.end(); ++fit )
        fit->expand( m_subMap );

    QList<Archive>::iterator ait( m_archList.begin() );
    for( ; ait != m_archList.end(); ++ait )
        ait->expand( m_subMap );

    QList<Dir>::iterator dit( m_dirList.begin() );
    for( ; dit != m_dirList.end(); ++dit )
        dit->expand( m_subMap );
}

void KDevAppTemplate::File::expand( QHash<QString, QString> hash )
{
    //kDebug(9010) << "File::expand1" << endl;
    expandMacros( source, hash );
    //kDebug(9010) << "File::expand2" << endl;
    expandMacros( dest, hash );
}

void KDevAppTemplate::Archive::expand( QHash<QString, QString> hash )
{
    expandMacros( source, hash );
    expandMacros( dest, hash );
}

void KDevAppTemplate::Dir::expand( QHash<QString, QString> hash )
{
    expandMacros( dir, hash );
}

void KDevAppTemplate::setSubMapXML()
{
    m_subMapXML = FileTemplate::normalSubstMapToXML( m_subMap );
}

bool KDevAppTemplate::unpackTemplateArchive()
{
    QString archiveName = m_basePath + "/" + m_sourceArchive;

    // Unpack template archive to temp dir, and get the name
    KTempDir archDir;
    archDir.setAutoDelete(true);
    KTar templateArchive( archiveName, "application/x-gzip" );
    if( templateArchive.open( QIODevice::ReadOnly ) )
    {
    //    unpackArchive(templateArchive.directory(), archDir.name(), false);
    }
    else
    {
        kDebug(9010) << "After KTar::open fail" << endl;
        KMessageBox::sorry(0/**@todo*/, i18n("The template %1 cannot be opened.", archiveName ) );
        templateArchive.close();
        return false;
    }
    templateArchive.close();

    addToSubMap( "src", archDir.name() );

    return true;
}

bool KDevAppTemplate::execFinishCommand( AppWizardPart* part )
{
    if( m_finishCmd.isEmpty())
        return true;

    KDevAppFrontend *appFrontend = part->extension<KDevAppFrontend>("KDevelop/AppFrontend");
    if( !appFrontend )
        return false;

    QString finishCmdDir = KMacroExpander::expandMacros(m_finishCmdDir, m_subMap);
    QString finishCmd = KMacroExpander::expandMacros(m_finishCmd, m_subMap);
    appFrontend->startAppCommand(finishCmdDir, finishCmd, false);

    return true;
}

bool KDevAppTemplate::installProject( QWidget* parentWidget )
{
    // Create dirs
    foreach( Dir dir, m_dirList )
    {
        kDebug( 9000 ) << "Process dir " << dir.dir  << endl;
        if( m_subMap[dir.option] != "false" )
        {
            if( !KIO::NetAccess::mkdir( dir.dir, parentWidget ) )
            {
                KMessageBox::sorry(parentWidget, i18n(
                                   "The directory %1 cannot be created.", 
                                    dir.dir ) );
                return false;
            }
        }
    }

    // Unpack archives
    foreach( Archive arch, m_archList )
        if( m_subMap[arch.option] != "false" )
        {
            kDebug( 9010 ) << "unpacking archive " << arch.source << endl;
            KTar archive( arch.source, "application/x-gzip" );
            if( archive.open( QIODevice::ReadOnly ) )
            {
                arch.unpack( archive.directory() );
            }
            else
            {
                KMessageBox::sorry(parentWidget, i18n("The archive %1 cannot be opened.", arch.source) );
                archive.close();
                return false;
            }
            archive.close();
        }

    // Copy files & Process
    foreach( File file, m_fileList )
    {
        kDebug( 9010 ) << "Process file " << file.source << endl;
        if( m_subMap[file.option] != "false" )
        {
            if( !file.copy(this) )
            {
                KMessageBox::sorry(parentWidget, i18n("The file %1 cannot be created.", file.dest) );
                return false;
            }
            file.setPermissions();
        }
    }
    return true;
}

bool KDevAppTemplate::File::copy( KDevAppTemplate* templ )
{
    kDebug( 9010 ) << "Copy: " << source << " to " << dest << endl;

    if( !process )  // Do a simple copy operation
        return KIO::NetAccess::copy( source, dest, 0 );

    // Process the file and save it at the destFile location
    QFile inputFile( source);
    QFile outputFile( dest );

    const QHash<QString,QString> &subMap = isXML ? templ->subMapXML() : templ->subMap();
    if( inputFile.open( QIODevice::ReadOnly ) && outputFile.open(QIODevice::WriteOnly) )
    {
        QTextStream input( &inputFile );
        QTextStream output( &outputFile );
        while( !input.atEnd() )
            output << KMacroExpander::expandMacros(input.readLine(), subMap) << "\n";
        // Preserve file mode...
        struct stat fmode;
        ::fstat( inputFile.handle(), &fmode);
        ::fchmod( outputFile.handle(), fmode.st_mode );
    }
    else
    {
        inputFile.close();
        outputFile.close();
        return false;
    }
    return true;
}

void KDevAppTemplate::File::setPermissions() const
{
    kDebug(9010) << "KDevAppTemplate::File::setPermissions()" << endl;
    kDebug(9010) << "  dest: " << dest << endl;

    KIO::UDSEntry sourceentry;
    KUrl sourceurl = KUrl::fromPathOrURL(source);
    if( KIO::NetAccess::stat(sourceurl, sourceentry, 0) )
    {
        KFileItem sourceit(sourceentry, sourceurl);
        int sourcemode = sourceit.permissions();
        if( sourcemode & 00100 )
        {
            kDebug(9010) << "source is executable" << endl;
            KIO::UDSEntry entry;
            KUrl kurl = KUrl::fromPathOrURL(dest);
            if( KIO::NetAccess::stat(kurl, entry, 0) )
            {
                KFileItem it(entry, kurl);
                int mode = it.permissions();
                kDebug(9010) << "stat shows permissions: " << mode << endl;
                KIO::chmod(KUrl::fromPathOrURL(dest), mode | 00100 );
            }
        }
    }
}

void KDevAppTemplate::Archive::unpack( const KArchiveDirectory *dir )
{
/*
    KIO::NetAccess::mkdir( dest , this );
    kDebug(9010) << "Dir : " << dir->name() << " at " << dest << endl;
    QStringList entries = dir->entries();
    kDebug(9010) << "Entries : " << entries.join(",") << endl;

    KTempDir tdir;

    QStringList::Iterator entry = entries.begin();
    for( ; entry != entries.end(); ++entry )
    {

        if( dir->entry( (*entry) )->isDirectory()  )
        {
            const KArchiveDirectory *file = (KArchiveDirectory *)dir->entry( (*entry) );
            unpackArchive( file , dest + "/" + file->name(), process);
        }
        else if( dir->entry( (*entry) )->isFile()  )
        {
            const KArchiveFile *file = (KArchiveFile *) dir->entry( (*entry) );
            if( !process )
            {
                file->copyTo( dest );
                setPermissions(file, dest + "/" + file->name());
            }
            else
            {
                file->copyTo(tdir.name());
                // assume that an archive does not contain XML files
                // ( where should we currently get that info from? )
                if ( !copyFile( QDir::cleanPath(tdir.name()+"/"+file->name()), dest + "/" + file->name(), false, process ) )
                {
                    KMessageBox::sorry(this, i18n("The file %1 cannot be created.", dest) );
                    return;
                }
                setPermissions(file, dest + "/" + file->name());
            }
        }
    }
    tdir.unlink();
*/
}
/*
void KDevAppTemplate::Archive::setPermissions(const KArchiveFile *source, QString dest)
{
    kDebug(9010) << "KDevAppTemplate::Archive::setPermissions(const KArchiveFile *source, QString dest)" << endl;
    kDebug(9010) << "  dest: " << dest << endl;

    if( source->permissions() & 00100 )
    {
        kDebug(9010) << "source is executable" << endl;
        KIO::UDSEntry entry;
        KUrl kurl = KUrl::fromPathOrURL(dest);
        if( KIO::NetAccess::stat(kurl, entry, 0) )
        {
            KFileItem it(entry, kurl);
            int mode = it.permissions();
            kDebug(9010) << "stat shows permissions: " << mode << endl;
            KIO::chmod(KUrl::fromPathOrURL(dest), mode | 00100 );
        }
    }
}
*/
KDevAppTemplateModel::KDevAppTemplateModel(QObject *parent)
        : KDevItemModel(parent), folderIcon( SmallIcon( "folder" ) )
{
    KConfig *config = KGlobal::config();
    config->setGroup("General Options");

    KStandardDirs *dirs = AppWizardFactory::instance()->dirs();
    QStringList templateNames = dirs->findAllResources("apptemplates", QString::null, false, true);

    kDebug(9010) << "Templates: " << endl;
    foreach( QString templateName, templateNames )
    {
        QString templateFile = KGlobal::dirs()->findResource("apptemplates", templateName);
        kDebug(9010) << templateName << " in " << templateFile << endl;
        KConfig* templateConfig = new KConfig( templateFile );
        templateConfig->setGroup("General");

        QString category = templateConfig->readEntry("Category");
        if( category.endsWith('/') )
            category.remove(category.length()-1, 1);
        if( category.startsWith('/') )
            category.remove(1,1);

        QString basePath( AppWizardUtil::kdevRoot( templateFile ) );
        KDevAppGroup* group = getCategory( category );
        appendItem( new KDevAppTemplate( templateConfig, basePath, group ), group );
    }
}

KDevAppGroup* KDevAppTemplateModel::getCategory( const QString& path )
{
    KDevItemCollection *curCollection = root();
    QStringList list = QStringList::split("/",path);
    QString curPath;
    foreach( QString current, list )
    {
        curPath += current;
        bool found = false;
        int i = 0;
        while( i < curCollection->itemCount() && !found )
        {
            KDevAppGroup* curGroup = reinterpret_cast<KDevAppItem*>(curCollection->itemAt(i++))->groupItem();
            if( curGroup && path.lower().startsWith( curGroup->path().lower() ) )
            {
                curCollection = curGroup;
                found = true;
            }
        }
        if( !found )
        {
            KDevItemCollection* prevCollection = curCollection;
            curCollection = new KDevAppGroup(current, curPath, curCollection);
            appendItem( curCollection, prevCollection );
        }
        curPath += '/';
    }
    return reinterpret_cast<KDevAppItem*>(curCollection)->groupItem();
}

int KDevAppTemplateModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
