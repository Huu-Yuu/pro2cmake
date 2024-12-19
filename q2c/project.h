//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QList>
#include <QHash>
#include <QDateTime>
#include "logs.h"

class CMakeOption
{
    public:
        CMakeOption(QString name, QString description, QString __default);
        QString Name;
        QString Description;
        QString Default;
};

class Project
{
    enum QtVersion
    {
        QtVersion_Qt4,
        QtVersion_Qt5,
        QtVersion_All
    };

    enum ParserState
    {
        ParserState_LookingForKeyword,  ///< 分析器状态 查找关键字
        ParserState_FetchingData        ///< 分析器状态提取数据
    };

    public:
        Project();
        bool Load(const QString& text);
        bool ParseQmake(const QString& text);
        QString ToQmake() const;
        QString ToCmake();
        QList<CMakeOption> CMakeOptions;
        //! This is used to determine which target libraries are needed, if you specify All it means
        //! CMake中会有一个开关，让用户在构建时做出决定
        QtVersion Version;
        QString ProjectName;
        QString CMakeMinumumVersion;
    private:
        static QString FinishCut(QString text);
        // 解析标准的qmake列表，将解析结果存储在list中
        bool ParseStandardQMakeList(QList<QString> *list, const QString& line, QString text);
        // 处理简单的关键字，word为关键字，line为当前行内容
        bool ProcessSimpleKeyword(const QString& word, const QString& line);
        // 处理复杂的关键字，word为关键字，line为当前行内容，data_buffer为附加数据缓冲区
        bool ProcessComplexKeyword(const QString& word, const QString& line, const QString& data_buffer);
        // 获取CMake默认的Qt库列表
        QString GetCMakeDefaultQtLibs();
        // 获取适用于Qt4的CMake库列表
        QString GetCMakeQt4Libs();
        // 获取适用于Qt5的CMake库列表
        QString GetCMakeQt5Libs();
        // 获取CMake Qt模块列表
        QString GetCMakeQtModules();

        QList<QString> KnownSimpleKeywords; ///< 已知简单关键字
        QList<QString> KnownComplexKeywords;    ///< 已知复杂关键字
        //! 源文档中必须包含的关键字
        QList<QString> RequiredKeywords;            ///< 必填关键字
        QList<QString> RemainingRequiredKeywords;   ///< 剩余必填关键字
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> Modules;
        QList<QString> UIList;
};

#endif // PROJECT_H
