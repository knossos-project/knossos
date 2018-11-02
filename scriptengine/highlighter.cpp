/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "highlighter.h"


Highlighter::Highlighter(QTextDocument *document) :
    QSyntaxHighlighter(document)
{
    Rule rule;

    QTextCharFormat keywordFormat;
    keywordFormat.setFontWeight(QFont::Bold);
    keywordFormat.setForeground(Qt::red);

    // http://docs.python.org/2/reference/lexical_analysis.html#keywords
    QStringList keywordPattern;
    keywordPattern  << "\\band\\b"      << "\\bdel\\b"      << "\\bfrom\\b"
                    << "\\bnot\\b"      << "\\bwhile\\b"    << "\\bas\\b"
                    << "\\belif\\b"     << "\\bglobal\\b"   << "\\bor\\b"
                    << "\\bwith\\b"     << "\\bassert\\b"   << "\\belse\\b"
                    << "\\bif\\b"       << "\\bpass\\b"     << "\\byield\\b"
                    << "\\bbreak\\b"    << "\\bexcept\\b"   << "\\bimport\\b"
                    << "\\bprint\\b"    << "\\bclass\\b"    << "\\bexec\\b"
                    << "\\bin\\b"       << "\\braise\\b"    << "\\bcontinue\\b"
                    << "\\bfinally\\b"  << "\\bis\\b"       << "\\breturn\\b"
                    << "\\bdef\\b"      << "\\bfor\\b"      << "\\blambda\\b"
                    << "\\btry\\b";

    foreach(const QString &pattern, keywordPattern) {
       rule.pattern = QRegExp(pattern);
       rule.format  = keywordFormat;
       rules.append(rule);
    }

    QTextCharFormat numberFormat;
    numberFormat.setFontWeight(QFont::Light);
    numberFormat.setForeground(Qt::red);

    QStringList numberPattern;
    numberPattern << "\\b[0-9]+\\b";

    foreach(const QString &pattern, numberPattern) {
       rule.pattern = QRegExp(pattern);
       rule.format  = numberFormat;
       rules.append(rule);
    }

    QTextCharFormat knossosClasses;
    knossosClasses.setFontWeight(QFont::Bold);
    knossosClasses.setForeground(Qt::black);

    QStringList knossosPattern;
    knossosPattern << "\\bTreeListElement\\b" << "\\bNodeListElement\\b" << "\\bSegmentListElement"
                   << "\\bColor\\b" << "\\bskeleton\\b" << "\\bknossos\\b";
    foreach(const QString &pattern, knossosPattern) {
       rule.pattern = QRegExp(pattern);
       rule.format  = knossosClasses;
       rules.append(rule);
    }


}

void Highlighter::highlightBlock(const QString &text) {

    foreach(const Rule rule, rules) {
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
}
