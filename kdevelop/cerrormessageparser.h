/***************************************************************************
                          cerrormessageparser.h  -  description                              
                             -------------------                                         
    begin                : Tue Mar 30 1999                                           
    copyright            : (C) 1999 by Sandy Meier
    email                : smeier@rz.uni-potsdam.de              
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef CERRORMESSAGEPARSER_H
#define CERRORMESSAGEPARSER_H

#include <qstring.h>
#include <qlist.h>

struct TErrorMessageInfo {
  QString filename;
  int errorline;
  int makeoutputline;
};

/** a small "parser" for the makeoutput
  *@author Sandy Meier
  */

class CErrorMessageParser {
public: 
  CErrorMessageParser();
  ~CErrorMessageParser();
  
  void parse(QString makeoutput,QString startdir);

  /** get the error info for a specific line from the parse "makeoutput")*/
  TErrorMessageInfo getInfo(int makeoutputline);

  /** get the next error/warning*/
  TErrorMessageInfo getNext();

  /** get the previous error/warning*/
  TErrorMessageInfo getPrev();
  /** print out the parsed informtions*/
  void out();
  
  QList<TErrorMessageInfo> m_info_list;
};


#endif
