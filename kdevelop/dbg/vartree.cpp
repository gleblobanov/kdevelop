// **************************************************************************
//                          vartree.cpp  -  description
//                             -------------------
//    begin                : Sun Aug 8 1999
//    copyright            : (C) 1999 by John Birch
//    email                : jb.nz@writeme.com
// **************************************************************************

// **************************************************************************
// *                                                                        *
// *   This program is free software; you can redistribute it and/or modify *
// *   it under the terms of the GNU General Public License as published by *
// *   the Free Software Foundation; either version 2 of the License, or    *
// *   (at your option) any later version.                                  *
// *                                                                        *
// **************************************************************************

#include "vartree.h"
#include "gdbparser.h"

//#include <kapp.h>     // here for i18n only! yuck!
#include <kpopupmenu.h>
#include <klineedit.h>
#include <klocale.h>

#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qcursor.h>


#if defined(DBG_MONITOR)
  #define DBG_DISPLAY(X)          {emit rawData(QString(X));}
#else
  #define DBG_DISPLAY(X)          {;}
#endif

// **************************************************************************

//TODO - change to a base class parser and setup a factory
//static GDBParser* parser = 0;

//static GDBParser* getParser()
//{
//  if (!parser)
//    parser = new GDBParser;

//  return parser;
//}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarViewer::VarViewer( QWidget *parent, const char *name ) :
  QWidget( parent, name )
{
  QVBoxLayout *topLayout = new QVBoxLayout(this, 2);

  varTree_ = new VarTree(this);
  varTree_->setFocusPolicy(QWidget::NoFocus);
  topLayout->addWidget( varTree_, 10 );

  QBoxLayout *watchEntry = new QHBoxLayout();
  topLayout->addLayout( watchEntry );

  QLabel *label = new QLabel( i18n("Watch: "), this );
//  label->setMinimumSize( label->sizeHint() );
//  label->setMaximumSize( label->sizeHint() );
  watchEntry->addWidget( label );

  // make the size small so that it can fit within the parent widget
  // size. The parents size is currently 4 tabs wide with <=3chars
  // in each tab. (ie quite small!) 
  watchVarEntry_ = new KLineEdit(this);
//  watchVarEntry_->setMinimumSize(0,0); //watchVarEntry_->sizeHint() );
  watchVarEntry_->setFocusPolicy(QWidget::ClickFocus);
  watchEntry->addWidget( watchVarEntry_ );

  // just add a bit of space at the end of the entry widget
//  QLabel *blank = new QLabel( " ", this );
//  blank->setMinimumSize( blank->sizeHint() );
//  blank->setMaximumSize( blank->sizeHint() );
//  watchEntry->addWidget( blank );

  QPushButton* addButton = new QPushButton( i18n("Add"), this );
//  addButton->setMinimumSize( addButton->sizeHint() );
//  addButton->setMaximumSize( addButton->sizeHint() );
  addButton->setFocusPolicy(QWidget::NoFocus);
  watchEntry->addWidget( addButton );

  connect(addButton, SIGNAL(clicked()), SLOT(slotAddWatchVariable()));
  connect(watchVarEntry_, SIGNAL(returnPressed()), SLOT(slotAddWatchVariable()));

  topLayout->activate();
}

// **************************************************************************

void VarViewer::clear()
{
  varTree_->clear();
}

// **************************************************************************

void VarViewer::setEnabled(bool bEnabled)
{
  QWidget::setEnabled(bEnabled);
  if (bEnabled && parentWidget()) {
    varTree_->setColumnWidth(0, parentWidget()->width()/2);
  }
}

// **************************************************************************

void VarViewer::slotAddWatchVariable()
{
  QString watchVar(watchVarEntry_->text());
  if (!watchVar.isEmpty())
    varTree_->slotAddWatchVariable(watchVar);
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarTree::VarTree( QWidget *parent, const char *name ) :
  QListView(parent, name),
  activeFlag_(0),
  currentThread_(-1)
{
  setRootIsDecorated(true);
  setSorting(-1);
  setFrameStyle(Panel | Sunken);
  setLineWidth(2);
  addColumn(i18n("Variable"));
  addColumn(i18n("Value"));
  setMultiSelection (false);

  connect (this,  SIGNAL(rightButtonClicked ( QListViewItem *, const QPoint &, int )),
                  SLOT(slotRightButtonClicked ( QListViewItem *, const QPoint &, int )));

  setColumnWidthMode(0, QListView::Manual);
}

// **************************************************************************

VarTree::~VarTree()
{
}

// **************************************************************************

void VarTree::slotRightButtonClicked( QListViewItem* selectedItem,
                                      const QPoint &,
                                      int)
{
  if (!selectedItem)
    return;

  setSelected (selectedItem, true);    // Need to select this item.
  if (selectedItem->parent())
  {
    QListViewItem* item = findRoot(selectedItem);
    KPopupMenu popup(selectedItem->text(VarNameCol));
    if (dynamic_cast<WatchRoot*>(item))
    {
      popup.insertItem( i18n("Delete watch variable"), this, SLOT(slotRemoveWatchVariable()) );
    }

    popup.insertItem( i18n("Toggle watchpoint"), this, SLOT(slotToggleWatchpoint()) );
    popup.exec(QCursor::pos());
  }
}

// **************************************************************************

void VarTree::slotToggleWatchpoint()
{
  if (VarItem* item = dynamic_cast<VarItem*>(currentItem()))
    emit toggleWatchpoint(item->fullName());
}

// **************************************************************************

void VarTree::slotRemoveWatchVariable()
{
  delete currentItem();
}

// **************************************************************************

void VarTree::slotAddWatchVariable(const QString& watchVar)
{
  VarItem* varItem = new VarItem(findWatch(), watchVar, typeUnknown);
  emitExpandItem(varItem);
}

// **************************************************************************

void VarTree::emitSetLocalViewState(bool localsOn, int frameNo, int threadNo)
{
  // FIXME: Is the following true wrt THREADS

  // When they want to _close_ a frame then we need to check the state of
  // all other frames to determine whether we still need the locals.
  if (!localsOn)
  {
    QListViewItem* sibling = firstChild();
    while (sibling)
    {
      VarFrameRoot* frame = dynamic_cast<VarFrameRoot*> (sibling);
      if (frame && frame->isOpen())
      {
        localsOn = true;
        break;
      }

      sibling = sibling->nextSibling();
    }
  }

  emit setLocalViewState(localsOn);
  emit selectFrame(frameNo, threadNo);
}

// **************************************************************************

QListViewItem* VarTree::findRoot(QListViewItem* item) const
{
  while (item->parent())
    item = item->parent();

  return item;
}

// **************************************************************************

VarFrameRoot* VarTree::findFrame(int frameNo, int threadNo) const
{
  QListViewItem* sibling = firstChild();

  // frames only exist on th top level so we only need to
  // check the siblings
  while (sibling)
  {
    VarFrameRoot* frame = dynamic_cast<VarFrameRoot*> (sibling);
    if (frame && frame->matchDetails(frameNo, threadNo))
      return frame;

    sibling = sibling->nextSibling();
  }

  return 0;
}

// **************************************************************************

WatchRoot* VarTree::findWatch()
{
  QListViewItem* sibling = firstChild();

  while (sibling)
  {
    if (WatchRoot* watch = dynamic_cast<WatchRoot*> (sibling))
      return watch;

    sibling = sibling->nextSibling();
  }

  return new WatchRoot(this);
}

// **************************************************************************

void VarTree::trim()
{
  QListViewItem* child = firstChild();
  while (child)
  {
    QListViewItem* nextChild = child->nextSibling();

    // don't trim the watch root
    if (!(dynamic_cast<WatchRoot*> (child)))
    {
      if (TrimmableItem* item = dynamic_cast<TrimmableItem*> (child))
      {
        if (item->isActive())
          item->trim();
        else
          delete item;
      }
    }
    child = nextChild;
  }
}

// **************************************************************************

void VarTree::trimExcessFrames()
{
  QListViewItem* child = firstChild();
  while (child)
  {
    QListViewItem* nextChild = child->nextSibling();
    if (VarFrameRoot* frame = dynamic_cast<VarFrameRoot*> (child))
    {
      // remove all frames except the current frame
      if (!frame->matchDetails(0, currentThread_))
        delete frame;
    }

    child = nextChild;
  }
}

// **************************************************************************

QListViewItem* VarTree::lastChild() const
{
  QListViewItem* child = firstChild();
  if (child)
    while (QListViewItem* nextChild = child->nextSibling())
      child = nextChild;

  return child;
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

TrimmableItem::TrimmableItem(VarTree* parent) :
  QListViewItem (parent, parent->lastChild()),
  activeFlag_(0)
{
  setActive();
}

// **************************************************************************

TrimmableItem::TrimmableItem(TrimmableItem* parent) :
  QListViewItem (parent, parent->lastChild()),
  activeFlag_(0),
  waitingForData_(false)
{
  setActive();
}

// **************************************************************************

TrimmableItem::~TrimmableItem()
{
}

// **************************************************************************

int TrimmableItem::rootActiveFlag() const
{
  return ((VarTree*)listView())->activeFlag();
}

// **************************************************************************

bool TrimmableItem::isTrimmable() const
{
  return !waitingForData_;
}

// **************************************************************************

QListViewItem* TrimmableItem::lastChild() const
{
  QListViewItem* child = firstChild();
  if (child)
    while (QListViewItem* nextChild = child->nextSibling())
      child = nextChild;

  return child;
}

// **************************************************************************

TrimmableItem* TrimmableItem::findMatch
                (const QString& match, DataType type) const
{
  QListViewItem* child = firstChild();

  // Check the siblings on this branch
  while (child)
  {
    if (child->text(VarNameCol) == match)
    {
      if (TrimmableItem* item = dynamic_cast<TrimmableItem*> (child))
        if (item->getDataType() == type)
          return item;
    }

    child = child->nextSibling();
  }

  return 0;
}

// **************************************************************************

void TrimmableItem::trim()
{
  QListViewItem* child = firstChild();
  while (child)
  {
    QListViewItem* nextChild = child->nextSibling();
    if (TrimmableItem* item = dynamic_cast<TrimmableItem*>(child))
    {
      // Never trim a branch if we are waiting on data to arrive.
      if (isTrimmable())
      {
        if (item->isActive())
          item->trim();      // recurse
        else
          delete item;
      }
    }
    child = nextChild;
  }
}

// **************************************************************************

DataType TrimmableItem::getDataType() const
{
  return typeUnknown;
}

// **************************************************************************

void TrimmableItem::setCache(const QCString&)
{
  ASSERT(false);
}

// **************************************************************************

QCString TrimmableItem::getCache()
{
  ASSERT(false);
  return QCString();
}

// **************************************************************************

void TrimmableItem::updateValue(char* /* buf */)
{
  waitingForData_ = false;
}

// **************************************************************************

QString TrimmableItem::key (int, bool) const
{
  return QString::null;
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarItem::VarItem( TrimmableItem* parent, const QString& varName, DataType dataType) :
  TrimmableItem (parent),
  cache_(QCString()),
  dataType_(dataType),
  highlight_(false)
{
  setText (VarNameCol, varName);
}

// **************************************************************************

VarItem::~VarItem()
{
}

// **************************************************************************

QString VarItem::varPath() const
{
  QString vPath("");
  const VarItem* item = this;

  // This stops at the root item (VarFrameRoot or WatchRoot)
  while ((item = dynamic_cast<const VarItem*> (item->parent())))
  {
    if (item->getDataType() != typeArray)
    {
      if ((item->text(VarNameCol))[0] != '<')
      {
        QString itemName = item->text(VarNameCol);
        if (vPath.isEmpty())
          vPath = itemName.replace(QRegExp("^static "), "");
        else
          vPath = itemName.replace(QRegExp("^static "), "") + "." + vPath;
      }
    }
  }

  return vPath;
}

// **************************************************************************

QString VarItem::fullName() const
{
  QString itemName(getName());
  ASSERT (!itemName.isEmpty());
  QString vPath = varPath();
  if (itemName[0] == '<')
    return vPath;

  if (vPath.isEmpty())
    return itemName.replace(QRegExp("^static "), "");

  return varPath() + "." + itemName.replace(QRegExp("^static "), "");
}

// **************************************************************************

void VarItem::setText ( int column, const QString& data )
{
  if (!isActive() && isOpen() && dataType_ == typePointer)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandItem(this);
  }

  setActive();
  if (column == ValueCol)
  {
    QString oldValue(text(column));
    if (!oldValue.isEmpty())                   // Don't highlight new items
      highlight_ = (oldValue != QString(data));
  }

  QListViewItem::setText(column, data);
  repaint();
}

// **************************************************************************

void VarItem::updateValue(char* buf)
{
  TrimmableItem::updateValue(buf);

  // Hack due to my bad QString implementation - this just tidies up the display
  if ((::strncmp(buf, "There is no member named len.", 29) == 0) ||
      (::strncmp(buf, "There is no member or method named len.", 39) == 0))
  {
    return;
  }

  if (*buf == '$')
  {
    if (char* end = strchr(buf, '='))
        buf = end+2;
  }

  if (dataType_ == typeUnknown)
  {
    dataType_ = GDBParser::getGDBParser()->determineType(buf);
    if (dataType_ == typeArray)
      buf++;

    // Try fixing a format string here by overriding the dataType calculated
    // from this data
    QString varName=getName();
    if (dataType_ == typePointer && varName[0] == '/')
      dataType_ = typeValue;
  }

  GDBParser::getGDBParser()->parseData(this, buf, true, false);
  setActive();
}

// **************************************************************************

void VarItem::setCache(const QCString& value)
{
  cache_ = value;
  setExpandable(true);
  checkForRequests();
  if (isOpen())
    setOpen(true);
  setActive();
}

// **************************************************************************

void VarItem::setOpen(bool open)
{
  if (open)
  {
    if (cache_)
    {
      QCString value = cache_;
      cache_ = QCString();
      GDBParser::getGDBParser()->parseData(this, value.data(), false, false);
      trim();
    }
    else
    {
      if (dataType_ == typePointer || dataType_ == typeReference)
      {
        waitingForData();
        ((VarTree*)listView())->emitExpandItem(this);
      }
    }
  }

  QListViewItem::setOpen(open);
}

// **************************************************************************

QCString VarItem::getCache()
{
  return cache_;
}

// **************************************************************************

void VarItem::checkForRequests()
{
  // Signature for a QT2.x and QT3.x  QString
  if (cache_.find("d = 0x") == 0)      // Eeeek - too small
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
           QCString().sprintf("(($len=($data=%s.d).len)?*((char*)&$data.unicode[0])@($len>100?200:$len*2):\"\")",
           fullName().latin1()));
  }

  // Signature for a QT2.x and QT3.x  QString when print statics are "on".
  if (cache_.find("static null = {static null = <same as static member of an already seen type>, d = 0x") == 0)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
           QCString().sprintf("(($len=($data=%s.d).len)?*((char*)&$data.unicode[0])@($len>100?200:$len*2):\"\")",
           fullName().latin1()));
  }

  // Signature for a QT2.0.x QT2.1 QCString
  if (cache_.find("<QArray<char>> = {<QGArray> = {shd = ") == 0)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
                                          fullName().latin1()+QCString(".shd.data"));
  }

  // Signature for a QT2.0.x QT2.1 QDir
  // statics are broken
  if (cache_.find("dPath = {d = 0x") == 0)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
           QCString().sprintf("(($len=($data=%s.dPath.d).len)?*((char*)&$data.unicode[0])@($len>100?200:$len*2):\"\")",
           fullName().latin1()));
  }
  // Signature for a QT1.44 QString
  if (cache_.find("<QArrayT<char>> = {<QGArray> = {shd = ") == 0)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
                                          fullName().latin1()+QCString(".shd.data"));
  }

  // Signature for a QT1.44 QDir
  if (cache_.find("dPath = {<QArrayT<char>> = {<QGArray> = {shd") == 0)
  {
    waitingForData();
    ((VarTree*)listView())->emitExpandUserItem(this,
                                          fullName().latin1()+QCString(".dPath.shd.data"));
  }
}

// **************************************************************************

DataType VarItem::getDataType() const
{
  return dataType_;
}

// **************************************************************************

// Overridden to highlight the changed items
void VarItem::paintCell( QPainter * p, const QColorGroup & cg,
                                int column, int width, int align )
{
  if ( !p )
    return;

  if (column == ValueCol && highlight_)
  {
    QColorGroup hl_cg( cg.foreground(), cg.background(), cg.light(),
                        cg.dark(), cg.mid(), red, cg.base());
    QListViewItem::paintCell( p, hl_cg, column, width, align );
  }
  else
    QListViewItem::paintCell( p, cg, column, width, align );
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

VarFrameRoot::VarFrameRoot(VarTree* parent, int frameNo, int threadNo) :
  TrimmableItem (parent),
  needLocals_(true),
  frameNo_(frameNo),
  threadNo_(threadNo),
  params_(QCString()),
  locals_(QCString())
{
  setExpandable(true);
}

// **************************************************************************

VarFrameRoot::~VarFrameRoot()
{
}

// **************************************************************************

void VarFrameRoot::setLocals(char* locals)
{

  ASSERT(isActive());

  // "No symbol table info available" or "No locals."
  bool noLocals = (locals &&  (::strncmp(locals, "No ", 3) == 0));
  setExpandable(!params_.isEmpty() || !noLocals);

  if (noLocals)
  {
    locals_ = "";
    if (locals)
      if (char* end = strchr(locals, '\n'))
        *end = 0;      // clobber the new line
  }
  else
    locals_ = locals;

  if (!isExpandable() && noLocals)
    setText ( ValueCol, locals );

  needLocals_ = false;
  if (isOpen())
    setOpen(true);
}

// **************************************************************************

void VarFrameRoot::setParams(const QCString& params)
{
  setActive();
  params_ = params;
  needLocals_ = true;
}

// **************************************************************************

// Override setOpen so that we can decide what to do when we do change
// state. This
void VarFrameRoot::setOpen(bool open)
{
  bool localStateChange = (isOpen() != open);
  QListViewItem::setOpen(open);

  if (localStateChange)
    emit ((VarTree*)listView())->emitSetLocalViewState(open, frameNo_, threadNo_);

  if (!open)
    return;

  GDBParser::getGDBParser()->parseData(this, params_.data(), false, true);
  GDBParser::getGDBParser()->parseData(this, locals_.data(), false, false);

  locals_ = QCString();
  params_ = QCString();
}

// **************************************************************************

bool VarFrameRoot::matchDetails(int frameNo, int threadNo)
{
  return frameNo == frameNo_ && threadNo == threadNo_;
}

// **************************************************************************
// **************************************************************************
// **************************************************************************
// **************************************************************************

WatchRoot::WatchRoot(VarTree* parent) :
  TrimmableItem(parent)
{
  setText(0, i18n("Watch"));
  setOpen(true);
}

// **************************************************************************

WatchRoot::~WatchRoot()
{
}

// **************************************************************************

void WatchRoot::requestWatchVars()
{
  for (QListViewItem* child = firstChild(); child; child = child->nextSibling())
    if (VarItem* varItem = dynamic_cast<VarItem*>(child))
      ((VarTree*)listView())->emitExpandItem(varItem);
}

// **************************************************************************
// **************************************************************************
// **************************************************************************

#include "vartree.moc"
