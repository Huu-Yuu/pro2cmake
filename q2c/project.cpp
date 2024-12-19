//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "project.h"
#include "generic.h"
#include <QStringList>
#include <utility>

Project::Project()
{
    this->ProjectName = "";
    this->CMakeMinumumVersion = "VERSION 2.6";
    this->KnownSimpleKeywords << "TARGET";
    this->RequiredKeywords << "TARGET";
    this->KnownComplexKeywords << "SOURCES" << "HEADERS";
    this->Version = QtVersion_All;
    if (Configuration::only_qt4)
    {
        this->Version = QtVersion_Qt4;
    }
    else if (Configuration::only_qt5)
    {
        this->Version = QtVersion_Qt5;
    }
    this->RemainingRequiredKeywords = this->RequiredKeywords;
    this->Modules << "core";
}

QString generateCMakeOptions(QList<CMakeOption> &options)
{
    QString result;
    for (const auto& option : options)
    {
        result += "option(" + option.Name + " \"" + option.Description + "\" " + option.Default + ")\n";
    }
    return result;
}

bool Project::Load(const QString& text)
{
    this->ParseQmake(text);
    return true;
}

bool Project::ParseQmake(const QString& text)
{
    ParserState state = ParserState_LookingForKeyword;
    // 逐行处理文件
    QStringList lines = text.split("\n");
    QString data_buffer;
    QString current_word;
    QString current_line;
    for (auto &line : lines)
    {
        // 处理空格
        while (line.startsWith(" "))
            line = line.mid(1);
        // 忽略#号开头的
        if (line.startsWith("#") || line.isEmpty())
            continue;
        if (ParserState_LookingForKeyword == state)
        {
            // 寻找关键字
            QString keyword = line;
            if (keyword.contains(" "))
                keyword = keyword.mid(0, keyword.indexOf(" "));
            Logs::DebugLog("找到关键字: " + keyword);
            if (this->KnownSimpleKeywords.contains(keyword))
            {
                if (!this->ProcessSimpleKeyword(keyword, line))
                    return false;
            }
            else if (this->KnownComplexKeywords.contains(keyword))
            {
                current_word = keyword;
                current_line = line;
                data_buffer = line;
                if (line.endsWith("\\"))
                {
                    state = ParserState_FetchingData;
                }
                else
                {
                    this->ProcessComplexKeyword(keyword, current_line, data_buffer);
                }
            }
            else
            {
                Logs::DebugLog("忽略未知关键字: " + keyword);
            }
        }
        else if (ParserState_FetchingData == state)
        {
            data_buffer += "\n" + line;
            if (!line.endsWith("\\"))
            {
                state = ParserState_LookingForKeyword;
                this->ProcessComplexKeyword(current_word, current_line, data_buffer);
            }
        }
    }
    if (!this->RemainingRequiredKeywords.isEmpty())
    {
        foreach (QString word, this->RemainingRequiredKeywords)
        {
            Logs::ErrorLog("未找到所需关键字: " + word);
        }
        return false;
    }
    return true;
}

QString Project::ToQmake() const
{
    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using pro2cmake\n";
    source += "# https://github.com/Huu-Yuu/pro2cmake at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "TARGET = " + ProjectName;
    return source;
}

QString Project::ToCmake()
{
    if (this->Version == QtVersion_All)
    {
        this->CMakeOptions.append(CMakeOption("QT5BUILD", "Build using Qt5 libs", "TRUE"));
    }

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from qmake file using pro2cmake\n";
    source += "# https://github.com/Huu-Yuu/pro2cmake at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "cmake_minimum_required (" + this->CMakeMinumumVersion + ")\n";
    source += "project(" + ProjectName + ")\n";
    //! \todo Somewhere here we should generate options for CMake based on Qt version preference
    source += generateCMakeOptions(this->CMakeOptions);

    // 添加QT库
    source += this->GetCMakeDefaultQtLibs();

    // 添加头文件和源文件
    if (!this->Sources.isEmpty())
    {
        source += "set(" + this->ProjectName + "_SOURCES";
        for (const auto &src : this->Sources)
        {
            source += " \"" + src + "\"";
        }
        source += ")\n";
    }
    if (!this->Headers.isEmpty())
    {
        source += "set(" + this->ProjectName + "_HEADERS";
        for (const QString &src : this->Headers) 
        {
            source += " \"" + src + "\"";
        }

        source += ")\n";
    }
    source += "add_executable(" + this->ProjectName;
    if (!this->Sources.isEmpty())
        source += " ${" + this->ProjectName + "_SOURCES}";
    if (!this->Headers.isEmpty())
        source += " ${" + this->ProjectName + "_HEADERS}";
    source += ")\n";

    // Qt5 hack
    source += this->GetCMakeQtModules();

    return source;
}

QString Project::FinishCut(QString text)
{
    if (text.contains("\n"))
    {
        text = text.mid(0, text.indexOf("\n"));
    }
    if (text.contains(" "))
    {
        text = text.mid(0, text.indexOf(" "));
    }
    return text;
}

bool Project::ParseStandardQMakeList(QList<QString> *list, const QString& line, QString text)
{
    if (!line.contains("="))
    {
        Logs::ErrorLog("语法错误：预期 '=' 或 '+='， 这两个都没有找到");
        Logs::ErrorLog("Line: " + line);
        return false;
    }
    text = text.mid(text.indexOf("=") + 1);
//    text = text.replace("\n", " ");
//    text = text.replace("\\", " ");
    if (!line.contains("+="))
    {
        // 清理当前缓冲区
        list->clear();
    }
    else if (line.contains("-="))
    {
        QList<QString> items = text.split(" ", QString::SkipEmptyParts);
        for (const QString &rm : items)
        {
            list->removeAll(rm);
        }
        return true;
    }
//    list->append(text.split(" ", QString::SkipEmptyParts));
    list->append(text);
    return true;
}

bool Project::ProcessSimpleKeyword(const QString& word, const QString& line)
{
    if (this->RemainingRequiredKeywords.contains(word))
        this->RemainingRequiredKeywords.removeAll(word);
    if (word == "TARGET")
    {
        if (!line.contains("="))
        {
            Logs::ErrorLog("Syntax error: expected '=' not found");
            Logs::ErrorLog("Line: " + line);
            return false;
        }
        QString target_name = line.mid(line.indexOf("=") + 1);
        while (target_name.startsWith(" "))
            target_name = target_name.mid(1);
        // 删除引号
        target_name.replace("\"", "");
        // 项目名称也不以空格结尾  删除两段空格
        target_name = target_name.trimmed();
        target_name = target_name.replace(" ", "_");
        this->ProjectName = target_name;
    }
    return true;
}

bool Project::ProcessComplexKeyword(const QString& word, const QString& line, const QString& data_buffer)
{
    if (this->RemainingRequiredKeywords.contains(word))
        this->RemainingRequiredKeywords.removeAll(word);
    if (word == "SOURCES")
    {
        if (!this->ParseStandardQMakeList(&this->Sources, line, data_buffer))
            return false;
    }
    else if (word == "HEADERS")
    {
        if (!this->ParseStandardQMakeList(&this->Headers, line, data_buffer))
            return false;
    }
    return true;
}

QString Project::GetCMakeDefaultQtLibs()
{
    if (this->Version == QtVersion_Qt4)
        return this->GetCMakeQt4Libs();
    else if (this->Version == QtVersion_Qt5)
        return this->GetCMakeQt5Libs();

    QString result = "IF (QT5BUILD)\n";
    result += Generic::Indent(this->GetCMakeQt5Libs());
    /*
    QT5_WRAP_UI(project_FORMS_HEADERS ${project_FORMS})
    QT5_ADD_RESOURCES(project_RESOURCES_RCC ${project_RESOURCES})
    */
    result += Generic::Indent("QT5_WRAP_CPP(" + this->ProjectName + "_HEADERS_MOC ${HuggleLite_HEADERS})\n");
    result += "ELSE()\n";
    result += Generic::Indent(this->GetCMakeQt4Libs());
    result += "ENDIF()\n";
    return result;
}

QString Project::GetCMakeQt4Libs()
{
    QString result;
    result += "find_package(Qt4 REQUIRED)\n";
    return result;
}

QString Project::GetCMakeQt5Libs()
{
    QString result;
    QString includes;

    //! \todo Move this somewhere so it's cached between the calls
    QHash<QString, QString> Qt5ModuleCMakeNames;
    QHash<QString, QString> Qt5ModuleIncludeDir;

    Qt5ModuleCMakeNames.insert("core", "Qt5Core");
    Qt5ModuleIncludeDir.insert("core", "${Qt5Core_INCLUDE_DIRS}");
    Qt5ModuleCMakeNames.insert("gui", "Qt5Gui");
    Qt5ModuleIncludeDir.insert("gui", "${Qt5Gui_INCLUDE_DIRS}");
    Qt5ModuleCMakeNames.insert("xml", "Qt5Xml");
    Qt5ModuleIncludeDir.insert("xml", "${Qt5Xml_INCLUDE_DIRS}");
    Qt5ModuleCMakeNames.insert("widgets", "Qt5Widgets");
    Qt5ModuleIncludeDir.insert("widgets", "${Qt5Widgets_INCLUDE_DIRS}");
    Qt5ModuleCMakeNames.insert("network", "Qt5Network");
    Qt5ModuleIncludeDir.insert("network", "${Qt5Network_INCLUDE_DIRS}");
    Qt5ModuleCMakeNames.insert("multimedia", "Qt5Multimedia");
    Qt5ModuleIncludeDir.insert("multimedia", "${Qt5Multimedia_INCLUDE_DIRS}");

    for (const auto &module : Modules)
    {
        if (Qt5ModuleCMakeNames.contains(module))
            result += "find_package(" + Qt5ModuleCMakeNames[module] + " REQUIRED)\n";
        if (Qt5ModuleIncludeDir.contains(module))
            includes += " " + Qt5ModuleIncludeDir[module];
    }

    if (includes.size())
    {
        result += "set(QT_INCLUDES" + includes + ")\n";
        result += "include_directories(${QT_INCLUDES})\n";
    }
    return result;
}

QString Project::GetCMakeQtModules()
{
    if (this->Version == QtVersion_Qt4)
        return "";

    if (this->Modules.isEmpty())
        return "";

    QString modules_string = "qt5_use_modules(" + this->ProjectName;
    foreach (QString module, this->Modules)
        modules_string += " " + Generic::CapitalFirst(module);

    modules_string += ")";

    if (this->Version == QtVersion_Qt5)
    {
        return modules_string;
    }
    else
    {
        return "IF (QT5BUILD)\n" + Generic::Indent(modules_string) + "\n" + "ENDIF()\n";
    }
}

CMakeOption::CMakeOption(QString name, QString description, QString _default)
{
    this->Name = std::move(name);
    this->Description = std::move(description);
    this->Default = std::move(_default);
}
