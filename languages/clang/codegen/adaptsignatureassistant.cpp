/*
   Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>
   Copyright 2014 Kevin Funk <kfunk@kde.org>

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

#include "adaptsignatureassistant.h"

#include <interfaces/icore.h>
#include <interfaces/ilanguagecontroller.h>
#include <language/assistant/renameaction.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/functiondefinition.h>
#include <language/duchain/classfunctiondeclaration.h>
#include <language/duchain/types/functiontype.h>

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include "../util/clangdebug.h"

using namespace KDevelop;

namespace {
Declaration *getDeclarationAtCursor(const KTextEditor::Cursor &cursor, const QUrl &documentUrl)
{
    ENSURE_CHAIN_READ_LOCKED
    ReferencedTopDUContext top(DUChainUtils::standardContextForUrl(documentUrl));
    if (!top) {
        clangDebug() << "no context found for document" << documentUrl;
        return nullptr;
    }
    const auto *context = top->findContextAt(top->transformToLocalRevision(cursor), true);
    return context->type() == DUContext::Function ? context->owner() : nullptr;
}

bool isConstructor(const Declaration *functionDecl)
{
    auto classFun = dynamic_cast<const ClassFunctionDeclaration*>(functionDecl);
    return classFun && classFun->isConstructor();
}

Signature getDeclarationSignature(const Declaration *functionDecl, const DUContext *functionCtxt, bool includeDefaults)
{
    ENSURE_CHAIN_READ_LOCKED
    int pos = 0;
    Signature signature;
    const AbstractFunctionDeclaration* abstractFunDecl = dynamic_cast<const AbstractFunctionDeclaration*>(functionDecl);
    foreach(Declaration * parameter, functionCtxt->localDeclarations()) {
        signature.defaultParams << (includeDefaults ? abstractFunDecl->defaultParameterForArgument(pos).str() : QString());
        signature.parameters << qMakePair(parameter->indexedType(), parameter->identifier().identifier().str());
        ++pos;
    }
    signature.isConst = functionDecl->abstractType() && functionDecl->abstractType()->modifiers() & AbstractType::ConstModifier;

    if (!isConstructor(functionDecl)) {
        if (auto funType = functionDecl->type<FunctionType>()) {
            signature.returnType = IndexedType(funType->returnType());
        }
    }
    return signature;
}
}

AdaptSignatureAssistant::AdaptSignatureAssistant(ILanguageSupport* supportedLanguage)
    : StaticAssistant(supportedLanguage)
{
}

QString AdaptSignatureAssistant::title() const
{
    return i18n("Adapt Signature");
}

void AdaptSignatureAssistant::reset()
{
    doHide();
    clearActions();

    m_editingDefinition = {};
    m_declarationName = {};
    m_otherSideId = DeclarationId();
    m_otherSideTopContext = {};
    m_otherSideContext = {};
    m_oldSignature = {};
    m_document = nullptr;
    m_view.clear();
}

void AdaptSignatureAssistant::textChanged(KTextEditor::Document* doc, const KTextEditor::Range& invocationRange, const QString& removedText)
{
    reset();

    m_document = doc;
    m_lastEditPosition = invocationRange.end();

    KTextEditor::Range sigAssistRange = invocationRange;
    if (!removedText.isEmpty()) {
        sigAssistRange.setRange(sigAssistRange.start(), sigAssistRange.start());
    }

    DUChainReadLocker lock(DUChain::lock(), 300);
    if (!lock.locked()) {
        clangDebug() << "failed to lock duchain in time";
        return;
    }
    KTextEditor::Range simpleInvocationRange = KTextEditor::Range(sigAssistRange);
    Declaration* funDecl = getDeclarationAtCursor(simpleInvocationRange.start(), m_document->url());
    if (!funDecl || !funDecl->type<FunctionType>()) {
        clangDebug() << "No function at cursor";
        return;
    }
    /*
       TODO: Port?
       if(QtFunctionDeclaration* classFun = dynamic_cast<QtFunctionDeclaration*>(funDecl)) {
       if (classFun->isSignal()) {
        // do not offer to change signature of a signal, as the implementation will be generated by moc
        return;
       }
       }
     */

    Declaration* otherSide = 0;
    FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(funDecl);
    if (definition) {
        m_editingDefinition = true;
        otherSide = definition->declaration();
    } else if ((definition = FunctionDefinition::definition(funDecl))) {
        m_editingDefinition = false;
        otherSide = definition;
    }
    if (!otherSide) {
        clangDebug() << "no other side for signature found";
        return;
    }
    m_otherSideContext = DUContextPointer(DUChainUtils::getFunctionContext(otherSide));
    if (!m_otherSideContext) {
        clangDebug() << "no context for other side found";
        return;
    }
    m_declarationName = funDecl->identifier();
    m_otherSideId = otherSide->id();
    m_otherSideTopContext = ReferencedTopDUContext(otherSide->topContext());
    m_oldSignature = getDeclarationSignature(otherSide, m_otherSideContext.data(), true);

    //Schedule an update, to make sure the ranges match
    DUChain::self()->updateContextForUrl(m_otherSideTopContext->url(), TopDUContext::AllDeclarationsAndContexts);
}

bool AdaptSignatureAssistant::isUseful() const
{
    return !m_declarationName.isEmpty() && m_otherSideId.isValid() && !actions().isEmpty();
}

bool AdaptSignatureAssistant::getSignatureChanges(const Signature& newSignature, QList<int>& oldPositions) const
{
    bool changed = false;
    for (int i = 0; i < newSignature.parameters.size(); ++i) {
        oldPositions.append(-1);
    }

    for (int curNewParam = newSignature.parameters.size() - 1; curNewParam >= 0; --curNewParam) {
        int foundAt = -1;

        for (int curOldParam = m_oldSignature.parameters.size() - 1; curOldParam >= 0; --curOldParam) {
            if (newSignature.parameters[curNewParam].first != m_oldSignature.parameters[curOldParam].first) {
                continue;  //Different type == different parameters
            }
            if (newSignature.parameters[curNewParam].second == m_oldSignature.parameters[curOldParam].second || curOldParam == curNewParam) {
                //given the same type and either the same position or the same name, it's (probably) the same argument
                foundAt = curOldParam;

                if (newSignature.parameters[curNewParam].second != m_oldSignature.parameters[curOldParam].second || curOldParam != curNewParam) {
                    changed = true;  //Either the name changed at this position, or position of this name has changed
                }
                if (newSignature.parameters[curNewParam].second == m_oldSignature.parameters[curOldParam].second) {
                    break;  //Found an argument with the same name and type, no need to look further
                }
                //else: position/type match, but name match will trump, allowing: (int i=0, int j=1) => (int j=1, int i=0)
            }
        }

        if (foundAt < 0) {
            changed = true;
        }
        oldPositions[curNewParam] = foundAt;
    }

    if (newSignature.parameters.size() != m_oldSignature.parameters.size()) {
        changed = true;
    }
    if (newSignature.isConst != m_oldSignature.isConst) {
        changed = true;
    }
    if (newSignature.returnType != m_oldSignature.returnType) {
        changed = true;
    }
    return changed;
}

void AdaptSignatureAssistant::setDefaultParams(Signature& newSignature, const QList<int>& oldPositions) const
{
    bool hadDefaultParam = false;
    for (int i = 0; i < newSignature.defaultParams.size(); ++i) {
        const auto oldPos = oldPositions[i];
        if (oldPos == -1) {
            // default-initialize new argument if we encountered a previous default param
            if (hadDefaultParam) {
                newSignature.defaultParams[i] = QStringLiteral("{} /* TODO */");
            }
        } else {
            newSignature.defaultParams[i] = m_oldSignature.defaultParams[oldPos];
            hadDefaultParam = hadDefaultParam || !newSignature.defaultParams[i].isEmpty();
        }
    }
}

QList<RenameAction*> AdaptSignatureAssistant::getRenameActions(const Signature &newSignature, const QList<int> &oldPositions) const
{
    ENSURE_CHAIN_READ_LOCKED
    QList<RenameAction*> renameActions;
    if (!m_otherSideContext) {
        return renameActions;
    }
    for (int i = newSignature.parameters.size() - 1; i >= 0; --i) {
        if (oldPositions[i] == -1) {
            continue;  //new parameter
        }
        Declaration *renamedDecl = m_otherSideContext->localDeclarations()[oldPositions[i]];
        if (newSignature.parameters[i].second != m_oldSignature.parameters[oldPositions[i]].second) {
            QMap<IndexedString, QList<RangeInRevision> > uses = renamedDecl->uses();
            if (!uses.isEmpty()) {
                renameActions << new RenameAction(renamedDecl->identifier(), newSignature.parameters[i].second,
                                                  RevisionedFileRanges::convert(uses));
            }
        }
    }

    return renameActions;
}

void AdaptSignatureAssistant::updateReady(const KDevelop::IndexedString& document, const KDevelop::ReferencedTopDUContext& top)
{
    if (!top || !m_document || document.toUrl() != m_document->url() || top->url() != IndexedString(m_document->url())) {
        return;
    }
    clearActions();

    DUChainReadLocker lock;

    Declaration *functionDecl = getDeclarationAtCursor(m_lastEditPosition, m_document->url());
    if (!functionDecl || functionDecl->identifier() != m_declarationName) {
        clangDebug() << "No function found at" << m_document->url() << m_lastEditPosition;
        return;
    }
    DUContext *functionCtxt = DUChainUtils::getFunctionContext(functionDecl);
    if (!functionCtxt) {
        clangDebug() << "No function context found for" << functionDecl->toString();
        return;
    }
#if 0 // TODO: Port
    if (QtFunctionDeclaration * classFun = dynamic_cast<QtFunctionDeclaration*>(functionDecl)) {
        if (classFun->isSignal()) {
            // do not offer to change signature of a signal, as the implementation will be generated by moc
            return;
        }
    }
#endif

    //ParseJob having finished, get the signature that was modified
    Signature newSignature = getDeclarationSignature(functionDecl, functionCtxt, false);

    //Check for changes between m_oldSignature and newSignature, use oldPositions to store old<->new param index mapping
    QList<int> oldPositions;
    if (!getSignatureChanges(newSignature, oldPositions)) {
        reset();
        clangDebug() << "no changes to signature";
        return; //No changes to signature
    }
    QList<RenameAction*> renameActions;
    if (m_editingDefinition) {
        setDefaultParams(newSignature, oldPositions); //restore default parameters before updating the declarations
    } else {
        renameActions = getRenameActions(newSignature, oldPositions);  //rename as needed when updating the definition
    }
    IAssistantAction::Ptr action(new AdaptSignatureAction(m_otherSideId, m_otherSideTopContext,
                                                          m_oldSignature, newSignature,
                                                          m_editingDefinition, renameActions));
    connect(action.data(), &IAssistantAction::executed,
            this, &AdaptSignatureAssistant::reset);
    addAction(action);
    emit actionsChanged();
}

KTextEditor::Range AdaptSignatureAssistant::displayRange() const
{
    if (!m_document) {
        return {};
    }

    auto s = m_lastEditPosition;
    KTextEditor::Range ran = {s.line(), 0, s.line(), m_document->lineLength(s.line())};
    qDebug() << "display range:" << ran;
    return ran;
}

#include "moc_adaptsignatureassistant.cpp"
