#ifndef KROSSIDENTIFIER_H
#define KROSSIDENTIFIER_H

#include<QtCore/QVariant>

//This is file has been generated by xmltokross, you should not edit this file but the files used to generate it.

namespace KDevelop { class IndexedIdentifier; }
namespace KDevelop { class IndexedQualifiedIdentifier; }
namespace KDevelop { class IndexedTypeIdentifier; }
namespace KDevelop { class Identifier; }
namespace KDevelop { class QualifiedIdentifier; }
namespace KDevelop { class TypeIdentifier; }
namespace Handlers
{
	QVariant _kDevelopTypeIdentifierHandler(void* type);
	QVariant kDevelopTypeIdentifierHandler(KDevelop::TypeIdentifier* type);
	QVariant kDevelopTypeIdentifierHandler(const KDevelop::TypeIdentifier* type);

	QVariant _kDevelopQualifiedIdentifierHandler(void* type);
	QVariant kDevelopQualifiedIdentifierHandler(KDevelop::QualifiedIdentifier* type);
	QVariant kDevelopQualifiedIdentifierHandler(const KDevelop::QualifiedIdentifier* type);

	QVariant _kDevelopIdentifierHandler(void* type);
	QVariant kDevelopIdentifierHandler(KDevelop::Identifier* type);
	QVariant kDevelopIdentifierHandler(const KDevelop::Identifier* type);

	QVariant _kDevelopIndexedTypeIdentifierHandler(void* type);
	QVariant kDevelopIndexedTypeIdentifierHandler(KDevelop::IndexedTypeIdentifier* type);
	QVariant kDevelopIndexedTypeIdentifierHandler(const KDevelop::IndexedTypeIdentifier* type);

	QVariant _kDevelopIndexedQualifiedIdentifierHandler(void* type);
	QVariant kDevelopIndexedQualifiedIdentifierHandler(KDevelop::IndexedQualifiedIdentifier* type);
	QVariant kDevelopIndexedQualifiedIdentifierHandler(const KDevelop::IndexedQualifiedIdentifier* type);

	QVariant _kDevelopIndexedIdentifierHandler(void* type);
	QVariant kDevelopIndexedIdentifierHandler(KDevelop::IndexedIdentifier* type);
	QVariant kDevelopIndexedIdentifierHandler(const KDevelop::IndexedIdentifier* type);

}

#endif
