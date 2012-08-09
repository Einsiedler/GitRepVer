#include "../inc/stdafx.h"
#include <boost/program_options.hpp>

#include <QtCore/QProcess>
#include <QtCore/QStringListIterator>
#include <QtCore/QDateTime>

#include <string>
#include <iostream>


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm);
bool process_git_output  (const QString& a_git_output);

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

    std::string wdir = vm["pdir"].as<std::string>();

    std::cout << "The GIT repository's dir is " << wdir << std::endl;

    QProcess proc;

    QStringList arguments;
    {
        arguments.push_back("log");
        arguments.push_back("-1");
        arguments.push_back("--all");
        arguments.push_back("--date-order");
    }
    proc.setWorkingDirectory(QString::fromStdString(wdir));
    proc.start("git", arguments);

    proc.waitForFinished();

    process_git_output(proc.readAllStandardOutput());

    return 0;
}


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm)
{
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("pdir", po::value<std::string>(), "A project's directory of the GDK's repository.")
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



bool process_git_output( const QString& a_git_output )
{
    if(a_git_output.isEmpty())
        return false;
    
    long long   comm_time_utc = 0;
    std::string comm_author   = "undetermined";
    std::string comm_hash     = "undetermined";

    QStringList list = a_git_output.split("\n");
    foreach (const QString &str, list) 
    {
        if(str.contains("Date:"))
        { // [2] |Date:   Fri Jul 20 18:31:04 2012 +0300
            QDateTime dt = QDateTime::fromString( str.mid(strlen("Date:"), (str.length() - strlen("Date:") - strlen("+0000"))).simplified(), "ddd MMM dd hh:mm:ss yyyy");
            /* // Code for testing a parser.
            int i_day_of_week = dt.date().dayOfWeek();
            int i_day         = dt.date().day();
            int i_month       = dt.date().month();
            int i_year        = dt.date().year();
            */
            comm_time_utc = dt.toMSecsSinceEpoch();
        }
        else if(str.contains("Author:"))
        { // [1] |Author: einsiedler <a.bondarenko@codetiburon.com>
            comm_author = str.mid(strlen("Author:"), str.indexOf("<") - strlen("Author:")).simplified().toStdString();
        }
        else if(str.contains("commit "))
        { // [0] |commit 43f19cdfb9c09e4ea144ebf957f2500634489f9e
            comm_hash = str.mid(strlen("commit ")).simplified().toStdString();
        }
    }

    return true;
}
