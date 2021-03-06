/*****************************************************************************
 * web.cpp - QStarDict, a StarDict clone written with using Qt               *
 * Copyright (C) 2008 Alexander Rodin                                        *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License along   *
 * with this program; if not, write to the Free Software Foundation, Inc.,   *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.               *
 *****************************************************************************/

#include "web.h"

#include <QBuffer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QUrl>
#include <QTextCodec>
#include "settingsdialog.h"
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#include "web-meta.h"
#endif


Web::Web(QObject *parent)
    : QObject(parent)
{
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
QStarDict::PluginMetadata Web::metadata() const
{
    QStarDict::PluginMetadata md;
    md.id = PLUGIN_ID;
    md.name = QString::fromUtf8(PLUGIN_NAME);
    md.version = PLUGIN_VERSION;
    md.description = PLUGIN_DESCRIPTION;
    md.authors = QString::fromUtf8(PLUGIN_AUTHORS).split(';', QString::SkipEmptyParts);
    md.features = QString::fromLatin1(PLUGIN_FEATURES).split(';', QString::SkipEmptyParts);
    md.icon = QIcon(":/icons/web.png");
    return md;
}
#else
QIcon Web::pluginIcon() const
{
    return QIcon(":/icons/web.png");
}
#endif

QStringList Web::availableDicts() const
{
    QStringList result = QDir(qsd->configDir(PLUGIN_ID)).entryList(QStringList("*.webdict"), QDir::Files, QDir::Name);
    result.replaceInStrings(".webdict", "");
    return result;
}

void Web::setLoadedDicts(const QStringList &dicts)
{
    for (QStringList::const_iterator i = dicts.begin(); i != dicts.end(); ++i)
    {
        QString filename = qsd->configDir(PLUGIN_ID) + "/" + *i + ".webdict";
        if (! QFile::exists(filename))
            continue;
        QSettings dict(filename, QSettings::IniFormat);
        QString query = dict.value("query").toString();
        if (! query.isEmpty())
        {
            m_loadedDicts[*i].query = query;
            m_loadedDicts[*i].codec = dict.value("charset").toByteArray();
        }
    }
}

Web::DictInfo Web::dictInfo(const QString &dict)
{
    QString filename = qsd->configDir(PLUGIN_ID) + "/" + dict + ".webdict";
    if (! QFile::exists(filename))
        return DictInfo();
    QSettings dictFile(filename, QSettings::IniFormat);
    DictInfo info(PLUGIN_ID, dict,
            dictFile.value("author").toString(),
            dictFile.value("description").toString());
    return info;
}

bool Web::isTranslatable(const QString &dict, const QString &word)
{
    if (! m_loadedDicts.contains(dict))
        return false;
    // TODO
    Q_UNUSED(word);
    return true;
}

Web::Translation Web::translate(const QString &dict, const QString &word)
{
    if (! m_loadedDicts.contains(dict))
        return Translation();
    QUrl url(m_loadedDicts[dict].query.replace("%s", word));
    QEventLoop loop;
    QNetworkAccessManager qnam;
    QNetworkReply *reply = qnam.get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QTextCodec *codec = QTextCodec::codecForName(m_loadedDicts[dict].codec);
    QString translation;
    if (codec)
        translation = codec->toUnicode(reply->readAll());
    else
        translation = QString::fromUtf8(reply->readAll());
    return Translation(dict, word, translation);
}

int Web::execSettingsDialog(QWidget *parent)
{
    ::SettingsDialog dialog(this, parent);
    return dialog.exec();
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(web, Web)
#endif

// vim: tabstop=4 softtabstop=4 shiftwidth=4 expandtab cindent textwidth=120 formatoptions=tc

