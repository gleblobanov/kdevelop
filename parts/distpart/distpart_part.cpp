#include "distpart_part.h"

#include <qwhatsthis.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kgenericfactory.h>

#include "kdevcore.h"

#include "distpart_widget.h"
#include "specsupport.h"

typedef KGenericFactory<DistpartPart> DistpartFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevdistpart, DistpartFactory( "kdevdistpart" ) );

DistpartPart::DistpartPart(QObject *parent, const char *name, const QStringList &)
        : KDevPlugin(parent, name) {
    setInstance(DistpartFactory::instance());

    setXMLFile("kdevpart_distpart.rc");

    m_action =  new KAction( i18n("Project Distribution and Publishing"), "package", 0,
                             this, SLOT(show()),
                             actionCollection(), "make_dist" );

    m_action->setStatusText(i18n("Make Source and Binary Distribution"));
    m_action->setWhatsThis(i18n("Distribution and Publishing:\n\n"
                                "fnork fnork blub.... \n"
                                "bork bork bork....."));
    //QWhatsThis::add(m_widget, i18n("This will help users package and publish their software."));

    m_dialog = new DistpartDialog(this);
    
    // set up package
    //KURL projectURL;  // we need to get this from the base project
    thePackage = new SpecSupport(this);
    //thePackage->loadFile(projectURL);
}


DistpartPart::~DistpartPart() {
    delete m_dialog;
    delete thePackage;
}

DistpartDialog* DistpartPart::getDlg() {
    return m_dialog;
}

void DistpartPart::show() {
    m_dialog->show();
}

void DistpartPart::hide() {
    m_dialog->hide();
}
#include "distpart_part.moc"
