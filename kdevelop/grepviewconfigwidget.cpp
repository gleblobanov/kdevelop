/***************************************************************************
                             grepviewconfigwidget.cpp
                             ----------------------
    copyright            : (C) 1999 by Bernd Gehrmann
    email                : bernd@physik.hu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#include <qlabel.h>
#include <qlayout.h>
#include "grepviewconfigwidget.h"


GrepViewConfigWidget::GrepViewConfigWidget(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAutoAdd(true);
    new QLabel("Test", parent);
    layout->addStretch();
}


GrepViewConfigWidget::~GrepViewConfigWidget()
{
}
