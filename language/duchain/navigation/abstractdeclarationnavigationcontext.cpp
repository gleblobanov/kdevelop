/*
   Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "abstractdeclarationnavigationcontext.h"

#include <QtGui/QTextDocument>

#include <klocale.h>

#include "../functiondeclaration.h"
#include "../functiondefinition.h"
#include "../classfunctiondeclaration.h"
#include "../namespacealiasdeclaration.h"
#include "../forwarddeclaration.h"
#include "../types/enumeratortype.h"
#include "../types/enumerationtype.h"
#include "../types/functiontype.h"
#include "../duchainutils.h"
#include "../types/pointertype.h"
#include "../types/referencetype.h"
#include "../types/typeutils.h"
#include "../persistentsymboltable.h"

namespace KDevelop {
AbstractDeclarationNavigationContext::AbstractDeclarationNavigationContext( DeclarationPointer decl, KDevelop::TopDUContextPointer topContext, AbstractNavigationContext* previousContext)
  : AbstractNavigationContext(topContext, previousContext), m_declaration(decl), m_fullBackwardSearch(false)
{
  //Jump from definition to declaration if possible
  FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(m_declaration.data());
  if(definition && definition->declaration())
    m_declaration = DeclarationPointer(definition->declaration());
}

QString AbstractDeclarationNavigationContext::name() const
{
  if(m_declaration.data())
    return m_declaration->qualifiedIdentifier().toString();
  else
    return declarationName(m_declaration);
}

QString AbstractDeclarationNavigationContext::html(bool shorten)
{
  m_linkCount = 0;
  m_currentText  = "<html><body><p><small><small>";

  addExternalHtml(m_prefix);

  if(!m_declaration.data()) {
    m_currentText += i18n("<br /> lost declaration <br />");
    return m_currentText;
  }
  
  if( shorten && !m_declaration->comment().isEmpty() ) {
    QString comment = m_declaration->comment();
    if( comment.length() > 60 ) {
      comment.truncate(60);
      comment += "...";
    }
    comment.replace('\n', ' ');
    comment.replace("<br />", " ");
    comment.replace("<br/>", " ");
    m_currentText += commentHighlight(Qt::escape(comment)) + "   ";
  }
  
  if( m_previousContext ) {
    m_currentText += navigationHighlight("Back to ");
    makeLink( m_previousContext->name(), m_previousContext->name(), NavigationAction(m_previousContext) );
    m_currentText += "<br>";
  }

  if( !shorten ) {
    const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(m_declaration.data());
    if( function ) {
      htmlFunction();
    } else if( m_declaration->isTypeAlias() || m_declaration->kind() == Declaration::Instance ) {
      if( m_declaration->isTypeAlias() )
        m_currentText += importantHighlight("typedef ");

      if(m_declaration->type<EnumeratorType>())
        m_currentText += i18n("enumerator") + " ";

      eventuallyMakeTypeLinks( m_declaration->abstractType() );

      m_currentText += " " + nameHighlight(Qt::escape(declarationName(m_declaration))) + "<br>";
    }else{
      if( m_declaration->kind() == Declaration::Type && m_declaration->abstractType().cast<StructureType>() ) {
        htmlClass();
      }

      if(m_declaration->type<EnumerationType>()) {
        EnumerationType::Ptr enumeration = m_declaration->type<EnumerationType>();
        m_currentText += i18n("enumeration") + " " + Qt::escape(m_declaration->identifier().toString()) + "<br>";
      }

      if(m_declaration->isForwardDeclaration()) {
        ForwardDeclaration* forwardDec = static_cast<ForwardDeclaration*>(m_declaration.data());
        Declaration* resolved = forwardDec->resolve(m_topContext.data());
        if(resolved) {
          m_currentText += "(" + i18n("resolved forward-declaration") + ": ";
          makeLink(resolved->identifier().toString(), KDevelop::DeclarationPointer(resolved), NavigationAction::NavigateDeclaration );
          m_currentText += ") ";
        }else{
          m_currentText += i18n("(unresolved forward-declaration)") + " ";
          QualifiedIdentifier id = forwardDec->qualifiedIdentifier();
          uint count;
          const IndexedDeclaration* decls;
          PersistentSymbolTable::self().declarations(id, count, decls);
          bool had = false;
          for(uint a = 0; a < count; ++a) {
            if(decls[a].isValid() && !decls[a].data()->isForwardDeclaration()) {
              m_currentText += "<br />";
              makeLink(i18n("possible resolution from"), KDevelop::DeclarationPointer(decls[a].data()), NavigationAction::NavigateDeclaration);
              m_currentText += " " + decls[a].data()->url().str();
              had = true;
            }
          }
          if(had)
            m_currentText += "<br />";
        }
      }
    }

    QualifiedIdentifier identifier = m_declaration->qualifiedIdentifier();
    if( identifier.count() > 1 ) {
      if( m_declaration->context() && m_declaration->context()->owner() )
      {
        Declaration* decl = m_declaration->context()->owner();

        FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(decl);
        if(definition && definition->declaration())
          decl = definition->declaration();

        if(decl->abstractType().cast<EnumerationType>())
          m_currentText += labelHighlight(i18n("Enum: "));
        else
          m_currentText += labelHighlight(i18n("Container: "));

        makeLink( declarationName(DeclarationPointer(decl)), DeclarationPointer(decl), NavigationAction::NavigateDeclaration );
        m_currentText += " ";
      } else {
        QualifiedIdentifier parent = identifier;
        parent.pop();
        m_currentText += labelHighlight(i18n("Scope: ")) + Qt::escape(parent.toString()) + " ";
      }
    }
  }

  QString access = stringFromAccess(m_declaration);
  if( !access.isEmpty() )
    m_currentText += labelHighlight(i18n("Access: ")) + propertyHighlight(Qt::escape(access)) + " ";


  ///@todo Enumerations

  QString detailsHtml;
  QStringList details = declarationDetails(m_declaration);
  if( !details.isEmpty() ) {
    bool first = true;
    foreach( QString str, details ) {
      if( !first )
        detailsHtml += ", ";
      first = false;
      detailsHtml += propertyHighlight(str);
    }
  }

  QString kind = declarationKind(m_declaration);
  if( !kind.isEmpty() ) {
    if( !detailsHtml.isEmpty() )
      m_currentText += labelHighlight(i18n("Kind: ")) + importantHighlight(Qt::escape(kind)) + " " + detailsHtml + " ";
    else
      m_currentText += labelHighlight(i18n("Kind: ")) + importantHighlight(Qt::escape(kind)) + " ";
  } else if( !detailsHtml.isEmpty() ) {
    m_currentText += labelHighlight(i18n("Modifiers: ")) +  importantHighlight(Qt::escape(kind)) + " ";
  }

  m_currentText += "<br />";

  if(!shorten)
    htmlAdditionalNavigation();
  
  if( !shorten && !m_declaration->comment().isEmpty() ) {
    QString comment = m_declaration->comment();
    comment.replace("<br />", "\n"); //do not escape html newlines within the comment
    comment.replace("<br/>", "\n");
    comment = Qt::escape(comment);
    comment.replace("\n", "<br />"); //Replicate newlines in html
    m_currentText += commentHighlight(comment);
    m_currentText += "<br />";
  }

  if( !shorten ) {
    if(dynamic_cast<FunctionDefinition*>(m_declaration.data()))
      m_currentText += labelHighlight(i18n( "Def.: " ));
    else
      m_currentText += labelHighlight(i18n( "Decl.: " ));

    makeLink( QString("%1 :%2").arg( KUrl(m_declaration->url().str()).fileName() ).arg( m_declaration->range().textRange().start().line()+1 ), m_declaration, NavigationAction::JumpToSource );
    m_currentText += " ";
    //m_currentText += "<br />";
    if(!dynamic_cast<FunctionDefinition*>(m_declaration.data())) {
      if( FunctionDefinition* definition = FunctionDefinition::definition(m_declaration.data()) ) {
        m_currentText += labelHighlight(i18n( " Def.: " ));
        makeLink( QString("%1 :%2").arg( KUrl(definition->url().str()).fileName() ).arg( definition->range().textRange().start().line()+1 ), DeclarationPointer(definition), NavigationAction::JumpToSource );
      }
    }

    if( FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(m_declaration.data()) ) {
      if(definition->declaration()) {
        m_currentText += labelHighlight(i18n( " Decl.: " ));
        makeLink( QString("%1 :%2").arg( KUrl(definition->declaration()->url().str()).fileName() ).arg( definition->declaration()->range().textRange().start().line()+1 ), DeclarationPointer(definition->declaration()), NavigationAction::JumpToSource );
      }
    }
    
    m_currentText += " ";
    makeLink(i18n("Show uses"), "show_uses", NavigationAction(m_declaration, NavigationAction::NavigateUses));
  }
    //m_currentText += "<br />";

  addExternalHtml(m_suffix);

  m_currentText += "</small></small></p></body></html>";

  return m_currentText;
}

void AbstractDeclarationNavigationContext::htmlFunction()
{
  const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(m_declaration.data());
  Q_ASSERT(function);

  const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(m_declaration.data());
  const FunctionType::Ptr type = m_declaration->abstractType().cast<FunctionType>();
  if( !type ) {
    m_currentText += errorHighlight("Invalid type<br />");
    return;
  }
  if( !classFunDecl || !classFunDecl->isConstructor() || !classFunDecl->isDestructor() ) {
    eventuallyMakeTypeLinks( type->returnType() );
    m_currentText += ' ' + nameHighlight(Qt::escape(m_declaration->identifier().toString()));
  }

  if( type->arguments().count() == 0 )
  {
    m_currentText += "()";
  } else {
    m_currentText += "( ";
    bool first = true;

    KDevelop::DUContext* argumentContext = DUChainUtils::getArgumentContext(m_declaration.data());

    if(argumentContext) {
      int firstDefaultParam = argumentContext->localDeclarations().count() - function->defaultParametersSize();
      int currentArgNum = 0;

      foreach(Declaration* argument, argumentContext->localDeclarations()) {
        if( !first )
          m_currentText += ", ";
        first = false;

        AbstractType::Ptr argType = argument->abstractType();

        eventuallyMakeTypeLinks( argType );
        m_currentText += " " + nameHighlight(Qt::escape(argument->identifier().toString()));

        if( currentArgNum >= firstDefaultParam )
          m_currentText += " = " + Qt::escape(function->defaultParameters()[ currentArgNum - firstDefaultParam ].str());

        ++currentArgNum;
      }
    }

    m_currentText += " )";
  }
  m_currentText += "<br />";
}

void AbstractDeclarationNavigationContext::htmlAdditionalNavigation()
{
  ///Check if the function overrides or hides another one
  const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(m_declaration.data());
  if(classFunDecl) {
    
    Declaration* overridden = DUChainUtils::getOverridden(m_declaration.data());

    if(overridden) {
        m_currentText += i18n("Overrides a") + " ";
        makeLink(i18n("function"), QString("jump_to_overridden"), NavigationAction(DeclarationPointer(overridden), KDevelop::NavigationAction::NavigateDeclaration));
        m_currentText += " " + i18n("from") + " ";
        makeLink(overridden->context()->scopeIdentifier(true).toString(), QString("jump_to_overridden_container"), NavigationAction(DeclarationPointer(overridden->context()->owner()), KDevelop::NavigationAction::NavigateDeclaration));
        
        m_currentText += "<br />";
    }else{
      //Check if this declarations hides other declarations
      QList<Declaration*> decls;
      foreach(DUContext::Import import, m_declaration->context()->importedParentContexts())
        if(import.context(m_topContext.data()))
          decls += import.context(m_topContext.data())->findDeclarations(QualifiedIdentifier(m_declaration->identifier()), 
                                                SimpleCursor::invalid(), AbstractType::Ptr(), m_topContext.data(), DUContext::DontSearchInParent);
      uint num = 0;
      foreach(Declaration* decl, decls) {
        m_currentText += i18n("Hides a") + " ";
        makeLink(i18n("function"), QString("jump_to_hide_%1").arg(num), NavigationAction(DeclarationPointer(decl), KDevelop::NavigationAction::NavigateDeclaration));
        m_currentText += " " + i18n("from") + " ";
        makeLink(decl->context()->scopeIdentifier(true).toString(), QString("jump_to_hide_container_%1").arg(num), NavigationAction(DeclarationPointer(decl->context()->owner()), KDevelop::NavigationAction::NavigateDeclaration));
        
        m_currentText += "<br />";
        ++num;
      }
    }
    
    ///Show all places where this function is overridden
    if(classFunDecl->isVirtual()) {
      Declaration* classDecl = m_declaration->context()->owner();
      if(classDecl) {
        uint maxAllowedSteps = m_fullBackwardSearch ? (uint)-1 : 10;
        QList<Declaration*> overriders = DUChainUtils::getOverriders(classDecl, classFunDecl, maxAllowedSteps);
        
        if(!overriders.isEmpty()) {
          m_currentText += i18n("Overridden in") + " ";
          bool first = true;
          foreach(Declaration* overrider, overriders) {
            if(!first)
              m_currentText += ", ";
            first = false;
            
            QString name = overrider->context()->scopeIdentifier(true).toString();
            makeLink(name, name, NavigationAction(DeclarationPointer(overrider), NavigationAction::NavigateDeclaration));
          }
          m_currentText += "<br />";
        }
        if(maxAllowedSteps == 0)
          createFullBackwardSearchLink(overriders.isEmpty() ? i18n("Overriders possible, show all") : i18n("More overriders possible, show all"));
      }
    }
  }
  
  ///Show all classes that inherit this one
  uint maxAllowedSteps = m_fullBackwardSearch ? (uint)-1 : 10;
  QList<Declaration*> inheriters = DUChainUtils::getInheriters(m_declaration.data(), maxAllowedSteps);
  
  if(!inheriters.isEmpty()) {
      m_currentText += i18n("Inherited by") + " ";
      bool first = true;
      foreach(Declaration* importer, inheriters) {
        if(!first)
          m_currentText += ", ";
        first = false;
        
        QString importerName = importer->qualifiedIdentifier().toString();
        makeLink(importerName, importerName, NavigationAction(DeclarationPointer(importer), KDevelop::NavigationAction::NavigateDeclaration));
      }
      m_currentText += "<br />";
  }
  if(maxAllowedSteps == 0)
    createFullBackwardSearchLink(inheriters.isEmpty() ? i18n("Inheriters possible, show all") : i18n("More inheriters possible, show all"));
}

void AbstractDeclarationNavigationContext::createFullBackwardSearchLink(QString string)
{
  makeLink(string, "m_fullBackwardSearch=true", NavigationAction("m_fullBackwardSearch=true"));
  m_currentText += "<br />";
}

NavigationContextPointer AbstractDeclarationNavigationContext::executeKeyAction( QString key )
{
  if(key == "m_fullBackwardSearch=true") {
    m_fullBackwardSearch = true;
    clear();
  }
  return NavigationContextPointer(this);
}

void AbstractDeclarationNavigationContext::htmlClass()
{
  StructureType::Ptr klass = m_declaration->abstractType().cast<StructureType>();
  Q_ASSERT(klass);

  m_currentText += "class ";
  eventuallyMakeTypeLinks( klass.cast<AbstractType>() );
}

void AbstractDeclarationNavigationContext::htmlIdentifiedType(AbstractType::Ptr type, const IdentifiedType* idType)
{
  Q_ASSERT(type);
  Q_ASSERT(idType);

  if( idType->declaration(m_topContext.data()) ) {

    //Remove the last template-identifiers, because we create those directly
    QualifiedIdentifier id = idType->qualifiedIdentifier();
    Identifier lastId = id.last();
    id.pop();
    lastId.clearTemplateIdentifiers();
    id.push(lastId);

    //We leave out the * and & reference and pointer signs, those are added to the end
    makeLink(id.toString() , DeclarationPointer(idType->declaration(m_topContext.data())), NavigationAction::NavigateDeclaration );
  } else {
    m_currentText += Qt::escape(type->toString());
  }
}

void AbstractDeclarationNavigationContext::eventuallyMakeTypeLinks( AbstractType::Ptr type )
{
  if( !type ) {
    m_currentText += Qt::escape("<no type>");
    return;
  }
  AbstractType::Ptr target = TypeUtils::targetType( type, m_topContext.data() );
  const IdentifiedType* idType = dynamic_cast<const IdentifiedType*>( target.unsafeData() );

  if( idType ) {
    ///@todo This is C++ specific, move into subclass
    
    if(target->modifiers() & AbstractType::ConstModifier)
      m_currentText += "const ";
    
    htmlIdentifiedType(type, idType);

    PointerType::Ptr pointer = type.cast<PointerType>();
    ReferenceType::Ptr ref = type.cast<ReferenceType>();
    
    //Add reference and pointer
    while(pointer || ref) {
      if(pointer) {
        m_currentText += Qt::escape("*");
        if(pointer->modifiers() & AbstractType::ConstModifier)
          m_currentText += " const";
        ref = pointer->baseType().cast<ReferenceType>();
        pointer = pointer->baseType().cast<PointerType>();
      }
      if(ref) {
        m_currentText += Qt::escape("&");
        if(ref->modifiers() & AbstractType::ConstModifier)
          m_currentText += " const";
        pointer = ref->baseType().cast<PointerType>();
        ref = ref->baseType().cast<ReferenceType>();
      }
    }

  } else {
    m_currentText += Qt::escape(type->toString());
  }
}

DeclarationPointer AbstractDeclarationNavigationContext::declaration() const
{
  return m_declaration;
}

QString AbstractDeclarationNavigationContext::stringFromAccess(Declaration::AccessPolicy access)
{
  switch(access) {
    case Declaration::Private:
      return "private";
    case Declaration::Protected:
      return "protected";
    case Declaration::Public:
      return "public";
  }
  return "";
}

QString AbstractDeclarationNavigationContext::stringFromAccess(DeclarationPointer decl)
{
  const ClassMemberDeclaration* memberDecl = dynamic_cast<const ClassMemberDeclaration*>(decl.data());
  if( memberDecl ) {
    return stringFromAccess(memberDecl->accessPolicy());
  }
  return QString();
}

QString AbstractDeclarationNavigationContext::declarationName( DeclarationPointer decl )
{
  if( NamespaceAliasDeclaration* alias = dynamic_cast<NamespaceAliasDeclaration*>(decl.data()) ) {
    if( alias->identifier().isEmpty() )
      return "using namespace " + alias->importIdentifier().toString();
    else
      return "namespace " + alias->identifier().toString() + " = " + alias->importIdentifier().toString();
  }

  if( !decl )
    return i18nc("An unknown declaration that is unknown", "Unknown");
  else
    return decl->identifier().toString();
}

QStringList AbstractDeclarationNavigationContext::declarationDetails(DeclarationPointer decl)
{
  QStringList details;
  const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(decl.data());
  const ClassMemberDeclaration* memberDecl = dynamic_cast<const ClassMemberDeclaration*>(decl.data());
  if( memberDecl ) {
    if( memberDecl->isMutable() )
      details << "mutable";
    if( memberDecl->isRegister() )
      details << "register";
    if( memberDecl->isStatic() )
      details << "static";
    if( memberDecl->isAuto() )
      details << "auto";
    if( memberDecl->isExtern() )
      details << "extern";
    if( memberDecl->isFriend() )
      details << "friend";
  }

  if( decl->isDefinition() )
    details << "definition";

  if( memberDecl && memberDecl->isForwardDeclaration() )
    details << "forward";

  AbstractType::Ptr t(decl->abstractType());
  if( t ) {
    if( t->modifiers() & AbstractType::ConstModifier )
      details << "constant";
    if( t->modifiers() & AbstractType::VolatileModifier )
      details << "volatile";
  }
  if( function ) {

    if( function->isInline() )
      details << "inline";

    if( function->isExplicit() )
      details << "explicit";

    if( function->isVirtual() )
      details << "virtual";

    const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(decl.data());

    if( classFunDecl ) {
      if( classFunDecl->isSignal() )
        details << "signal";
      if( classFunDecl->isSlot() )
        details << "slot";
      if( classFunDecl->isConstructor() )
        details << "constructor";
      if( classFunDecl->isDestructor() )
        details << "destructor";
      if( classFunDecl->isConversionFunction() )
        details << "conversion-function";
    }
  }
  return details;
}

}
