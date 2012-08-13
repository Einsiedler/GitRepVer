#include "../inc/stdafx.h"
#include <boost/program_options.hpp>

#include <QtCore/QProcess>
#include <QtCore/QStringListIterator>
#include <QtCore/QDateTime>
#include <QtCore/QFile>

#include <string>
#include <iostream>


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm);
bool process_git_output(const QString& a_git_output, long long& r_comm_time_utc, std::string& r_comm_author, std::string& r_comm_hash, int& r_commits_count );


int main(int argc, char *argv[])
{
    namespace po = boost::program_options;

    po::variables_map vm;

    if(!extract_cl_variables(argc, argv, vm))
        return -1;

    if (!vm.count("pdir"))
    {
        std::cout << "The GIT repository's dir was not set." << std::endl;
        return -1;
    }
    if (!vm.count("in_templ"))
    {
        std::cout << "The template file was not set." << std::endl;
        return -2;
    }
    if (!vm.count("out_templ"))
    {
        std::cout << "The output header file was not set." << std::endl;
        return -3;
    }
    if (!vm.count("ver_major"))
    {
        std::cout << "The major version of a product was not set." << std::endl;
        return -4;
    }
    if (!vm.count("ver_minor"))
    {
        std::cout << "The minor version of a product was not set." << std::endl;
        return -5;
    }
    if (!vm.count("ver_release"))
    {
        std::cout << "The release version of a product was not set." << std::endl;
        return -6;
    }

    std::string wdir = vm["pdir"].as<std::string>();

    std::cout << "The GIT repository's dir is " << wdir << std::endl;

    QProcess proc;

    QStringList arguments;
    {
        arguments.push_back("log");
        //arguments.push_back("-1");
        arguments.push_back("--all");
        arguments.push_back("--date-order");
    }
    proc.setWorkingDirectory(QString::fromStdString(wdir));
    proc.start("git", arguments);

    proc.waitForFinished();

    long long   r_comm_time_utc;
    std::string r_comm_author;
    std::string r_comm_hash;

    int commits_count = 0;
    process_git_output(proc.readAllStandardOutput(), r_comm_time_utc, r_comm_author, r_comm_hash, commits_count);

    QFile template_file(QString::fromStdString(vm["in_templ"].as<std::string>()));

    if(!template_file.open(QIODevice::ReadOnly))
        return -7;

    QString template_string(template_file.readAll());
    template_file.close();

    if(template_string.isEmpty())
        return -8;

    QString out_header_file_str;

    QStringList list = template_string.split("\n", QString::SkipEmptyParts);

    foreach(const QString& line, list) 
    {
        if(line.contains("GIT_REP_COMMIT_TIMESTAMP"))
            out_header_file_str += QString("\n#define GIT_REP_COMMIT_TIMESTAMP %1 \n").arg(r_comm_time_utc);
        else if(line.contains("GIT_REP_COMMIT_AUTHOR")) 
            out_header_file_str += QString("\n#define GIT_REP_COMMIT_AUTHOR \"%1\" \n").arg(QString::fromStdString(r_comm_author));
        else if(line.contains("GIT_REP_COMMIT_HASH")) 
            out_header_file_str += QString("\n#define GIT_REP_COMMIT_HASH \"%1\" \n").arg(QString::fromStdString(r_comm_hash));
        else if(line.contains("GIT_REP_OVERALL_COMMITS"))
            out_header_file_str += QString("\n#define GIT_REP_OVERALL_COMMITS %1 \n").arg(QString::number(commits_count));
    }

    if(out_header_file_str.isEmpty())
        return -9;

    out_header_file_str = QString("#ifndef __%1_VERSION_H__\n#define __%1_VERSION_H__\n\n%2\n"
                                  "#define PROJECT_BUILD_TIMESTAMP %3\n\n"
                                  "#define PRODUCT_VERSION_MAJOR %4\n\n"
                                  "#define PRODUCT_VERSION_MINOR %5\n\n"
                                  "#define PRODUCT_VERSION_RELEASE %6\n\n"
                                  "#define PRODUCT_VERSION_STRING \"%4.%5.%6.%7\"\n\n"
                                  "#endif\n")
                                 .arg(QString::fromStdString(r_comm_author).toUpper())
                                 .arg(out_header_file_str)
                                 .arg(QDateTime::currentMSecsSinceEpoch())
                                 .arg(vm["ver_major"].as<int>()).arg(vm["ver_minor"].as<int>()).arg(vm["ver_release"].as<int>()).arg(QString::number(commits_count));

    QFile out_headr_file(QString::fromStdString(vm["out_templ"].as<std::string>()));
    if(!out_headr_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return -10;

    out_headr_file.write(out_header_file_str.toUtf8());
    out_headr_file.close();

    return 0;
}


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm)
{
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help"       , "produce help message")
        ("pdir"       , po::value<std::string>(), "A project's directory of the GDK's repository.")
        ("in_templ"   , po::value<std::string>(), "A path to the template file.")
        ("out_templ"  , po::value<std::string>(), "A path to the output header file.")
        ("ver_major"  , po::value<int>(), "A major number of version.")
        ("ver_minor"  , po::value<int>(), "A minor number of version.")
        ("ver_release", po::value<int>(), "A release number of version.")
        ;

    po::store (po::parse_command_line(argc, argv, desc), r_vm);
    po::notify(r_vm);    

    if (r_vm.count("help"))
    {
        std::cout << desc << std::endl;

        return false;
    }

    return true;
}



bool process_git_output( const QString& a_git_output
                       , long long&     r_comm_time_utc
                       , std::string&   r_comm_author
                       , std::string&   r_comm_hash
                       , int&           r_commits_count)
{
    if(a_git_output.isEmpty())
        return false;
    
    r_comm_time_utc = 0;
    r_comm_author   = "undetermined";
    r_comm_hash     = "undetermined";
    r_commits_count = 0;

    QStringList list = a_git_output.split("\n");

    const int items_limit   = 2;
    int       items_count   = -1; 

    foreach (const QString &str, list) 
    {
        if((items_limit > items_count) && str.contains("Date:"))
        { // [2] |Date:   Fri Jul 20 18:31:04 2012 +0300
            QDateTime dt = QDateTime::fromString( str.mid(strlen("Date:"), (str.length() - strlen("Date:") - strlen("+0000"))).simplified(), "ddd MMM dd hh:mm:ss yyyy");
            /* // Code for testing a parser.
            int i_day_of_week = dt.date().dayOfWeek();
            int i_day         = dt.date().day();
            int i_month       = dt.date().month();
            int i_year        = dt.date().year();
            */
            r_comm_time_utc = dt.toMSecsSinceEpoch();

            ++items_count;
        }
        else if((items_limit > items_count) && str.contains("Author:"))
        { // [1] |Author: einsiedler <a.bondarenko@codetiburon.com>
            r_comm_author = str.mid(strlen("Author:"), str.indexOf("<") - strlen("Author:")).simplified().toStdString();

            ++items_count;
        }
        else if(str.contains("commit "))
        { // [0] |commit 43f19cdfb9c09e4ea144ebf957f2500634489f9e
            ++r_commits_count;

            if(items_limit > items_count)
            {
                r_comm_hash = str.mid(strlen("commit ")).simplified().toStdString();

                ++items_count;
            }
        }
    }

    return true;
}
